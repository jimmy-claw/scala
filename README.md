# Scala — Secure CALendar App

A privacy-first shared calendar app built on [Logos Core](https://logos.co).

**Scala** = **S**ecure **CAL**endar **A**pp

## Architecture (v0.2.0)

Scala is now a two-module system compatible with **basecamp 0.2.0**:

| Module | Type | Description |
|--------|------|-------------|
| `scala` | `core` (universal) | Core business logic — calendar/event CRUD, sync, sharing, search, reminders |
| `scala-ui` | `ui_qml` (universal) | QML frontend with C++ backend that delegates to core via typed replica |

Both modules use the [logos-module-builder 0.2.0](https://github.com/logos-co/logos-module-builder) universal authoring model:
- Auto-generated plugin wrappers from `*.impl.h` headers
- Typed inter-module SDK via `modules().dep.method()`
- Event subscriptions via `logos_events:` declarations
- No hand-written Qt plugins, no `Q_PLUGIN_METADATA`, no manual IPC wiring

## Building

### Prerequisites
- Nix (2.4+) with flakes enabled
- GitHub access token for fetching flake inputs

### Build core module
```bash
nix build -L .#scala_module
# Output: result/bin/libscala_plugin.so
```

### Build UI module
```bash
cd scala-ui
nix build -L .#scala_ui_module
# Output: result/bin/libscala_ui_plugin.so
```

### Development (override local dependencies)
```bash
# Point scala-ui at your local scala checkout
cd scala-ui
nix flake update --override-input scala ../
nix build -L .#scala_ui_module
```

## Installing

Via lgpm (Logos Package Manager):
```bash
lgpm install ./result  # core module
cd scala-ui && lgpm install ./result  # UI module
```

Or manually:
```bash
mkdir -p ~/.local/share/logos/modules
cp result/bin/libscala_plugin.so ~/.local/share/logos/modules/
# Repeat for scala-ui
```

## Running

Launch basecamp 0.2.0 — Scala will auto-load as an available module.

For standalone testing:
```bash
nix run github:logos-co/logos-basecamp/0.2.0#app
```

## Project Structure

```
scala/
├── metadata.json          # Core module metadata (universal interface)
├── flake.nix              # Nix build config (mkLogosModule)
├── CMakeLists.txt         # LogosModule.cmake macro (~15 lines)
├── src/
│   ├── scala_impl.h       # Universal-pattern entry point (ScalaImpl : LogosModuleContext)
│   ├── scala_impl.cpp     # Implementation (calendar/event CRUD, sync, sharing)
│   ├── calendar_store.h/.cpp  # Local persistence layer
│   ├── calendar_sync.h/.cpp   # P2P sync via Logos Messaging
│   └── qr_generator.h/.cpp    # QR code generation (share links)
├── scala-ui/              # UI module (separate flake, ui_qml type)
│   ├── metadata.json      # UI module metadata
│   ├── flake.nix          # Nix build config (mkLogosQmlModule)
│   ├── CMakeLists.txt     # LogosModule.cmake + .rep reference
│   ├── src/
│   │   ├── scala_ui_backend.rep   # QML-visible interface definition
│   │   ├── scala_ui_backend.h/.cpp  # C++ backend (delegates to modules().scala.*)
│   └── qml/               # QML views (CalendarView, EventModal, etc.)
├── docs/
│   └── migration-plan-basecamp-0.2.0.md  # Migration documentation
└── legacy/                # Archived v0.1 files (Qt plugins, Makefile, etc.)
```

## Dependencies

| Dependency | Status | Notes |
|------------|--------|-------|
| [kv_module](https://github.com/jimmy-claw/logos-kv-module) | Transitional fallback | Still in old format; 0.2.0 builder handles mixed graphs |
| messaging_module | Planned | For P2P calendar sync (not yet wired) |
| accounts_module | Planned | For identity management (not yet wired) |

## Status

🚧 Migration to basecamp 0.2.0 in progress — see [migration plan](docs/migration-plan-basecamp-0.2.0.md)

**Completed:**
- ✅ Core module universal pattern (`scala_impl.h/.cpp`)
- ✅ UI module scaffold + C++ backend (`scala_ui_backend.*`)
- ✅ QML ported to typed replica pattern (async `logos.watch()`)
- ✅ Build system migrated to Nix flake only (Makefile removed)
- ✅ CI updated for logos-module-builder

**Pending:**
- ⏳ Compilation verification (requires Crib build environment)
- ⏳ Integration test in basecamp 0.2.0
- ⏳ kv_module migration to universal pattern
- ⏳ Wire messaging_module for P2P sync
- ⏳ Package as .lgx

## Legacy (v0.1)

The v0.1 Qt-plugin code is archived in `legacy/`. Key changes from v0.1:
- Hand-written Qt plugins → auto-generated universal wrappers
- `QString`/`QJsonDocument` → `std::string`/standard C++
- Manual `LogosAPIClient` → typed `modules().dep.method()` SDK
- `module.yaml` + `Makefile` → `metadata.json` + Nix flake only

## License

MIT — see LICENSE file
