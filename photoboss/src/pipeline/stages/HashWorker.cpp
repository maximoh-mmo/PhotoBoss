#include "pipeline/stages/HashWorker.h"
#include "util/AppSettings.h"
#include "util/ScopedTimer.h"
#include <QDebug>

namespace photoboss {

HashWorker::HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
                                     Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
                                     QObject* parent)
    : StageBase(parent)
    , m_inputQueue_(inputQueue)
    , m_outputQueue_(outputQueue)
    , m_imageLoader_()
    , m_hashEngine_(HashCatalog::createAll())
{
    // Register as producer for the downstream queue.
    m_outputQueue_.register_producer();
}

HashWorker::~HashWorker() = default;

void HashWorker::doRun()
{
    while (true) {
        std::unique_ptr<DiskReadResult> item;
        if (!m_inputQueue_.wait_and_pop(item)) {
            break;
        }
        SCOPED_TIMER("HashWorker");

        // Decode at thumbnail size so the QImage can be forwarded to
        // ThumbnailGenerator instead of requiring a second disk read.
        std::optional<QImage> img = m_imageLoader_.load(*item, settings::ThumbnailWidth);

        // Compute hashes using both raw bytes and, if available, the QImage.
        // decodedImage is passed through to the result for ThumbnailGenerator.
        m_outputQueue_.emplace(m_hashEngine_.compute(*item, img));
    }
}

void HashWorker::onStop()
{
    // Signal that this worker will produce no more results.
    m_outputQueue_.producer_done();
}

} // namespace photoboss
