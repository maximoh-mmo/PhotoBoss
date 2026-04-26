#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "types/GroupTypes.h"

namespace photoboss {

class DirectoryScanner;
class DiskReader;
class ResultProcessor;
class CacheLookup;
class CacheStore;
class HashWorker;
class ThumbnailGenerator;

class PipelineFactory : public QObject {
    Q_OBJECT
public:
    enum class StorageStrategy {
        Sequential,  // HDD - minimize seeks
        Parallel     // SSD - maximize throughput
    };

    struct Config {
        ScanRequest request;
        StorageStrategy storage;
        bool enableCache = true;
        bool enableThumbnails = true;
        size_t queueCapacity = 100;
    };

    struct Pipeline {
        // Queues
        Queue<FileIdentityBatchPtr> scan;
        Queue<FileIdentityBatchPtr> disk;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> cacheStoreQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;
        Queue<ThumbnailRequestPtr> thumbnailQueue;

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
        std::vector<ThumbnailGenerator*> thumbnailGenerators;

        std::vector<QThread*> hashWorkerThreads;
        std::vector<QThread*> thumbnailWorkerThreads;
    };

    explicit PipelineFactory(QObject* parent = nullptr);
    ~PipelineFactory();

    static std::unique_ptr<Pipeline> create(const Config& config, quint64 scanId);

private:
    static void createScanner(Pipeline& pipeline, const Config& config);
    static void createCacheLookup(Pipeline& pipeline, const Config& config, quint64 scanId);
    static void createDiskReader(Pipeline& pipeline, const Config& config);
    static void createHashWorkers(Pipeline& pipeline, const Config& config);
    static void createCacheStore(Pipeline& pipeline, const Config& config, quint64 scanId);
    static void createThumbnailGenerators(Pipeline& pipeline, const Config& config);
    static void createResultProcessor(Pipeline& pipeline, const Config& config);
};

}