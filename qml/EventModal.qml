import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    width: Math.min(parent.width - 40, 480)
    height: Math.min(parent.height - 40, 560)
    padding: 0

    // ── Theme ──────────────────────────────────────────────────────────────
    property color headerColor: "#2196F3"
    property color fieldBg: "#f5f5f5"
    property color fieldBorder: "#e0e0e0"
    property color saveBtnColor: "#4CAF50"
    property color cancelBtnColor: "#9e9e9e"

    // ── Mode: "create" or "edit" ───────────────────────────────────────────
    property string mode: "create"

    // ── Form fields ────────────────────────────────────────────────────────
    property alias eventTitle: titleField.text
    property alias eventDescription: descField.text
    property alias eventLocation: locationField.text
    property bool allDay: allDaySwitch.checked
    property string startDate: ""
    property string startTime: ""
    property string endDate: ""
    property string endTime: ""

    // Internal: parse "YYYY-MM-DD" → spinbox values
    function _applyDate(dateStr, yearSpin, monthSpin, daySpin) {
        if (!dateStr || dateStr === "") {
            var now = new Date()
            yearSpin.value = now.getFullYear()
            monthSpin.value = now.getMonth() + 1
            daySpin.value = now.getDate()
            return
        }
        var parts = dateStr.split("-")
        yearSpin.value = parseInt(parts[0])
        monthSpin.value = parseInt(parts[1])
        daySpin.value = parseInt(parts[2])
    }
    function _applyTime(timeStr, hourSpin, minuteSpin) {
        if (!timeStr || timeStr === "") {
            hourSpin.value = 9
            minuteSpin.value = 0
            return
        }
        var parts = timeStr.split(":")
        hourSpin.value = parseInt(parts[0])
        minuteSpin.value = parseInt(parts[1])
    }
    function _dateStr(yearSpin, monthSpin, daySpin) {
        var pad = function(n) { return n < 10 ? "0" + n : "" + n }
        return yearSpin.value + "-" + pad(monthSpin.value) + "-" + pad(daySpin.value)
    }
    function _timeStr(hourSpin, minuteSpin) {
        var pad = function(n) { return n < 10 ? "0" + n : "" + n }
        return pad(hourSpin.value) + ":" + pad(minuteSpin.value)
    }
    property string calendarId: ""
    property string eventId: ""
    property int reminderMinutes: -1
    property var calendars: []

    signal saveClicked(var eventData)
    signal cancelClicked()

    function clear() {
        titleField.text = "";
        descField.text = "";
        locationField.text = "";
        allDaySwitch.checked = false;
        mode = "create";
        eventId = "";
        calendarId = "";
        reminderMinutes = -1;
        reminderCombo.currentIndex = 0;
        calendarCombo.currentIndex = 0;
        _applyDate(startDate, startYearSpin, startMonthSpin, startDaySpin)
        _applyTime(startTime, startHourSpin, startMinuteSpin)
        _applyDate(endDate, endYearSpin, endMonthSpin, endDaySpin)
        _applyTime(endTime, endHourSpin, endMinuteSpin)
    }

    function loadEvent(ev) {
        mode = "edit";
        eventId = ev.id || "";
        titleField.text = ev.title || "";
        descField.text = ev.description || "";
        locationField.text = ev.location || "";
        allDaySwitch.checked = ev.allDay || false;
        calendarId = ev.calendarId || "";
        _applyDate(ev.startDate || "", startYearSpin, startMonthSpin, startDaySpin)
        _applyTime(ev.startTime || "", startHourSpin, startMinuteSpin)
        _applyDate(ev.endDate || "", endYearSpin, endMonthSpin, endDaySpin)
        _applyTime(ev.endTime || "", endHourSpin, endMinuteSpin)
        // Restore reminder setting
        reminderMinutes = (ev.reminderMinutes !== undefined) ? ev.reminderMinutes : -1
        var reminderOptions = [-1, 15, 30, 60]
        for (var ri = 0; ri < reminderOptions.length; ri++) {
            if (reminderOptions[ri] === reminderMinutes) {
                reminderCombo.currentIndex = ri;
                break;
            }
        }
        // Select the matching calendar in the combo
        for (var i = 0; i < calendars.length; i++) {
            if (calendars[i].id === calendarId) {
                calendarCombo.currentIndex = i;
                break;
            }
        }
    }

    background: Rectangle {
        radius: 8
        color: "white"
        border.color: "#e0e0e0"
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header ─────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: headerColor
            radius: 8

            // Square off bottom corners
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 8
                color: headerColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Text {
                    text: mode === "edit" ? "Edit Event" : "New Event"
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "X"
                    flat: true
                    onClicked: { root.close(); cancelClicked(); }
                    implicitWidth: 32
                    implicitHeight: 32
                    contentItem: Text {
                        text: "X"
                        color: "white"
                        font.pixelSize: 16
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: "transparent" }
                }
            }
        }

        // ── Form fields ────────────────────────────────────────────────────
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: formColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: formColumn
                width: parent.width
                spacing: 12
                anchors.margins: 16
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 12

                // Calendar selector
                Text {
                    text: "Calendar"
                    font.pixelSize: 13
                    color: "#555"
                    visible: calendars.length > 0
                }
                ComboBox {
                    id: calendarCombo
                    Layout.fillWidth: true
                    visible: calendars.length > 0
                    model: calendars
                    textRole: "name"
                    onCurrentIndexChanged: {
                        if (currentIndex >= 0 && currentIndex < calendars.length) {
                            calendarId = calendars[currentIndex].id
                        }
                    }
                    delegate: ItemDelegate {
                        width: calendarCombo.width
                        contentItem: RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 10; height: 10; radius: 5
                                color: modelData.color || "#2196F3"
                            }
                            Text {
                                text: modelData.name || ""
                                font.pixelSize: 13
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                // Title
                Text { text: "Title *"; font.pixelSize: 13; color: "#555" }
                TextField {
                    id: titleField
                    Layout.fillWidth: true
                    placeholderText: "Event title"
                    font.pixelSize: 14
                    background: Rectangle {
                        radius: 4; color: fieldBg; border.color: fieldBorder
                    }
                }

                // All-day toggle
                RowLayout {
                    spacing: 8
                    Text { text: "All day"; font.pixelSize: 13; color: "#555" }
                    Switch { id: allDaySwitch }
                }

                // Start date / time
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Start Date"; font.pixelSize: 12; color: "#555" }
                        RowLayout {
                            spacing: 4
                            SpinBox {
                                id: startYearSpin
                                from: 2020; to: 2099; value: new Date().getFullYear()
                                editable: true
                                implicitWidth: 90
                                font.pixelSize: 12
                            }
                            Text { text: "-"; color: "#555"; font.pixelSize: 14 }
                            SpinBox {
                                id: startMonthSpin
                                from: 1; to: 12; value: new Date().getMonth() + 1
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                            }
                            Text { text: "-"; color: "#555"; font.pixelSize: 14 }
                            SpinBox {
                                id: startDaySpin
                                from: 1; to: 31; value: new Date().getDate()
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                            }
                        }
                    }

                    ColumnLayout {
                        visible: !allDaySwitch.checked
                        Text { text: "Start Time"; font.pixelSize: 12; color: "#555" }
                        RowLayout {
                            spacing: 4
                            SpinBox {
                                id: startHourSpin
                                from: 0; to: 23; value: 9
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                                textFromValue: function(value) {
                                    return value < 10 ? "0" + value : "" + value
                                }
                            }
                            Text { text: ":"; color: "#555"; font.pixelSize: 14; font.bold: true }
                            SpinBox {
                                id: startMinuteSpin
                                from: 0; to: 59; value: 0; stepSize: 5
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                                textFromValue: function(value) {
                                    return value < 10 ? "0" + value : "" + value
                                }
                            }
                        }
                    }
                }

                // End date / time
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "End Date"; font.pixelSize: 12; color: "#555" }
                        RowLayout {
                            spacing: 4
                            SpinBox {
                                id: endYearSpin
                                from: 2020; to: 2099; value: new Date().getFullYear()
                                editable: true
                                implicitWidth: 90
                                font.pixelSize: 12
                            }
                            Text { text: "-"; color: "#555"; font.pixelSize: 14 }
                            SpinBox {
                                id: endMonthSpin
                                from: 1; to: 12; value: new Date().getMonth() + 1
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                            }
                            Text { text: "-"; color: "#555"; font.pixelSize: 14 }
                            SpinBox {
                                id: endDaySpin
                                from: 1; to: 31; value: new Date().getDate()
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                            }
                        }
                    }

                    ColumnLayout {
                        visible: !allDaySwitch.checked
                        Text { text: "End Time"; font.pixelSize: 12; color: "#555" }
                        RowLayout {
                            spacing: 4
                            SpinBox {
                                id: endHourSpin
                                from: 0; to: 23; value: 10
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                                textFromValue: function(value) {
                                    return value < 10 ? "0" + value : "" + value
                                }
                            }
                            Text { text: ":"; color: "#555"; font.pixelSize: 14; font.bold: true }
                            SpinBox {
                                id: endMinuteSpin
                                from: 0; to: 59; value: 0; stepSize: 5
                                editable: true
                                implicitWidth: 70
                                font.pixelSize: 12
                                textFromValue: function(value) {
                                    return value < 10 ? "0" + value : "" + value
                                }
                            }
                        }
                    }
                }

                // Location
                Text { text: "Location"; font.pixelSize: 13; color: "#555" }
                TextField {
                    id: locationField
                    Layout.fillWidth: true
                    placeholderText: "Add location"
                    font.pixelSize: 14
                    background: Rectangle {
                        radius: 4; color: fieldBg; border.color: fieldBorder
                    }
                }

                // Description
                Text { text: "Description"; font.pixelSize: 13; color: "#555" }
                TextArea {
                    id: descField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 72
                    placeholderText: "Add description"
                    font.pixelSize: 14
                    wrapMode: TextArea.Wrap
                    background: Rectangle {
                        radius: 4; color: fieldBg; border.color: fieldBorder
                    }
                }

                // Reminder
                Text { text: "Reminder"; font.pixelSize: 13; color: "#555" }
                ComboBox {
                    id: reminderCombo
                    Layout.fillWidth: true
                    model: ["None", "15 minutes before", "30 minutes before", "1 hour before"]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        var values = [-1, 15, 30, 60]
                        reminderMinutes = values[currentIndex]
                    }
                }
            }
        }

        // ── Action buttons ─────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "white"
            border.color: "#eee"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: { root.close(); cancelClicked(); }
                    background: Rectangle {
                        radius: 6
                        color: parent.pressed ? Qt.darker(cancelBtnColor, 1.2)
                             : parent.hovered ? cancelBtnColor : "#eeeeee"
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14
                        color: "#555"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: mode === "edit" ? "Save Changes" : "Create Event"
                    enabled: titleField.text.trim().length > 0
                    onClicked: {
                        var data = {
                            id: eventId,
                            title: titleField.text,
                            description: descField.text,
                            location: locationField.text,
                            startDate: _dateStr(startYearSpin, startMonthSpin, startDaySpin),
                            startTime: _timeStr(startHourSpin, startMinuteSpin),
                            endDate: _dateStr(endYearSpin, endMonthSpin, endDaySpin),
                            endTime: _timeStr(endHourSpin, endMinuteSpin),
                            allDay: allDaySwitch.checked,
                            calendarId: calendarId,
                            reminderMinutes: reminderMinutes
                        };
                        saveClicked(data);
                        root.close();
                    }
                    background: Rectangle {
                        radius: 6
                        color: parent.enabled
                            ? (parent.pressed ? Qt.darker(saveBtnColor, 1.2)
                               : parent.hovered
                                 ? Qt.lighter(saveBtnColor, 1.1)
                                 : saveBtnColor)
                            : "#cccccc"
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14; font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }
}
