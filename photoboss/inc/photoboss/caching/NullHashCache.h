#pragma once
#include "caching/IHashCache.h"
#include "types/DataTypes.h"
#include <vector>
namespace photoboss
{
    class NullHashCache : public IHashCache {
    public:
        CacheLookupResult lookup(const CacheQuery& query) override {
            return { CacheLookupResult() };
        }

        void store(const HashedImageResult&, const QMap<QString, int>&) override {
            // no-op
        }

        void storeBatch(const std::vector<std::pair<HashedImageResult, QMap<QString, int>>>&) override {
            // no-op
        }
    };
}