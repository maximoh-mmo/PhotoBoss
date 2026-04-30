#pragma once

#include <QObject>
#include <QThread>
#include <memory>
#include <optional>

#include "pipeline/StageBase.h"
#include "types/DataTypes.h"
#include "pipeline/stages/ImageLoader.h"
#include "pipeline/HashEngine.h"
#include "util/Queue.h"

namespace photoboss {

/**
 * Orchestrator used by the factory pipeline. It pulls DiskReadResult items from
 * the input queue, obtains a QImage via ImageLoader, delegates the actual hash
 * computation to HashEngine, and finally emits the HashedImageResult downstream.
 *
 * The class mirrors the public interface of the legacy HashWorker (run() and
 * onStop()) so that PipelineFactory can use it interchangeably.
 */
class HashWorker : public StageBase {
    Q_OBJECT
public:
    HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
                     Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
                     QObject* parent = nullptr);
    ~HashWorker() override;

    void run() override;
    void onStop() override;

private:
    Queue<std::unique_ptr<DiskReadResult>>& m_inputQueue;
    Queue<std::shared_ptr<HashedImageResult>>& m_outputQueue;
    ImageLoader m_imageLoader;
    HashEngine m_hashEngine;
};

} // namespace photoboss::pipeline::factory
