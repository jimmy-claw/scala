# Scala v0.2.0 — Remaining Work Plan

**Updated:** 2026-07-16  
**Branch:** `plan/migration-basecamp-0.2.0` (PR #79, CI ✅)  
**kv_module branch:** `plan/migration-basecamp-0.2.0` (PR #29, CI ✅)  

---

## What's Done ✅

| Item | Status |
|------|--------|
| Phase 1: Core module universal pattern | ✅ Merged to PR #79 |
| Phase 2: UI module scaffold + C++ backend | ✅ Merged to PR #79 |
| Phase 3.1-3.2: Build verification (CI) | ✅ All 3 CI jobs pass |
| Phase 4: Cleanup & CI | ✅ Makefile archived, README updated |
| kv_module migration to universal pattern | ✅ PR #29 open, CI passes |

---

## Remaining Work (Ordered)

### 1. Merge kv_module PR #29 ⏳
- **Repo:** `jimmy-claw/logos-kv-module`
- **PR:** https://github.com/jimmy-claw/logos-kv-module/pull/29
- **CI:** ✅ passes (Build kv_module — 2m40s)
- **Action:** Review and merge. This unblocks re-adding kv_module as a scala dependency.

### 2. Add .lgx Packaging to Scala CI ✅ DONE 2026-07-16
✅ Commit `4ec84b1` — CI now produces scala.lgx + scala-ui.lgx artifacts
✅ Commit `946dc78` — Fixed variant: use `.#lgx-portable` → `linux-amd64` (was `linux-amd64-dev`, basecamp rejected it)
✅ All 5 CI jobs pass, both .lgx artifacts downloadable (~3.8 MB each, self-contained)

```yaml
# In .github/workflows/ci.yml — add after build-core and build-ui jobs:

package-lgx-core:
  name: "Package scala core as .lgx"
  runs-on: ubuntu-latest
  needs: build-core
  steps:
    - uses: actions/checkout@v4
    - uses: cachix/install-nix-action@v27
      with:
        extra_nix_config: |
          access-tokens = github.com=${{ secrets.GITHUB_TOKEN }}
    - name: Build .lgx package
      run: nix build -L --override-input logos-module-builder github:logos-co/logos-module-builder .#lgx
    - name: Upload .lgx artifact
      uses: actions/upload-artifact@v4
      with:
        name: scala.lgx
        path: result/

package-lgx-ui:
  name: "Package scala-ui as .lgx"
  runs-on: ubuntu-latest
  needs: build-ui
  steps:
    - uses: actions/checkout@v4
    - uses: cachix/install-nix-action@v27
      with:
        extra_nix_config: |
          access-tokens = github.com=${{ secrets.GITHUB_TOKEN }}
    - name: Build .lgx package
      run: cd scala-ui && nix build -L --override-input logos-module-builder github:logos-co/logos-module-builder --override-input scala path:.. .#lgx
    - name: Upload .lgx artifact
      uses: actions/upload-artifact@v4
      with:
        name: scala-ui.lgx
        path: result/
```

### 3. Merge Scala PR #79 ⏳
- **Depends on:** Step 2 (CI must include .lgx packaging)
- **Action:** After CI is green with .lgx steps, merge to main.

### 4. Integration Test in basecamp 0.2.0 ⏳
```bash
# Build basecamp
nix build github:logos-co/logos-basecamp/0.2.0#app -o /tmp/basecamp

# Install modules via lgpm
/tmp/basecamp/bin/lgpm install ./scala/result --as scala
/tmp/basecamp/bin/lgpm install ./scala-ui/result --as scala_ui

# Launch and verify
/tmp/basecamp/bin/logos-basecamp
```
- **Requires:** basecamp 0.2.0 available, working .lgx packages

### 5. Re-add kv_module Dependency ⏳
- **Depends on:** Step 1 (kv_module PR #29 merged)
- **Action:** 
  - Add `kv_module` back to `flake.nix` inputs
  - Add `"kv_module"` back to `metadata.json` dependencies
  - Re-enable KV persistence in `CalendarStore` (currently using in-memory fallback)
  - Verify build + tests pass

### 6. Wire messaging_module for P2P Sync ⏳
- **Depends on:** Steps 1-5 complete
- **Scope:** Replace `LogosAPIClient` calls in `calendar_sync.cpp` with `modules().messaging_module.*`
- **Testing:** Verify calendar sharing + invite link flow works end-to-end

---

## Blocked Items (Wait for External)

| Item | Blocked By |
|------|-----------|
| #34: At-rest KV encryption | kv-module encryption merge upstream |

---

## Quick Reference

| Resource | Location |
|----------|----------|
| Scala repo | `~/scala` → github.com/jimmy-claw/scala |
| kv_module repo | github.com/jimmy-claw/logos-kv-module |
| Scala PR #79 | https://github.com/jimmy-claw/scala/pull/79 |
| kv_module PR #29 | https://github.com/jimmy-claw/logos-kv-module/pull/29 |
| Build machine | Crib (192.168.0.152) — NOT Pi5 (SIGKILL) |
