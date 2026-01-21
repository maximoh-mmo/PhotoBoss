#include <QDirIterator>
#include <QFileInfo>
#include "DirectoryScanner.h"

namespace photoboss {

    DirectoryScanner::DirectoryScanner(QObject* parent) : QObject(parent) {}

    DirectoryScanner::~DirectoryScanner() {}

    void DirectoryScanner::StartScan(const QString& directory, bool recursive) {
        m_cancelled_.store(false);
        emit status(QStringLiteral("Scanner: starting"));

        QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
        if (recursive) flags |= QDirIterator::Subdirectories;

        QStringList filters = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tiff" };

        QDirIterator it(directory, filters, QDir::Files | QDir::NoSymLinks, flags);

        // Send results in batches to avoid signal thrash
        constexpr int batch_size = 200;
        auto batch = std::make_shared<std::vector<Fingerprint>>();
        batch->reserve(batch_size);

        int count = 0;
        while (it.hasNext()) {
            if (m_cancelled_.load()) {
                emit status(QStringLiteral("Scanner: cancelled"));
                emit finished();
                return;
            }

            QString path = it.next();
            QFileInfo file_info(path);
            if (!file_info.isFile()) continue;

            Fingerprint fingerprint;
            fingerprint.path = file_info.filePath();
            fingerprint.size = static_cast<quint64>(file_info.size());
            fingerprint.modifiedTime = static_cast<quint64>(file_info.lastModified().toSecsSinceEpoch());
            fingerprint.format = file_info.suffix().toUpper();
            
            batch->push_back(std::move(fingerprint));
            ++count;

            if (static_cast<int>(batch->size()) >= batch_size) {
                emit fileBatchFound(batch);
                batch = std::make_shared<std::vector<Fingerprint>>();
                batch->reserve(batch_size);
            }

            // occasional status update
            if ((count & 0x3FF) == 0) {
                emit status(QStringLiteral("Scanner: scanned %1 files").arg(count));
            }
        }

        if (!batch->empty()) emit fileBatchFound(batch);

        emit status(QStringLiteral("Scanner: finished. Total %1 files").arg(count));
        emit finished();
    }

} // namespace photoboss
