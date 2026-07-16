# Scala v0.3 Migration Plan

**Date:** 2026-07-16
**Status:** In Progress
**Target:** Fix persistence, adopt Logos design system, migrate sync to LMAO

---

## Problem Statement

1. **No persistence** — CalendarStore falls back to in-memory `QMap` because `kv_module` is not wired in the universal pattern (basecamp 0.2.0). Data vanishes on restart.
2. **Hardcoded UI** — QML views use hardcoded colors (#2196F3, #4CAF50) and standard QtQuick.Controls instead of Logos design system components.
3. **Sync uses delivery_module** — CalendarSync talks to `delivery_module` via `LogosAPIClient`. Should use LMAO (logos-messaging-a2a) for proper P2P sync with encryption, presence, and reliability.

---

## Phase 1: Persistence (CRITICAL — blocks everything)

### Root Cause
`CalendarStore` uses `#ifdef LOGOS_CORE_AVAILABLE` to decide between kv_module and in-memory fallback. In the universal pattern build, this macro is NOT defined, so ALL data lives in `QMap<QString, QString> m_mem` → lost on exit.

### Solution: File-based LocalStorage (standalone, no kv_module dependency)

**New file:** `src/local_storage.h/.cpp`
- SQLite or flat-file JSON store using `QStandardPaths::AppDataLocation`
- Simple key-value API matching the current `CalendarStore::kvSet/kvGet/kvRemove`
- Automatic save on every write, load on construction
- Zero external dependencies (pure Qt)

**Changes to CalendarStore:**
- Remove `#ifdef LOGOS_CORE_AVAILABLE` branching
- Always use LocalStorage as the backend
- Keep kv_module interface for future when it's available in universal pattern (adapter pattern)

**Files to modify:**
- `src/calendar_store.h` — add LocalStorage pointer, remove LOGOS_CORE conditional
- `src/calendar_store.cpp` — wire through LocalStorage
- `src/local_storage.h` — NEW: file-based KV store
- `src/local_storage.cpp` — NEW: implementation
- `CMakeLists.txt` — add new source files

### Acceptance Criteria
- Create events → restart app → events still there ✅
- No external dependencies (works standalone and in basecamp)

---

## Phase 2: Logos Design System Migration

### Current State
All QML files use:
- Hardcoded colors (`#2196F3`, `#4CAF50`, `#f9f9f9`, etc.)
- Standard `QtQuick.Controls` components (Button, TextField, ComboBox, etc.)
- Manual styling in every component

### Target State
All QML files use:
- `import Logos.Theme` — `Theme.palette.*` for all colors
- `import Logos.Controls` — `LogosButton`, `LogosText`, `LogosTextField`, etc.
- Consistent typography via `Theme.typography.*`
- Consistent spacing via `Theme.spacing.*`

### Design System Components Available (map to Scala usage)

| Current Qt Component | Logos Replacement | Notes |
|---------------------|-------------------|-------|
| `Button` | `LogosButton` | Primary/Secondary variants, icon support |
| `Text` | `LogosText` | Themed colors, typography tokens |
| `TextField` | `LogosTextField` | Consistent styling |
| `TextArea` | `LogosTextArea` | Consistent styling |
| `ComboBox` | `LogosComboBox` | Themed dropdown |
| `Switch` | `LogosSwitch` | Themed toggle |
| `CheckBox` | `LogosCheckbox` | Themed checkbox |
| `SpinBox` | `LogosSpinBox` | Themed number input |
| `Popup/Dialog` | `LogosDialog` | Themed modal dialog |
| `Rectangle` (backgrounds) | Use `Theme.palette.background*` colors | Replace hardcoded hex |
| Custom styled buttons | `LogosButton` with variant | Remove manual backgrounds |

### Files to Migrate (in order of impact)

1. **`qml/CalendarView.qml`** — main view, toolbar, search, week/day views (~1330 lines, biggest file)
2. **`qml/EventModal.qml`** — event creation/edit popup
3. **`qml/CalendarSidebar.qml`** — sidebar with calendar list
4. **`qml/CalendarGrid.qml`** — month grid view
5. **`qml/EventDetails.qml`** — event detail panel
6. **`qml/ShareDialog.qml`** — share calendar dialog
7. **`qml/SettingsPanel.qml`** — settings panel

### Migration Pattern (per file)

```qml
// BEFORE
import QtQuick.Controls 2.15

Rectangle {
    color: "#2196F3"
    Button {
        text: "Save"
        background: Rectangle { color: "#4CAF50" }
        contentItem: Text { color: "white" }
    }
}

// AFTER
import Logos.Theme
import Logos.Controls

LogosFrame {
    // Use Theme.palette.primary instead of hardcoded colors
    LogosButton {
        text: "Save"
        variant: LogosButton.Variant.Primary
    }
}
```

### Acceptance Criteria
- All hardcoded hex colors replaced with `Theme.palette.*` references
- All standard Qt controls replaced with Logos equivalents
- UI renders correctly in dark theme (only available theme)
- No visual regression — layout and functionality preserved

---

## Phase 3: LMAO-based Sync

### Current State
`CalendarSync` uses `delivery_module` via `LogosAPIClient::invokeRemoteMethod`:
- Manual topic management (`/scala/1/<calendarId>/json`)
- Manual subscription handling
- No encryption (encryption key stored but not used for actual encryption)
- No presence/discovery — must know peer's calendar ID

### Target State
Use LMAO primitives for calendar sync:
- **WakuA2ANode** as the transport layer (via C FFI or direct integration)
- **X25519 + ChaCha20** encryption for shared calendar messages
- **Presence discovery** — find peers who have Scala installed
- **SDS reliability** — automatic retransmission, deduplication
- **Storage offload** — large payloads (calendar exports) via Logos Storage

### Architecture

```
CalendarView (QML)
    │
ScalaUiBackend (C++)
    │  modules().scala.*
ScalaImpl (core module)
    │
CalendarSync → LMAOSync (NEW)
    │
WakuA2ANode (via lmao-ffi or direct Rust→C bridge)
    │
Logos Messaging Network
```

### Implementation Approach

**Option A: C FFI wrapper (recommended)**
- Use `lmao-ffi` crate to expose LMAO as a C library
- CalendarSync calls into the C library for send/receive/presence
- Minimal changes to Scala code — just replace delivery_module calls with FFI calls

**Option B: Direct Rust module (future)**
- Rewrite Scala core in Rust (much larger effort)
- Full LMAO integration without FFI overhead

### New Files
- `src/lmao_sync.h/.cpp` — C++ wrapper around lmao-ffi
- `src/calendar_sync_v2.h/.cpp` — new sync implementation using LMAOSync

### Changes to CalendarSync
- Replace `delivery_module` calls with LMAO FFI calls
- Use proper encryption (ChaCha20) instead of HMAC-only signatures
- Add presence announcement for Scala peers
- Implement proper message deduplication (SDS handles this)

### Acceptance Criteria
- Two Scala instances can sync events via LMAO ✅
- Messages are encrypted end-to-end ✅
- Presence discovery works (find other Scala users) ✅
- Graceful degradation when offline (queue messages locally)

---

## Execution Order

1. **Phase 1: Persistence** (~2 hours) — CRITICAL, unblocks testing
2. **Phase 2: Design System** (~4 hours) — visual improvement, no logic changes
3. **Phase 3: LMAO Sync** (~6 hours) — requires FFI setup, most complex

---

## Risk Assessment

| Risk | Mitigation |
|------|-----------|
| LocalStorage corrupts data | JSON validation on load, backup before write |
| Design system breaks layout | Incremental migration per file, test after each |
| LMAO FFI not ready | Keep delivery_module as fallback (#ifdef) |
| Build fails on Crib | Test builds incrementally, commit working state |

---

## Git Strategy

- Branch: `feat/v0.3-persistence-design-sync`
- Commits per phase (not mixed):
  - `feat: add LocalStorage for file-based persistence`
  - `refactor: CalendarStore always uses LocalStorage`
  - `style: migrate CalendarView to Logos design system`
  - `style: migrate EventModal to Logos design system`
  - `style: migrate remaining QML files to Logos design system`
  - `feat: add LMAO sync wrapper (lmao_sync)`
  - `refactor: CalendarSync uses LMAO for P2P messaging`
