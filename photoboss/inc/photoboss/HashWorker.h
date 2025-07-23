#pragma once

#include "DiskReader.h"  // For DiskReadResult

namespace photoboss {

    struct HashedImageResult {
        ImageFileMetaData meta;
        QString hash;  // SHA256, pHash, etc.
    };

    class HashWorker : public QObject {
        Q_OBJECT
    public:
        explicit HashWorker(QObject* parent = nullptr);

    public slots:
        void computeHash(std::unique_ptr<DiskReadResult> imageData);

    signals:
        void imageHashed(std::unique_ptr<HashedImageResult> result);
    };
}
