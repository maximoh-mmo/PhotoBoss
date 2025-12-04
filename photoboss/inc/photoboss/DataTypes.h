#pragma once
#include <qdatetime.h>

namespace photoboss {
    typedef struct
    {
        QString path;
        quint64 size;
        QDateTime lastModified;
        QString suffix;
    } ImageFileMetaData;
    
    typedef struct {
        ImageFileMetaData meta;
        QByteArray imageBytes;
    } DiskReadResult;

    typedef struct {
        ImageFileMetaData meta;
        std::map<QString, QString> hashes;  // SHA256, pHash, etc.
    } HashedImageResult;
}