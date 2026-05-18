#pragma once

#include <QString>

namespace photoboss {

class IDeletionStrategy {
public:
    virtual ~IDeletionStrategy() = default;
    virtual bool deleteFile(const QString& path) = 0;
};

} // namespace photoboss
