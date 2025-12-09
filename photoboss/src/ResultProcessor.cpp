#include "ResultProcessor.h"

namespace photoboss {

    ResultProcessor::ResultProcessor(
        Queue<std::shared_ptr<HashedImageResult>>& queue,
        QObject* parent)
        : QObject(parent)
        , queue_(queue)
        , running_(true)
    {
    }

    void ResultProcessor::run()
    {
        while (true) {

            std::shared_ptr<HashedImageResult> result;
            queue_.pop(result);

            // IMPORTANT: re-check after wake-up
            if (!running_)
                break;

            // Process result
            emit hash_stored(result->meta.path);
        }
    }

    void ResultProcessor::handleResult(
        const std::shared_ptr<HashedImageResult>& result)
    {
        qDebug() << "Processing:" << result->meta.path;
    }

    void ResultProcessor::stop()
    {
        running_ = false;
        queue_.notify_all(); // wake pop() so the loop can exit
    }

}
