#include "pipeline/stages/ThumbnailGenerator.h"
#include "caching/SqliteHashCache.h"
#include "util/OrientImage.h"
#include "util/ScopedTimer.h"
#include <QImageReader>
#include <QTransform>

namespace photoboss {

    ThumbnailGenerator::ThumbnailGenerator(
        Queue<ThumbnailRequestPtr>& input,
        quint64 scanId,
        QObject* parent
    ) : StageBase(parent), m_input_(input),
        m_cache_(std::make_unique<SqliteHashCache>(scanId))
    {
    }

    void ThumbnailGenerator::doRun()
    {
        ThumbnailRequestPtr request;
        while (m_input_.wait_and_pop(request)) {
            SCOPED_TIMER("ThumbnailGenerator");
            QImage img;

            // 1. Fastest path: forwarded from HashWorker (pre-decoded + already rotated by ImageLoader)
            if (request->preDecoded) {
                QSize targetSize(request->width, request->height);
                QSize scaledSize = request->preDecoded->size();
                scaledSize.scale(targetSize, Qt::KeepAspectRatio);
                img = request->preDecoded->scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            } else if (request->fileIdentity) {
                // 2. Second-fastest: persistent thumbnail cache (rotation already baked into pixels)
                auto cached = m_cache_->getThumbnail(
                    *request->fileIdentity, request->width, 0);
                if (cached) {
                    img = std::move(*cached);
                } else {
                    // 3. Slowest path: decode from disk
                    QImageReader reader(request->path);
                    if (!reader.canRead()) continue;

                    QSize originalSize = reader.size();
                    QSize targetSize(request->width, request->height);

                    bool swapped = (request->rotation >= 5 && request->rotation <= 8);
                    if (swapped) targetSize.transpose();

                    QSize scaledSize = originalSize;
                    scaledSize.scale(targetSize, Qt::KeepAspectRatio);
                    if (scaledSize.isValid())
                        reader.setScaledSize(scaledSize);

                    QImage rawImg = reader.read();
                    if (rawImg.isNull()) continue;

                    img = OrientImage(std::move(rawImg), request->rotation);

                    // Cache for next scan (rotation=0 since pixels are already oriented)
                    m_cache_->putThumbnail(
                        *request->fileIdentity, request->width, 0, img);
                }
            } else {
                // 3b. No cache available (fileIdentity missing), decode from disk
                QImageReader reader(request->path);
                if (!reader.canRead()) continue;

                QSize originalSize = reader.size();
                QSize targetSize(request->width, request->height);

                bool swapped = (request->rotation >= 5 && request->rotation <= 8);
                if (swapped) targetSize.transpose();

                QSize scaledSize = originalSize;
                scaledSize.scale(targetSize, Qt::KeepAspectRatio);
                if (scaledSize.isValid())
                    reader.setScaledSize(scaledSize);

                QImage rawImg = reader.read();
                if (rawImg.isNull()) continue;

                img = OrientImage(std::move(rawImg), request->rotation);
            }

            ThumbnailResult result;
            result.path = request->path;
            result.image = std::move(img);
            emit thumbnailReady(result);
        }
    }

    void ThumbnailGenerator::onStop()
    {
        emit workerFinished();
    }
}
