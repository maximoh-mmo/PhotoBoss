#include "pipeline/PipelineFactory.h"
#include "pipeline/stages/ExifRead.h"
#include "pipeline/stages/FileEnumerator.h"
#include "pipeline/stages/DiskReader.h"
#include "pipeline/factory/FactoryHashWorker.h"
#include "pipeline/stages/ResultProcessor.h"
#include "pipeline/stages/CacheLookup.h"
#include "pipeline/stages/CacheStore.h"
#include "pipeline/stages/ThumbnailGenerator.h"
#include "pipeline/StageBase.h"
#include "caching/SqliteHashCache.h"
#include "util/AppSettings.h"
#include "pipeline/Pipeline.h"

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

    std::unique_ptr<Pipeline> PipelineFactory::create(const Config& config)
    {
        auto pipeline = std::make_unique<Pipeline>();

        Queue<std::shared_ptr<QStringList>> pathsQueue;
        auto enumerator = new FileEnumerator(
            config.request,
            pathsQueue
        );

        QThread enumeratorThread;
        enumerator->moveToThread(&enumeratorThread);
        
        pipeline->AddQueue(&pathsQueue);
        pipeline->AddStage(enumerator);
        pipeline->AddThread(&enumeratorThread);

        const int workerCount = config.storage == StorageStrategy::Parallel
            ? std::max(1, QThread::idealThreadCount() - 1)
            : 1;

        Queue<FileIdentityBatchPtr> exifQueue;
        std::vector<QThread*> exifReaderThreads;
        std::vector<ExifRead*> exifReaders;

        for (int i = 0; i < workerCount; ++i) {
            ExifRead* reader = new ExifRead(
                pathsQueue,
                exifQueue
            );
            QThread* thread = new QThread();
            exifReaderThreads.push_back(thread);
            reader->moveToThread(thread);
            exifReaders.push_back(reader);
        }
		pipeline->AddQueue(&exifQueue);
		for (auto reader : exifReaders) {
            pipeline->AddStage(reader);
        }
        for (auto thread : exifReaderThreads) {
            pipeline->AddThread(thread);
		}

        Queue<FileIdentityBatchPtr> disk;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;
        CacheLookup* cacheLookup = new CacheLookup(
            exifQueue,
            disk,
            resultQueue,
            "CacheLookup",
			pipeline->ScanId()
        );

        QThread cacheLookupThread;
        cacheLookup->moveToThread(&cacheLookupThread);
		pipeline->AddQueue(&disk);
		pipeline->AddQueue(&resultQueue);
		pipeline->AddStage(cacheLookup);
        pipeline->AddThread(&cacheLookupThread);

        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        DiskReader* reader = new DiskReader(
            disk,
            readQueue
        );

        QThread readerThread;
        reader->moveToThread(&readerThread);
        pipeline->AddQueue(&readQueue);
        pipeline->AddStage(reader);
        pipeline->AddThread(&readerThread);

        int workers = config.storage == StorageStrategy::Parallel
            ? std::max(1, QThread::idealThreadCount() - 1)
            : 1;

        Queue<std::shared_ptr<HashedImageResult>> cacheStoreQueue;
        std::vector<FactoryHashWorker*> factoryHashWorkers;
        std::vector<QThread*> hashWorkerThreads;
        for (int i = 0; i < workers; ++i) {
            FactoryHashWorker* worker = new FactoryHashWorker(
                readQueue,
                cacheStoreQueue
            );
            QThread* thread = new QThread();
            hashWorkerThreads.push_back(thread);
            worker->moveToThread(thread);
            factoryHashWorkers.push_back(worker);
        }
		pipeline->AddQueue(&cacheStoreQueue);
        for (auto worker : factoryHashWorkers) {
            pipeline->AddStage(worker);
		}
        for (auto thread : hashWorkerThreads) {
            pipeline->AddThread(thread);
        }

        QThread cacheStoreThread;
        CacheStore* cacheStore = new CacheStore(
            cacheStoreQueue,
            resultQueue,
            "CacheStore",
            pipeline->ScanId()
        );
        cacheStore->moveToThread(&cacheStoreThread);
        pipeline->AddStage(cacheStore);
        pipeline->AddThread(&cacheStoreThread);

        workers = std::max(2, QThread::idealThreadCount() / 2);
        std::vector<QThread*> thumbnailWorkerThreads;
        Queue<ThumbnailRequestPtr> thumbnailQueue;
        std::vector<ThumbnailGenerator*> thumbnailGenerators;

        for (int i = 0; i < workers; ++i) {
            ThumbnailGenerator* worker = new ThumbnailGenerator(
                thumbnailQueue,
                "ThumbnailWorker"
            );
            QThread* thread = new QThread();
            thumbnailWorkerThreads.push_back(thread);
            worker->moveToThread(thread);
            thumbnailGenerators.push_back(worker);
        }
		pipeline->AddQueue(&thumbnailQueue);
        for (auto worker : thumbnailGenerators) {
			pipeline->AddStage(worker);
		}
        for (auto thread : thumbnailWorkerThreads) {
            pipeline->AddThread(thread);
        }

        QThread resultThread;

        ResultProcessor* resultProcessor = new ResultProcessor(
            resultQueue,
            thumbnailQueue,
            "ResultProcessor"
        );
        resultProcessor->moveToThread(&resultThread);

		return pipeline;

    }
}