#!/usr/bin/env bash
# e2e test for scala_module — all --call flags in one logoscore session
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULES_DIR="${SCALA_MODULES_DIR:-$SCRIPT_DIR/../modules}"
LOGOSCORE="${LOGOSCORE:-$(for d in $(ls -d /nix/store/*logos-liblogos-bin-* 2>/dev/null | grep -v '.drv'); do
  test -f "$d/bin/logoscore" && echo "$d/bin/logoscore" && break
done)}"
TIMEOUT="${SCALA_E2E_TIMEOUT:-90}"

# Skip optional module lookups (messaging/accounts timeout ~20s each)
export SCALA_E2E_MINIMAL=1

if [[ -z "$LOGOSCORE" ]]; then
  echo "ERROR: logoscore not found. Set LOGOSCORE env var." >&2
  exit 1
fi

echo "=== scala_module e2e tests ==="
echo "logoscore: $LOGOSCORE"
echo "modules:   $MODULES_DIR"
echo ""

TMPLOG="$(mktemp /tmp/scala-e2e-XXXXXX.log)"
LSPID=""

cleanup() {
  [[ -n "$LSPID" ]] && kill "$LSPID" 2>/dev/null; wait "$LSPID" 2>/dev/null || true
  rm -f "$TMPLOG"
}
trap cleanup EXIT INT TERM

# Run all calls in one logoscore session
"$LOGOSCORE" \
  --modules-dir "$MODULES_DIR" \
  --load-modules kv_module,scala_module \
  --call 'scala_module.listCalendars()' \
  --call 'scala_module.createCalendar(E2ECalendar,#ff0000)' \
  --call 'scala_module.listCalendars()' \
  --call 'scala_module.getPendingReminders()' \
  > "$TMPLOG" 2>&1 &
LSPID=$!

EXPECTED=4

# Wait for all results (max TIMEOUT seconds)
for i in $(seq 1 "$TIMEOUT"); do
  COUNT=$(grep -c 'Method call successful' "$TMPLOG" 2>/dev/null || true)
  if [[ "$COUNT" -ge "$EXPECTED" ]]; then break; fi
  # Check if process died early
  if ! kill -0 "$LSPID" 2>/dev/null; then break; fi
  sleep 1
done

# Kill logoscore now that we have results
kill "$LSPID" 2>/dev/null
wait "$LSPID" 2>/dev/null || true
LSPID=""

# Extract results per call
ACTUAL=$(grep -c 'Method call successful' "$TMPLOG" 2>/dev/null || true)

PASS=0
FAIL=0

TESTS=(
  "listCalendars (initial)"
  "createCalendar E2ECalendar #ff0000"
  "listCalendars (after create)"
  "getPendingReminders"
)

for idx in "${!TESTS[@]}"; do
  name="${TESTS[$idx]}"
  needed=$((idx + 1))
  if [[ "$ACTUAL" -ge "$needed" ]]; then
    echo "  PASS: $name"
    PASS=$((PASS + 1))
  else
    echo "  FAIL: $name"
    FAIL=$((FAIL + 1))
  fi
done

echo ""
echo "=== Results: $PASS passed, $FAIL failed (of $EXPECTED) ==="

if [[ $FAIL -gt 0 ]]; then
  echo ""
  echo "--- logoscore output (last 30 lines) ---"
  tail -30 "$TMPLOG" 2>/dev/null || true
  exit 1
fi
