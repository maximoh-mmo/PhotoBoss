#include "pipeline/FactoryPipelineController.h"
#include "pipeline/PipelineFactory.h"
#include "util/StorageInfo.h"
#include "caching/SqliteHashCache.h"


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
}

FactoryPipelineController::~FactoryPipelineController()
{
    if (state() == Pipeline::PipelineState::Running) {
        stop();
    }
    delete m_pipeline_.get();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void FactoryPipelineController::start(const ScanRequest& request)
{
    if (state() != Pipeline::PipelineState::Stopped)
        return;

    SetPipelineState(Pipeline::PipelineState::Running);

    // Reset cumulative progress counters (used by UI)
    m_findProgress_ = 0;
    m_findTotal_ = 0;
    m_analyzeProgress_ = 0;
    m_analyzeTotal_ = 0;
    m_groupProgress_ = 0;
    m_groupTotal_ = 0;

    // obtain a fresh scan‑id (identical to legacy controller)

    createPipeline(request);
}

// ---------------------------------------------------------------------------
// stop() – shutdown queues and threads, but don't destroy the pipeline yet (wait for thumbnails to finish)
// ---------------------------------------------------------------------------
void FactoryPipelineController::stop()
{
    if (m_pipeline_ == nullptr || m_pipeline_->GetPipelineState() != Pipeline::PipelineState::Running)
        return;

    SetPipelineState(Pipeline::PipelineState::Stopping);

    if (m_pipeline_) {
        // discard any pending work
        m_pipeline_->clearQueues();
        m_pipeline_->requestShutdown();
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
    m_pipeline_ = PipelineFactory::create(cfg);

    // ------------------------------------------------------------------
    // 3️ Wire signals – everything is the same as in the legacy controller
    // ------------------------------------------------------------------


    // ---------------------------------------------------------------
    // CacheLookup (factory produces a worker that already owns a thread)
    // ---------------------------------------------------------------
    CacheLookup* cacheLookup = static_cast<CacheLookup*>(m_pipeline_->cacheLookupWorker.get());
    connect(cacheLookup, &CacheLookup::total,
            this, [this](int total){ m_analyzeTotal_ = total; emit analyzeTotal(total); });
    connect(cacheLookup, &CacheLookup::progress,
            this, [this](int current){ m_analyzeProgress_ = current; emit analyzeProgress(current); });
    connect(cacheLookup, &CacheLookup::canceled,
            this, [this](){ emit scanCanceled(); });
    // ---------------------------------------------------------------
    // ResultProcessor
    // ---------------------------------------------------------------
    ResultProcessor* resultProcessor = static_cast<ResultProcessor*>(m_pipeline_->resultProcessorWorker.get());
    connect(resultProcessor, &ResultProcessor::groupAdded,
            this, [this](int groupId){ emit groupAdded(groupId); });
    connect(resultProcessor, &ResultProcessor::groupUpdated,
            this, [this](int groupId){ emit groupUpdated(groupId); });
    connect(resultProcessor, &ResultProcessor::duplicateGroup,
            this, [this](int master, const std::vector<int>& duplicates){
                emit duplicateGroup(master, duplicates);
            });
    connect(resultProcessor, &ResultProcessor::total,
            this, [this](int total){ m_groupTotal_ = total; emit totalGroups(total); });
    connect(resultProcessor, &ResultProcessor::progress,
            this, [this](int current){ m_groupProgress_ = current; emit groupProgress(current); });
    // ---------------------------------------------------------------
    // CacheStore
    // ---------------------------------------------------------------
    CacheStore* cacheStore = static_cast<CacheStore*>(m_pipeline_->cacheStoreWorker.get());
    connect(cacheStore, &CacheStore::finished,
            this, [this](){ emit cacheStoreFinished(); });
    // ---------------------------------------------------------------
    // ThumbnailGenerator (multiple workers, same signals)
    // ---------------------------------------------------------------
    for (auto& workerPtr : m_pipeline_->thumbnailWorkers) {
        ThumbnailGenerator* tg = static_cast<ThumbnailGenerator*>(workerPtr.get());
        connect(tg, &ThumbnailGenerator::thumbnailReady,
                this, [this](int id, const QImage& img){ emit thumbnailReady(id, img); });
    }
    // ---------------------------------------------------------------
    // Phase updates – emit a generic signal for UI status bar
    // ---------------------------------------------------------------
    connect(m_pipeline_.get(), &Pipeline::phaseUpdate,
            this, [&](Pipeline::PipelinePhase phase){ emit phaseUpdate(phase); });

    // ---------------------------------------------------------------
    // 4️⃣ Start all threads
    // ---------------------------------------------------------------
    // Legacy scanner thread
    m_pipeline_->scannerThread.start();

    // Factory‑created threads (including cache‑lookup, disk‑reader, hash workers,
    // cache‑store, thumbnail workers, and result processor)
    m_pipeline_->cacheLookupThread.start();
    m_pipeline_->diskThread.start();
    for (auto& th : m_pipeline_->hashWorkerThreads) th.start();
    m_pipeline_->cacheStoreThread.start();
    for (auto& th : m_pipeline_->thumbnailThreads) th.start();
    m_pipeline_->resultThread.start();
}

// ---------------------------------------------------------------------------
// Private state helper – updates member and emits a signal
// ---------------------------------------------------------------------------
void FactoryPipelineController::SetPipelineState(Pipeline::PipelineState state)
{
    if (m_pipeline_ && m_pipeline_->GetPipelineState() == state) return;
    if (m_pipeline_) m_pipeline_->SetPipelineState(state);
    emit pipelineStateChanged(state);
}

} // namespace photoboss
