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
- ✅ Event reminders — QSystemTrayIcon notifications, configurable (15m/30m/1h before)
- ✅ Multi-instance dev testing — `SCALA_NAMESPACE` env var for isolated KV namespaces
- ✅ Standalone runner for local development and screenshots
- ✅ CLI wrapper (`scala-cli.sh`) for headless use
- ✅ 59 tests across 6 test suites

**Planned:**
- Logos Core IComponent packaging (lgpm)
- Real message signing (full crypto)
- At-rest KV encryption (#34)
- Logos Storage attachments
- QML UI tests with QQuickTest (#41)
- CLI integration tests (#51)

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
make test           # run all tests (59 tests, 6 suites)
make standalone     # build standalone runner
make screenshot     # take a headless screenshot (requires xvfb + scrot)
make install-cli    # install scala-cli.sh to ~/.local/bin/scala-cli
make clean          # remove build dirs
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

```bash
make install-cli

# With logoscore running (make run-core in another terminal):
scala-cli list-calendars
scala-cli create-calendar Work '#3b82f6'
scala-cli share <calendar-id>      # prints scala:// invite link
scala-cli join 'scala://...'       # join a shared calendar
scala-cli identity                 # show current identity
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

## Architecture

```
QML UI (Logos Core IComponent)
  ↓
C++ Module (LogosCalendar)
  ├── Local KV storage    — via logos-kv-module inter-module calls (namespace-isolated)
  ├── Logos Messaging     — P2P sync, per-calendar topic + encryption
  ├── Logos Core Identity — stable sender pubkey, event ownership, signing
  └── Logos Storage       — attachments (planned)
```

## Related

- Original prototype: [vpavlin/scala](https://github.com/vpavlin/scala)
- [Lope](https://github.com/jimmy-claw/lope) — notes app, same stack
- [logos-kv-module](https://github.com/jimmy-claw/logos-kv-module) — KV storage module used by Scala
- [Logos Core](https://github.com/logos-co/logos-app)
- [Local-First Conf 2026](https://localfirstconf.com) — CFP deadline May 1, 2026 (submission planned)
