#include "pipeline/FactoryPipelineController.h"
#include "pipeline/PipelineFactory.h"
#include "util/StorageInfo.h"

namespace photoboss {

    // ---------------------------------------------------------------------------
    // Helper: decide which storage strategy to use (SSD → Parallel, HDD → Sequential)
    // ---------------------------------------------------------------------------
    static PipelineFactory::StorageStrategy decideStrategy(const ScanRequest& req)
    {
        return StorageInfo::isFastStorage(req.directory)
            ? PipelineFactory::StorageStrategy::Parallel
            : PipelineFactory::StorageStrategy::Sequential;
    }

    // ---------------------------------------------------------------------------
    // Constructor / Destructor
    // ---------------------------------------------------------------------------
    FactoryPipelineController::FactoryPipelineController(QObject* parent)
        : QObject(parent)
    {
        m_uiQueue_ = std::make_unique<UiUpdateQueue>();
    }

    FactoryPipelineController::~FactoryPipelineController()
    {
        if (state() == Pipeline::PipelineState::Running) {
            stop();
        }
    }

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------
    void FactoryPipelineController::start(const ScanRequest& request)
    {
        if (state() != Pipeline::PipelineState::Stopped)
            return;

        // obtain a fresh scan‑id (identical to legacy controller)

        createPipeline(request);

		m_pipeline_->start();
    }

    // ---------------------------------------------------------------------------
    // stop() – shutdown queues and threads, but don't destroy the pipeline yet (wait for thumbnails to finish)
    // ---------------------------------------------------------------------------
    void FactoryPipelineController::stop()
    {
        if (m_pipeline_ == nullptr || m_pipeline_->State() != Pipeline::PipelineState::Running)
            return;

        if (m_pipeline_) {
            // discard any pending work
			m_pipeline_->stop();
        }
    }

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    void FactoryPipelineController::createPipeline(const ScanRequest& request)
    {
        // ------------------------------------------------------------------
        // 1️ Decide storage strategy and build the factory config
        // ------------------------------------------------------------------
        PipelineFactory::Config cfg{ request, decideStrategy(request) };

        // ------------------------------------------------------------------
        // 2️ Build the whole pipeline via the factory
        // ------------------------------------------------------------------
        m_pipeline_ = PipelineFactory::create(cfg, m_uiQueue_.get());
    }

} // namespace photoboss
