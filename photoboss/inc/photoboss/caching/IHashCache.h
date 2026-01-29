#pragma once
#include <optional>
#include "util/DataTypes.h"

namespace photoboss {
    class IHashCache {

    public:
        virtual ~IHashCache() = default;

        virtual CacheLookupResult lookup(
            const CacheQuery& query
        ) = 0;

        virtual void store(
            const HashedImageResult& result,
            const QMap<QString, int>& methodVersions
        ) = 0;
    };
}