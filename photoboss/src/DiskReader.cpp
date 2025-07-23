#include "DiskReader.h"
#include <QFile>
#include <QThread>

namespace photoboss {

    DiskReader::DiskReader(Queue<std::unique_ptr<DiskReadResult>>& queue, QObject* parent)
        : QObject(parent), m_cancelled_(false), m_output_queue(queue) {
    }

    void DiskReader::Cancel() {
        m_cancelled_.store(true);
    }

    void DiskReader::Start(const std::unique_ptr<std::list<ImageFileMetaData>>& files) {
        if (!files) {
            emit Finished();
            return;
        }

        m_cancelled_.store(false);
        int total = static_cast<int>(files->size());
        int current = 0;

        for (const auto& meta : *files) {
            if (m_cancelled_) break;

            QFile file(meta.path);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray imageBytes = file.readAll();
                auto result = std::make_unique<DiskReadResult>();
                result->meta = meta;
                result->imageBytes = std::move(imageBytes);
                m_output_queue.push(std::move(result));
            }

            ++current;
            if (current % 50 == 0 || current == total)
                emit ReadProgress(current, total);

            QThread::msleep(1); // Small sleep to avoid hammering disk
        }
        emit Finished();
    }
}