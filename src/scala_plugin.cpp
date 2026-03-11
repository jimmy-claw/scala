#include "scala_plugin.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

ScalaPlugin::ScalaPlugin(QObject *parent)
    : QObject(parent)
{
}

void ScalaPlugin::initLogos(LogosAPI *api) {
    logosAPI = api;

    // Create the calendar module and wire it to Logos Core
    m_calendar = new LogosCalendar(this);
    m_calendar->initLogos(api);

    // Forward calendar signals through the plugin
    connect(m_calendar, &LogosCalendar::eventResponse,
            this, &ScalaPlugin::eventResponse);

    // Deferred QML UI launch (runs on next event-loop iteration)
    QTimer::singleShot(0, this, [this]() {
        auto *guiApp = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (!guiApp) {
            qInfo() << "ScalaPlugin: no QGuiApplication — running headless";
            return;
        }

        // Look for QML files relative to the modules directory
        QString modulePath = QCoreApplication::applicationDirPath()
                             + QStringLiteral("/../modules/scala_module");
        QString qmlPath = modulePath + QStringLiteral("/qml/CalendarView.qml");

        if (!QFile::exists(qmlPath)) {
            qInfo() << "ScalaPlugin: QML not found at" << qmlPath
                     << "— running headless";
            return;
        }

        auto *engine = new QQmlApplicationEngine(this);
        engine->rootContext()->setContextProperty(
            QStringLiteral("calendarModule"), m_calendar);
        engine->load(QUrl::fromLocalFile(qmlPath));

        qInfo() << "ScalaPlugin: QML UI launched from" << qmlPath;
    });

    qInfo() << "ScalaPlugin: initialized. version:" << version();
    emit eventResponse(QStringLiteral("initialized"),
                       QVariantList() << QStringLiteral("scala_module")
                                      << QStringLiteral("0.1.0"));
}
