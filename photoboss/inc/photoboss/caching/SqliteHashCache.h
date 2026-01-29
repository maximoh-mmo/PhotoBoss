#pragma once
#include "caching/IHashCache.h"
#include <QSqlDatabase>

namespace photoboss {
    class SqliteHashCache : public IHashCache
    {
	public:
		static QString defaultDatabasePath();

        explicit SqliteHashCache();
        ~SqliteHashCache();

        CacheLookupResult lookup(const CacheQuery& query) override;
        
        void store(const HashedImageResult& result, 
            const QMap<QString, int>& methodVersions) override;

    private:
        QSqlDatabase m_db_;
		QString m_dbPath_;
		bool m_valid_ = false;
        bool initSchema();
        bool ensureMethod(const QString& key, int version, int& outMethodId);
    };
}
