#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "util/Queue.h"
#include "HashWorker.h"
#include "util/DataTypes.h"
#include "hashing/HashMethod.h"
#include "pipeline/stages/Pipeline.h"
#include "caching/NullHashCache.h"

namespace photoboss {
	class DirectoryScanner;
	class DiskReader;
	class ResultProcessor;
	class CacheLookup;
    class CacheStore;

    enum class PipelineState {
        Stopped,
        Starting,
        Idle,
        Scanning,
        Stopping
    };

    struct Pipeline {
        // Queues
        Queue<FileIdentityBatchPtr> scan;
        Queue<FileIdentityBatchPtr> disk;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> cacheStoreQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;

        // Threads
        QThread scannerThread;
        QThread cacheLookupThread;
        QThread cacheStoreThread;
        QThread readerThread;
        QThread resultThread;

        // Workers
        DirectoryScanner* scanner = nullptr;
        DiskReader* reader = nullptr;
        ResultProcessor* resultProcessor = nullptr;
        CacheLookup* cacheLookup = nullptr;
		CacheStore* cacheStore = nullptr;
		std::vector<HashWorker*> hashWorkers;

        Pipeline() = default;
        Pipeline(size_t scanCap,
		    size_t diskCap,
            size_t readCap,
            size_t resultCap)
            : scan(scanCap)
		    , disk(diskCap)
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

        void start(const ScanRequest& request);
        void stop();
		void restart();

    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> result);
		void status(const QString& message);
		void diskReadProgress(int current, int total);
        void pipelineStateChanged(PipelineState state);
        
    private:
        void createPipeline(const ScanRequest& request);
		void destroyPipeline();
    private:
        std::unique_ptr<Pipeline> m_pipeline_;
        std::unique_ptr <IHashCache> m_cache_;
		PipelineState m_state_ = PipelineState::Stopped;
		void SetPipelineState(PipelineState state);
		ScanRequest m_current_request_;
		std::vector<QThread*> m_hash_worker_threads_;
    };
}