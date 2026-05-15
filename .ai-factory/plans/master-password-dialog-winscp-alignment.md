# Plan: Master Password Dialog — WinSCP Alignment

> Mode: Fast | Skip Tests | Logging: Verbose | Docs: Yes
> Created: 2026-05-07
> WinSCP Reference: 6.5.6 (`D:\Projects\WinSCP-work\winscp-master\source`)

---

## Goal

Align NetBox's Master Password dialog UX with WinSCP's `TMasterPasswordDialog`. The target is a clean 3-field change dialog:

```
┌──────────────────────────────────────────┐
│ Master password                          │
├──────────────────────────────────────────┤
│ Current master password:  [_________]    │
│ New master password:      [_________]    │
│ Re-enter master password: [_________]    │
├──────────────────────────────────────────┤
│                  [ OK ]  [ Cancel ]      │
└──────────────────────────────────────────┘
```

OK is **disabled** until all fields are valid. Clicking OK validates current password, confirms passwords match, checks strength, then commits the change.

---

## Current State vs WinSCP

| Aspect | WinSCP (`UserInterface.cpp:1260`) | NetBox Far-UI (`WinSCPDialogs.cpp:1058`) | Gap |
|--------|-----------------------------------|------------------------------------------|-----|
| **Dialog type** | Standalone modal, 3 fields only | Single checkbox-driven dialog (set/change/clear in one) | Mixed-purpose |
| **OK button** | Disabled until all fields pass `IsValidPassword() >= 0` | Always enabled | **UX gap — no proactive validation** |
| **Validation timing** | `DoChange()` on every keystroke for enable; `DoValidate()` on submit | All validation deferred to post-`ShowModal()` | **UX gap** |
| **Fields** | 3 × `TPasswordEdit` | 3 × `TFarEdit` (password mode) | Match |
| **Labels / MsgIDs** | `MASTER_PASSWORD_CURRENT/NEW/CONFIRM` | `NB_MASTER_PASSWORD_CURRENT/NEW/CONFIRM` | Match |
| **Validation logic** | Same (`ValidateMasterPassword`, `IsValidPassword`) | Same | Match |
| **Backend** | `WinConfiguration->ChangeMasterPassword()` | Same | Match |

### Key Insight

The **backend is already fully aligned** — same `ValidateMasterPassword()`, `ChangeMasterPassword()`, `ClearMasterPassword()`, same `IsValidPassword()`, same MsgIDs. The gap is purely **UI behavior**: the OK button is always enabled in Far-UI, and the dialog mixes set/change/clear into one checkbox-driven form.

---

## Architecture

All changes are in the Plugin Layer only. No changes to core, windows, or third-party layers.

```
Plugin Layer (src/NetBox/)
  WinSCPDialogs.cpp → MasterPasswordConfigurationDialog() [MODIFY]
       ↓ (calls)
Windows Layer (src/windows/)
  WinConfiguration → ValidateMasterPassword(), ChangeMasterPassword() [EXISTING]
  MasterPassword.cpp [EXISTING]
Core Layer (src/core/)
  Cryptography.cpp → IsValidPassword() [EXISTING]
```

---

## Files to Modify

| File | Change | Description |
|------|--------|-------------|
| `src/NetBox/WinSCPDialogs.cpp` | **MODIFY** | Add real-time OK enable/disable via `Change()` override; reorder validation |

**No changes to** `src/windows/UserInterface.cpp` — the VCL `TMasterPasswordDialog` already implements real-time OK enable/disable.
---

## Implementation Plan

### Approach

WinSCP has **three separate dialogs**: Set new, Change, Clear. NetBox has **one checkbox-driven dialog**. For Far Manager compatibility, we keep the single-dialog approach but improve it in two ways:

1. **Real-time OK enable/disable** — match WinSCP's `DoChange()` pattern
2. **Cleaner mode separation** — the checkbox toggle already handles this; ensure each mode has correct field visibility and enablement

### Change 1: Real-time OK button enable/disable

**Target:** `MasterPasswordConfigurationDialog()` at WinSCPDialogs.cpp:1058

**WinSCP pattern** (`UserInterface.cpp:1347-1351`):
```cpp
void TMasterPasswordDialog::DoChange(bool &CanSubmit)
{
  CanSubmit = (!UseMP || IsValidPassword(CurrentEdit->Text) >= 0) &&
              ((NewEdit == nullptr) || IsValidPassword(NewEdit->Text) >= 0) &&
              ((ConfirmEdit == nullptr) || IsValidPassword(ConfirmEdit->Text) >= 0);
}
```

**NetBox equivalent:**
- Override the virtual `Change()` method on the dialog class (pattern: `TLinkDialog::Change()` at WinSCPDialogs.cpp:6829, `TPropertiesDialog::Change()` at line 5782)
- `OkButton->SetEnabled(CanSubmit)` dynamically in `Change()`
|- Logic: OK enabled when all visible password fields pass `IsValidPassword() >= 0` (non-empty; weak passwords are caught at submit time with a warning, matching WinSCP)
**Note:** Checkbox toggle (DN_BTNCLICK) does not trigger `Change()` automatically. The checkbox click handler must also call the OK update logic.

**Change mode** (checkbox ON + existing MP):
```

CanSubmit = IsValidPassword(CurrentEdit->Text) >= 0 &&
            IsValidPassword(NewEdit->Text) >= 0 &&
            IsValidPassword(ConfirmEdit->Text) >= 0;
```

**Set mode** (checkbox ON + no MP):
```
CanSubmit = IsValidPassword(NewEdit->Text) >= 0 &&
            IsValidPassword(ConfirmEdit->Text) >= 0;
```

**Clear mode** (checkbox OFF + has MP):
```
CanSubmit = IsValidPassword(CurrentEdit->Text) >= 0;
```

**Edge case:** New==Confirm is NOT checked for button enablement (matching WinSCP — that's checked at submit time via `DoValidate()`).

### Change 2: Validation order alignment

**Current NetBox order** (change branch):
1. Check `NewPwd.IsEmpty()`
2. Check `ValidateMasterPassword(CurrentPwd)`
3. Check `NewPwd != ConfirmPwd`
4. Warn if weak password

**WinSCP order** (`DoValidate()` at UserInterface.cpp:1367-1398):
1. Check `ValidateMasterPassword(CurrentPwd)` — **first**, not after empty check
2. Check `NewPwd != ConfirmPwd`
3. Warn if weak password (`IsValidPassword(NewPwd) <= 0`)
4. (Empty check is handled implicitly by OK disablement — OK is never enabled if fields empty)

**Fix:** Reorder NetBox validation to: (1) ValidateMasterPassword, (2) password match, (3) strength warning. Remove redundant empty checks since OK is now disabled when fields are empty.

### Change 3: Dialog structure — optional enhancement

Keep the checkbox-driven approach. The checkbox already provides clean mode separation:
- Checkbox + CurrentEdit visibility = change mode
- Checkbox + no CurrentEdit = set mode
- Unchecked + CurrentEdit = clear mode

This is a valid Far Manager adaptation of WinSCP's three separate dialogs. No structural change needed.

---

## Tasks

### Task 1: Add real-time OK button enable/disable

**Target:** `src/NetBox/WinSCPDialogs.cpp` — `MasterPasswordConfigurationDialog()`

- Override the virtual `Change()` method on the dialog class (pattern: `TLinkDialog::Change()` at WinSCPDialogs.cpp:6829)
- Implement `UpdateOkButton()` called from `Change()` and from the checkbox click handler:
  - Determines current mode (set/change/clear) from `UseMP` and checkbox state
  - Evaluates whether all visible fields pass `IsValidPassword() >= 0` (non-empty)
  - Calls `OkButton->SetEnabled(CanSubmit)`
|- On dialog init: call `UpdateOkButton()` to set initial OK state (disabled)

**Logging:** `AppLogFmt(L"MasterPassword OK enabled=%d (mode=%d)", enabled, mode)`

**Verification:** OK starts disabled, becomes enabled when all visible fields are non-empty. Weak passwords (0) still enable OK; strength is checked at submit time with a warning dialog. Toggling checkbox re-evaluates. Clear mode enables OK when current password field is non-empty.

---

### Task 2: Reorder validation to match WinSCP

**Target:** `src/NetBox/WinSCPDialogs.cpp` — change branch (~lines 1130-1158), set branch (~lines 1168-1189), clear branch (~lines 1196-1220)

**Change:**
- **Change branch:** Remove `NewPwd.IsEmpty()` check (handled by OK disablement). Reorder: (1) `ValidateMasterPassword(CurrentPwd)` first, (2) `NewPwd != ConfirmPwd` second, (3) `IsValidPassword(NewPwd)` warning third.
- **Set branch:** Remove `NewPwd.IsEmpty()` check. Reorder to: (1) `NewPwd != ConfirmPwd` first, (2) `IsValidPassword(NewPwd)` warning second.
- **Clear branch:** Remove `CurrentPwd.IsEmpty()` check. Just: (1) `ValidateMasterPassword(CurrentPwd)`, (2) clear.

**Logging:** Log each validation step result at verbose level.

**Verification:** Wrong current password → error shown first. Mismatched passwords → error shown after. Weak password → warning with OK/Cancel.

---

### Task 3: Verify MsgIDs match WinSCP

**Target:** `src/NetBox/NetBoxEng.lng` (validation only — no changes expected)

**Check:**
|- `NB_MASTER_PASSWORD_CURRENT` = "&Current master password:" ✓
|- `NB_MASTER_PASSWORD_NEW` = "&New master password:" ✓
|- `NB_MASTER_PASSWORD_CONFIRM` = "&Re-enter master password:" ✓
|- `NB_MASTER_PASSWORD_INCORRECT` ✓
|- `NB_MASTER_PASSWORD_DIFFERENT` ✓
|- `NB_MASTER_PASSWORD_SIMPLE2` ✓
|- `NB_MASTER_PASSWORD_CHANGED` ✓
|- `NB_MASTER_PASSWORD_SET2` ✓
|- `NB_MASTER_PASSWORD_CLEARED2` ✓

All MsgIDs already match WinSCP's resource strings. No changes expected.

**Status:** Already implemented in commit `41691bb2f`.

---

### Task 4: Wire `[ Change master password... ]` button to checkbox state

**Target:** `src/NetBox/WinSCPDialogs.cpp` — `TSecurityConfigurationDialog::Change()`

**Gap found:** `FChangeMpBtn` is enabled at dialog init (`Execute()`) and after returning from `TMasterPasswordDialog` (`ChangeMpBtnClick()`), but NOT when the user toggles `FUseMpCheck` in real-time.

**Fix:**
|- In `TSecurityConfigurationDialog::Change()`, add `FChangeMpBtn->SetEnabled(FUseMpCheck->GetChecked());` inside the `if (GetHandle())` block.

**Verification:** Toggling "Use master password" immediately enables/disables the "Change master password..." button. Button remains correctly enabled after returning from the change dialog.

---

### Task 6: Save `[x] Use master password` checkbox state

**Target:** `src/NetBox/WinSCPDialogs.cpp` — `TSecurityConfigurationDialog::Execute()`

**Gap found:** `FUseMpCheck` checkbox state is read from `WinConfiguration->GetUseMasterPassword()` on dialog open, but never saved back when the dialog is confirmed with OK.

**Fix:**
|- After `ShowModal() == brOK`, compare `FUseMpCheck->GetChecked()` with initial `UseMP` state.
|- If checked (was unchecked): open `TMasterPasswordDialog` in set mode (`UseMP=false`) to set a new password.
|- If unchecked (was checked): call `WinConfiguration->ClearMasterPassword()` to disable master password and recrypt stored passwords.
|- Handle recryption errors with appropriate warning dialogs.
|- Log each action at verbose level.

**Verification:** Checking "Use master password" and clicking OK opens the master password set dialog. Unchecking and clicking OK clears the master password with confirmation. The checkbox state persists across dialog reopening.

---

### Task 5: Fix dialog vertical padding

**Target:** `src/NetBox/WinSCPDialogs.cpp` — `TMasterPasswordDialog` constructor

**Gap found:** Dialog height `18` leaves ~7 empty rows below the buttons. Content (max 7 rows) + separator/buttons (2 rows) + borders (2 rows) needs only ~12 rows.

**Fix:**
|- Reduce `SetSize(TPoint(70, 18))` to `SetSize(TPoint(70, 12))`.

**Verification:** Dialog renders compactly with no trailing empty rows in either set mode or change mode.
---

## Commit Plan

1. `fix(ui): add real-time OK enable/disable to master password dialog` — Task 1
2. `fix(ui): reorder master password validation to match WinSCP` — Task 2
3. `fix(ui): wire Change master password button to Use master password checkbox` — Task 4
4. `fix(ui): reduce excessive vertical padding in master password dialog` — Task 5

---

## Edge Cases

1. **All fields empty on open** — OK disabled, standard state
2. **Current password valid but New==Confirm mismatch** — OK enabled (matches WinSCP), mismatch caught at submit

3. **Weak password** — OK enabled even for weak passwords (`IsValidPassword` returns 0), matching WinSCP's `>= 0` threshold. Strength check runs at submit time via `IsValidPassword(NewPwd) <= 0` warning with OK/Cancel. User can proceed with a weak password after confirming.
4. **Checkbox toggle mid-typing** — OK state re-evaluated immediately
5. **Dialog cancel** — No changes committed
6. **RecryptPasswords stub** — `ChangeMasterPassword()` calls `RecryptPasswords()` internally. MSVC stub means stored passwords won't be recrypted. Not in scope for this plan (tracked in `master-password-infrastructure.md`).

---

## Out of Scope

- Splitting into 3 separate dialogs (checkbox-driven approach is the Far Manager adaptation)
- RecryptPasswords MSVC stub fix (separate plan)
- Adding rate limiting to `ValidateMasterPassword()` (separate plan)
- Adding `TSecureString` for plaintext passwords (future work)

---

## Research Context

Source: WinSCP `TMasterPasswordDialog` at `D:\Projects\WinSCP-work\winscp-master\source\windows\UserInterface.cpp:1260-1418`
- `Init(bool Current)`: `Current=false` → 3-field change dialog
- `DoChange()`: real-time OK enable via `IsValidPassword() >= 0`
- `DoValidate()`: `ValidateMasterPassword()` → `New != Confirm` → strength warning
- Backend: `WinConfiguration->ChangeMasterPassword()`, `ValidateMasterPassword()`, `IsValidPassword()`

Source: `.ai-factory/references/master-password-infrastructure-research.md`
- Backend already aligned: same validation methods, same `ChangeMasterPassword` flow
- RecryptPasswords stub is a known issue, separate from this UI alignment