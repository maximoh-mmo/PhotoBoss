#pragma once

#include "ui/IDeletionStrategy.h"
#include <QFile>

namespace photoboss {

class TrashDeletionStrategy : public IDeletionStrategy {
public:
    bool deleteFile(const QString& path) override
    {
        return QFile::moveToTrash(path);
    }
};

} // namespace photoboss
