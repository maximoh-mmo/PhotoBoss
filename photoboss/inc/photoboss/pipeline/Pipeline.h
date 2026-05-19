#pragma once
#include "util/Queue.h"
#include "util/StageMetrics.h"
#include "types/DataTypes.h"
#include <QThread>
#include <memory>
#include "StageBase.h"

namespace photoboss {
    class Pipeline : public QObject {
        Q_OBJECT

    public:
        enum class Phase { Find, Analyze, Group };
        Q_ENUM(Phase)
            enum class PipelineState { Stopped, Running, Stopping };
        Q_ENUM(PipelineState)

            explicit Pipeline(QObject* parent = nullptr);
		~Pipeline();

        void addStage(StageBase* stage) { m_allStages_.push_back(std::move(stage)); }
        void addQueue(std::unique_ptr<IQueue> queue) { m_allQueues_.push_back(std::move(queue)); }
        void addThread(QThread* thread);

		void start();
		void stop();
        PipelineState state() const { return m_state_; }
		Phase getPhase() const { return m_currentPhase_; }
		quint64 scanId() const { return m_scanId_; }

    signals:
        void stateChanged(PipelineState state);

    private:
        void clearQueues();
        void requestShutdown();
        void onThreadFinished();
        std::vector<StageBase*> m_allStages_;
        std::vector<std::unique_ptr<IQueue>> m_allQueues_;
        std::vector<QThread*> m_allThreads_;
        int m_runningThreads_ = 0;
		Phase m_currentPhase_ = Phase::Find;
		PipelineState m_state_ = PipelineState::Stopped;
        quint64 m_scanId_;
    };
}