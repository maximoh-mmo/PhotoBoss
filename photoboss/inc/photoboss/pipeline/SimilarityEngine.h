#pragma once

#include "util/DataTypes.h"
#include "util/GroupTypes.h"

namespace photoboss {
    class HashMethod;

    class HashScorer {
    public:
        virtual ~HashScorer() = default;
        virtual QString key() const = 0;
        virtual double compare(
            const HashedImageResult& a,
            const HashedImageResult& b
        ) const = 0;
    };

    class SimilarityEngine {
    public:
        struct Config {
            double strongThreshold = 0.97;
            double weakThreshold = 0.92;

            double pHashWeight = 0.60;
            double dHashWeight = 0.30;
            double aHashWeight = 0.05;
            double ratioWeight = 0.05;
        };

        explicit SimilarityEngine(Config cfg = {});

        std::vector<ImageGroup> group(
            const std::vector<std::shared_ptr<HashedImageResult>>& images,
            std::function<void(qint64 current, qint64 total)> progressCb = nullptr
        );

    private:
        struct ImageNode {
            HashedImageResult* result;
            QSize resolution;
            quint64 fileSize;
        };

        struct ExactGroup {
            QString sha;
            std::vector<ImageNode> images;
            ImageNode* representative;
        };

        struct SimilarityGroup {
            std::vector<ImageNode*> members;
            ImageNode* representative;
        };

    private:
        Config m_cfg;

        struct WeightedHash {
            std::unique_ptr<HashMethod> method;
            double weight;
        };

        std::vector<WeightedHash> m_hashes;


    private:
        void initHashes();

        double confidence(
            const HashedImageResult& a,
            const HashedImageResult& b
        ) const;

        ImageNode* selectRepresentative(
            std::vector<ImageNode>& images
        ) const;

        ImageGroup buildGroup(
            const SimilarityGroup& group
        ) const;

        static bool better(
            const ImageNode& a,
            const ImageNode& b
        );

        static double score(const ImageNode& img);
    };
}