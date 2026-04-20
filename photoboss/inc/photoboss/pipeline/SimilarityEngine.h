#pragma once

#include "util/DataTypes.h"
#include "util/GroupTypes.h"
#include "util/AppSettings.h"

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
            double strongThreshold = settings::SimilarityStrongThreshold;
            double weakThreshold = settings::SimilarityWeakThreshold;

            // Hard gate thresholds - images must exceed BOTH to be considered similar
            double pHashGate = settings::SimilarityPHashGate;
            double dHashGate = settings::SimilarityDHashGate;

            double pHashWeight = 0.60;
            double dHashWeight = 0.30;
            double aHashWeight = 0.05;
            double ratioWeight = 0.05;
        };

        explicit SimilarityEngine(Config cfg = {});

        void addImage(const std::shared_ptr<HashedImageResult>& img);

        std::vector<ImageGroup> getGroups() const;

    private:
        struct ImageNode {
            HashedImageResult* result;
            QSize resolution;
            quint64 fileSize;
        };

        struct ExactGroup {
            QString sha;
            std::vector<ImageNode*> images;
            ImageNode* representative;
        };

        struct SimilarityGroup {
            quint64 id = 0;
            std::vector<ImageNode*> members;
            ImageNode* representative;
        };

    private:
        Config m_cfg;
        quint64 m_nextGroupId = 1;

        std::list<ImageNode> m_nodes;
        std::unordered_map<QString, ExactGroup> m_exactGroups;
        std::vector<SimilarityGroup> m_clusters;

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