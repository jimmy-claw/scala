#include "calendar_module.h"

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api_provider.h>
#include <logos_api_client.h>
#endif

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

    // Get messaging module client for sync
    m_messagingClient = m_logosAPI->getClient("messaging_module");
    if (!m_messagingClient) {
        qWarning() << "LogosCalendar: failed to get messaging_module client"
                    << "(sync will use stub)";
    } else {
        m_sync->setMessagingClient(m_messagingClient);
    }

    qInfo() << "LogosCalendar: initialized. version:" << version();
    emit eventResponse("initialized", QVariantList() << "scala" << "0.1.0");
}
#endif

// ── Calendar operations ──────────────────────────────────────────────────────

QString LogosCalendar::createCalendar(const QString &name, const QString &color) {
    scala::Calendar cal;
    cal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    cal.name = name;
    cal.color = color;
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
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    ev.createdAt = now;
    ev.updatedAt = now;

    m_store.saveEvent(ev);

    // Broadcast to peers if calendar is syncing
    if (m_sync->isSyncing(calendarId)) {
        SyncMessage msg;
        msg.type = SyncMessageType::CreateEvent;
        msg.calendarId = calendarId;
        msg.payload = QString::fromUtf8(
            QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
        msg.timestamp = now;
        m_sync->sendMessage(calendarId, msg);
    }

    emit eventResponse("event_created", QVariantList() << ev.id << calendarId);
    return ev.id;
}

bool LogosCalendar::updateEvent(const QString &eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(eventJson.toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);
    ev.updatedAt = QDateTime::currentMSecsSinceEpoch();

    bool ok = m_store.updateEvent(ev);
    if (ok) {
        // Broadcast to peers if calendar is syncing
        if (m_sync->isSyncing(ev.calendarId)) {
            SyncMessage msg;
            msg.type = SyncMessageType::UpdateEvent;
            msg.calendarId = ev.calendarId;
            msg.payload = QString::fromUtf8(
                QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
            msg.timestamp = ev.updatedAt;
            m_sync->sendMessage(ev.calendarId, msg);
        }
        emit eventResponse("event_updated", QVariantList() << ev.id);
    }
    return ok;
}

bool LogosCalendar::deleteEvent(const QString &id) {
    // Get event before deleting to know the calendarId
    auto ev = m_store.getEvent(id);
    bool ok = m_store.deleteEvent(id);
    if (ok) {
        if (!ev.calendarId.isEmpty() && m_sync->isSyncing(ev.calendarId)) {
            SyncMessage msg;
            msg.type = SyncMessageType::DeleteEvent;
            msg.calendarId = ev.calendarId;
            msg.payload = id;
            msg.timestamp = QDateTime::currentMSecsSinceEpoch();
            m_sync->sendMessage(ev.calendarId, msg);
        }
        emit eventResponse("event_deleted", QVariantList() << id);
    }
    return ok;
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
