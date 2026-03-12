# Scala — Secure CALendar App

A privacy-first shared calendar app built on [Logos Core](https://logos.co).

**Scala** = **S**ecure **CAL**endar **A**pp

## Status

🚧 Active development — v0.1 complete, Logos Core integration in progress.

**Implemented:**
- ✅ C++ Qt plugin (`LogosCalendar`) with full calendar/event CRUD
- ✅ Local storage via [logos-kv-module](https://github.com/jimmy-claw/logos-kv-module) inter-module calls
- ✅ QML UI — month view, week view, day view, sidebar, event modal, event details, share dialog
- ✅ Calendar sidebar — My Calendars vs Imported sections
- ✅ Date/time picker in EventModal
- ✅ Settings panel — default view, first day of week, identity display
- ✅ Logos Messaging sync layer — per-calendar topics, CREATE/UPDATE/DELETE/SYNC messages
- ✅ Calendar sharing — `scala://` invite links, QR placeholder, join flow
- ✅ Logos Core identity — stable sender ID, event ownership checks, message signing stub
- ✅ Event search — search across all calendars by title/description/location (Ctrl+F)
- ✅ Event reminders — QSystemTrayIcon notifications, configurable (15m/30m/1h before)
- ✅ Multi-instance dev testing — `SCALA_NAMESPACE` env var for isolated KV namespaces
- ✅ Standalone runner for local development and screenshots
- ✅ C++ CLI client (`scala_cli`) — connects to running logoscore via QtRO
- ✅ CLI wrapper (`scala-cli.sh`) for headless use
- ✅ 59 tests across 6 test suites
- ✅ CLI integration tests (`make test-cli`)
- ✅ Headless logoscore plugin (`scala_module`) — loads in logoscore without Qt Quick/GUI
- ✅ QML UI wired to scala_module via `LogosAPIClient` (`ScalaBridge`) — standalone talks to running logoscore over QtRO

**Planned:**
- Real message signing (full crypto)
- At-rest KV encryption (#34)
- Logos Storage attachments
- QML UI tests with QQuickTest (#41)

## Background

Scala was originally built as a demo/prototype using React + Waku JS SDK ([vpavlin/scala](https://github.com/vpavlin/scala)), serving as a real-world test for `ReliableChannel` in js-waku (based on SDS — Scalable Data Sync).

This repo is a rewrite on Logos Core — native C++/QML module with Logos Messaging for P2P sync, local storage, and Logos identity.

## Building & Running

### Quick start (Makefile)

```bash
# One-time setup — builds logoscore + kv_module via Nix (~10-30 min first time)
make setup

# Terminal 1 — start Logos Core with kv_module
make run-core

# Terminal 2 — build and run Scala
make dev
```

Other targets:
```bash
make build          # build plugin only (no standalone)
make build-module   # build headless logoscore plugin (scala_module)
make build-cli      # build C++ CLI binary (scala_cli)
make run-module     # run logoscore with kv_module + scala_module
make test           # run all tests (59 tests, 6 suites)
make test-cli       # CLI integration tests (requires make run-core)
make standalone     # build standalone runner
make screenshot     # take a headless screenshot (requires xvfb + scrot)
make install-cli    # install scala-cli.sh to ~/.local/bin/scala-cli
make clean          # remove build dirs

# CLI via logoscore --call (no running logoscore needed):
make list-calendars
make create-calendar NAME=MyCal COLOR='#3b82f6'
make list-events CAL=<calendar-id>
make get-identity
make share-calendar CAL=<calendar-id>
make join-calendar LINK='scala://...'
```

### Multi-instance testing (sharing/sync)

To test calendar sharing between two "users" on the same machine:

```bash
# Terminal 1 — Alice
make run-core  # shared logoscore instance

# Terminal 2 — Alice's Scala
SCALA_NAMESPACE=alice make dev

# Terminal 3 — Bob's Scala
SCALA_NAMESPACE=bob DISPLAY=:21 ./build-standalone/scala_standalone
```

Each instance uses an isolated KV namespace (`scala:alice:*` vs `scala:bob:*`).

### CLI (headless use)

The C++ CLI binary connects directly to a running logoscore instance via QtRemoteObjects:

```bash
# Build the CLI binary
make build-cli

# Terminal 1 — start logoscore with scala_module
make run-module

# Terminal 2 — use the CLI
./build-module/scala_cli listCalendars
./build-module/scala_cli createCalendar Work '#3b82f6'
./build-module/scala_cli listEvents <calendarId>
./build-module/scala_cli generateShareLink <calendarId>
./build-module/scala_cli getIdentity
```

The `scala-cli.sh` wrapper provides friendly command aliases:

```bash
make install-cli
scala-cli list-calendars
scala-cli create-calendar Work '#3b82f6'
scala-cli share <calendar-id>
scala-cli join 'scala://...'
scala-cli identity
```

### scala-cli (direct wrapper)

`tools/scala-cli` is a standalone bash wrapper that invokes logoscore `--call` directly
(no running logoscore session needed):

```bash
# Auto-detects logoscore from nix store
tools/scala-cli listCalendars
tools/scala-cli createCalendar MyCalendar "#3b82f6"
tools/scala-cli getPendingReminders

# Override logoscore path
LOGOSCORE=/path/to/logoscore tools/scala-cli listCalendars
```

### E2E tests

```bash
# Run all e2e tests (sets SCALA_E2E_MINIMAL=1 to skip optional module timeouts)
bash tools/e2e-test.sh

# Tests: listCalendars, createCalendar, listCalendars (verify), getPendingReminders
# Runs all calls in a single logoscore session for speed
```

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install cmake build-essential qt6-base-dev qt6-declarative-dev \
    libqt6qml6 qt6-remoteobjects-dev xvfb scrot
```

### Run with Logos Core (recommended)

```bash
make setup      # one-time: builds logoscore + kv_module via Nix
make run-core   # terminal 1: starts Logos Core
make dev        # terminal 2: builds + runs Scala
```

See [Makefile](./Makefile) for details on nix store path auto-detection.

### Run tests

```bash
make test
# or: cmake -B build -DBUILD_TESTS=ON && cmake --build build && cd build && ctest -V
```

### Screenshot

```bash
make screenshot
# saves screenshot.png in repo root
```

### Running standalone UI with logoscore

When built with Logos SDK, the standalone runner connects to the running `scala_module`
via `LogosAPIClient` (QtRemoteObjects). This is the recommended way to run the UI:

```bash
# Terminal 1 — start logoscore with kv_module + scala_module
make run-module

# Terminal 2 — build and run the standalone UI (connects to scala_module via QtRO)
make standalone LOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged LOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged
./build-standalone/scala_standalone
```

The standalone reads the scala_module auth token from `/tmp/logos_scala_module` on startup.
If logoscore is not running, the build still works — it falls back to in-memory `LogosCalendar`.

## Architecture

```
logos_host (logoscore)
  └── scala_module_plugin.so  ← headless, Qt Core/Qml/RemoteObjects only
        └── ScalaPlugin → LogosCalendar (CalendarModule API via QtRO)

QML UI (standalone process)
  └── ScalaBridge (QObject, C++)
        └── LogosAPIClient("scala_module", "scala_ui", tokenManager)
              └── QtRO → logos_host (scala_module) → ScalaPlugin

scala_cli (C++ binary)
  └── connects to logoscore QtRO registry → invokes methods, prints results

C++ Module (LogosCalendar)
  ├── Local KV storage    — via logos-kv-module inter-module calls (namespace-isolated)
  ├── Logos Messaging     — P2P sync, per-calendar topic + encryption
  ├── Logos Core Identity — stable sender pubkey, event ownership, signing
  └── Logos Storage       — attachments (planned)
```

The plugin `.so` loaded by `logos_host` is **headless** — no Qt Quick, no QML engine, no GUI.
The QML UI runs as a separate process. When built with `LOGOS_CORE_AVAILABLE`, the standalone
uses `ScalaBridge` → `LogosAPIClient` to dispatch all calls to the running `scala_module` over
QtRemoteObjects. Without it, it falls back to an in-memory `LogosCalendar` instance.

## Related

- Original prototype: [vpavlin/scala](https://github.com/vpavlin/scala)
- [Lope](https://github.com/jimmy-claw/lope) — notes app, same stack
- [logos-kv-module](https://github.com/jimmy-claw/logos-kv-module) — KV storage module used by Scala
- [Logos Core](https://github.com/logos-co/logos-app)
- [Local-First Conf 2026](https://localfirstconf.com) — CFP deadline May 1, 2026 (submission planned)
