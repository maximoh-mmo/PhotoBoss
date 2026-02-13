#pragma once
#include "caching/IHashCache.h"
#include <QSqlDatabase>
#include "util/Token.h"

namespace photoboss {
    class SqliteHashCache : public IHashCache
    {
	public:
		static QString defaultDatabasePath();
        
        explicit SqliteHashCache(quint64 scanId);
        ~SqliteHashCache();

        CacheLookupResult lookup(const CacheQuery& query);
        
        void store(
            const HashedImageResult& result,
            const QMap<QString, int>& methodVersions);

        void prune(const QString& root);

        quint64 nextScanId(const photoboss::Token&);
    private:
        QSqlDatabase m_db_;
		QString m_dbPath_;
        bool m_valid_ = false;
        bool m_initialized_ = false;
        quint64 m_scan_id_ = -1;
        static QString connectionName();
        bool initSchemaOrMigrate();
        bool initSchema();
        int readSchemaVersion();
        bool migrateStep(int version);
        bool migrate_0_to_1();
        bool ensureMethod(const QString& key, int version, int& outMethodId);
        void updateScanIdForFile(int fileId);
        void ensureOpen();
    };
}
