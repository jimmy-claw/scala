#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "types.h"

class TestCalendar : public QObject {
    Q_OBJECT

private slots:
    void calendarJsonRoundtrip();
    void calendarJsonRoundtripShared();
    void eventJsonRoundtrip();
    void eventJsonRoundtripAllDay();
    void eventJsonRoundtripWithAttendees();
    void fromJsonMissingFields();
};

void TestCalendar::calendarJsonRoundtrip() {
    scala::Calendar cal;
    cal.id = "cal-001";
    cal.name = "Work";
    cal.color = "#FF5722";
    cal.isShared = false;
    cal.encryptionKey = "";
    cal.createdAt = 1700000000000LL;
    cal.updatedAt = 1700000001000LL;

    QJsonObject json = cal.toJson();
    scala::Calendar restored = scala::Calendar::fromJson(json);

    QCOMPARE(restored.id, cal.id);
    QCOMPARE(restored.name, cal.name);
    QCOMPARE(restored.color, cal.color);
    QCOMPARE(restored.isShared, cal.isShared);
    QCOMPARE(restored.encryptionKey, cal.encryptionKey);
    QCOMPARE(restored.createdAt, cal.createdAt);
    QCOMPARE(restored.updatedAt, cal.updatedAt);
}

void TestCalendar::calendarJsonRoundtripShared() {
    scala::Calendar cal;
    cal.id = "cal-shared-001";
    cal.name = "Team Sprint";
    cal.color = "#4CAF50";
    cal.isShared = true;
    cal.encryptionKey = "dGVzdGtleQ==";
    cal.createdAt = 1700000000000LL;
    cal.updatedAt = 1700000001000LL;

    QJsonObject json = cal.toJson();
    scala::Calendar restored = scala::Calendar::fromJson(json);

    QCOMPARE(restored.isShared, true);
    QCOMPARE(restored.encryptionKey, cal.encryptionKey);
}

void TestCalendar::eventJsonRoundtrip() {
    scala::CalendarEvent ev;
    ev.id = "evt-001";
    ev.calendarId = "cal-001";
    ev.title = "Standup";
    ev.startTime = QDateTime::fromMSecsSinceEpoch(1700000000000LL);
    ev.endTime = QDateTime::fromMSecsSinceEpoch(1700000900000LL);
    ev.allDay = false;
    ev.description = "Daily standup meeting";
    ev.location = "Room 42";
    ev.attendees = {};
    ev.createdAt = 1700000000000LL;
    ev.updatedAt = 1700000000000LL;

    QJsonObject json = ev.toJson();
    scala::CalendarEvent restored = scala::CalendarEvent::fromJson(json);

    QCOMPARE(restored.id, ev.id);
    QCOMPARE(restored.calendarId, ev.calendarId);
    QCOMPARE(restored.title, ev.title);
    QCOMPARE(restored.startTime, ev.startTime);
    QCOMPARE(restored.endTime, ev.endTime);
    QCOMPARE(restored.allDay, ev.allDay);
    QCOMPARE(restored.description, ev.description);
    QCOMPARE(restored.location, ev.location);
    QCOMPARE(restored.createdAt, ev.createdAt);
    QCOMPARE(restored.updatedAt, ev.updatedAt);
}

void TestCalendar::eventJsonRoundtripAllDay() {
    scala::CalendarEvent ev;
    ev.id = "evt-allday";
    ev.calendarId = "cal-001";
    ev.title = "Holiday";
    ev.startTime = QDateTime::fromMSecsSinceEpoch(1700000000000LL);
    ev.endTime = QDateTime::fromMSecsSinceEpoch(1700086400000LL);
    ev.allDay = true;
    ev.createdAt = 1700000000000LL;
    ev.updatedAt = 1700000000000LL;

    QJsonObject json = ev.toJson();
    scala::CalendarEvent restored = scala::CalendarEvent::fromJson(json);

    QCOMPARE(restored.allDay, true);
    QCOMPARE(restored.title, QStringLiteral("Holiday"));
}

void TestCalendar::eventJsonRoundtripWithAttendees() {
    scala::CalendarEvent ev;
    ev.id = "evt-attend";
    ev.calendarId = "cal-001";
    ev.title = "Planning";
    ev.startTime = QDateTime::fromMSecsSinceEpoch(1700000000000LL);
    ev.endTime = QDateTime::fromMSecsSinceEpoch(1700003600000LL);
    ev.attendees = {"pubkey_alice", "pubkey_bob", "pubkey_carol"};
    ev.createdAt = 1700000000000LL;
    ev.updatedAt = 1700000000000LL;

    QJsonObject json = ev.toJson();
    scala::CalendarEvent restored = scala::CalendarEvent::fromJson(json);

    QCOMPARE(restored.attendees.size(), 3);
    QCOMPARE(restored.attendees.at(0), QStringLiteral("pubkey_alice"));
    QCOMPARE(restored.attendees.at(1), QStringLiteral("pubkey_bob"));
    QCOMPARE(restored.attendees.at(2), QStringLiteral("pubkey_carol"));
}

void TestCalendar::fromJsonMissingFields() {
    // Empty JSON should produce default-initialized structs
    QJsonObject emptyObj;

    scala::Calendar cal = scala::Calendar::fromJson(emptyObj);
    QVERIFY(cal.id.isEmpty());
    QVERIFY(cal.name.isEmpty());
    QCOMPARE(cal.isShared, false);
    QCOMPARE(cal.createdAt, 0LL);

    scala::CalendarEvent ev = scala::CalendarEvent::fromJson(emptyObj);
    QVERIFY(ev.id.isEmpty());
    QVERIFY(ev.title.isEmpty());
    QCOMPARE(ev.allDay, false);
    QCOMPARE(ev.attendees.size(), 0);
}

QTEST_MAIN(TestCalendar)
#include "test_calendar.moc"
