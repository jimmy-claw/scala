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
    : QObject(parent) {}

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

    emit eventResponse("event_created", QVariantList() << ev.id << calendarId);
    return ev.id;
}

bool LogosCalendar::updateEvent(const QString &eventJson) {
    QJsonDocument doc = QJsonDocument::fromJson(eventJson.toUtf8());
    QJsonObject obj = doc.object();

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(obj);
    ev.updatedAt = QDateTime::currentMSecsSinceEpoch();

    bool ok = m_store.updateEvent(ev);
    if (ok)
        emit eventResponse("event_updated", QVariantList() << ev.id);
    return ok;
}

bool LogosCalendar::deleteEvent(const QString &id) {
    bool ok = m_store.deleteEvent(id);
    if (ok)
        emit eventResponse("event_deleted", QVariantList() << id);
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
