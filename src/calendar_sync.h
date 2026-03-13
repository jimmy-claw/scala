#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QString>

#ifdef LOGOS_CORE_AVAILABLE
class LogosAPIClient;
#endif

// ── Sync message types (mirror original js-waku types) ──────────────────────

enum class SyncMessageType {
    CreateEvent,
    UpdateEvent,
    DeleteEvent,
    SyncEvents,          // bulk sync on join
    UnshareCalendar,
    CalendarReconnected
};

// ── SyncMessage ─────────────────────────────────────────────────────────────

struct SyncMessage {
    SyncMessageType type = SyncMessageType::CreateEvent;
    QString calendarId;
    QString payload;     // JSON-encoded CalendarEvent or empty
    QString senderId;    // Logos identity pubkey
    qint64 timestamp = 0;

    QString signature;

    QJsonObject toJson() const;
    static SyncMessage fromJson(const QJsonObject &obj);
    QByteArray toBytes() const;
    static SyncMessage fromBytes(const QByteArray &data);

    static QString typeToString(SyncMessageType t);
    static SyncMessageType typeFromString(const QString &s);

    // Sign message with identity key (stub — real crypto in follow-up)
    // For now: HMAC-SHA256(key, payload) as signature field
    static QString sign(const QString &payload, const QString &key);
    static bool verify(const QString &payload, const QString &signature, const QString &key);
};

// ── CalendarSync ────────────────────────────────────────────────────────────

class CalendarSync : public QObject {
    Q_OBJECT

public:
    explicit CalendarSync(QObject *parent = nullptr);

    /// Start syncing a calendar (subscribe to its topic).
    void startSync(const QString &calendarId, const QString &encryptionKey);

    /// Stop syncing a calendar.
    void stopSync(const QString &calendarId);

    /// Send a sync message for a calendar.
    void sendMessage(const QString &calendarId, const SyncMessage &msg);

    /// Whether a calendar is currently syncing.
    bool isSyncing(const QString &calendarId) const;

    /// Topic format: /scala/1/<calendarId>/json
    static QString topicForCalendar(const QString &calendarId);

#ifdef LOGOS_CORE_AVAILABLE
    void setDeliveryClient(LogosAPIClient *client);
#endif

signals:
    void messageReceived(const QString &calendarId, const SyncMessage &msg);
    void syncStarted(const QString &calendarId);
    void syncStopped(const QString &calendarId);
    void syncError(const QString &calendarId, const QString &error);

private:
    // calendarId -> encryption key
    QMap<QString, QString> m_activeTopics;
    QMap<QString, QString> m_subscribedTopics;

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPIClient *m_deliveryClient = nullptr;
    bool m_deliveryNodeStarted = false;
    bool m_deliveryNodeStarting = false;
    QStringList m_pendingSubscriptions;

    void ensureDeliveryNode();
    void onDeliveryMessageReceived(const QString &hash, const QString &topic,
                                   const QString &payloadBase64, qint64 timestamp);
#endif
};
