#include <QApplication>
#include <QCoreApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>
#include <QFile>
#include <QPluginLoader>

#include "calendar_module.h"

#ifdef KV_MODULE_AVAILABLE
#include "i_kv_module.h"
#endif

#ifdef LOGOS_CORE_AVAILABLE
#include "scala_bridge.h"
#endif

// ── CLI mode (connects to running logoscore via QtRO) ────────────────────────

#ifdef LOGOS_CORE_AVAILABLE

static void printUsage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [command] [args...]\n"
        "\n"
        "  (no args)                          Launch GUI\n"
        "\n"
        "Commands:\n"
        "  list-calendars                     List all calendars (JSON)\n"
        "  create-calendar <name> <color>     Create a calendar\n"
        "  delete-calendar <id>               Delete a calendar\n"
        "  list-events <calendar-id>          List events for a calendar (JSON)\n"
        "  create-event <cal-id> <json>       Create an event\n"
        "  update-event <json>                Update an event\n"
        "  delete-event <id>                  Delete an event\n"
        "  get-event <id>                     Get a single event (JSON)\n"
        "  get-identity                       Get public key\n"
        "  generate-share-link <cal-id>       Generate share link\n"
        "  handle-share-link <link>           Join via share link\n"
        "  search-events <query>              Search events\n"
        "  get-pending-reminders              Get pending reminders\n"
        "  help                               Show this help\n"
        "\n"
        "Connects to a running logoscore instance via QtRemoteObjects.\n"
        "Start logoscore first:\n"
        "  logoscore --modules-dir ./modules --load-modules kv_module,scala_module\n",
        prog);
}

static int dispatchCommand(const QString &cmd, int argc, char *argv[],
                           ScalaBridge *bridge)
{
    QTextStream out(stdout);

    if (cmd == "list-calendars") {
        out << bridge->listCalendars() << "\n";
    } else if (cmd == "create-calendar") {
        if (argc < 4) { fprintf(stderr, "Usage: create-calendar <name> <color>\n"); return 1; }
        out << bridge->createCalendar(QString::fromLocal8Bit(argv[2]),
                                      QString::fromLocal8Bit(argv[3])) << "\n";
    } else if (cmd == "delete-calendar") {
        if (argc < 3) { fprintf(stderr, "Usage: delete-calendar <id>\n"); return 1; }
        bool ok = bridge->deleteCalendar(QString::fromLocal8Bit(argv[2]));
        out << (ok ? "deleted" : "failed") << "\n";
        return ok ? 0 : 1;
    } else if (cmd == "list-events") {
        if (argc < 3) { fprintf(stderr, "Usage: list-events <calendar-id>\n"); return 1; }
        out << bridge->listEvents(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "create-event") {
        if (argc < 4) { fprintf(stderr, "Usage: create-event <calendar-id> <event-json>\n"); return 1; }
        out << bridge->createEvent(QString::fromLocal8Bit(argv[2]),
                                   QString::fromLocal8Bit(argv[3])) << "\n";
    } else if (cmd == "update-event") {
        if (argc < 3) { fprintf(stderr, "Usage: update-event <event-json>\n"); return 1; }
        out << bridge->updateEvent(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "delete-event") {
        if (argc < 3) { fprintf(stderr, "Usage: delete-event <id>\n"); return 1; }
        out << bridge->deleteEvent(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "get-event") {
        if (argc < 3) { fprintf(stderr, "Usage: get-event <id>\n"); return 1; }
        out << bridge->getEvent(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "get-identity") {
        out << bridge->getIdentity() << "\n";
    } else if (cmd == "generate-share-link") {
        if (argc < 3) { fprintf(stderr, "Usage: generate-share-link <calendar-id>\n"); return 1; }
        out << bridge->generateShareLink(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "handle-share-link") {
        if (argc < 3) { fprintf(stderr, "Usage: handle-share-link <link>\n"); return 1; }
        bool ok = bridge->handleShareLink(QString::fromLocal8Bit(argv[2]));
        out << (ok ? "joined" : "failed") << "\n";
        return ok ? 0 : 1;
    } else if (cmd == "search-events") {
        if (argc < 3) { fprintf(stderr, "Usage: search-events <query>\n"); return 1; }
        out << bridge->searchEvents(QString::fromLocal8Bit(argv[2])) << "\n";
    } else if (cmd == "get-pending-reminders") {
        out << bridge->getPendingReminders() << "\n";
    } else {
        fprintf(stderr, "Unknown command: %s\n\n", argv[1]);
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}

static int runCli(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString cmd = QString::fromLocal8Bit(argv[1]);
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    // Fast-fail: check socket/token file before attempting QtRO connection
    if (!QFile::exists(QStringLiteral("/tmp/logos_scala_module"))) {
        fprintf(stderr,
            "Error: scala_module not running (no socket file).\n"
            "Start with:\n"
            "  logoscore --modules-dir ./modules --load-modules kv_module,scala_module\n");
        return 1;
    }

    int exitCode = 1;

    // ScalaBridge needs the Qt event loop for QtRemoteObjects.
    // Schedule construction on the event loop so QtRO can connect async.
    QTimer::singleShot(0, &app, [&]() {
        auto *bridge = new ScalaBridge(&app);

        // Poll for connection (up to 5 seconds)
        auto *connTimer = new QTimer(&app);
        connTimer->setInterval(100);
        int retries = 0;

        QObject::connect(connTimer, &QTimer::timeout, &app,
            [&, bridge, connTimer, retries]() mutable {
                if (bridge->isConnected()) {
                    connTimer->stop();
                    exitCode = dispatchCommand(cmd, argc, argv, bridge);
                    app.quit();
                } else if (++retries > 50) {
                    connTimer->stop();
                    fprintf(stderr,
                        "Error: scala_module not responding (socket exists but cannot connect).\n"
                        "Check that logoscore is still running.\n");
                    exitCode = 1;
                    app.quit();
                }
            });

        connTimer->start();
    });

    app.exec();
    return exitCode;
}

#endif // LOGOS_CORE_AVAILABLE

// ── GUI mode ─────────────────────────────────────────────────────────────────

static int runGui(int argc, char *argv[])
{
    // Software rendering for headless/no-GPU environments — set before QApp
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");

    QApplication app(argc, argv);

    // ── Module setup ────────────────────────────────────────────────────────
    // In-memory fallback is always available; ScalaBridge talks to
    // the running scala_module via QtRO when LOGOS_CORE_AVAILABLE.
    LogosCalendar module;
    module.setNamespace(qEnvironmentVariable("SCALA_NAMESPACE", "default"));

#ifdef KV_MODULE_AVAILABLE
    // Try to load kv_module plugin for persistence (no logos_host needed)
    QPluginLoader kvLoader(QStringLiteral("kv_module_plugin"));
    if (kvLoader.load()) {
        QObject *plugin = kvLoader.instance();
        if (plugin) {
            IKvModule *kvModule = qobject_cast<IKvModule *>(plugin);
            if (kvModule) {
                module.setKvModule(kvModule);
                qDebug() << "scala_standalone: loaded kv_module plugin for persistence";
            } else {
                qWarning() << "scala_standalone: kv_module plugin loaded but not IKvModule interface";
            }
        } else {
            qWarning() << "scala_standalone: failed to get kv_module plugin instance:" << kvLoader.errorString();
        }
    } else {
        qDebug() << "scala_standalone: kv_module plugin not found, using in-memory fallback";
    }
#endif

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

// ── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
#ifdef LOGOS_CORE_AVAILABLE
    if (argc > 1)
        return runCli(argc, argv);
#endif

    return runGui(argc, argv);
}
