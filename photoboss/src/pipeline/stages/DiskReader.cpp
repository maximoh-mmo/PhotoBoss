#include "stages/DiskReader.h"
#include <QFile>
#include <QThread>
#include <QCryptographicHash>
#include "util/FileTypes.h"

namespace photoboss {

    DiskReader::DiskReader(
        Queue<FingerprintBatchPtr>& input_queue,
        Queue<std::unique_ptr<DiskReadResult>>& queue,
        QObject* parent)
        : PipelineStage(parent), m_input_queue(input_queue), m_output_queue(queue) {
    }


    void DiskReader::Run() {

        while (true) {
            FingerprintBatchPtr batch;
            if (!m_input_queue.wait_and_pop(batch)) {
                break; // scanner finished
            }

            int total = static_cast<int>(batch->size());
            int current = 0;

            for (const auto& fingerprint : *batch) {

                QFile file(fingerprint.path);
                if (file.open(QIODevice::ReadOnly)) {
                    auto result = std::make_unique<DiskReadResult>();
                    result->fingerprint = fingerprint;
                    result->imageBytes = file.readAll();
                    if (!result->fingerprint.md5Computed) {
                        // Compute MD5 if not already computed
                        result->fingerprint.md5 = QString(QCryptographicHash::hash(
                            result->imageBytes, QCryptographicHash::Md5).toHex());
                        result->fingerprint.md5Computed = true;
					}
                    if (!m_output_queue.push(std::move(result))) {
						qDebug() << "DiskReader: Output queue shutdown," << result->fingerprint.path << "dropped, stopping.";
                        return;
                    }
                }

                if (++current % 50 == 0 || current == total)
                    emit ReadProgress(current, total);
            }
        }
    }
}