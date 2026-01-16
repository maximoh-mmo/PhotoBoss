#pragma once
#include <qdatetime.h>

namespace photoboss {
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
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
    };
    
    struct Group {
        QString name;
        QString path;
    };

    struct HashConfig {
        QString name;
        bool enabled;
    };

    enum class PipelineState {
        Stopped,
        Starting,
        Idle,
        Scanning,
        Stopping
    };

    using LoadedImagePtr = std::shared_ptr<DiskReadResult>;
    using HashResultPtr = std::shared_ptr<HashedImageResult>;
    using FileMetaListPtr = std::shared_ptr<std::vector<Fingerprint>>;
}