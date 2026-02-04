#pragma once
#include <QObject>
#include "util/Queue.h"
#include "util/DataTypes.h"
#include "pipeline/stages/Pipeline.h"
#include "pipeline/PipelineController.h"
#include "util/GroupTypes.h"
#include "hashing/HashMethod.h"

namespace photoboss {
    class ResultProcessor : public Sink<std::shared_ptr<HashedImageResult>>
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
        explicit ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
            QString id, QObject* parent = nullptr);

        // Inherited via Sink
        void consume(const std::shared_ptr<HashedImageResult>& item) override;
        void run() override;
    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> imageHash);
        void groupsReady(std::vector<ImageGroup>);
    private:
        void group_items();
        ImageGroup buildGroup(const GroupRep& rep);
        bool better(const ImageScore& a, const ImageScore& b);
        std::vector<ImageGroup> m_groups_;
        std::vector<std::shared_ptr<HashedImageResult>> m_results_;
        std::unique_ptr<HashMethod> m_pHash;
    };
}