#include "caching/SqliteHashCache.h"
#include <QSqlError>
#include <QSqlQuery>
#include <qstandardpaths.h>
#include <qdir.h>

namespace photoboss
{
    constexpr int SCHEMA_VERSION = 1;

    bool execOrLog(QSqlQuery& query, const QString& context)
    {
        if (!query.exec()) {
            qWarning() << "[SqliteHashCache]" << context
                << "failed:" << query.lastError().text();
            return false;
        }
        return true;
    }

    QString SqliteHashCache::defaultDatabasePath()
    {
        const QString baseDir =
            QStandardPaths::writableLocation(
                QStandardPaths::AppLocalDataLocation
            );


        QDir dir(baseDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        return dir.filePath("hash_cache.sqlite");
    }

    SqliteHashCache::SqliteHashCache()
    {
        m_dbPath_ = defaultDatabasePath();
        // NOTE: single SqliteHashCache instance per process
        m_db_ = QSqlDatabase::addDatabase("QSQLITE", "hash_cache");
        m_db_.setDatabaseName(m_dbPath_);

        if (!m_db_.open())
        {
            // log and disable cache
            qDebug() << "Failed to open hash cache database:" << m_db_.lastError().text();
            return;
        }
        QSqlQuery pragma(m_db_);
        pragma.exec("PRAGMA journal_mode=WAL;");
        pragma.exec("PRAGMA synchronous=NORMAL;");
        pragma.exec("PRAGMA foreign_keys=ON;");

        m_valid_ = initSchema();
    }

    SqliteHashCache::~SqliteHashCache()
    {
        qDebug() << "~SqliteHashCache() called";

        if (m_db_.isOpen()) {
            qDebug() << "Closing database connection";
            m_db_.close();
        }

        qDebug() << "~SqliteHashCache() finished";
    }

    CacheLookupResult SqliteHashCache::lookup(const CacheQuery& query)
    {
        if (!m_valid_)
            return { Lookup::Error, query.fileIdentity };

        QSqlQuery q(m_db_);

        // 1. find file
        q.prepare(R"(
        SELECT id FROM files
        WHERE path = :path
          AND size = :size
          AND modified_time = :mtime;
    )");
        q.bindValue(":path", query.fileIdentity.path());
        q.bindValue(":size", query.fileIdentity.size());
        q.bindValue(":mtime", query.fileIdentity.modifiedTime());

        if (!q.exec() || !q.next())
            return { Lookup::Error, query.fileIdentity };

        const int fileId = q.value(0).toInt();

        // 2. resolve hash methods
        QMap<QString, int> methodIds;

        for (const QString& key : query.requiredMethods) {
            QSqlQuery mq(m_db_);
            mq.prepare(R"(
            SELECT id FROM hash_methods WHERE key = :key;
        )");
            mq.bindValue(":key", key);

            if (!mq.exec() || !mq.next())
                return { Lookup::Miss, query.fileIdentity };

            methodIds.insert(key, mq.value(0).toInt());
        }

        // 3. fetch hashes
        auto result = HashedImageResult{query.fileIdentity,HashSource::Cache,{},{} };
		
        for (auto it = methodIds.begin(); it != methodIds.end(); ++it) {
            QSqlQuery hq(m_db_);
            hq.prepare(R"(
            SELECT hash_value FROM hashes
            WHERE file_id = :file
              AND method_id = :method;
        )");
            hq.bindValue(":file", fileId);
            hq.bindValue(":method", it.value());

            if (!hq.exec() || !hq.next())
                return { Lookup::Error, query.fileIdentity };

            result.hashes.emplace(it.key(), hq.value(0).toString());
        }
        return { Lookup::Hit, result };
    }

    void SqliteHashCache::store(const HashedImageResult& result, const QMap<QString, int>& methodVersions)
    {
        if (!m_valid_)
            return;

        QSqlQuery q(m_db_);

        if (!q.exec("BEGIN TRANSACTION;")) {
            qWarning() << "[SqliteHashCache] BEGIN failed";
            return;
        }

        // upsert file
        q.prepare(R"(INSERT INTO files (path, size, modified_time, format)
            VALUES (:path, :size, :mtime, :format)
            ON CONFLICT(path) DO UPDATE SET
            size = excluded.size,
            modified_time = excluded.modified_time,
            format = excluded.format;)"
        );
        q.bindValue(":path", result.fileIdentity.path());
        q.bindValue(":size", result.fileIdentity.size());
        q.bindValue(":mtime", result.fileIdentity.modifiedTime());
        q.bindValue(":format", QVariant());

        if (!execOrLog(q, "upsert file")) {
            q.exec("ROLLBACK;");
            return;
        }

        // retrieve file id
        q.prepare("SELECT id FROM files WHERE path = :path;");
        q.bindValue(":path", result.fileIdentity.path());
        if (!execOrLog(q, "fetch file id") || !q.next()) {
            q.exec("ROLLBACK;");
            return;
        }
        const int fileId = q.value(0).toInt();

        // hashes
        const qint64 now = QDateTime::currentSecsSinceEpoch();

        for (auto it = result.hashes.begin(); it != result.hashes.end(); ++it) {
            const QString& key = it->first;
			const QString& value = it->second;
            const int version = methodVersions.value(key, 0);

            int methodId;
            if (!ensureMethod(key, version, methodId)) {
                q.exec("ROLLBACK;");
                return;
            }

            QSqlQuery hq(m_db_);
            hq.prepare(R"(
            INSERT INTO hashes (file_id, method_id, hash_value, computed_at)
            VALUES (:file, :method, :value, :time)
            ON CONFLICT(file_id, method_id) DO UPDATE SET
                hash_value = excluded.hash_value,
                computed_at = excluded.computed_at;
        )");
            hq.bindValue(":file", fileId);
            hq.bindValue(":method", methodId);
            hq.bindValue(":value", value);
            hq.bindValue(":time", now);

            if (!execOrLog(hq, "upsert hash")) {
                q.exec("ROLLBACK;");
                return;
            }
        }

        q.exec("COMMIT;");
    }

    bool SqliteHashCache::initSchema()
    {
        QSqlQuery q(m_db_);

        if (!q.exec("BEGIN TRANSACTION;"))
            return false;

        // meta
        q.prepare(R"(CREATE TABLE IF NOT EXISTS meta (
            key TEXT PRIMARY KEY, 
            value TEXT NOT NULL);)"
        );

        if (!execOrLog(q, "create meta")) return false;

        // files
        q.prepare(R"(CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            size INTEGER NOT NULL,
            modified_time INTEGER NOT NULL,
            format TEXT);)"
        );

        if (!execOrLog(q, "create files")) return false;

        q.prepare(R"(CREATE INDEX IF NOT EXISTS idx_files_identity
            ON files(path, size, modified_time);
    )");
        if (!execOrLog(q, "create files index")) return false;

        // hash methods
        q.prepare(R"(
        CREATE TABLE IF NOT EXISTS hash_methods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT NOT NULL UNIQUE,
            version INTEGER NOT NULL
        );
    )");
        if (!execOrLog(q, "create hash_methods")) return false;

        // hashes
        q.prepare(R"(
        CREATE TABLE IF NOT EXISTS hashes (
            file_id INTEGER NOT NULL,
            method_id INTEGER NOT NULL,
            hash_value TEXT NOT NULL,
            computed_at INTEGER NOT NULL,
            PRIMARY KEY (file_id, method_id),
            FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,
            FOREIGN KEY (method_id) REFERENCES hash_methods(id) ON DELETE CASCADE
        );
    )");
        if (!execOrLog(q, "create hashes")) return false;

        // schema version
        q.prepare(R"(
        INSERT OR IGNORE INTO meta (key, value)
        VALUES ('schema_version', :version);
    )");
        q.bindValue(":version", SCHEMA_VERSION);
        if (!execOrLog(q, "insert schema_version")) return false;

        // m_valid_ate schema version
        q.prepare("SELECT value FROM meta WHERE key='schema_version';");
        if (!execOrLog(q, "read schema_version")) return false;

        if (q.next()) {
            int version = q.value(0).toInt();
            if (version != SCHEMA_VERSION) {
                qWarning() << "[SqliteHashCache] Schema version mismatch:"
                    << version << "!=" << SCHEMA_VERSION;
                q.exec("ROLLBACK;");
                return false;
            }
        }

        q.exec("COMMIT;");
        return true;
    }

    bool SqliteHashCache::ensureMethod(const QString& key, int version, int& outMethodId)
    {
        QSqlQuery q(m_db_);

        q.prepare(R"(
        INSERT INTO hash_methods (key, version)
        VALUES (:key, :version)
        ON CONFLICT(key) DO UPDATE SET
            version = excluded.version;
    )");
        q.bindValue(":key", key);
        q.bindValue(":version", version);

        if (!execOrLog(q, "ensure hash method"))
            return false;

        q.prepare("SELECT id FROM hash_methods WHERE key = :key;");
        q.bindValue(":key", key);

        if (!execOrLog(q, "fetch method id") || !q.next())
            return false;

        outMethodId = q.value(0).toInt();
        return true;
    }
}