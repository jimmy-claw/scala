#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "calendar_module.h"
#include "calendar_sync.h"

class TestIdentity : public QObject {
    Q_OBJECT

private slots:
    void generateStableIdentityNonEmpty();
    void generateStableIdentitySameValue();
    void setGetIdentityRoundtrip();
    void creatorIdSetOnCreateEvent();
    void creatorCanDeleteOwnEvent();
    void nonCreatorDeleteReturnsError();
    void creatorCanUpdateOwnEvent();
    void nonCreatorUpdateReturnsError();
    void signVerifyRoundtrip();
    void signVerifyBadSignature();
    void creatorIdInEventJson();
};

void TestIdentity::generateStableIdentityNonEmpty() {
    LogosCalendar module;
    QString identity = module.getIdentity();
    QVERIFY(!identity.isEmpty());
    // SHA256 hex is 64 chars
    QCOMPARE(identity.length(), 64);
}

void TestIdentity::generateStableIdentitySameValue() {
    LogosCalendar m1;
    LogosCalendar m2;
    // Both should generate the same stable identity on the same machine
    QCOMPARE(m1.getIdentity(), m2.getIdentity());
}

void TestIdentity::setGetIdentityRoundtrip() {
    LogosCalendar module;
    QString original = module.getIdentity();
    QVERIFY(!original.isEmpty());

    QString custom = "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789";
    module.setIdentity(custom);
    QCOMPARE(module.getIdentity(), custom);

    // Setting same value again should be a no-op
    module.setIdentity(custom);
    QCOMPARE(module.getIdentity(), custom);
}

void TestIdentity::creatorIdSetOnCreateEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Test", "#FF0000");

    QString evId = module.createEvent(calId, R"({"title":"Meeting"})");
    QVERIFY(!evId.isEmpty());

    QString evJson = module.getEvent(evId);
    QJsonDocument doc = QJsonDocument::fromJson(evJson.toUtf8());
    QJsonObject obj = doc.object();

    QCOMPARE(obj["creatorId"].toString(), module.getIdentity());
}

void TestIdentity::creatorCanDeleteOwnEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Test", "#FF0000");

    QString evId = module.createEvent(calId, R"({"title":"My Event"})");
    QVERIFY(!evId.isEmpty());

    // Creator deletes own event — should succeed
    QString result = module.deleteEvent(evId);
    QCOMPARE(result, evId);

    // Event should be gone
    QString gone = module.getEvent(evId);
    QVERIFY(gone.isEmpty());
}

void TestIdentity::nonCreatorDeleteReturnsError() {
    LogosCalendar creator;
    QString calId = creator.createCalendar("Test", "#FF0000");

    QString evId = creator.createEvent(calId, R"({"title":"Protected Event"})");
    QVERIFY(!evId.isEmpty());

    // Different identity tries to delete
    LogosCalendar other;
    other.setIdentity("aaaa1111bbbb2222cccc3333dddd4444eeee5555ffff6666000011112222abcd");

    // Need to make the event visible to the other module — copy it
    // In real use, sync would handle this; for testing, we re-create the store scenario
    // The other module has a separate store, so we create the event there too
    QString evJson = creator.getEvent(evId);
    QJsonDocument doc = QJsonDocument::fromJson(evJson.toUtf8());
    QJsonObject obj = doc.object();
    // Create a calendar with same id in other module's store
    other.createCalendar("Test", "#FF0000");
    // Save the event directly by creating it with the same creatorId
    // We'll test ownership by having the other module try to delete from creator's store
    // Actually, since each LogosCalendar has its own store, we test with the same instance
    // by changing identity
    LogosCalendar module;
    QString calId2 = module.createCalendar("Test", "#FF0000");
    QString evId2 = module.createEvent(calId2, R"({"title":"Protected"})");
    QVERIFY(!evId2.isEmpty());

    // Change identity to simulate a different user
    module.setIdentity("aaaa1111bbbb2222cccc3333dddd4444eeee5555ffff6666000011112222abcd");

    // Try to delete — should fail with not_authorized
    QString result = module.deleteEvent(evId2);
    QVERIFY(!result.isEmpty());
    QJsonDocument resDoc = QJsonDocument::fromJson(result.toUtf8());
    QCOMPARE(resDoc.object()["error"].toString(), QStringLiteral("not_authorized"));

    // Event should still exist
    // Switch identity back to verify
    module.setIdentity(module.getIdentity()); // identity is already changed
    // The event should still be in the store
    QString stillThere = module.getEvent(evId2);
    QVERIFY(!stillThere.isEmpty());
}

void TestIdentity::creatorCanUpdateOwnEvent() {
    LogosCalendar module;
    QString calId = module.createCalendar("Test", "#FF0000");
    QString evId = module.createEvent(calId, R"({"title":"Original"})");

    // Build update JSON with required fields
    QJsonObject updateObj;
    updateObj["id"] = evId;
    updateObj["calendarId"] = calId;
    updateObj["title"] = "Updated";
    QString updateJson = QString::fromUtf8(
        QJsonDocument(updateObj).toJson(QJsonDocument::Compact));

    QString result = module.updateEvent(updateJson);
    QCOMPARE(result, evId);
}

void TestIdentity::nonCreatorUpdateReturnsError() {
    LogosCalendar module;
    QString calId = module.createCalendar("Test", "#FF0000");
    QString evId = module.createEvent(calId, R"({"title":"Original"})");
    QString originalIdentity = module.getIdentity();

    // Switch to different identity
    module.setIdentity("bbbb2222cccc3333dddd4444eeee5555ffff6666000011112222333344445555");

    QJsonObject updateObj;
    updateObj["id"] = evId;
    updateObj["calendarId"] = calId;
    updateObj["title"] = "Hacked";
    QString updateJson = QString::fromUtf8(
        QJsonDocument(updateObj).toJson(QJsonDocument::Compact));

    QString result = module.updateEvent(updateJson);
    QVERIFY(!result.isEmpty());
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    QCOMPARE(doc.object()["error"].toString(), QStringLiteral("not_authorized"));
}

void TestIdentity::signVerifyRoundtrip() {
    QString payload = "test payload data";
    QString key = "encryption-key-123";

    QString sig = SyncMessage::sign(payload, key);
    QVERIFY(!sig.isEmpty());
    QVERIFY(SyncMessage::verify(payload, sig, key));
}

void TestIdentity::signVerifyBadSignature() {
    QString payload = "test payload data";
    QString key = "encryption-key-123";

    QVERIFY(!SyncMessage::verify(payload, "bad-signature", key));
    QVERIFY(!SyncMessage::verify(payload, SyncMessage::sign(payload, key), "wrong-key"));
}

void TestIdentity::creatorIdInEventJson() {
    scala::CalendarEvent ev;
    ev.id = "evt-001";
    ev.calendarId = "cal-001";
    ev.title = "Test";
    ev.creatorId = "abcdef1234567890";
    ev.createdAt = 1700000000000LL;
    ev.updatedAt = 1700000000000LL;

    QJsonObject json = ev.toJson();
    QCOMPARE(json["creatorId"].toString(), QStringLiteral("abcdef1234567890"));

    scala::CalendarEvent restored = scala::CalendarEvent::fromJson(json);
    QCOMPARE(restored.creatorId, ev.creatorId);
}

QTEST_MAIN(TestIdentity)
#include "test_identity.moc"
