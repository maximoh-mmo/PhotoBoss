#pragma once
#include "types/DataTypes.h"
namespace photoboss
{
    enum class Lookup {
        Hit,
        Miss,
        Error
	};

    struct CacheLookupResult {
        Lookup hit;
        HashedImageResult hashedImage;

        CacheLookupResult()
            : hit(Lookup::Error)
            , hashedImage(FileIdentity{}, HashSource::Error)
        {
		}
        CacheLookupResult(Lookup lookupType, HashedImageResult result)
            : hit(lookupType)
            , hashedImage(std::move(result))
        {
        }
    };

    struct CacheQuery {
        FileIdentity fileIdentity;
        QList<QString> hashMethods; // e.g. ["md5", "phash"]

        explicit CacheQuery(FileIdentity id)
            : fileIdentity(std::move(id))
        {
        }
    };
}