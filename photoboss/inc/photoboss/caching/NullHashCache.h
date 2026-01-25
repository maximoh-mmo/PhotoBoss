#pragma once
#include "IHashCache.h"
#include "util/CacheTypes.h"
namespace photoboss
{
    class NullHashCache : public IHashCache {
    public:
        CacheLookupResult lookup(const CacheQuery& query) override {
            return { Route::CacheMiss , { query.fingerprint, HashedImageResult::HashSource::Fresh, {}, {} } };
        }

        void store(const HashedImageResult&, const QMap<QString, int>&) override {
            // no-op
        }
    };
}