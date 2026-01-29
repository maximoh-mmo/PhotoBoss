#pragma once
#include "FileTypes.h"
#include <QList>
#include <map>

namespace photoboss {

    enum class Route {
        CacheHit,
        CacheMiss
    };

    struct CacheLookupResult {
        Route hit;
        HashedImageResult hashedImage; // valid only if hit == true
    };

    struct CacheQuery {
        Fingerprint fingerprint;
        QList<QString> requiredMethods; // e.g. ["md5", "phash"]
    };

}