#include "pipeline/PipelineFactory.h"
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
        
        auto identityQueue = std::make_unique<Queue<FileIdentity>>();
        auto disk = std::make_unique<Queue<FileIdentity>>();
        auto resultQueue = std::make_unique<Queue<std::shared_ptr<HashedImageResult>>>();
        auto readQueue = std::make_unique<Queue<std::unique_ptr<DiskReadResult>>>(settings::ReadQueueCapacity);
        auto cacheStoreQueue = std::make_unique<Queue<std::shared_ptr<HashedImageResult>>>();
        auto thumbnailQueue = std::make_unique<Queue<ThumbnailRequestPtr>>();

        // Get raw pointers for stages (ownership transferred to pipeline later)
        Queue<FileIdentity>* identityQueuePtr = identityQueue.get();
        Queue<FileIdentity>* diskPtr = disk.get();
        Queue<std::shared_ptr<HashedImageResult>>* resultQueuePtr = resultQueue.get();
        Queue<std::unique_ptr<DiskReadResult>>* readQueuePtr = readQueue.get();
        Queue<std::shared_ptr<HashedImageResult>>* cacheStoreQueuePtr = cacheStoreQueue.get();
        Queue<ThumbnailRequestPtr>* thumbnailQueuePtr = thumbnailQueue.get();

        auto enumerator = new FileEnumerator(
            config.request,
            *identityQueuePtr
        );

        CacheLookup* cacheLookup = new CacheLookup(
            *identityQueuePtr,
            *diskPtr,
            *resultQueuePtr,
			pipeline->ScanId()
        );

        int diskReaderCount = config.storage == StorageStrategy::Parallel
            ? std::max(1, QThread::idealThreadCount() / 2)
            : 1;

        std::vector<DiskReader*> diskReaders;
        for (int i = 0; i < diskReaderCount; ++i) {
            DiskReader* dr = new DiskReader(*diskPtr, *readQueuePtr);
            diskReaders.push_back(dr);
            QThread* thread = new QThread();
            moveToThread(pipeline.get(), dr, thread);
        }

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
        
		std::vector<HashWorker*> hashWorkers;
        for (int i = 0; i < workers; ++i) {
            HashWorker* worker = new HashWorker(
                *readQueuePtr,
                *cacheStoreQueuePtr
            );
			hashWorkers.push_back(worker);
			QThread* thread = new QThread();
			moveToThread(pipeline.get(), worker, thread);
        }

        workers = std::max(2, QThread::idealThreadCount() / 2);
        
        std::vector<ThumbnailGenerator*> thumbnailWorkers;
        for (int i = 0; i < workers; ++i) {
            ThumbnailGenerator* worker = new ThumbnailGenerator(
                *thumbnailQueuePtr, pipeline->ScanId()
            );
            thumbnailWorkers.push_back(worker);
            QThread* thread = new QThread();
            moveToThread(pipeline.get(), worker, thread);
        }

        moveToThread(pipeline.get(), enumerator);
        moveToThread(pipeline.get(), cacheLookup);
        moveToThread(pipeline.get(), cacheStore);
        moveToThread(pipeline.get(), resultProcessor);

        if (sink) {

            // Status messages
            QObject::connect(enumerator,
                &FileEnumerator::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });
            QObject::connect(cacheStore,
				&CacheStore::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });
            QObject::connect(resultProcessor,
                &ResultProcessor::status,
                [sink](const QString& msg) { sink->setStatusMessage(msg); });

            QObject::connect(enumerator,
                &FileEnumerator::finalCount,
                [sink](int total) { sink->setFileTotal(total); });

            QObject::connect(enumerator,
                &FileEnumerator::incrementProgress,
                [sink](int inc) { sink->incrementPhaseProgress(Pipeline::Phase::Find, inc); });

            // Connect ALL workers for phase progress
            QObject::connect(cacheLookup,
                &CacheLookup::incrementProgress,
                [sink](int inc) { sink->incrementPhaseProgress(Pipeline::Phase::Analyze, inc); });

            QObject::connect(resultProcessor,
                &ResultProcessor::incrementProgress,
                [sink](int inc) { sink->incrementPhaseProgress(Pipeline::Phase::Group, inc); });

            // Group additions and updates (for immediate UI feedback)
            QObject::connect(resultProcessor,
                &ResultProcessor::groupAdded,
                [sink](const ImageGroup& group) { sink->addPendingGroup(group); });
            QObject::connect(resultProcessor,
                &ResultProcessor::groupUpdated,
                [sink](const ImageGroup& group) { sink->updateGroup(group); });

            // Thumbnail ready (for immediate thumbnail display)
            for (auto* worker : thumbnailWorkers) {
                QObject::connect(worker,
                &ThumbnailGenerator::thumbnailReady,
                [sink](const ThumbnailResult& result) { sink->setThumbnail(result); });
            }

            QObject::connect(pipeline.get(), &Pipeline::stateChanged,
                [sink](Pipeline::PipelineState s) { sink->setPipelineState(s); });
        }

		// Transfer queue ownership to pipeline
		pipeline->AddQueue(std::move(identityQueue));
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
        QObject::connect(stage, &StageBase::stageFinished,
            thread, &QThread::quit,
            Qt::QueuedConnection);
        QObject::connect(thread, &QThread::finished,
            stage, &QObject::deleteLater,
            Qt::QueuedConnection);
		pipeline->AddThread(thread);
        pipeline->AddStage(stage);
    }
}