#pragma once

#include "calendar_store.h"
#include "types.h"

#include <QObject>
#include <QString>

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
    virtual bool updateEvent(const QString &eventJson) = 0;
    virtual bool deleteEvent(const QString &id) = 0;
    virtual QString listEvents(const QString &calendarId) = 0;
    virtual QString getEvent(const QString &id) = 0;
};

#define ILogosCalendar_iid "com.logos.module.ILogosCalendar"
Q_DECLARE_INTERFACE(ILogosCalendar, ILogosCalendar_iid)

/**
 * LogosCalendar — Qt plugin implementing the Scala calendar module.
 */
#ifdef LOGOS_CORE_AVAILABLE
class LogosCalendar final : public QObject, public PluginInterface, public ILogosCalendar {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ILogosCalendar_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface ILogosCalendar)
#else
class LogosCalendar final : public QObject, public ILogosCalendar {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ILogosCalendar_iid FILE "metadata.json")
    Q_INTERFACES(ILogosCalendar)
#endif

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

    // ── ILogosCalendar ──────────────────────────────────────────────────────
    Q_INVOKABLE QString createCalendar(const QString &name, const QString &color) override;
    Q_INVOKABLE QString listCalendars() override;
    Q_INVOKABLE bool deleteCalendar(const QString &id) override;
    Q_INVOKABLE QString createEvent(const QString &calendarId, const QString &eventJson) override;
    Q_INVOKABLE bool updateEvent(const QString &eventJson) override;
    Q_INVOKABLE bool deleteEvent(const QString &id) override;
    Q_INVOKABLE QString listEvents(const QString &calendarId) override;
    Q_INVOKABLE QString getEvent(const QString &id) override;

signals:
    void eventResponse(const QString &eventName, const QVariantList &args);

private:
    CalendarStore m_store;

#ifdef LOGOS_CORE_AVAILABLE
    LogosAPI *m_logosAPI = nullptr;
    LogosAPIClient *m_kvClient = nullptr;
#endif
};
