#include "pipeline/stages/HashWorker.h"
#include "hashing/HashMethod.h"
#include "hashing/HashCatalog.h"
#include <QThread>
#include <qimagereader.h>
#include <qimage.h>

namespace photoboss {

    HashWorker::HashWorker(
        Queue<std::unique_ptr<DiskReadResult>>& inputQueue,
		Queue<std::shared_ptr<HashedImageResult>>& outputQueue,
        QObject* parent)
        : StageBase("HashWorker",parent)
        , m_input(inputQueue)
		, m_output(outputQueue)
        , m_hashMethods(std::move(HashCatalog::createAll()))
    {        
        if (m_hashMethods.empty()) {
            qWarning() << "HashWorker: No active hash methods provided.";
        } else {
            qDebug() << "HashWorker: Initialized with"
                     << m_hashMethods.size() << "hash methods.";
		}
    }

    void HashWorker::run()
    {        
        
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

            for (auto& hash : m_hashMethods) {
                if (hash.method->InputType() == HashInput::Bytes) {
                    try {
                        result->hashes.emplace(hash.method->key(), hash.method->compute(item->imageBytes));
                    }
                    catch (const std::exception& e) {
                        result->hashes.emplace(hash.method->key(), e.what());
                        result->source = HashSource::Error;
                        continue;
                    }
                }
            }

                qDebug() << "HashWorker: Processing image:" << item->fileIdentity.path();
                
                QImage img;

                if (!loadAndOrient(item->imageBytes, item->fileIdentity.exif().orientation.value_or(1), img)) {
                    qDebug() << "Image load failed" << item->fileIdentity.path();
                    result->source = HashSource::Error;
                }

                else {
                    for (auto& method : m_hashMethods) {
                        try {
                            result->hashes.emplace(method.method->key(), method.method->compute(PerceptualImage(img)));
                        }
                        catch (const std::exception& e) {
                            result->hashes.emplace(method.method->key(), e.what());
                            result->source = HashSource::Error;
                        }
                    }
                }
            
            m_output.emplace(std::move(result));
        }
    }
    bool HashWorker::loadAndOrient(const QByteArray& imageBytes, int orientation, QImage& image)
    {
        QImage img;
        if (!img.loadFromData(imageBytes))
            return false;
        QTransform t;

        switch (orientation) {
        case 2: // Mirror horizontal
            t.scale(-1, 1);
            t.translate(-img.width(), 0);
            break;

        case 3: // Rotate 180
            t.rotate(180);
            t.translate(-img.width(), -img.height());
            break;

        case 4: // Mirror vertical
            t.scale(1, -1);
            t.translate(0, -img.height());
            break;

        case 5: // Mirror horizontal + rotate 90 CCW
            t.rotate(90);       
            t.translate(0, -img.height());
            t.scale(-1, 1);
            break;

        case 6: // Rotate 90 CW
            t.rotate(90);
            t.translate(0, -img.height());
            break;

        case 7: // Mirror horizontal + rotate 90 CW
            t.rotate(-90);
            t.translate(-img.width(), 0);
            t.scale(-1, 1);
            break;

        case 8: // Rotate 270
            t.rotate(270);
            t.translate(-img.width(), 0);
            break;

        default: // 1 or unknown
            image = img;
            return true;
        }

        image = img.transformed(t, Qt::SmoothTransformation);
        return !image.isNull();
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
