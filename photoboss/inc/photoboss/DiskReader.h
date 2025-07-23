#pragma once

#include <list>
#include <memory>
#include <QImage>

#include "ImageScanner.h"  // For ImageFileMetaData

namespace photoboss {

    struct DiskReadResult {
        ImageFileMetaData meta;
        QByteArray imageBytes;
    };

    class DiskReader : public QObject {
        Q_OBJECT
    public:
        explicit DiskReader(QObject* parent = nullptr);

    public slots:
        void Start(const std::unique_ptr<std::list<ImageFileMetaData>>& files);
        void Cancel();

    signals:
        void ImageRead(const std::unique_ptr<DiskReadResult>& result);
        void Finished();
        void ReadProgress(int current, int total);

    private:
        std::atomic<bool> m_cancelled_;
    };
}