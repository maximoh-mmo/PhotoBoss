#pragma once
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <vector>
#include <memory>


namespace photoboss
{

    struct Fingerprint
    {
    public:
        QString path;
        // File format (e.g., "JPEG", "PNG", etc.)
        QString format;
        // Fast Change Identifier (FCI) for quick change detection
        quint64 size = 0;
        quint64 modifiedTime = 0;
        // MD5 hash of the file contents
        QString md5;
        bool md5Computed = false;
    };

    using FingerprintBatchPtr = std::shared_ptr<std::vector<Fingerprint>>;

    struct DiskReadResult {
        Fingerprint fingerprint;
        QByteArray imageBytes;
    };

    struct HashedImageResult {
        Fingerprint fingerprint;
        enum class HashSource {
            Fresh,
            Cache,
            Error
        } source;
        QDateTime cachedAt;
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
    };
}