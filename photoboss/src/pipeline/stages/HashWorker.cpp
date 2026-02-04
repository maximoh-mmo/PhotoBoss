#include "pipeline/HashWorker.h"
#include "hashing/HashMethod.h"
#include <QThread>
#include <qimagereader.h>
#include <qimage.h>

namespace photoboss {

    HashWorker::HashWorker(
        Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
		Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
        const std::vector<HashRegistry::Entry>& activeMethods,
        QObject* parent)
        : StageBase("HashWorker",parent)
        , m_input(inputQueue)
		, m_output(outputQueue)
    {
        for (const auto& entry : activeMethods) {
            if (entry.factory()->InputType() == HashInput::Image) {
                m_image_methods.push_back(entry.factory());
            }
            else {
                m_byte_methods.push_back(entry.factory());
            }
		}
        
        if (m_image_methods.empty() && m_byte_methods.empty()) {
            qWarning() << "HashWorker: No active hash methods provided.";
        } else {
            qDebug() << "HashWorker: Initialized with"
                     << m_image_methods.size() + m_byte_methods.size() << "hash methods.";
		}
    }

    void HashWorker::run()
    {        
        if (m_image_methods.empty() && m_byte_methods.empty()) {
            qWarning() << "HashWorker: No active hash methods, thread will exit.";
            return;
        }

        const bool decodeRequired = !m_image_methods.empty();

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
            QImageReader reader(item->fileIdentity.path());
            qDebug() << "image resolution = " << reader.size();

            auto result = std::make_shared<HashedImageResult>(item->fileIdentity, HashSource::Fresh, 
                QDateTime::currentDateTimeUtc(), reader.size());

            for (auto& method : m_byte_methods) {
                try {
                    result->hashes.emplace(method->key(), method->compute(item->imageBytes));
                }
                catch (const std::exception& e) {
                    result->hashes.emplace(method->key(), e.what());
                    result->source = HashSource::Error;
                    continue;
                }
            }

            if (decodeRequired) {
                qDebug() << "HashWorker: Processing image:" << item->fileIdentity.path();
                QImage img;
                if (!img.loadFromData(item->imageBytes)) {
                    qDebug() << "Image load failed" << item->fileIdentity.path();
                    result->source = HashSource::Error;
                }

                else {
                    for (auto& method : m_image_methods) {
                        try {
                            result->hashes.emplace(method->key(), method->compute(PerceptualImage(img)));
                        }
                        catch (const std::exception& e) {
                            result->hashes.emplace(method->key(), e.what());
                            result->source = HashSource::Error;
                        }
                    }
                }
            }
            m_output.emplace(std::move(result));
        }
    }
    void HashWorker::onStart()
    {
        m_output.register_producer();
    }
    void HashWorker::onStop()
    {
        m_output.producer_done();
    }
}
