#pragma once
#include <qdatetime.h>

namespace photoboss {

    enum class PipelineState {
        Stopped,
        Starting,
        Idle,
        Scanning,
        Stopping
    };

    enum class HashSource {
        Fresh,
        Cache,
        Error
    };

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

    struct DiskReadResult {
        Fingerprint fingerprint;
        QByteArray imageBytes;
    };

    struct HashedImageResult {
        Fingerprint fingerprint;
        HashSource source;
        QDateTime cachedAt;
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
    };

    struct CacheLookupResult {
        bool hit;
        HashedImageResult hashedImage; // valid only if hit == true
    };

    struct CacheQuery {
        Fingerprint fingerprint;
        QList<QString> requiredMethods; // e.g. ["md5", "phash"]
    };

    struct HashConfig {
        QString name;
        bool enabled;
    };

    using FingerprintBatchPtr = std::shared_ptr<std::vector<Fingerprint>>;

}