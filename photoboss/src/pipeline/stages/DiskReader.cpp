#include "pipeline/stages/DiskReader.h"
#include <QFile>
#include <QThread>
#include <QCryptographicHash>
#include "util/DataTypes.h"

namespace photoboss {

    DiskReader::DiskReader(
        Queue<FileIdentityBatchPtr>& input_queue,
        Queue<std::unique_ptr<DiskReadResult>>& queue,
        QObject* parent)
        : StageBase("DiskReader",parent), m_input_queue(input_queue), m_output_queue(queue) {
    }


    void DiskReader::run() {

        while (true) {
            FileIdentityBatchPtr batch;
            if (!m_input_queue.wait_and_pop(batch)) {
                break; // scanner finished
            }

            int total = static_cast<int>(batch->size());
            int current = 0;

            for (const auto& fileIdentity : *batch) {

                QFile file(fileIdentity.path());
                if (file.open(QIODevice::ReadOnly)) {
                    auto result = std::make_unique<DiskReadResult>(fileIdentity, file.readAll());
                    
                    if (!m_output_queue.push(std::move(result))) {
						qDebug() << "DiskReader: Output queue shutdown," << result->fileIdentity.path() << "dropped, stopping.";
                        return;
                    }
                }

                if (++current % 50 == 0 || current == total)
                    emit ReadProgress(current, total);
            }
        }
    }


    void DiskReader::onStart()
    {
        qDebug() << "diskreader registered";
        m_output_queue.register_producer();
       
    }
    void DiskReader::onStop()
    {
        qDebug() << "diskreader deregistered";
        m_output_queue.producer_done();
    }
}