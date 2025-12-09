#pragma once
#include <qdatetime.h>

namespace photoboss {
    struct ImageFileMetaData
    {
        QString path;
        quint64 size;
        QDateTime lastModified;
        QString suffix;
    };
    
    struct DiskReadResult {
        ImageFileMetaData meta;
        QByteArray imageBytes;
    };

    struct HashedImageResult {
        ImageFileMetaData meta;
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
    };
    struct Group {
        QString name;
        QString path;
    };
}