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
#include "ui/UiUpdateQueue.h"

namespace photoboss {

    class FactoryPipelineController : public QObject {
        Q_OBJECT

    public:

            explicit FactoryPipelineController(QObject* parent = nullptr);
        ~FactoryPipelineController() override;

        void start(const ScanRequest& request);
        void stop();
        Pipeline::PipelineState state() const { return m_pipeline_ ? m_pipeline_->State() : Pipeline::PipelineState::Stopped; }

    private:
        void createPipeline(const ScanRequest& request);
        
		std::unique_ptr<UiUpdateQueue> m_uiQueue_;
        std::unique_ptr<Pipeline> m_pipeline_;
	};

} // namespace photoboss
