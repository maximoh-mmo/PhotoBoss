#pragma once
#include "IHashCache.h"
#include <QSqlDatabase>

namespace photoboss {
    class SqliteHashCache : public IHashCache
    {
        explicit SqliteHashCache(const QString& dbPath);
        ~SqliteHashCache();

        CacheLookupResult lookup(const CacheQuery& query) override;
        
        void store(const HashedImageResult& result, 
            const QMap<QString, int>& methodVersions) override;

    private:
        QSqlDatabase m_db_;
		bool m_valid_ = false;
        bool initSchema();
        bool ensureMethod(const QString& key, int version, int& outMethodId);
    };
}
