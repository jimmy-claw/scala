#pragma once

#include <QObject>
#include <QString>

class LogosAPIClient;
class TokenManager;

class ScalaBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString identity READ getIdentity NOTIFY identityChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit ScalaBridge(QObject *parent = nullptr);
    ~ScalaBridge() override;

    bool isConnected() const;

    // ── Calendar CRUD ───────────────────────────────────────────────────────
    Q_INVOKABLE QString listCalendars();
    Q_INVOKABLE QString createCalendar(const QString &name, const QString &color);
    Q_INVOKABLE bool deleteCalendar(const QString &id);

    // ── Event CRUD ──────────────────────────────────────────────────────────
    Q_INVOKABLE QString listEvents(const QString &calendarId);
    Q_INVOKABLE QString createEvent(const QString &calendarId, const QString &eventJson);
    Q_INVOKABLE QString updateEvent(const QString &eventJson);
    Q_INVOKABLE QString deleteEvent(const QString &id);
    Q_INVOKABLE QString getEvent(const QString &id);

    // ── Identity ────────────────────────────────────────────────────────────
    Q_INVOKABLE QString getIdentity();
    Q_INVOKABLE void setIdentity(const QString &pubkeyHex);

    // ── Share link ──────────────────────────────────────────────────────────
    Q_INVOKABLE QString generateShareLink(const QString &calendarId);
    Q_INVOKABLE bool handleShareLink(const QString &link);

    // ── Search & reminders ──────────────────────────────────────────────────
    Q_INVOKABLE QString searchEvents(const QString &query);
    Q_INVOKABLE QString getPendingReminders();

    // ── Settings ────────────────────────────────────────────────────────────
    Q_INVOKABLE void setSetting(const QString &key, const QString &value);
    Q_INVOKABLE QString getSetting(const QString &key, const QString &defaultValue = QString());

signals:
    void identityChanged();
    void connectedChanged();

private:
    QVariant call(const QString &method, const QVariantList &args = {});
    void loadToken();

    LogosAPIClient *m_client = nullptr;
    QString m_cachedIdentity;
};
