#include "pipeline/SimilarityEngine.h"
#include "hashing/HashCatalog.h"
#include <algorithm>
#include <array>
#include <unordered_map>
#include <unordered_set>

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
                m_hashes_.push_back({ std::move(h.method), m_cfg.pHashWeight });
            else if (key == "Difference Hash")
                m_hashes_.push_back({ std::move(h.method), m_cfg.dHashWeight });
            else if (key == "Average Hash")
                m_hashes_.push_back({ std::move(h.method), m_cfg.aHashWeight });
            else if (key == "Aspect Ratio")
                m_hashes_.push_back({ std::move(h.method), m_cfg.ratioWeight });
        }
    }

    std::array<quint16, 4> SimilarityEngine::extractSubHashes(
        const QString& phashHex)
    {
        quint64 h = phashHex.toULongLong(nullptr, 16);
        return {{
            static_cast<quint16>(h & 0xFFFF),
            static_cast<quint16>((h >> 16) & 0xFFFF),
            static_cast<quint16>((h >> 32) & 0xFFFF),
            static_cast<quint16>((h >> 48) & 0xFFFF)
        }};
    }

    void SimilarityEngine::addImage(const std::shared_ptr<HashedImageResult>& img)
    {
        if (!img || !img->hashes.count("SHA256")) {
            return;
        }

        m_nodes_.push_back({img.get(), img->resolution, img->fileIdentity.size()});
        ImageNode* node = &m_nodes_.back();

        const QString& sha = img->hashes.at("SHA256");
        auto it = m_exactGroups_.find(sha);

        if (it == m_exactGroups_.end()) {
            ExactGroup eg;
            eg.sha = sha;
            eg.images.push_back(node);
            eg.representative = node;

            bool placed = false;
            const auto phashIt = img->hashes.find("Perceptual Hash");

            // Try inverted index lookup (pHash must exist and index must be non-empty)
            if (phashIt != img->hashes.end() && !m_subHashIndex_.empty()) {
                auto subs = extractSubHashes(phashIt->second);
                std::unordered_map<size_t, int> matchCount;
                for (auto sub : subs) {
                    auto idxIt = m_subHashIndex_.find(sub);
                    if (idxIt == m_subHashIndex_.end()) continue;
                    std::set<size_t> seenInBucket;
                    for (size_t ci : idxIt->second) {
                        if (seenInBucket.insert(ci).second)
                            matchCount[ci]++;
                    }
                }
                // Check candidates with >= 2 matching sub-hashes
                for (const auto& [ci, count] : matchCount) {
                    if (count >= 2 && ci < m_clusters_.size()) {
                        double sim = confidence(*node->result, *m_clusters_[ci].representative->result);
                        if (sim >= m_cfg.strongThreshold) {
                            m_clusters_[ci].members.push_back(node);
                            if (better(*node, *m_clusters_[ci].representative))
                                m_clusters_[ci].representative = node;
                            m_dirtyClusterIndices_.insert(ci);
                            placed = true;
                            break;
                        }
                    }
                }
            }

            // Fallback: scan all clusters only when pHash is missing
            if (!placed && phashIt == img->hashes.end()) {
                for (auto& cluster : m_clusters_) {
                    double sim = confidence(*node->result, *cluster.representative->result);
                    if (sim >= m_cfg.strongThreshold) {
                        cluster.members.push_back(node);
                        if (better(*node, *cluster.representative))
                            cluster.representative = node;
                        m_dirtyClusterIndices_.insert(static_cast<size_t>(&cluster - m_clusters_.data()));
                        placed = true;
                        break;
                    }
                }
            }

            if (!placed) {
                SimilarityGroup c;
                c.id = m_nextGroupId++;
                c.representative = node;
                c.members.push_back(node);
                m_clusters_.push_back(std::move(c));
                m_dirtyClusterIndices_.insert(m_clusters_.size() - 1);

                // Add new cluster to inverted index
                if (phashIt != img->hashes.end()) {
                    auto subs = extractSubHashes(phashIt->second);
                    size_t clusterIdx = m_clusters_.size() - 1;
                    for (auto sub : subs)
                        m_subHashIndex_[sub].push_back(clusterIdx);
                }
            }

            m_exactGroups_.insert({sha, std::move(eg)});
        } else {
            ExactGroup& eg = it->second;
            eg.images.push_back(node);

            ImageNode* oldRep = eg.representative;
            bool newlyBetter = better(*node, *oldRep);
            if (newlyBetter) {
                eg.representative = node;
            }

            for (auto& cluster : m_clusters_) {
                // Find cluster containing the ExactGroup
                if (std::find(cluster.members.begin(), cluster.members.end(), oldRep) != cluster.members.end()) {
                    cluster.members.push_back(node);
                    m_dirtyClusterIndices_.insert(static_cast<size_t>(&cluster - m_clusters_.data()));
                    // Update representative of cluster if needed
                    if (newlyBetter && cluster.representative == oldRep) {
                        cluster.representative = node;
                        // Add new rep's sub-hashes to inverted index
                        const auto& phashIt = img->hashes.find("Perceptual Hash");
                        if (phashIt != img->hashes.end()) {
                            auto subs = extractSubHashes(phashIt->second);
                            size_t ci = static_cast<size_t>(&cluster - m_clusters_.data());
                            for (auto sub : subs)
                                m_subHashIndex_[sub].push_back(ci);
                        }
                    }
                    break;
                }
            }
        }
    }

    std::vector<ImageGroup> SimilarityEngine::getGroups() const
    {
        std::vector<ImageGroup> out;
        out.reserve(m_clusters_.size());

        for (const auto& c : m_clusters_) {
            out.push_back(buildGroup(c));
        }

        return out;
    }

    SimilarityEngine::GroupDelta SimilarityEngine::getGroupDelta()
    {
        GroupDelta delta;

        for (size_t ci : m_dirtyClusterIndices_) {
            if (ci >= m_clusters_.size()) continue;
            const auto& cluster = m_clusters_[ci];

            bool wasMulti = m_previouslyMultiImageClusterIds.contains(cluster.id);
            bool isMulti = cluster.members.size() > 1;
            size_t prevSize = 0;
            auto it = m_previousClusterSizes.find(cluster.id);
            if (it != m_previousClusterSizes.end()) {
                prevSize = it->second;
            }
            if (!wasMulti && isMulti) {
                delta.newlyFormed.push_back(buildGroup(cluster));
            } else if (isMulti && cluster.members.size() > prevSize) {
                delta.grown.push_back(buildGroup(cluster));
            }

            if (isMulti) {
                m_previouslyMultiImageClusterIds.insert(cluster.id);
                m_previousClusterSizes[cluster.id] = cluster.members.size();
            }
        }

        m_dirtyClusterIndices_.clear();
        return delta;
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

        for (const auto& h : m_hashes_) {
            const QString& key = h.method->key();

            if (!a.hashes.count(key) || !b.hashes.count(key))
                continue;

            double sim = h.method->compare(
                a.hashes.at(key),
                b.hashes.at(key)
            );

            if (key == "Perceptual Hash" && sim < m_cfg.pHashGate)
                return 0.0;

            if (key == "Difference Hash" && sim < m_cfg.dHashGate)
                return 0.0;

            score += sim * h.weight;
            weightSum += h.weight;
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