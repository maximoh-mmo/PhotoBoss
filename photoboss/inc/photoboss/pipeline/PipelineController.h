#pragma once
#include <QObject>
#include <QThread>
#include <memory>
#include <vector>
#include "Queue.h"
#include "HashWorker.h"
#include "DataTypes.h"
#include "HashMethod.h"
#include "Pipeline.h"
#include "NullHashCache.h"

namespace photoboss {

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
        std::unique_ptr <NullHashCache> m_cache_;
		PipelineState m_state_ = PipelineState::Stopped;
		void SetPipelineState(PipelineState state);

        std::vector<HashRegistry::Entry> m_active_hash_methods_;
		std::vector<QThread*> m_hash_worker_threads_;
    };
}