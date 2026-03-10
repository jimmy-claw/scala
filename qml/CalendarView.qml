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

    // ── Mock events for demo ───────────────────────────────────────────────
    property var mockEvents: [
        { day: 5,  color: "#4CAF50" },
        { day: 5,  color: "#2196F3" },
        { day: 12, color: "#4CAF50" },
        { day: 18, color: "#FF9800" },
        { day: 22, color: "#2196F3" },
        { day: 22, color: "#FF9800" },
        { day: 28, color: "#4CAF50" }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Sidebar ────────────────────────────────────────────────────────
        CalendarSidebar {
            Layout.preferredWidth: 220
            Layout.fillHeight: true

            onCalendarToggled: function(calId, vis) {
                console.log("Calendar toggled:", calId, vis);
            }
            onNewCalendarRequested: {
                console.log("New calendar requested");
            }
            onCalendarSelected: function(calId) {
                console.log("Calendar selected:", calId);
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

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "+ New Event"
                        onClicked: {
                            eventModal.clear();
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
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    events: mockEvents

                    onDayClicked: function(day, month, year) {
                        console.log("Day clicked:", day, month + 1, year);

                        // Demo: show event details for days with events
                        var dayEvents = eventsForDay(day);
                        if (dayEvents.length > 0) {
                            eventDetails.loadEvent({
                                id: "evt-" + day,
                                title: "Sample Event on " + day,
                                description: "This is a sample event.",
                                location: "Conference Room A",
                                startDate: year + "-" + (month+1) + "-" + day,
                                startTime: "09:00",
                                endDate: year + "-" + (month+1) + "-" + day,
                                endTime: "10:00",
                                allDay: false,
                                calendarName: "Personal",
                                calendarColor: dayEvents[0].color
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
                        eventModal.loadEvent({
                            id: evId,
                            title: eventDetails.eventTitle,
                            description: eventDetails.eventDescription,
                            location: eventDetails.eventLocation,
                            startDate: eventDetails.eventStartDate,
                            startTime: eventDetails.eventStartTime,
                            endDate: eventDetails.eventEndDate,
                            endTime: eventDetails.eventEndTime,
                            allDay: eventDetails.eventAllDay,
                            calendarId: ""
                        });
                        eventModal.open();
                    }

                    onDeleteConfirmed: function(evId) {
                        console.log("Delete event:", evId);
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
            console.log("Event saved:", JSON.stringify(eventData));
        }

        onCancelClicked: {
            console.log("Event creation cancelled");
        }
    }
}
