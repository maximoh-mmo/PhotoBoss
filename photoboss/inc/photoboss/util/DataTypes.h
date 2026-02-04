#pragma once
#include <qdatetime.h>
#include <qsize.h>
#include "util/FileIdentity.h"

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

		// Constructor to initialize fileIdentity
        HashedImageResult(FileIdentity id,
            HashSource src = HashSource::Fresh,
            QDateTime time = QDateTime::currentDateTimeUtc(),
            QSize resolution = {0,0},
            std::map<QString, QString> hashMap = {})
            : fileIdentity(std::move(id))
            , source(src)
            , cachedAt(time)
            , resolution(resolution)
            , hashes(std::move(hashMap))
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

    using FileIdentityBatchPtr = std::shared_ptr<std::vector<FileIdentity>>;
}