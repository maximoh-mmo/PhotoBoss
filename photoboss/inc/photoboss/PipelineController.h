#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "Queue.h"
#include "HashWorker.h"

#include "DataTypes.h"
#include "HashMethod.h"

namespace photoboss {
    class DirectoryScanner;
    class DiskReader;
	class ResultProcessor;

    struct Pipeline {
        // Queues
        Queue<FingerprintBatchPtr> scanQueue;
        Queue<FingerprintBatchPtr> diskQueue;
        Queue<std::unique_ptr<DiskReadResult>> readQueue;
        Queue<std::shared_ptr<HashedImageResult>> resultQueue;

        // Threads
        QThread scannerThread;
        QThread readerThread;

        // Workers
        DirectoryScanner* scanner = nullptr;
        DiskReader* reader = nullptr;
        std::vector<HashWorker*> hashWorkers;

		Pipeline() = default;
        Pipeline(int scanQueueSize, int readQueueSize, int resultQueueSize, int diskQueueSize)
            : scanQueue(scanQueueSize), readQueue(readQueueSize), resultQueue(resultQueueSize), diskQueue(diskQueueSize){
		}
    };

    class PipelineController : public QObject
    {
        Q_OBJECT
    public:
        explicit PipelineController(
            QObject* parent = nullptr);

        ~PipelineController() override;

        void start();
        void startScan(const QString& folder, bool recursive);
        void stop();
		void restart();

        void initializeDefaultHashes();
        void updateActiveHashes(const std::set<QString>& enabledKeys);

    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> result);
		void status(const QString& message);
		void diskReadProgress(int current, int total);
        void pipelineStateChanged(PipelineState state);

    private:
        void createPipeline();
		void destroyPipeline();
    private:
        std::unique_ptr<Pipeline> m_pipeline_;
		PipelineState m_state_ = PipelineState::Stopped;
		void SetPipelineState(PipelineState state);

        std::vector<HashRegistry::Entry> m_active_hash_methods_;
		std::vector<QThread*> m_hash_worker_threads_;
    };
}