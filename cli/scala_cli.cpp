#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "cpp/logos_api_client.h"
#include "cpp/token_manager.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        QTextStream out(stdout);
        out << "Usage: scala-cli <method> [args...]\n";
        out << "\nExamples:\n";
        out << "  scala-cli listCalendars\n";
        out << "  scala-cli createCalendar MyCalendar '#3b82f6'\n";
        out << "  scala-cli listEvents <calendarId>\n";
        out << "  scala-cli generateShareLink <calendarId>\n";
        out << "  scala-cli handleShareLink <link>\n";
        out << "  scala-cli getIdentity\n";
        return 1;
    }

    // Read token from running logoscore instance
    QFile tokenFile("/tmp/logos_scala_module");
    if (!tokenFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot read token from /tmp/logos_scala_module.";
        qCritical() << "Is logoscore running with scala_module loaded? (make run-module)";
        return 1;
    }
    QString token = QTextStream(&tokenFile).readAll().trimmed();
    tokenFile.close();

    // Connect to running logoscore via QtRemoteObjects
    TokenManager& tokenManager = TokenManager::instance();
    tokenManager.saveToken("scala_module", token);

    LogosAPIClient client("scala_module", "cli", &tokenManager);

    if (!client.isConnected()) {
        qCritical() << "Cannot connect to scala_module. Is logoscore running?";
        return 1;
    }

    QString method = argv[1];
    QVariantList args;
    for (int i = 2; i < argc; i++)
        args << QString(argv[i]);

    QVariant result = client.invokeRemoteMethod("scala_module", method, args);

    if (result.isValid()) {
        QTextStream(stdout) << result.toString() << "\n";
    } else {
        qWarning() << "Method call failed or returned no result.";
        return 1;
    }

    return 0;
}
