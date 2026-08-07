import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Logos.Theme
import Logos.Controls

Item {
    id: root
    width: 800
    height: 600

    // ── Backend (typed replica, generated from scala_ui_backend.rep) ───────
    readonly property var backend: logos.module("scala_ui")
    property bool ready: false

    Connections {
        target: logos
        function onViewModuleReadyChanged(moduleName, isReady) {
            if (moduleName === "scala_ui")
                root.ready = isReady && root.backend !== null
        }
    }
    Component.onCompleted: {
        root.ready = root.backend !== null && logos.isViewModuleReady("scala_ui")
        if (root.ready) {
            _loadSettings()
            _refreshAll()
        }
    }

    // ── Theme colors (from Logos design system) ───────────────────────────
    readonly property color primaryColor: "#89b4fa"
    readonly property color bgColor: "#1e1e2e"
    readonly property color toolbarColor: "#2a2a3c"
    readonly property color sidebarBg: "#2a2a3c"
    readonly property color newEventBtnColor: "#89b4fa"

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
        "#a6e3a1", "#89b4fa", "#FF9800", "#9C27B0",
        "#F44336", "#00BCD4", "#795548", "#607D8B"
    ]

    // ── Data helpers (async via logos.watch) ───────────────────────────────
    function _refreshAll() {
        _refreshCalendars()
    }

    function _refreshCalendars() {
        if (!root.ready) return
        logos.watch(backend.listCalendars(),
            function(value) {
                calendarList = JSON.parse(value || "[]")
                updateSidebarModel()
                _refreshEvents()
            },
            function(error) { console.warn("[scala] listCalendars failed:", error) }
        )
    }

    function _refreshEvents() {
        if (!root.ready || calendarList.length === 0) return
        allEvents = []
        var remaining = calendarList.length
        for (var i = 0; i < calendarList.length; i++) {
            var calId = calendarList[i].id
            logos.watch(backend.listEvents(calId),
                function(value) {
                    var evts = JSON.parse(value || "[]")
                    allEvents = allEvents.concat(evts)
                    remaining--
                    if (remaining === 0) {
                        // All events loaded — update grid
                        calendarGrid.events = eventsForGrid()
                    }
                },
                function(error) {
                    console.warn("[scala] listEvents failed for", calId, error)
                    remaining--
                }
            )
        }
    }

    // Legacy wrappers (keep names for compatibility with existing code)
    function refreshCalendars() { _refreshCalendars() }
    function refreshEvents() { _refreshEvents() }

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
                dots.push({ day: d.getDate(), color: cal ? cal.color : "#89b4fa" })
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

    // ── Settings helpers ───────────────────────────────────────────────────
    function _loadSettings() {
        if (!root.ready) return
        logos.watch(backend.getSetting("defaultView", "month"),
            function(value) {
                if (value === "month" || value === "week" || value === "day")
                    viewMode = value
            },
            function(error) { console.warn("[scala] getSetting failed:", error) }
        )
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
            currentIdentity: (root.ready && root.backend) ? backend.currentIdentity : ""

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
                if (!root.ready) return
                logos.watch(backend.generateShareLink(calId),
                    function(value) {
                        shareDialog.shareLink = value
                        shareDialog.open()
                    },
                    function(error) { console.warn("[scala] generateShareLink failed:", error) }
                )
                shareDialog.calendarName = calName
                shareDialog.open()
            }
        }

        // ── Main area ──────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ── Toolbar (Logos design system) ──────────────────────────────
            LogosToolBar {
                Layout.fillWidth: true

                LogosText {
                    text: "Scala Calendar"
                    font.pixelSize: Theme.typography.headingSmall
                    font.weight: Theme.typography.weightBold
                    color: "#cdd6f4"
                }

                LogosText {
                    readonly property string ident: (root.ready && root.backend) ? backend.currentIdentity : ""
                    text: ident.length > 0 ? "ID: " + ident.substring(0,8) + "..." : ""
                    font.pixelSize: Theme.typography.caption
                    color: "#cdd6f4"Muted
                    visible: text !== ""
                }

                Item { Layout.fillWidth: true }

                // ── View mode toggle ──────────────────────────────
                LogosButton {
                    text: "Month"
                    onClicked: viewMode = "month"
                }

                LogosButton {
                    text: "Week"
                    onClicked: viewMode = "week"
                }

                LogosButton {
                    text: "Day"
                    onClicked: viewMode = "day"
                }

                LogosToolSeparator {}

                // ── New Event button ──────────────────────────────
                LogosButton {
                    text: "+ New Event"
                    onClicked: {
                        eventModal.clear();
                        eventModal.calendars = calendarList;
                        eventModal.open();
                    }
                }

                    Text {
                        readonly property string ident: (root.ready && root.backend) ? backend.currentIdentity : ""
                        text: ident.length > 0 ? "ID: " + ident.substring(0,8) + "..." : ""
                        color: "#ccddee"
                        font.pixelSize: 11
                        visible: text !== ""
                    }

                    // ── Search button + field ────────────────────────
                LogosIconButton {
                    onClicked: {
                        searchActive = !searchActive
                        if (searchActive) searchField.forceActiveFocus()
                        else { searchResults = []; searchField.text = "" }
                    }
                }

                LogosTextField {
                    id: searchField
                    visible: searchActive
                    placeholderText: "Search events..."
                    implicitWidth: 180
                    onTextChanged: {
                        if (text.length >= 2 && root.ready) {
                            logos.watch(backend.searchEvents(text),
                                function(value) { searchResults = JSON.parse(value || "[]") },
                                function(error) { console.warn("[scala] search failed:", error) }
                            )
                        } else if (text.length < 2) {
                            searchResults = []
                        }
                    }
                    Keys.onEscapePressed: {
                        searchActive = false
                        searchResults = []
                        text = ""
                    }
                }

                // ── Settings button ──────────────────────────────
                LogosIconButton {
                    onClicked: settingsPanel.open()
                }
            }

            // ── Search results overlay (Logos design system) ───────────────
            ListView {
                id: searchResultsList
                visible: searchActive && searchResults.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(searchResults.length * 60, 300)
                clip: true
                model: searchResults
                z: 10
                delegate: LogosItemDelegate {
                    width: searchResultsList.width
                    height: 56
                    highlighted: mouseArea.containsMouse
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

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacing.large
                        anchors.rightMargin: Theme.spacing.large
                        spacing: Theme.spacing.medium

                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: modelData.calendarColor || "#89b4fa"
                            Layout.alignment: Qt.AlignVCenter
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter

                            LogosText {
                                text: modelData.title || ""
                                font.pixelSize: Theme.typography.bodyText
                                font.weight: Theme.typography.weightMedium
                                color: "#cdd6f4"
                            }
                            LogosText {
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
                                font.pixelSize: Theme.typography.caption
                                color: "#cdd6f4"Muted
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
                                calendarColor: cal ? cal.color : "#89b4fa"
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

                        // Week navigation bar (Logos design system)
                        LogosFrame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacing.medium
                                anchors.rightMargin: Theme.spacing.medium

                                LogosIconButton {
                                    onClicked: goToPrevWeek()
                                }

                                Item { Layout.fillWidth: true }

                                LogosText {
                                    text: weekRangeLabel()
                                    font.pixelSize: Theme.typography.bodyText
                                    font.weight: Theme.typography.weightBold
                                    color: "#cdd6f4"
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Item { Layout.fillWidth: true }

                                LogosIconButton {
                                    onClicked: goToNextWeek()
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
                                    border.color: "#45475a"
                                    border.width: 1
                                    color: "#1e1e2e"

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
                                            color: dayColumn.isToday ? "#2a2a3c" : "#2a2a3c"

                                            ColumnLayout {
                                                anchors.centerIn: parent
                                                spacing: 2

                                                Text {
                                                    text: ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"][index]
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                    color: dayColumn.isToday ? "#a6e3a1" : "#89b4fa"
                                                    Layout.alignment: Qt.AlignHCenter
                                                }
                                                Text {
                                                    text: dayColumn.columnDate.getDate()
                                                    font.pixelSize: 16
                                                    font.bold: dayColumn.isToday
                                                    color: dayColumn.isToday ? "#a6e3a1" : "#cdd6f4"
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
                                                            return cal ? cal.color : "#89b4fa"
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
                                                                    calendarColor: cal ? cal.color : "#89b4fa"
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

                        // Day navigation bar (Logos design system)
                        LogosFrame {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Theme.spacing.medium
                                anchors.rightMargin: Theme.spacing.medium

                                LogosIconButton {
                                    onClicked: goToPrevDay()
                                }

                                LogosButton {
                                    text: "Today"
                                    onClicked: goToToday()
                                    enabled: !isDayViewToday()
                                }

                                Item { Layout.fillWidth: true }

                                LogosText {
                                    text: dayViewLabel()
                                    font.pixelSize: Theme.typography.bodyText
                                    font.weight: Theme.typography.weightBold
                                    color: "#cdd6f4"
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Item { Layout.fillWidth: true }

                                LogosIconButton {
                                    onClicked: goToNextDay()
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
                                        color: "#1e1e2e"
                                        border.color: "#33334a"
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
                                                    color: "#cdd6f4"Muted
                                                }
                                            }

                                            // Separator
                                            Rectangle {
                                                Layout.preferredWidth: 1
                                                Layout.fillHeight: true
                                                color: "#45475a"
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
                                                                return cal ? cal.color : "#89b4fa"
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
                                                                        calendarColor: cal ? cal.color : "#89b4fa"
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
                        if (!root.ready) return
                        logos.watch(backend.getEvent(evId),
                            function(value) {
                                _loadEventForEdit(JSON.parse(value || "{}"))
                            },
                            function(error) { console.warn("[scala] getEvent failed:", error) }
                        )
                    }

                    function _loadEventForEdit(ev) {
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
                        if (!root.ready) return
                        logos.watch(backend.deleteEvent(evId),
                            function(value) {
                                refreshEvents()
                                calendarGrid.events = eventsForGrid()
                                showEventDetails = false
                                eventDetails.eventId = ""
                            },
                            function(error) { console.warn("[scala] deleteEvent failed:", error) }
                        )

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
                if (!root.ready) return
                logos.watch(backend.updateEvent(JSON.stringify(evJson)),
                    function(value) { _onEventSaved() },
                    function(error) { console.warn("[scala] updateEvent failed:", error) }
                )
            } else {
                var calId = eventData.calendarId || selectedCalendarId
                    || (calendarList.length > 0 ? calendarList[0].id : "")
                evJson.calendarId = calId
                if (!root.ready) return
                logos.watch(backend.createEvent(calId, JSON.stringify(evJson)),
                    function(value) { _onEventSaved() },
                    function(error) { console.warn("[scala] createEvent failed:", error) }
                )
            }
        }

        // Helper: called after successful event save (create or update)
        function _onEventSaved() {
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

        background: Rectangle { radius: 12; color: "white"; border.color: "#45475a" }

        onOpened: {
            newCalNameField.text = ""
            selectedColor = presetColors[0]
            newCalNameField.forceActiveFocus()
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text { text: "New Calendar"; font.pixelSize: 18; font.bold: true; color: "#cdd6f4" }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#45475a" }

            Text { text: "Name"; font.pixelSize: 13; color: "#cdd6f4"Muted }
            TextField {
                id: newCalNameField
                Layout.fillWidth: true
                placeholderText: "Calendar name"
                font.pixelSize: 14
                background: Rectangle { radius: 4; color: "#33334a"; border.color: "#45475a" }
            }

            Text { text: "Color"; font.pixelSize: 13; color: "#cdd6f4"Muted }
            Row {
                spacing: 8
                Repeater {
                    model: presetColors
                    delegate: Rectangle {
                        width: 28; height: 28; radius: 14
                        color: modelData
                        border.width: newCalendarDialog.selectedColor === modelData ? 3 : 0
                        border.color: "#cdd6f4"
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
                    background: Rectangle { radius: 6; color: parent.hovered ? "#45475a" : "#33334a" }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14; color: "#cdd6f4"Muted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                Button {
                    text: "Create"
                    enabled: newCalNameField.text.trim().length > 0
                    onClicked: {
                        if (!root.ready) return
                        var name = newCalNameField.text.trim()
                        var color = newCalendarDialog.selectedColor
                        logos.watch(backend.createCalendar(name, color),
                            function(value) {
                                refreshCalendars()
                                refreshEvents()
                                calendarGrid.events = eventsForGrid()
                                newCalendarDialog.close()
                            },
                            function(error) { console.warn("[scala] createCalendar failed:", error) }
                        )
                    }
                    background: Rectangle {
                        radius: 6
                        color: parent.enabled
                            ? (parent.hovered ? Qt.lighter("#a6e3a1", 1.1) : "#a6e3a1")
                            : "#cdd6f4"Muted
                    }
                    contentItem: Text {
                        text: parent.text; font.pixelSize: 14; font.bold: true; color: "#1e1e2e"
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
            if (!root.ready) return
            logos.watch(backend.handleShareLink(link),
                function(value) {
                    if (value) {
                        refreshCalendars()
                        refreshEvents()
                        calendarGrid.events = eventsForGrid()
                        shareDialog.close()
                    }
                },
                function(error) { console.warn("[scala] handleShareLink failed:", error) }
            )
    }
    }

    // ── Settings panel ───────────────────────────────────────────────────
    SettingsPanel {
        id: settingsPanel

        onSettingsSaved: {
            // Apply default view setting
            _loadSettings()
        }
    }
}
}
