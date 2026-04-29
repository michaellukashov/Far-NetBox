# Combo Box Dropdown Keyboard Shortcuts Fix

**Reference:** Dropdown keyboard shortcuts for `TFarComboBox` with `DIF_DROPDOWNLIST`  
**Created:** 2026-04-29  
**Status:** ✅ COMPLETE

## Goal
Add keyboard shortcuts to open combo box dropdown lists in NetBox session dialogs (and other dialogs using `TFarComboBox`).

## Context
NetBox session dialogs use `TFarComboBox` controls with `SetDropDownList(true)` (e.g., Protocol selection, FTP encryption). Users expect standard Windows shortcuts to open these dropdowns.

## Shortcuts Implemented

| Shortcut | Status | Implementation |
|----------|--------|---------------|
| **Alt+Down** | ✅ Complete | `SendDialogMessage(DM_SETDROPDOWNOPENED, 1)` in `TFarComboBox::ItemProc()` |
| **Ctrl+Shift+Down** | ✅ Complete | `DefaultItemProc(DM_SETDROPDOWNOPENED, 1)` in `TFarComboBox::ItemProc()` |
| **Ctrl+Down** | ⚠️ Known Limitation | Works only in terminals that don't intercept Ctrl+Down |

## Implementation

**File:** `src/NetBox/FarDialog.cpp`  
**Function:** `TFarComboBox::ItemProc()` (lines ~2621-2655)

Added `DN_CONTROLINPUT` handler detecting `VK_DOWN` with modifier masks. **All three shortcuts use `SendDialogMessage`** (`DefaultItemProc` / `DefDlgProc` does NOT handle `DM_SETDROPDOWNOPENED`):
- `ALTMASK` → `SendDialogMessage(DM_SETDROPDOWNOPENED, nb::ToPtr(1)); return 1;`
- `CTRLMASK + SHIFTMASK` → `SendDialogMessage(DM_SETDROPDOWNOPENED, nb::ToPtr(1)); return 1;`
- `CTRLMASK` only → `SendDialogMessage(DM_SETDROPDOWNOPENED, nb::ToPtr(1)); return 1;`

## Root Cause: Ctrl+Down Terminal Interception

Ctrl+Down fails in many terminal hosts (Windows Terminal, ConEmu) because they intercept it for viewport scrolling. The key event never reaches Far Manager or the plugin.

**Evidence:**
- Ctrl+Down works in plain `cmd.exe` without a terminal host
- Ctrl+Down fails even in native Far Manager dialogs (no plugin code involved)
- Alt+Down works everywhere because terminals don't typically intercept Alt+Arrow

## Workarounds for Users

1. **Use Alt+Down** — primary shortcut, works in all terminals
2. **Use Ctrl+Shift+Down** — fallback, works in most terminals that intercept plain Ctrl+Down
3. **Configure terminal** to pass Ctrl+Down through:
   - **Windows Terminal:** Add `"command": null, "keys": "ctrl+down"` to `settings.json`
   - **ConEmu:** Disable Ctrl+Down in Settings → Keys & Macro

## Related References

- [Combo Box Dropdown Keyboard Exploration](../references/combo-box-dropdown-keyboard-exploration.md) — detailed analysis of terminal interception, Far Manager native handling, and failed approaches
- `src/NetBox/FarDialog.cpp` — implementation
- `src/NetBox/FarDialog.h` — `TFarComboBox` class, control masks
- `src/NetBox/FarPlugin.h` — `ALTMASK`, `CTRLMASK`, `SHIFTMASK` definitions
- `src/NetBox/WinSCPDialogs.cpp` — session dialog combo box creation

## Build Verification

- ✅ x64 build: zero new warnings (FarDialog.cpp compiles successfully)
- ✅ x86 build: zero new warnings, complete success

## Test Results

| Test | Result |
|------|--------|
| Alt+Down opens Protocol dropdown | ✅ PASS |
| Ctrl+Shift+Down opens Protocol dropdown | ✅ PASS (user confirmed 2026-04-29) |
| Ctrl+Down opens Protocol dropdown (in affected terminals) | ❌ FAIL (known terminal limitation) |
| Plain Down still moves focus | ✅ PASS |
| Queue dialog unaffected | ✅ PASS |

## Commit

`feat(dialog): add Alt+Down and Ctrl+Down keyboard shortcuts to open combo box dropdown lists`

Modified: `src/NetBox/FarDialog.cpp`
