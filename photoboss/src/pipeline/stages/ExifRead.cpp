#include <QFileInfo>
#include "pipeline/stages/ExifRead.h"
#include "exif/ExifParser.h"
#include "util/AppSettings.h"

namespace photoboss {

ExifRead::ExifRead(
    Queue<std::shared_ptr<QStringList>>& inputQueue,
    Queue<FileIdentityBatchPtr>& outputQueue,
    QObject* parent)
    : StageBase(parent)
    , m_inputQueue_(inputQueue)
    , m_outputQueue_(outputQueue)
{
    m_outputQueue_.register_producer();
}

ExifRead::~ExifRead() {}

void ExifRead::run()
{
    emit status("Reading image metadata...");

    int batchSize = settings::DirectoryScanBatchSize;
    auto batch = std::make_shared<std::vector<FileIdentity>>();
    batch->reserve(batchSize);

    int totalProcessed = 0;

    while (true) {
        if (m_cancelled_.load()) {
            break;
        }

        std::shared_ptr<QStringList> batchPtr;
        if (!m_inputQueue_.wait_and_pop(batchPtr)) {
            break;
        }

        for (const QString& path : *batchPtr) {
            QFileInfo fileInfo(path);
            auto exif = exif::ExifParser::parse(path);

            FileIdentity fileIdentity(
                fileInfo.fileName(),
                fileInfo.absolutePath(),
                fileInfo.suffix().toUpper(),
                static_cast<quint64>(fileInfo.size()),
                static_cast<quint64>(fileInfo.lastModified().toSecsSinceEpoch()),
                exif
            );

            batch->push_back(std::move(fileIdentity));
            ++totalProcessed;
        }

        if (static_cast<int>(batch->size()) >= batchSize) {
            m_outputQueue_.emplace(batch);
            batch = std::make_shared<std::vector<FileIdentity>>();
            batch->reserve(batchSize);
        }

        emit progress(totalProcessed, totalProcessed);
    }

    if (!batch->empty()) {
        m_outputQueue_.emplace(batch);
    }

    emit progress(totalProcessed, totalProcessed);
    emit status(QString("Metadata read complete for %1 files").arg(totalProcessed));

    m_outputQueue_.producer_done();
}

void ExifRead::onStop()
{
    m_cancelled_.store(true);
}

}