#pragma once
#include "DiskReader.h"  // For DiskReadResult
#include "HashMethod.h"

namespace photoboss {

    class HashWorker : public QObject {
        Q_OBJECT
    public:
        explicit HashWorker(
            Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
            Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
            const std::vector<HashRegistry::Entry>& activeMethods,
            QObject* parent = nullptr);

    public slots:
        void Run();

    private:
        Queue<std::unique_ptr<DiskReadResult>>& m_input;
        Queue<std::shared_ptr<HashedImageResult>>& m_output;
        std::vector<std::unique_ptr<HashMethod>> m_hash_methods; 
    };
}