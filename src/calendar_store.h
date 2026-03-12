#pragma once

#include "types.h"
#include "i_kv_module.h"

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
 * Supports THREE modes:
 * 1. Logos Core via logos_host: uses LogosAPIClient to call kv_module via QtRO (requires logos_host running)
 * 2. Direct kv_module plugin: loads kv_module_plugin.so directly (no logos_host needed)
 * 3. In-memory fallback: for standalone/test mode (no persistence)
 *
 * Priority: mode 1 > mode 2 > mode 3
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
    // Mode 1: through logos_host (current behavior, requires logos_host running)
    void setClient(LogosAPIClient *client);
#endif

    // Mode 2: direct kv_module plugin (NEW — no logos_host needed!)
    void setKvModule(IKvModule *kv);

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

    // Index helpers
    QStringList getIndex(const QString &indexKey) const;
    void setIndex(const QString &indexKey, const QStringList &ids) const;
    void addToIndex(const QString &indexKey, const QString &id) const;
    void removeFromIndex(const QString &indexKey, const QString &id) const;

    // Find which calendar owns an event (scans index keys)
    QString findCalendarIdForEvent(const QString &eventId) const;

    // KV clients (priority order: logos_host > direct kv_module > memory)
#ifdef LOGOS_CORE_AVAILABLE
    LogosAPIClient *m_kvClient = nullptr;
#endif
    IKvModule *m_kvModule = nullptr;
    mutable QMap<QString, QString> m_mem;
};
