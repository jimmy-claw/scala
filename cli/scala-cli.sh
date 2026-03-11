#!/bin/bash
# Scala CLI — connects to a running logoscore instance via the C++ client

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCALA_CLI_BIN="${SCALA_CLI_BIN:-$SCRIPT_DIR/../build-module/scala_cli}"

if [ ! -x "$SCALA_CLI_BIN" ]; then
    echo "Error: scala_cli binary not found at $SCALA_CLI_BIN"
    echo "Build it with: make build-cli"
    exit 1
fi

case "$1" in
    list-calendars)   exec "$SCALA_CLI_BIN" listCalendars ;;
    list-events)      exec "$SCALA_CLI_BIN" listEvents "$2" ;;
    create-calendar)  exec "$SCALA_CLI_BIN" createCalendar "$2" "$3" ;;
    share)            exec "$SCALA_CLI_BIN" generateShareLink "$2" ;;
    join)             exec "$SCALA_CLI_BIN" handleShareLink "$2" ;;
    identity)         exec "$SCALA_CLI_BIN" getIdentity ;;
    help|--help|-h|"")
        echo "Usage: scala-cli <command> [args]"
        echo ""
        echo "Commands:"
        echo "  list-calendars           List all calendars"
        echo "  list-events <calId>      List events for a calendar"
        echo "  create-calendar <n> <c>  Create calendar (name, color)"
        echo "  share <calId>            Generate share link"
        echo "  join <link>              Join a shared calendar"
        echo "  identity                 Show current identity"
        echo ""
        echo "Requires logoscore running with scala_module (make run-module)."
        echo "You can also call the binary directly: $SCALA_CLI_BIN <method> [args...]"
        ;;
    *)
        # Pass through unknown commands directly to the binary
        exec "$SCALA_CLI_BIN" "$@"
        ;;
esac
