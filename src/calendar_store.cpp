#include "calendar_store.h"

#ifdef LOGOS_CORE_AVAILABLE
#include <logos_api_client.h>
#endif

#include <QDebug>
#include <QJsonDocument>

// ── Construction ─────────────────────────────────────────────────────────────

CalendarStore::CalendarStore() : m_kvModule(nullptr) {
    // Default to in-memory mode if no kv module is set
#ifdef LOGOS_CORE_AVAILABLE
    m_kvClient = nullptr;
#endif
    m_mem.clear();
    qInfo() << "CalendarStore initialized:"
            << "(kvModule=" << (m_kvModule ? "set" : "nullptr")
            << ", kvClient=" << (m_kvClient ? "set" : "nullptr")
            << ")";
}

#ifdef LOGOS_CORE_AVAILABLE
void CalendarStore::setClient(LogosAPIClient *client) {
    m_kvClient = client;
}
#endif

void CalendarStore::setKvModule(IKvModule *kv) {
    m_kvModule = kv;
}

// ── Namespace ────────────────────────────────────────────────────────────────

void CalendarStore::setNamespace(const QString &ns) {
    m_namespace = ns.isEmpty() ? QStringLiteral("default") : ns;
}

QString CalendarStore::namespacedKey(const QString &key) const {
    return QStringLiteral("scala:") + m_namespace + QStringLiteral(":") + key;
}

// ── KV helpers ───────────────────────────────────────────────────────────────
// Priority order: logos_host (QtRO) > direct kv_module (plugin) > in-memory

void CalendarStore::kvSet(const QString &key, const QString &value) const {
    const QString nsKey = namespacedKey(key);
    qInfo() << "CalendarStore::kvSet" << key << "->" << nsKey
            << "(kvModule=" << (m_kvModule ? "set" : "nullptr")
            << ", kvClient=" << (m_kvClient ? "set" : "nullptr") << ")";
    
    // Priority 2: direct kv_module plugin (no logos_host needed!)
    if (m_kvModule) {
        m_kvModule->set(KV_NS, nsKey, value);
        qInfo() << "  -> using kv_module (direct)";
        return;
    }
    
    // Fallback: logos_host (QtRO) - for backwards compatibility
#ifdef LOGOS_CORE_AVAILABLE
    if (m_kvClient) {
        m_kvClient->invokeRemoteMethod("kv_module", "set",
                                       QString(KV_NS), nsKey, value);
        return;
    }
#endif
        m_kvModule->set(KV_NS, nsKey, value);
        return;
    }
    
    // Priority 3: in-memory fallback
    qInfo() << "  -> using in-memory fallback";
    m_mem[nsKey] = value;
}

QString CalendarStore::kvGet(const QString &key) const {
    const QString nsKey = namespacedKey(key);
    qInfo() << "CalendarStore::kvGet" << key << "->" << nsKey
            << "(kvModule=" << (m_kvModule ? "get" : "nullptr")
            << ", kvClient=" << (m_kvClient ? "get" : "nullptr") << ")";
    
    // Priority 2: direct kv_module plugin (no logos_host needed!)
    if (m_kvModule) {
        QString result = m_kvModule->get(KV_NS, nsKey);
        qInfo() << "  -> using kv_module (direct), result=" << (result.isEmpty() ? "EMPTY" : "OK");
        return result;
    }
    
    // Fallback: logos_host (QtRO) - for backwards compatibility
#ifdef LOGOS_CORE_AVAILABLE
    if (m_kvClient) {
        QVariant result = m_kvClient->invokeRemoteMethod("kv_module", "get",
                                                         QString(KV_NS), nsKey);
        if (result.isValid()) {
            return result.toString();
        }
    }
#endif
    
    // Priority 3: in-memory fallback
    return m_mem.value(nsKey);
}

void CalendarStore::kvRemove(const QString &key) const {
    const QString nsKey = namespacedKey(key);
    qInfo() << "CalendarStore::kvRemove" << key << "->" << nsKey
            << "(kvModule=" << (m_kvModule ? "remove" : "nullptr")
            << ", kvClient=" << (m_kvClient ? "remove" : "nullptr") << ")";
    
    // Priority 2: direct kv_module plugin (no logos_host needed!)
    if (m_kvModule) {
        m_kvModule->remove(KV_NS, nsKey);
        qInfo() << "  -> using kv_module (direct)";
        return;
    }
    
    // Fallback: logos_host (QtRO) - for backwards compatibility
#ifdef LOGOS_CORE_AVAILABLE
    if (m_kvClient) {
        m_kvClient->invokeRemoteMethod("kv_module", "remove",
                                       QString(KV_NS), nsKey);
        return;
    }
#endif
    
    // Priority 3: in-memory fallback
    m_mem.remove(nsKey);
}

// ── Index helpers ────────────────────────────────────────────────────────────

QStringList CalendarStore::getIndex(const QString &indexKey) const {
    QString raw = kvGet(indexKey);
    if (raw.isEmpty())
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    QStringList ids;
    for (const auto &v : doc.array())
        ids.append(v.toString());
    return ids;
}

void CalendarStore::setIndex(const QString &indexKey, const QStringList &ids) const {
    QJsonArray arr;
    for (const auto &id : ids)
        arr.append(id);
    kvSet(indexKey, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void CalendarStore::addToIndex(const QString &indexKey, const QString &id) const {
    QStringList ids = getIndex(indexKey);
    if (!ids.contains(id)) {
        ids.append(id);
        setIndex(indexKey, ids);
    }
}

void CalendarStore::removeFromIndex(const QString &indexKey, const QString &id) const {
    QStringList ids = getIndex(indexKey);
    ids.removeAll(id);
    setIndex(indexKey, ids);
}

// ── Calendar CRUD ────────────────────────────────────────────────────────────

QString CalendarStore::saveCalendar(const scala::Calendar &cal) {
    QString key = QStringLiteral("calendar:") + cal.id;
    QString json = QString::fromUtf8(
        QJsonDocument(cal.toJson()).toJson(QJsonDocument::Compact));
    kvSet(key, json);
    addToIndex(QStringLiteral("calendars"), cal.id);
    return cal.id;
}

scala::Calendar CalendarStore::getCalendar(const QString &id) const {
    QString key = QStringLiteral("calendar:") + id;
    QString raw = kvGet(key);
    if (raw.isEmpty())
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    return scala::Calendar::fromJson(doc.object());
}

QList<scala::Calendar> CalendarStore::listCalendars() const {
    QStringList ids = getIndex(QStringLiteral("calendars"));
    QList<scala::Calendar> result;
    for (const auto &id : ids) {
        auto cal = getCalendar(id);
        if (!cal.id.isEmpty())
            result.append(cal);
    }
    return result;
}

bool CalendarStore::deleteCalendar(const QString &id) {
    QString key = QStringLiteral("calendar:") + id;
    QString raw = kvGet(key);
    if (raw.isEmpty())
        return false;

    // Delete all events in this calendar
    QStringList eventIds = getIndex(QStringLiteral("events:") + id);
    for (const auto &eid : eventIds)
        kvRemove(QStringLiteral("event:") + id + QStringLiteral(":") + eid);
    kvRemove(QStringLiteral("events:") + id);

    kvRemove(key);
    removeFromIndex(QStringLiteral("calendars"), id);
    return true;
}

// ── Event CRUD ───────────────────────────────────────────────────────────────

QString CalendarStore::saveEvent(const scala::CalendarEvent &ev) {
    QString key = QStringLiteral("event:") + ev.calendarId +
                  QStringLiteral(":") + ev.id;
    QString json = QString::fromUtf8(
        QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
    kvSet(key, json);
    addToIndex(QStringLiteral("events:") + ev.calendarId, ev.id);
    return ev.id;
}

scala::CalendarEvent CalendarStore::getEvent(const QString &id) const {
    // We need to find which calendar this event belongs to
    QString calendarId = findCalendarIdForEvent(id);
    if (calendarId.isEmpty())
        return {};

    QString key = QStringLiteral("event:") + calendarId +
                  QStringLiteral(":") + id;
    QString raw = kvGet(key);
    if (raw.isEmpty())
        return {};

    QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    return scala::CalendarEvent::fromJson(doc.object());
}

QList<scala::CalendarEvent> CalendarStore::listEvents(const QString &calendarId) const {
    QStringList ids = getIndex(QStringLiteral("events:") + calendarId);
    QList<scala::CalendarEvent> result;
    for (const auto &id : ids) {
        QString key = QStringLiteral("event:") + calendarId +
                      QStringLiteral(":") + id;
        QString raw = kvGet(key);
        if (raw.isEmpty())
            continue;
        QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
        result.append(scala::CalendarEvent::fromJson(doc.object()));
    }
    return result;
}

bool CalendarStore::updateEvent(const scala::CalendarEvent &ev) {
    QString key = QStringLiteral("event:") + ev.calendarId +
                  QStringLiteral(":") + ev.id;
    QString existing = kvGet(key);
    if (existing.isEmpty())
        return false;

    QString json = QString::fromUtf8(
        QJsonDocument(ev.toJson()).toJson(QJsonDocument::Compact));
    kvSet(key, json);
    return true;
}

bool CalendarStore::deleteEvent(const QString &id) {
    QString calendarId = findCalendarIdForEvent(id);
    if (calendarId.isEmpty())
        return false;

    QString key = QStringLiteral("event:") + calendarId +
                  QStringLiteral(":") + id;
    kvRemove(key);
    removeFromIndex(QStringLiteral("events:") + calendarId, id);
    return true;
}

QString CalendarStore::findCalendarIdForEvent(const QString &eventId) const {
    QStringList calIds = getIndex(QStringLiteral("calendars"));
    for (const auto &calId : calIds) {
        QStringList eventIds = getIndex(QStringLiteral("events:") + calId);
        if (eventIds.contains(eventId))
            return calId;
    }
    return {};
}
