#include "HashWorker.h"
#include "HashMethod.h"
#include <QThread>

namespace photoboss {

    HashWorker::HashWorker(Queue<std::unique_ptr<DiskReadResult>>& inputQueue, std::vector<std::shared_ptr<HashMethod>>& methods, QObject* parent)
		: QObject(parent), m_queue(inputQueue), m_hash_methods(methods)
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
        auto result = std::make_shared<HashedImageResult>();
        result->meta = imageData->meta;
        for (auto& method : m_hash_methods)
        {
			if (!method->isEnabled()) 
                continue;
            QString hash = method->computeHash(imageData->imageBytes);
            // Store the hash
            result->hashes.emplace(method->getName(), hash);
		}       

        emit image_hashed(result);
    }
}
