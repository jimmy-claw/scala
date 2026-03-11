#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "calendar_module.h"

#include <interface.h>
#include <logos_api.h>

/**
 * ScalaPlugin — Headless logoscore plugin wrapper for LogosCalendar.
 *
 * This plugin is loaded by logos_host as a shared library. It must NOT use
 * Qt Quick, QML engine, or any GUI classes — only Qt Core/Qml/RemoteObjects.
 * The QML UI runs as a separate process that connects via QtRO.
 */
class ScalaPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.example.PluginInterface" FILE "metadata.json")
    Q_INTERFACES(PluginInterface)

public:
    explicit ScalaPlugin(QObject *parent = nullptr);

    // ── PluginInterface ─────────────────────────────────────────────────────
    [[nodiscard]] QString name() const override { return QStringLiteral("scala_module"); }
    Q_INVOKABLE QString version() const override { return QStringLiteral("0.1.0"); }
    Q_INVOKABLE void initLogos(LogosAPI *api);

    // ── Namespace / Identity ────────────────────────────────────────────────
    Q_INVOKABLE void setNamespace(const QString &ns);
    Q_INVOKABLE QString getIdentity() const;
    Q_INVOKABLE void setIdentity(const QString &pubkeyHex);

    // ── Calendar CRUD ───────────────────────────────────────────────────────
    Q_INVOKABLE QString createCalendar(const QString &name, const QString &color);
    Q_INVOKABLE QString listCalendars();
    Q_INVOKABLE bool deleteCalendar(const QString &id);

    // ── Event CRUD ──────────────────────────────────────────────────────────
    Q_INVOKABLE QString createEvent(const QString &calendarId, const QString &eventJson);
    Q_INVOKABLE QString updateEvent(const QString &eventJson);
    Q_INVOKABLE QString deleteEvent(const QString &id);
    Q_INVOKABLE QString listEvents(const QString &calendarId);
    Q_INVOKABLE QString getEvent(const QString &id);

    // ── Sync API ────────────────────────────────────────────────────────────
    Q_INVOKABLE QString shareCalendar(const QString &calendarId);
    Q_INVOKABLE bool joinSharedCalendar(const QString &calendarId,
                                        const QString &encryptionKey);
    Q_INVOKABLE QString getSyncStatus(const QString &calendarId);

    // ── Share link API ──────────────────────────────────────────────────────
    Q_INVOKABLE QString generateShareLink(const QString &calendarId);
    Q_INVOKABLE QString parseShareLink(const QString &link);
    Q_INVOKABLE bool handleShareLink(const QString &link);

    // ── Search / Reminders / Settings ───────────────────────────────────────
    Q_INVOKABLE QString searchEvents(const QString &query);
    Q_INVOKABLE QString getPendingReminders();
    Q_INVOKABLE void setSetting(const QString &key, const QString &value);
    Q_INVOKABLE QString getSetting(const QString &key, const QString &defaultValue = QString());

signals:
    void eventResponse(const QString &eventName, const QVariantList &args);

private:
    LogosCalendar *m_calendar = nullptr;
    LogosAPI *m_logosAPI = nullptr;
};
