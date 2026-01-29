#include "stages/ResultProcessor.h"

namespace photoboss {

    ResultProcessor::ResultProcessor(
        Queue<std::shared_ptr<HashedImageResult>>& queue,
        QObject* parent)
        : PipelineStage(parent)
        , m_input(queue)
    {
    }

    void ResultProcessor::Run()
    {
        while (true) {
            std::shared_ptr<HashedImageResult> result;

            if (!m_input.wait_and_pop(result)) {
                break;
            }

            if (!result) continue;

            Q_ASSERT(!result->hashes.empty());

            emit imageHashed(result);
        }
    }

    void ResultProcessor::stop()
    {
        m_input.shutdown(); // triggers wait_and_pop to return false
    }
}
