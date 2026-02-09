#include "caching/SqliteHashCache.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QDebug>

namespace photoboss
{
    constexpr int SCHEMA_VERSION = 2;

    static bool execOrLog(QSqlQuery& query, const QString& context)
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

    QString SqliteHashCache::connectionName()
    {
        return QStringLiteral("hash_cache_%1")
            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    }

    SqliteHashCache::SqliteHashCache()
        : m_dbPath_(defaultDatabasePath())
    {
    }

    SqliteHashCache::~SqliteHashCache()
    {
        if (!m_initialized_)
            return;

        const QString name = m_db_.connectionName();

        if (m_db_.isOpen())
            m_db_.close();

        m_db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
    }

    // ------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------

    CacheLookupResult SqliteHashCache::lookup(const CacheQuery& cacheQuery)
    {
        ensureOpen();
        if (!m_valid_)
            return { Lookup::Error, cacheQuery.fileIdentity };

        QSqlQuery query(m_db_);

        // 1) find file
        query.prepare(R"(
            SELECT id
            FROM files
            WHERE path = :path
              AND size = :size
              AND modified_time = :mtime
        )");
        query.bindValue(":path", cacheQuery.fileIdentity.path());
        query.bindValue(":size", cacheQuery.fileIdentity.size());
        query.bindValue(":mtime", cacheQuery.fileIdentity.modifiedTime());

        if (!query.exec() || !query.next())
            return { Lookup::Miss, cacheQuery.fileIdentity };

        const int fileId = query.value(0).toInt();

        QMap<QString, int> methodIds;
        for (const QString& key : cacheQuery.hashMethods) {
            QSqlQuery methodQuery(m_db_);
            methodQuery.prepare("SELECT id FROM hash_methods WHERE key = :key;");
            methodQuery.bindValue(":key", key);

            if (!methodQuery.exec() || !methodQuery.next())
                return { Lookup::Miss, cacheQuery.fileIdentity };

            methodIds.insert(key, methodQuery.value(0).toInt());
        }

        QSqlQuery exifQuery(m_db_);

        exifQuery.prepare(R"(
            SELECT
            orientation,
            datetime_original,
            camera_make,
            camera_model
            FROM file_exif
            WHERE file_id = :file;
        )");
        exifQuery.bindValue(":file", fileId);

        ExifData cachedExif;

        if (exifQuery.exec() && exifQuery.next()) {
            if (!exifQuery.isNull(0)) cachedExif.orientation = exifQuery.value(0).toInt();
            if (!exifQuery.isNull(1)) cachedExif.dateTimeOriginal = exifQuery.value(1).toULongLong();
            if (!exifQuery.isNull(2)) cachedExif.cameraMake = exifQuery.value(2).toString();
            if (!exifQuery.isNull(3)) cachedExif.cameraModel = exifQuery.value(3).toString();
        }

        if (!(cachedExif == cacheQuery.fileIdentity.exif())) {
            return { Lookup::Miss, cacheQuery.fileIdentity };
        }

        // 3) fetch hashes
        HashedImageResult result{
            cacheQuery.fileIdentity,
            HashSource::Cache,
            QDateTime{},
            QSize{},
            {}
        };

        for (auto method = methodIds.begin(); method != methodIds.end(); ++method) {
            QSqlQuery hashQuery(m_db_);
            hashQuery.prepare(R"(
                SELECT hash_value
                FROM hashes
                WHERE file_id = :file
                  AND method_id = :method;
            )");
            hashQuery.bindValue(":file", fileId);
            hashQuery.bindValue(":method", method.value());

            if (!hashQuery.exec() || !hashQuery.next())
                return { Lookup::Error, cacheQuery.fileIdentity };

            result.hashes.emplace(method.key(), hashQuery.value(0).toString());
        }

        QSqlQuery resolution(m_db_);
        resolution.prepare(R"(
                SELECT height, width
                FROM files
                WHERE id = :id;
            )");
        resolution.bindValue(":id", fileId);

        if (resolution.exec() && resolution.next()) { 
            if (!resolution.isNull(0) && !resolution.isNull(1)){
                result.resolution = { resolution.value(0).toInt(), resolution.value(1).toInt() };
            };
        }

        return { Lookup::Hit, result };
    }

    void SqliteHashCache::store(
        const HashedImageResult& result,
        const QMap<QString, int>& methodVersions)
    {
        ensureOpen();
        if (!m_valid_)
            return;

        QSqlQuery q(m_db_);

        if (!q.exec("BEGIN TRANSACTION;")) {
            qWarning() << "[SqliteHashCache] BEGIN failed";
            return;
        }

        // upsert file
        q.prepare(R"(
            INSERT INTO files (path, size, modified_time, format, width, height)
            VALUES (:path, :size, :mtime, :format, :width, :height)
            ON CONFLICT(path) DO UPDATE SET
                size = excluded.size,
                modified_time = excluded.modified_time,
                format = excluded.format,
                width = excluded.width,
                height = excluded.height;
        )");
        q.bindValue(":path", result.fileIdentity.path());
        q.bindValue(":size", result.fileIdentity.size());
        q.bindValue(":mtime", result.fileIdentity.modifiedTime());
        q.bindValue(":format", result.fileIdentity.extension());
        q.bindValue(":width", result.resolution.width());
        q.bindValue(":height", result.resolution.height());

        if (!execOrLog(q, "upsert file")) {
            q.exec("ROLLBACK;");
            return;
        }

        // fetch file id
        q.prepare("SELECT id FROM files WHERE path = :path;");
        q.bindValue(":path", result.fileIdentity.path());

        if (!execOrLog(q, "fetch file id") || !q.next()) {
            q.exec("ROLLBACK;");
            return;
        }

        const int fileId = q.value(0).toInt();
        const qint64 now = QDateTime::currentSecsSinceEpoch();

        // upsert hashes
        for (auto it = result.hashes.begin(); it != result.hashes.end(); ++it) {
            int methodId;
            if (!ensureMethod(it->first, methodVersions.value(it->first, 0), methodId)) {
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
            hq.bindValue(":value", it->second);
            hq.bindValue(":time", now);

            if (!execOrLog(hq, "upsert hash")) {
                q.exec("ROLLBACK;");
                return;
            }
        }

        //upsert exif data
        const ExifData& exif = result.fileIdentity.exif();

        QSqlQuery ex(m_db_);
        ex.prepare(R"(
            INSERT INTO file_exif (
                file_id,
                orientation,
                datetime_original,
                camera_make,
                camera_model
                )
            VALUES (
                :file,
                :orientation,
                :datetime,
                :make,
                :model
            )
            ON CONFLICT(file_id) DO UPDATE SET
                orientation = excluded.orientation,
                datetime_original = excluded.datetime_original,
                camera_make = excluded.camera_make,
                camera_model = excluded.camera_model;
        )");

        ex.bindValue(":file", fileId);

        ex.bindValue(":orientation", exif.orientation ? 
            QVariant(*exif.orientation) : QVariant());

        ex.bindValue(":datetime", exif.dateTimeOriginal ? 
            QVariant::fromValue<qulonglong>(*exif.dateTimeOriginal) : QVariant());

        ex.bindValue(":make", exif.cameraMake ? 
            QVariant(*exif.cameraMake) : QVariant());

        ex.bindValue(":model", exif.cameraModel ? 
            QVariant(*exif.cameraModel) : QVariant());

        if (!execOrLog(ex, "upsert exif")) {
            q.exec("ROLLBACK;");
            return;
        }

        q.exec("COMMIT;");
    }

    // ------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------

    void SqliteHashCache::ensureOpen()
    {
        if (m_initialized_)
            return;

        const QString name = connectionName();

        if (QSqlDatabase::contains(name)) {
            m_db_ = QSqlDatabase::database(name);
        }
        else {
            m_db_ = QSqlDatabase::addDatabase("QSQLITE", name);
            m_db_.setDatabaseName(m_dbPath_);

            if (!m_db_.open()) {
                qCritical() << "[SqliteHashCache] Failed to open DB:"
                    << m_db_.lastError().text();
                return;
            }

            QSqlQuery pragma(m_db_);
            pragma.exec("PRAGMA journal_mode=WAL;");
            pragma.exec("PRAGMA synchronous=NORMAL;");
            pragma.exec("PRAGMA foreign_keys=ON;");
        }

        m_valid_ = initSchema();
        m_initialized_ = true;
    }

    bool SqliteHashCache::initSchema()
    {
        QSqlQuery q(m_db_);

        if (!q.exec("BEGIN TRANSACTION;"))
            return false;

        // meta
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS meta (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            );
        )");
        if (!execOrLog(q, "create meta")) return false;

        // files
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS files (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                path TEXT NOT NULL UNIQUE,
                size INTEGER NOT NULL,
                modified_time INTEGER NOT NULL,
                format TEXT,
                width INTEGER,
                height INTEGER
            );
        )");
        if (!execOrLog(q, "create files")) return false;

        q.prepare(R"(
            CREATE INDEX IF NOT EXISTS idx_files_identity
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

        // exif
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS file_exif (
                file_id INTEGER PRIMARY KEY,
                orientation INTEGER,
                datetime_original INTEGER,
                camera_make TEXT,
                camera_model TEXT,
                FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
                );
            )");

        if (!execOrLog(q, "create file_exif")) return false;
        // schema version
        q.prepare(R"(
            INSERT OR IGNORE INTO meta (key, value)
            VALUES ('schema_version', :version);
        )");
        q.bindValue(":version", SCHEMA_VERSION);
        if (!execOrLog(q, "insert schema_version")) return false;

        q.prepare("SELECT value FROM meta WHERE key='schema_version';");
        if (!execOrLog(q, "read schema_version")) return false;

        if (q.next() && q.value(0).toInt() != SCHEMA_VERSION) {
            qWarning() << "[SqliteHashCache] Schema version mismatch";
            q.exec("ROLLBACK;");
            return false;
        }

        q.exec("COMMIT;");
        return true;
    }

    bool SqliteHashCache::ensureMethod(
        const QString& key,
        int version,
        int& outMethodId)
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