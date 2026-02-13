#include "pipeline/SimilarityEngine.h"
#include "hashing/HashCatalog.h"
#include <algorithm>
#include <unordered_map>

namespace photoboss {

    SimilarityEngine::SimilarityEngine(Config cfg)
        : m_cfg(cfg)
    {
        initHashes();
    }

    void SimilarityEngine::initHashes()
    {
        auto all = HashCatalog::createAll();

        for (auto& h : all) {
            const QString& key = h.method->key();

            if (key == "Perceptual Hash")
                m_hashes.push_back({ std::move(h.method), m_cfg.pHashWeight });
            else if (key == "Difference Hash")
                m_hashes.push_back({ std::move(h.method), m_cfg.dHashWeight });
            else if (key == "Average Hash")
                m_hashes.push_back({ std::move(h.method), m_cfg.aHashWeight });
            else if (key == "Aspect Ratio")
                m_hashes.push_back({ std::move(h.method), m_cfg.ratioWeight });
        }
    }

    std::vector<ImageGroup> SimilarityEngine::group(
        const std::vector<std::shared_ptr<HashedImageResult>>& images
    )
    {
        // ---------------------------
        // 1. Exact SHA grouping
        // ---------------------------
        std::unordered_map<QString, ExactGroup> exact;

        for (const auto& img : images) {
            if (!img || !img->hashes.count("SHA256"))
                continue;

            const QString& sha = img->hashes.at("SHA256");

            auto& g = exact[sha];
            g.sha = sha;

            g.images.push_back({
                img.get(),
                img->resolution,
                img->fileIdentity.size()
                });
        }

        // Select representative for each exact group
        for (auto& [_, g] : exact) {
            g.representative = selectRepresentative(g.images);
        }

        // ---------------------------
        // 2. Similarity clustering
        // ---------------------------
        std::vector<SimilarityGroup> clusters;

        for (auto& [_, eg] : exact) {
            bool placed = false;

            for (auto& cluster : clusters) {
                double sim = confidence(
                    *eg.representative->result,
                    *cluster.representative->result
                );

                if (sim >= m_cfg.strongThreshold) {
                    for (auto& img : eg.images)
                        cluster.members.push_back(&img);

                    placed = true;
                    break;
                }
            }

            if (!placed) {
                SimilarityGroup c;
                c.representative = eg.representative;

                for (auto& img : eg.images)
                    c.members.push_back(&img);

                clusters.push_back(std::move(c));
            }
        }

        // ---------------------------
        // 3. Build output groups
        // ---------------------------
        std::vector<ImageGroup> out;
        out.reserve(clusters.size());

        for (const auto& c : clusters) {
            out.push_back(buildGroup(c));
        }

        return out;
    }

    // ------------------------------------------------------------
    // Confidence calculation
    // ------------------------------------------------------------

    double SimilarityEngine::confidence(
        const HashedImageResult& a,
        const HashedImageResult& b
    ) const
    {
        double score = 0.0;
        double weightSum = 0.0;

        for (const auto& h : m_hashes) {
            const QString& key = h.method->key();

            if (!a.hashes.count(key) || !b.hashes.count(key))
                continue;

            double sim = h.method->compare(
                a.hashes.at(key),
                b.hashes.at(key)
            );

            score += sim * h.weight;
            weightSum += h.weight;
        }

        return weightSum > 0.0 ? score / weightSum : 0.0;
    }

    // ------------------------------------------------------------
    // Representative selection
    // ------------------------------------------------------------

    SimilarityEngine::ImageNode*
        SimilarityEngine::selectRepresentative(
            std::vector<ImageNode>& images
        ) const
    {
        Q_ASSERT(!images.empty());

        auto it = std::max_element(
            images.begin(),
            images.end(),
            [](const ImageNode& a, const ImageNode& b) {
                return SimilarityEngine::better(b, a);
            }
        );

        return &(*it);
    }

    bool SimilarityEngine::better(
        const ImageNode& a,
        const ImageNode& b
    )
    {
        const quint64 aPixels =
            static_cast<quint64>(a.resolution.width()) *
            static_cast<quint64>(a.resolution.height());

        const quint64 bPixels =
            static_cast<quint64>(b.resolution.width()) *
            static_cast<quint64>(b.resolution.height());

        if (aPixels != bPixels)
            return aPixels > bPixels;

        return a.fileSize > b.fileSize;
    }

    // ------------------------------------------------------------
    // Build ImageGroup
    // ------------------------------------------------------------

    ImageGroup SimilarityEngine::buildGroup(
        const SimilarityGroup& g
    ) const
    {
        ImageGroup out;
        out.bestIndex = 0;

        ImageNode* best = g.representative;

        for (size_t i = 0; i < g.members.size(); ++i) {
            const auto* img = g.members[i];

            ImageEntry e;
            e.path = img->result->fileIdentity.path()+"/"+img->result->fileIdentity.name();
            e.fileSize = img->fileSize;
            e.lastModified = img->result->fileIdentity.modifiedTime();
            e.resolution = img->resolution;
            e.format = img->result->fileIdentity.extension();
            e.isBest = (img == best);
            e.rotation = img->result->fileIdentity.exif().orientation.value_or(1);
            
            if (e.isBest)
                out.bestIndex = static_cast<int>(i);

            out.images.push_back(std::move(e));
        }

        return out;
    }

}
