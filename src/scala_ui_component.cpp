#include "scala_ui_component.h"
#include "scala_plugin.h"

#include <QQuickWidget>
#include <QQmlContext>

QWidget* ScalaUIComponent::createWidget(LogosAPI* logosAPI) {
    auto* quickWidget = new QQuickWidget();
    quickWidget->setMinimumSize(800, 600);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    auto* backend = new ScalaPlugin();
    backend->setParent(quickWidget);

    if (logosAPI) {
        backend->initLogos(logosAPI);
    }

    quickWidget->rootContext()->setContextProperty("calendarModule", backend);

    quickWidget->setSource(QUrl("qrc:/scala/CalendarView.qml"));

    return quickWidget;
}

void ScalaUIComponent::destroyWidget(QWidget* widget) {
    delete widget;
}
