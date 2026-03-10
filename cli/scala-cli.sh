#!/bin/bash
# Scala CLI — wrapper around logoscore --call for headless use

LOGOSCORE="${SCALA_LOGOSCORE:-$(which logoscore 2>/dev/null)}"
MODULES_DIR="${SCALA_MODULES_DIR:-$HOME/.local/share/logos/modules}"
NAMESPACE="${SCALA_NAMESPACE:-default}"

if [ -z "$LOGOSCORE" ]; then
    # Try nix store
    LOGOSCORE=$(ls -d /nix/store/*logos-liblogos-bin-*/bin/logoscore 2>/dev/null | head -1)
fi

if [ -z "$LOGOSCORE" ]; then
    echo "Error: logoscore not found. Set SCALA_LOGOSCORE or install logos-liblogos."
    exit 1
fi

call() {
    "$LOGOSCORE" --modules-dir "$MODULES_DIR" --load-modules kv_module,scala_module \
        --call "scala_module.$1" 2>/dev/null | grep -v '^Debug'
}

case "$1" in
    list-calendars)   call "listCalendars()" ;;
    list-events)      call "listEvents($2)" ;;
    create-calendar)  call "createCalendar($2,$3)" ;;
    share)            call "generateShareLink($2)" ;;
    join)             call "handleShareLink($2)" ;;
    identity)         call "getIdentity()" ;;
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
        ;;
    *)
        echo "Unknown command: $1. Run 'scala-cli help' for usage."
        exit 1
        ;;
esac
