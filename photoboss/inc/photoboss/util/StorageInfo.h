#pragma once
#include <QStorageInfo>
#include <QString>

namespace photoboss {

    class StorageInfo {
    public:
        static bool isFastStorage(const QString& path);
    };

}