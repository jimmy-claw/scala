#include "local_storage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

LocalStorage::LocalStorage(QObject *parent)
    : QObject(parent)
    , m_loaded(false)
{
    // Determine storage directory
    m_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/scala-data";
    QDir().mkpath(m_dataDir);

    m_storagePath = m_dataDir + "/calendar_store.json";

    // Load existing data
    load();
}

LocalStorage::~LocalStorage() {
    // Final save on destruction
    if (!m_data.isEmpty()) {
        save();
    }
}

void LocalStorage::load() {
    QFile file(m_storagePath);
    if (!file.exists()) {
        qDebug() << "LocalStorage: no existing data at" << m_storagePath;
        m_loaded = true;
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "LocalStorage: cannot open" << m_storagePath << file.errorString();
        emit loadError(file.errorString());
        m_loaded = true;  // continue with empty data
        return;
    }

    QByteArray rawData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "LocalStorage: JSON parse error at line" << parseError.offset
                   << ":" << parseError.errorString();
        emit loadError(parseError.errorString());

        // Attempt recovery: rename corrupt file and start fresh
        QString backupPath = m_storagePath + ".corrupt." + QString::number(QDateTime::currentMSecsSinceEpoch());
        QFile::copy(m_storagePath, backupPath);
        qWarning() << "LocalStorage: backed up corrupt file to" << backupPath;
        m_loaded = true;
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "LocalStorage: expected JSON object at top level";
        emit loadError("Expected JSON object");
        m_loaded = true;
        return;
    }

    QJsonObject obj = doc.object();
    if (validateLoadedData(obj)) {
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            m_data.insert(it.key(), it.value().toString());
        }
        qDebug() << "LocalStorage: loaded" << m_data.size() << "keys from" << m_storagePath;
    } else {
        qWarning() << "LocalStorage: validation failed, starting with empty store";
        emit loadError("Validation failed");
    }

    m_loaded = true;
}

bool LocalStorage::validateLoadedData(const QJsonObject &obj) {
    // Basic validation: all values must be strings
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        if (!it.value().isString()) {
            qWarning() << "LocalStorage: non-string value for key" << it.key();
            return false;
        }
    }
    return true;
}

void LocalStorage::set(const QString &key, const QString &value) {
    if (value.isEmpty()) {
        // Empty value = delete
        remove(key);
        return;
    }

    m_data.insert(key, value);
    save();
}

QString LocalStorage::get(const QString &key) const {
    return m_data.value(key);
}

bool LocalStorage::remove(const QString &key) {
    if (m_data.remove(key)) {
        save();
        return true;
    }
    return false;
}

bool LocalStorage::contains(const QString &key) const {
    return m_data.contains(key);
}

QStringList LocalStorage::keys() const {
    return m_data.keys();
}

void LocalStorage::clear() {
    m_data.clear();
    save();
}

QString LocalStorage::dataDir() const {
    return m_dataDir;
}

QString LocalStorage::storagePath() const {
    return m_storagePath;
}

void LocalStorage::save() {
    QJsonObject obj;
    for (auto it = m_data.constBegin(); it != m_data.constEnd(); ++it) {
        obj.insert(it.key(), it.value());
    }

    QJsonDocument doc(obj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // Atomic write: write to temp file, then rename
    QString tempPath = m_storagePath + ".tmp";
    QFile file(tempPath);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "LocalStorage: cannot open temp file" << tempPath << file.errorString();
        return;
    }

    file.write(jsonData);
    file.flush();
    file.close();

    // Atomic rename
    if (!QFile::rename(tempPath, m_storagePath)) {
        qWarning() << "LocalStorage: failed to rename" << tempPath << "to" << m_storagePath;
        QFile::remove(tempPath);  // clean up
        return;
    }

    emit saved();
}

int LocalStorage::count() const {
    return m_data.size();
}

qint64 LocalStorage::fileSize() const {
    QFile file(m_storagePath);
    if (!file.exists())
        return 0;
    return file.size();
}
