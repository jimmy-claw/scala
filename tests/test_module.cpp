#include <QtTest>
#include "../src/calendar_module.h"

class TestModule : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void createAndListCalendar();
    void createAndListEvent();
    void updateEvent();
    void deleteEvent();
    void generateShareLink();
    void searchEvents();
};

void TestModule::createAndListCalendar() {
    LogosCalendar module;
    // createCalendar returns just the calendar ID
    QString calId = module.createCalendar("Work", "#3b82f6");
    QVERIFY(!calId.isEmpty());

    QString list = module.listCalendars();
    auto arr = QJsonDocument::fromJson(list.toUtf8()).array();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()["name"].toString(), QString("Work"));
    QCOMPARE(arr[0].toObject()["color"].toString(), QString("#3b82f6"));
    QCOMPARE(arr[0].toObject()["id"].toString(), calId);
}

void TestModule::createAndListEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Work", "#3b82f6");

    QJsonObject ev;
    ev["title"] = "Standup";
    ev["date"] = "2026-03-10";
    ev["startTime"] = "09:00";
    ev["endTime"] = "09:30";
    // createEvent returns the event ID
    QString eventId = module.createEvent(calId, QJsonDocument(ev).toJson(QJsonDocument::Compact));
    QVERIFY(!eventId.isEmpty());

    QString events = module.listEvents(calId);
    auto arr = QJsonDocument::fromJson(events.toUtf8()).array();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()["title"].toString(), QString("Standup"));
}

void TestModule::updateEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Work", "#3b82f6");

    QJsonObject ev;
    ev["title"] = "Standup";
    ev["date"] = "2026-03-10";
    module.createEvent(calId, QJsonDocument(ev).toJson(QJsonDocument::Compact));

    QString events = module.listEvents(calId);
    QJsonObject created = QJsonDocument::fromJson(events.toUtf8()).array()[0].toObject();
    created["title"] = "Standup Updated";
    // updateEvent takes only the event JSON (calendarId is already in the object)
    module.updateEvent(QJsonDocument(created).toJson(QJsonDocument::Compact));

    QString updated = module.listEvents(calId);
    QCOMPARE(QJsonDocument::fromJson(updated.toUtf8()).array()[0].toObject()["title"].toString(),
             QString("Standup Updated"));
}

void TestModule::deleteEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Work", "#3b82f6");

    QJsonObject ev;
    ev["title"] = "To Delete";
    ev["date"] = "2026-03-10";
    module.createEvent(calId, QJsonDocument(ev).toJson(QJsonDocument::Compact));

    QString events = module.listEvents(calId);
    QString eventId = QJsonDocument::fromJson(events.toUtf8()).array()[0].toObject()["id"].toString();
    // deleteEvent takes only the event ID
    module.deleteEvent(eventId);

    QString after = module.listEvents(calId);
    QCOMPARE(QJsonDocument::fromJson(after.toUtf8()).array().size(), 0);
}

void TestModule::generateShareLink() {
    LogosCalendar module;
    QString calId = module.createCalendar("Shared", "#f59e0b");

    QString link = module.generateShareLink(calId);
    QVERIFY(!link.isEmpty());
    QVERIFY(link.startsWith("scala://"));
}

void TestModule::searchEvents() {
    LogosCalendar module;
    QString calId = module.createCalendar("Work", "#3b82f6");

    QJsonObject ev;
    ev["title"] = "Standup";
    ev["date"] = "2026-03-10";
    module.createEvent(calId, QJsonDocument(ev).toJson(QJsonDocument::Compact));

    // Search finds it by title substring
    QString results = module.searchEvents("stand");
    auto arr = QJsonDocument::fromJson(results.toUtf8()).array();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()["title"].toString(), QString("Standup"));
    QCOMPARE(arr[0].toObject()["calendarName"].toString(), QString("Work"));

    // Case-insensitive
    results = module.searchEvents("STAND");
    arr = QJsonDocument::fromJson(results.toUtf8()).array();
    QCOMPARE(arr.size(), 1);

    // No match
    results = module.searchEvents("xyz_no_match");
    arr = QJsonDocument::fromJson(results.toUtf8()).array();
    QCOMPARE(arr.size(), 0);

    // Empty query returns empty
    results = module.searchEvents("");
    arr = QJsonDocument::fromJson(results.toUtf8()).array();
    QCOMPARE(arr.size(), 0);
}

QTEST_MAIN(TestModule)
#include "test_module.moc"
