# Direct KV Module Support - No Logos Host Required

## Problem

Scala currently requires `logos_host` (logoscore) to be running in order to access KV storage persistence. This is because `CalendarStore` uses `LogosAPIClient` to make QtRO calls to `kv_module` through the logos_host plugin system.

This creates a blocker: Scala cannot run as a standalone application with persistence unless logos_host is started first.

## Solution

Add direct support for linking against the `kv_module` plugin, which allows Scala to call KV operations directly without going through logos_host.

### Architecture Changes

**Before:**
```
Scala UI → ScalaBridge → LogosAPIClient → logos_host → kv_module
```

**After (three modes, priority order):**
```
1. Logos Host mode:  Scala UI → ScalaBridge → LogosAPIClient → logos_host → kv_module
2. Direct mode:      Scala UI → CalendarStore → kv_module plugin (direct link)
3. Memory mode:      Scala UI → CalendarStore → QMap (in-memory fallback)
```

## Changes

### 1. `calendar_store.h`
- Added `#include "i_kv_module.h"` to get `IKvModule` interface
- Added `setKvModule(IKvModule *kv)` method for direct plugin support
- Changed member variables:
  - `m_kvClient`: `LogosAPIClient*` (logos_host mode)
  - `m_kvModule`: `IKvModule*` (direct plugin mode) ← NEW
  - `m_mem`: `QMap` (memory mode, now always available)

### 2. `calendar_store.cpp`
- Updated `kvSet()`, `kvGet()`, `kvRemove()` with priority-based fallback:
  ```cpp
  // Priority 1: logos_host (QtRO)
  if (m_kvClient) { ... }
  
  // Priority 2: direct kv_module plugin
  if (m_kvModule) { m_kvModule->get(ns, key); return; }
  
  // Priority 3: in-memory
  return m_mem.value(key);
  ```

### 3. `CMakeLists.txt`
- Added `KV_MODULE_AVAILABLE` CMake flag
- Added `KV_MODULE_INCLUDE_DIR` detection
- Links `kv_module_plugin` when available:
  ```cmake
  if(KV_MODULE_AVAILABLE)
      target_link_libraries(scala_module_plugin PRIVATE kv_module_plugin)
      message(STATUS "KV module linked: scala can use kv_module directly")
  endif()
  ```

### 4. `scripts/start-scala-with-kv.sh`
- New script to demonstrate standalone usage
- Auto-detects `kv_module_plugin.so` location
- Shows how to build and run without logos_host

## Usage

### Mode 1: Logos Host (existing)
```bash
logoscore --modules-dir ./modules --load-modules kv_module,scala_module
./build/scala_standalone  # CLI mode connects via QtRO
```

### Mode 2: Direct kv_module (NEW)
```bash
# Build logos-kv-module first
cd logos-kv-module && cmake -B build -DBUILD_TESTS=ON && cmake --build build

# Build Scala with kv_module support
cd scala
cmake -B build \
    -DCMAKE_PREFIX_PATH=/usr/lib/qt6 \
    -DKV_MODULE_AVAILABLE=ON \
    -DKV_MODULE_INCLUDE_DIR=../logos-kv-module/src

cmake --build build

# Run standalone with persistence (no logos_host needed!)
./build/scala_standalone
```

### Mode 3: In-memory (testing)
```bash
# No kv_module linked, no LOGOS_CORE_AVAILABLE
cmake -B build -DBUILD_STANDALONE=ON
cmake --build build

./build/scala_standalone  # In-memory mode, no persistence
```

## Benefits

✅ **No logos_host dependency** - Scala can run as standalone with persistence  
✅ **Backwards compatible** - Existing logos_host mode still works  
✅ **Flexibility** - Choose mode at build time  
✅ **Testing** - Easy to test without starting full Logos Core stack  
✅ **Clear architecture** - Priority-based fallback is explicit and maintainable

## Testing

Test scenarios:
1. **Direct mode**: Run `scripts/start-scala-with-kv.sh` with kv_module path
2. **Logos host mode**: Current CI tests (should still pass)
3. **Memory mode**: Run without kv_module linked, verify in-memory fallback works

## Next Steps

- [ ] Add kv_module dependency to Nix flake
- [ ] Add CI job for direct mode build
- [ ] Update README with direct mode usage
- [ ] Consider making direct mode the default when kv_module is available

## Related Issues

- Blocks `make run-module` testing on Václav's machine
- Related to upstream `logos-co/logos-liblogos#66` (async getClient deadlock)
- Addresses KV persistence blocker
