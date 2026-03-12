# feat: add direct kv_module plugin support - PR #70

## Quick Summary

Scala can now run **standalone with persistence** WITHOUT requiring `logos_host` (logoscore) to be running!

### What Changed

**Before:** Scala needed logos_host running to access KV storage  
**After:** Scala links directly to `kv_module_plugin.so` and calls KV operations directly

### How It Works

`CalendarStore` now supports **three modes** (priority order):

1. **Logos Host mode** (existing): `Scala → LogosAPIClient → logos_host → kv_module`
2. **Direct kv_module mode** (NEW): `Scala → kv_module plugin` ← **NO logos_host needed!**
3. **In-memory mode** (testing): `Scala → QMap` (no persistence)

### Try It Yourself

```bash
# 1. Clone and build logos-kv-module first
cd ~ && git clone https://github.com/jimmy-claw/logos-kv-module.git
cd logos-kv-module && cmake -B build && cmake --build build

# 2. Build Scala with direct kv_module support
cd ~ && git clone https://github.com/jimmy-claw/scala.git
cd scala
cmake -B build \
    -DCMAKE_PREFIX_PATH=/usr/lib/qt6 \
    -DKV_MODULE_AVAILABLE=ON \
    -DKV_MODULE_INCLUDE_DIR=../logos-kv-module/src

cmake --build build

# 3. Run! No logos_host needed
./build/scala_standalone
```

Or use the helper script:
```bash
./scripts/start-scala-with-kv.sh ~/logos-kv-module/build/lib
```

## Files Changed

- `src/calendar_store.h` - Added `setKvModule()` method
- `src/calendar_store.cpp` - Priority-based fallback logic
- `CMakeLists.txt` - Detect & link kv_module plugin
- `scripts/start-scala-with-kv.sh` - Demo script

## Benefits

✅ **No logos_host dependency** - Standalone with persistence  
✅ **Backwards compatible** - Existing mode still works  
✅ **Easier testing** - No need to start full Logos Core stack  
✅ **Clear architecture** - Explicit priority order  

## PR Details

- **URL:** https://github.com/jimmy-claw/scala/pull/70
- **Branch:** `feature/direct-kv-module-support`
- **Status:** Open (awaiting CI)

## Related

- Fixes KV persistence blocker
- Alternative to upstream `logos-co/logos-liblogos#66` fix
- Enables standalone dev workflow
