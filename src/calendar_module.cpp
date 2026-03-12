#include <QTimer>
#include "calendar_module.h"

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api_provider.h>
#include <logos_api_client.h>
#endif

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

// ── Construction ─────────────────────────────────────────────────────────────

LogosCalendar::LogosCalendar(QObject *parent)
    : QObject(parent)
    , m_sync(new CalendarSync(this))
{
    connect(m_sync, &CalendarSync::messageReceived,
            this, &LogosCalendar::onSyncMessageReceived);
    connect(m_sync, &CalendarSync::syncStarted,
            this, [this](const QString &calendarId) {
        emit syncStatusChanged(calendarId, QStringLiteral("syncing"));
    });
    connect(m_sync, &CalendarSync::syncStopped,
            this, [this](const QString &calendarId) {
        emit syncStatusChanged(calendarId, QStringLiteral("offline"));
    });

    // Load stored identity or generate stable fallback
    QString stored = m_store.kvGet(QStringLiteral("identity"));
    if (!stored.isEmpty()) {
        m_identity = stored;
    } else {
        m_identity = generateStableIdentity();
        m_store.kvSet(QStringLiteral("identity"), m_identity);
    }
}

// ── Namespace ────────────────────────────────────────────────────────────────

void LogosCalendar::setNamespace(const QString &ns) {
    m_store.setNamespace(ns);
}

// ── Identity ─────────────────────────────────────────────────────────────────

QString LogosCalendar::getIdentity() const {
    return m_identity;
}

void LogosCalendar::setIdentity(const QString &pubkeyHex) {
    if (m_identity != pubkeyHex) {
        m_identity = pubkeyHex;
        m_store.kvSet(QStringLiteral("identity"), m_identity);
        emit identityChanged();
    }
}

QString LogosCalendar::generateStableIdentity() {
    QByteArray machineId = QSysInfo::machineUniqueId();
    if (machineId.isEmpty())
        machineId = QByteArray("scala-fallback-id");
    QByteArray hash = QCryptographicHash::hash(
        machineId, QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

// ── Logos Core lifecycle ─────────────────────────────────────────────────────

#ifdef LOGOS_CORE_AVAILABLE
void LogosCalendar::initLogos(LogosAPI *logosAPIInstance) {
    m_logosAPI = logosAPIInstance;

    if (!m_logosAPI) {
        qWarning() << "LogosCalendar: initLogos called with null LogosAPI";
        qInfo() << "LogosCalendar: initialized (headless). version:" << version();
        return;
    }

    // NOTE: Do NOT call logosAPI->getProvider()->registerObject() here.
    // The SDK wraps us in a ModuleProxy that handles registration automatically.

    m_kvClient = m_logosAPI->getClient("kv_module");
    if (!m_kvClient) {
        qWarning() << "LogosCalendar: failed to get kv_module client";
    } else {
        m_store.setClient(m_kvClient);
    }

    // Skip optional module lookups in e2e/CI mode (they timeout ~20s each)
    if (qgetenv("SCALA_E2E_MINIMAL").isEmpty()) {
        // Get messaging module client for sync
        m_messagingClient = m_logosAPI->getClient("messaging_module");
        if (!m_messagingClient) {
            qWarning() << "LogosCalendar: failed to get messaging_module client"
                        << "(sync will use stub)";
        } else {
            m_sync->setMessagingClient(m_messagingClient);
        }

        // Get identity from accounts module (optional — may not be loaded)
        auto accountsClient = m_logosAPI->getClient("accounts_module");
        if (accountsClient) {
            QTimer::singleShot(500, this, [this, accountsClient]() {
                QVariant result = accountsClient->invokeRemoteMethod(
                    "accounts_module", "getActiveAccountPubkey");
                QString pubkey = result.toString();
                if (!pubkey.isEmpty()) {
                    m_identity = pubkey;
                    m_store.kvSet(QStringLiteral("identity"), m_identity);
                }
            });
        }
    } else {
        qInfo() << "LogosCalendar: SCALA_E2E_MINIMAL set — skipping messaging/accounts lookups";
    }

    qInfo() << "LogosCalendar: initialized. version:" << version()
            << "identity:" << m_identity.left(8) + "...";
    emit eventResponse("initialized", QVariantList() << "scala" << "0.1.0");
}
#endif

// ── Calendar operations ──────────────────────────────────────────────────────

QString LogosCalendar::createCalendar(const QString &name, const QString &color) {
    scala::Calendar cal;
    cal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cal.name = name;
    cal.color = color;
    cal.creatorId = m_identity;
    cal.isShared = false;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    cal.createdAt = now;
    cal.updatedAt = now;

    m_store.saveCalendar(cal);

    emit eventResponse("calendar_created", QVariantList() << cal.id << name);
    return cal.id;
}

QString LogosCalendar::listCalendars() {
    auto calendars = m_store.listCalendars();
    QJsonArray arr;
    for (const auto &cal : calendars)
        arr.append(cal.toJson());
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

bool LogosCalendar::deleteCalendar(const QString &id) {
    // Stop sync if running before deleting
    if (m_sync->isSyncing(id))
        m_sync->stopSync(id);

    bool ok = m_store.deleteCalendar(id);
    if (ok)
        emit eventResponse("calendar_deleted", QVariantList() << id);
    return ok;
}

// ── Event operations ─────────────────────────────────────────────────────────

QString LogosCalendar::createEvent(const QString &calendarId, const QString &eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(eventJson.toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);
    ev.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    ev.calendarId = calendarId;
    ev.creatorId = m_identity;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ev.createdAt = now;
    ev.updatedAt = now;

    m_store.saveEvent(ev);

    // Broadcast to peers if calendar is syncing
    if (m_sync->isSyncing(calendarId)) {
        SyncMessage msg;
        msg.type = SyncMessageType::CreateEvent;
        msg.calendarId = calendarId;
        msg.senderId = m_identity;
        msg.payload = QString::fromUtf8(
            QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
        msg.timestamp = now;
        m_sync->sendMessage(calendarId, msg);
    }

    emit eventResponse("event_created", QVariantList() << ev.id << calendarId);
    return ev.id;
}

QString LogosCalendar::updateEvent(const QString &eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(eventJson.toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);

    // Ownership check: only creator can update
    auto existing = m_store.getEvent(ev.id);
    if (!existing.id.isEmpty() && !existing.creatorId.isEmpty()
        && existing.creatorId != m_identity) {
        QJsonObject err;
        err["error"] = QStringLiteral("not_authorized");
        return QString::fromUtf8(QJsonDocument(err).toJson(QJsonDocument::Compact));
    }

    ev.creatorId = existing.creatorId;
    ev.updatedAt = QDateTime::currentMSecsSinceEpoch();

    bool ok = m_store.updateEvent(ev);
    if (ok) {
        // Broadcast to peers if calendar is syncing
        if (m_sync->isSyncing(ev.calendarId)) {
            SyncMessage msg;
            msg.type = SyncMessageType::UpdateEvent;
            msg.calendarId = ev.calendarId;
            msg.senderId = m_identity;
            msg.payload = QString::fromUtf8(
                QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
            msg.timestamp = ev.updatedAt;
            m_sync->sendMessage(ev.calendarId, msg);
        }
        emit eventResponse("event_updated", QVariantList() << ev.id);
        return ev.id;
    }
    return {};
}

QString LogosCalendar::deleteEvent(const QString &id) {
    // Get event before deleting to check ownership and know calendarId
    auto ev = m_store.getEvent(id);

    // Ownership check: only creator can delete
    if (!ev.id.isEmpty() && !ev.creatorId.isEmpty()
        && ev.creatorId != m_identity) {
        QJsonObject err;
        err["error"] = QStringLiteral("not_authorized");
        return QString::fromUtf8(QJsonDocument(err).toJson(QJsonDocument::Compact));
    }

    bool ok = m_store.deleteEvent(id);
    if (ok) {
        if (!ev.calendarId.isEmpty() && m_sync->isSyncing(ev.calendarId)) {
            SyncMessage msg;
            msg.type = SyncMessageType::DeleteEvent;
            msg.calendarId = ev.calendarId;
            msg.senderId = m_identity;
            msg.payload = id;
            msg.timestamp = QDateTime::currentMSecsSinceEpoch();
            m_sync->sendMessage(ev.calendarId, msg);
        }
        emit eventResponse("event_deleted", QVariantList() << id);
        return id;
    }
    return {};
}

QString LogosCalendar::listEvents(const QString &calendarId) {
    auto events = m_store.listEvents(calendarId);
    QJsonArray arr;
    for (const auto &ev : events)
        arr.append(ev.toJson());
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

QString LogosCalendar::getEvent(const QString &id) {
    auto ev = m_store.getEvent(id);
    if (ev.id.isEmpty())
        return {};
    return QString::fromUtf8(
        QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
}

// ── Sync API ─────────────────────────────────────────────────────────────────

QString LogosCalendar::shareCalendar(const QString &calendarId) {
    auto cal = m_store.getCalendar(calendarId);
    if (cal.id.isEmpty())
        return {};

    // Generate encryption key if not already shared
    if (cal.encryptionKey.isEmpty()) {
        cal.encryptionKey = QUuid::createUuid().toString(QUuid::WithoutBraces)
                          + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    cal.isShared = true;
    cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
    m_store.saveCalendar(cal);

    m_sync->startSync(calendarId, cal.encryptionKey);

    emit eventResponse("calendar_shared", QVariantList() << calendarId);
    return cal.encryptionKey;
}

bool LogosCalendar::joinSharedCalendar(const QString &calendarId,
                                       const QString &encryptionKey) {
    if (calendarId.isEmpty() || encryptionKey.isEmpty())
        return false;

    // Create local calendar entry if it doesn't exist
    auto cal = m_store.getCalendar(calendarId);
    if (cal.id.isEmpty()) {
        cal.id = calendarId;
        cal.name = QStringLiteral("Shared Calendar");
        cal.color = QStringLiteral("#9C27B0");
        cal.isShared = true;
        cal.encryptionKey = encryptionKey;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        cal.createdAt = now;
        cal.updatedAt = now;
        m_store.saveCalendar(cal);
    } else {
        cal.isShared = true;
        cal.encryptionKey = encryptionKey;
        cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
        m_store.saveCalendar(cal);
    }

    m_sync->startSync(calendarId, encryptionKey);

    // Request bulk sync from peers
    SyncMessage msg;
    msg.type = SyncMessageType::SyncEvents;
    msg.calendarId = calendarId;
    msg.senderId = m_identity;
    msg.timestamp = QDateTime::currentMSecsSinceEpoch();
    m_sync->sendMessage(calendarId, msg);

    emit eventResponse("calendar_joined", QVariantList() << calendarId);
    return true;
}

QString LogosCalendar::getSyncStatus(const QString &calendarId) {
    auto cal = m_store.getCalendar(calendarId);
    if (cal.id.isEmpty() || !cal.isShared)
        return QStringLiteral("not_shared");

    if (m_sync->isSyncing(calendarId))
        return QStringLiteral("syncing");

    return QStringLiteral("offline");
}

// ── Share link API ───────────────────────────────────────────────────────────

QString LogosCalendar::generateShareLink(const QString &calendarId) {
    auto cal = m_store.getCalendar(calendarId);
    if (cal.id.isEmpty())
        return {};

    // Ensure calendar is shared (generates key if needed)
    if (!cal.isShared || cal.encryptionKey.isEmpty())
        shareCalendar(calendarId);

    // Re-read after shareCalendar may have updated the key
    cal = m_store.getCalendar(calendarId);
    if (cal.encryptionKey.isEmpty())
        return {};

    QByteArray base64Key = cal.encryptionKey.toUtf8().toBase64(QByteArray::Base64UrlEncoding
                                                               | QByteArray::OmitTrailingEquals);

    QUrl url;
    url.setScheme(QStringLiteral("scala"));
    url.setHost(QStringLiteral("join"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), calendarId);
    query.addQueryItem(QStringLiteral("key"), QString::fromLatin1(base64Key));
    query.addQueryItem(QStringLiteral("name"), cal.name);
    url.setQuery(query);

    return url.toString();
}

QString LogosCalendar::parseShareLink(const QString &link) {
    QUrl url(link);
    if (!url.isValid() || url.scheme() != QStringLiteral("scala")
        || url.host() != QStringLiteral("join"))
        return {};

    QUrlQuery query(url);
    QString id = query.queryItemValue(QStringLiteral("id"));
    QString base64Key = query.queryItemValue(QStringLiteral("key"));
    QString name = query.queryItemValue(QStringLiteral("name"));

    if (id.isEmpty() || base64Key.isEmpty())
        return {};

    QByteArray key = QByteArray::fromBase64(base64Key.toLatin1(),
                                            QByteArray::Base64UrlEncoding
                                            | QByteArray::OmitTrailingEquals);

    QJsonObject result;
    result[QStringLiteral("id")] = id;
    result[QStringLiteral("key")] = QString::fromUtf8(key);
    result[QStringLiteral("name")] = name;

    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

bool LogosCalendar::handleShareLink(const QString &link) {
    QString parsed = parseShareLink(link);
    if (parsed.isEmpty())
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(parsed.toUtf8());
    QJsonObject obj = doc.object();

    QString id = obj[QStringLiteral("id")].toString();
    QString key = obj[QStringLiteral("key")].toString();
    QString name = obj[QStringLiteral("name")].toString();

    if (id.isEmpty() || key.isEmpty())
        return false;

    // If calendar doesn't exist yet, create with the shared name
    auto cal = m_store.getCalendar(id);
    if (cal.id.isEmpty() && !name.isEmpty()) {
        cal.id = id;
        cal.name = name;
        cal.color = QStringLiteral("#9C27B0");
        cal.isShared = true;
        cal.encryptionKey = key;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        cal.createdAt = now;
        cal.updatedAt = now;
        m_store.saveCalendar(cal);
    }

    return joinSharedCalendar(id, key);
}

// ── Search API ───────────────────────────────────────────────────────────────

QString LogosCalendar::searchEvents(const QString &query) {
    if (query.trimmed().isEmpty())
        return QStringLiteral("[]");

    QString lowerQuery = query.toLower();
    QJsonArray results;

    auto calendars = m_store.listCalendars();
    for (const auto &cal : calendars) {
        auto events = m_store.listEvents(cal.id);
        for (const auto &ev : events) {
            if (ev.title.toLower().contains(lowerQuery)
                || ev.description.toLower().contains(lowerQuery)
                || ev.location.toLower().contains(lowerQuery)) {
                QJsonObject obj = ev.toJson();
                obj[QStringLiteral("calendarName")] = cal.name;
                obj[QStringLiteral("calendarColor")] = cal.color;
                results.append(obj);
            }
        }
    }

    return QString::fromUtf8(QJsonDocument(results).toJson(QJsonDocument::Compact));
}

// ── Reminders API ────────────────────────────────────────────────────────────

QString LogosCalendar::getPendingReminders() {
    QJsonArray pending;
    QDateTime now = QDateTime::currentDateTime();

    auto calendars = m_store.listCalendars();
    for (const auto &cal : calendars) {
        auto events = m_store.listEvents(cal.id);
        for (const auto &ev : events) {
            if (ev.reminderMinutes < 0)
                continue;

            // Check if already reminded
            QString reminderKey = QStringLiteral("reminder:") + ev.id;
            if (m_store.kvGet(reminderKey) == QStringLiteral("fired"))
                continue;

            // Check if event starts within the reminder window
            qint64 msBefore = static_cast<qint64>(ev.reminderMinutes) * 60 * 1000;
            QDateTime reminderTime = ev.startTime.addMSecs(-msBefore);
            if (now >= reminderTime && now < ev.startTime) {
                QJsonObject obj;
                obj[QStringLiteral("id")] = ev.id;
                obj[QStringLiteral("title")] = ev.title;
                obj[QStringLiteral("startTime")] = ev.startTime.toMSecsSinceEpoch();
                obj[QStringLiteral("calendarId")] = ev.calendarId;
                pending.append(obj);

                // Mark as fired
                m_store.kvSet(reminderKey, QStringLiteral("fired"));
            }
        }
    }

    return QString::fromUtf8(QJsonDocument(pending).toJson(QJsonDocument::Compact));
}

// ── Settings API ─────────────────────────────────────────────────────────────

void LogosCalendar::setSetting(const QString &key, const QString &value) {
    m_store.kvSet(QStringLiteral("setting_") + key, value);
}

QString LogosCalendar::getSetting(const QString &key, const QString &defaultValue) {
    QString val = m_store.kvGet(QStringLiteral("setting_") + key);
    return val.isEmpty() ? defaultValue : val;
}

// ── Incoming sync messages ───────────────────────────────────────────────────

void LogosCalendar::onSyncMessageReceived(const QString &calendarId,
                                           const SyncMessage &msg) {
    switch (msg.type) {
    case SyncMessageType::CreateEvent: {
        QJsonDocument doc = QJsonDocument::fromJson(msg.payload.toUtf8());
        scala::CalendarEvent ev = scala::CalendarEvent::fromJson(doc.object());
        if (!ev.id.isEmpty()) {
            m_store.saveEvent(ev);
            emit eventResponse("event_created", QVariantList() << ev.id << calendarId);
        }
        break;
    }
    case SyncMessageType::UpdateEvent: {
        QJsonDocument doc = QJsonDocument::fromJson(msg.payload.toUtf8());
        scala::CalendarEvent ev = scala::CalendarEvent::fromJson(doc.object());
        if (!ev.id.isEmpty()) {
            m_store.updateEvent(ev);
            emit eventResponse("event_updated", QVariantList() << ev.id);
        }
        break;
    }
    case SyncMessageType::DeleteEvent: {
        QString eventId = msg.payload;
        if (!eventId.isEmpty()) {
            m_store.deleteEvent(eventId);
            emit eventResponse("event_deleted", QVariantList() << eventId);
        }
        break;
    }
    case SyncMessageType::SyncEvents: {
        // Peer is requesting bulk sync — send all events for this calendar
        auto events = m_store.listEvents(calendarId);
        for (const auto &ev : events) {
            SyncMessage reply;
            reply.type = SyncMessageType::CreateEvent;
            reply.calendarId = calendarId;
            reply.senderId = m_identity;
            reply.payload = QString::fromUtf8(
                QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
            reply.timestamp = QDateTime::currentMSecsSinceEpoch();
            m_sync->sendMessage(calendarId, reply);
        }
        break;
    }
    case SyncMessageType::UnshareCalendar: {
        m_sync->stopSync(calendarId);
        auto cal = m_store.getCalendar(calendarId);
        if (!cal.id.isEmpty()) {
            cal.isShared = false;
            cal.encryptionKey.clear();
            cal.updatedAt = QDateTime::currentMSecsSinceEpoch();
            m_store.saveCalendar(cal);
        }
        emit eventResponse("calendar_unshared", QVariantList() << calendarId);
        break;
    }
    case SyncMessageType::CalendarReconnected: {
        qDebug() << "CalendarSync: peer reconnected for" << calendarId;
        break;
    }
    }
}

// ── Encryption API ──────────────────────────────────────────────────────────

QByteArray LogosCalendar::deriveKey(const QString &password, const QByteArray &salt) {
    // PBKDF2-SHA256 with 100 000 iterations → 32-byte key
    // Qt provides QMessageAuthenticationCode for HMAC, but not PBKDF2 directly.
    // We implement PBKDF2 using QMessageAuthenticationCode (HMAC-SHA256) per RFC 2898.
    constexpr int iterations = 100000;
    constexpr int keyLen = 32; // 256 bits

    QByteArray password_bytes = password.toUtf8();
    QByteArray derived(keyLen, '\0');

    // PBKDF2 block 1 (dkLen ≤ hLen → single block)
    QByteArray u = salt;
    u.append('\x00'); u.append('\x00'); u.append('\x00'); u.append('\x01'); // INT(1) big-endian

    QByteArray T = QMessageAuthenticationCode::hash(u, password_bytes, QCryptographicHash::Sha256);
    QByteArray prev = T;

    for (int i = 1; i < iterations; ++i) {
        QByteArray next = QMessageAuthenticationCode::hash(prev, password_bytes,
                                                           QCryptographicHash::Sha256);
        for (int j = 0; j < T.size(); ++j)
            T[j] = static_cast<char>(
                static_cast<unsigned char>(T[j]) ^
                static_cast<unsigned char>(next[j]));
        prev = next;
    }

    return T.left(keyLen);
}

bool LogosCalendar::enableEncryption(const QString &password) {
    if (password.isEmpty()) {
        qWarning() << "LogosCalendar::enableEncryption: password must not be empty";
        return false;
    }

    // Load or generate salt (stored plaintext — salt is not secret)
    const QString saltKey = QStringLiteral("encryption:salt");
    QString saltHex = m_store.kvGet(saltKey);
    QByteArray salt;
    if (saltHex.isEmpty()) {
        // First-time setup — generate a random 16-byte salt
        salt.resize(16);
        for (int i = 0; i < salt.size(); ++i)
            salt[i] = static_cast<char>(
                QCryptographicHash::hash(
                    QByteArray::number(QDateTime::currentMSecsSinceEpoch() + i),
                    QCryptographicHash::Sha256)[i % 32]);
        saltHex = QString::fromLatin1(salt.toHex());
        m_store.kvSet(saltKey, saltHex);
        qInfo() << "LogosCalendar: generated new encryption salt";
    } else {
        salt = QByteArray::fromHex(saltHex.toLatin1());
        qInfo() << "LogosCalendar: loaded existing encryption salt";
    }

    QByteArray key = deriveKey(password, salt);
    if (key.size() != 32) {
        qWarning() << "LogosCalendar::enableEncryption: key derivation failed";
        return false;
    }

    m_store.enableEncryption(key);
    m_encryptionEnabled = true;

    emit encryptionChanged(true);
    qInfo() << "LogosCalendar: at-rest encryption enabled";
    return true;
}

void LogosCalendar::disableEncryption() {
    m_store.disableEncryption();
    m_encryptionEnabled = false;
    emit encryptionChanged(false);
    qInfo() << "LogosCalendar: at-rest encryption disabled";
}

bool LogosCalendar::isEncryptionEnabled() const {
    return m_encryptionEnabled;
}
