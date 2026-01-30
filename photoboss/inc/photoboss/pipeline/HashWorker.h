#pragma once

#include "hashing/HashMethod.h"
#include "hashing/HashRegistry.h"
#include "pipeline/stages/Pipeline.h"
#include "util/DataTypes.h"
#include "util/Queue.h"

namespace photoboss {
    class HashWorker : public StageBase {
        Q_OBJECT
    public:
        explicit HashWorker(
            Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
            Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
            const std::vector<HashRegistry::Entry>& activeMethods,
            QObject* parent = nullptr);

        void run() override;

    private:
        Queue<std::unique_ptr<DiskReadResult>>& m_input;
        Queue<std::shared_ptr<HashedImageResult>>& m_output;
        std::vector<std::unique_ptr<HashMethod>> m_byte_methods;
        std::vector<std::unique_ptr<HashMethod>> m_image_methods;

        // Inherited via StageBase
        void onStart() override;
        void onStop() override;
    };
}