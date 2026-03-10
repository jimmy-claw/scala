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
# cmake (recommended for local dev)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4

# Nix (for Logos Core packaging)
nix build .#module   # builds scala_plugin.so
nix build .#ui       # packages QML files
```

### Build the standalone runner

```bash
cmake -B build-standalone -DCMAKE_BUILD_TYPE=Debug -DBUILD_STANDALONE=ON
cmake --build build-standalone -j4 --target scala_standalone
```


### Run with Logos Core (recommended — real KV + messaging)

Scala uses Logos Core modules for storage and messaging. For real persistence, run `logoscore` with `kv_module` alongside the standalone runner.

**Step 1: Get logoscore**

There is no pre-built binary yet — you need to build it from source via Nix:

```bash
# Requires Nix with flakes enabled
nix build github:logos-co/logos-liblogos
# Binary will be at: ./result/bin/logoscore
```

Or if your team has a pre-built binary (e.g. from Václav's laptop e2e tests):
```
/nix/store/<hash>-logos-liblogos-build-0.1.0/bin/logoscore
```

**Step 2: Build and install kv_module**

```bash
git clone https://github.com/jimmy-claw/logos-kv-module
cd logos-kv-module
nix build .#module
mkdir -p ~/.local/share/logos/modules/kv_module
cp result/lib/kv_module_plugin.so ~/.local/share/logos/modules/kv_module/
cp manifest.json ~/.local/share/logos/modules/kv_module/
```

**Step 3: Run**

```bash
# Terminal 1 — Logos Core with kv_module
logoscore --modules-dir ~/.local/share/logos/modules --load kv_module

# Terminal 2 — Scala
LOGOS_CORE_AVAILABLE=1 ./build-standalone/scala_standalone
```

> **Note:** Logos Core is under active development. If you hit issues, the standalone runner (without Logos Core) works for UI development — data will be in-memory only.

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
