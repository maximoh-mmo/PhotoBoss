#pragma once

#include <QObject>
#include <memory>
#include <set>
#include <vector>
#include <QThread>
#include "pipeline/Pipeline.h"
#include "util/PipelineTypes.h"
#include "util/FileTypes.h"
#include "util/CacheTypes.h"
#include "util/HashTypes.h"
#include "hashing/HashRegistry.h"
#include "stages/DirectoryScanner.h"
#include "stages/CacheLookup.h"
#include "stages/HashWorker.h"
#include "stages/DiskReader.h"
#include "stages/ResultProcessor.h"

namespace photoboss {
    struct Pipeline {
        // Queues
        Queue<std::shared_ptr<Queue<FingerprintBatchPtr>>> scanQueue;
        Queue<std::shared_ptr<Queue<FingerprintBatchPtr>>> diskQueue;
        Queue<std::shared_ptr<Queue<CacheLookupResult>>> routerQueue;
        Queue<std::shared_ptr<Queue<DiskReadResult>>> readQueue;
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
			, routerQueue(diskCap)
            , readQueue(readCap)
            , resultQueue(resultCap)

        {
        }
    };

    class PipelineController : public QObject
    {
        class NullHashCache;
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