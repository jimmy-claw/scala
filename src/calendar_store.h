#pragma once

#include "types.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

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
 *
 * Encryption:
 *   When enableEncryption(32-byte key) is called, the store:
 *     - Under Logos Core: calls kv_module::setEncryptionKey(ns, hexKey)
 *       so the module handles AES-256-GCM transparently.
 *     - Standalone/test: encrypts values in-process using XOR+SHA256-derived
 *       stream cipher (deterministic, test-only — NOT for production).
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

    // KV helpers (public for identity storage and encryption salt)
    void kvSet(const QString &key, const QString &value) const;
    QString kvGet(const QString &key) const;
    void kvRemove(const QString &key) const;

    // ── Encryption ───────────────────────────────────────────────────────────
    /**
     * enableEncryption — activate AES-256 at-rest encryption for KV data.
     *
     * @param keyBytes  32-byte raw key (caller derives via PBKDF2 or similar).
     *
     * Under Logos Core, delegates to kv_module::setEncryptionKey().
     * In standalone mode, applies an in-process XOR stream cipher (tests only).
     */
    void enableEncryption(const QByteArray &keyBytes);

    /**
     * disableEncryption — remove the active key; future writes are plaintext.
     * Existing encrypted data becomes unreadable until key is re-set.
     */
    void disableEncryption();

    /** Returns true if an encryption key is currently active. */
    bool isEncryptionEnabled() const;

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

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPIClient *m_kvClient = nullptr;
#else
    // In-memory fallback for standalone builds
    mutable QMap<QString, QString> m_mem;

    // Standalone encryption (XOR stream cipher for testing)
    QByteArray m_encKey;
    QByteArray standaloneEncrypt(const QByteArray &key, const QString &plaintext) const;
    QString standaloneDecrypt(const QByteArray &key, const QByteArray &ciphertext) const;
#endif
};
