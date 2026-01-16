#include "ResultProcessor.h"

namespace photoboss {

    ResultProcessor::ResultProcessor(
        Queue<std::shared_ptr<HashedImageResult>>& queue,
        QObject* parent)
        : QObject(parent)
        , queue_(queue)
    {
    }

    void ResultProcessor::Run()
    {
        while (true) {

            std::shared_ptr<HashedImageResult> result;
            if (!queue_.wait_and_pop(result)) {
                break; // queue has been shutdown
            }

            if (result) {
                emit hash_stored(result->fingerprint.path);
                HandleResult(result);
            }
        }
    }

    void ResultProcessor::HandleResult(
        const std::shared_ptr<HashedImageResult>& result)
    {
        qDebug() << "Processing:" << result->fingerprint.path;
        // TODO: implement grouping/duplicate detection here
    }

    void ResultProcessor::stop()
    {
        queue_.shutdown(); // triggers wait_and_pop to return false
    }
}
