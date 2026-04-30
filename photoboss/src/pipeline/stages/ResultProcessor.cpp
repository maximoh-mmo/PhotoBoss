#include "pipeline/stages/ResultProcessor.h"
#include "hashing/HashCatalog.h"
#include "pipeline/SimilarityEngine.h"
#include "util/AppSettings.h"
#include <QElapsedTimer>

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        Queue<ThumbnailRequestPtr>& thumbnailQueue,
        QObject* parent) :
        StageBase(parent),
        m_input_(queue),
        m_thumbnailOutput_(thumbnailQueue),
        m_items_()
    {
        m_thumbnailOutput_.register_producer();
    }

    void ResultProcessor::run() {
        SimilarityEngine engine;
        std::shared_ptr<HashedImageResult> item;

        int processedCount = 0;
        bool firstEmit = false;

        while (m_input_.wait_and_pop(item)) {
            Q_ASSERT(!item->hashes.empty());

            if (!firstEmit) {
                emit status(QString("Processing Hashed results..."));
                firstEmit = true;
            }

            engine.addImage(item);
            processedCount++;
            emit progress(processedCount, processedCount);

            m_items_.push_back(std::move(item));

            auto delta = engine.getGroupDelta();

            for (const auto& g : delta.newlyFormed) {
                emit groupAdded(g);
                m_emittedGroups_.insert(g.id);
                m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                for (const auto& img : g.images) {
                    if (!m_thumbnailRequested_.contains(img.path)) {
                        auto thumbReq = std::make_shared<ThumbnailRequest>();
                        thumbReq->path = img.path;
                        thumbReq->rotation = img.rotation;
                        thumbReq->width = settings::ThumbnailWidth;
                        thumbReq->height = settings::ThumbnailWidth;
                        m_thumbnailOutput_.push(std::move(thumbReq));
                        m_thumbnailRequested_.insert(img.path);
                    }
                }
            }

            for (const auto& g : delta.grown) {
                emit groupUpdated(g);
                int prevSize = m_emittedSizes_[g.id];
                m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                int newSize = static_cast<int>(g.images.size());
                for (int i = prevSize; i < newSize; i++) {
                    const auto& img = g.images[i];
                    if (!m_thumbnailRequested_.contains(img.path)) {
                        auto thumbReq = std::make_shared<ThumbnailRequest>();
                        thumbReq->path = img.path;
                        thumbReq->rotation = img.rotation;
                        thumbReq->width = settings::ThumbnailWidth;
                        thumbReq->height = settings::ThumbnailWidth;
                        m_thumbnailOutput_.push(std::move(thumbReq));
                        m_thumbnailRequested_.insert(img.path);
                    }
                }
            }
        }

        emit status(QString("Compiling final groups..."));
        auto groups = engine.getGroups();
        std::vector<ImageGroup> result;
        for (const auto& g : groups) {
            if (g.images.size() > 1) {
                result.push_back(g);
            }
        }
        emit groupingFinished(result);
    }
 

    void ResultProcessor::onStop()
    {
        m_thumbnailOutput_.producer_done();
    }
}