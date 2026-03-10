import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // ── Theme colors ───────────────────────────────────────────────────────
    property color bgColor: "#ffffff"
    property color headerBg: "#e3f2fd"
    property color headerText: "#1565C0"
    property color cellBorder: "#e0e0e0"
    property color todayBg: "#e8f5e9"
    property color todayText: "#2e7d32"
    property color selectedBg: "#e3f2fd"
    property color dayTextColor: "#333333"
    property color otherMonthText: "#bdbdbd"
    property color hoverBg: "#f5f5f5"

    // ── State ──────────────────────────────────────────────────────────────
    property int displayYear: new Date().getFullYear()
    property int displayMonth: new Date().getMonth()
    property int selectedDay: -1
    property var events: []       // list of {day: int, color: string}

    signal dayClicked(int day, int month, int year)
    signal navigated(int month, int year)

    // ── Helpers ────────────────────────────────────────────────────────────
    function daysInMonth(month, year) {
        return new Date(year, month + 1, 0).getDate();
    }
    function firstDayOfWeek(month, year) {
        return new Date(year, month, 1).getDay();
    }
    function monthName(month) {
        var names = ["January","February","March","April","May","June",
                     "July","August","September","October","November","December"];
        return names[month];
    }
    function isToday(day) {
        var now = new Date();
        return day === now.getDate()
            && displayMonth === now.getMonth()
            && displayYear === now.getFullYear();
    }
    function eventsForDay(day) {
        var result = [];
        for (var i = 0; i < events.length; i++) {
            if (events[i].day === day) result.push(events[i]);
        }
        return result;
    }

    function goToPrevMonth() {
        if (displayMonth === 0) {
            displayMonth = 11;
            displayYear--;
        } else {
            displayMonth--;
        }
        selectedDay = -1;
        navigated(displayMonth, displayYear);
    }
    function goToNextMonth() {
        if (displayMonth === 11) {
            displayMonth = 0;
            displayYear++;
        } else {
            displayMonth++;
        }
        selectedDay = -1;
        navigated(displayMonth, displayYear);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Month navigation toolbar ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: bgColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8

                Button {
                    text: "<"
                    flat: true
                    onClicked: goToPrevMonth()
                    implicitWidth: 36
                    implicitHeight: 36
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: monthName(displayMonth) + " " + displayYear
                    font.pixelSize: 16
                    font.bold: true
                    color: dayTextColor
                    horizontalAlignment: Text.AlignHCenter
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: ">"
                    flat: true
                    onClicked: goToNextMonth()
                    implicitWidth: 36
                    implicitHeight: 36
                }
            }
        }

        // ── Day-of-week headers ────────────────────────────────────────────
        Row {
            Layout.fillWidth: true
            Layout.preferredHeight: 28

            Repeater {
                model: ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"]
                delegate: Rectangle {
                    width: root.width / 7
                    height: 28
                    color: headerBg

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 11
                        font.bold: true
                        color: headerText
                    }
                }
            }
        }

        // ── Day cells grid (6 rows x 7 cols = 42 cells) ───────────────────
        Grid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 7

            Repeater {
                model: 42
                delegate: Rectangle {
                    id: dayCell
                    width: root.width / 7
                    height: (root.height - 44 - 28) / 6
                    border.color: cellBorder
                    border.width: 1

                    property int dayNumber: {
                        var offset = firstDayOfWeek(displayMonth, displayYear);
                        var d = index - offset + 1;
                        return d;
                    }
                    property bool isCurrentMonth: dayNumber >= 1
                        && dayNumber <= daysInMonth(displayMonth, displayYear)
                    property bool isTodayCell: isCurrentMonth && isToday(dayNumber)
                    property bool isSelected: isCurrentMonth
                        && dayNumber === selectedDay

                    color: {
                        if (!isCurrentMonth) return bgColor;
                        if (isSelected) return selectedBg;
                        if (isTodayCell) return todayBg;
                        if (cellMouse.containsMouse) return hoverBg;
                        return bgColor;
                    }

                    MouseArea {
                        id: cellMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (dayCell.isCurrentMonth) {
                                selectedDay = dayCell.dayNumber;
                                dayClicked(dayCell.dayNumber,
                                           displayMonth, displayYear);
                            }
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 2
                        spacing: 1

                        Text {
                            text: dayCell.isCurrentMonth
                                  ? dayCell.dayNumber.toString() : ""
                            font.pixelSize: 12
                            font.bold: dayCell.isTodayCell
                            color: {
                                if (dayCell.isTodayCell) return todayText;
                                if (!dayCell.isCurrentMonth)
                                    return otherMonthText;
                                return dayTextColor;
                            }
                        }

                        // Event indicators (colored dots)
                        Row {
                            spacing: 3
                            visible: dayCell.isCurrentMonth

                            Repeater {
                                model: dayCell.isCurrentMonth
                                       ? eventsForDay(dayCell.dayNumber) : []
                                delegate: Rectangle {
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: modelData.color || "#2196F3"
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
