#include "stages/HashWorker.h"
#include "hashing/HashRegistry.h"
#include <QThread>

namespace photoboss {

    HashWorker::HashWorker(
        Queue<std::shared_ptr<DiskReadResult>>& inputQueue,
		Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
        const std::vector<HashRegistry::Entry>& activeMethods,
        QObject* parent)
        : PipelineStage(parent)
        , m_input(inputQueue)
		, m_output(outputQueue)
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

            if (!m_input.wait_and_pop(item)) {
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
                qDebug() << "Image load failed" << item->fingerprint.path;
				continue;
            }

            auto result = std::make_shared<HashedImageResult>();
            result->source = HashedImageResult::HashSource::Fresh;
            result->fingerprint = item->fingerprint;

            for (auto& method : m_hash_methods) {
                try {
                    result->hashes.emplace(method->key(), method->computeHash(img));
                }
                catch (const std::exception& e) {
                    result->hashes.emplace(method->key(), e.what());
					result->source = HashedImageResult::HashSource::Error;
                    continue;
                }
			}
			m_output.emplace(result);
        }
    }
}
