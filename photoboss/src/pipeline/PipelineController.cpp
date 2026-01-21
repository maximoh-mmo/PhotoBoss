#include "PipelineController.h"

#include "DirectoryScanner.h"
#include "DiskReader.h"
#include "HashWorker.h"
#include "ResultProcessor.h"
#include "CacheLookup.h"
#include <QDebug>
#include <QThread>

namespace photoboss {

    PipelineController::PipelineController(
        QObject* parent)
        : QObject(parent)
    {
        initializeDefaultHashes();
    }

    PipelineController::~PipelineController()
    {
        stop();
    }

    void PipelineController::initializeDefaultHashes()
    {
        const auto names = HashRegistry::registeredNames();

        std::set<QString> enabledKeys;
        for (const auto& name : names) {
            enabledKeys.insert(name);
        }

        m_active_hash_methods_ = HashRegistry::createSnapshot(enabledKeys);

        qDebug() << "Default-enabled hash methods:"
            << m_active_hash_methods_.size();
    }

    void PipelineController::start()
    {
        if (m_state_ != PipelineState::Stopped)
            return;

        SetPipelineState(PipelineState::Starting);
        createPipeline();
        SetPipelineState(PipelineState::Idle);

    }

    void PipelineController::startScan(const QString& folder, bool recursive)
    {
        if (!m_pipeline_) return;

        if (m_state_ != PipelineState::Idle)
            return;

        SetPipelineState(PipelineState::Scanning);

        QMetaObject::invokeMethod(
            m_pipeline_->scanner,
            "StartScan",
            Qt::QueuedConnection,
            Q_ARG(QString, folder),
            Q_ARG(bool, recursive)
        );
    }

    void PipelineController::stop()
    {
        if (m_state_ == PipelineState::Stopped)
            return;

        SetPipelineState(PipelineState::Stopping);

        if (m_pipeline_) {
            m_pipeline_->scanQueue.shutdown();
			m_pipeline_->diskQueue.shutdown();
            m_pipeline_->readQueue.shutdown();
            m_pipeline_->resultQueue.shutdown();
        }

        destroyPipeline();
        SetPipelineState(PipelineState::Stopped);
    }

    void PipelineController::createPipeline()
    {
        m_pipeline_ = std::make_unique<Pipeline>(100,50,200,50);
        
        // ---- Scanner ----
        m_pipeline_->scanner = new DirectoryScanner();
        m_pipeline_->scanner->moveToThread(&m_pipeline_->scannerThread);

        connect(&m_pipeline_->scannerThread, &QThread::finished,
            m_pipeline_->scanner, &QObject::deleteLater);

        connect(m_pipeline_->scanner, &DirectoryScanner::fileBatchFound,
            this, [this](FingerprintBatchPtr batch) {
                m_pipeline_->scanQueue.push(std::move(batch));
            });

        connect(m_pipeline_->scanner, &DirectoryScanner::finished,
            this, [this]() {
                m_pipeline_->scanQueue.shutdown();
            });

        m_pipeline_->scannerThread.start();
        
		// ---- Cache Lookup ----
		m_cache_ = std::make_unique<NullHashCache>();
        m_pipeline_->cacheLookup = new CacheLookup(
            m_pipeline_->scanQueue,
            m_pipeline_->diskQueue,
            m_pipeline_->resultQueue,
            *m_cache_,
            m_active_hash_methods_);
		m_pipeline_->cacheLookup->moveToThread(&m_pipeline_->cacheThread);

		connect(&m_pipeline_->cacheThread, &QThread::started,
			m_pipeline_->cacheLookup, &PipelineStage::Run);

		connect(&m_pipeline_->cacheThread, &QThread::finished,
			m_pipeline_->cacheLookup, &QObject::deleteLater);

		m_pipeline_->cacheThread.start();

        // ---- Disk Reader ----
        m_pipeline_->reader = new DiskReader(m_pipeline_->diskQueue, m_pipeline_->readQueue);
        m_pipeline_->reader->moveToThread(&m_pipeline_->readerThread);

        connect(m_pipeline_->reader, &DiskReader::ReadProgress,
            this, &PipelineController::diskReadProgress);

        connect(&m_pipeline_->readerThread, &QThread::started,
            m_pipeline_->reader, &PipelineStage::Run);

        connect(&m_pipeline_->readerThread, &QThread::finished,
            m_pipeline_->reader, &QObject::deleteLater);

        m_pipeline_->readerThread.start();

        // ---- Hash Workers ----
        const int workers = std::max(1, QThread::idealThreadCount() - 1);

        for (int i = 0; i < workers; ++i) {
            HashWorker* worker = new HashWorker(m_pipeline_->readQueue, m_pipeline_->resultQueue, m_active_hash_methods_);
            QThread* thread = new QThread(this);
			m_hash_worker_threads_.push_back(thread);
            worker->moveToThread(thread);

            connect(thread, &QThread::started,
                worker, &PipelineStage::Run);

            connect(thread, &QThread::finished,
                worker, &QObject::deleteLater);

            connect(thread, &QThread::finished,
				thread, &QObject::deleteLater);

            thread->start();

            m_pipeline_->hashWorkers.push_back(worker);
        }

		// ---- Result Processor ----
		m_pipeline_->resultProcessor = new ResultProcessor(m_pipeline_->resultQueue);
		m_pipeline_->resultProcessor->moveToThread(&m_pipeline_->resultThread);

		connect(&m_pipeline_->resultThread, &QThread::started,
			m_pipeline_->resultProcessor, &PipelineStage::Run);

		connect(&m_pipeline_->resultThread, &QThread::finished,
			m_pipeline_->resultProcessor, &QObject::deleteLater);

		connect(m_pipeline_->resultProcessor, &ResultProcessor::imageHashed,
			this, &PipelineController::imageHashed);

        m_pipeline_->resultThread.start();
    }

    void PipelineController::destroyPipeline()
    {
        if (!m_pipeline_) return;

        m_pipeline_->scannerThread.quit();
        m_pipeline_->scannerThread.wait();

        m_pipeline_->cacheThread.quit();
        m_pipeline_->cacheThread.wait();

        m_pipeline_->readerThread.quit();
        m_pipeline_->readerThread.wait();

        for (auto* thread : m_hash_worker_threads_) {
            thread->quit();
            thread->wait();
		}

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
        start();
    }
    void PipelineController::updateActiveHashes(const std::set<QString>& enabledKeys)
    {
        m_active_hash_methods_ = HashRegistry::createSnapshot(enabledKeys);

        if (m_active_hash_methods_.empty()) {
            emit status("Please enable at least one hash method");
            return;
        }
        else {
            qDebug() << "Active hash methods updated, count:"
                << m_active_hash_methods_.size();
        }

        if (m_state_ != PipelineState::Stopped) {
            restart(); // full teardown + rebuild
        }
    }
}