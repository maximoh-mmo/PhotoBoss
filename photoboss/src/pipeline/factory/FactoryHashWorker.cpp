#include "pipeline/factory/FactoryHashWorker.h"
#include <QDebug>

namespace photoboss::pipeline::factory {

FactoryHashWorker::FactoryHashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
                                     Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
                                     QObject* parent)
    : StageBase("FactoryHashWorker", parent)
    , m_inputQueue(inputQueue)
    , m_outputQueue(outputQueue)
    , m_imageLoader()
    , m_hashEngine(HashCatalog::createAll())
{
    // Register as producer for the downstream queue.
    m_outputQueue.register_producer();
}

FactoryHashWorker::~FactoryHashWorker() = default;

void FactoryHashWorker::run()
{
    while (true) {
        std::unique_ptr<DiskReadResult> item;
        if (!m_inputQueue.wait_and_pop(item)) {
            // Upstream shutdown – exit the loop.
            break;
        }

        // Decode the image (may fail).
        std::optional<QImage> img = m_imageLoader.load(*item);

        // Compute hashes using both raw bytes and, if available, the QImage.
        auto result = m_hashEngine.compute(*item, img);

        // Forward the result downstream.
        m_outputQueue.emplace(std::move(result));
    }
}

void FactoryHashWorker::onStop()
{
    // Signal that this worker will produce no more results.
    m_outputQueue.producer_done();
}

} // namespace photoboss::pipeline::factory
