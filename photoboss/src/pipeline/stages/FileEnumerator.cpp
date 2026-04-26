#include <QDirIterator>
#include <QFileInfo>
#include "pipeline/stages/FileEnumerator.h"

namespace photoboss {

FileEnumerator::FileEnumerator(
    ScanRequest request,
    Queue<std::shared_ptr<QStringList>>& outputQueue,
    QObject* parent)
    : StageBase("FileEnumerator", parent)
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

    QStringList batch;
    const int batchSize = 100;

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

        batch.push_back(path);
        ++count;

        if (batch.size() >= batchSize) {
            m_outputQueue_.emplace(std::make_shared<QStringList>(batch));
            batch.clear();
        }
    }

    if (!batch.isEmpty()) {
        m_outputQueue_.emplace(std::make_shared<QStringList>(batch));
    }

    emit progress(count, count);
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