#include "ImageScanner.h"
#include <qdiriterator.h>
#include <qelapsedtimer.h>

namespace photoboss
{
    ImageScanner::ImageScanner(QObject* parent) : QObject(parent)
        , m_cancelled_(false)
    {
    }

    void ImageScanner::StartScan(const QString& directoryPath, const bool recursive)
    {
        m_cancelled_.store(false);
        std::unique_ptr<std::list<ImageFileMetaData>> imageFiles = std::make_unique<std::list<ImageFileMetaData>>();
        QDirIterator iterator = QDirIterator(
            directoryPath,
            QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.gif" << "*.bmp",
            QDir::Files | QDir::NoSymLinks,
            recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        int count = 0;

		QElapsedTimer timer; // Timer to manage progress updates
        timer.start();

        while (iterator.hasNext())
        {
            if (m_cancelled_.load())
            {
                emit scanCancelled();
                return;
            }

            count++;
            if (timer.elapsed() > 50) {
                emit fileCount(count);
                timer.restart();
            }
            QString filePath = iterator.next();
            QFileInfo fileInfo(filePath);
            if (fileInfo.isFile())
            {
                ImageFileMetaData metaData;
                metaData.path = filePath;
                metaData.size = fileInfo.size();
                metaData.lastModified = fileInfo.lastModified();
                metaData.suffix = fileInfo.suffix().toLower();
                imageFiles->push_back(metaData);
            }
        }
        emit fileCount(count);
        emit filePathsCollected(imageFiles);
    }

    void ImageScanner::Cancel()
    {
        m_cancelled_.store(true);
    }
}
