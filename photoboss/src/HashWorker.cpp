#include "HashWorker.h"
#include <QCryptographicHash>

namespace photoboss {

    HashWorker::HashWorker(QObject* parent)
        : QObject(parent) {
    }

    void HashWorker::computeHash(std::unique_ptr<DiskReadResult> imageData)
    {
        QCryptographicHash hasher(QCryptographicHash::Md5);
        hasher.addData(imageData->imageBytes);
        QString hash = hasher.result().toHex();

        auto result = std::make_unique<HashedImageResult>();
        result->meta = imageData->meta;
        result->hash = hash;

        emit imageHashed(std::move(result));
    }
}