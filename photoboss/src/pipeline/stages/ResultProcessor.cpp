#include "pipeline/stages/ResultProcessor.h"
#include "hashing/HashCatalog.h"
#include "pipeline/SimilarityEngine.h"
#include "util/AppSettings.h"
#include <QElapsedTimer>

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        Queue<ThumbnailRequestPtr>& thumbnailQueue,
        QString id,
        QObject* parent) :
        StageBase(std::move(id), parent),
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
        QElapsedTimer timer;
        timer.start();
        
        while (m_input_.wait_and_pop(item)) {
            emit status(QString("Processing Hashed results..."));
            Q_ASSERT(!item->hashes.empty());
            
            engine.addImage(item);

            // Push thumbnail request
            auto thumbReq = std::make_shared<ThumbnailRequest>();
            thumbReq->path = item->fileIdentity.path() + "/" + item->fileIdentity.name();
			thumbReq->rotation = item->fileIdentity.exif().orientation.value_or(1);
            thumbReq->width = settings::ThumbnailWidth;
            thumbReq->height = settings::ThumbnailWidth;
            m_thumbnailOutput_.push(std::move(thumbReq));

            m_items_.push_back(std::move(item));
            
            processedCount++;
            emit progress(processedCount, processedCount);

            if (timer.elapsed() > 500) {
                auto tempGroups = engine.getGroups();
                for (const auto& g : tempGroups) {
                    if (g.images.size() > 1) {
                        if (!m_emittedGroups_.contains(g.id)) {
                            emit groupAdded(g);
                            m_emittedGroups_.insert(g.id);
                            m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                        } else if (m_emittedSizes_[g.id] <static_cast<int>(g.images.size())) {
                            emit groupUpdated(g);
                            m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                        }
                    }
                }
                timer.restart();
            }
        }

        emit status(QString("Compiling final groups..."));
        
        auto groups = engine.getGroups();
        
        std::vector<ImageGroup> result;
        for (const auto& g : groups) {
            if (g.images.size() > 1) {
                result.push_back(g);
                if (!m_emittedGroups_.contains(g.id)) {
                    emit groupAdded(g);
                    m_emittedGroups_.insert(g.id);
                    m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                } else if (m_emittedSizes_[g.id] < static_cast<int>(g.images.size())) {
                    emit groupUpdated(g);
                    m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                }
            }
        }
        emit groupingFinished(result);
    }


    void ResultProcessor::onStop()
    {
        m_thumbnailOutput_.producer_done();
    }
}