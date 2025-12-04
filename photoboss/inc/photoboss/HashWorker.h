#pragma once
#include "DiskReader.h"  // For DiskReadResult
#include "HashMethod.h"

namespace photoboss {

    class HashWorker : public QObject {
        Q_OBJECT
    public:
        explicit HashWorker(Queue<std::unique_ptr<DiskReadResult>>& queue,
            std::vector<std::shared_ptr<HashMethod>>& methods,
            QObject* parent = nullptr);
        void Cancel();

    public slots:
        void run();

    signals:
        void image_hashed(std::shared_ptr<HashedImageResult> result);

    protected:
        void compute_hash(const std::unique_ptr<DiskReadResult>& imageData);
    private:
        Queue<std::unique_ptr<DiskReadResult>>& m_queue;
        std::vector<std::shared_ptr<HashMethod>>& m_hash_methods;
        bool b_cancelled = false;
    };
}