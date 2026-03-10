import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "white"
    border.color: "#e0e0e0"
    border.width: 1
    radius: 8

    // ── Theme ──────────────────────────────────────────────────────────────
    property color headerColor: "#2196F3"
    property color labelColor: "#757575"
    property color valueColor: "#212121"
    property color editBtnColor: "#2196F3"
    property color deleteBtnColor: "#f44336"

    // ── Event data ─────────────────────────────────────────────────────────
    property string eventId: ""
    property string eventTitle: ""
    property string eventDescription: ""
    property string eventLocation: ""
    property string eventStartDate: ""
    property string eventStartTime: ""
    property string eventEndDate: ""
    property string eventEndTime: ""
    property bool eventAllDay: false
    property string eventCalendarName: ""
    property color eventCalendarColor: "#2196F3"

    property bool confirmingDelete: false

    signal editRequested(string eventId)
    signal deleteConfirmed(string eventId)
    signal closeRequested()

    function loadEvent(ev) {
        eventId = ev.id || "";
        eventTitle = ev.title || "";
        eventDescription = ev.description || "";
        eventLocation = ev.location || "";
        eventStartDate = ev.startDate || "";
        eventStartTime = ev.startTime || "";
        eventEndDate = ev.endDate || "";
        eventEndTime = ev.endTime || "";
        eventAllDay = ev.allDay || false;
        eventCalendarName = ev.calendarName || "";
        eventCalendarColor = ev.calendarColor || "#2196F3";
        confirmingDelete = false;
    }

    visible: eventId !== ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: 0

        // ── Header ─────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: eventCalendarColor
            radius: 8

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 8
                color: eventCalendarColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12

                Text {
                    text: eventTitle
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Button {
                    text: "X"
                    flat: true
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: closeRequested()
                    contentItem: Text {
                        text: "X"; color: "white"; font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: "transparent" }
                }
            }
        }

        // ── Details body ───────────────────────────────────────────────────
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: detailsCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: detailsCol
                width: parent.width
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                anchors.top: parent.top
                anchors.topMargin: 16
                spacing: 14

                // Calendar
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 10; height: 10; radius: 5
                        color: eventCalendarColor
                    }
                    Text {
                        text: eventCalendarName
                        font.pixelSize: 13; color: labelColor
                    }
                }

                // Date & time
                ColumnLayout {
                    spacing: 4
                    Text { text: "When"; font.pixelSize: 12;
                           font.bold: true; color: labelColor }
                    Text {
                        text: {
                            var s = eventStartDate;
                            if (!eventAllDay && eventStartTime)
                                s += "  " + eventStartTime;
                            if (eventEndDate) {
                                s += "  \u2013  " + eventEndDate;
                                if (!eventAllDay && eventEndTime)
                                    s += "  " + eventEndTime;
                            }
                            if (eventAllDay) s += "  (All day)";
                            return s;
                        }
                        font.pixelSize: 14; color: valueColor
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                // Location
                ColumnLayout {
                    spacing: 4
                    visible: eventLocation !== ""
                    Text { text: "Location"; font.pixelSize: 12;
                           font.bold: true; color: labelColor }
                    Text {
                        text: eventLocation
                        font.pixelSize: 14; color: valueColor
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                // Description
                ColumnLayout {
                    spacing: 4
                    visible: eventDescription !== ""
                    Text { text: "Description"; font.pixelSize: 12;
                           font.bold: true; color: labelColor }
                    Text {
                        text: eventDescription
                        font.pixelSize: 14; color: valueColor
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // ── Action buttons ─────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "white"
            border.color: "#eee"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Button {
                    text: "Edit"
                    onClicked: editRequested(eventId)
                    background: Rectangle {
                        radius: 6
                        color: parent.pressed ? Qt.darker(editBtnColor, 1.2)
                             : parent.hovered
                               ? Qt.lighter(editBtnColor, 1.1)
                               : editBtnColor
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 13
                        color: "white"; font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true }

                // Delete with confirmation
                Button {
                    text: confirmingDelete ? "Confirm Delete" : "Delete"
                    onClicked: {
                        if (confirmingDelete) {
                            deleteConfirmed(eventId);
                            confirmingDelete = false;
                        } else {
                            confirmingDelete = true;
                        }
                    }
                    background: Rectangle {
                        radius: 6
                        color: {
                            if (confirmingDelete)
                                return parent.pressed
                                    ? Qt.darker(deleteBtnColor, 1.3)
                                    : deleteBtnColor;
                            return parent.pressed ? "#e0e0e0"
                                 : parent.hovered ? "#f5f5f5" : "#eeeeee";
                        }
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 13
                        color: confirmingDelete ? "white" : deleteBtnColor
                        font.bold: confirmingDelete
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }
}
