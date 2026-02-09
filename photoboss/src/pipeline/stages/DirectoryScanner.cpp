#include <QDirIterator>
#include <QFileInfo>
#include "pipeline/stages/DirectoryScanner.h"
#include "exif/ExifReader.h"

namespace photoboss {

    DirectoryScanner::DirectoryScanner(ScanRequest request,Queue<FileIdentityBatchPtr>& outputQueue, QObject* parent) :
        StageBase("DirectoryScanner", parent),
		m_request_(std::move(request)),
        m_output(outputQueue)
    {}

    DirectoryScanner::~DirectoryScanner() {}

    void DirectoryScanner::run() {
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
            auto exif = exif::ExifReader::read(path);

            FileIdentity fileIdentity(
                path,
                file_info.suffix().toUpper(),
                static_cast<quint64>(file_info.size()), 
                static_cast<quint64>(file_info.lastModified().toSecsSinceEpoch()),
                exif
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
    void DirectoryScanner::onStart()
    {
        m_output.register_producer();
    }
    void DirectoryScanner::onStop()
    {
        m_output.producer_done();
    }
}
