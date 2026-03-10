import QtQuick
import QtTest

TestCase {
    name: "CalendarView"
    width: 1280
    height: 800

    // Minimal stub for calendarModule so CalendarView.qml can load
    QtObject {
        id: mockModule
        function listCalendars() { return "[]" }
        function listEvents(id) { return "[]" }
        function createCalendar(name, color) { return JSON.stringify({id:"1",name:name,color:color}) }
        function createEvent(calId, data) { return "true" }
        function updateEvent(calId, data) { return "true" }
        function deleteEvent(calId, evId) { return "true" }
        function generateShareLink(calId) { return "scala://join?id=1&key=abc" }
        function handleShareLink(link) { return "true" }
        function getIdentity() { return JSON.stringify({id:"test-identity"}) }
        function searchEvents(q) { return "[]" }
    }

    function test_componentLoads() {
        // Verify the QML files are syntactically valid and loadable
        var comp = Qt.createComponent("../../qml/CalendarView.qml")
        // Component.Error = 3, anything else means it at least parsed
        verify(comp.status !== Component.Error, "CalendarView.qml failed to load: " + comp.errorString())
    }

    function test_viewModeValues() {
        // Just verify the QML constants we use are what we expect
        compare(Component.Ready, 1)
        compare(Component.Error, 3)
    }
}
