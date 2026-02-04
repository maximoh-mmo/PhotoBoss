#include "pipeline/ResultProcessor.h"

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        const std::vector<HashRegistry::Entry>& activeMethods,
        QString id,
        QObject* parent) :
        Sink(queue, std::move(id), parent),
        m_results_()
    {
        for (const auto& entry : activeMethods) {
            if (entry.key == "Perceptual Hash") {
                m_pHash = entry.factory();
                break;
            }
        }

        Q_ASSERT(m_pHash && "Perceptual Hash method not available");
    }

    void ResultProcessor::consume(const std::shared_ptr<HashedImageResult>& item)
    {
        Q_ASSERT(!item->hashes.empty());
        m_results_.push_back(std::move(item));
        return;
    }

    void ResultProcessor::run() {
        std::shared_ptr<HashedImageResult> item;
        while (m_input.wait_and_pop(item)) {
            consume(item);
        }
        group_items();
    }

    void ResultProcessor::group_items() 
    {
        ExactMap exactGroups;
        std::vector<GroupRep> perceptualGroups;
        
        for (auto& r : m_results_) {
            const auto& sha = r->hashes.at("SHA256");
            if (r->hashes.count("SHA256") <= 0)
                continue;

            if (r->hashes.count("Perceptual Hash") <= 0)
                continue;
            exactGroups[sha].push_back({
                r.get(),
                r->resolution,
                r->fileIdentity.size()
                });
        }

        constexpr double SIM_THRESHOLD = 0.95;

        for (auto& [sha, images] : exactGroups) {
            QString repHash = images[0].result->hashes.at("Perceptual Hash");
            bool merged = false;

            for (auto& pg : perceptualGroups) {
                double sim = m_pHash->compare(repHash, pg.pHash);
                if (sim >= SIM_THRESHOLD) {
                    pg.images.insert(pg.images.end(),
                        images.begin(), images.end());
                    merged = true;
                    break;
                }
            }

            if (!merged) {
                perceptualGroups.push_back({ images, repHash });
            }
        }

        m_groups_.clear();

        for (const auto& rep : perceptualGroups) {
            m_groups_.push_back(buildGroup(rep));
        }

        for (const auto& pg : perceptualGroups) {
            if (pg.images.size() <= 1)
                continue;

            qDebug().noquote() << "\n=== Duplicate Group ("
                << pg.images.size()
                << " images ) ===";

            for (const auto& img : pg.images) {
                const auto* r = img.result;

                qDebug().noquote()
                    << "  "
                    << r->fileIdentity.path()
                    << "|"
                    << img.resolution.width() << "x" << img.resolution.height()
                    << "|"
                    << r->fileIdentity.size();
            }
        }
    }

    ImageGroup ResultProcessor::buildGroup(const GroupRep& rep)
    {
        ImageGroup group;
        group.bestIndex = 0;

        ImageScore bestScore{ 0, 0 };

        for (size_t i = 0; i < rep.images.size(); ++i) {
            const auto& img = rep.images[i];

            ImageEntry entry;
            entry.path = img.result->fileIdentity.path();
            entry.fileSize = img.result->fileIdentity.size();
            entry.lastModified = img.result->fileIdentity.modifiedTime();
            entry.resolution = img.resolution;
            entry.format = img.result->fileIdentity.extension();
            entry.isBest = false;

            ImageScore s{
                entry.resolution.width() * entry.resolution.height(),
                entry.fileSize
            };

            if (i == 0 || better(s, bestScore)) {
                bestScore = s;
                group.bestIndex = static_cast<int>(i);
            }

            group.images.push_back(std::move(entry));
        }

        group.images[group.bestIndex].isBest = true;
        return group;
    }

    bool ResultProcessor::better(const ImageScore& a, const ImageScore& b)
    {
        if (a.pixels != b.pixels) return a.pixels > b.pixels;
        return a.fileSize > b.fileSize;
    }
}