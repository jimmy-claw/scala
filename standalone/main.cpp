#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>

#include "calendar_module.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    // Software rendering for headless/no-GPU environments
    qputenv("QT_QUICK_BACKEND", "software");
    qputenv("LIBGL_ALWAYS_SOFTWARE", "1");

    LogosCalendar module;

    QQuickView view;
    view.rootContext()->setContextProperty("calendarModule", &module);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl(QStringLiteral("qrc:/ScalaApp/ScalaApp/qml/CalendarView.qml")));
    if (view.status() == QQuickView::Error) return -1;
    view.resize(800, 600);
    view.show();

    return app.exec();
}
