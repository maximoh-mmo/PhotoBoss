#include "pipeline/ResultProcessor.h"

namespace photoboss {
    ResultProcessor::ResultProcessor(Queue<std::shared_ptr<HashedImageResult>>& queue,
        QString id,
        QObject* parent) :
        Sink(queue, std::move(id), parent)
    {
    }

    void ResultProcessor::consume(const std::shared_ptr<HashedImageResult>& item)
    {
        Q_ASSERT(!item->hashes.empty());
        emit imageHashed(item);
        return;
    }
}