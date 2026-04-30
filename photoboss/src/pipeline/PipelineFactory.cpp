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
#include "ui/IUiUpdateSink.h"

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

	std::unique_ptr<Pipeline> PipelineFactory::create(const Config& config, IUiUpdateSink* sink)
    {
        auto pipeline = std::make_unique<Pipeline>();
        
        auto pathsQueue = std::make_unique<Queue<std::shared_ptr<QStringList>>>();
        auto exifQueue = std::make_unique<Queue<FileIdentityBatchPtr>>();
        auto disk = std::make_unique<Queue<FileIdentityBatchPtr>>();
        auto resultQueue = std::make_unique<Queue<std::shared_ptr<HashedImageResult>>>();
        auto readQueue = std::make_unique<Queue<std::unique_ptr<DiskReadResult>>>();
        auto cacheStoreQueue = std::make_unique<Queue<std::shared_ptr<HashedImageResult>>>();
        auto thumbnailQueue = std::make_unique<Queue<ThumbnailRequestPtr>>();

        // Get raw pointers for stages (ownership transferred to pipeline later)
        Queue<std::shared_ptr<QStringList>>* pathsQueuePtr = pathsQueue.get();
        Queue<FileIdentityBatchPtr>* exifQueuePtr = exifQueue.get();
        Queue<FileIdentityBatchPtr>* diskPtr = disk.get();
        Queue<std::shared_ptr<HashedImageResult>>* resultQueuePtr = resultQueue.get();
        Queue<std::unique_ptr<DiskReadResult>>* readQueuePtr = readQueue.get();
        Queue<std::shared_ptr<HashedImageResult>>* cacheStoreQueuePtr = cacheStoreQueue.get();
        Queue<ThumbnailRequestPtr>* thumbnailQueuePtr = thumbnailQueue.get();

        auto enumerator = new FileEnumerator(
            config.request,
            *pathsQueuePtr
        );

        const int workerCount = config.storage == StorageStrategy::Parallel
            ? std::max(1, QThread::idealThreadCount() - 1)
            : 1;

        for (int i = 0; i < workerCount; ++i) {
            ExifRead* reader = new ExifRead(
                *pathsQueuePtr,
                *exifQueuePtr
            );

            QThread* thread = new QThread();
			moveToThread(pipeline.get(), reader, thread);
        }

        CacheLookup* cacheLookup = new CacheLookup(
            *exifQueuePtr,
            *diskPtr,
            *resultQueuePtr,
			pipeline->ScanId()
        );

        DiskReader* reader = new DiskReader(
            *diskPtr,
            *readQueuePtr
        );

        ResultProcessor* resultProcessor = new ResultProcessor(
            *resultQueuePtr,
            *thumbnailQueuePtr
        );

        CacheStore* cacheStore = new CacheStore(
            *cacheStoreQueuePtr,
            *resultQueuePtr,
            pipeline->ScanId()
        );

        int workers = config.storage == StorageStrategy::Parallel
            ? std::max(1, QThread::idealThreadCount() - 1)
            : 1;
        
        for (int i = 0; i < workers; ++i) {
            FactoryHashWorker* worker = new FactoryHashWorker(
                *readQueuePtr,
                *cacheStoreQueuePtr
            );
			moveToThread(pipeline.get(), worker);
        }

        workers = std::max(2, QThread::idealThreadCount() / 2);
        
        for (int i = 0; i < workers; ++i) {
            ThumbnailGenerator* worker = new ThumbnailGenerator(
                *thumbnailQueuePtr
            );
            QThread* thread = new QThread();
            moveToThread(pipeline.get(), worker, thread);
        }

        moveToThread(pipeline.get(), enumerator);
        moveToThread(pipeline.get(), cacheLookup);
        moveToThread(pipeline.get(), reader);
        moveToThread(pipeline.get(), cacheStore);
        moveToThread(pipeline.get(), resultProcessor);

        if (sink) {
            QObject::connect(enumerator,
                &FileEnumerator::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });
            QObject::connect(cacheStore,
				&CacheStore::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });
            QObject::connect(resultProcessor,
                &ResultProcessor::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });
            QObject::connect(cacheLookup,
                &CacheLookup::progress,
                [sink](int current, int total) { sink->setPhaseProgress(Pipeline::Phase::Analyze, current, total); });
            QObject::connect(resultProcessor,
                &ResultProcessor::progress,
				[sink](int current, int total) { sink->setPhaseProgress(Pipeline::Phase::Group, current, total); });
        }

		// Transfer queue ownership to pipeline
		pipeline->AddQueue(std::move(pathsQueue));
		pipeline->AddQueue(std::move(exifQueue));
		pipeline->AddQueue(std::move(disk));
		pipeline->AddQueue(std::move(resultQueue));
		pipeline->AddQueue(std::move(readQueue));
		pipeline->AddQueue(std::move(cacheStoreQueue));
		pipeline->AddQueue(std::move(thumbnailQueue));

		return pipeline;
    }

    void PipelineFactory::moveToThread(Pipeline* pipeline, StageBase* stage, QThread* thread)
    {
        if (!thread) {
            thread = new QThread();
        }
        stage->moveToThread(thread);
        QObject::connect(thread, &QThread::started,
            stage, &StageBase::Run,
            Qt::QueuedConnection);
        QObject::connect(thread, &QThread::finished,
            stage, &QObject::deleteLater,
            Qt::QueuedConnection);
		pipeline->AddThread(thread);
        pipeline->AddStage(stage);
    }
}