#include "pipeline/stages/HashWorker.h"
#include "util/AppSettings.h"
#include "util/ScopedTimer.h"
#include <QDebug>

namespace photoboss {

HashWorker::HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
                                     Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
                                     QObject* parent)
    : StageBase(parent)
    , m_inputQueue(inputQueue)
    , m_outputQueue(outputQueue)
    , m_imageLoader()
    , m_hashEngine(HashCatalog::createAll())
{
    // Register as producer for the downstream queue.
    m_outputQueue.register_producer();
}

HashWorker::~HashWorker() = default;

void HashWorker::run()
{
    while (true) {
        std::unique_ptr<DiskReadResult> item;
        if (!m_inputQueue.wait_and_pop(item)) {
            break;
        }
        SCOPED_TIMER("HashWorker");

        // Decode at thumbnail size so the QImage can be forwarded to
        // ThumbnailGenerator instead of requiring a second disk read.
        std::optional<QImage> img = m_imageLoader.load(*item, settings::ThumbnailWidth);

        // Compute hashes using both raw bytes and, if available, the QImage.
        // decodedImage is passed through to the result for ThumbnailGenerator.
        m_outputQueue.emplace(m_hashEngine.compute(*item, img));
    }
}

void HashWorker::onStop()
{
    // Signal that this worker will produce no more results.
    m_outputQueue.producer_done();
}

} // namespace photoboss::pipeline::factory
