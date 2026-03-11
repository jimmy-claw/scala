#include "scala_plugin.h"

#include <QDebug>
#include <QTimer>

// ── Construction ─────────────────────────────────────────────────────────────

ScalaPlugin::ScalaPlugin(QObject *parent)
    : QObject(parent)
    , m_calendar(new LogosCalendar(this))
{
    // Forward eventResponse from the inner calendar to the plugin signal
    connect(m_calendar, &LogosCalendar::eventResponse,
            this, &ScalaPlugin::eventResponse);
}

// ── Logos Core lifecycle ─────────────────────────────────────────────────────

void ScalaPlugin::initLogos(LogosAPI *api) {
    m_logosAPI = api;

    if (!m_logosAPI) {
        qWarning() << "ScalaPlugin: initLogos called with null LogosAPI";
        qInfo() << "ScalaPlugin: initialized (headless). version:" << version();
        return;
    }

    // Defer getClient calls to avoid race with other modules registering
    QTimer::singleShot(200, this, [this]() {
        m_calendar->initLogos(m_logosAPI);
        qInfo() << "ScalaPlugin: initialized. version:" << version();
    });
}

// ── Forwarded methods ────────────────────────────────────────────────────────

void ScalaPlugin::setNamespace(const QString &ns) {
    m_calendar->setNamespace(ns);
}

QString ScalaPlugin::getIdentity() const {
    return m_calendar->getIdentity();
}

void ScalaPlugin::setIdentity(const QString &pubkeyHex) {
    m_calendar->setIdentity(pubkeyHex);
}

QString ScalaPlugin::createCalendar(const QString &name, const QString &color) {
    return m_calendar->createCalendar(name, color);
}

QString ScalaPlugin::listCalendars() {
    return m_calendar->listCalendars();
}

bool ScalaPlugin::deleteCalendar(const QString &id) {
    return m_calendar->deleteCalendar(id);
}

QString ScalaPlugin::createEvent(const QString &calendarId, const QString &eventJson) {
    return m_calendar->createEvent(calendarId, eventJson);
}

QString ScalaPlugin::updateEvent(const QString &eventJson) {
    return m_calendar->updateEvent(eventJson);
}

QString ScalaPlugin::deleteEvent(const QString &id) {
    return m_calendar->deleteEvent(id);
}

QString ScalaPlugin::listEvents(const QString &calendarId) {
    return m_calendar->listEvents(calendarId);
}

QString ScalaPlugin::getEvent(const QString &id) {
    return m_calendar->getEvent(id);
}

QString ScalaPlugin::shareCalendar(const QString &calendarId) {
    return m_calendar->shareCalendar(calendarId);
}

bool ScalaPlugin::joinSharedCalendar(const QString &calendarId,
                                      const QString &encryptionKey) {
    return m_calendar->joinSharedCalendar(calendarId, encryptionKey);
}

QString ScalaPlugin::getSyncStatus(const QString &calendarId) {
    return m_calendar->getSyncStatus(calendarId);
}

QString ScalaPlugin::generateShareLink(const QString &calendarId) {
    return m_calendar->generateShareLink(calendarId);
}

QString ScalaPlugin::parseShareLink(const QString &link) {
    return m_calendar->parseShareLink(link);
}

bool ScalaPlugin::handleShareLink(const QString &link) {
    return m_calendar->handleShareLink(link);
}

QString ScalaPlugin::searchEvents(const QString &query) {
    return m_calendar->searchEvents(query);
}

QString ScalaPlugin::getPendingReminders() {
    return m_calendar->getPendingReminders();
}

void ScalaPlugin::setSetting(const QString &key, const QString &value) {
    m_calendar->setSetting(key, value);
}

QString ScalaPlugin::getSetting(const QString &key, const QString &defaultValue) {
    return m_calendar->getSetting(key, defaultValue);
}
