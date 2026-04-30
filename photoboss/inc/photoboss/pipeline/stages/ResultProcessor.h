#pragma once
#include <QObject>
#include "util/Queue.h"
#include "types/DataTypes.h"
#include "pipeline/StageBase.h"
#include "types/GroupTypes.h"
#include "hashing/HashMethod.h"

#include <QSet>
#include <QMap>

namespace photoboss {
    class ResultProcessor : public StageBase
    {
        Q_OBJECT

        struct InternalImage {
            HashedImageResult* result;
            QSize resolution;
            quint64 fileSize;
        };

        struct GroupRep {
            std::vector<InternalImage> images;
            QString pHash;
        };

        struct ImageScore {
            int pixels;
            quint64 fileSize;
        };
        using ExactMap = std::unordered_map<QString, std::vector<InternalImage>>;
     
    public:
        explicit ResultProcessor(
            Queue<std::shared_ptr<HashedImageResult>>& queue,
            Queue<ThumbnailRequestPtr>& thumbnailQueue,
            QObject* parent = nullptr
        );

    signals:
        void groupingFinished(const std::vector<ImageGroup> groups);
        void groupAdded(const ImageGroup& group);
        void groupUpdated(const ImageGroup& group);
    
    protected:
        void run() override;

    private:
        Queue<std::shared_ptr<HashedImageResult>>& m_input_;
        Queue<ThumbnailRequestPtr>& m_thumbnailOutput_;
        std::vector<std::shared_ptr<HashedImageResult>> m_items_;
        QSet<quint64> m_emittedGroups_;
        QMap<quint64, int> m_emittedSizes_;
        QSet<QString> m_thumbnailRequested_; // Track which images have had thumbnails requested
        
        // Inherited via StageBase
        void onStop() override;
    };
}