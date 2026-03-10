#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

#include "calendar_sync.h"
#include "calendar_module.h"

// ── SyncMessage tests ───────────────────────────────────────────────────────

class TestSync : public QObject {
    Q_OBJECT

private slots:
    void syncMessageJsonRoundtrip();
    void syncMessageBytesRoundtrip();
    void syncMessageAllTypes();
    void syncMessageFromEmptyJson();
    void topicForCalendarFormat();
    void topicForCalendarDifferentIds();
    void calendarSyncStartStop();
    void calendarSyncSendWithoutSync();
    void shareCalendarGeneratesKey();
    void shareCalendarIdempotent();
    void joinSharedCalendar();
    void getSyncStatusValues();
};

void TestSync::syncMessageJsonRoundtrip() {
    SyncMessage msg;
    msg.type = SyncMessageType::CreateEvent;
    msg.calendarId = "cal-001";
    msg.payload = R"({"title":"Meeting"})";
    msg.senderId = "pubkey_alice";
    msg.timestamp = 1700000000000LL;

    QJsonObject json = msg.toJson();
    SyncMessage restored = SyncMessage::fromJson(json);

    QCOMPARE(restored.type, msg.type);
    QCOMPARE(restored.calendarId, msg.calendarId);
    QCOMPARE(restored.payload, msg.payload);
    QCOMPARE(restored.senderId, msg.senderId);
    QCOMPARE(restored.timestamp, msg.timestamp);
}

void TestSync::syncMessageBytesRoundtrip() {
    SyncMessage msg;
    msg.type = SyncMessageType::UpdateEvent;
    msg.calendarId = "cal-002";
    msg.payload = R"({"id":"evt-1","title":"Updated"})";
    msg.senderId = "pubkey_bob";
    msg.timestamp = 1700000001000LL;

    QByteArray bytes = msg.toBytes();
    QVERIFY(!bytes.isEmpty());

    SyncMessage restored = SyncMessage::fromBytes(bytes);

    QCOMPARE(restored.type, SyncMessageType::UpdateEvent);
    QCOMPARE(restored.calendarId, msg.calendarId);
    QCOMPARE(restored.payload, msg.payload);
    QCOMPARE(restored.senderId, msg.senderId);
    QCOMPARE(restored.timestamp, msg.timestamp);
}

void TestSync::syncMessageAllTypes() {
    // Verify all message types roundtrip through string conversion
    QList<SyncMessageType> types = {
        SyncMessageType::CreateEvent,
        SyncMessageType::UpdateEvent,
        SyncMessageType::DeleteEvent,
        SyncMessageType::SyncEvents,
        SyncMessageType::UnshareCalendar,
        SyncMessageType::CalendarReconnected
    };

    for (auto t : types) {
        SyncMessage msg;
        msg.type = t;
        msg.calendarId = "cal-test";
        msg.timestamp = 1700000000000LL;

        QJsonObject json = msg.toJson();
        SyncMessage restored = SyncMessage::fromJson(json);
        QCOMPARE(restored.type, t);
    }
}

void TestSync::syncMessageFromEmptyJson() {
    QJsonObject emptyObj;
    SyncMessage msg = SyncMessage::fromJson(emptyObj);

    // Defaults
    QCOMPARE(msg.type, SyncMessageType::CreateEvent);
    QVERIFY(msg.calendarId.isEmpty());
    QVERIFY(msg.payload.isEmpty());
    QVERIFY(msg.senderId.isEmpty());
    QCOMPARE(msg.timestamp, 0LL);
}

void TestSync::topicForCalendarFormat() {
    QString topic = CalendarSync::topicForCalendar("abc-123");
    QCOMPARE(topic, QStringLiteral("/scala/1/abc-123/json"));
}

void TestSync::topicForCalendarDifferentIds() {
    QString t1 = CalendarSync::topicForCalendar("id-1");
    QString t2 = CalendarSync::topicForCalendar("id-2");

    QVERIFY(t1 != t2);
    QVERIFY(t1.startsWith("/scala/1/"));
    QVERIFY(t1.endsWith("/json"));
    QVERIFY(t2.startsWith("/scala/1/"));
    QVERIFY(t2.endsWith("/json"));
}

void TestSync::calendarSyncStartStop() {
    CalendarSync sync;
    QSignalSpy startedSpy(&sync, &CalendarSync::syncStarted);
    QSignalSpy stoppedSpy(&sync, &CalendarSync::syncStopped);

    QVERIFY(!sync.isSyncing("cal-001"));

    sync.startSync("cal-001", "key123");
    QVERIFY(sync.isSyncing("cal-001"));
    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(startedSpy.at(0).at(0).toString(), QStringLiteral("cal-001"));

    // Starting again should be a no-op
    sync.startSync("cal-001", "key123");
    QCOMPARE(startedSpy.count(), 1);

    sync.stopSync("cal-001");
    QVERIFY(!sync.isSyncing("cal-001"));
    QCOMPARE(stoppedSpy.count(), 1);
}

void TestSync::calendarSyncSendWithoutSync() {
    CalendarSync sync;
    QSignalSpy errorSpy(&sync, &CalendarSync::syncError);

    SyncMessage msg;
    msg.type = SyncMessageType::CreateEvent;
    msg.calendarId = "cal-not-syncing";

    sync.sendMessage("cal-not-syncing", msg);
    QCOMPARE(errorSpy.count(), 1);
}

void TestSync::shareCalendarGeneratesKey() {
    LogosCalendar module;

    // Create a calendar first
    QString calId = module.createCalendar("Team", "#FF0000");
    QVERIFY(!calId.isEmpty());

    // Share it
    QString key = module.shareCalendar(calId);
    QVERIFY(!key.isEmpty());

    // Verify sync status
    QCOMPARE(module.getSyncStatus(calId), QStringLiteral("syncing"));
}

void TestSync::shareCalendarIdempotent() {
    LogosCalendar module;

    QString calId = module.createCalendar("Team", "#00FF00");
    QString key1 = module.shareCalendar(calId);
    QString key2 = module.shareCalendar(calId);

    // Same key returned on re-share
    QCOMPARE(key1, key2);
}

void TestSync::joinSharedCalendar() {
    LogosCalendar module;

    bool ok = module.joinSharedCalendar("remote-cal-123", "someEncryptionKey");
    QVERIFY(ok);
    QCOMPARE(module.getSyncStatus("remote-cal-123"), QStringLiteral("syncing"));

    // Reject empty params
    QVERIFY(!module.joinSharedCalendar("", "key"));
    QVERIFY(!module.joinSharedCalendar("id", ""));
}

void TestSync::getSyncStatusValues() {
    LogosCalendar module;

    // Non-existent calendar
    QCOMPARE(module.getSyncStatus("nonexistent"), QStringLiteral("not_shared"));

    // Created but not shared
    QString calId = module.createCalendar("Private", "#0000FF");
    QCOMPARE(module.getSyncStatus(calId), QStringLiteral("not_shared"));

    // After sharing
    module.shareCalendar(calId);
    QCOMPARE(module.getSyncStatus(calId), QStringLiteral("syncing"));
}

QTEST_MAIN(TestSync)
#include "test_sync.moc"
