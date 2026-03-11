#include "scala_bridge.h"

#include <logos_api_client.h>
#include <token_manager.h>

#include <QFile>
#include <QDebug>

static const QString MODULE_NAME   = QStringLiteral("scala_module");
static const QString ORIGIN_NAME   = QStringLiteral("scala_ui");
static const QString OBJECT_NAME   = QStringLiteral("scala");
static const QString TOKEN_FILE    = QStringLiteral("/tmp/logos_scala_module");

ScalaBridge::ScalaBridge(QObject *parent)
    : QObject(parent)
{
    loadToken();

    m_client = new LogosAPIClient(MODULE_NAME, ORIGIN_NAME,
                                  &TokenManager::instance(), this);

    if (!m_client->isConnected()) {
        qWarning() << "ScalaBridge: not connected to" << MODULE_NAME
                    << "— will retry on first call";
    }
}

ScalaBridge::~ScalaBridge() = default;

// ── Token bootstrap ────────────────────────────────────────────────────────

void ScalaBridge::loadToken()
{
    QFile f(TOKEN_FILE);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "ScalaBridge: no token file at" << TOKEN_FILE;
        return;
    }
    QString token = QString::fromUtf8(f.readAll()).trimmed();
    f.close();

    if (!token.isEmpty()) {
        TokenManager::instance().saveToken(MODULE_NAME, token);
        qDebug() << "ScalaBridge: loaded token for" << MODULE_NAME;
    }
}

// ── Connection ─────────────────────────────────────────────────────────────

bool ScalaBridge::isConnected() const
{
    return m_client && m_client->isConnected();
}

// ── Private helper ─────────────────────────────────────────────────────────

QVariant ScalaBridge::call(const QString &method, const QVariantList &args)
{
    if (!m_client) return {};

    switch (args.size()) {
    case 0:
        return m_client->invokeRemoteMethod(OBJECT_NAME, method);
    case 1:
        return m_client->invokeRemoteMethod(OBJECT_NAME, method, args[0]);
    case 2:
        return m_client->invokeRemoteMethod(OBJECT_NAME, method, args[0], args[1]);
    case 3:
        return m_client->invokeRemoteMethod(OBJECT_NAME, method,
                                            args[0], args[1], args[2]);
    default:
        qWarning() << "ScalaBridge::call: too many args for" << method;
        return {};
    }
}

// ── Calendar CRUD ──────────────────────────────────────────────────────────

QString ScalaBridge::listCalendars()
{
    return call(QStringLiteral("listCalendars")).toString();
}

QString ScalaBridge::createCalendar(const QString &name, const QString &color)
{
    return call(QStringLiteral("createCalendar"), {name, color}).toString();
}

bool ScalaBridge::deleteCalendar(const QString &id)
{
    return call(QStringLiteral("deleteCalendar"), {id}).toBool();
}

// ── Event CRUD ─────────────────────────────────────────────────────────────

QString ScalaBridge::listEvents(const QString &calendarId)
{
    return call(QStringLiteral("listEvents"), {calendarId}).toString();
}

QString ScalaBridge::createEvent(const QString &calendarId, const QString &eventJson)
{
    return call(QStringLiteral("createEvent"), {calendarId, eventJson}).toString();
}

QString ScalaBridge::updateEvent(const QString &eventJson)
{
    return call(QStringLiteral("updateEvent"), {eventJson}).toString();
}

QString ScalaBridge::deleteEvent(const QString &id)
{
    return call(QStringLiteral("deleteEvent"), {id}).toString();
}

QString ScalaBridge::getEvent(const QString &id)
{
    return call(QStringLiteral("getEvent"), {id}).toString();
}

// ── Identity ───────────────────────────────────────────────────────────────

QString ScalaBridge::getIdentity()
{
    QString id = call(QStringLiteral("getIdentity")).toString();
    if (!id.isEmpty() && id != m_cachedIdentity) {
        m_cachedIdentity = id;
        emit identityChanged();
    }
    return id.isEmpty() ? m_cachedIdentity : id;
}

void ScalaBridge::setIdentity(const QString &pubkeyHex)
{
    call(QStringLiteral("setIdentity"), {pubkeyHex});
    m_cachedIdentity = pubkeyHex;
    emit identityChanged();
}

// ── Share link ─────────────────────────────────────────────────────────────

QString ScalaBridge::generateShareLink(const QString &calendarId)
{
    return call(QStringLiteral("generateShareLink"), {calendarId}).toString();
}

bool ScalaBridge::handleShareLink(const QString &link)
{
    return call(QStringLiteral("handleShareLink"), {link}).toBool();
}

// ── Search & reminders ─────────────────────────────────────────────────────

QString ScalaBridge::searchEvents(const QString &query)
{
    return call(QStringLiteral("searchEvents"), {query}).toString();
}

QString ScalaBridge::getPendingReminders()
{
    return call(QStringLiteral("getPendingReminders")).toString();
}

// ── Settings ───────────────────────────────────────────────────────────────

void ScalaBridge::setSetting(const QString &key, const QString &value)
{
    call(QStringLiteral("setSetting"), {key, value});
}

QString ScalaBridge::getSetting(const QString &key, const QString &defaultValue)
{
    QString result = call(QStringLiteral("getSetting"), {key, defaultValue}).toString();
    return result.isEmpty() ? defaultValue : result;
}
