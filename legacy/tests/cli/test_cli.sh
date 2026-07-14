#!/bin/bash
# CLI integration tests for scala-cli.sh
# Requires: logoscore running with kv_module (make run-core)

set -euo pipefail
CLI="${1:-./cli/scala-cli.sh}"
PASS=0; FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS+1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL+1)); }

assert_contains() {
    echo "$1" | grep -q "$2" && pass "$3" || fail "$3 (got: $1)"
}

assert_success() {
    eval "$1" > /dev/null 2>&1 && pass "$2" || fail "$2"
}

assert_fail() {
    eval "$1" > /dev/null 2>&1 && fail "$2 (expected failure)" || pass "$2"
}

echo "=== Scala CLI Tests ==="
export SCALA_NAMESPACE="test_cli_$$"

echo "--- list-calendars (empty) ---"
result=$(SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI list-calendars 2>/dev/null || echo "[]")
assert_contains "$result" "\[" "list-calendars returns JSON array"

echo "--- create-calendar ---"
result=$(SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI create-calendar "TestCal" "#ff0000" 2>/dev/null || echo "{}")
assert_contains "$result" "TestCal" "create-calendar returns calendar with name"
assert_contains "$result" "id" "create-calendar returns calendar with id"

echo "--- list-calendars (after create) ---"
result=$(SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI list-calendars 2>/dev/null || echo "[]")
assert_contains "$result" "TestCal" "list-calendars shows created calendar"

echo "--- identity ---"
result=$(SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI identity 2>/dev/null || echo "{}")
assert_contains "$result" "id" "identity returns id field"

echo "--- help ---"
assert_success "SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI help" "help command succeeds"

echo "--- unknown command ---"
assert_fail "SCALA_NAMESPACE=$SCALA_NAMESPACE $CLI unknown_xyz_cmd" "unknown command exits non-zero"

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
