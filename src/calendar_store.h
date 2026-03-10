#pragma once

#include "types.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>

#ifdef LOGOS_CORE_AVAILABLE
class LogosAPIClient;
#endif

/**
 * CalendarStore — wraps KV operations for calendar/event persistence.
 *
 * Under Logos Core, delegates to kv_module via inter-module QtRO calls.
 * In standalone/test mode, uses an in-memory map fallback.
 *
 * Key patterns (namespace "scala"):
 *   calendar:{id}          → Calendar JSON
 *   event:{calendarId}:{id} → CalendarEvent JSON
 *   calendars              → JSON array of calendar IDs
 *   events:{calendarId}    → JSON array of event IDs
 */
class CalendarStore {
public:
    CalendarStore();

#ifdef LOGOS_CORE_AVAILABLE
    void setClient(LogosAPIClient *client);
#endif

    // ── Calendar CRUD ────────────────────────────────────────────────────────
    QString saveCalendar(const scala::Calendar &cal);
    scala::Calendar getCalendar(const QString &id) const;
    QList<scala::Calendar> listCalendars() const;
    bool deleteCalendar(const QString &id);

    // ── Event CRUD ───────────────────────────────────────────────────────────
    QString saveEvent(const scala::CalendarEvent &ev);
    scala::CalendarEvent getEvent(const QString &id) const;
    QList<scala::CalendarEvent> listEvents(const QString &calendarId) const;
    bool updateEvent(const scala::CalendarEvent &ev);
    bool deleteEvent(const QString &id);

private:
    static constexpr const char *KV_NS = "scala";

    // KV helpers
    void kvSet(const QString &key, const QString &value) const;
    QString kvGet(const QString &key) const;
    void kvRemove(const QString &key) const;

    // Index helpers
    QStringList getIndex(const QString &indexKey) const;
    void setIndex(const QString &indexKey, const QStringList &ids) const;
    void addToIndex(const QString &indexKey, const QString &id) const;
    void removeFromIndex(const QString &indexKey, const QString &id) const;

    // Find which calendar owns an event (scans index keys)
    QString findCalendarIdForEvent(const QString &eventId) const;

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPIClient *m_kvClient = nullptr;
#else
    // In-memory fallback for standalone builds
    mutable QMap<QString, QString> m_mem;
#endif
};
