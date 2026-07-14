#include <QtTest>
#include "../src/calendar_store.h"

class TestNamespace : public QObject {
    Q_OBJECT
private slots:
    void isolationBetweenNamespaces();
    void defaultNamespaceIsDefault();
    void sameNamespaceSharesData();
};

void TestNamespace::isolationBetweenNamespaces() {
    // Use a single store instance since in-memory map is per-instance
    CalendarStore store;

    store.setNamespace("alice");
    store.kvSet("mykey", "alice-value");

    // Switch to bob — should NOT see alice's data
    store.setNamespace("bob");
    QCOMPARE(store.kvGet("mykey"), QString());

    // Switch back to alice — should still see it
    store.setNamespace("alice");
    QCOMPARE(store.kvGet("mykey"), QString("alice-value"));
}

void TestNamespace::defaultNamespaceIsDefault() {
    CalendarStore store;
    // Default namespace is "default" — write without calling setNamespace
    store.kvSet("key", "hello");

    // Explicitly set to "default" — should see the same data
    store.setNamespace("default");
    QCOMPARE(store.kvGet("key"), QString("hello"));
}

void TestNamespace::sameNamespaceSharesData() {
    CalendarStore store;
    store.setNamespace("shared");
    store.kvSet("key", "value");

    // Re-set same namespace — data should persist
    store.setNamespace("shared");
    QCOMPARE(store.kvGet("key"), QString("value"));
}

QTEST_MAIN(TestNamespace)
#include "test_namespace.moc"
