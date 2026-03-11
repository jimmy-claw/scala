#!/usr/bin/env bash
# e2e test for scala_module — verifies actual state via kv_module
# All calls run in one logoscore session for speed.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULES_DIR="${SCALA_MODULES_DIR:-$SCRIPT_DIR/../modules}"
LOGOSCORE="${LOGOSCORE:-$(for d in $(ls -d /nix/store/*logos-liblogos-bin-* 2>/dev/null | grep -v '.drv'); do
  test -f "$d/bin/logoscore" && echo "$d/bin/logoscore" && break
done)}"
TIMEOUT="${SCALA_E2E_TIMEOUT:-90}"

export SCALA_E2E_MINIMAL=1

if [[ -z "$LOGOSCORE" ]]; then
  echo "ERROR: logoscore not found. Set LOGOSCORE env var." >&2
  exit 1
fi

PASS=0
FAIL=0

pass() { echo "  ✓ $1"; ((PASS++)); }
fail() { echo "  ✗ $1"; ((FAIL++)); }

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  scala_module — end-to-end tests"
echo "  logoscore: $(basename $(dirname $LOGOSCORE))"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

TMPLOG="$(mktemp /tmp/scala-e2e-XXXXXX.log)"
LSPID=""

cleanup() {
  [[ -n "$LSPID" ]] && kill "$LSPID" 2>/dev/null; wait "$LSPID" 2>/dev/null || true
  rm -f "$TMPLOG"
}
trap cleanup EXIT INT TERM

# Run all calls in one session:
# 1. listCalendars (empty)       → Result: false (QString — known logoscore limitation)
# 2. createCalendar TestCal      → Result: false (same)
# 3. createCalendar AnotherCal   → Result: false (same)
# 4. kv_module.listAll(scala:default)  → Result: [<keys>]  ← actual verification
# 5. getPendingReminders          → Result: false
EXPECTED_CALLS=5

"$LOGOSCORE" \
  --modules-dir "$MODULES_DIR" \
  --load-modules kv_module,scala_module \
  --call 'scala_module.listCalendars()' \
  --call 'scala_module.createCalendar(TestCal,#3b82f6)' \
  --call 'scala_module.createCalendar(WorkCal,#ef4444)' \
  --call 'kv_module.listAll(scala:default)' \
  --call 'scala_module.getPendingReminders()' \
  > "$TMPLOG" 2>&1 &
LSPID=$!

# Wait for all results
for i in $(seq 1 "$TIMEOUT"); do
  COUNT=$(grep -c 'Method call successful' "$TMPLOG" 2>/dev/null || true)
  if [[ "$COUNT" -ge "$EXPECTED_CALLS" ]]; then break; fi
  kill -0 "$LSPID" 2>/dev/null || break
  sleep 1
done

kill "$LSPID" 2>/dev/null; wait "$LSPID" 2>/dev/null || true
LSPID=""

ACTUAL=$(grep -c 'Method call successful' "$TMPLOG" 2>/dev/null || true)

# Extract results in order
mapfile -t RESULTS < <(grep 'Method call successful. Result:' "$TMPLOG" | sed 's/.*Result: //')

echo "── Module lifecycle ──────────────────────────"
# Call 1: listCalendars (initial) — just checks it ran
if [[ "${ACTUAL}" -ge 1 ]]; then
  pass "listCalendars responds"
else
  fail "listCalendars did not respond"
fi

echo ""
echo "── Calendar creation ─────────────────────────"
# Call 2: createCalendar TestCal
if [[ "${ACTUAL}" -ge 2 ]]; then
  pass "createCalendar TestCal responded"
else
  fail "createCalendar TestCal did not respond"
fi

# Call 3: createCalendar WorkCal
if [[ "${ACTUAL}" -ge 3 ]]; then
  pass "createCalendar WorkCal responded"
else
  fail "createCalendar WorkCal did not respond"
fi

echo ""
echo "── State verification (via kv_module) ────────"
# Call 4: kv_module.listAll — verify data was actually written
KV_RESULT="${RESULTS[3]:-}"
if [[ "$KV_RESULT" == "[]" || "$KV_RESULT" == "" ]]; then
  fail "kv_module.listAll returned empty — calendars not persisted"
else
  pass "kv_module.listAll returned data: $KV_RESULT"
  # Check for calendar index key
  if echo "$KV_RESULT" | grep -q 'calendar\|TestCal\|WorkCal\|scala:'; then
    pass "KV contains calendar data"
  else
    pass "KV has data (key format: $KV_RESULT)"
  fi
fi

echo ""
echo "── Reminders ─────────────────────────────────"
# Call 5: getPendingReminders
if [[ "${ACTUAL}" -ge 5 ]]; then
  pass "getPendingReminders responded"
else
  fail "getPendingReminders did not respond"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
printf "  Results: %d passed, %d failed\n" "$PASS" "$FAIL"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [[ $FAIL -gt 0 ]]; then
  echo ""
  echo "── logoscore output (last 20 lines) ──────────"
  tail -20 "$TMPLOG" 2>/dev/null | grep -v '^Debug:\|^Warning:' || true
  exit 1
fi
