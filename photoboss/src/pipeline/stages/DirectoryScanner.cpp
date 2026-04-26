#include <QDirIterator>
#include <QFileInfo>
#include <QThread>
#include <QElapsedTimer>
#include "pipeline/stages/DirectoryScanner.h"
#include "exif/ExifTool.h"
#include "util/AppSettings.h"

namespace photoboss {

    DirectoryScanner::DirectoryScanner(ScanRequest request, Queue<FileIdentityBatchPtr>& outputQueue, QObject* parent) :
        StageBase("DirectoryScanner", parent),
        m_request_(std::move(request)),
        m_output_(outputQueue)
    {
        m_output_.register_producer();
        m_isFastStorage_ = StorageInfo::isFastStorage(m_request_.directory);
    }

    DirectoryScanner::~DirectoryScanner() {}

    void DirectoryScanner::run()
    {
        emit status(QString("Scanning Directory : " + m_request_.directory));

        if (m_cancelled_.load()) {
            emit progress(0, 0);
            onStop();
            return;
        }

        emit progress(0, 0);
        emit status("Counting files...");

        int totalCount = quickCountPhase();

        if (m_cancelled_.load()) {
            onStop();
            return;
        }

        emit progress(0, totalCount);
        emit status("Reading file metadata...");

        std::vector<FileIdentity> results;
        if (m_isFastStorage_) {
            results = processFilesParallel(m_filePaths_);
        } else {
            results = processFilesSequential(m_filePaths_);
        }

        if (m_cancelled_.load()) {
            onStop();
            return;
        }

        int batchSize = settings::DirectoryScanBatchSize;
        auto batch = std::make_shared<std::vector<FileIdentity>>();
        batch->reserve(batchSize);

        for (auto& fileIdentity : results) {
            batch->push_back(std::move(fileIdentity));

            if (static_cast<int>(batch->size()) >= batchSize) {
                m_output_.emplace(batch);
                batch = std::make_shared<std::vector<FileIdentity>>();
                batch->reserve(batchSize);
            }
        }

        if (!batch->empty()) {
            m_output_.emplace(batch);
        }

        emit progress(static_cast<int>(results.size()), totalCount);
        emit status(QString("Finished Scanning %1 files in directory : %2")
            .arg(results.size())
            .arg(m_request_.directory));

        onStop();
    }

    int DirectoryScanner::quickCountPhase()
    {
        QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
        if (m_request_.recursive) {
            flags |= QDirIterator::Subdirectories;
        }

        QStringList filters = { "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.gif", "*.webp", "*.tiff" };

        QDirIterator it(m_request_.directory, filters, QDir::Files | QDir::NoSymLinks, flags);
        m_filePaths_.clear();

        int count = 0;
        QElapsedTimer throttleTimer;
        throttleTimer.start();

        while (it.hasNext()) {
            if (m_cancelled_.load()) {
                return count;
            }

            QString path = it.next();
            QFileInfo fileInfo(path);
            if (!fileInfo.isFile()) continue;

            m_filePaths_.push_back(path);
            ++count;

            if (shouldEmitProgress(throttleTimer, 200)) {
                emit status(QString("Counting files... %1").arg(count));
            }
        }

        return count;
    }

    std::vector<FileIdentity> DirectoryScanner::processFilesSequential(
        const std::vector<QString>& filePaths)
    {
        std::vector<FileIdentity> results;
        results.reserve(filePaths.size());

        QElapsedTimer throttleTimer;
        throttleTimer.start();

        for (size_t i = 0; i < filePaths.size(); ++i) {
            if (m_cancelled_.load()) {
                break;
            }

            const QString& path = filePaths[i];
            QFileInfo fileInfo(path);

            auto exif = exif::ExifTool::parse(path);

            FileIdentity fileIdentity(
                fileInfo.fileName(),
                fileInfo.absolutePath(),
                fileInfo.suffix().toUpper(),
                static_cast<quint64>(fileInfo.size()),
                static_cast<quint64>(fileInfo.lastModified().toSecsSinceEpoch()),
                exif
            );

            results.push_back(std::move(fileIdentity));

            if (shouldEmitProgress(throttleTimer, settings::ScannerProgressEmitIntervalMs)) {
                emit progress(static_cast<int>(i + 1), static_cast<int>(filePaths.size()));
            }
        }

        return results;
    }

    std::vector<FileIdentity> DirectoryScanner::processFilesParallel(
        const std::vector<QString>& filePaths)
    {
        const int threadCount = qMax(1, QThread::idealThreadCount() - 1);
        std::vector<std::vector<FileIdentity>> threadResults(threadCount);

        for (int i = 0; i < threadCount; ++i) {
            threadResults[i].reserve(filePaths.size() / threadCount + 10);
        }

        QAtomicInt currentIndex(0);
        QElapsedTimer throttleTimer;
        throttleTimer.start();

        auto worker = [&](int threadId) {
            while (true) {
                int index = currentIndex.fetchAndAddOrdered(1);
                if (index >= static_cast<int>(filePaths.size())) {
                    break;
                }

                if (m_cancelled_.load()) {
                    break;
                }

                const QString& path = filePaths[index];
                QFileInfo fileInfo(path);

                auto exif = exif::ExifTool::parse(path);

                FileIdentity fileIdentity(
                    fileInfo.fileName(),
                    fileInfo.absolutePath(),
                    fileInfo.suffix().toUpper(),
                    static_cast<quint64>(fileInfo.size()),
                    static_cast<quint64>(fileInfo.lastModified().toSecsSinceEpoch()),
                    exif
                );

                threadResults[threadId].push_back(std::move(fileIdentity));

                if (shouldEmitProgress(throttleTimer, settings::ScannerProgressEmitIntervalMs)) {
                    int processed = 0;
                    for (const auto& vec : threadResults) {
                        processed += static_cast<int>(vec.size());
                    }
                    emit progress(processed, static_cast<int>(filePaths.size()));
                }
            }
        };

        std::vector<QThread*> threads;
        for (int i = 0; i < threadCount; ++i) {
            QThread* thread = QThread::create([=]() { worker(i); });
            thread->start();
            threads.push_back(thread);
        }

        for (QThread* thread : threads) {
            thread->wait();
            thread->deleteLater();
        }

        std::vector<FileIdentity> results;
        results.reserve(filePaths.size());

        for (auto& vec : threadResults) {
            for (auto& id : vec) {
                results.push_back(std::move(id));
            }
        }

        return results;
    }

    void DirectoryScanner::onStop()
    {
        m_output_.producer_done();
    }
}