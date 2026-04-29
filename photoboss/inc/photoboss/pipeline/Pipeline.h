#pragma once
#include "util/Queue.h"
#include "types/DataTypes.h"
#include <qthread.h>
#include "StageBase.h"

namespace photoboss {
    class DirectoryScanner;
    class DiskReader;
    class ResultProcessor;
    class CacheLookup;
    class CacheStore;
    class FactoryHashWorker;
	class ThumbnailGenerator;
	class FileEnumerator;
	class ExifRead;

    class Pipeline : public QObject {
        Q_OBJECT
    public:
        enum class Phase { Find, Analyze, Group };
        Q_ENUM(Phase)
            enum class PipelineState { Stopped, Running, Stopping };
        Q_ENUM(PipelineState)

            explicit Pipeline(QObject* parent = nullptr);
		~Pipeline();

        void AddStage(StageBase* stage) { allStages.push_back(std::move(stage)); }
        void AddQueue(IQueue* queue) { allQueues.push_back(std::move(queue)); }
        void AddThread(QThread* thread) { allThreads.push_back(std::move(thread)); }

        void clearQueues();
        void requestShutdown();
        void SetPipelineState(PipelineState state) { this->state = state; }
        PipelineState GetPipelineState() const { return state; }
		Phase GetPhase() const { return currentPhase; }
		quint64 ScanId() const { return m_scanId_; }

    private:
        std::vector<StageBase*> allStages;
        std::vector<IQueue*> allQueues;
        std::vector<QThread*> allThreads;
		Phase currentPhase = Phase::Find;
		PipelineState state = PipelineState::Stopped;
        quint64 m_scanId_;
    };
}