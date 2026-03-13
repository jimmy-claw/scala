#include "scala_ui_component.h"
#include "calendar_module.h"

#include <QQuickWidget>
#include <QQmlContext>

QWidget* ScalaUIComponent::createWidget(LogosAPI* logosAPI) {
    auto* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(800, 600);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto* backend = new LogosCalendar();
    backend->setParent(quickWidget);

#ifdef LOGOS_CORE_AVAILABLE
    if (logosAPI) {
        backend->initLogos(logosAPI);
    }
#endif

    quickWidget->rootContext()->setContextProperty("calendarModule", backend);

    quickWidget->setSource(QUrl("qrc:/scala/CalendarView.qml"));

    return quickWidget;
}

void ScalaUIComponent::destroyWidget(QWidget* widget) {
    delete widget;
}
