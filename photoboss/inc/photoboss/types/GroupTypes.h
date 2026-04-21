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
        int rotation;

        bool isBest;
    };

    struct ImageGroup {
        quint64 id = 0;
        std::vector<ImageEntry> images;
        int bestIndex; // index into images
    };

    struct HashScore {
        QString key;
        double similarity; // 1.0 ? 0.0
    };
}