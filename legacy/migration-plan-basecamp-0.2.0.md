# Scala → basecamp 0.2.0 Migration Plan

**Date:** 2026-07-14  
**Target:** logos-basecamp 0.2.0-RC3 + logos-module-builder 0.2.0  
**Source state:** scala v0.1 (hand-written Qt plugins, manual CMake, old SDK)  

---

## Executive Summary

Scala was built against the pre-split Logos SDK (logos-cpp-sdk + logos-liblogos as separate manual-link targets). Basecamp 0.2.0 uses the **universal authoring model** via `logos-module-builder` — pure-C++ impl headers, auto-generated Qt plugin glue, LIDL-based dependency contracts, and nix-flake-only builds.

**Scope of change:** ~80% of build/config files rewrite, ~30% of source code refactoring (the business logic in `calendar_module.cpp` stays largely intact).

---

## Gap Analysis — What's Different

### 1. metadata.json format (BREAKING)

| Field | Current scala | basecamp 0.2.0 expects |
|-------|--------------|----------------------|
| `name` | `"scala_module"` | `"scala"` (or keep, but must be consistent everywhere) |
| `type` | `"core"` | `"core"` ✅ (same) |
| `main` | Platform-specific object with `.so`/`.dylib` keys | Single string: `"scala_plugin"` (no extension) |
| `interface` | *(missing)* | **`"universal"`** — required for auto-generation |
| `author` | `"Jimmy Claw"` | *(removed — use flake/git metadata)* |
| `capabilities` | `[]` | *(removed — replaced by access policy)* |
| `dependencies` | `["kv_module"]` | `["kv_module", "messaging_module", "accounts_module"]` ✅ (same format, expand list) |
| `nix` section | *(missing)* | **Required** — build config (packages, cmake, external_libs) |

### 2. CMakeLists.txt (BREAKING)

| Aspect | Current scala | basecamp 0.2.0 expects |
|--------|--------------|----------------------|
| Build system | Manual CMake with `find_package(Qt6)`, manual SDK linking, multiple conditional targets (`BUILD_MODULE`, `BUILD_STANDALONE`, `BUILD_UI_PLUGIN`) | `LogosModule.cmake` from `logos-module-builder`, single `logos_module()` macro call |
| Plugin generation | Hand-written `scala_plugin.cpp/h`, `plugin.cpp`, `scala_ui_component.cpp/h` | Auto-generated from `*_impl.h` header by `logos-cpp-generator` |
| SDK linking | Manual `LOGOS_CPP_SDK_ROOT` / `LOGOS_LIBLOGOS_ROOT` paths, merged symlink dirs | Handled by nix flake + module-builder — no manual paths |

### 3. flake.nix (BREAKING)

| Aspect | Current scala | basecamp 0.2.0 expects |
|--------|--------------|----------------------|
| Config file | `module.yaml` | `metadata.json` only (no `module.yaml`) |
| Builder call | `mkLogosModule { configFile = ./module.yaml }` + custom output overrides | `mkLogosModule { src = ./.; configFile = ./metadata.json; flakeInputs = inputs; }` |
| SDK inputs | Manual `logos-cpp-sdk`, `logos-liblogos` inputs with follows | **Removed** — module-builder handles everything internally |
| Pinning | Unpinned `logos-module-builder` URL | Pinned to `github:logos-co/logos-module-builder/0.2.0` |
| Custom outputs | Manual `ui`, `ui-plugin` packages | Handled by module-builder templates (`ui-qml-backend`) |

### 4. Source Code Pattern (MAJOR)

**Current:** Hand-written Qt plugin classes:
- `ScalaPlugin : QObject, PluginInterface` — manual `initLogos(LogosAPI*)`, forwarding all methods to inner `LogosCalendar`
- `LogosCalendar : QObject, PluginInterface, ILogosCalendar` — manual `Q_PLUGIN_METADATA`, `Q_INTERFACES`, `Q_INVOKABLE` on every method
- `ScalaUIComponent : QObject, IComponent` — manual `createWidget(LogosAPI*)` for basecamp integration
- `ScalaBridge` — manual `LogosAPIClient` wiring for standalone→logoscore QtRO connection

**New (universal pattern):**
- **Core module:** One plain C++ class `ScalaImpl : LogosModuleContext` in `src/scala_impl.h/.cpp` — no Qt, no `Q_OBJECT`, no `Q_PLUGIN_METADATA`. All public methods auto-exposed. Events via `logos_events:` section. Inter-module calls via `modules().kv_module.get(...)`.
- **UI module (separate):** `ui_qml` type with either QML-only or C++ backend (`.rep` file + `*Backend : SimpleSource, LogosUiPluginContext`). The UI is a *different module* from the core — basecamp loads them separately.

### 5. Build System (BREAKING)

| Aspect | Current scala | basecamp 0.2.0 expects |
|--------|--------------|----------------------|
| Makefile | 200-line Makefile with nix store auto-detection, merged symlink dirs, CLI targets | **Removed** — `nix build`, `nix run` only |
| logoscore --call | Manual CLI wrapper scripts (`scala-cli.sh`, `tools/scala-cli`) | Built-in `logoscore` daemon/client workflow |
| kv_module setup | Clone + cmake + manual symlink in Makefile | Declared as flake input + `dependencies` in metadata.json |

### 6. Inter-Module Communication (MODERATE)

| Aspect | Current scala | basecamp 0.2.0 expects |
|--------|--------------|----------------------|
| KV calls | Manual `LogosAPIClient("kv_module", ...)` with token management | `modules().kv_module.get(key)` — auto-generated typed SDK |
| Messaging | Manual `LogosAPIClient` to messaging_module | `modules().messaging_module.publish(...)` |
| Accounts | Manual `LogosAPIClient` to accounts_module | `modules().accounts_module.getIdentity()` |
| Events | Manual `eventResponse` signal wiring | `logos_events:` section in impl header, generator emits routing |

### 7. UI Integration (MAJOR)

**Current:** Scala is a single monolithic module trying to be both core backend AND UI plugin (`IComponent`).

**New:** Two separate modules:
1. **`scala`** — `core` type, headless backend (calendar CRUD, sync, storage). Loaded by `logos_host`.
2. **`scala_ui`** — `ui_qml` type with C++ backend. The view that basecamp displays as a tab. Calls `scala` via `modules().scala.*`.

This is the architecture basecamp 0.2.0 expects: core modules run in `logos_host` processes; UI modules load into basecamp's process (or spawn `ui-host` for C++ backends).

---

## Migration Plan — Step by Step

### Phase 1: Core Module (`scala`) — Universal Pattern

**Goal:** Convert the backend logic to the universal authoring model. No UI yet.

#### Step 1.1: Rewrite metadata.json
```json
{
  "name": "scala",
  "version": "0.2.0",
  "type": "core",
  "category": "general",
  "description": "Secure Calendar App — privacy-first shared calendar module",
  "main": "scala_plugin",
  "interface": "universal",
  "dependencies": ["kv_module", "messaging_module", "accounts_module"],

  "nix": {
    "packages": { "build": [], "runtime": [] },
    "external_libraries": [],
    "cmake": {
      "find_packages": [],
      "extra_sources": [],
      "extra_include_dirs": [],
      "extra_link_libraries": []
    }
  }
}
```

#### Step 1.2: Create scala_impl.h/.cpp
- Extract business logic from `calendar_module.cpp` into a plain C++ class
- Inherit `LogosModuleContext` (not `PluginInterface`)
- Use `std::string`/`int64_t` types (not `QString`/`int`) — generator handles wire translation
- Declare events with `logos_events:` section
- Replace manual `LogosAPIClient` calls with `modules().dep.method()`
- Remove: `ScalaPlugin`, `plugin.cpp`, all `Q_PLUGIN_METADATA`/`Q_INTERFACES` boilerplate

Key methods to port (from `calendar_module.h`):
```cpp
class ScalaImpl : public LogosModuleContext {
public:
    // Calendar CRUD
    std::string createCalendar(const std::string& name, const std::string& color);
    std::string listCalendars();
    bool deleteCalendar(const std::string& id);
    std::string createEvent(const std::string& calendarId, const std::string& eventJson);
    // ... (all CRUD methods)
    
    // Sync/Share
    std::string shareCalendar(const std::string& calendarId);
    bool joinSharedCalendar(const std::string& calendarId, const std::string& encryptionKey);
    std::string generateShareLink(const std::string& calendarId);
    
    // Search/Reminders/Settings
    std::string searchEvents(const std::string& query);
    std::string getPendingReminders();
    void setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultValue);
    
    // Identity
    void setNamespace(const std::string& ns);
    std::string getIdentity() const;
    void setIdentity(const std::string& pubkeyHex);

logos_events:
    void eventResponse(const std::string& eventName, /* typed args */);
};
```

#### Step 1.3: Rewrite CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.14)
project(ScalaModule LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found")
endif()

logos_module(
    NAME scala
    SOURCES
        src/scala_impl.h
        src/scala_impl.cpp
        src/calendar_store.cpp
        src/calendar_sync.cpp
        src/qr_generator.cpp
)
```

#### Step 1.4: Rewrite flake.nix
```nix
{
  description = "Secure Calendar App — core module for Logos";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/0.2.0";
    
    # Dependency modules (for LIDL contract consumption)
    kv_module.url = "github:jimmy-claw/logos-kv-module";
    messaging_module.url = "github:logos-co/logos-messaging-module";
    accounts_module.url = "github:logos-co/logos-accounts-module";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
```

#### Step 1.5: Remove obsolete files
- `plugin.cpp` — replaced by auto-generated plugin
- `scala_plugin.cpp/h` — replaced by auto-generated plugin
- `scala_ui_component.cpp/h` — moved to UI module (Phase 2)
- `scala_bridge.cpp/h` — replaced by `modules()` typed SDK
- `interfaces/IComponent.h` — no longer needed
- `module.yaml` — replaced by metadata.json
- `Makefile` — replaced by nix flake
- `standalone/` — replaced by `logos-standalone-app` from module-builder
- `cli/scala-cli.sh`, `tools/scala-cli` — replaced by `logoscore` daemon/client

#### Step 1.6: Adapt business logic files
- `calendar_store.cpp/h` — replace `LogosAPIClient` with `modules().kv_module.*`
- `calendar_sync.cpp/h` — replace `LogosAPIClient` with `modules().messaging_module.*`
- Convert `QString` → `std::string`, `int` → `int64_t` throughout

**Estimated effort:** 2-3 days of focused work  
**Risk:** Medium — business logic porting requires careful type conversion and testing

---

### Phase 2: UI Module (`scala_ui`) — ui_qml with C++ Backend

**Goal:** Create the UI as a separate `ui_qml` module that basecamp loads as a tab.

#### Step 2.1: Scaffold from template
```bash
mkdir scala-ui && cd scala-ui
nix flake init -t github:logos-co/logos-module-builder/0.2.0#ui-qml-backend
rm -f src/ui_example.rep src/ui_example_backend.h src/ui_example_backend.cpp
```

#### Step 2.2: metadata.json
```json
{
  "name": "scala_ui",
  "version": "0.2.0",
  "type": "ui_qml",
  "interface": "universal",
  "category": "general",
  "description": "Scala Calendar UI — QML view with process-isolated backend",
  "main": "scala_ui_plugin",
  "view": "qml/CalendarView.qml",
  "icon": null,
  "dependencies": ["scala"],
  "codegen": { "rep": "src/scala_ui.rep" },

  "nix": {
    "packages": { "build": [], "runtime": [] },
    "external_libraries": [],
    "cmake": {
      "find_packages": [],
      "extra_sources": [],
      "extra_include_dirs": [],
      "extra_link_libraries": []
    }
  }
}
```

#### Step 2.3: Create .rep file
Define the QML-visible interface (what QML can call on the backend):
```
// src/scala_ui.rep
module ScalaUi {
    struct Calendar {
        QString id;
        QString name;
        QString color;
    };
    
    struct Event {
        QString id;
        QString calendarId;
        QString title;
        // ... event fields
    };
    
    // CRUD exposed to QML
    QString createCalendar(QString name, QString color);
    QStringList listCalendars();
    bool deleteCalendar(QString id);
    // ... all methods needed by the UI
    
    // Properties for auto-sync
    PROP QString currentView;  // READWRITE
    PROP int eventCount;       // READONLY (backend sets)
    
    // Signals for push updates
    SIGNAL calendarsUpdated();
    SIGNAL eventsUpdated(QString calendarId);
};
```

#### Step 2.4: Create Backend class
```cpp
// src/scala_ui_backend.h
#include "rep_scala_ui_source.h"
#include "logos_ui_plugin_context.h"

class ScalaUiBackend : public ScalaUiSimpleSource,
                       public LogosUiPluginContext {
public:
    // Override .rep slots — delegate to scala core module
    QString createCalendar(QString name, QString color) override;
    QStringList listCalendars() override;
    // ... all slot overrides
    
    void onContextReady() override;  // arm subscriptions

private:
    void notifyCalendarsUpdated();
    void notifyEventsUpdated(const QString& calendarId);
};
```

The key insight: the UI backend **doesn't contain business logic** — it delegates everything to `modules().scala.*`. It only handles QML↔backend bridging and signal/property updates.

#### Step 2.5: Port QML files
- Move existing QML files from `scala/qml/` into `scala-ui/qml/`
- Replace `LogosAPIClient`-based calls with `logos.module("scala_ui")` + typed replica
- Replace direct `logos.callModule()` with backend slot calls
- Update imports to use the new module structure

#### Step 2.6: flake.nix for UI module
```nix
{
  description = "Scala Calendar UI";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder/0.2.0";
    scala.url = "path:/path/to/scala";  # or github:jimmy-claw/scala
  };

  outputs = inputs@{ logos-module-builder, scala, ... }:
    logos-module-builder.lib.mkLogosQmlModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
    };
}
```

**Estimated effort:** 2-3 days  
**Risk:** Medium — QML porting requires understanding the new `logos.module()` API vs old `LogosAPIClient`

---

### Phase 3: Build, Test, Package

#### Step 3.1: Verify core module builds
```bash
cd scala
nix build -L                          # builds the .so
nix run github:logos-co/logos-module -- metadata ./result/lib/scala_plugin.so  # inspect with lm
```

#### Step 3.2: Verify UI module builds
```bash
cd scala-ui
nix build -L                          # builds the plugin
nix run -L                            # launches logos-standalone-app with your UI (for testing)
```

#### Step 3.3: Integration test in basecamp
```bash
# Build basecamp 0.2.0
nix build github:logos-co/logos-basecamp/0.2.0#app -o /tmp/basecamp

# Install scala + scala_ui via lgpm
/tmp/basecamp/bin/lgpm install ./scala/result --as scala
/tmp/basecamp/bin/lgpm install ./scala-ui/result --as scala_ui

# Launch basecamp — scala should appear in the sidebar/apps
/tmp/basecamp/bin/logos-basecamp
```

#### Step 3.4: Package as .lgx
```bash
cd scala
nix build .#package -o scala.lgx      # or use nix-bundle-lgx

cd scala-ui  
nix build .#package -o scala-ui.lgx
```

**Estimated effort:** 1-2 days  
**Risk:** Low — mostly following documented procedures

---

### Phase 4: Cleanup & CI

#### Step 4.1: Update GitHub Actions
Replace current CI (cachix + manual cmake) with module-builder's built-in CI template.

#### Step 4.2: Update README
Document the new build flow (`nix build`, `nix run`), remove Makefile instructions.

#### Step 4.3: Archive old files
Move obsolete files to `legacy/` directory (or delete if git history is sufficient):
- Old CMakeLists.txt → `legacy/CMakeLists.txt.manual`
- Old Makefile → `legacy/Makefile.manual`
- Old plugin source → `legacy/plugin/`

**Estimated effort:** 0.5 days  
**Risk:** Low

---

## Files to Keep / Delete / Rewrite

### Keep (business logic — port types only)
- `src/calendar_store.cpp/h` — adapt `LogosAPIClient` → `modules().kv_module`
- `src/calendar_sync.cpp/h` — adapt `LogosAPIClient` → `modules().messaging_module`
- `src/qr_generator.cpp/h` — keep as-is (no SDK deps)
- `src/types.h` — adapt types (`QString` → `std::string`)

### Rewrite completely
- `metadata.json` — new format with `interface: "universal"` + `nix` section
- `CMakeLists.txt` — use `LogosModule.cmake`
- `flake.nix` — use `mkLogosModule` with pinned 0.2.0

### Delete (replaced by auto-generation or no longer needed)
- `src/plugin.cpp` — auto-generated
- `src/scala_plugin.cpp/h` — auto-generated
- `src/scala_ui_component.cpp/h` → moved to Phase 2 (UI module)
- `src/scala_bridge.cpp/h` — replaced by `modules()` SDK
- `interfaces/IComponent.h` — no longer needed
- `module.yaml` — replaced by metadata.json
- `Makefile` — replaced by nix flake
- `standalone/main.cpp` — use `logos-standalone-app` from module-builder
- `cli/scala-cli.sh`, `tools/scala-cli` — use `logoscore` daemon/client
- `ui_metadata.json` — absorbed into new metadata.json

### Move to scala-ui/ (Phase 2)
- All `qml/*.qml` files
- `qml/scala_ui.qrc` (likely no longer needed with module-builder)

---

## Dependencies That Need Updating

| Dependency | Current state | Action needed |
|-----------|--------------|--------------|
| `kv_module` (logos-kv-module) | Custom repo, old SDK format | Must also be migrated to universal pattern OR use a released 0.2.0-compatible version |
| `messaging_module` | Referenced in module.yaml but no local integration code visible | Confirm availability as 0.2.0-compatible flake input |
| `accounts_module` | Same as messaging | Confirm availability |

**Critical path:** If `kv_module` isn't compatible with 0.2.0, it needs to be migrated first (or in parallel), because scala's core logic depends on it for persistence.

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|-----------|
| kv_module not 0.2.0 compatible | High — blocks core module | Migrate kv_module first or use `dependency_overrides` with LIDL contract |
| Business logic type conversion bugs | Medium — QString→std::string changes throughout | Keep existing tests, adapt them to new test framework |
| QML API changes (logos.module vs LogosAPIClient) | Medium — UI won't work if calls are wrong | Test with `nix run` in standalone app before basecamp integration |
| Messaging sync logic breaks | High — core feature | Port carefully, keep existing message format, test P2P flow |
| Basecamp 0.2.0 API changes post-RC3 | Low-Medium | Pin to exact commit, not just tag |

---

## Estimated Total Effort

| Phase | Days | Complexity |
|-------|------|-----------|
| Phase 1: Core module universal pattern | 2-3 | Medium |
| Phase 2: UI module (separate repo) | 2-3 | Medium |
| Phase 3: Build, test, package | 1-2 | Low |
| Phase 4: Cleanup & CI | 0.5 | Low |
| **Contingency (kv_module migration)** | 1-2 | Depends on kv_module state |
| **Total** | **6.5-10.5 days** | |

---

## Recommended Execution Order

1. **Check kv_module compatibility** — can it be consumed as a 0.2.0 flake input? If not, migrate it first (or in parallel).
2. **Phase 1** — get the core module building and passing tests with `logoscore`.
3. **Phase 2** — scaffold UI module, port QML, verify with `nix run` standalone.
4. **Phase 3** — integrate into basecamp 0.2.0, full E2E test.
5. **Phase 4** — cleanup, CI, docs.
