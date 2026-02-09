#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/StageBase.h"
#include "pipeline/PipelineController.h"
#include "util/GroupTypes.h"
#include "hashing/HashMethod.h"

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
            QString id, 
            QObject* parent = nullptr
        );

    signals:
        void groupingFinished(const std::vector<ImageGroup> groups);
    
    protected:
        void run() override;

    private:
        Queue<std::shared_ptr<HashedImageResult>>& m_input;
        std::vector<std::shared_ptr<HashedImageResult>> m_items;
        
        // Inherited via StageBase
        void onStart() override {};
        void onStop() override {};
    };
}