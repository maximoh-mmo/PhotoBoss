#pragma once
#include <qdatetime.h>
#include <qsize.h>
#include <qimage.h>
#include "types/FileIdentity.h"

namespace photoboss {

   enum class HashSource {
        Fresh,
        Cache,
        Error
    };

   enum class HashInput {
       Bytes,
       Image
   };

    struct DiskReadResult {
        FileIdentity fileIdentity;
        QByteArray imageBytes;

        DiskReadResult(FileIdentity id, QByteArray bytes)
            : fileIdentity(std::move(id)), imageBytes(std::move(bytes)) {
        }
    };

    struct HashedImageResult {
        FileIdentity fileIdentity;
        HashSource source;
        QDateTime cachedAt;
        QSize resolution;
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
        std::optional<QImage> decodedImage;

		// Constructor to initialize fileIdentity
        HashedImageResult(FileIdentity id,
            HashSource src = HashSource::Fresh,
            QDateTime time = QDateTime::currentDateTimeUtc(),
            QSize resolution = {0,0},
            std::map<QString, QString> hashMap = {},
            std::optional<QImage> decodedImg = {})
            : fileIdentity(std::move(id))
            , source(src)
            , cachedAt(time)
            , resolution(resolution)
            , hashes(std::move(hashMap))
            , decodedImage(std::move(decodedImg))
        { }
    };

    struct HashConfig {
        QString name;
        bool enabled;
    };

    struct ScanRequest {
        QString directory;
        bool recursive;
        ScanRequest(QString dir = {}, bool rec = false)
            : directory(std::move(dir)), recursive(rec) {
        }
	};

    struct ThumbnailRequest {
        QString path;
        int rotation;
        int width;
        int height;
        std::optional<QImage> preDecoded;
        std::optional<FileIdentity> fileIdentity;
    };

    using ThumbnailRequestPtr = std::shared_ptr<ThumbnailRequest>;

    struct ThumbnailResult {
        QString path;
        QImage image;
    };
}