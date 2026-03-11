#include <QApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "calendar_module.h"

#ifdef LOGOS_CORE_AVAILABLE
#include "scala_bridge.h"
#endif

int main(int argc, char *argv[]) {
    // Software rendering for headless/no-GPU environments — set before QApp
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");

    QApplication app(argc, argv);

    // ── Module setup ────────────────────────────────────────────────────────
    // In-memory fallback is always available; ScalaBridge talks to
    // the running scala_module via QtRO when LOGOS_CORE_AVAILABLE.
    LogosCalendar module;
    module.setNamespace(qEnvironmentVariable("SCALA_NAMESPACE", "default"));

#ifdef LOGOS_CORE_AVAILABLE
    ScalaBridge bridge;
    QObject *calendarBackend = &bridge;
    qDebug() << "scala_standalone: using ScalaBridge, connected =" << bridge.isConnected();
#else
    QObject *calendarBackend = &module;
    qDebug() << "scala_standalone: using local LogosCalendar (in-memory)";
#endif

    // ── System tray icon ────────────────────────────────────────────────────
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon::fromTheme(
        QStringLiteral("x-office-calendar"),
        app.style()->standardIcon(QStyle::SP_ComputerIcon)));
    trayIcon.setToolTip(QStringLiteral("Scala Calendar"));
    trayIcon.show();

    // ── Reminder timer (every 60 seconds) ───────────────────────────────────
    QTimer reminderTimer;
    QObject::connect(&reminderTimer, &QTimer::timeout, [calendarBackend, &trayIcon]() {
        QString json;
        QMetaObject::invokeMethod(calendarBackend, "getPendingReminders",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(QString, json));
        QJsonArray reminders = QJsonDocument::fromJson(json.toUtf8()).array();
        for (const auto &val : reminders) {
            QJsonObject ev = val.toObject();
            QString title = ev[QStringLiteral("title")].toString();
            QDateTime start = QDateTime::fromMSecsSinceEpoch(
                static_cast<qint64>(ev[QStringLiteral("startTime")].toDouble()));
            QString timeStr = start.toString(QStringLiteral("hh:mm"));
            trayIcon.showMessage(
                QStringLiteral("Upcoming event"),
                title + QStringLiteral(" at ") + timeStr,
                QSystemTrayIcon::Information, 5000);
        }
    });
    reminderTimer.start(60000);

    // ── QML view ────────────────────────────────────────────────────────────
    QQuickView view;
    view.rootContext()->setContextProperty("calendarModule", calendarBackend);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl(QStringLiteral("qrc:/ScalaApp/ScalaApp/qml/CalendarView.qml")));
    if (view.status() == QQuickView::Error) {
        const auto errors = view.errors();
        for (const auto &e : errors)
            qWarning() << "QML error:" << e.toString();
        return -1;
    }
    view.resize(800, 600);
    view.show();

    return app.exec();
}
