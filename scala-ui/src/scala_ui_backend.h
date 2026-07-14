#pragma once

#include "rep_scala_ui_backend_source.h"
#include "logos_ui_plugin_context.h"

/**
 * ScalaUiBackend — C++ backend for the Scala Calendar QML UI.
 *
 * Derives:
 *   - ScalaUiBackendSimpleSource — generated from scala_ui_backend.rep;
 *     override its slots (QML replica gets each return value via QtRO).
 *   - LogosUiPluginContext — supplies modules() (Qt-typed callers + typed event
 *     subscriptions for "dependencies") and onContextReady().
 *
 * The *Plugin / *Interface classes (Q_PLUGIN_METADATA, initLogos wiring,
 * QtRO registration) are generated around it.
 */
class ScalaUiBackend : public ScalaUiBackendSimpleSource,
                       public LogosUiPluginContext
{
public:
    // ── Identity slots ─────────────────────────────────────────────────────
    QString getIdentity() override;
    void setIdentity(QString pubkeyHex) override;

    // ── Calendar CRUD slots ────────────────────────────────────────────────
    QString createCalendar(QString name, QString color) override;
    QString listCalendars() override;
    bool deleteCalendar(QString id) override;

    // ── Event CRUD slots ───────────────────────────────────────────────────
    QString createEvent(QString calendarId, QString eventJson) override;
    QString updateEvent(QString eventJson) override;
    bool deleteEvent(QString id) override;
    QString listEvents(QString calendarId) override;
    QString getEvent(QString id) override;

    // ── Sync / Sharing slots ───────────────────────────────────────────────
    QString shareCalendar(QString calendarId) override;
    bool joinSharedCalendar(QString calendarId, QString encryptionKey) override;
    QString getSyncStatus(QString calendarId) override;

    // ── Share Link slots ───────────────────────────────────────────────────
    QString generateShareLink(QString calendarId) override;
    QString parseShareLink(QString link) override;
    bool handleShareLink(QString link) override;

    // ── Search slot ────────────────────────────────────────────────────────
    QString searchEvents(QString query) override;

    // ── Reminders slot ─────────────────────────────────────────────────────
    QString getPendingReminders() override;

    // ── Settings slots ─────────────────────────────────────────────────────
    void setSetting(QString key, QString value) override;
    QString getSetting(QString key, QString defaultValue) override;

    // ── Context lifecycle ──────────────────────────────────────────────────
    void onContextReady() override;

private:
    // Helper: update the currentIdentity PROP after identity changes
    void updateIdentityProp();
};
