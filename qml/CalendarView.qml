import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    width: 800
    height: 600

    // ── Theme colors ───────────────────────────────────────────────────────
    property color primaryColor: "#2196F3"
    property color bgColor: "#ffffff"
    property color toolbarColor: "#2196F3"
    property color sidebarBg: "#f9f9f9"
    property color newEventBtnColor: "#4CAF50"

    // ── State ──────────────────────────────────────────────────────────────
    property var selectedEvent: null
    property bool showEventDetails: false
    property var calendarList: []
    property var allEvents: []
    property string selectedCalendarId: ""

    // ── Preset colors for new calendar dialog ──────────────────────────────
    property var presetColors: [
        "#4CAF50", "#2196F3", "#FF9800", "#9C27B0",
        "#F44336", "#00BCD4", "#795548", "#607D8B"
    ]

    // ── Data helpers ───────────────────────────────────────────────────────
    function refreshCalendars() {
        var result = calendarModule.listCalendars()
        calendarList = JSON.parse(result || "[]")
        updateSidebarModel()
    }

    function refreshEvents() {
        allEvents = []
        for (var i = 0; i < calendarList.length; i++) {
            var evts = JSON.parse(calendarModule.listEvents(calendarList[i].id) || "[]")
            allEvents = allEvents.concat(evts)
        }
    }

    function updateSidebarModel() {
        sidebarCalendarModel.clear()
        for (var i = 0; i < calendarList.length; i++) {
            var c = calendarList[i]
            sidebarCalendarModel.append({
                calId: c.id,
                calName: c.name,
                calColor: c.color,
                calVisible: true
            })
        }
    }

    function findCalendar(calId) {
        for (var i = 0; i < calendarList.length; i++) {
            if (calendarList[i].id === calId) return calendarList[i]
        }
        return null
    }

    function eventsForGrid() {
        var dots = []
        var month = calendarGrid.displayMonth
        var year = calendarGrid.displayYear
        for (var i = 0; i < allEvents.length; i++) {
            var ev = allEvents[i]
            var d = new Date(ev.startTime)
            if (d.getMonth() === month && d.getFullYear() === year) {
                var cal = findCalendar(ev.calendarId)
                dots.push({ day: d.getDate(), color: cal ? cal.color : "#2196F3" })
            }
        }
        return dots
    }

    function msFromDateTime(dateStr, timeStr, fallbackNow) {
        if (!dateStr || dateStr === "") {
            return fallbackNow ? Date.now() : 0
        }
        var parts = dateStr.split("-")
        var y = parseInt(parts[0]), m = parseInt(parts[1]) - 1, day = parseInt(parts[2])
        var h = 0, min = 0
        if (timeStr && timeStr !== "") {
            var tp = timeStr.split(":")
            h = parseInt(tp[0])
            min = parseInt(tp[1])
        }
        return new Date(y, m, day, h, min).getTime()
    }

    // ── Sidebar calendar model ─────────────────────────────────────────────
    ListModel { id: sidebarCalendarModel }

    // ── Load data on startup ───────────────────────────────────────────────
    Component.onCompleted: {
        refreshCalendars()
        refreshEvents()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Sidebar ────────────────────────────────────────────────────────
        CalendarSidebar {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            calendarModel: sidebarCalendarModel

            onCalendarToggled: function(calId, vis) {
                console.log("Calendar toggled:", calId, vis);
            }
            onNewCalendarRequested: {
                newCalendarDialog.open()
            }
            onCalendarSelected: function(calId) {
                selectedCalendarId = calId
                console.log("Calendar selected:", calId);
            }
            onShareRequested: function(calId, calName) {
                var link = calendarModule.generateShareLink(calId)
                shareDialog.shareLink = link
                shareDialog.calendarName = calName
                shareDialog.open()
            }
        }

        // ── Main area ──────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Toolbar ────────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: toolbarColor

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

                    Text {
                        text: typeof calendarModule !== "undefined" && calendarModule.identity
                              ? "ID: " + calendarModule.identity.substring(0,8) + "..."
                              : ""
                        color: "#ccddee"
                        font.pixelSize: 11
                        visible: text !== ""
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "+ New Event"
                        onClicked: {
                            eventModal.clear();
                            eventModal.calendars = calendarList;
                            eventModal.open();
                        }
                        background: Rectangle {
                            radius: 6
                            color: parent.pressed
                                ? Qt.darker(newEventBtnColor, 1.2)
                                : parent.hovered
                                  ? Qt.lighter(newEventBtnColor, 1.1)
                                  : newEventBtnColor
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

            // ── Content: grid + optional details panel ─────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Calendar grid
                CalendarGrid {
                    id: calendarGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    events: eventsForGrid()

                    onNavigated: {
                        // Re-evaluate events binding when month changes
                        events = eventsForGrid()
                    }

                    onDayClicked: function(day, month, year) {
                        console.log("Day clicked:", day, month + 1, year);

                        var dayEvents = []
                        for (var i = 0; i < allEvents.length; i++) {
                            var ev = allEvents[i]
                            var d = new Date(ev.startTime)
                            if (d.getDate() === day && d.getMonth() === month && d.getFullYear() === year) {
                                dayEvents.push(ev)
                            }
                        }

                        if (dayEvents.length > 0) {
                            var first = dayEvents[0]
                            var startDt = new Date(first.startTime)
                            var endDt = new Date(first.endTime)
                            var cal = findCalendar(first.calendarId)

                            var pad = function(n) { return n < 10 ? "0" + n : "" + n }

                            eventDetails.loadEvent({
                                id: first.id,
                                title: first.title,
                                description: first.description || "",
                                location: first.location || "",
                                startDate: startDt.getFullYear() + "-" + pad(startDt.getMonth()+1) + "-" + pad(startDt.getDate()),
                                startTime: pad(startDt.getHours()) + ":" + pad(startDt.getMinutes()),
                                endDate: endDt.getFullYear() + "-" + pad(endDt.getMonth()+1) + "-" + pad(endDt.getDate()),
                                endTime: pad(endDt.getHours()) + ":" + pad(endDt.getMinutes()),
                                allDay: first.allDay || false,
                                calendarName: cal ? cal.name : "",
                                calendarColor: cal ? cal.color : "#2196F3"
                            });
                            showEventDetails = true;
                        } else {
                            showEventDetails = false;
                            eventDetails.eventId = "";
                        }
                    }
                }

                // Event details panel (right side)
                EventDetails {
                    id: eventDetails
                    Layout.preferredWidth: showEventDetails ? 280 : 0
                    Layout.fillHeight: true
                    visible: showEventDetails
                    clip: true

                    Behavior on Layout.preferredWidth {
                        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                    }

                    onEditRequested: function(evId) {
                        var evJson = calendarModule.getEvent(evId)
                        var ev = JSON.parse(evJson || "{}")
                        var startDt = new Date(ev.startTime)
                        var endDt = new Date(ev.endTime)
                        var pad = function(n) { return n < 10 ? "0" + n : "" + n }

                        eventModal.loadEvent({
                            id: ev.id,
                            title: ev.title,
                            description: ev.description || "",
                            location: ev.location || "",
                            startDate: startDt.getFullYear() + "-" + pad(startDt.getMonth()+1) + "-" + pad(startDt.getDate()),
                            startTime: pad(startDt.getHours()) + ":" + pad(startDt.getMinutes()),
                            endDate: endDt.getFullYear() + "-" + pad(endDt.getMonth()+1) + "-" + pad(endDt.getDate()),
                            endTime: pad(endDt.getHours()) + ":" + pad(endDt.getMinutes()),
                            allDay: ev.allDay || false,
                            calendarId: ev.calendarId || ""
                        });
                        eventModal.calendars = calendarList;
                        eventModal.open();
                    }

                    onDeleteConfirmed: function(evId) {
                        calendarModule.deleteEvent(evId)
                        refreshEvents()
                        calendarGrid.events = eventsForGrid()
                        showEventDetails = false;
                        eventDetails.eventId = "";
                    }

                    onCloseRequested: {
                        showEventDetails = false;
                        eventDetails.eventId = "";
                    }
                }
            }
        }
    }

    // ── Event modal (overlay) ──────────────────────────────────────────────
    EventModal {
        id: eventModal

        onSaveClicked: function(eventData) {
            var evJson = {
                title: eventData.title,
                description: eventData.description,
                location: eventData.location,
                startTime: msFromDateTime(eventData.startDate, eventData.startTime, true),
                endTime: msFromDateTime(eventData.endDate, eventData.endTime, true),
                allDay: eventData.allDay
            }

            if (eventData.id && eventData.id !== "") {
                evJson.id = eventData.id
                evJson.calendarId = eventData.calendarId
                calendarModule.updateEvent(JSON.stringify(evJson))
            } else {
                var calId = eventData.calendarId || selectedCalendarId
                    || (calendarList.length > 0 ? calendarList[0].id : "")
                evJson.calendarId = calId
                calendarModule.createEvent(calId, JSON.stringify(evJson))
            }
            refreshEvents()
            calendarGrid.events = eventsForGrid()
        }

        onCancelClicked: {
            console.log("Event creation cancelled");
        }
    }

    // ── New Calendar dialog ────────────────────────────────────────────────
    Popup {
        id: newCalendarDialog
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 340
        height: 280
        padding: 20
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property string selectedColor: presetColors[0]

        background: Rectangle { radius: 12; color: "white"; border.color: "#e0e0e0" }

        onOpened: {
            newCalNameField.text = ""
            selectedColor = presetColors[0]
            newCalNameField.forceActiveFocus()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text { text: "New Calendar"; font.pixelSize: 18; font.bold: true; color: "#212121" }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#e0e0e0" }

            Text { text: "Name"; font.pixelSize: 13; color: "#555" }
            TextField {
                id: newCalNameField
                Layout.fillWidth: true
                placeholderText: "Calendar name"
                font.pixelSize: 14
                background: Rectangle { radius: 4; color: "#f5f5f5"; border.color: "#e0e0e0" }
            }

            Text { text: "Color"; font.pixelSize: 13; color: "#555" }
            Row {
                spacing: 8
                Repeater {
                    model: presetColors
                    delegate: Rectangle {
                        width: 28; height: 28; radius: 14
                        color: modelData
                        border.width: newCalendarDialog.selectedColor === modelData ? 3 : 0
                        border.color: "#333"
                        MouseArea {
                            anchors.fill: parent
                            onClicked: newCalendarDialog.selectedColor = modelData
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                spacing: 8
                Item { Layout.fillWidth: true }
                Button {
                    text: "Cancel"
                    onClicked: newCalendarDialog.close()
                    background: Rectangle { radius: 6; color: parent.hovered ? "#eee" : "#f5f5f5" }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14; color: "#555"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Button {
                    text: "Create"
                    enabled: newCalNameField.text.trim().length > 0
                    onClicked: {
                        calendarModule.createCalendar(newCalNameField.text.trim(),
                                                      newCalendarDialog.selectedColor)
                        refreshCalendars()
                        refreshEvents()
                        calendarGrid.events = eventsForGrid()
                        newCalendarDialog.close()
                    }
                    background: Rectangle {
                        radius: 6
                        color: parent.enabled
                            ? (parent.hovered ? Qt.lighter("#4CAF50", 1.1) : "#4CAF50")
                            : "#ccc"
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14; font.bold: true; color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    // ── Share dialog ───────────────────────────────────────────────────────
    ShareDialog {
        id: shareDialog

        onJoinRequested: function(link) {
            var ok = calendarModule.handleShareLink(link)
            if (ok) {
                refreshCalendars()
                refreshEvents()
                calendarGrid.events = eventsForGrid()
                shareDialog.close()
            }
        }
    }
}
