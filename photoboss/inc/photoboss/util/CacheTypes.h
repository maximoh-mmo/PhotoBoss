#pragma once
#include "util/DataTypes.h"
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
        QList<QString> requiredMethods; // e.g. ["md5", "phash"]

        CacheQuery(FileIdentity id, std::map<QString, QString> hashMap = {})
            : fileIdentity(std::move(id)),
			requiredMethods()
        {
        }
    };
}