#pragma once
#include <qdatetime.h>
#include <qsize.h>

namespace photoboss {

    struct ImageEntry {
        QString path;
        quint64 fileSize;
        quint64 lastModified;
        QSize resolution;
        QString format;

        bool isBest;
    };

    struct ImageGroup {
        std::vector<ImageEntry> images;
        int bestIndex; // index into images
    };

}