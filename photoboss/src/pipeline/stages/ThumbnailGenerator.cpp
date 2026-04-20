#include "pipeline/stages/ThumbnailGenerator.h"
#include "util/OrientImage.h"
#include <QImageReader>
#include <QTransform>

namespace photoboss {

    ThumbnailGenerator::ThumbnailGenerator(
        Queue<ThumbnailRequestPtr>& input,
        QString id,
        QObject* parent
    ) : StageBase(std::move(id), parent), m_input_(input)
    {
    }

    void ThumbnailGenerator::run()
    {
        ThumbnailRequestPtr request;
        while (m_input_.wait_and_pop(request)) {
            QImageReader reader(request->path);
            if (!reader.canRead()) continue;

            QSize originalSize = reader.size();
            QSize targetSize(request->width, request->height);

            // If rotation swaps dimensions, we must swap the scaled size target to 
            // maintain aspect ratio during the sub-sampled load.
            bool swapped = (request->rotation >= 5 && request->rotation <= 8);
            if (swapped) {
                targetSize.transpose();
            }

            // Calculate best fit size for decoder
            QSize scaledSize = originalSize;
            scaledSize.scale(targetSize, Qt::KeepAspectRatio);
            
            if (scaledSize.isValid()) {
                reader.setScaledSize(scaledSize);
            }

            QImage img = reader.read();
            if (img.isNull()) continue;

            // Apply orientation transformation via utility
            img = OrientImage(std::move(img), request->rotation);

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
