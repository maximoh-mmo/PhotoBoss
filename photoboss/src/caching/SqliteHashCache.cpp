#include "caching/SqliteHashCache.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QDebug>
#include <QBuffer>
#include <QImageWriter>
#include "util/AppSettings.h"

namespace photoboss
{
    // -----------------------------
    // Constructors / Destructors
    // -----------------------------

    SqliteHashCache::SqliteHashCache(quint64 scanId)
        : m_dbPath_(defaultDatabasePath())
        , m_scanId_(scanId)
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

    // -----------------------------
    // Internal helpers
    // -----------------------------

    static bool execOrLog(QSqlQuery& query, const QString& context)
    {
        if (!query.exec()) {
            qDebug() << "[SqliteHashCache]" << context
                << "failed:" << query.lastError().text();
            return false;
        }
        return true;
    }

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
                qCritical() << "[SqliteHashCache] Failed to open DB:" << m_db_.lastError().text();
                return;
            }

            QSqlQuery pragma(m_db_);
            pragma.exec("PRAGMA journal_mode=WAL;");
            pragma.exec("PRAGMA synchronous=NORMAL;");
            pragma.exec("PRAGMA foreign_keys=ON;");
        }

        m_valid_ = initSchemaOrMigrate();
        m_initialized_ = true;
    }

    QString SqliteHashCache::defaultDatabasePath()
    {
        const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        QDir dir(baseDir);
        if (!dir.exists())
            dir.mkpath(".");
        return dir.filePath("hash_cache.sqlite");
    }

    QString SqliteHashCache::connectionName()
    {
        return QStringLiteral("hash_cache_%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    }

    // -----------------------------
    // Schema initialization & migration
    // -----------------------------

    bool SqliteHashCache::initSchemaOrMigrate()
    {
        if (!initSchema())
            return false;

        int version = readSchemaVersion();
        while (version < settings::SCHEMA_VERSION) {
            if (!migrateStep(version))
                return false;
            version = readSchemaVersion();
        }

        return true;
    }

    bool SqliteHashCache::initSchema()
    {
        QSqlQuery q(m_db_);
        if (!q.exec("BEGIN IMMEDIATE TRANSACTION;"))
            return false;

        // meta table
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS meta (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            );
        )");
        if (!execOrLog(q, "create meta")) { q.exec("ROLLBACK;"); return false; }

        // files table
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS files (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                path TEXT NOT NULL,
                size INTEGER NOT NULL,
                modified_time INTEGER NOT NULL,
                format TEXT,
                width INTEGER,
                height INTEGER,
                last_seen_scan_id INTEGER,
                UNIQUE(name, path)
            );
        )");
        if (!execOrLog(q, "create files")) { q.exec("ROLLBACK;"); return false; }

        q.prepare("CREATE INDEX IF NOT EXISTS idx_files_identity ON files(path, size, modified_time);");
        if (!execOrLog(q, "create files index")) { q.exec("ROLLBACK;"); return false; }
        q.prepare("CREATE INDEX IF NOT EXISTS idx_files_scan ON files(last_seen_scan_id);");
        if (!execOrLog(q, "create scan index")) { q.exec("ROLLBACK;"); return false; }

        // hash_methods table
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS hash_methods (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key TEXT NOT NULL UNIQUE,
                version INTEGER NOT NULL
            );
        )");
        if (!execOrLog(q, "create hash_methods")) { q.exec("ROLLBACK;"); return false; }

        // hashes table
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
        if (!execOrLog(q, "create hashes")) { q.exec("ROLLBACK;"); return false; }

        // file_exif table
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
        if (!execOrLog(q, "create file_exif")) { q.exec("ROLLBACK;"); return false; }

        // thumbnails table
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS thumbnails (
                file_id INTEGER NOT NULL,
                width INTEGER NOT NULL,
                rotation INTEGER NOT NULL,
                data BLOB NOT NULL,
                PRIMARY KEY (file_id, width, rotation),
                FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
            );
        )");
        if (!execOrLog(q, "create thumbnails")) { q.exec("ROLLBACK;"); return false; }

        // initialize schema_version & last_scan_id
        q.prepare(R"(INSERT OR IGNORE INTO meta(key, value) VALUES('schema_version', '0');)");
        if (!execOrLog(q, "init schema_version")) { q.exec("ROLLBACK;"); return false; }
        q.prepare(R"(INSERT OR IGNORE INTO meta(key, value) VALUES('last_scan_id', '0');)");
        if (!execOrLog(q, "init last_scan_id")) { q.exec("ROLLBACK;"); return false; }

        q.exec("COMMIT;");
        return true;
    }

    int SqliteHashCache::readSchemaVersion()
    {
        QSqlQuery q(m_db_);
        q.prepare("SELECT value FROM meta WHERE key='schema_version';");
        if (!execOrLog(q, "read schema_version") || !q.next())
            return 0;
        return q.value(0).toInt();
    }

    bool SqliteHashCache::migrateStep(int version)
    {
        switch (version)
        {
        case 0: return migrate_0_to_1();
        default:
            qWarning() << "[SqliteHashCache] Unknown migration step:" << version;
            return false;
        }
    }

    bool SqliteHashCache::migrate_0_to_1()
    {
        QSqlQuery q(m_db_);
        if (!q.exec("BEGIN IMMEDIATE TRANSACTION;")) return false;

        // Hashes from old full-resolution decode are incompatible with
        // new IDCT-scaled decode — clear them all so they recompute.
        q.exec("DELETE FROM hashes;");
        if (!execOrLog(q, "delete hashes")) { q.exec("ROLLBACK;"); return false; }
        q.exec("DELETE FROM hash_methods;");
        if (!execOrLog(q, "delete hash methods")) { q.exec("ROLLBACK;"); return false; }

        // Add thumbnail cache table
        q.prepare(R"(
            CREATE TABLE IF NOT EXISTS thumbnails (
                file_id INTEGER NOT NULL,
                width INTEGER NOT NULL,
                rotation INTEGER NOT NULL,
                data BLOB NOT NULL,
                PRIMARY KEY (file_id, width, rotation),
                FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE
            );
        )");
        if (!execOrLog(q, "create thumbnails")) { q.exec("ROLLBACK;"); return false; }

        q.prepare("UPDATE meta SET value='1' WHERE key='schema_version';");
        if (!execOrLog(q, "bump schema_version")) { q.exec("ROLLBACK;"); return false; }

        q.exec("COMMIT;");
        return true;
    }

    // -----------------------------
    // Ensure hash method exists
    // -----------------------------

    bool SqliteHashCache::ensureMethod(const QString& key, int version, int& outMethodId)
    {
        QSqlQuery q(m_db_);
        q.prepare(R"(
            INSERT INTO hash_methods(key, version)
            VALUES(:key, :version)
            ON CONFLICT(key) DO UPDATE SET version=excluded.version;
        )");
        q.bindValue(":key", key);
        q.bindValue(":version", version);
        if (!execOrLog(q, "ensure hash method")) return false;

        q.prepare("SELECT id FROM hash_methods WHERE key=:key;");
        q.bindValue(":key", key);
        if (!execOrLog(q, "fetch method id") || !q.next()) return false;

        outMethodId = q.value(0).toInt();
        return true;
    }

    // -----------------------------
    // Scan ID
    // -----------------------------

    quint64 SqliteHashCache::nextScanId(const photoboss::Token&)
    {
        ensureOpen();
        if (!m_valid_) return 0;

        QSqlQuery q(m_db_);
        if (!q.exec("BEGIN IMMEDIATE TRANSACTION;")) return 0;

        q.prepare("SELECT value FROM meta WHERE key='last_scan_id';");
        if (!execOrLog(q, "read last_scan_id") || !q.next()) { q.exec("ROLLBACK;"); return 0; }

        quint64 next = q.value(0).toULongLong() + 1;

        QSqlQuery update(m_db_);
        update.prepare("UPDATE meta SET value=:v WHERE key='last_scan_id';");
        update.bindValue(":v", next);
        if (!execOrLog(update, "update last_scan_id")) { q.exec("ROLLBACK;"); return 0; }

        q.exec("COMMIT;");
        return next;
    }

    void SqliteHashCache::updateScanIdForFile(int fileId)
    {
        QSqlQuery q(m_db_);
        q.prepare(R"(
        UPDATE files
        SET last_seen_scan_id = :scanId
        WHERE id = :fileId
    )");
        q.bindValue(":scanId", m_scanId_);
        q.bindValue(":fileId", fileId);
        q.exec();
    }

    // -----------------------------
    // Lookup
    // -----------------------------

    CacheLookupResult SqliteHashCache::lookup(const CacheQuery& cacheQuery)
    {
        ensureOpen();
        if (!m_valid_) return { Lookup::Error, cacheQuery.fileIdentity };

        QSqlQuery q(m_db_);
        // single query: file, exif, all requested hashes
        QString placeholders;
        for (int i = 0; i < cacheQuery.hashMethods.size(); ++i) {
            if (i) placeholders += ",";
            placeholders += "?";
        }

        QString sql = QString(R"(
            SELECT f.id, f.width, f.height, e.orientation, e.datetime_original,
                   e.camera_make, e.camera_model, hm.key, h.hash_value
            FROM files f
            LEFT JOIN file_exif e ON e.file_id=f.id
            LEFT JOIN hashes h ON h.file_id=f.id
            LEFT JOIN hash_methods hm ON hm.id=h.method_id
            WHERE f.name=? AND f.path=? AND f.size=? AND f.modified_time=? AND hm.key IN (%1);
        )").arg(placeholders);

        q.prepare(sql);
        q.addBindValue(cacheQuery.fileIdentity.name());
        q.addBindValue(cacheQuery.fileIdentity.path());
        q.addBindValue(cacheQuery.fileIdentity.size());
        q.addBindValue(cacheQuery.fileIdentity.modifiedTime());
        for (const auto& key : cacheQuery.hashMethods)
            q.addBindValue(key);

        if (!q.exec()) return { Lookup::Error, cacheQuery.fileIdentity };
        if (!q.next()) return { Lookup::Miss, cacheQuery.fileIdentity };

        const int fileId = q.value(0).toInt();
        HashedImageResult result{ cacheQuery.fileIdentity, HashSource::Cache, QDateTime{}, QSize{}, {} };
        if (!q.isNull(1) && !q.isNull(2)) result.resolution = { q.value(1).toInt(), q.value(2).toInt() };

        ExifData cachedExif;
        if (!q.isNull(3)) cachedExif.orientation = q.value(3).toInt();
        if (!q.isNull(4)) cachedExif.dateTimeOriginal = q.value(4).toULongLong();
        if (!q.isNull(5)) cachedExif.cameraMake = q.value(5).toString();
        if (!q.isNull(6)) cachedExif.cameraModel = q.value(6).toString();
        QSet<QString> requestedMethods{ cacheQuery.hashMethods.begin(), cacheQuery.hashMethods.end() };
        QSet<QString> foundMethods;

        do {
            if (!q.isNull(7) && !q.isNull(8)) {
                const QString methodKey = q.value(7).toString();
                const QString hashValue = q.value(8).toString();
                result.hashes.emplace(methodKey, hashValue);
                foundMethods.insert(methodKey);
            }
        } while (q.next());

        if (foundMethods == requestedMethods) {
            // cache hit - mark as seen for this scan
            updateScanIdForFile(fileId);
            return { Lookup::Hit, result };
        } 
        
        return CacheLookupResult{ Lookup::Miss, cacheQuery.fileIdentity };
    }

    // -----------------------------
    // Optimized Store
    // -----------------------------

    bool SqliteHashCache::storeItem(QSqlQuery& q, const HashedImageResult& result,
        const QMap<QString, int>& methodVersions)
    {
        // upsert file
        q.prepare(R"(
            INSERT INTO files(name, path, size, modified_time, format, width, height, last_seen_scan_id)
            VALUES(:name, :path, :size, :mtime, :format, :width, :height, :scan)
            ON CONFLICT(name, path) DO UPDATE SET
                size=excluded.size, modified_time=excluded.modified_time,
                format=excluded.format, width=excluded.width, height=excluded.height,
                last_seen_scan_id=excluded.last_seen_scan_id;
        )");
        q.bindValue(":name", result.fileIdentity.name());
        q.bindValue(":path", result.fileIdentity.path());
        q.bindValue(":size", result.fileIdentity.size());
        q.bindValue(":mtime", result.fileIdentity.modifiedTime());
        q.bindValue(":format", result.fileIdentity.extension());
        q.bindValue(":width", result.resolution.width());
        q.bindValue(":height", result.resolution.height());
        q.bindValue(":scan", m_scanId_);

        if (!execOrLog(q, "upsert file")) return false;

        // fetch file id
        q.prepare("SELECT id FROM files WHERE name=:name AND path=:path;");
        q.bindValue(":name", result.fileIdentity.name());
        q.bindValue(":path", result.fileIdentity.path());
        if (!execOrLog(q, "fetch file id") || !q.next()) return false;
        const int fileId = q.value(0).toInt();
        const qint64 now = QDateTime::currentSecsSinceEpoch();

        // upsert hashes
        for (auto it = result.hashes.begin(); it != result.hashes.end(); ++it) {
            int methodId;
            if (!ensureMethod(it->first, methodVersions.value(it->first, 0), methodId)) return false;

            QSqlQuery hq(m_db_);
            hq.prepare(R"(
                INSERT INTO hashes(file_id, method_id, hash_value, computed_at)
                VALUES(:file, :method, :value, :time)
                ON CONFLICT(file_id, method_id) DO UPDATE SET
                    hash_value=excluded.hash_value, computed_at=excluded.computed_at;
            )");
            hq.bindValue(":file", fileId);
            hq.bindValue(":method", methodId);
            hq.bindValue(":value", it->second);
            hq.bindValue(":time", now);

            if (!execOrLog(hq, "upsert hash")) return false;
        }

        // upsert exif
        const ExifData& exif = result.fileIdentity.exif();
        QSqlQuery ex(m_db_);
        ex.prepare(R"(
            INSERT INTO file_exif(file_id, orientation, datetime_original, camera_make, camera_model)
            VALUES(:file, :orientation, :datetime, :make, :model)
            ON CONFLICT(file_id) DO UPDATE SET
                orientation=excluded.orientation,
                datetime_original=excluded.datetime_original,
                camera_make=excluded.camera_make,
                camera_model=excluded.camera_model;
        )");
        ex.bindValue(":file", fileId);
        ex.bindValue(":orientation", exif.orientation ? QVariant(*exif.orientation) : QVariant());
        ex.bindValue(":datetime", exif.dateTimeOriginal ? QVariant::fromValue<qulonglong>(*exif.dateTimeOriginal) : QVariant());
        ex.bindValue(":make", exif.cameraMake ? QVariant(*exif.cameraMake) : QVariant());
        ex.bindValue(":model", exif.cameraModel ? QVariant(*exif.cameraModel) : QVariant());

        if (!execOrLog(ex, "upsert exif")) return false;

        return true;
    }

    void SqliteHashCache::store(const HashedImageResult& result, const QMap<QString, int>& methodVersions)
    {
        ensureOpen();
        if (!m_valid_) return;

        QSqlQuery q(m_db_);
        if (!q.exec("BEGIN TRANSACTION;")) { qWarning() << "[SqliteHashCache] BEGIN failed"; return; }

        if (!storeItem(q, result, methodVersions)) {
            q.exec("ROLLBACK;");
            return;
        }

        q.exec("COMMIT;");
    }

    void SqliteHashCache::storeBatch(
        const std::vector<std::pair<HashedImageResult, QMap<QString, int>>>& batch)
    {
        ensureOpen();
        if (!m_valid_ || batch.empty()) return;

        QSqlQuery q(m_db_);
        if (!q.exec("BEGIN IMMEDIATE TRANSACTION;")) return;

        for (const auto& [result, versions] : batch) {
            if (!storeItem(q, result, versions)) {
                q.exec("ROLLBACK;");
                return;
            }

            // Persist thumbnail from fresh images (decodedImage is already rotated by ImageLoader)
            if (result.decodedImage) {
                q.prepare("SELECT id FROM files WHERE name=:name AND path=:path;");
                q.bindValue(":name", result.fileIdentity.name());
                q.bindValue(":path", result.fileIdentity.path());
                if (!execOrLog(q, "find file for thumbnail") || !q.next()) continue;
                int fileId = q.value(0).toInt();

                // Serialize as JPEG BLOB
                QByteArray blob;
                QBuffer buf(&blob);
                buf.open(QIODevice::WriteOnly);
                QImageWriter writer(&buf, "JPEG");
                writer.setQuality(85);
                if (!writer.write(*result.decodedImage)) continue;
                buf.close();

                // Clean up old rotation-keyed entries and upsert with rotation=0
                q.prepare("DELETE FROM thumbnails WHERE file_id=:file AND rotation!=0;");
                q.bindValue(":file", fileId);
                execOrLog(q, "delete old thumbnail rotations");

                q.prepare(R"(
                    INSERT INTO thumbnails(file_id, width, rotation, data)
                    VALUES(:file, :width, 0, :data)
                    ON CONFLICT(file_id, width, rotation) DO UPDATE SET
                        data = excluded.data;
                )");
                q.bindValue(":file", fileId);
                q.bindValue(":width", settings::ThumbnailWidth);
                q.bindValue(":data", blob);
                execOrLog(q, "upsert thumbnail");
            }
        }

        q.exec("COMMIT;");
    }

    std::optional<QImage> SqliteHashCache::getThumbnail(
        const FileIdentity& fi, int width, int rotation)
    {
        ensureOpen();
        if (!m_valid_) return std::nullopt;

        QSqlQuery q(m_db_);
        q.prepare(R"(
            SELECT t.data FROM thumbnails t
            JOIN files f ON f.id = t.file_id
            WHERE f.name = :name AND f.path = :path
              AND f.size = :size AND f.modified_time = :mtime
              AND t.width = :width AND t.rotation = :rotation
        )");
        q.bindValue(":name", fi.name());
        q.bindValue(":path", fi.path());
        q.bindValue(":size", fi.size());
        q.bindValue(":mtime", fi.modifiedTime());
        q.bindValue(":width", width);
        q.bindValue(":rotation", rotation);

        if (!execOrLog(q, "get thumbnail") || !q.next()) return std::nullopt;

        QImage img;
        if (!img.loadFromData(q.value(0).toByteArray())) return std::nullopt;
        return img;
    }

    void SqliteHashCache::putThumbnail(
        const FileIdentity& fi, int width, int rotation, const QImage& image)
    {
        ensureOpen();
        if (!m_valid_) return;

        QSqlQuery q(m_db_);
        // Find file id
        q.prepare("SELECT id FROM files WHERE name=:name AND path=:path;");
        q.bindValue(":name", fi.name());
        q.bindValue(":path", fi.path());
        if (!execOrLog(q, "find file for thumbnail") || !q.next()) return;
        int fileId = q.value(0).toInt();

        // Serialize QImage as JPEG BLOB
        QByteArray blob;
        QBuffer buf(&blob);
        buf.open(QIODevice::WriteOnly);
        QImageWriter writer(&buf, "JPEG");
        writer.setQuality(85);
        if (!writer.write(image)) return;
        buf.close();

        // Upsert thumbnail
        q.prepare(R"(
            INSERT INTO thumbnails(file_id, width, rotation, data)
            VALUES(:file, :width, :rotation, :data)
            ON CONFLICT(file_id, width, rotation) DO UPDATE SET
                data = excluded.data;
        )");
        q.bindValue(":file", fileId);
        q.bindValue(":width", width);
        q.bindValue(":rotation", rotation);
        q.bindValue(":data", blob);
        execOrLog(q, "upsert thumbnail");
    }

    void SqliteHashCache::prune(const QString& path)
    {
        ensureOpen();
        if (!m_valid_) return;

        QSqlQuery q(m_db_);
        q.prepare(R"(
            DELETE FROM files
            WHERE path=:path AND (last_seen_scan_id IS NULL OR last_seen_scan_id!=:scanId);
        )");
        q.bindValue(":path", path);
        q.bindValue(":scanId", m_scanId_);

        if (!execOrLog(q, "prune files")) return;

        qDebug() << "[SqliteHashCache] Pruned" << q.numRowsAffected() << "stale entries for root" << path;
    }

} // namespace photoboss
