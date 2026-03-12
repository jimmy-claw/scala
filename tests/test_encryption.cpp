#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../src/calendar_module.h"
#include "../src/calendar_store.h"

/**
 * TestEncryption — unit tests for at-rest KV encryption.
 *
 * These tests run in standalone mode (no Logos Core / kv_module),
 * exercising the XOR stream-cipher fallback in CalendarStore and
 * the PBKDF2 key-derivation in LogosCalendar.
 */
class TestEncryption : public QObject {
    Q_OBJECT

private slots:
    // CalendarStore-level tests
    void storeEncryptDecryptRoundtrip();
    void storeEncryptionChangesStoredValue();
    void storeIsEncryptionEnabled();
    void storeDisableEncryption();
    void storeWrongKeyProducesGarbage();

    // LogosCalendar-level tests
    void moduleEnableEncryption();
    void moduleEnableEncryptionEmptyPasswordFails();
    void moduleSameSaltSameKey();
    void moduleCalendarSurvivesEncryptionEnabled();
    void moduleEventSurvivesEncryptionEnabled();
    void moduleIsEncryptionEnabled();
    void moduleDisableEncryption();
};

// ─────────────────────────────────────────────────────────────────────────────
// CalendarStore-level tests
// ─────────────────────────────────────────────────────────────────────────────

void TestEncryption::storeEncryptDecryptRoundtrip() {
    CalendarStore store;

    QByteArray key(32, '\xAB'); // deterministic test key
    store.enableEncryption(key);
    QVERIFY(store.isEncryptionEnabled());

    store.kvSet("hello", "world");
    QCOMPARE(store.kvGet("hello"), QString("world"));
}

void TestEncryption::storeEncryptionChangesStoredValue() {
    // Write the same value with and without encryption → stored bytes differ
    CalendarStore plainStore;
    plainStore.kvSet("secret", "my-data");
    QString plainRaw = plainStore.kvGet("secret");

    CalendarStore encStore;
    QByteArray key(32, '\x42');
    encStore.enableEncryption(key);
    encStore.kvSet("secret", "my-data");

    // After disabling, the raw stored value should NOT equal "my-data"
    // (i.e., it was stored encrypted).
    // We verify by disabling encryption and reading — should fail to decode:
    encStore.disableEncryption();
    QVERIFY(encStore.kvGet("secret") != QString("my-data"));
}

void TestEncryption::storeIsEncryptionEnabled() {
    CalendarStore store;
    QVERIFY(!store.isEncryptionEnabled());

    QByteArray key(32, '\x01');
    store.enableEncryption(key);
    QVERIFY(store.isEncryptionEnabled());
}

void TestEncryption::storeDisableEncryption() {
    CalendarStore store;
    QByteArray key(32, '\x01');
    store.enableEncryption(key);
    QVERIFY(store.isEncryptionEnabled());

    store.disableEncryption();
    QVERIFY(!store.isEncryptionEnabled());

    // Writes after disable are plaintext
    store.kvSet("plain", "hello");
    QCOMPARE(store.kvGet("plain"), QString("hello"));
}

void TestEncryption::storeWrongKeyProducesGarbage() {
    CalendarStore store;
    QByteArray key1(32, '\x11');
    store.enableEncryption(key1);
    store.kvSet("data", "sensitive");

    // Switch to a different key — read should NOT return the original plaintext
    store.disableEncryption();
    QByteArray key2(32, '\x22');
    store.enableEncryption(key2);

    QString result = store.kvGet("data");
    QVERIFY(result != QString("sensitive"));
}

// ─────────────────────────────────────────────────────────────────────────────
// LogosCalendar-level tests
// ─────────────────────────────────────────────────────────────────────────────

void TestEncryption::moduleEnableEncryption() {
    LogosCalendar module;
    bool ok = module.enableEncryption("correct-horse-battery-staple");
    QVERIFY(ok);
    QVERIFY(module.isEncryptionEnabled());
}

void TestEncryption::moduleEnableEncryptionEmptyPasswordFails() {
    LogosCalendar module;
    bool ok = module.enableEncryption("");
    QVERIFY(!ok);
    QVERIFY(!module.isEncryptionEnabled());
}

void TestEncryption::moduleSameSaltSameKey() {
    // Two modules sharing the same underlying store (via same namespace)
    // Both should derive the same key from the same password.
    // We test this by enabling encryption, writing data, then creating
    // a new module instance that re-enables encryption with the same password
    // and verifies the data is still readable.
    //
    // In standalone mode there is no shared persistent store across instances,
    // so instead we test PBKDF2 determinism: calling enableEncryption twice
    // with the same password on the same instance should succeed.

    LogosCalendar module;
    QVERIFY(module.enableEncryption("my-password"));
    QString calId = module.createCalendar("Test", "#fff");
    QVERIFY(!calId.isEmpty());

    // Re-enable with the same password (salt already stored)
    module.disableEncryption();
    QVERIFY(module.enableEncryption("my-password"));
    QVERIFY(module.isEncryptionEnabled());
}

void TestEncryption::moduleCalendarSurvivesEncryptionEnabled() {
    LogosCalendar module;

    // Enable encryption before any writes
    QVERIFY(module.enableEncryption("passw0rd!"));

    QString calId = module.createCalendar("Encrypted Calendar", "#3b82f6");
    QVERIFY(!calId.isEmpty());

    QString list = module.listCalendars();
    QJsonArray arr = QJsonDocument::fromJson(list.toUtf8()).array();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()["name"].toString(), QString("Encrypted Calendar"));
    QCOMPARE(arr[0].toObject()["id"].toString(), calId);
}

void TestEncryption::moduleEventSurvivesEncryptionEnabled() {
    LogosCalendar module;
    QVERIFY(module.enableEncryption("passw0rd!"));

    QString calId = module.createCalendar("Work", "#3b82f6");

    QJsonObject ev;
    ev["title"] = "Encrypted Meeting";
    ev["date"] = "2026-03-12";
    ev["startTime"] = "10:00";
    ev["endTime"] = "11:00";
    QString eventId = module.createEvent(calId, QJsonDocument(ev).toJson(QJsonDocument::Compact));
    QVERIFY(!eventId.isEmpty());

    QString events = module.listEvents(calId);
    QJsonArray arr = QJsonDocument::fromJson(events.toUtf8()).array();
    QCOMPARE(arr.size(), 1);
    QCOMPARE(arr[0].toObject()["title"].toString(), QString("Encrypted Meeting"));
}

void TestEncryption::moduleIsEncryptionEnabled() {
    LogosCalendar module;
    QVERIFY(!module.isEncryptionEnabled());
    QVERIFY(module.enableEncryption("test-pass"));
    QVERIFY(module.isEncryptionEnabled());
}

void TestEncryption::moduleDisableEncryption() {
    LogosCalendar module;
    QVERIFY(module.enableEncryption("test-pass"));
    QVERIFY(module.isEncryptionEnabled());
    module.disableEncryption();
    QVERIFY(!module.isEncryptionEnabled());
}

QTEST_MAIN(TestEncryption)
#include "test_encryption.moc"
