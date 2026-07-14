#include "scala_ui_backend.h"

// Generated umbrella: LogosModules (behind modules()) from
// metadata.json#dependencies — typed wrappers + typed event accessors.
#include "logos_sdk.h"

// ── Identity ────────────────────────────────────────────────────────────────

QString ScalaUiBackend::getIdentity()
{
    return QString::fromStdString(modules().scala.getIdentity());
}

void ScalaUiBackend::setIdentity(QString pubkeyHex)
{
    modules().scala.setIdentity(pubkeyHex.toStdString());
    updateIdentityProp();
}

// ── Calendar CRUD ───────────────────────────────────────────────────────────

QString ScalaUiBackend::createCalendar(QString name, QString color)
{
    return QString::fromStdString(
        modules().scala.createCalendar(name.toStdString(), color.toStdString()));
}

QString ScalaUiBackend::listCalendars()
{
    return QString::fromStdString(modules().scala.listCalendars());
}

bool ScalaUiBackend::deleteCalendar(QString id)
{
    return modules().scala.deleteCalendar(id.toStdString());
}

// ── Event CRUD ──────────────────────────────────────────────────────────────

QString ScalaUiBackend::createEvent(QString calendarId, QString eventJson)
{
    return QString::fromStdString(
        modules().scala.createEvent(calendarId.toStdString(), eventJson.toStdString()));
}

QString ScalaUiBackend::updateEvent(QString eventJson)
{
    return QString::fromStdString(modules().scala.updateEvent(eventJson.toStdString()));
}

bool ScalaUiBackend::deleteEvent(QString id)
{
    return modules().scala.deleteEvent(id.toStdString());
}

QString ScalaUiBackend::listEvents(QString calendarId)
{
    return QString::fromStdString(
        modules().scala.listEvents(calendarId.toStdString()));
}

QString ScalaUiBackend::getEvent(QString id)
{
    return QString::fromStdString(modules().scala.getEvent(id.toStdString()));
}

// ── Sync / Sharing ──────────────────────────────────────────────────────────

QString ScalaUiBackend::shareCalendar(QString calendarId)
{
    return QString::fromStdString(
        modules().scala.shareCalendar(calendarId.toStdString()));
}

bool ScalaUiBackend::joinSharedCalendar(QString calendarId, QString encryptionKey)
{
    return modules().scala.joinSharedCalendar(
        calendarId.toStdString(), encryptionKey.toStdString());
}

QString ScalaUiBackend::getSyncStatus(QString calendarId)
{
    return QString::fromStdString(
        modules().scala.getSyncStatus(calendarId.toStdString()));
}

// ── Share Links ─────────────────────────────────────────────────────────────

QString ScalaUiBackend::generateShareLink(QString calendarId)
{
    return QString::fromStdString(
        modules().scala.generateShareLink(calendarId.toStdString()));
}

QString ScalaUiBackend::parseShareLink(QString link)
{
    return QString::fromStdString(modules().scala.parseShareLink(link.toStdString()));
}

bool ScalaUiBackend::handleShareLink(QString link)
{
    return modules().scala.handleShareLink(link.toStdString());
}

// ── Search ──────────────────────────────────────────────────────────────────

QString ScalaUiBackend::searchEvents(QString query)
{
    return QString::fromStdString(modules().scala.searchEvents(query.toStdString()));
}

// ── Reminders ───────────────────────────────────────────────────────────────

QString ScalaUiBackend::getPendingReminders()
{
    return QString::fromStdString(modules().scala.getPendingReminders());
}

// ── Settings ────────────────────────────────────────────────────────────────

void ScalaUiBackend::setSetting(QString key, QString value)
{
    modules().scala.setSetting(key.toStdString(), value.toStdString());
}

QString ScalaUiBackend::getSetting(QString key, QString defaultValue)
{
    return QString::fromStdString(
        modules().scala.getSetting(key.toStdString(), defaultValue.toStdString()));
}

// ── Context lifecycle ───────────────────────────────────────────────────────

void ScalaUiBackend::onContextReady()
{
    // Typed module-event subscription. `identityChanged` is scala's event;
    // the generated wrapper exposes it as onIdentityChanged + a Qt-typed callback.
    modules().scala.onIdentityChanged([this](const QString& newIdentity) {
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
    QString identity = QString::fromStdString(modules().scala.getIdentity());
    setCurrentIdentity(identity);
}
