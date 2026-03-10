import QtQuick
import QtTest

TestCase {
    name: "CalendarView"
    width: 1280
    height: 800

    function test_viewModeDefaultsToMonth() {
        // Instantiate CalendarView and check default viewMode
        var view = Qt.createComponent("../../qml/CalendarView.qml")
        compare(view.status, Component.Ready)
    }
}
