#pragma once
#include "DiskReader.h"  // For DiskReadResult
#include "HashMethod.h"

namespace photoboss {

    class HashWorker : public QObject {
        Q_OBJECT
    public:
        explicit HashWorker(
            Queue<std::unique_ptr<DiskReadResult>>& queue,
            const std::vector<HashRegistry::Entry>& activeMethods,
            QObject* parent = nullptr);

    public slots:
        void Run();

    signals:
        void imageHashed(std::shared_ptr<HashedImageResult> result);
		void imageHashError(const QString& path, const QString& error);

    private:
        Queue<std::unique_ptr<DiskReadResult>>& m_queue;
        std::vector<std::unique_ptr<HashMethod>> m_hash_methods; 
    };
}