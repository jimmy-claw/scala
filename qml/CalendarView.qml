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
    property string viewMode: "month"   // "month", "week", or "day"
    property date weekStartDate: getMonday(new Date())
    property date dayViewDate: new Date()
    property bool searchActive: false
    property var searchResults: []

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
                calVisible: true,
                creatorId: c.creatorId || ""
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

    // ── Week view helpers ────────────────────────────────────────────────
    function getMonday(d) {
        var date = new Date(d)
        var day = date.getDay()
        var diff = (day === 0 ? -6 : 1) - day  // Monday = 1
        date.setDate(date.getDate() + diff)
        date.setHours(0, 0, 0, 0)
        return date
    }

    function weekDayDate(dayOffset) {
        var d = new Date(weekStartDate)
        d.setDate(d.getDate() + dayOffset)
        return d
    }

    function eventsForDate(d) {
        var result = []
        for (var i = 0; i < allEvents.length; i++) {
            var ev = allEvents[i]
            var evDate = new Date(ev.startTime)
            if (evDate.getDate() === d.getDate()
                && evDate.getMonth() === d.getMonth()
                && evDate.getFullYear() === d.getFullYear()) {
                result.push(ev)
            }
        }
        // Sort by start time
        result.sort(function(a, b) { return a.startTime - b.startTime })
        return result
    }

    function formatTime(ms) {
        var d = new Date(ms)
        var h = d.getHours()
        var m = d.getMinutes()
        return (h < 10 ? "0" : "") + h + ":" + (m < 10 ? "0" : "") + m
    }

    function goToPrevWeek() {
        var d = new Date(weekStartDate)
        d.setDate(d.getDate() - 7)
        weekStartDate = d
    }

    function goToNextWeek() {
        var d = new Date(weekStartDate)
        d.setDate(d.getDate() + 7)
        weekStartDate = d
    }

    function weekRangeLabel() {
        var start = weekStartDate
        var end = new Date(weekStartDate)
        end.setDate(end.getDate() + 6)
        var months = ["Jan","Feb","Mar","Apr","May","Jun",
                      "Jul","Aug","Sep","Oct","Nov","Dec"]
        if (start.getMonth() === end.getMonth()) {
            return months[start.getMonth()] + " " + start.getDate()
                   + "–" + end.getDate() + ", " + start.getFullYear()
        }
        return months[start.getMonth()] + " " + start.getDate()
               + " – " + months[end.getMonth()] + " " + end.getDate()
               + ", " + end.getFullYear()
    }

    // ── Day view helpers ──────────────────────────────────────────────
    function goToPrevDay() {
        var d = new Date(dayViewDate)
        d.setDate(d.getDate() - 1)
        dayViewDate = d
    }

    function goToNextDay() {
        var d = new Date(dayViewDate)
        d.setDate(d.getDate() + 1)
        dayViewDate = d
    }

    function goToToday() {
        dayViewDate = new Date()
    }

    function dayViewLabel() {
        var months = ["January","February","March","April","May","June",
                      "July","August","September","October","November","December"]
        var days = ["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"]
        return days[dayViewDate.getDay()] + ", " + months[dayViewDate.getMonth()] + " "
               + dayViewDate.getDate() + ", " + dayViewDate.getFullYear()
    }

    function isDayViewToday() {
        var now = new Date()
        return dayViewDate.getDate() === now.getDate()
            && dayViewDate.getMonth() === now.getMonth()
            && dayViewDate.getFullYear() === now.getFullYear()
    }

    function eventsForDayView() {
        return eventsForDate(dayViewDate)
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
        // Load settings
        if (typeof calendarModule !== "undefined") {
            var dv = calendarModule.getSetting("defaultView", "month")
            if (dv === "month" || dv === "week" || dv === "day")
                viewMode = dv
        }
    }

    // ── Ctrl+F shortcut ───────────────────────────────────────────────────
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: {
            searchActive = true
            searchField.forceActiveFocus()
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Sidebar ────────────────────────────────────────────────────────
        CalendarSidebar {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            calendarModel: sidebarCalendarModel
            currentIdentity: typeof calendarModule !== "undefined" ? calendarModule.getIdentity() : ""

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

                    // ── View mode toggle ──────────────────────────────
                    Row {
                        spacing: 0
                        Button {
                            text: "Month"
                            flat: true
                            onClicked: viewMode = "month"
                            background: Rectangle {
                                radius: 4
                                color: viewMode === "month" ? "#1976D2" : "transparent"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 13
                                font.bold: viewMode === "month"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Button {
                            text: "Week"
                            flat: true
                            onClicked: viewMode = "week"
                            background: Rectangle {
                                radius: 4
                                color: viewMode === "week" ? "#1976D2" : "transparent"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 13
                                font.bold: viewMode === "week"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Button {
                            text: "Day"
                            flat: true
                            onClicked: viewMode = "day"
                            background: Rectangle {
                                radius: 4
                                color: viewMode === "day" ? "#1976D2" : "transparent"
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "white"
                                font.pixelSize: 13
                                font.bold: viewMode === "day"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    Item { width: 12 }

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

                    Item { width: 4 }

                    // Search button
                    Button {
                        text: "\uD83D\uDD0D"
                        flat: true
                        onClicked: {
                            searchActive = !searchActive
                            if (searchActive) searchField.forceActiveFocus()
                            else { searchResults = []; searchField.text = "" }
                        }
                        implicitWidth: 36
                        implicitHeight: 36
                        ToolTip.visible: hovered
                        ToolTip.text: "Search events (Ctrl+F)"
                        background: Rectangle {
                            radius: 4
                            color: searchActive ? "#1976D2" : (parent.hovered ? "#1976D2" : "transparent")
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    TextField {
                        id: searchField
                        visible: searchActive
                        placeholderText: "Search events..."
                        implicitWidth: 180
                        onTextChanged: {
                            if (text.length >= 2)
                                searchResults = JSON.parse(calendarModule.searchEvents(text) || "[]")
                            else
                                searchResults = []
                        }
                        Keys.onEscapePressed: {
                            searchActive = false
                            searchResults = []
                            text = ""
                        }
                        background: Rectangle {
                            radius: 4
                            color: "white"
                            border.color: "#90CAF9"
                            border.width: 1
                        }
                    }

                    Item { width: 4 }

                    // Settings button
                    Button {
                        text: "\u2699"
                        flat: true
                        onClicked: settingsPanel.open()
                        implicitWidth: 36
                        implicitHeight: 36
                        ToolTip.visible: hovered
                        ToolTip.text: "Settings"
                        background: Rectangle {
                            radius: 4
                            color: parent.hovered ? "#1976D2" : "transparent"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            font.pixelSize: 18
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            // ── Search results overlay ──────────────────────────────────────
            ListView {
                id: searchResultsList
                visible: searchActive && searchResults.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(searchResults.length * 60, 300)
                clip: true
                model: searchResults
                z: 10
                delegate: Rectangle {
                    width: searchResultsList.width
                    height: 56
                    color: mouseArea.containsMouse ? "#e3f2fd" : (index % 2 === 0 ? "#ffffff" : "#fafafa")
                    border.color: "#e0e0e0"
                    border.width: index === searchResults.length - 1 ? 1 : 0

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            var ev = searchResults[index]
                            if (ev.startTime) {
                                var d = new Date(ev.startTime)
                                dayViewDate = d
                                viewMode = "day"
                            }
                            searchActive = false
                            searchResults = []
                            searchField.text = ""
                        }
                    }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10
                        anchors.verticalCenter: parent.verticalCenter

                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: modelData.calendarColor || "#2196F3"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                text: modelData.title || ""
                                font.pixelSize: 14
                                font.bold: true
                                color: "#333"
                            }
                            Text {
                                text: {
                                    var parts = []
                                    if (modelData.startTime) {
                                        var d = new Date(modelData.startTime)
                                        parts.push(d.toLocaleDateString())
                                    }
                                    if (modelData.calendarName)
                                        parts.push(modelData.calendarName)
                                    return parts.join(" \u2022 ")
                                }
                                font.pixelSize: 11
                                color: "#888"
                            }
                        }
                    }
                }
            }

            // ── Content: grid + optional details panel ─────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Calendar grid (month view)
                CalendarGrid {
                    id: calendarGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: viewMode === "month"
                    events: eventsForGrid()

                    onNavigated: {
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

                // ── Week view ─────────────────────────────────────────────
                Item {
                    id: weekView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: viewMode === "week"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Week navigation bar
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            color: "#ffffff"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8

                                Button {
                                    text: "<"
                                    flat: true
                                    onClicked: goToPrevWeek()
                                    implicitWidth: 36
                                    implicitHeight: 36
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: weekRangeLabel()
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#333333"
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: ">"
                                    flat: true
                                    onClicked: goToNextWeek()
                                    implicitWidth: 36
                                    implicitHeight: 36
                                }
                            }
                        }

                        // Day columns
                        Row {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Repeater {
                                model: 7
                                delegate: Rectangle {
                                    id: dayColumn
                                    width: weekView.width / 7
                                    height: weekView.height - 44
                                    border.color: "#e0e0e0"
                                    border.width: 1
                                    color: "#ffffff"

                                    property date columnDate: weekDayDate(index)
                                    property var dayEvents: eventsForDate(columnDate)
                                    property bool isToday: {
                                        var now = new Date()
                                        return columnDate.getDate() === now.getDate()
                                            && columnDate.getMonth() === now.getMonth()
                                            && columnDate.getFullYear() === now.getFullYear()
                                    }

                                    ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 0

                                        // Day header
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 48
                                            color: dayColumn.isToday ? "#e8f5e9" : "#e3f2fd"

                                            ColumnLayout {
                                                anchors.centerIn: parent
                                                spacing: 2

                                                Text {
                                                    text: ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"][index]
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    color: dayColumn.isToday ? "#2e7d32" : "#1565C0"
                                                    Layout.alignment: Qt.AlignHCenter
                                                }
                                                Text {
                                                    text: dayColumn.columnDate.getDate()
                                                    font.pixelSize: 16
                                                    font.bold: dayColumn.isToday
                                                    color: dayColumn.isToday ? "#2e7d32" : "#333"
                                                    Layout.alignment: Qt.AlignHCenter
                                                }
                                            }
                                        }

                                        // Events list
                                        Flickable {
                                            Layout.fillWidth: true
                                            Layout.fillHeight: true
                                            contentHeight: eventsColumn.height
                                            clip: true

                                            Column {
                                                id: eventsColumn
                                                width: parent.width
                                                spacing: 4
                                                topPadding: 4
                                                leftPadding: 2
                                                rightPadding: 2

                                                Repeater {
                                                    model: dayColumn.dayEvents

                                                    delegate: Rectangle {
                                                        width: eventsColumn.width - 4
                                                        height: eventContent.height + 8
                                                        radius: 4
                                                        color: {
                                                            var cal = findCalendar(modelData.calendarId)
                                                            return cal ? cal.color : "#2196F3"
                                                        }
                                                        opacity: eventMouse.containsMouse ? 0.85 : 1.0

                                                        MouseArea {
                                                            id: eventMouse
                                                            anchors.fill: parent
                                                            hoverEnabled: true
                                                            onClicked: {
                                                                var ev = modelData
                                                                var startDt = new Date(ev.startTime)
                                                                var endDt = new Date(ev.endTime)
                                                                var cal = findCalendar(ev.calendarId)
                                                                var pad = function(n) { return n < 10 ? "0" + n : "" + n }

                                                                eventDetails.loadEvent({
                                                                    id: ev.id,
                                                                    title: ev.title,
                                                                    description: ev.description || "",
                                                                    location: ev.location || "",
                                                                    startDate: startDt.getFullYear() + "-" + pad(startDt.getMonth()+1) + "-" + pad(startDt.getDate()),
                                                                    startTime: pad(startDt.getHours()) + ":" + pad(startDt.getMinutes()),
                                                                    endDate: endDt.getFullYear() + "-" + pad(endDt.getMonth()+1) + "-" + pad(endDt.getDate()),
                                                                    endTime: pad(endDt.getHours()) + ":" + pad(endDt.getMinutes()),
                                                                    allDay: ev.allDay || false,
                                                                    calendarName: cal ? cal.name : "",
                                                                    calendarColor: cal ? cal.color : "#2196F3"
                                                                });
                                                                showEventDetails = true
                                                            }
                                                        }

                                                        Column {
                                                            id: eventContent
                                                            anchors.left: parent.left
                                                            anchors.right: parent.right
                                                            anchors.top: parent.top
                                                            anchors.margins: 4
                                                            spacing: 1

                                                            Text {
                                                                width: parent.width
                                                                text: modelData.allDay ? "All day" : formatTime(modelData.startTime)
                                                                font.pixelSize: 9
                                                                color: "white"
                                                                opacity: 0.85
                                                                elide: Text.ElideRight
                                                            }
                                                            Text {
                                                                width: parent.width
                                                                text: modelData.title || "Untitled"
                                                                font.pixelSize: 11
                                                                font.bold: true
                                                                color: "white"
                                                                elide: Text.ElideRight
                                                                wrapMode: Text.Wrap
                                                                maximumLineCount: 2
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Day view ─────────────────────────────────────────────
                Item {
                    id: dayView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: viewMode === "day"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Day navigation bar
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            color: "#ffffff"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8

                                Button {
                                    text: "<"
                                    flat: true
                                    onClicked: goToPrevDay()
                                    implicitWidth: 36
                                    implicitHeight: 36
                                }

                                Button {
                                    text: "Today"
                                    flat: true
                                    onClicked: goToToday()
                                    enabled: !isDayViewToday()
                                    background: Rectangle {
                                        radius: 4
                                        color: parent.hovered ? "#e0e0e0" : "#f5f5f5"
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        font.pixelSize: 12
                                        color: parent.enabled ? "#333" : "#aaa"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                Text {
                                    text: dayViewLabel()
                                    font.pixelSize: 16
                                    font.bold: true
                                    color: "#333333"
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Item { Layout.fillWidth: true }

                                Button {
                                    text: ">"
                                    flat: true
                                    onClicked: goToNextDay()
                                    implicitWidth: 36
                                    implicitHeight: 36
                                }
                            }
                        }

                        // Time grid
                        Flickable {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            contentHeight: timeGridColumn.height
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds

                            Column {
                                id: timeGridColumn
                                width: parent.width

                                Repeater {
                                    model: 24  // hours 0–23
                                    delegate: Rectangle {
                                        id: hourRow
                                        width: timeGridColumn.width
                                        height: 60
                                        color: "#ffffff"
                                        border.color: "#f0f0f0"
                                        border.width: 1

                                        property int hour: index
                                        property var hourEvents: {
                                            var result = []
                                            var dayEvts = eventsForDayView()
                                            for (var i = 0; i < dayEvts.length; i++) {
                                                var ev = dayEvts[i]
                                                var evStart = new Date(ev.startTime)
                                                if (evStart.getHours() === hour) {
                                                    result.push(ev)
                                                }
                                            }
                                            return result
                                        }

                                        RowLayout {
                                            anchors.fill: parent
                                            spacing: 0

                                            // Hour label
                                            Rectangle {
                                                Layout.preferredWidth: 60
                                                Layout.fillHeight: true
                                                color: "transparent"

                                                Text {
                                                    anchors.top: parent.top
                                                    anchors.topMargin: 4
                                                    anchors.right: parent.right
                                                    anchors.rightMargin: 8
                                                    text: (hour < 10 ? "0" : "") + hour + ":00"
                                                    font.pixelSize: 11
                                                    color: "#999999"
                                                }
                                            }

                                            // Separator
                                            Rectangle {
                                                Layout.preferredWidth: 1
                                                Layout.fillHeight: true
                                                color: "#e0e0e0"
                                            }

                                            // Event area
                                            Item {
                                                Layout.fillWidth: true
                                                Layout.fillHeight: true

                                                // Clickable empty area
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        var pad = function(n) { return n < 10 ? "0" + n : "" + n }
                                                        var d = dayViewDate
                                                        var dateStr = d.getFullYear() + "-" + pad(d.getMonth() + 1) + "-" + pad(d.getDate())
                                                        var timeStr = pad(hourRow.hour) + ":00"
                                                        var endTimeStr = pad(hourRow.hour + 1 < 24 ? hourRow.hour + 1 : 23) + ":00"
                                                        eventModal.clear()
                                                        eventModal.calendars = calendarList
                                                        eventModal.startDate = dateStr
                                                        eventModal.startTime = timeStr
                                                        eventModal.endDate = dateStr
                                                        eventModal.endTime = endTimeStr
                                                        eventModal.open()
                                                    }
                                                }

                                                // Events as colored blocks
                                                Column {
                                                    anchors.fill: parent
                                                    anchors.margins: 2
                                                    spacing: 2

                                                    Repeater {
                                                        model: hourRow.hourEvents

                                                        delegate: Rectangle {
                                                            width: parent.width
                                                            height: {
                                                                var ev = modelData
                                                                var startMs = ev.startTime
                                                                var endMs = ev.endTime
                                                                var durationMin = (endMs - startMs) / 60000
                                                                return Math.max(24, Math.min(durationMin, 60))
                                                            }
                                                            radius: 4
                                                            color: {
                                                                var cal = findCalendar(modelData.calendarId)
                                                                return cal ? cal.color : "#2196F3"
                                                            }
                                                            opacity: evtMouse.containsMouse ? 0.85 : 1.0

                                                            MouseArea {
                                                                id: evtMouse
                                                                anchors.fill: parent
                                                                hoverEnabled: true
                                                                onClicked: {
                                                                    var ev = modelData
                                                                    var startDt = new Date(ev.startTime)
                                                                    var endDt = new Date(ev.endTime)
                                                                    var cal = findCalendar(ev.calendarId)
                                                                    var pad = function(n) { return n < 10 ? "0" + n : "" + n }

                                                                    eventDetails.loadEvent({
                                                                        id: ev.id,
                                                                        title: ev.title,
                                                                        description: ev.description || "",
                                                                        location: ev.location || "",
                                                                        startDate: startDt.getFullYear() + "-" + pad(startDt.getMonth()+1) + "-" + pad(startDt.getDate()),
                                                                        startTime: pad(startDt.getHours()) + ":" + pad(startDt.getMinutes()),
                                                                        endDate: endDt.getFullYear() + "-" + pad(endDt.getMonth()+1) + "-" + pad(endDt.getDate()),
                                                                        endTime: pad(endDt.getHours()) + ":" + pad(endDt.getMinutes()),
                                                                        allDay: ev.allDay || false,
                                                                        calendarName: cal ? cal.name : "",
                                                                        calendarColor: cal ? cal.color : "#2196F3"
                                                                    });
                                                                    showEventDetails = true
                                                                }
                                                            }

                                                            RowLayout {
                                                                anchors.fill: parent
                                                                anchors.margins: 4
                                                                spacing: 6

                                                                Text {
                                                                    text: formatTime(modelData.startTime) + "–" + formatTime(modelData.endTime)
                                                                    font.pixelSize: 10
                                                                    color: "white"
                                                                    opacity: 0.9
                                                                }
                                                                Text {
                                                                    Layout.fillWidth: true
                                                                    text: modelData.title || "Untitled"
                                                                    font.pixelSize: 12
                                                                    font.bold: true
                                                                    color: "white"
                                                                    elide: Text.ElideRight
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
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
                allDay: eventData.allDay,
                reminderMinutes: eventData.reminderMinutes !== undefined ? eventData.reminderMinutes : -1
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

    // ── Settings panel ───────────────────────────────────────────────────
    SettingsPanel {
        id: settingsPanel

        onSettingsSaved: {
            // Apply default view setting
            if (typeof calendarModule !== "undefined") {
                var dv = calendarModule.getSetting("defaultView", "month")
                if (dv === "month" || dv === "week" || dv === "day")
                    viewMode = dv
            }
        }
    }
}
