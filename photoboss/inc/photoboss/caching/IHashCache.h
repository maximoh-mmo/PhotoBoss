#pragma once
#include <optional>
#include <vector>
#include "types/CacheTypes.h"

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

        virtual void storeBatch(
            const std::vector<std::pair<HashedImageResult, QMap<QString, int>>>& batch
        ) = 0;
    };
}