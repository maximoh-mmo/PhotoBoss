#include "pipeline/stages/ResultProcessor.h"
#include "hashing/HashCatalog.h"
#include "pipeline/SimilarityEngine.h"

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        QString id,
        QObject* parent) :
        StageBase(std::move(id), parent),
        m_input(queue),
        m_items()
    {
    }

    void ResultProcessor::run() {
        std::shared_ptr<HashedImageResult> item;
        while (m_input.wait_and_pop(item)) {
            Q_ASSERT(!item->hashes.empty());
            m_items.push_back(std::move(item));
        }
        SimilarityEngine engine;
        auto groups = engine.group(m_items);

        int duplicateGroups = 0;
        int duplicateImages = 0;

        std::vector<ImageGroup> result;
        for (const auto& g : groups) {
            if (g.images.size() > 1) {
                result.push_back(g);
                ++duplicateGroups;
                duplicateImages += static_cast<int>(g.images.size());
            }
        }

        qDebug().noquote()
            << "\nFound"
            << duplicateGroups
            << "duplicate groups covering"
            << duplicateImages
            << "images.";

        emit groupingFinished(result);
    }
}