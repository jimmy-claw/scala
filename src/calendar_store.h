#pragma once

#include "types.h"
#include "local_storage.h"

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
 * ALWAYS uses LocalStorage (file-based JSON) for persistence.
 * When running under Logos Core with kv_module available, additionally
 * syncs to kv_module for cross-module access.
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
    ~CalendarStore();

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

    // KV helpers (public for identity storage)
    void kvSet(const QString &key, const QString &value) const;
    QString kvGet(const QString &key) const;
    void kvRemove(const QString &key) const;

    // Namespace support for multi-instance testing
    void setNamespace(const QString &ns);
    QString namespacedKey(const QString &key) const;

private:
    static constexpr const char *KV_NS = "scala";
    QString m_namespace = QStringLiteral("default");

    // File-based persistence (ALWAYS available)
    LocalStorage *m_storage = nullptr;

#ifdef LOGOS_CORE_AVAILABLE
    // Optional: kv_module client for cross-module access
    LogosAPIClient *m_kvClient = nullptr;
#endif

    // Index helpers
    QStringList getIndex(const QString &indexKey) const;
    void setIndex(const QString &indexKey, const QStringList &ids) const;
    void addToIndex(const QString &indexKey, const QString &id) const;
    void removeFromIndex(const QString &indexKey, const QString &id) const;

    // Find which calendar owns an event (scans index keys)
    QString findCalendarIdForEvent(const QString &eventId) const;
};
