#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "types/GroupTypes.h"

namespace photoboss {

class ExifRead;
class FileEnumerator;
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
        Queue<std::shared_ptr<QStringList>> pathsQueue;
        Queue<FileIdentityBatchPtr> exifQueue;
        Queue<FileIdentityBatchPtr> disk;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> cacheStoreQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;
        Queue<ThumbnailRequestPtr> thumbnailQueue;

        QThread enumeratorThread;
        FileEnumerator* enumerator = nullptr;

        std::vector<QThread*> exifReaderThreads;
        std::vector<ExifRead*> exifReaders;

        QThread cacheLookupThread;
        QThread cacheStoreThread;
        QThread readerThread;
        QThread resultThread;

        CacheLookup* cacheLookup = nullptr;
        CacheStore* cacheStore = nullptr;
        DiskReader* reader = nullptr;
        ResultProcessor* resultProcessor = nullptr;
        std::vector<HashWorker*> hashWorkers;
        std::vector<ThumbnailGenerator*> thumbnailGenerators;

        std::vector<QThread*> hashWorkerThreads;
        std::vector<QThread*> thumbnailWorkerThreads;
    };

    explicit PipelineFactory(QObject* parent = nullptr);
    ~PipelineFactory();

    static std::unique_ptr<Pipeline> create(const Config& config, quint64 scanId);

private:
    static void createFileEnumerator(Pipeline& pipeline, const Config& config);
    static void createExifReaders(Pipeline& pipeline, const Config& config);
    static void createCacheLookup(Pipeline& pipeline, const Config& config, quint64 scanId);
    static void createDiskReader(Pipeline& pipeline, const Config& config);
    static void createHashWorkers(Pipeline& pipeline, const Config& config);
    static void createCacheStore(Pipeline& pipeline, const Config& config, quint64 scanId);
    static void createThumbnailGenerators(Pipeline& pipeline, const Config& config);
    static void createResultProcessor(Pipeline& pipeline, const Config& config);
};

}