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
        QElapsedTimer throttleTimer;
        throttleTimer.start();
        QElapsedTimer groupEmitTimer;
        groupEmitTimer.start();
        bool firstEmit = false;
         
        while (m_input_.wait_and_pop(item)) {
            Q_ASSERT(!item->hashes.empty());
             
            if(!firstEmit) {
                emit status(QString("Processing Hashed results..."));
                firstEmit = true;
            }
             
            engine.addImage(item);
            processedCount++;
             
            if (shouldEmitProgress(throttleTimer, settings::ResultProgressEmitIntervalMs)) {
                emit progress(processedCount, processedCount);
            }
             
            m_items_.push_back(std::move(item));
       
             
            if (groupEmitTimer.elapsed() > 500) {
                 auto tempGroups = engine.getGroups();
                 for (const auto& g : tempGroups) {
                     if (g.images.size() > 1) {
                         if (!m_emittedGroups_.contains(g.id)) {
                             emit groupAdded(g);
                             m_emittedGroups_.insert(g.id);
                             m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                             // Emit thumbnails for newly confirmed group
                              for (const auto& img : g.images) {
                                  // Only emit thumbnail if we haven't already requested one for this image
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
                         } else if (m_emittedSizes_[g.id] <static_cast<int>(g.images.size())) {
                             emit groupUpdated(g);
                             m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                             // Emit thumbnails for newly added images to existing group
                              int prevSize = m_emittedSizes_[g.id];
                              int newSize = static_cast<int>(g.images.size());
                              for (int i = prevSize; i < newSize; i++) {
                                  const auto& img = g.images[i];
                                  // Only emit thumbnail if we haven't already requested one for this image
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
                 }
                 groupEmitTimer.restart();
             }
         }
         
         emit progress(processedCount, processedCount);
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
                      // Emit thumbnails for newly confirmed group
                       for (const auto& img : g.images) {
                           // Only emit thumbnail if we haven't already requested one for this image
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
                  } else if (m_emittedSizes_[g.id] < static_cast<int>(g.images.size())) {
                      emit groupUpdated(g);
                      m_emittedSizes_[g.id] = static_cast<int>(g.images.size());
                      // Emit thumbnails for newly added images to existing group
                      int prevSize = m_emittedSizes_[g.id];
                      int newSize = static_cast<int>(g.images.size());
                      for (int i = prevSize; i < newSize; i++) {
                          const auto& img = g.images[i];
                          // Only emit thumbnail if we haven't already requested one for this image
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
          }
         emit groupingFinished(result);
     }
 

     void ResultProcessor::onStop()
     {
         m_thumbnailOutput_.producer_done();
     }
}