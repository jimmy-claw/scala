import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Logos.Theme
import Logos.Controls

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    width: Math.min(parent.width - 40, 420)
    height: Math.min(parent.height - 40, 480)
    padding: 0

    // ── Theme ──────────────────────────────────────────────────────────────
    property color headerColor: "#89b4fa"
    property color fieldBg: "#2a2a3c"
    property color fieldBorder: "#45475a"
    property color saveBtnColor: "#a6e3a1"

    signal settingsSaved()

    // Access backend through parent (CalendarView)
    readonly property var backend: (parent && parent.backend) ? parent.backend : null
    readonly property bool ready: (parent && parent.ready) ? parent.ready : false

    function loadSettings() {
        if (!root.ready || !backend) return
        // Load settings asynchronously via logos.watch
        logos.watch(backend.getSetting("defaultView", "month"),
            function(value) {
                if (value === "week") defaultViewCombo.currentIndex = 1
                else if (value === "day") defaultViewCombo.currentIndex = 2
                else defaultViewCombo.currentIndex = 0
            }
        )
        logos.watch(backend.getSetting("firstDayOfWeek", "monday"),
            function(value) { firstDayCombo.currentIndex = value === "sunday" ? 1 : 0 }
        )
        logos.watch(backend.getSetting("showDeclinedEvents", "false"),
            function(value) { showDeclinedSwitch.checked = value === "true" }
        )
        // Identity is a PROP — read directly, no async needed
        identityField.text = backend.currentIdentity || ""
    }

    function saveSettings() {
        if (!root.ready || !backend) return
        var views = ["month", "week", "day"]
        backend.setSetting("defaultView", views[defaultViewCombo.currentIndex])

        var days = ["monday", "sunday"]
        backend.setSetting("firstDayOfWeek", days[firstDayCombo.currentIndex])

        backend.setSetting("showDeclinedEvents",
            showDeclinedSwitch.checked ? "true" : "false")

        settingsSaved()
    }

    onOpened: loadSettings()

    background: Rectangle {
        radius: 8
        color: "white"
        border.color: "#45475a"
    }

    contentItem: ColumnLayout {
        spacing: 0

        // ── Header ─────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: headerColor
            radius: 8

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
                    text: "Settings"
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "X"
                    flat: true
                    onClicked: root.close()
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

        // ── Settings form ─────────────────────────────────────────────────
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: settingsColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: 16
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.top: parent.top
                anchors.topMargin: 16

                // Default view
                Text { text: "Default View"; font.pixelSize: 13; font.bold: true; color: "#cdd6f4" }
                ComboBox {
                    id: defaultViewCombo
                    Layout.fillWidth: true
                    model: ["Month", "Week", "Day"]
                    background: Rectangle {
                        radius: 4; color: fieldBg; border.color: fieldBorder
                    }
                }

                // Divider
                Rectangle { Layout.fillWidth: true; height: 1; color: "#45475a" }

                // First day of week
                Text { text: "First Day of Week"; font.pixelSize: 13; font.bold: true; color: "#cdd6f4" }
                ComboBox {
                    id: firstDayCombo
                    Layout.fillWidth: true
                    model: ["Monday", "Sunday"]
                    background: Rectangle {
                        radius: 4; color: fieldBg; border.color: fieldBorder
                    }
                }

                // Divider
                Rectangle { Layout.fillWidth: true; height: 1; color: "#45475a" }

                // Show declined events
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Text {
                        text: "Show Declined Events"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#cdd6f4"
                        Layout.fillWidth: true
                    }
                    Switch {
                        id: showDeclinedSwitch
                    }
                }

                // Divider
                Rectangle { Layout.fillWidth: true; height: 1; color: "#45475a" }

                // Identity
                Text { text: "Identity"; font.pixelSize: 13; font.bold: true; color: "#cdd6f4" }
                Text { text: "Your public key (read-only)"; font.pixelSize: 11; color: "#9399b2" }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: identityField
                        Layout.fillWidth: true
                        readOnly: true
                        font.pixelSize: 12
                        font.family: "monospace"
                        selectByMouse: true
                        background: Rectangle {
                            radius: 4; color: "#33334a"; border.color: fieldBorder
                        }
                    }

                    Button {
                        text: "Copy"
                        implicitWidth: 60
                        onClicked: {
                            identityField.selectAll()
                            identityField.copy()
                            identityField.deselect()
                            copyTooltip.visible = true
                            copyTimer.restart()
                        }
                        background: Rectangle {
                            radius: 4
                            color: parent.hovered ? "#2a2a3c" : fieldBg
                            border.color: fieldBorder
                        }
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 12
                            color: "#cdd6f4"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        ToolTip {
                            id: copyTooltip
                            text: "Copied!"
                            visible: false
                            delay: 0
                            timeout: 1500
                        }

                        Timer {
                            id: copyTimer
                            interval: 1500
                            onTriggered: copyTooltip.visible = false
                        }
                    }
                }

                Item { Layout.preferredHeight: 8 }
            }
        }

        // ── Action buttons ─────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "#1e1e2e"
            border.color: "#45475a"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: root.close()
                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? "#45475a" : "#33334a"
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14
                        color: "#9399b2"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Button {
                    text: "Save"
                    onClicked: {
                        saveSettings()
                        root.close()
                    }
                    background: Rectangle {
                        radius: 6
                        color: parent.pressed ? Qt.darker(saveBtnColor, 1.2)
                             : parent.hovered ? Qt.lighter(saveBtnColor, 1.1)
                             : saveBtnColor
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
