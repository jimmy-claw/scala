#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "calendar_module.h"
#include "qr_generator.h"

class TestSharing : public QObject {
    Q_OBJECT

private slots:
    void generateAndParseRoundtrip();
    void parseLinkContainsAllFields();
    void parseInvalidLinkReturnsEmpty();
    void parseWrongSchemeReturnsEmpty();
    void parseMissingIdReturnsEmpty();
    void parseMissingKeyReturnsEmpty();
    void handleShareLinkJoinsCalendar();
    void handleInvalidLinkReturnsFalse();
    void qrGeneratorReturnsDataUrl();
    void qrGeneratorEmptyInputReturnsEmpty();
};

void TestSharing::generateAndParseRoundtrip() {
    LogosCalendar cal;

    // Create a calendar first
    QString calId = cal.createCalendar("Test Calendar", "#FF0000");
    QVERIFY(!calId.isEmpty());

    // Generate a share link
    QString link = cal.generateShareLink(calId);
    QVERIFY(!link.isEmpty());
    QVERIFY(link.startsWith("scala://join"));

    // Parse it back
    QString parsed = cal.parseShareLink(link);
    QVERIFY(!parsed.isEmpty());

    QJsonDocument doc = QJsonDocument::fromJson(parsed.toUtf8());
    QJsonObject obj = doc.object();

    QCOMPARE(obj["id"].toString(), calId);
    QVERIFY(!obj["key"].toString().isEmpty());
    QCOMPARE(obj["name"].toString(), QStringLiteral("Test Calendar"));
}

void TestSharing::parseLinkContainsAllFields() {
    LogosCalendar cal;
    QString calId = cal.createCalendar("My Calendar", "#00FF00");
    QString link = cal.generateShareLink(calId);

    QString parsed = cal.parseShareLink(link);
    QJsonDocument doc = QJsonDocument::fromJson(parsed.toUtf8());
    QJsonObject obj = doc.object();

    // All three fields must be present
    QVERIFY(obj.contains("id"));
    QVERIFY(obj.contains("key"));
    QVERIFY(obj.contains("name"));

    QVERIFY(!obj["id"].toString().isEmpty());
    QVERIFY(!obj["key"].toString().isEmpty());
    QCOMPARE(obj["name"].toString(), QStringLiteral("My Calendar"));
}

void TestSharing::parseInvalidLinkReturnsEmpty() {
    LogosCalendar cal;

    QVERIFY(cal.parseShareLink("").isEmpty());
    QVERIFY(cal.parseShareLink("not a url").isEmpty());
    QVERIFY(cal.parseShareLink("https://example.com").isEmpty());
    QVERIFY(cal.parseShareLink("scala://invalid").isEmpty());
}

void TestSharing::parseWrongSchemeReturnsEmpty() {
    LogosCalendar cal;
    QVERIFY(cal.parseShareLink("https://join?id=abc&key=def&name=test").isEmpty());
}

void TestSharing::parseMissingIdReturnsEmpty() {
    LogosCalendar cal;
    QVERIFY(cal.parseShareLink("scala://join?key=dGVzdA&name=test").isEmpty());
}

void TestSharing::parseMissingKeyReturnsEmpty() {
    LogosCalendar cal;
    QVERIFY(cal.parseShareLink("scala://join?id=abc&name=test").isEmpty());
}

void TestSharing::handleShareLinkJoinsCalendar() {
    // Create a calendar on one instance and generate a share link
    LogosCalendar source;
    QString calId = source.createCalendar("Shared Cal", "#0000FF");
    QString link = source.generateShareLink(calId);
    QVERIFY(!link.isEmpty());

    // Join from another instance
    LogosCalendar dest;
    bool joined = dest.handleShareLink(link);
    QVERIFY(joined);

    // Verify the calendar was created on dest
    QString calendarsJson = dest.listCalendars();
    QVERIFY(calendarsJson.contains(calId));
}

void TestSharing::handleInvalidLinkReturnsFalse() {
    LogosCalendar cal;
    QVERIFY(!cal.handleShareLink(""));
    QVERIFY(!cal.handleShareLink("garbage"));
    QVERIFY(!cal.handleShareLink("https://example.com"));
}

void TestSharing::qrGeneratorReturnsDataUrl() {
    QString url = QrGenerator::generateQrDataUrl("scala://join?id=test&key=abc");
    QVERIFY(!url.isEmpty());
    QVERIFY(url.startsWith("data:image/svg+xml;base64,"));
}

void TestSharing::qrGeneratorEmptyInputReturnsEmpty() {
    QVERIFY(QrGenerator::generateQrDataUrl("").isEmpty());
}

QTEST_MAIN(TestSharing)
#include "test_sharing.moc"
