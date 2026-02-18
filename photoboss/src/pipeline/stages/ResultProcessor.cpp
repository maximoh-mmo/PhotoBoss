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
			emit status(QString("Processing Hashed results..."));
            Q_ASSERT(!item->hashes.empty());
            m_items.push_back(std::move(item));
            emit progress(static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
        }
        SimilarityEngine engine;
        auto groups = engine.group(m_items);
        emit status(QString("Grouping results..."));
        
        std::vector<ImageGroup> result;
        for (const auto& g : groups) {
            if (g.images.size() > 1) {
                result.push_back(g);
            }
        }
        emit groupingFinished(result);
    }
}