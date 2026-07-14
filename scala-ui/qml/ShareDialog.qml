import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
        color: "white"
        border.color: "#e0e0e0"
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
            color: "#212121"
            Layout.fillWidth: true
        }

        Text {
            text: calendarName
            font.pixelSize: 14
            color: "#757575"
            Layout.fillWidth: true
            visible: calendarName.length > 0
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
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
                    color: "#424242"
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
                        color: "#f5f5f5"
                        border.color: "#e0e0e0"
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
                        color: parent.pressed ? "#1976D2"
                             : parent.hovered ? "#42A5F5"
                             : "#2196F3"
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "white"
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
                    color: "#bdbdbd"
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
                    color: "#424242"
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
                        color: "white"
                        border.color: joinLinkField.activeFocus ? "#2196F3" : "#e0e0e0"
                        border.width: 1
                    }
                }

                Text {
                    id: joinError
                    Layout.fillWidth: true
                    color: "#D32F2F"
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
                        color: !parent.enabled ? "#BDBDBD"
                             : parent.pressed ? "#388E3C"
                             : parent.hovered ? "#66BB6A"
                             : "#4CAF50"
                    }

                    contentItem: Text {
                        text: parent.text
                        color: "white"
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
                color: "#757575"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
