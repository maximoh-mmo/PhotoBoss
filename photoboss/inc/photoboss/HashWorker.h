#pragma once

#include "DiskReader.h"  // For DiskReadResult

namespace photoboss {

    struct HashedImageResult {
        ImageFileMetaData meta;
        QString hash;  // SHA256, pHash, etc.
    };

    class HashWorker : public QObject {
        Q_OBJECT
    public:
        explicit HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue, QObject* parent = nullptr);
        void Cancel();

    public slots:
        void run();

    signals:
        void image_hashed(HashedImageResult* result);

    protected:
        void compute_hash(const std::unique_ptr<DiskReadResult>& imageData);
    private:
        Queue<std::unique_ptr<DiskReadResult>>& m_queue;
        bool b_cancelled = false;
    };
}
