#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");
    const QUrl url(QStringLiteral("qrc:/qml/CalendarView.qml"));
    engine.load(url);
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
