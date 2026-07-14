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
    property var calendarModel: null
    property string currentIdentity: ""

    signal calendarToggled(string calendarId, bool visible)
    signal newCalendarRequested()
    signal calendarSelected(string calendarId)
    signal shareRequested(string calendarId, string calendarName)

    // ── Filter helpers ───────────────────────────────────────────────────
    function myCalendars() {
        var result = []
        if (!calendarModel) return result
        for (var i = 0; i < calendarModel.count; i++) {
            var item = calendarModel.get(i)
            if (item.creatorId === currentIdentity && currentIdentity !== "") {
                result.push(item)
            }
        }
        return result
    }

    function importedCalendars() {
        var result = []
        if (!calendarModel) return result
        for (var i = 0; i < calendarModel.count; i++) {
            var item = calendarModel.get(i)
            if (item.creatorId !== currentIdentity || currentIdentity === "") {
                result.push(item)
            }
        }
        return result
    }

    // ── Default calendars (mock) ───────────────────────────────────────────
    ListModel {
        id: defaultModel
        ListElement { calId: "personal"; calName: "Personal";
                      calColor: "#4CAF50"; calVisible: true; creatorId: "" }
        ListElement { calId: "work";     calName: "Work";
                      calColor: "#2196F3"; calVisible: true; creatorId: "" }
        ListElement { calId: "family";   calName: "Family";
                      calColor: "#FF9800"; calVisible: true; creatorId: "" }
    }

    // Use defaultModel as fallback when no calendarModel is set
    Component.onCompleted: {
        if (!calendarModel) calendarModel = defaultModel
    }

    // ── Calendar delegate component ──────────────────────────────────────
    Component {
        id: calendarDelegate

        Rectangle {
            width: parent ? parent.width : 200
            height: 40
            radius: 6
            color: delMouse.containsMouse ? "#f0f0f0" : "transparent"

            property string itemCalId
            property string itemCalName
            property string itemCalColor
            property bool itemCalVisible

            MouseArea {
                id: delMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: calendarSelected(itemCalId)
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
                    color: itemCalColor
                }

                // Calendar name
                Text {
                    text: itemCalName
                    font.pixelSize: 14
                    color: titleColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Share button
                Button {
                    implicitWidth: 28
                    implicitHeight: 28
                    flat: true
                    text: "\u21AA"
                    font.pixelSize: 14
                    ToolTip.visible: hovered
                    ToolTip.text: "Share"
                    onClicked: shareRequested(itemCalId, itemCalName)

                    background: Rectangle {
                        radius: 4
                        color: parent.hovered ? "#e3f2fd" : "transparent"
                    }
                }

                // Visibility toggle
                CheckBox {
                    checked: itemCalVisible
                    onToggled: {
                        calendarToggled(itemCalId, checked)
                    }
                }
            }
        }
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

        // ── Scrollable calendar sections ─────────────────────────────────
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: sectionsColumn.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: sectionsColumn
                width: parent.width
                spacing: 8

                // ── My Calendars section ─────────────────────────────────
                Column {
                    width: parent.width
                    spacing: 2

                    Text {
                        text: "My Calendars"
                        font.pixelSize: 12
                        font.bold: true
                        color: subtitleColor
                        leftPadding: 4
                        topPadding: 4
                        bottomPadding: 4
                    }

                    Repeater {
                        model: myCalendars()

                        delegate: Loader {
                            width: sectionsColumn.width
                            sourceComponent: calendarDelegate
                            onLoaded: {
                                item.itemCalId = modelData.calId
                                item.itemCalName = modelData.calName
                                item.itemCalColor = modelData.calColor
                                item.itemCalVisible = modelData.calVisible
                            }
                        }
                    }

                    // Empty state
                    Text {
                        visible: myCalendars().length === 0
                        text: "No calendars yet"
                        font.pixelSize: 12
                        font.italic: true
                        color: "#aaa"
                        leftPadding: 8
                        topPadding: 4
                    }
                }

                // ── Divider ──────────────────────────────────────────────
                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#e0e0e0"
                    visible: importedCalendars().length > 0
                }

                // ── Imported section ─────────────────────────────────────
                Column {
                    width: parent.width
                    spacing: 2
                    visible: importedCalendars().length > 0

                    Text {
                        text: "Imported"
                        font.pixelSize: 12
                        font.bold: true
                        color: subtitleColor
                        leftPadding: 4
                        topPadding: 4
                        bottomPadding: 4
                    }

                    Repeater {
                        model: importedCalendars()

                        delegate: Loader {
                            width: sectionsColumn.width
                            sourceComponent: calendarDelegate
                            onLoaded: {
                                item.itemCalId = modelData.calId
                                item.itemCalName = modelData.calName
                                item.itemCalColor = modelData.calColor
                                item.itemCalVisible = modelData.calVisible
                            }
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
