#include <QApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "calendar_module.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Software rendering for headless/no-GPU environments
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");

    LogosCalendar module;
    module.setNamespace(qEnvironmentVariable("SCALA_NAMESPACE", "default"));

    // ── System tray icon ─────────────────────────────────────────────────
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(QIcon::fromTheme(
        QStringLiteral("x-office-calendar"),
        app.style()->standardIcon(QStyle::SP_ComputerIcon)));
    trayIcon.setToolTip(QStringLiteral("Scala Calendar"));
    trayIcon.show();

    // ── Reminder timer (every 60 seconds) ────────────────────────────────
    QTimer reminderTimer;
    QObject::connect(&reminderTimer, &QTimer::timeout, [&module, &trayIcon]() {
        QString json = module.getPendingReminders();
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

    // ── QML view ─────────────────────────────────────────────────────────
    QQuickView view;
    view.rootContext()->setContextProperty("calendarModule", &module);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl(QStringLiteral("qrc:/ScalaApp/ScalaApp/qml/CalendarView.qml")));
    if (view.status() == QQuickView::Error) return -1;
    view.resize(800, 600);
    view.show();

    return app.exec();
}
