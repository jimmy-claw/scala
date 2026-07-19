#include "scala_ui_backend.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — typed wrappers + typed event accessors.
#include "logos_sdk.h"

// ── Identity ────────────────────────────────────────────────────────────────

QString ScalaUiBackend::getIdentity()
{
    return modules().scala.getIdentity();
}

void ScalaUiBackend::setIdentity(QString pubkeyHex)
{
    modules().scala.setIdentity(pubkeyHex);
    updateIdentityProp();
}

// ── Calendar CRUD ───────────────────────────────────────────────────────────

QString ScalaUiBackend::createCalendar(QString name, QString color)
{
    return modules().scala.createCalendar(name, color);
}

QString ScalaUiBackend::listCalendars()
{
    return modules().scala.listCalendars();
}

bool ScalaUiBackend::deleteCalendar(QString id)
{
    return modules().scala.deleteCalendar(id);
}

// ── Event CRUD ──────────────────────────────────────────────────────────────

QString ScalaUiBackend::createEvent(QString calendarId, QString eventJson)
{
    return modules().scala.createEvent(calendarId, eventJson);
}

QString ScalaUiBackend::updateEvent(QString eventJson)
{
    return modules().scala.updateEvent(eventJson);
}

bool ScalaUiBackend::deleteEvent(QString id)
{
    return modules().scala.deleteEvent(id);
}

QString ScalaUiBackend::listEvents(QString calendarId)
{
    return modules().scala.listEvents(calendarId);
}

QString ScalaUiBackend::getEvent(QString id)
{
    return modules().scala.getEvent(id);
}

// ── Sync / Sharing ──────────────────────────────────────────────────────────

QString ScalaUiBackend::shareCalendar(QString calendarId)
{
    return modules().scala.shareCalendar(calendarId);
}

bool ScalaUiBackend::joinSharedCalendar(QString calendarId, QString encryptionKey)
{
    return modules().scala.joinSharedCalendar(calendarId, encryptionKey);
}

QString ScalaUiBackend::getSyncStatus(QString calendarId)
{
    return modules().scala.getSyncStatus(calendarId);
}

// ── Share Links ─────────────────────────────────────────────────────────────

QString ScalaUiBackend::generateShareLink(QString calendarId)
{
    return modules().scala.generateShareLink(calendarId);
}

QString ScalaUiBackend::parseShareLink(QString link)
{
    return modules().scala.parseShareLink(link);
}

bool ScalaUiBackend::handleShareLink(QString link)
{
    return modules().scala.handleShareLink(link);
}

// ── Search ──────────────────────────────────────────────────────────────────

QString ScalaUiBackend::searchEvents(QString query)
{
    return modules().scala.searchEvents(query);
}

// ── Reminders ───────────────────────────────────────────────────────────────

QString ScalaUiBackend::getPendingReminders()
{
    return modules().scala.getPendingReminders();
}

// ── Settings ────────────────────────────────────────────────────────────────

void ScalaUiBackend::setSetting(QString key, QString value)
{
    modules().scala.setSetting(key, value);
}

QString ScalaUiBackend::getSetting(QString key, QString defaultValue)
{
    return modules().scala.getSetting(key, defaultValue);
}

// ── Context lifecycle ───────────────────────────────────────────────────────

void ScalaUiBackend::onContextReady()
{
    // Typed module-event subscription. `identityChanged` is scala's event;
    // the generated wrapper exposes it as onIdentityChanged(std::function<void()>).
    // No params passed through — fetch current identity inside callback.
    modules().scala.onIdentityChanged([this]() {
        QString newIdentity = getIdentity();
        setPendingReminderCount(0);  // reset on identity change
        emit identityChanged(newIdentity);
        updateIdentityProp();
    });

    // Subscribe to sync status changes (if scala exposes this event)
    // TODO: Wire up when scala_impl declares the event in logos_events:
    // modules().scala.onSyncStatusChanged([this](const QString& calId, const QString& status) {
    //     setPendingReminderCount(...);
    //     emit syncStatusChanged(calId, status);
    // });

    // Initialize identity PROP
    updateIdentityProp();
}

// ── Helpers ─────────────────────────────────────────────────────────────────

void ScalaUiBackend::updateIdentityProp()
{
    QString identity = modules().scala.getIdentity();
    setCurrentIdentity(identity);
}
