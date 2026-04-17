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

    void SimilarityEngine::addImage(const std::shared_ptr<HashedImageResult>& img)
    {
        if (!img || !img->hashes.count("SHA256")) return;

        m_nodes.push_back({img.get(), img->resolution, img->fileIdentity.size()});
        ImageNode* node = &m_nodes.back();

        const QString& sha = img->hashes.at("SHA256");
        auto it = m_exactGroups.find(sha);

        if (it == m_exactGroups.end()) {
            ExactGroup eg;
            eg.sha = sha;
            eg.images.push_back(node);
            eg.representative = node;

            bool placed = false;
            for (auto& cluster : m_clusters) {
                double sim = confidence(*node->result, *cluster.representative->result);
                if (sim >= m_cfg.strongThreshold) {
                    cluster.members.push_back(node);
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                SimilarityGroup c;
                c.id = m_nextGroupId++;
                c.representative = node;
                c.members.push_back(node);
                m_clusters.push_back(std::move(c));
            }

            m_exactGroups.insert({sha, std::move(eg)});
        } else {
            ExactGroup& eg = it->second;
            eg.images.push_back(node);

            ImageNode* oldRep = eg.representative;
            bool newlyBetter = better(*node, *oldRep);
            if (newlyBetter) {
                eg.representative = node;
            }

            for (auto& cluster : m_clusters) {
                // Find cluster containing the ExactGroup
                if (std::find(cluster.members.begin(), cluster.members.end(), oldRep) != cluster.members.end()) {
                    cluster.members.push_back(node);
                    // Update representative of cluster if needed
                    if (newlyBetter && cluster.representative == oldRep) {
                        cluster.representative = node;
                    }
                    break;
                }
            }
        }
    }

    std::vector<ImageGroup> SimilarityEngine::getGroups() const
    {
        std::vector<ImageGroup> out;
        out.reserve(m_clusters.size());

        for (const auto& c : m_clusters) {
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
        double phashSim = 0.0;
        double dhashSim = 0.0;

        for (const auto& h : m_hashes) {
            const QString& key = h.method->key();

            if (!a.hashes.count(key) || !b.hashes.count(key))
                continue;

            double sim = h.method->compare(
                a.hashes.at(key),
                b.hashes.at(key)
            );

            if (key == "Perceptual Hash")
                phashSim = sim;

            if (key == "Difference Hash")
                dhashSim = sim;

            score += sim * h.weight;
            weightSum += h.weight;
        }

        // --------------------------------------------------
        // Hard Gate: if either perceptual or difference hashes 
        // are below acceptable thresholds return 0
        // --------------------------------------------------

        if (phashSim < 0.98 || dhashSim < 0.94) {
            return 0.0;
        }

        return weightSum > 0.0 ? score / weightSum : 0.0;
    }

    // ------------------------------------------------------------
    // Representative selection
    // ------------------------------------------------------------



    bool SimilarityEngine::better(
        const ImageNode& a,
        const ImageNode& b
    )
    {
        return score(a) > score(b);
    }

    // ------------------------------------------------------------
    // Image Score calculation
    // ------------------------------------------------------------

    double SimilarityEngine::score(const ImageNode& img)
    {
        const double pixels =
            static_cast<double>(img.resolution.width()) *
            static_cast<double>(img.resolution.height());

        double score = pixels;

        score += img.fileSize * 0.001;

        const QString ext = img.result->fileIdentity.extension().toLower();
        if (ext == "png") score *= 1.05;

        if (img.result->fileIdentity.exif().dateTimeOriginal)
            score *= 1.02;

        return score;
    }

    // ------------------------------------------------------------
    // Build ImageGroup
    // ------------------------------------------------------------

    ImageGroup SimilarityEngine::buildGroup(
        const SimilarityGroup& g
    ) const
    {
        ImageGroup out;
        out.id = g.id;
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