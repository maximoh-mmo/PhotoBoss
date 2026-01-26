#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "util/Queue.h"
#include "HashWorker.h"
#include "util/DataTypes.h"
#include "HashMethod.h"
#include "pipeline/stages/Pipeline.h"
#include "NullHashCache.h"

namespace photoboss {
	class DirectoryScanner;
	class DiskReader;
	class ResultProcessor;
	class CacheLookup;
    
    struct Pipeline {
        // Queues
        Queue<FingerprintBatchPtr> scanQueue;
        Queue<FingerprintBatchPtr> diskQueue;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;

        // Threads
        QThread scannerThread;
        QThread readerThread;
        QThread resultThread;
        QThread cacheThread;

        // Workers
        DirectoryScanner* scanner = nullptr;
        DiskReader* reader = nullptr;
        ResultProcessor* resultProcessor = nullptr;
        CacheLookup* cacheLookup = nullptr;
        std::vector<HashWorker*> hashWorkers;

        Pipeline() = default;
        Pipeline(size_t scanCap,
            size_t diskCap,
            size_t readCap,
            size_t resultCap)
            : scanQueue(scanCap)
            , diskQueue(diskCap)
            , readQueue(readCap)
            , resultQueue(resultCap)
        {
        }
    };

    class PipelineController : public QObject
    {
        Q_OBJECT
    public:
        explicit PipelineController(
            QObject* parent = nullptr);

        ~PipelineController() override;

        void start();
        void startScan(const QString& folder, bool recursive);
        void stop();
		void restart();

        void initializeDefaultHashes();
        void updateActiveHashes(const std::set<QString>& enabledKeys);

    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> result);
		void status(const QString& message);
		void diskReadProgress(int current, int total);
        void pipelineStateChanged(PipelineState state);

    private:
        void createPipeline();
		void destroyPipeline();
    private:
        std::unique_ptr<Pipeline> m_pipeline_;
        std::unique_ptr <NullHashCache> m_cache_;
		PipelineState m_state_ = PipelineState::Stopped;
		void SetPipelineState(PipelineState state);

        std::vector<HashRegistry::Entry> m_active_hash_methods_;
		std::vector<QThread*> m_hash_worker_threads_;
    };
}