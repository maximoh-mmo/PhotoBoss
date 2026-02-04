#include <QDirIterator>
#include <QFileInfo>
#include "pipeline/DirectoryScanner.h"
#include "exif/ExifReader.h"

namespace photoboss {

    DirectoryScanner::DirectoryScanner(ScanRequest request,Queue<FileIdentityBatchPtr>& outputQueue, QObject* parent) :
        Source<FileIdentityBatchPtr>(outputQueue, "DirectoryScanner", parent),
		m_request_(std::move(request))
    {}

    DirectoryScanner::~DirectoryScanner() {}

    void DirectoryScanner::produce() {
        bool cancelled = false;

        emit status(QStringLiteral("Scanner: starting"));

        QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
        if (m_request_.recursive) {
            flags |= QDirIterator::Subdirectories;
        }

        QStringList filters = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tiff" };

        QDirIterator it(m_request_.directory, filters, QDir::Files | QDir::NoSymLinks, flags);

        constexpr int batch_size = 200;
        auto batch = std::make_shared<std::vector<FileIdentity>>();
        batch->reserve(batch_size);

        int count = 0;
        
        while (it.hasNext()) {
            if (m_cancelled_.load()) {
                cancelled = true;
				break;
            }

            QString path = it.next();
            QFileInfo file_info(path);
            if (!file_info.isFile()) continue;

            FileIdentity fileIdentity(
                path,
                file_info.suffix().toUpper(),
                static_cast<quint64>(file_info.size()), 
                static_cast<quint64>(file_info.lastModified().toSecsSinceEpoch()),
                exif::ExifReader::read(path)
            );
            
            batch->push_back(fileIdentity);
            ++count;

            if (static_cast<int>(batch->size()) >= batch_size) {
				m_output.emplace(batch);
                batch = std::make_shared<std::vector<FileIdentity>>();
                batch->reserve(batch_size);
            }

            // occasional status update
            if ((count & 0x3FF) == 0) {
                emit status(QStringLiteral("Scanner: scanned %1 files").arg(count));
            }
        }

        if (!batch->empty()) {
            m_output.emplace(batch);
        }
    }
}
