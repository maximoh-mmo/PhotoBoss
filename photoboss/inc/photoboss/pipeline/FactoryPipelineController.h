#pragma once

#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include <atomic>

#include "util/Queue.h"
#include "types/DataTypes.h"
#include "types/GroupTypes.h"
#include "hashing/HashMethod.h"
#include "caching/IHashCache.h"
#include "pipeline/Pipeline.h"


namespace photoboss {

    class FactoryPipelineController : public QObject {
        Q_OBJECT

    public:

            explicit FactoryPipelineController(QObject* parent = nullptr);
        ~FactoryPipelineController() override;

        void start(const ScanRequest& request);
        void stop();
        Pipeline::PipelineState state() const { return m_pipeline_ ? m_pipeline_->GetPipelineState() : Pipeline::PipelineState::Stopped; }

    signals:
        void finalGroups(const std::vector<ImageGroup> groups);
        void groupAdded(const ImageGroup& group);
        void groupUpdated(const ImageGroup& group);
        void thumbnailReady(const ThumbnailResult& result);
        void status(const QString& message);
        void pipelineStateChanged(Pipeline::PipelineState state);
        void phaseUpdate(Pipeline::Phase phase, int cur, int tot);

    private:
        void createPipeline(const ScanRequest& request);
        void SetPipelineState(Pipeline::PipelineState state);

        std::unique_ptr<Pipeline> m_pipeline_;
        std::unique_ptr <IHashCache> m_cache_;
        std::vector<QThread*> m_hash_worker_threads_;
        std::vector<QThread*> m_thumbnail_worker_threads_;
        std::atomic<int> m_activeThumbnailWorkers_{ 0 };

        quint64 m_findProgress_ = 0;
        quint64 m_findTotal_ = 0;
        quint64 m_analyzeProgress_ = 0;
        quint64 m_analyzeTotal_ = 0;
        quint64 m_groupProgress_ = 0;
        quint64 m_groupTotal_ = 0;
	};

} // namespace photoboss
