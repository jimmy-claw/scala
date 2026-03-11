#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include "calendar_module.h"

#include <interface.h>

/**
 * ScalaPlugin — Thin logoscore plugin wrapper around LogosCalendar.
 *
 * logoscore loads this class via QPluginLoader. On initLogos() it creates
 * a LogosCalendar instance, wires it to the Logos SDK, and optionally
 * launches the QML UI if a display is available.
 */
class ScalaPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "metadata.json")
    Q_INTERFACES(PluginInterface)

public:
    explicit ScalaPlugin(QObject *parent = nullptr);

    [[nodiscard]] QString name() const override { return QStringLiteral("scala_module"); }
    Q_INVOKABLE QString version() const override { return QStringLiteral("0.1.0"); }

    Q_INVOKABLE void initLogos(LogosAPI *api);

signals:
    void eventResponse(const QString &eventName, const QVariantList &args);

private:
    LogosCalendar *m_calendar = nullptr;
};
