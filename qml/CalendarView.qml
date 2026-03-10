import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 800
    height: 600

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Calendar list sidebar ────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 200
            Layout.fillHeight: true
            color: "#f5f5f5"
            border.color: "#ddd"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Text {
                    text: "Calendars"
                    font.pixelSize: 18
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "No calendars yet"
                        color: "#999"
                        font.pixelSize: 13
                    }
                }
            }
        }

        // ── Main calendar area ───────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                color: "#2196F3"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    Text {
                        text: "Scala Calendar"
                        color: "white"
                        font.pixelSize: 20
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Add Event"
                        onClicked: {
                            // No-op placeholder
                            console.log("Add Event clicked (not yet implemented)")
                        }
                    }
                }
            }

            // Month grid
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "white"

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    columns: 7
                    rowSpacing: 1
                    columnSpacing: 1

                    // Day-of-week headers
                    Repeater {
                        model: ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: "#e3f2fd"

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.bold: true
                                font.pixelSize: 12
                                color: "#1565C0"
                            }
                        }
                    }

                    // Static 5x7 day cells (placeholder)
                    Repeater {
                        model: 35
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            border.color: "#eee"
                            color: "white"

                            Text {
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.margins: 4
                                text: {
                                    var day = index - 5 + 1  // offset for month start
                                    return (day >= 1 && day <= 31) ? day.toString() : ""
                                }
                                font.pixelSize: 11
                                color: "#333"
                            }
                        }
                    }
                }
            }
        }
    }
}
