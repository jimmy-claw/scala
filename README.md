# Scala — Secure CALendar App

A privacy-first shared calendar app built on [Logos Core](https://logos.co).

**Scala** = **S**ecure **CAL**endar **A**pp

## Status

🚧 Active development — core features implemented, Basecamp/Logos Core integration in progress.

**Implemented:**
- ✅ C++ Qt plugin (`LogosCalendar`) with full calendar/event CRUD
- ✅ Local storage via [logos-kv-module](https://github.com/jimmy-claw/logos-kv-module) inter-module calls
- ✅ QML UI — month grid, sidebar, event modal, event details, share dialog
- ✅ Logos Messaging sync layer — per-calendar topics, CREATE/UPDATE/DELETE/SYNC messages
- ✅ Calendar sharing — `scala://` invite links, QR placeholder, join flow
- ✅ Logos Core identity — stable sender ID, event ownership checks, message signing stub
- ✅ Standalone runner for local development and screenshots

**Planned:**
- Nix build + logos-module-builder packaging (#8)
- Basecamp / Logos Core IComponent integration
- Real message signing (full crypto)
- System notifications
- Logos Storage attachments

## Background

Scala was originally built as a demo/prototype using React + Waku JS SDK ([vpavlin/scala](https://github.com/vpavlin/scala)), serving as a real-world test for `ReliableChannel` in js-waku (based on SDS — Scalable Data Sync).

This repo is a rewrite on Logos Core — native C++/QML module with Logos Messaging for P2P sync, local storage, and Logos identity.

## Building & Running

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install cmake build-essential qt6-base-dev qt6-declarative-dev libqt6qml6

# For standalone runner screenshots
sudo apt install xvfb scrot
```

### Build the plugin

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

### Build the standalone runner

```bash
cmake -B build-standalone -DCMAKE_BUILD_TYPE=Debug -DBUILD_STANDALONE=ON
cmake --build build-standalone -j4 --target scala_standalone
```

### Run locally

```bash
# With display (local machine)
./build-standalone/scala_standalone

# Headless (no GPU / CI)
Xvfb :20 -screen 0 1280x800x24 &
DISPLAY=:20 QT_QUICK_BACKEND=software LIBGL_ALWAYS_SOFTWARE=1 \
    ./build-standalone/scala_standalone
```

### Screenshot

```bash
bash scripts/screenshot.sh
# Saves screenshot.png in repo root
```

### Run tests

```bash
cmake --build build -j4
cd build && ctest -V
```

## Architecture

```
QML UI (Logos Core IComponent)
  ↓
C++ Module (LogosCalendar)
  ├── Local KV storage    — via logos-kv-module inter-module calls
  ├── Logos Messaging     — P2P sync, per-calendar topic + encryption
  ├── Logos Core Identity — sender pubkey, event ownership
  └── Logos Storage       — attachments (future)
```

## Related

- Original prototype: [vpavlin/scala](https://github.com/vpavlin/scala)
- [Lope](https://github.com/jimmy-claw/lope) — notes app, same stack
- [logos-kv-module](https://github.com/jimmy-claw/logos-kv-module) — KV storage module used by Scala
- [Logos Core](https://github.com/logos-co/logos-app)
