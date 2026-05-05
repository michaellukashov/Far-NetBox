# Plan: Preferences/Security Tab — WinSCP Alignment

> Mode: Fast | Skip Tests | Logging: Verbose | Docs: Yes
> Created: 2026-05-07
> WinSCP Reference: 6.5.6 (`D:\Projects\WinSCP-work\winscp-master\source`)

---

## Goal

Implement the **Preferences/Security Tab** dialog in NetBox, aligning with WinSCP's Security preferences sheet. This consolidates three currently-scattered security settings (master password, session password, trusted host CAs) into a single coherent configuration dialog, and adds the **missing Trusted Host CA management UI**.

---

## Current State vs WinSCP

| Feature | WinSCP | NetBox Current | Gap |
|---------|--------|----------------|-----|
| **Master Password** | SecuritySheet: checkbox + "Change master password…" button | Standalone `MasterPasswordConfigurationDialog()` in WinSCPDialogs.cpp:1058 | Split into its own menu item; needs consolidation into Security tab |
| **Session Remember Password** | SecuritySheet: checkbox | `RememberPasswordCheck` in TransferConfigurationDialog (WinSCPDialogs.cpp:775) | Wrong dialog; should be in Security tab |
| **Trusted Host CAs — Load from PuTTY** | SecuritySheet: checkbox `SshHostCAsFromPuTTYCheck` | `FSshHostCAsFromPuTTY` exists in `TConfiguration` but NO UI | Zero UI — data model only |
| **Trusted Host CAs — List (Name/Hosts)** | SecuritySheet: OwnerData ListView (Name, Hosts columns) | `TSshHostCA`/`TSshHostCAList` in Configuration.h fully implemented | Zero UI |
| **Trusted Host CAs — Add/Edit/Remove** | `DoSshHostCADialog()` — custom dialog with Name, PublicKey, ValidityExpression, PermitRsaSha* checkboxes, Browse button | `NB_SSH_HOST_CA_*` MsgIDs exist in MsgIDs.h:377-390 | Zero UI |
| **Trusted Host CAs — Configure in PuTTY** | Button that launches PuTTY with `-host-ca` | N/A | Missing |

### Key Insight

The data model is **fully implemented** in NetBox. All backend properties exist:
- `TWinConfiguration::FUseMasterPassword` — master password state
- `TGUIConfiguration::FSessionRememberPassword` — session password caching
- `TConfiguration::FSshHostCAList` — user's trusted CA list
- `TConfiguration::FPuttySshHostCAList` — PuTTY's trusted CA list (lazy-loaded)
- `TConfiguration::FSshHostCAsFromPuTTY` — toggle between user/PuTTY list
- `TSshHostCA` struct: Name, PublicKey, ValidityExpression, PermitRsaSha1/256/512

The **only gap is UI**. All MsgIDs for SSH host CA strings are already defined.

---

## Architecture

### Dependency Flow
```
Plugin Layer (src/NetBox/)
  WinSCPDialogs.cpp → SecurityConfigurationDialog() [NEW]
  WinSCPPlugin.cpp → ConfigureEx() [MODIFY: replace MMasterPassword with MSecurity]
  MsgIDs.h → NB_CONFIG_SECURITY, new IDs [ADD]
  guid.h → SecurityConfigurationDialogGuid [ADD]
  FarPluginStrings.cpp → string mapping [ADD]
  *.lng → localized strings [ADD]
       ↓
Core Layer (src/core/)
  Configuration.h/.cpp → TSshHostCA, TSshHostCAList [EXISTING, no changes]
       ↓
Windows Layer (src/windows/)
  WinConfiguration → UseMasterPassword, ChangeMasterPassword [EXISTING, no changes]
  GUIConfiguration → SessionRememberPassword [EXISTING, no changes]
  MasterPassword.cpp → ValidateMasterPassword, etc. [EXISTING, no changes]
```

### Dialog Design (Far Manager Text Mode)

Unlike WinSCP's GUI (VCL forms with ListView), NetBox uses `TFarDialog` with text-mode controls. The Security tab will be a **standalone flat dialog** (following NetBox's existing preference dialog pattern — not tabbed), containing:

```
╔═══════════════════════════════════════════════════════════════╗
║ NetBox - Security                                             ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║ ■ Master password                                             ║
║   [ ] Use master password                                     ║
║   [Change master password…]                                   ║
║                                                               ║
║ ■ Session password                                            ║
║   [ ] Remember password for duration of session               ║
║                                                               ║
║ ■ Trusted host certification authorities                      ║
║   [ ] Load authorities from PuTTY                             ║
║   ┌─────────────────────────────────────────────────────┐     ║
║   │ Name                │ Hosts                          │     ║
║   │─────────────────────│───────────────────────────────│     ║
║   │                     │                               │     ║
║   └─────────────────────────────────────────────────────┘     ║
║   [Add…]  [Edit…]  [Remove]                                   ║
║                                                               ║
╠═══════════════════════════════════════════════════════════════╣
║                    [ OK ]  [ Cancel ]                          ║
╚═══════════════════════════════════════════════════════════════╝
```

### SSH Host CA Detail Dialog (Add/Edit)

A separate sub-dialog launched when Add or Edit is clicked:

```
╔═════════════════════════════════════════════════════╗
║ Add Trusted Host Certification Authority             ║
╠═════════════════════════════════════════════════════╣
║ Name:        [________________________]              ║
║ Public key:  [________________________]  [Browse…]   ║
║ Hosts:       [________________________]              ║
║ [ ] Permit RSA-SHA1                                ║
║ [ ] Permit RSA-SHA256                             ║
║ [ ] Permit RSA-SHA512                             ║
╠═════════════════════════════════════════════════════╣
║              [ OK ]  [ Cancel ]                      ║
╚═════════════════════════════════════════════════════╝
```

---

## Files to Modify

| File | Change Type | Description |
|------|-------------|-------------|
| `src/NetBox/WinSCPDialogs.cpp` | **ADD** | `SecurityConfigurationDialog()`, `SshHostCADialog()` |
| `src/NetBox/WinSCPPlugin.cpp` | **MODIFY** | Replace `MMasterPassword` menu item with `MSecurity` |
| `src/NetBox/WinSCPPlugin.h` | **MODIFY** | Replace `MasterPasswordConfigurationDialog()` declaration with `SecurityConfigurationDialog()` |
| `src/base/MsgIDs.h` | **ADD** | `NB_CONFIG_SECURITY`, `NB_SECURITY_MASTER_PASSWORD_GROUP`, `NB_SECURITY_USE_MASTER_PASSWORD`, `NB_SECURITY_CHANGE_MASTER_PASSWORD`, `NB_SECURITY_SESSION_PASSWORD_GROUP`, `NB_SECURITY_REMEMBER_PASSWORD`, `NB_SECURITY_SSH_HOST_CA_GROUP`, `NB_SECURITY_SSH_HOST_CA_FROM_PUTTY`, `NB_SECURITY_SSH_HOST_CA_NAME_COL`, `NB_SECURITY_SSH_HOST_CA_HOSTS_COL`, `NB_SECURITY_SSH_HOST_CA_ADD_BTN`, `NB_SECURITY_SSH_HOST_CA_EDIT_BTN`, `NB_SECURITY_SSH_HOST_CA_REMOVE_BTN` (CA detail dialog reuses existing `NB_SSH_HOST_CA_*` IDs 377-390) |
| `src/NetBox/guid.h` | **ADD** | `SecurityConfigurationDialogGuid`, `SshHostCADialogGuid` |
| `src/NetBox/FarPluginStrings.cpp` | **ADD** | Map new MsgIDs to .lng ordinals |
| `src/NetBox/NetBoxEng.lng` | **ADD** | English strings for all new IDs |
| `src/NetBox/NetBoxRus.lng` | **ADD** | Russian strings |
| `src/NetBox/NetBoxFr.lng` | **ADD** | French strings |
| `src/NetBox/NetBoxPol.lng` | **ADD** | Polish strings |
| `src/NetBox/NetBoxSpa.lng` | **ADD** | Spanish strings |

**No changes to core layer** — `TSshHostCA`, `TSshHostCAList`, `TConfiguration` are complete.

---

## Tasks

### Task 1: Add new MsgIDs and GUID definitions

**Target:** `src/base/MsgIDs.h`, `src/NetBox/guid.h`

- Add `NB_CONFIG_SECURITY` enum value after existing `NB_CONFIG_PANEL` (line ~161)
- Add new Security parent dialog MsgIDs (group labels, column headers, button captions) after existing `NB_SSH_HOST_CA_*` block (line ~390)
- **Reuse** existing `NB_SSH_HOST_CA_*` IDs (377-390) for the CA detail sub-dialog — these cover fields (Name, PublicKey, Hosts, Browse, validation messages, signature types) already defined
- Add `SecurityConfigurationDialogGuid` and `SshHostCADialogGuid` DEFINE_GUIDs in `guid.h`

**Verification:** Build succeeds with zero warnings.

---

### Task 2: Add localized strings to all .lng files

**Target:** `src/NetBox/NetBoxEng.lng`, `NetBoxRus.lng`, `NetBoxPol.lng`, `NetBoxFr.lng`, `NetBoxSpa.lng`, `src/NetBox/FarPluginStrings.cpp`

**Change:**

- Add English strings for all **new** MsgIDs (group labels, column headers, Security-dialog button captions). CA detail dialog fields reuse existing `NB_SSH_HOST_CA_*` IDs already in `.lng` files — no new strings needed there.
- Add placeholder strings for RU/FR/POL/SPA (English fallback acceptable)
- Map new MsgIDs to .lng ordinals in `FarPluginStrings.cpp`

**Convention:** Preserve blank-line separators between groups. Strip `read`-tool `xx|` hash artifacts. CRLF line endings.

**Verification:** Build succeeds, all message IDs resolve without crash.

---

### Task 3: Implement SecurityConfigurationDialog()

**Target:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPPlugin.h`

**Change:**
- Add `bool SecurityConfigurationDialog()` method to `TWinSCPPlugin` class declaration in header
- Implement in WinSCPDialogs.cpp as a `TWinSCPDialog` (flat dialog). Set `Dialog->SetSize(TPoint(74, 22))` and `Dialog->SetDialogGuid(&SecurityConfigurationDialogGuid)`.
  Note: `GetGUIConfiguration()` is a global accessor (`src/windows/GUIConfiguration.h:398`), not a `TWinSCPPlugin` member.

**Dialog layout (Far Manager text-mode controls):**

1. **Master Password group separator** — `TFarSeparator` with caption "Master password"
2. **UseMasterPassword checkbox** — `TFarCheckBox`, bound to `WinConfiguration->GetUseMasterPassword()`
3. **ChangeMasterPassword button** — `TFarButton`, enabled only when UseMasterPassword is checked, caption "Change master password…"
   - On click: invoke the existing master password change logic (refactored from `MasterPasswordConfigurationDialog`)
4. **Session Password group separator** — `TFarSeparator` with caption "Session password"
5. **SessionRememberPassword checkbox** — `TFarCheckBox`, bound to `GetGUIConfiguration()->GetSessionRememberPassword()`
6. **Trusted Host CA group separator** — `TFarSeparator` with caption "Trusted host certification authorities"
7. **SshHostCAsFromPuTTY checkbox** — `TFarCheckBox`, bound to `GetConfiguration()->SshHostCAsFromPuTTY`
8. **CA list box** — `TFarListBox` (single-column — display as concatenated `"Name  (hosts)"` since Far Manager list boxes lack native multi-column support)
   - When PuTTY checkbox is checked: show PuTTY's CA list (read-only), disable Add/Edit/Remove
   - When unchecked: show user's CA list, enable Add/Edit/Remove
   - Follow existing NetBox pattern: `SetNoBox(true)`, `SetHeight(6)`, `SetRight()` for sizing
10. **Edit button** — `TFarButton`, launches `SshHostCADialog()` in Edit mode for selected CA
11. **Remove button** — `TFarButton`, removes selected CA from list

**Save logic:**
- On OK: save `SessionRememberPassword`, `SshHostCAsFromPuTTY`, `SshHostCAList` (create new `TSshHostCAList` from `FSshHostCAPlainList`)
- Follow WinSCP pattern from Preferences.cpp:1026-1030


**Enablement logic** (WinSCP pattern: Preferences.cpp:1479-1488, NetBox equivalent):
- `ChangeMasterPasswordButton->SetEnabled(WinConfiguration->GetUseMasterPassword())`
- Add/Edit/Remove buttons: `SetVisible(!GetConfiguration()->SshHostCAsFromPuTTY)`
- CA list box: `SetEnabled(!GetConfiguration()->SshHostCAsFromPuTTY)`

**Logging:** Log all security setting changes via `AppLogFmt()`.

**Verification:** Dialog shows, all controls work, settings persist across restart.

---

### Task 4: Implement SshHostCADialog() (Add/Edit sub-dialog)

**Target:** `src/NetBox/WinSCPDialogs.cpp`


- Add helper function `bool SshHostCADialog(bool Add, TSshHostCA & SshHostCA)`, set `Dialog->SetDialogGuid(&SshHostCADialogGuid)`
- **Reuse existing** `NB_SSH_HOST_CA_*` MsgIDs (377-390) for all field labels. Helpers exist: `LoadPublicKey()`, `ParseCertificatePublicKey()`, `IsCertificateValidityExpressionValid()` (PuttyIntf.cpp/PuttyTools.h)

**Minimum viable controls** (Far text-mode):

1. **Name edit** — `TFarEdit`, label `NB_SSH_HOST_CA_NAME`
2. **Public key edit** — `TFarEdit`, label `NB_SSH_HOST_CA_PUBLIC_KEY`. User can paste base64 or use Browse
3. **Validity expression edit** — `TFarEdit`, label `NB_SSH_HOST_CA_PUBLIC_HOSTS`
4. **Browse button** — `TFarButton`, caption `NB_SSH_HOST_CA_BROWSE`. Calls `GetOpenFileName()` → `LoadPublicKey()` → `EncodeBase64()`
5. **RSA signature checkboxes** — optional enhancement. Group `NB_SSH_HOST_CA_SIGNATURES`, checkboxes `NB_SSH_HOST_CA_SIGNATURE_TYPES`

**Validation:**
- Name must not be empty
- Public key must not be empty — show `NB_SSH_HOST_CA_NO_KEY`
- Validity expression via `IsCertificateValidityExpressionValid()` — show `NB_SSH_HOST_CA_NO_HOSTS` / `NB_SSH_HOST_CA_HOSTS_INVALID`

**Verification:** Add/Edit CA works, fields persist, validation catches empty fields.
---

### Task 5: Update menu routing and remove old MasterPassword dialog

**Target:** `src/NetBox/WinSCPPlugin.cpp`, `src/NetBox/WinSCPPlugin.h`

**Change:**
- In `ConfigureEx()` (line ~155): Replace `NB_MASTER_PASSWORD_CAPTION` menu item with `NB_CONFIG_SECURITY` menu item
- Replace `MasterPasswordConfigurationDialog()` call with `SecurityConfigurationDialog()` call
- In `WinSCPPlugin.h`: Replace `MasterPasswordConfigurationDialog()` declaration with `SecurityConfigurationDialog()`
- Remove the old `MasterPasswordConfigurationDialog()` implementation from WinSCPDialogs.cpp (its logic is absorbed into SecurityConfigurationDialog's Change button handler)

**Logging:** Log when security dialog is opened from menu.

**Verification:** Menu shows "Security" item, opens combined dialog, all three sections work.

---

### Task 6: Remove RememberPassword from Transfer dialog

**Target:** `src/NetBox/WinSCPDialogs.cpp`

**Change:**
- Remove `RememberPasswordCheck` checkbox from `TransferConfigurationDialog()` (lines 775-776, 787, 802)
- The setting now lives exclusively in the Security dialog

**Verification:** Transfer dialog no longer shows Remember Password checkbox. Security dialog does.

---

### Task 7: Handle master password change within Security dialog

**Target:** `src/NetBox/WinSCPDialogs.cpp`

**Change:**
- When "Change master password…" button is clicked:
  - If UseMasterPassword is OFF → turning it ON: prompt for new password + confirm (reuse existing password prompt logic from old `MasterPasswordConfigurationDialog`)
  - If UseMasterPassword is ON → changing: prompt for current password, then new + confirm
  - If UseMasterPassword is being turned OFF: prompt for current password, then clear
- Reuse existing `WinConfiguration->ValidateMasterPassword()`, `ChangeMasterPassword()`, `ClearMasterPassword()` methods
- Save config after successful change: `GetConfiguration()->DoSave(false, false)`
- Show success messages: `NB_MASTER_PASSWORD_SET2`, `NB_MASTER_PASSWORD_CHANGED`, `NB_MASTER_PASSWORD_CLEARED2`


**Exception safety:** Wrap recryption in try/catch, report errors. See `master-password-infrastructure-research.md`.

**CRITICAL:** MSVC `RecryptPasswords()` is a stub (`WinConfiguration.cpp:4074`). Master password change will NOT recrypt stored passwords. Log warning: `AppLogFmt(L"WARNING: RecryptPasswords is stub — stored passwords NOT recrypted")`. Fix tracked in `master-password-infrastructure.md`.


**Verification:** Master password set/change/clear works. Stub warning logged.

---

### Task 8: Implement PuTTY CA list refresh and display

**Target:** `src/NetBox/WinSCPDialogs.cpp`
**Change:**

- When SshHostCAsFromPuTTY checkbox is toggled:
  - Call `GetConfiguration()->RefreshPuttySshHostCAList()` (existing thin wrapper, `Configuration.cpp:2364` — resets cache to null)
  - Populate listbox from `GetActiveSshHostCAList()` (switches PuTTY/user list)
  - Update Add/Edit/Remove visibility (hidden in PuTTY mode)
- On init: call `RefreshPuttySshHostCAList()` if PuTTY mode already active
- Single-column display: `"Name  |  hosts"` (concatenated, since TFarListBox lacks native multi-column)

**Verification:** Toggling checkbox refreshes. User list editable, PuTTY list read-only.
---

### Commit Plan

1. `feat(ui): add Security tab MsgIDs, GUIDs, and localized strings` — Tasks 1, 2
2. `feat(ui): implement SecurityConfigurationDialog with master password and session password` — Tasks 3, 7
3. `feat(ui): implement SSH Host CA management dialogs` — Tasks 4, 8
4. `refactor(ui): replace MasterPassword menu with Security tab, move RememberPassword` — Tasks 5, 6
5. `docs: document Security preferences tab` — Task 9 (docs)

---

## Edge Cases


1. **Master password not set but UseMasterPassword checked** — Enable the checkbox only, the Change button handles the "set new" flow
2. **Invalid current password** — Show `NB_MASTER_PASSWORD_INCORRECT` message, return to dialog
3. **Empty CA name** — Validate in SshHostCADialog, show error
4. **Empty CA public key** — Validate, show `NB_SSH_HOST_CA_NO_KEY`
5. **PuTTY not installed** — PuTTY CA list returns empty; checkbox still visible but list shows nothing
6. **CA list empty** — Edit/Remove disabled (no selection), Add enabled
7. **Dialog cancel** — No settings changed (all changes in-memory until OK)
8. **Recryption errors during password change** — Collect errors, show warning
9. **RecryptPasswords stub** — Log warning at dialog open if UseMasterPassword active: `"WARNING: RecryptPasswords MSVC stub — stored passwords will not be recrypted on master password change"`
---

## Out of Scope

- RecryptPasswords MSVC stub fix (covered by `master-password-infrastructure.md` plan)
- ChangeMasterPassword exception safety (covered by `master-password-infrastructure.md` plan)
- Rate limiting on password validation (covered by `master-password-infrastructure.md` plan)
- TSecureString for plaintext passwords (future work)
- Thread-unsafe session counter fix (covered by `master-password-infrastructure.md` plan)
- "Configure in PuTTY" button (requires PuTTY binary discovery; defer to future enhancement)

---

## Research Context

Source: `.ai-factory/references/master-password-infrastructure-research.md`
- RecryptPasswords stub, exception safety gaps, thread safety issues documented
- Those are separate from this UI plan

Source: `.ai-factory/references/winscp-netbox-gap-analysis.md`
- Trusted host CA UI identified as Gap #5 in the comprehensive gap analysis
- Data model already complete, only UI missing
