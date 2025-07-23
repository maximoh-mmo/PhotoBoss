#include "HashWorker.h"
#include <QCryptographicHash>
#include <QThread>

namespace photoboss {

    HashWorker::HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue, QObject* parent)
        : QObject(parent) , m_queue(inputQueue)
    {
    }

    void HashWorker::Cancel()
    {
        b_cancelled = true;
    }

    void HashWorker::run()
    {
        while (!b_cancelled)
        {
            std::unique_ptr<DiskReadResult> input;
            if (!m_queue.try_pop(input))
            {
                QThread::msleep(10);
                continue;
            }
            compute_hash(input);
        }
    }

    void HashWorker::compute_hash(const std::unique_ptr<DiskReadResult>& imageData)
    {
        QCryptographicHash hasher(QCryptographicHash::Md5);
        hasher.addData(imageData->imageBytes);
        QString hash = hasher.result().toHex();

        auto result = std::make_unique<HashedImageResult>();
        result->meta = imageData->meta;
        result->hash = hash;

        emit image_hashed(result.release());
    }
}
