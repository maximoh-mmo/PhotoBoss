#pragma once
#include "IHashCache.h"
#include "DataTypes.h"
namespace photoboss
{
    class NullHashCache : public IHashCache {
    public:
        CacheLookupResult lookup(const CacheQuery& query) override {
            return { false, {} };
        }

        void store(const HashedImageResult&, const QMap<QString, int>&) override {
            // no-op
        }
    };
}