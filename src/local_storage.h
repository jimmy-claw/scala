#pragma once

#include <QString>
#include <QMap>
#include <QObject>

/**
 * LocalStorage — file-based key-value store for Scala data persistence.
 *
 * Stores all data as a single JSON file in QStandardPaths::AppDataLocation.
 * - Writes are atomic: serialize to temp file, then rename (prevents corruption)
 * - Loads on construction: reads the JSON file if it exists
 * - Auto-saves after every write operation
 * - Zero external dependencies: pure Qt, works standalone and in basecamp
 *
 * This replaces the in-memory QMap fallback that was losing data on restart.
 */
class LocalStorage : public QObject {
    Q_OBJECT

public:
    explicit LocalStorage(QObject *parent = nullptr);
    ~LocalStorage() override;

    // ── Key-Value API ──────────────────────────────────────────────────────

    /// Set a key-value pair. Saves to disk immediately.
    void set(const QString &key, const QString &value);

    /// Get the value for a key. Returns empty string if not found.
    QString get(const QString &key) const;

    /// Remove a key. Saves to disk immediately. Returns true if key existed.
    bool remove(const QString &key);

    /// Check if a key exists.
    bool contains(const QString &key) const;

    /// Get all keys.
    QStringList keys() const;

    /// Clear all data. Saves empty state to disk.
    void clear();

    // ── Path management ────────────────────────────────────────────────────

    /// Get the directory where data is stored.
    QString dataDir() const;

    /// Get the full path to the storage file.
    QString storagePath() const;

    /// Force a save (normally automatic, useful for explicit flush).
    void save();

    // ── Diagnostics ────────────────────────────────────────────────────────

    /// Number of keys stored.
    int count() const;

    /// Total size of storage file on disk (bytes), or 0 if not saved yet.
    qint64 fileSize() const;

signals:
    /// Emitted after a successful save to disk.
    void saved();

    /// Emitted if a load error occurs (corrupt file, etc.).
    void loadError(const QString &error);

private:
    QString m_dataDir;
    QString m_storagePath;
    QMap<QString, QString> m_data;
    bool m_loaded;

    /// Load data from disk. Called on construction.
    void load();

    /// Validate a loaded JSON document before accepting it.
    bool validateLoadedData(const QJsonObject &obj);
};
