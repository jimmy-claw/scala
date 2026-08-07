import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Logos.Theme
import Logos.Controls

Popup {
    id: shareDialog
    modal: true
    anchors.centerIn: parent
    width: 420
    height: 480
    padding: 20

    // ── Properties ──────────────────────────────────────────────────────────
    property string shareLink: ""
    property string qrDataUrl: ""
    property string calendarName: ""

    signal joinRequested(string link)

    background: Rectangle {
        radius: 12
        color: "#1e1e2e"
        border.color: "#45475a"
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // ── Title ───────────────────────────────────────────────────────────
        Text {
            text: "Share Calendar"
            font.pixelSize: 18
            font.bold: true
            color: "#cdd6f4"
            Layout.fillWidth: true
        }

        Text {
            text: calendarName
            font.pixelSize: 14
            color: "#9399b2"
            Layout.fillWidth: true
            visible: calendarName.length > 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#45475a"
        }

        // ── Tab bar ─────────────────────────────────────────────────────────
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                text: "Share"
                width: implicitWidth
            }
            TabButton {
                text: "Join"
                width: implicitWidth
            }
        }

        // ── Tab content ─────────────────────────────────────────────────────
        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            // ── Share tab ───────────────────────────────────────────────────
            ColumnLayout {
                spacing: 12

                Text {
                    text: "Share this link to invite others:"
                    font.pixelSize: 13
                    color: "#cdd6f4"
                }

                // Link text field (read-only, selectable)
                TextField {
                    id: linkField
                    Layout.fillWidth: true
                    text: shareLink
                    readOnly: true
                    selectByMouse: true
                    wrapMode: TextInput.WrapAnywhere
                    font.pixelSize: 12
                    font.family: "monospace"

                    background: Rectangle {
                        radius: 6
                        color: "#2a2a3c"
                        border.color: "#45475a"
                        border.width: 1
                    }
                }

                // Copy Link button
                Button {
                    Layout.fillWidth: true
                    text: copyTimer.running ? "Copied!" : "Copy Link"

                    onClicked: {
                        linkField.selectAll()
                        linkField.copy()
                        linkField.deselect()
                        copyTimer.start()
                    }

                    Timer {
                        id: copyTimer
                        interval: 2000
                    }

                    background: Rectangle {
                        radius: 6
                        color: parent.pressed ? "#89b4fa" : "#72a0e8"
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "#1e1e2e"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // QR code placeholder
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    width: 160
                    height: 160
                    source: qrDataUrl
                    visible: qrDataUrl.length > 0
                    fillMode: Image.PreserveAspectFit
                }

                // Fallback text when no QR data URL
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "(QR code placeholder)"
                    font.pixelSize: 11
                    color: "#9399b2"
                    visible: qrDataUrl.length === 0
                }

                Item { Layout.fillHeight: true }
            }

            // ── Join tab ────────────────────────────────────────────────────
            ColumnLayout {
                spacing: 12

                Text {
                    text: "Paste a share link to join a calendar:"
                    font.pixelSize: 13
                    color: "#cdd6f4"
                }

                TextField {
                    id: joinLinkField
                    Layout.fillWidth: true
                    placeholderText: "scala://join?id=...&key=...&name=..."
                    selectByMouse: true
                    font.pixelSize: 12
                    font.family: "monospace"

                    background: Rectangle {
                        radius: 6
                        color: "#1e1e2e"
                        border.color: joinLinkField.activeFocus ? "#89b4fa" : "#45475a"
                        border.width: 1
                    }
                }

                Text {
                    id: joinError
                    Layout.fillWidth: true
                    color: "#f38ba8"
                    font.pixelSize: 12
                    visible: text.length > 0
                }

                Button {
                    Layout.fillWidth: true
                    text: "Join Calendar"
                    enabled: joinLinkField.text.length > 0

                    onClicked: {
                        joinError.text = ""
                        joinRequested(joinLinkField.text)
                    }

                    background: Rectangle {
                        radius: 6
                        color: !parent.enabled ? "#9399b2"
                             : parent.pressed ? "#f38ba8"
                             : parent.hovered ? "#8bc987"
                             : "#a6e3a1"
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "#1e1e2e"
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // ── Close button ────────────────────────────────────────────────────
        Button {
            Layout.fillWidth: true
            text: "Close"
            flat: true
            onClicked: shareDialog.close()

            contentItem: Text {
                text: parent.text
                color: "#9399b2"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
