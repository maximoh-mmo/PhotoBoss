#include <QDirIterator>
#include <QFileInfo>
#include "pipeline/stages/FileEnumerator.h"

namespace photoboss {

FileEnumerator::FileEnumerator(
    ScanRequest request,
    Queue<FileIdentity>& outputQueue,
    QObject* parent)
    : StageBase(parent)
    , m_request_(std::move(request))
    , m_outputQueue_(outputQueue)
{
    m_outputQueue_.register_producer();
}

FileEnumerator::~FileEnumerator() {}

void FileEnumerator::run()
{
    emit status(QString("Enumerating Directory : " + m_request_.directory));

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (m_request_.recursive) {
        flags |= QDirIterator::Subdirectories;
    }

    QStringList filters = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tiff" };

    QDirIterator it(m_request_.directory, filters, QDir::Files | QDir::NoSymLinks, flags);

    int count = 0;

    while (it.hasNext()) {
        if (m_cancelled_.load()) {
            break;
        }

        QString path = it.next();
        QFileInfo fileInfo(path);

        if (!fileInfo.isFile()) {
            continue;
        }

        ++count;
        emit incrementProgress(1);
        m_outputQueue_.emplace(FileIdentity(
            fileInfo.fileName(),
            fileInfo.absolutePath(),
            fileInfo.suffix().toUpper(),
            static_cast<quint64>(fileInfo.size()),
            static_cast<quint64>(fileInfo.lastModified().toSecsSinceEpoch()),
            {} // EXIF parsed later in DiskReader
        ));
    }

    emit finalCount(count);
    emit status(QString("Enumerated %1 files in directory : %2")
        .arg(count)
        .arg(m_request_.directory));

    m_outputQueue_.producer_done();
}

void FileEnumerator::onStop()
{
    m_cancelled_.store(true);
}

}