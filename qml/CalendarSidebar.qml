import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#f9f9f9"
    border.color: "#e0e0e0"
    border.width: 1

    // ── Theme ──────────────────────────────────────────────────────────────
    property color titleColor: "#212121"
    property color subtitleColor: "#757575"
    property color accentColor: "#2196F3"
    property color newBtnColor: "#4CAF50"

    // ── Data ───────────────────────────────────────────────────────────────
    property alias calendarModel: calendarList.model

    signal calendarToggled(string calendarId, bool visible)
    signal newCalendarRequested()
    signal calendarSelected(string calendarId)

    // ── Default calendars (mock) ───────────────────────────────────────────
    ListModel {
        id: defaultModel
        ListElement { calId: "personal"; calName: "Personal";
                      calColor: "#4CAF50"; calVisible: true }
        ListElement { calId: "work";     calName: "Work";
                      calColor: "#2196F3"; calVisible: true }
        ListElement { calId: "family";   calName: "Family";
                      calColor: "#FF9800"; calVisible: true }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // ── Header ─────────────────────────────────────────────────────────
        Text {
            text: "Calendars"
            font.pixelSize: 18
            font.bold: true
            color: titleColor
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        // ── Calendar list ──────────────────────────────────────────────────
        ListView {
            id: calendarList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: defaultModel
            spacing: 2
            clip: true

            delegate: Rectangle {
                width: calendarList.width
                height: 40
                radius: 6
                color: delegateMouse.containsMouse ? "#f0f0f0" : "transparent"

                MouseArea {
                    id: delegateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: calendarSelected(calId)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8

                    // Color indicator
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: calColor
                    }

                    // Calendar name
                    Text {
                        text: calName
                        font.pixelSize: 14
                        color: titleColor
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Visibility toggle
                    CheckBox {
                        checked: calVisible
                        onToggled: {
                            calVisible = checked;
                            calendarToggled(calId, checked);
                        }
                    }
                }
            }
        }

        // ── New Calendar button ────────────────────────────────────────────
        Button {
            Layout.fillWidth: true
            text: "+ New Calendar"
            onClicked: newCalendarRequested()

            background: Rectangle {
                radius: 6
                color: parent.pressed ? Qt.darker(newBtnColor, 1.2)
                     : parent.hovered ? Qt.lighter(newBtnColor, 1.1)
                     : newBtnColor
            }

            contentItem: Text {
                text: parent.text
                color: "white"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
