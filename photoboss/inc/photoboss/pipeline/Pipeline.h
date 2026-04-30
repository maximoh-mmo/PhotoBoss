#pragma once
#include "util/Queue.h"
#include "types/DataTypes.h"
#include <qthread.h>
#include <memory>
#include "StageBase.h"

namespace photoboss {
    class DiskReader;
    class ResultProcessor;
    class CacheLookup;
    class CacheStore;
    class HashWorker;
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
        void AddQueue(std::unique_ptr<IQueue> queue) { allQueues.push_back(std::move(queue)); }
        void AddThread(QThread* thread) { allThreads.push_back(std::move(thread)); }

		void start();
		void stop();
        PipelineState State() const { return state; }
		Phase GetPhase() const { return currentPhase; }
		quint64 ScanId() const { return m_scanId_; }

    private:
        void clearQueues();
        void requestShutdown();
        std::vector<StageBase*> allStages;
        std::vector<std::unique_ptr<IQueue>> allQueues;
        std::vector<QThread*> allThreads;
		Phase currentPhase = Phase::Find;
		PipelineState state = PipelineState::Stopped;
        quint64 m_scanId_;
    };
}