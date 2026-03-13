#include "calendar_sync.h"

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api_client.h>
#endif

#include <QDebug>
#include <QJsonDocument>
#include <QMessageAuthenticationCode>

// ── SyncMessage helpers ─────────────────────────────────────────────────────

QString SyncMessage::typeToString(SyncMessageType t) {
    switch (t) {
    case SyncMessageType::CreateEvent:         return QStringLiteral("CreateEvent");
    case SyncMessageType::UpdateEvent:         return QStringLiteral("UpdateEvent");
    case SyncMessageType::DeleteEvent:         return QStringLiteral("DeleteEvent");
    case SyncMessageType::SyncEvents:          return QStringLiteral("SyncEvents");
    case SyncMessageType::UnshareCalendar:     return QStringLiteral("UnshareCalendar");
    case SyncMessageType::CalendarReconnected: return QStringLiteral("CalendarReconnected");
    }
    return QStringLiteral("CreateEvent");
}

SyncMessageType SyncMessage::typeFromString(const QString &s) {
    if (s == QLatin1String("CreateEvent"))         return SyncMessageType::CreateEvent;
    if (s == QLatin1String("UpdateEvent"))         return SyncMessageType::UpdateEvent;
    if (s == QLatin1String("DeleteEvent"))         return SyncMessageType::DeleteEvent;
    if (s == QLatin1String("SyncEvents"))          return SyncMessageType::SyncEvents;
    if (s == QLatin1String("UnshareCalendar"))     return SyncMessageType::UnshareCalendar;
    if (s == QLatin1String("CalendarReconnected")) return SyncMessageType::CalendarReconnected;
    return SyncMessageType::CreateEvent;
}

QJsonObject SyncMessage::toJson() const {
    QJsonObject obj;
    obj[QLatin1String("type")]       = typeToString(type);
    obj[QLatin1String("calendarId")] = calendarId;
    obj[QLatin1String("payload")]    = payload;
    obj[QLatin1String("senderId")]   = senderId;
    obj[QLatin1String("timestamp")]  = timestamp;
    if (!signature.isEmpty())
        obj[QLatin1String("signature")] = signature;
    return obj;
}

SyncMessage SyncMessage::fromJson(const QJsonObject &obj) {
    SyncMessage msg;
    msg.type       = typeFromString(obj[QLatin1String("type")].toString());
    msg.calendarId = obj[QLatin1String("calendarId")].toString();
    msg.payload    = obj[QLatin1String("payload")].toString();
    msg.senderId   = obj[QLatin1String("senderId")].toString();
    msg.timestamp  = static_cast<qint64>(obj[QLatin1String("timestamp")].toDouble());
    msg.signature  = obj[QLatin1String("signature")].toString();
    return msg;
}

QByteArray SyncMessage::toBytes() const {
    return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
}

SyncMessage SyncMessage::fromBytes(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return fromJson(doc.object());
}

QString SyncMessage::sign(const QString &payload, const QString &key) {
    QByteArray mac = QMessageAuthenticationCode::hash(
        payload.toUtf8(), key.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(mac.toHex());
}

bool SyncMessage::verify(const QString &payload, const QString &signature,
                          const QString &key) {
    return sign(payload, key) == signature;
}

// ── CalendarSync ────────────────────────────────────────────────────────────

CalendarSync::CalendarSync(QObject *parent)
    : QObject(parent) {}

QString CalendarSync::topicForCalendar(const QString &calendarId) {
    return QStringLiteral("/scala/1/") + calendarId + QStringLiteral("/json");
}

bool CalendarSync::isSyncing(const QString &calendarId) const {
    return m_activeTopics.contains(calendarId);
}

#ifdef LOGOS_CORE_AVAILABLE
void CalendarSync::setDeliveryClient(LogosAPIClient *client) {
    m_deliveryClient = client;

    if (m_deliveryClient) {
        ensureDeliveryNode();

        // Listen for messageReceived events from delivery_module
        QObject *replica = m_deliveryClient->requestObject("delivery_module");
        if (replica) {
            m_deliveryClient->onEvent(
                replica, this, QStringLiteral("messageReceived"),
                [this](const QString &eventName, const QVariantList &args) {
                    Q_UNUSED(eventName)
                    if (args.size() >= 4) {
                        onDeliveryMessageReceived(
                            args.at(0).toString(),   // hash
                            args.at(1).toString(),   // topic
                            args.at(2).toString(),   // payload_base64
                            args.at(3).toLongLong()  // timestamp
                        );
                    }
                });
            qInfo() << "CalendarSync: registered messageReceived event handler";
        } else {
            qWarning() << "CalendarSync: failed to get delivery_module replica for events";
        }
    }
}

void CalendarSync::ensureDeliveryNode() {
    if (m_deliveryNodeStarted || m_deliveryNodeStarting || !m_deliveryClient)
        return;

    m_deliveryNodeStarting = true;

    // Register connectionStateChanged — subscribe pending calendars only when connected
    QObject *replica = m_deliveryClient->requestObject("delivery_module");
    if (replica) {
        m_deliveryClient->onEvent(
            replica, this, QStringLiteral("connectionStateChanged"),
            [this](const QString &, const QVariantList &args) {
                QString status = args.value(0).toString();
                qInfo() << "CalendarSync: delivery connection state:" << status;
                if (!m_deliveryNodeStarted &&
                    (status.toLower() == "connected")) {
                    m_deliveryNodeStarted = true;
                    for (const QString &calendarId : m_pendingSubscriptions) {
                        QString topic = QStringLiteral("/scala/1/%1/json").arg(calendarId);
                        m_deliveryClient->invokeRemoteMethod(
                            "delivery_module", "subscribe", topic);
                        m_subscribedTopics.insert(calendarId, topic);
                        qInfo() << "CalendarSync: subscribed to topic" << topic;
                    }
                    m_pendingSubscriptions.clear();
                }
            });
    }

    // Create and start the delivery node
    m_deliveryClient->invokeRemoteMethod(
        "delivery_module", "createNode",
        QStringLiteral(R"({"logLevel":"INFO","mode":"Core","preset":"logos.dev"})"));
    m_deliveryClient->invokeRemoteMethod("delivery_module", "start");
    qInfo() << "CalendarSync: delivery node starting...";
}

void CalendarSync::onDeliveryMessageReceived(const QString &hash, const QString &topic,
                                              const QString &payloadBase64, qint64 timestamp) {
    Q_UNUSED(hash)
    Q_UNUSED(timestamp)

    // Find which calendar this topic belongs to
    for (auto it = m_activeTopics.constBegin(); it != m_activeTopics.constEnd(); ++it) {
        if (topicForCalendar(it.key()) == topic) {
            QByteArray payload = QByteArray::fromBase64(payloadBase64.toUtf8());
            SyncMessage msg = SyncMessage::fromBytes(payload);
            emit messageReceived(it.key(), msg);
            return;
        }
    }
    qDebug() << "CalendarSync: received message for unknown topic" << topic;
}
#endif

void CalendarSync::startSync(const QString &calendarId, const QString &encryptionKey) {
    if (m_activeTopics.contains(calendarId)) {
        qDebug() << "CalendarSync: already syncing" << calendarId;
        return;
    }

    const QString topic = topicForCalendar(calendarId);
    m_activeTopics.insert(calendarId, encryptionKey);

#ifdef LOGOS_CORE_AVAILABLE
    if (m_deliveryClient) {
        ensureDeliveryNode();
        if (m_deliveryNodeStarted) {
            // Node already connected — subscribe immediately
            m_deliveryClient->invokeRemoteMethod(
                "delivery_module", "subscribe", topic);
            m_subscribedTopics.insert(calendarId, topic);
            qInfo() << "CalendarSync: subscribed to topic" << topic;
        } else {
            // Node starting — queue for when connection is established
            m_pendingSubscriptions.append(calendarId);
            qInfo() << "CalendarSync: queued subscription for" << calendarId;
        }
        emit syncStarted(calendarId);
        return;
    }
    qWarning() << "CalendarSync: no delivery client, falling back to stub";
#endif

    // Stub: emit syncStarted immediately for testing without Logos Core
    qDebug() << "CalendarSync [stub]: startSync" << calendarId
             << "topic:" << topic;
    emit syncStarted(calendarId);
}

void CalendarSync::stopSync(const QString &calendarId) {
    if (!m_activeTopics.contains(calendarId)) {
        qDebug() << "CalendarSync: not syncing" << calendarId;
        return;
    }

    const QString topic = topicForCalendar(calendarId);
    m_activeTopics.remove(calendarId);

#ifdef LOGOS_CORE_AVAILABLE
    if (m_deliveryClient) {
        // delivery_module does not have an explicit unsubscribe;
        // topic is simply no longer tracked locally
        qInfo() << "CalendarSync: unsubscribed from topic" << topic;
        emit syncStopped(calendarId);
        return;
    }
#endif

    qDebug() << "CalendarSync [stub]: stopSync" << calendarId;
    emit syncStopped(calendarId);
}

void CalendarSync::sendMessage(const QString &calendarId, const SyncMessage &msg) {
    if (!m_activeTopics.contains(calendarId)) {
        emit syncError(calendarId,
                       QStringLiteral("Cannot send: calendar not syncing"));
        return;
    }

    const QString topic = topicForCalendar(calendarId);
    const QByteArray data = msg.toBytes();

#ifdef LOGOS_CORE_AVAILABLE
    if (m_deliveryClient) {
        ensureDeliveryNode();
        m_deliveryClient->invokeRemoteMethod(
            "delivery_module", "send", topic, data);

        qDebug() << "CalendarSync: sent" << SyncMessage::typeToString(msg.type)
                 << "to" << topic << "(" << data.size() << "bytes)";
        return;
    }
#endif

    // Stub: log the message for debugging
    qDebug() << "CalendarSync [stub]: sendMessage"
             << SyncMessage::typeToString(msg.type)
             << "calendarId:" << calendarId
             << "payload size:" << data.size();
}
