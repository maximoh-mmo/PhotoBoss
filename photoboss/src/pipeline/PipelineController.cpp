#include "pipeline/PipelineController.h"
#include "pipeline/stages/DirectoryScanner.h"
#include "pipeline/stages/DiskReader.h"
#include "pipeline/stages/HashWorker.h"
#include "pipeline/stages/ResultProcessor.h"
#include "pipeline/stages/CacheLookup.h"
#include "pipeline/stages/CacheStore.h"
#include "caching/SqliteHashCache.h"
#include "util/ShutdownToken.h"
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
        SetPipelineState(PipelineState::Starting);
        createPipeline(request);
        SetPipelineState(PipelineState::Idle);

    }

    void PipelineController::stop()
    {
        if (m_state_ == PipelineState::Stopped)
            return;

        SetPipelineState(PipelineState::Stopping);
        ShutdownToken t;
        if (m_pipeline_) {
            m_pipeline_->scan.request_shutdown(t);
			m_pipeline_->disk.request_shutdown(t);
            m_pipeline_->readQueue.request_shutdown(t);
            m_pipeline_->cacheStoreQueue.request_shutdown(t);
            m_pipeline_->resultQueue.request_shutdown(t);
        }

        destroyPipeline();
        SetPipelineState(PipelineState::Stopped);
    }

    void PipelineController::createPipeline(const ScanRequest& request)
    {
        m_current_request_ = request;
        m_pipeline_ = std::make_unique<Pipeline>(100, 50, 200, 50);

        // ---- Scanner ----
        m_pipeline_->scanner = new DirectoryScanner(request, m_pipeline_->scan);
        m_pipeline_->scanner->moveToThread(&m_pipeline_->scannerThread);

        connect(&m_pipeline_->scannerThread, &QThread::started,
            m_pipeline_->scanner, &StageBase::Run);

        connect(&m_pipeline_->scannerThread, &QThread::finished,
            m_pipeline_->scanner, &QObject::deleteLater);

        m_pipeline_->scannerThread.start();

        // ---- Cache Lookup ----
        m_pipeline_->cacheLookup = new CacheLookup(
            m_pipeline_->scan,
			m_pipeline_->disk,
			m_pipeline_->resultQueue,
			"CacheLookup"
        );

        m_pipeline_->cacheLookup->moveToThread(&m_pipeline_->cacheLookupThread);

        connect(&m_pipeline_->cacheLookupThread, &QThread::started,
            m_pipeline_->cacheLookup, &StageBase::Run);

        connect(&m_pipeline_->cacheLookupThread, &QThread::finished,
            m_pipeline_->cacheLookup, &QObject::deleteLater);

        m_pipeline_->cacheLookupThread.start();

        // ---- Disk Reader ----
        m_pipeline_->reader = new DiskReader(m_pipeline_->disk, m_pipeline_->readQueue);
        m_pipeline_->reader->moveToThread(&m_pipeline_->readerThread);

        connect(m_pipeline_->reader, &DiskReader::ReadProgress,
            this, &PipelineController::diskReadProgress);

        connect(&m_pipeline_->readerThread, &QThread::started,
            m_pipeline_->reader, &StageBase::Run);

        connect(&m_pipeline_->readerThread, &QThread::finished,
            m_pipeline_->reader, &QObject::deleteLater);

        m_pipeline_->readerThread.start();

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

            thread->start();

            m_pipeline_->hashWorkers.push_back(worker);
        }

		// ---- Cache Store ----
        
		m_pipeline_->cacheStore = new CacheStore(
			m_pipeline_->cacheStoreQueue,
			m_pipeline_->resultQueue,
            "CacheStore"
            );

        m_pipeline_->cacheStore->moveToThread(&m_pipeline_->cacheStoreThread);

        connect(&m_pipeline_->cacheStoreThread, &QThread::started,
            m_pipeline_->cacheStore, &StageBase::Run);

        connect(&m_pipeline_->cacheStoreThread, &QThread::finished,
            m_pipeline_->cacheStore, &QObject::deleteLater);
        
		m_pipeline_->cacheStoreThread.start();

		// ---- Result Processor ----
		m_pipeline_->resultProcessor = new ResultProcessor(m_pipeline_->resultQueue, "ResultProcessor");
		m_pipeline_->resultProcessor->moveToThread(&m_pipeline_->resultThread);

		connect(&m_pipeline_->resultThread, &QThread::started,
			m_pipeline_->resultProcessor, &StageBase::Run);

		connect(&m_pipeline_->resultThread, &QThread::finished,
			m_pipeline_->resultProcessor, &QObject::deleteLater);

        m_pipeline_->resultThread.start();
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

		m_pipeline_->resultThread.quit();
		m_pipeline_->resultThread.wait();

        m_pipeline_.reset(); // queues + workers destroyed cleanly
    
    }

    void PipelineController::SetPipelineState(PipelineState state)
    {
		m_state_ = state;
		emit pipelineStateChanged(state);
    }

    void PipelineController::restart()
    {
        stop();
        start(m_current_request_);
    }
    
}