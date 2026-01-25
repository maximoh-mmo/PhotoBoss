#pragma once

#include "hashing/HashMethod.h"
#include "pipeline/Pipeline.h"
#include "util/FileTypes.h"
#include "hashing/HashRegistry.h"
#include "util/Queue.h"

namespace photoboss {
    class HashWorker : public PipelineStage {
        Q_OBJECT
    public:
        explicit HashWorker(
            Queue<std::shared_ptr<DiskReadResult>>& inputQueue,
            Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
            const std::vector<HashRegistry::Entry>& activeMethods,
            QObject* parent = nullptr);

    public slots:
        void Run();

    private:
        Queue<std::shared_ptr<DiskReadResult>>& m_input;
        Queue<std::shared_ptr<HashedImageResult>>& m_output;
        std::vector<std::unique_ptr<HashMethod>> m_hash_methods; 
    };
}