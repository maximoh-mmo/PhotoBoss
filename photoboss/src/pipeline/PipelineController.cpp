#include "pipeline/PipelineController.h"
#include "pipeline/stages/DirectoryScanner.h"
#include "pipeline/stages/DiskReader.h"
#include "pipeline/stages/HashWorker.h"
#include "pipeline/stages/ResultProcessor.h"
#include "pipeline/stages/CacheLookup.h"
#include "pipeline/stages/CacheStore.h"
#include "pipeline/stages/ThumbnailGenerator.h"
#include "caching/SqliteHashCache.h"
#include "util/Token.h"
#include "util/AppSettings.h"
#include "types/DataTypes.h"
#include <QDebug>
#include <QThread>

namespace photoboss {

    PipelineController::PipelineController(
        QObject* parent)
        : QObject(parent)
    {
    }

    PipelineController::~PipelineController()
    {
        stop();
    }

    void PipelineController::start(const ScanRequest& request)
    {
        if (m_state_ != PipelineState::Stopped)
            return;
        SetPipelineState(PipelineState::Running);

        // Reset cumulative progress tracking
        m_findProgress_ = 0;
        m_findTotal_ = 0;
        m_analyzeProgress_ = 0;
        m_analyzeTotal_ = 0;
        m_groupProgress_ = 0;
        m_groupTotal_ = 0;

        Token t;
        m_scanId_ = SqliteHashCache(0).nextScanId(t);
        createPipeline(request);
    }

    void PipelineController::stop()
    {
        if (m_state_ != PipelineState::Running) {
            return;
        }

        SetPipelineState(PipelineState::Stopping);

        Token t;
        if (m_pipeline_) {
            m_pipeline_->scan.request_shutdown(t);
			m_pipeline_->disk.request_shutdown(t);
            m_pipeline_->readQueue.request_shutdown(t);
            m_pipeline_->cacheStoreQueue.request_shutdown(t);
            m_pipeline_->resultQueue.request_shutdown(t);
            m_pipeline_->thumbnailQueue.request_shutdown(t);
        }
    }

    void PipelineController::createPipeline(const ScanRequest& request)
    {
        m_current_request_ = request;
        m_pipeline_ = std::make_unique<Pipeline>(100, 50, 200, 50);

        // ---- Scanner ----
        m_pipeline_->scanner = new DirectoryScanner(request, m_pipeline_->scan);
        m_pipeline_->scanner->moveToThread(&m_pipeline_->scannerThread);

        connect(m_pipeline_->scanner, &DirectoryScanner::status,
            this, [this](QString msg) {
                emit status(msg);
            });

        connect(&m_pipeline_->scannerThread, &QThread::started,
            m_pipeline_->scanner, &StageBase::Run);

        connect(&m_pipeline_->scannerThread, &QThread::finished,
            m_pipeline_->scanner, &QObject::deleteLater);


        // ---- Cache Lookup ----
        m_pipeline_->cacheLookup = new CacheLookup(
            m_pipeline_->scan,
			m_pipeline_->disk,
			m_pipeline_->resultQueue,
			"CacheLookup",
            m_scanId_
        );

        m_pipeline_->cacheLookup->moveToThread(&m_pipeline_->cacheLookupThread);

        connect(&m_pipeline_->cacheLookupThread, &QThread::started,
            m_pipeline_->cacheLookup, &StageBase::Run);

        connect(&m_pipeline_->cacheLookupThread, &QThread::finished,
            m_pipeline_->cacheLookup, &QObject::deleteLater);


        // ---- Disk Reader ----
        m_pipeline_->reader = new DiskReader(m_pipeline_->disk, m_pipeline_->readQueue);
        m_pipeline_->reader->moveToThread(&m_pipeline_->readerThread);

        connect(&m_pipeline_->readerThread, &QThread::started,
            m_pipeline_->reader, &StageBase::Run);

        connect(&m_pipeline_->readerThread, &QThread::finished,
            m_pipeline_->reader, &QObject::deleteLater);


        // ---- Hash Workers ----
        const int workers = std::max(1, QThread::idealThreadCount() - 1);

        for (int i = 0; i < workers; ++i) {
            HashWorker* worker = new HashWorker(m_pipeline_->readQueue, m_pipeline_->cacheStoreQueue);
            QThread* thread = new QThread();
			m_hash_worker_threads_.push_back(thread);
            worker->moveToThread(thread);

            connect(thread, &QThread::started,
                worker, &StageBase::Run);

            connect(thread, &QThread::finished,
                worker, &QObject::deleteLater);

            connect(thread, &QThread::finished,
				thread, &QObject::deleteLater);

            m_pipeline_->hashWorkers.push_back(worker);
        }

		// ---- Cache Store ----
        
		m_pipeline_->cacheStore = new CacheStore(
			m_pipeline_->cacheStoreQueue,
			m_pipeline_->resultQueue,
            "CacheStore",
            m_scanId_
            );

        m_pipeline_->cacheStore->moveToThread(&m_pipeline_->cacheStoreThread);

        connect(&m_pipeline_->cacheStoreThread, &QThread::started,
            m_pipeline_->cacheStore, &StageBase::Run);

        connect(&m_pipeline_->cacheStoreThread, &QThread::finished,
            m_pipeline_->cacheStore, &QObject::deleteLater);
        

        // ---- Thumbnail Generators ----
        const int thumbWorkers = std::max(2, QThread::idealThreadCount() / 2);
        m_activeThumbnailWorkers_ = thumbWorkers;
        for (int i = 0; i < thumbWorkers; ++i) {
            ThumbnailGenerator* worker = new ThumbnailGenerator(m_pipeline_->thumbnailQueue, "ThumbnailWorker");
            QThread* thread = new QThread();
            m_thumbnail_worker_threads_.push_back(thread);
            worker->moveToThread(thread);

            connect(thread, &QThread::started, worker, &StageBase::Run);
            connect(thread, &QThread::finished, worker, &QObject::deleteLater);
            connect(thread, &QThread::finished, thread, &QObject::deleteLater);

            connect(worker, &ThumbnailGenerator::thumbnailReady, this, &PipelineController::thumbnailReady, Qt::QueuedConnection);
            connect(worker, &ThumbnailGenerator::workerFinished, this, &PipelineController::onThumbnailWorkerFinished, Qt::QueuedConnection);

            m_pipeline_->thumbnailGenerators.push_back(worker);
        }

		// ---- Result Processor ----
		m_pipeline_->resultProcessor = new ResultProcessor(
            m_pipeline_->resultQueue, 
            m_pipeline_->thumbnailQueue,
            "ResultProcessor"
        );
		m_pipeline_->resultProcessor->moveToThread(&m_pipeline_->resultThread);

		connect(&m_pipeline_->resultThread, &QThread::started,
			m_pipeline_->resultProcessor, &StageBase::Run);

		connect(&m_pipeline_->resultThread, &QThread::finished,
			m_pipeline_->resultProcessor, &QObject::deleteLater);

        connect(
            m_pipeline_->resultProcessor,
            &ResultProcessor::groupingFinished,
            this,
            &PipelineController::onGroupingFinished,
            Qt::QueuedConnection
        );

        connect(
            m_pipeline_->resultProcessor,
            &ResultProcessor::groupAdded,
            this,
            &PipelineController::groupAdded,
            Qt::QueuedConnection
        );

        connect(
            m_pipeline_->resultProcessor,
            &ResultProcessor::groupUpdated,
            this,
            &PipelineController::groupUpdated,
            Qt::QueuedConnection
        );


        connect(m_pipeline_->scanner, &StageBase::progress, this, [this](qint64 current, qint64 total) {
            // Track Find phase cumulative progress
            m_findProgress_ = current;
            m_findTotal_ = total;

            if (total == 0) {
                emit status("Scan started...");
            }
            // Emit per-phase progress (NOT cumulative) for Find phase spinner
            emit phaseUpdate(Phase::Find, static_cast<int>(current), static_cast<int>(total));
        });

        connect(m_pipeline_->resultProcessor, &StageBase::progress, this,
            [this](qint64 current, qint64 total) {
                // Track Analyze phase cumulative progress
                m_analyzeProgress_ = current;
                // Analyze processes same total as Find phase discovered
                m_analyzeTotal_ = m_findTotal_;

                emit status("Files Processed. Grouping Duplicates...");
                // Emit per-phase progress (NOT cumulative) for Analyze phase spinner
                emit phaseUpdate(Phase::Analyze, static_cast<int>(current), static_cast<int>(m_analyzeTotal_));
            });

        // ---- Start All Threads ----
        m_pipeline_->scannerThread.start();
        m_pipeline_->cacheLookupThread.start();
        m_pipeline_->readerThread.start();
        for (auto* thread : m_hash_worker_threads_) thread->start();
        m_pipeline_->cacheStoreThread.start();
        for (auto* thread : m_thumbnail_worker_threads_) thread->start();
        m_pipeline_->resultThread.start();

        SetPipelineState(PipelineState::Running);
    }

    void PipelineController::destroyPipeline()
    {
        if (!m_pipeline_) return;

        m_pipeline_->scannerThread.quit();
        m_pipeline_->scannerThread.wait();

        m_pipeline_->cacheLookupThread.quit();
        m_pipeline_->cacheLookupThread.wait();

        m_pipeline_->cacheStoreThread.quit();
        m_pipeline_->cacheStoreThread.wait();

        m_pipeline_->readerThread.quit();
        m_pipeline_->readerThread.wait();

        for (auto* thread : m_hash_worker_threads_) {
            if (thread->isRunning()) {
                thread->quit();
                thread->wait();
            }
        }
        m_hash_worker_threads_.clear();

        for (auto* thread : m_thumbnail_worker_threads_) {
            if (thread->isRunning()) {
                thread->quit();
                thread->wait();
            }
        }
        m_thumbnail_worker_threads_.clear();

		m_pipeline_->resultThread.quit();
		m_pipeline_->resultThread.wait();

        m_pipeline_.reset();  
    }

    void PipelineController::onGroupingFinished(const std::vector<ImageGroup> groups)
    {
        // Track Group phase completion
        m_groupProgress_ = m_findTotal_;
        m_groupTotal_ = m_findTotal_;

        // Emit per-phase progress for Group phase spinner (100% complete)
        emit phaseUpdate(Phase::Group, static_cast<int>(m_groupProgress_), static_cast<int>(m_groupTotal_));

        emit finalGroups(groups); // UI-facing signal
    }

    void PipelineController::onThumbnailWorkerFinished()
    {
        if (--m_activeThumbnailWorkers_ == 0) {
            qDebug() << "All thumbnail workers finished. Cleaning up pipeline.";

            // Prune stale cache entries for this directory
            if (!m_current_request_.directory.isEmpty()) {
                SqliteHashCache cache(m_scanId_);
                cache.prune(m_current_request_.directory);
            }

            SetPipelineState(PipelineState::Stopped);
            destroyPipeline();
        }
    }

    void PipelineController::SetPipelineState(PipelineState state)
    {
		m_state_ = state;
		emit pipelineStateChanged(state);
        qDebug() << "Pipeline state changed to " << m_state_;
    }

    void PipelineController::restart()
    {
        if (m_state_ == PipelineState::Running)
            stop();

        start(m_current_request_);
    }
}