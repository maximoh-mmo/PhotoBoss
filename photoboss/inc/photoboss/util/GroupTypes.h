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

    struct HashScore {
        QString key;
        double similarity; // 1.0 ? 0.0
    };

    struct SimilarityWeights {
        double pHash = 0.45;
        double dHash = 0.25;
        double aHash = 0.20;
        double ratio = 0.10;
    };
}