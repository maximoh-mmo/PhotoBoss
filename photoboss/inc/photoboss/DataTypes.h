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
    
    struct DiskReadResult {
        ImageFileMetaData meta;
        QByteArray imageBytes;
    };
}