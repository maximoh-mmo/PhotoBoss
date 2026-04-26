#include "pipeline/PipelineFactory.h"
#include "pipeline/stages/ExifRead.h"
#include "pipeline/stages/FileEnumerator.h"
#include "pipeline/stages/DiskReader.h"
#include "pipeline/stages/HashWorker.h"
#include "pipeline/stages/ResultProcessor.h"
#include "pipeline/stages/CacheLookup.h"
#include "pipeline/stages/CacheStore.h"
#include "pipeline/stages/ThumbnailGenerator.h"
#include "pipeline/StageBase.h"
#include "caching/SqliteHashCache.h"
#include "util/AppSettings.h"

#ifdef Q_PRIVATE_SLOTS
#undef Q_PRIVATE_SLOTS
#endif

namespace photoboss {

PipelineFactory::PipelineFactory(QObject* parent)
    : QObject(parent)
{
}

PipelineFactory::~PipelineFactory()
{
}

std::unique_ptr<PipelineFactory::Pipeline> PipelineFactory::create(const Config& config, quint64 scanId)
{
    auto pipeline = std::make_unique<Pipeline>();

    createFileEnumerator(*pipeline, config);
    createExifReaders(*pipeline, config);
    createCacheLookup(*pipeline, config, scanId);
    createDiskReader(*pipeline, config);
    createHashWorkers(*pipeline, config);
    createCacheStore(*pipeline, config, scanId);
    createThumbnailGenerators(*pipeline, config);
    createResultProcessor(*pipeline, config);

    return pipeline;
}

void PipelineFactory::createFileEnumerator(Pipeline& pipeline, const Config& config)
{
    pipeline.enumerator = new FileEnumerator(
        config.request,
        pipeline.pathsQueue
    );
    pipeline.enumerator->moveToThread(&pipeline.enumeratorThread);
}

void PipelineFactory::createExifReaders(Pipeline& pipeline, const Config& config)
{
    pipeline.exifQueue.register_producer();

    const int workerCount = config.storage == StorageStrategy::Parallel
        ? std::max(1, QThread::idealThreadCount() - 1)
        : 1;

    for (int i = 0; i < workerCount; ++i) {
        ExifRead* reader = new ExifRead(
            pipeline.pathsQueue,
            pipeline.exifQueue
        );
        QThread* thread = new QThread();
        pipeline.exifReaderThreads.push_back(thread);
        reader->moveToThread(thread);
        pipeline.exifReaders.push_back(reader);
    }
}

void PipelineFactory::createCacheLookup(Pipeline& pipeline, const Config& config, quint64 scanId)
{
    pipeline.cacheLookup = new CacheLookup(
        pipeline.exifQueue,
        pipeline.disk,
        pipeline.resultQueue,
        "CacheLookup",
        scanId
    );
    pipeline.cacheLookup->moveToThread(&pipeline.cacheLookupThread);
}

void PipelineFactory::createDiskReader(Pipeline& pipeline, const Config& config)
{
    pipeline.reader = new DiskReader(
        pipeline.disk,
        pipeline.readQueue
    );
    pipeline.reader->moveToThread(&pipeline.readerThread);
}

void PipelineFactory::createHashWorkers(Pipeline& pipeline, const Config& config)
{
    const int workers = config.storage == StorageStrategy::Parallel
        ? std::max(1, QThread::idealThreadCount() - 1)
        : 1;

    for (int i = 0; i < workers; ++i) {
        HashWorker* worker = new HashWorker(
            pipeline.readQueue,
            pipeline.cacheStoreQueue
        );
        QThread* thread = new QThread();
        pipeline.hashWorkerThreads.push_back(thread);
        worker->moveToThread(thread);
        pipeline.hashWorkers.push_back(worker);
    }
}

void PipelineFactory::createCacheStore(Pipeline& pipeline, const Config& config, quint64 scanId)
{
    pipeline.cacheStore = new CacheStore(
        pipeline.cacheStoreQueue,
        pipeline.resultQueue,
        "CacheStore",
        scanId
    );
    pipeline.cacheStore->moveToThread(&pipeline.cacheStoreThread);
}

void PipelineFactory::createThumbnailGenerators(Pipeline& pipeline, const Config& config)
{
    const int workers = std::max(2, QThread::idealThreadCount() / 2);

    for (int i = 0; i < workers; ++i) {
        ThumbnailGenerator* worker = new ThumbnailGenerator(
            pipeline.thumbnailQueue,
            "ThumbnailWorker"
        );
        QThread* thread = new QThread();
        pipeline.thumbnailWorkerThreads.push_back(thread);
        worker->moveToThread(thread);
        pipeline.thumbnailGenerators.push_back(worker);
    }
}

void PipelineFactory::createResultProcessor(Pipeline& pipeline, const Config& config)
{
    pipeline.resultProcessor = new ResultProcessor(
        pipeline.resultQueue,
        pipeline.thumbnailQueue,
        "ResultProcessor"
    );
    pipeline.resultProcessor->moveToThread(&pipeline.resultThread);
}

}