#include "PipelineController.h"

#include "DirectoryScanner.h"
#include "DiskReader.h"
#include "HashWorker.h"
#include "ResultProcessor.h"

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

        m_state_ = PipelineState::Starting;
        createPipeline();
        m_state_ = PipelineState::Idle;

    }

    void PipelineController::startScan(const QString& folder, bool recursive)
    {
        if (!m_pipeline_) return;

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

        m_state_ = PipelineState::Stopping;

        if (m_pipeline_ && m_pipeline_->scanner)
            m_pipeline_->scanner->RequestStop();

        if (m_pipeline_) {
            m_pipeline_->scanQueue.shutdown();
            m_pipeline_->readQueue.shutdown();
            m_pipeline_->resultQueue.shutdown();
        }

        destroyPipeline();
        m_state_ = PipelineState::Stopped;
    }

    void PipelineController::createPipeline()
    {
        m_pipeline_ = std::make_unique<Pipeline>(100,50,200);
        
        // ---- Scanner ----
        m_pipeline_->scanner = new DirectoryScanner();
        m_pipeline_->scanner->moveToThread(&m_pipeline_->scannerThread);

        connect(&m_pipeline_->scannerThread, &QThread::finished,
            m_pipeline_->scanner, &QObject::deleteLater);

        connect(m_pipeline_->scanner, &DirectoryScanner::fileBatchFound,
            this, [this](FileMetaListPtr batch) {
                m_pipeline_->scanQueue.push(std::move(batch));
            });

        connect(m_pipeline_->scanner, &DirectoryScanner::finished,
            this, [this]() {
                m_pipeline_->scanQueue.shutdown();
            });

        m_pipeline_->scannerThread.start();

        // ---- Disk Reader ----
        m_pipeline_->reader = new DiskReader(
            m_pipeline_->scanQueue,
            m_pipeline_->readQueue);

        m_pipeline_->reader->moveToThread(&m_pipeline_->readerThread);

        connect(&m_pipeline_->readerThread, &QThread::started,
            m_pipeline_->reader, &DiskReader::Run);

        connect(&m_pipeline_->readerThread, &QThread::finished,
            m_pipeline_->reader, &QObject::deleteLater);

        m_pipeline_->readerThread.start();

        // ---- Hash Workers ----
        const int workers = std::max(1, QThread::idealThreadCount() - 1);

        for (int i = 0; i < workers; ++i) {
            auto* worker = new HashWorker(m_pipeline_->readQueue, m_active_hash_methods_);
            auto thread = new QThread(this);

            worker->moveToThread(thread);

            connect(thread, &QThread::started,
                worker, &HashWorker::Run);

            connect(worker, &HashWorker::image_hashed,
                this, &PipelineController::imageHashed);

            connect(thread, &QThread::finished,
                worker, &QObject::deleteLater);

            connect(thread, &QThread::finished,
				thread, &QObject::deleteLater);

            thread->start();

            m_pipeline_->hashWorkers.push_back(worker);
        }
    }

    void PipelineController::destroyPipeline()
    {
        if (!m_pipeline_) return;

        m_pipeline_->scannerThread.quit();
        m_pipeline_->scannerThread.wait();

        m_pipeline_->readerThread.quit();
        m_pipeline_->readerThread.wait();

        m_pipeline_.reset(); // queues + workers destroyed cleanly
    }

    void PipelineController::restart()
    {
        stop();
        start();
    }
    void PipelineController::updateActiveHashes(const std::set<QString>& enabledKeys)
    {
        m_active_hash_methods_ = HashRegistry().createSnapshot(enabledKeys);

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