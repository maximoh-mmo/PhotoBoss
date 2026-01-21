#include "HashWorker.h"
#include "HashMethod.h"
#include <QThread>

namespace photoboss {

    HashWorker::HashWorker(
        Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
        const std::vector<HashRegistry::Entry>& activeMethods,
        QObject* parent)
        : QObject(parent)
        , m_queue(inputQueue)
    {
        for (const auto& entry : activeMethods) {
            m_hash_methods.push_back(entry.factory());
		}
        
        if (m_hash_methods.empty()) {
            qWarning() << "HashWorker: No active hash methods provided.";
        } else {
            qDebug() << "HashWorker: Initialized with"
                     << m_hash_methods.size() << "hash methods.";
		}
    }

    void HashWorker::Run()
    {
        if (m_hash_methods.empty()) {
            qWarning() << "HashWorker: No active hash methods, thread will exit.";
            return;
        }
        while (true)
        {
            std::unique_ptr<DiskReadResult> item;

            if (!m_queue.wait_and_pop(item)) {
				qDebug() << "HashWorker: Queue shutdown detected, exiting.";
                break; // upstream shutdown
            }

            if (!item) { 
				qDebug() << "HashWorker: Received null input, skipping.";
                continue; 
            }

			qDebug() << "HashWorker: Processing image:" << item->fingerprint.path;
            QImage img;
            if (!img.loadFromData(item->imageBytes)) {
                emit imageHashError(item->fingerprint.path, "Image load failed");
				continue;
            }

            auto result = std::make_shared<HashedImageResult>();
            result->fingerprint = item->fingerprint;

            for (auto& method : m_hash_methods) {
                try {
                    result->hashes.emplace(method->key(), method->computeHash(img));
                }
                catch (const std::exception& e) {
                    emit imageHashError(item->fingerprint.path,
                        QString("Hash computation failed for method %1: %2")
                        .arg(method->key(), e.what()));
                    continue;
                }
			}
            emit imageHashed(result);
        }
    }
}
