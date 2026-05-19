#include <QFileInfo>
#include "pipeline/stages/ExifRead.h"
#include "exif/ExifParser.h"

namespace photoboss {

ExifRead::ExifRead(
    Queue<QString>& inputQueue,
    Queue<FileIdentity>& outputQueue,
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

    int totalProcessed = 0;

    while (true) {
        if (m_cancelled_.load()) {
            break;
        }

        QString path;
        if (!m_inputQueue_.wait_and_pop(path)) {
            break;
        }

        QFileInfo fileInfo(path);
        auto exif = exif::ExifParser::parse(path);

        ++totalProcessed;
        emit incrementProgress(1);
        m_outputQueue_.emplace(FileIdentity(
            fileInfo.fileName(),
            fileInfo.absolutePath(),
            fileInfo.suffix().toUpper(),
            static_cast<quint64>(fileInfo.size()),
            static_cast<quint64>(fileInfo.lastModified().toSecsSinceEpoch()),
            exif
        ));
    }

    emit status(QString("Metadata read complete for %1 files").arg(totalProcessed));

    m_outputQueue_.producer_done();
}

void ExifRead::onStop()
{
    m_cancelled_.store(true);
}

}