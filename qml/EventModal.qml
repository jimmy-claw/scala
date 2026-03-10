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
    property string calendarId: ""
    property string eventId: ""
    property var calendars: []

    signal saveClicked(var eventData)
    signal cancelClicked()

    function clear() {
        titleField.text = "";
        descField.text = "";
        locationField.text = "";
        startDateField.text = "";
        startTimeField.text = "";
        endDateField.text = "";
        endTimeField.text = "";
        allDaySwitch.checked = false;
        mode = "create";
        eventId = "";
        calendarId = "";
        calendarCombo.currentIndex = 0;
    }

    function loadEvent(ev) {
        mode = "edit";
        eventId = ev.id || "";
        titleField.text = ev.title || "";
        descField.text = ev.description || "";
        locationField.text = ev.location || "";
        startDateField.text = ev.startDate || "";
        startTimeField.text = ev.startTime || "";
        endDateField.text = ev.endDate || "";
        endTimeField.text = ev.endTime || "";
        allDaySwitch.checked = ev.allDay || false;
        calendarId = ev.calendarId || "";
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
                        Text { text: "Start Date"; font.pixelSize: 12;
                               color: "#555" }
                        TextField {
                            id: startDateField
                            Layout.fillWidth: true
                            placeholderText: "YYYY-MM-DD"
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 4; color: fieldBg;
                                border.color: fieldBorder
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: !allDaySwitch.checked
                        Text { text: "Start Time"; font.pixelSize: 12;
                               color: "#555" }
                        TextField {
                            id: startTimeField
                            Layout.fillWidth: true
                            placeholderText: "HH:MM"
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 4; color: fieldBg;
                                border.color: fieldBorder
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
                        Text { text: "End Date"; font.pixelSize: 12;
                               color: "#555" }
                        TextField {
                            id: endDateField
                            Layout.fillWidth: true
                            placeholderText: "YYYY-MM-DD"
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 4; color: fieldBg;
                                border.color: fieldBorder
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: !allDaySwitch.checked
                        Text { text: "End Time"; font.pixelSize: 12;
                               color: "#555" }
                        TextField {
                            id: endTimeField
                            Layout.fillWidth: true
                            placeholderText: "HH:MM"
                            font.pixelSize: 13
                            background: Rectangle {
                                radius: 4; color: fieldBg;
                                border.color: fieldBorder
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
                            startDate: startDateField.text,
                            startTime: startTimeField.text,
                            endDate: endDateField.text,
                            endTime: endTimeField.text,
                            allDay: allDaySwitch.checked,
                            calendarId: calendarId
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
