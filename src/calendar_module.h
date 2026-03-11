#pragma once

#include "calendar_store.h"
#include "calendar_sync.h"
#include "types.h"

#include <QCryptographicHash>
#include <QObject>
#include <QString>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>

#ifdef LOGOS_CORE_AVAILABLE
#include <interface.h>
class LogosAPIClient;
#endif

/**
 * ILogosCalendar — Public interface for the Scala calendar module.
 */
class ILogosCalendar {
public:
    virtual ~ILogosCalendar() = default;

    virtual QString createCalendar(const QString &name, const QString &color) = 0;
    virtual QString listCalendars() = 0;
    virtual bool deleteCalendar(const QString &id) = 0;
    virtual QString createEvent(const QString &calendarId, const QString &eventJson) = 0;
    virtual QString updateEvent(const QString &eventJson) = 0;
    virtual QString deleteEvent(const QString &id) = 0;
    virtual QString listEvents(const QString &calendarId) = 0;
    virtual QString getEvent(const QString &id) = 0;

    // ── Sync API ───────────────────────────────────────────────────────────
    virtual QString shareCalendar(const QString &calendarId) = 0;
    virtual bool joinSharedCalendar(const QString &calendarId,
                                    const QString &encryptionKey) = 0;
    virtual QString getSyncStatus(const QString &calendarId) = 0;

    // ── Share link API ──────────────────────────────────────────────────────
    virtual QString generateShareLink(const QString &calendarId) = 0;
    virtual QString parseShareLink(const QString &link) = 0;
    virtual bool handleShareLink(const QString &link) = 0;
};

#define ILogosCalendar_iid "com.logos.module.ILogosCalendar"
Q_DECLARE_INTERFACE(ILogosCalendar, ILogosCalendar_iid)

/**
 * LogosCalendar — Qt plugin implementing the Scala calendar module.
 */
#if defined(LOGOS_CORE_AVAILABLE) && !defined(SCALA_MODULE_WRAPPER)
class LogosCalendar final : public QObject, public PluginInterface, public ILogosCalendar {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ILogosCalendar_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface ILogosCalendar)
#elif defined(LOGOS_CORE_AVAILABLE) && defined(SCALA_MODULE_WRAPPER)
class LogosCalendar final : public QObject, public PluginInterface, public ILogosCalendar {
    Q_OBJECT
    Q_INTERFACES(PluginInterface ILogosCalendar)
#else
class LogosCalendar final : public QObject, public ILogosCalendar {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ILogosCalendar_iid FILE "metadata.json")
    Q_INTERFACES(ILogosCalendar)
#endif

    Q_PROPERTY(QString identity READ getIdentity NOTIFY identityChanged)

public:
    explicit LogosCalendar(QObject *parent = nullptr);
    ~LogosCalendar() override = default;

    // ── PluginInterface ─────────────────────────────────────────────────────
#ifdef LOGOS_CORE_AVAILABLE
    [[nodiscard]] QString name() const override { return QStringLiteral("scala"); }
    Q_INVOKABLE QString version() const override { return QStringLiteral("0.1.0"); }
    Q_INVOKABLE void initLogos(LogosAPI *logosAPIInstance);
#else
    [[nodiscard]] QString name() const { return QStringLiteral("scala"); }
    Q_INVOKABLE QString version() const { return QStringLiteral("0.1.0"); }
#endif

    // ── Namespace API ─────────────────────────────────────────────────────
    Q_INVOKABLE void setNamespace(const QString &ns);

    // ── Identity API ────────────────────────────────────────────────────────
    Q_INVOKABLE QString getIdentity() const;
    Q_INVOKABLE void setIdentity(const QString &pubkeyHex);

    // ── ILogosCalendar ──────────────────────────────────────────────────────
    Q_INVOKABLE QString createCalendar(const QString &name, const QString &color) override;
    Q_INVOKABLE QString listCalendars() override;
    Q_INVOKABLE bool deleteCalendar(const QString &id) override;
    Q_INVOKABLE QString createEvent(const QString &calendarId, const QString &eventJson) override;
    Q_INVOKABLE QString updateEvent(const QString &eventJson) override;
    Q_INVOKABLE QString deleteEvent(const QString &id) override;
    Q_INVOKABLE QString listEvents(const QString &calendarId) override;
    Q_INVOKABLE QString getEvent(const QString &id) override;

    // ── Sync API ────────────────────────────────────────────────────────────
    Q_INVOKABLE QString shareCalendar(const QString &calendarId) override;
    Q_INVOKABLE bool joinSharedCalendar(const QString &calendarId,
                                        const QString &encryptionKey) override;
    Q_INVOKABLE QString getSyncStatus(const QString &calendarId) override;

    // ── Share link API ──────────────────────────────────────────────────────
    Q_INVOKABLE QString generateShareLink(const QString &calendarId) override;
    Q_INVOKABLE QString parseShareLink(const QString &link) override;
    Q_INVOKABLE bool handleShareLink(const QString &link) override;

    // ── Search API ────────────────────────────────────────────────────────
    Q_INVOKABLE QString searchEvents(const QString &query);

    // ── Reminders API ────────────────────────────────────────────────────
    Q_INVOKABLE QString getPendingReminders();

    // ── Settings API ─────────────────────────────────────────────────────
    Q_INVOKABLE void setSetting(const QString &key, const QString &value);
    Q_INVOKABLE QString getSetting(const QString &key, const QString &defaultValue = QString());

signals:
    void eventResponse(const QString &eventName, const QVariantList &args);
    void syncStatusChanged(const QString &calendarId, const QString &status);
    void identityChanged();

private:
    void onSyncMessageReceived(const QString &calendarId, const SyncMessage &msg);
    static QString generateStableIdentity();

    CalendarStore m_store;
    CalendarSync *m_sync = nullptr;
    QString m_identity;

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPI *m_logosAPI = nullptr;
    LogosAPIClient *m_kvClient = nullptr;
    LogosAPIClient *m_messagingClient = nullptr;
#endif
};
