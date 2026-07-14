#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <logos_module_context.h>  // LogosModuleContext base + logos_events: + modules()

// Forward declarations for internal components (ported to std types)
class CalendarStore;
class CalendarSync;

/**
 * ScalaImpl — Universal-pattern core module for the Scala calendar app.
 *
 * Pure C++ class — no Qt, no Q_OBJECT, no Q_PLUGIN_METADATA.
 * All public methods are auto-exposed by logos-cpp-generator.
 * Inter-module calls via modules().kv_module.* (auto-generated typed SDK).
 * Events declared below with logos_events: section.
 */
class ScalaImpl : public LogosModuleContext {
public:
    ScalaImpl() = default;
    ~ScalaImpl() override = default;

    // ── Namespace API ────────────────────────────────────────────────────────
    void setNamespace(const std::string& ns);

    // ── Identity API ─────────────────────────────────────────────────────────
    std::string getIdentity() const;
    void setIdentity(const std::string& pubkeyHex);

    // ── Calendar CRUD ────────────────────────────────────────────────────────
    /// Create a new calendar. Returns the calendar ID.
    std::string createCalendar(const std::string& name, const std::string& color);

    /// List all calendars. Returns JSON array string.
    std::string listCalendars();

    /// Delete a calendar and all its events.
    bool deleteCalendar(const std::string& id);

    // ── Event CRUD ───────────────────────────────────────────────────────────
    /// Create an event in a calendar. Returns the event ID.
    std::string createEvent(const std::string& calendarId, const std::string& eventJson);

    /// Update an existing event. Returns the event ID.
    std::string updateEvent(const std::string& eventJson);

    /// Delete an event.
    bool deleteEvent(const std::string& id);

    /// List all events in a calendar. Returns JSON array string.
    std::string listEvents(const std::string& calendarId);

    /// Get a single event by ID. Returns JSON object string.
    std::string getEvent(const std::string& id);

    // ── Sync API ─────────────────────────────────────────────────────────────
    /// Share a calendar. Returns the encryption key.
    std::string shareCalendar(const std::string& calendarId);

    /// Join a shared calendar with the provided encryption key.
    bool joinSharedCalendar(const std::string& calendarId, const std::string& encryptionKey);

    /// Get the current sync status for a calendar.
    std::string getSyncStatus(const std::string& calendarId);

    // ── Share link API ───────────────────────────────────────────────────────
    /// Generate a scala:// share link for a calendar.
    std::string generateShareLink(const std::string& calendarId);

    /// Parse a scala:// share link. Returns JSON with calendar info.
    std::string parseShareLink(const std::string& link);

    /// Handle a scala:// share link (join the calendar).
    bool handleShareLink(const std::string& link);

    // ── Search API ───────────────────────────────────────────────────────────
    /// Search events across all calendars by title/description/location. Returns JSON array string.
    std::string searchEvents(const std::string& query);

    // ── Reminders API ────────────────────────────────────────────────────────
    /// Get events with pending reminders. Returns JSON array string.
    std::string getPendingReminders();

    // ── Settings API ─────────────────────────────────────────────────────────
    void setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultValue);

    // ── Context lifecycle ────────────────────────────────────────────────────
    /// Called when the module context is fully initialized (deps are live).
    void onContextReady() override;

    // ── Events — emitted to subscribers via the host's eventResponse channel ─
logos_events:
    /// Emitted when a calendar's sync status changes.
    void syncStatusChanged(const std::string& calendarId, const std::string& status);

    /// Emitted when the module identity changes.
    void identityChanged();

private:
    CalendarStore* m_store = nullptr;
    CalendarSync* m_sync = nullptr;
    std::string m_identity;
    std::string m_namespace;

    static std::string generateStableIdentity();

    // Handle incoming sync messages from CalendarSync
    void onSyncMessageReceived(const std::string& calendarId, const std::string& msgJson);
};
