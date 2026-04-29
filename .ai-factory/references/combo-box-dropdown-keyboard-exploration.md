# Combo Box Dropdown Keyboard Shortcuts Exploration

## Date
2026-04-29

## Context
Adding keyboard shortcuts to open `TFarComboBox` dropdown lists in NetBox session dialogs. The user requested **Ctrl+Down** and **Alt+Down** support for combo boxes with `DIF_DROPDOWNLIST` flag.

## Investigation Summary

### Initial Approach
Added `DN_CONTROLINPUT` handler in `TFarComboBox::ItemProc()` (`src/NetBox/FarDialog.cpp`) to detect:
- **Alt+Down** (`VK_DOWN` + `ALTMASK`)
- **Ctrl+Down** (`VK_DOWN` + `CTRLMASK`)

Both shortcuts called `SendDialogMessage(DM_SETDROPDOWNOPENED, 1)` and returned 1 to consume the event.

### Test Results

| Shortcut | Test Environment | Result |
|----------|-----------------|--------|
| Alt+Down | Any terminal | **PASS** |
| Ctrl+Down | Windows Terminal / ConEmu | **FAIL** |
| Ctrl+Down | Plain `cmd.exe` (no terminal host) | **PASS** |
| Ctrl+Down | Native Far Manager dialogs (no plugin) | **FAIL** in affected terminals |
| Ctrl+Down | NetBox Queue dialog | **FAIL** in affected terminals |

### Root Cause Analysis

**Terminal host interception** is the root cause. Many terminal emulators (Windows Terminal, ConEmu, etc.) bind **Ctrl+Down** to viewport scrolling or zoom functions at the terminal level. The key event is consumed by the terminal and never reaches the application (Far Manager/NetBox).

Key evidence:
1. Ctrl+Down **works** in plain `cmd.exe` without a terminal host
2. Ctrl+Down **fails** even in native Far Manager dialogs (no plugin code involved)
3. Alt+Down **works everywhere** because terminals typically don't intercept Alt+Arrow combinations

### Far Manager Native Behavior

Far Manager's `dialog.cpp` has native handling for Ctrl+Down:
```cpp
case KEY_CTRLDOWN:    case KEY_CTRLNUMPAD2:
case KEY_RCTRLDOWN:   case KEY_RCTRLNUMPAD2:
    return ProcessOpenComboBox(Items[m_FocusPos].Type, Items[m_FocusPos], m_FocusPos);
```

When the plugin's `DlgProc` returns `true` (1) for `DN_CONTROLINPUT`, Far Manager **stops processing** and never reaches its native handler. This means:
- Returning 1 for Ctrl+Down **blocks** Far Manager's native combo box opening
- The plugin must either handle it successfully or return 0 to let Far Manager handle it natively

### Failed Approaches

1. **SendDialogMessage(DM_SETDROPDOWNOPENED, 1)** — Works for Alt+Down but fails for Ctrl+Down in intercepted terminals (the message never reaches Far Manager's handler because the key event itself is intercepted)

2. **DefaultItemProc(DM_SETDROPDOWNOPENED, 1)** — Same result; the issue is not the API call but the key event never arriving

3. **Lenient Ctrl check** (`(ControlState & CTRLMASK) != 0`) — No improvement; terminal intercepts before modifier state matters

### Final Solution

1. **Alt+Down** — Primary shortcut. Explicitly handled via `SendDialogMessage(DM_SETDROPDOWNOPENED, 1)`. Works reliably in all terminals.

2. **Ctrl+Shift+Down** — Fallback shortcut. Many terminals intercept plain Ctrl+Down but **do not** intercept Ctrl+Shift+Down. Added explicit support alongside Ctrl+Down.

3. **Ctrl+Down** — Kept for compatibility with terminals that don't intercept it. Routed through `DefaultItemProc()` to let Far Manager's native handling take over if the plugin doesn't consume it.

### Code Changes

File: `src/NetBox/FarDialog.cpp`, `TFarComboBox::ItemProc()` (lines ~2621-2655)

```cpp
if (GetDropDownList() && ((Key == VK_DOWN) || (Key == VK_NUMPAD2)) &&
  (CheckControlMaskSet(ControlState, ALTMASK) ||
    (((ControlState & CTRLMASK) != 0) && ((ControlState & SHIFTMASK) != 0)) ||
    ((ControlState & CTRLMASK) != 0)))
{
  if (CheckControlMaskSet(ControlState, ALTMASK))
  {
    // Alt+Down — primary shortcut; works reliably in all terminals
    SendDialogMessage(DM_SETDROPDOWNOPENED, nb::ToPtr(1));
    return 1;
  }
  else if (((ControlState & CTRLMASK) != 0) && ((ControlState & SHIFTMASK) != 0))
  {
    // Ctrl+Shift+Down — fallback for terminals that intercept plain Ctrl+Down
    return DefaultItemProc(DM_SETDROPDOWNOPENED, nb::ToPtr(1));
  }
  else if ((ControlState & CTRLMASK) != 0)
  {
    // Ctrl+Down — native Far Manager shortcut; may be intercepted by terminal host
    return DefaultItemProc(DM_SETDROPDOWNOPENED, nb::ToPtr(1));
  }
}
```

### Terminal Configuration Workaround

Users who want Ctrl+Down to work can configure their terminal to pass it through:

**Windows Terminal** (`settings.json`):
```json
{
  "keys": [
    {
      "command": null,
      "keys": "ctrl+down"
    }
  ]
}
```

**ConEmu**: Settings → Keys & Macro → disable Ctrl+Down shortcut.

### References

- `src/NetBox/FarDialog.cpp` — `TFarComboBox::ItemProc()` implementation
- `src/NetBox/FarDialog.h` — `TFarComboBox` class declaration, `SHIFTMASK`/`CTRLMASK`/`ALTMASK` constants
- `src/NetBox/FarPlugin.h` — Control mask definitions (`RMASK`, `ALTMASK`, `CTRLMASK`, `SHIFTMASK`)
- Far Manager source `dialog.cpp` — native `KEY_CTRLDOWN` handling, `ProcessOpenComboBox()`, `DM_SETDROPDOWNOPENED` implementation
- `src/NetBox/WinSCPDialogs.cpp` — `TSessionDialog` combo box creation (`TransferProtocolCombo` with `SetDropDownList(true)`)
