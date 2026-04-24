# Fix FTP Connection Non-Default Port Preservation in Dialog

**Branch:** N/A (fast mode)  
**Created:** 2026-04-25
**Refined:** 2026-04-25

## Settings

| Setting | Value |
|---------|-------|
| Testing | yes |
| Logging | verbose |
| Docs    | no   |

## Research Context

**Bug:** When a user creates or edits an FTP connection in NetBox and sets the port to any value other than 21 (e.g., 443), after closing the dialog and reopening it, the port field reverts to 21. The internal `TSessionData` still holds the correct port (443) and the connection works, but the dialog UI displays the wrong default.

**Root Cause Analysis:** In `src/NetBox/WinSCPDialogs.cpp`, `TSessionDialog::Execute()` loads the saved port correctly at line 3501 (`PortNumberEdit->SetAsInteger(SessionData->GetPortNumber())`) and sets the combo box indices at lines 3508-3512. It caches these indices in `FTransferProtocolIndex` and `FFtpEncryptionComboIndex` at lines 3514-3515. When `ShowModal()` calls the Far API `DialogInit`, the `TFarComboBox` control receives a spurious `DN_EDITCHANGE` message during initialization (via `TFarComboBox::ItemProc` setting `FItemChanged = true`). After `DialogInit`, `DN_INITDIALOG` triggers `Init()` -> `Change()`. `TSessionDialog::Change()` (lines 3149-3186) then checks `TransferProtocolCombo->GetSetChanged(false)`, which returns `true` because of the spurious flag. It unconditionally updates `FTransferProtocolIndex` and sets `DoChange = true`, then calls `TransferProtocolComboChange()`. That function reads the current port (443), sees it equals `HTTPSPortNumber`, and because the protocol is FTP with `ftpsNone`, it overwrites the edit with `FtpPortNumber` (21).

**Fix Strategy:** In `TSessionDialog::Change()`, before updating `FTransferProtocolIndex` or `FFtpEncryptionComboIndex`, compare the combo box's current `GetItemIndex()` with the cached index. Only if they actually differ should `DoChange` be set to `true` and `TransferProtocolComboChange()` invoked. This preserves the auto-adjustment behavior when the user manually switches protocols but suppresses the spurious reset during dialog initialization.

## Tasks

### Task 1: Verify session data port persistence
- **Files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`
- **Deliverable:** Confirm that `FPortNumber`, `GetPortNumber()`, `SetPortNumber()`, and `GetDefaultPort()` correctly store and return non-default FTP ports (e.g., 443) across save/load cycles. Verify `DefaultSettings()` initializes to `SshPortNumber` (22) but `SetPortNumber()` correctly overrides it for FTP sessions.
- **Logging:** Add `AppLogFmt` or `DEBUG_PRINTF` traces around `SetPortNumber` and `GetPortNumber` if needed to confirm the value is preserved.
- **Blocked by:** None.

### Task 2: Trace dialog initialization and port field population
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Map the exact path in `TSessionDialog::Execute()` (lines ~3482-3515) where `PortNumberEdit` is populated, and identify where `TransferProtocolCombo` and `FtpEncryptionCombo` are initialized. Confirm the port is correct immediately after line 3501. Verify that `FTransferProtocolIndex` and `FFtpEncryptionComboIndex` are cached after the combo boxes are set.
- **Logging:** Add `DEBUG_PRINTF` in `Execute()` after `PortNumberEdit->SetAsInteger(...)` to log the loaded port value, and after lines 3514-3515 to log the cached indices.
- **Blocked by:** None (can run in parallel with Task 1).

### Task 3: Analyze Far dialog message flow causing spurious reset
- **Files:** `src/NetBox/FarDialog.cpp`, `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Confirm in `TFarDialog::DialogProc()` and `TFarComboBox::ItemProc()` that `DN_EDITCHANGE` is sent during `DialogInit`, setting `FItemChanged = true` on the combo box items. Trace how `TSessionDialog::Change()` (lines ~3149-3186) consumes this flag and calls `TransferProtocolComboChange()`. Verify that `GetSetChanged(false)` resets the flag after reading it.
- **Logging:** Add `DEBUG_PRINTF` in `TFarComboBox::ItemProc` on `DN_EDITCHANGE`, in `TSessionDialog::Change()` to log `FItemChanged` state and `DoChange` decision, and in `TransferProtocolComboChange()` to log the port value before and after adjustment.
- **Blocked by:** Task 2.

### Task 4: Implement index-comparison guard in TSessionDialog::Change()
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Modify `TSessionDialog::Change()` around lines 3160-3173 to add index-comparison guards:
  - For `TransferProtocolCombo`: capture `NewIndex = TransferProtocolCombo->GetItemIndex()`. Only update `FTransferProtocolIndex` and set `DoChange = true` if `NewIndex != FTransferProtocolIndex`.
  - For `FtpEncryptionCombo`: capture `NewIndex = FtpEncryptionCombo->GetItemIndex()`. Only update `FFtpEncryptionComboIndex` and set `DoChange = true` if `NewIndex != FFtpEncryptionComboIndex`.
  - If `DoChange` is true, call `TransferProtocolComboChange()` as before.
- **Logging:** Add `DEBUG_PRINTF` before calling `TransferProtocolComboChange()` to log the old/new indices and confirm a real protocol change occurred.
- **Blocked by:** Task 3.

### Task 5: Cross-platform build verification
- **Files:** N/A (whole project)
- **Deliverable:** Run `cmd /c build-x64.bat` and `cmd /c build-x86.bat`. Confirm both complete with zero warnings under MSVC W4. If ARM64 build is available, also run `cmd /c build-arm64.bat`. Fix any compilation errors introduced by the change.
- **Logging:** Capture and review full build output for all platforms.
- **Blocked by:** Task 4.

### Task 6: Manual regression testing in Far Manager
- **Files:** N/A
- **Deliverable:** Launch Far Manager from `Far3_x64/`, open NetBox, and execute the following test matrix:
  1. **New FTP session, port 443:** Create a new FTP session, set port to 443, save, reopen dialog — verify port shows 443.
  2. **Existing FTP session, default port 21:** Create a new FTP session, leave port at 21, save, reopen dialog — verify port shows 21.
  3. **Protocol switch SFTP -> FTP:** Create an SFTP session (port 22), switch protocol to FTP in the dialog — verify port changes to 21.
  4. **Protocol switch FTP -> SFTP:** Create an FTP session (port 21), switch protocol to SFTP in the dialog — verify port changes to 22.
  5. **Protocol switch FTP -> FTPS implicit:** Create an FTP session (port 21), switch encryption to implicit — verify port changes to 990.
  6. **Protocol switch WebDAV -> HTTPS:** Create a WebDAV session (port 80), switch to HTTPS — verify port changes to 443.
  7. **WebDAV with custom port:** Create a WebDAV session, set port to 8080, save, reopen — verify port shows 8080.
  8. **S3 with custom port:** Create an S3 session, set port to 8443, save, reopen — verify port shows 8443.
- **Logging:** Document each test step, expected result, actual result, and pass/fail status.
- **Blocked by:** Task 5.

### Task 7: Verify no similar issue in Proxy Method combo
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Inspect the `GetProxyMethodCombo()->GetSetChanged(false)` block at line 3156. Confirm that it only updates `FProxyComboIndex` and does not trigger any side-effect function that could reset dialog fields. If it does, apply the same index-comparison guard pattern.
- **Logging:** Add `DEBUG_PRINTF` around the proxy method change block to log its behavior during initialization.
- **Blocked by:** Task 4.

## Commit Plan

- **Checkpoint 1** (after Task 4 and Task 7):  
  `fix(dialog): prevent spurious FTP port reset during dialog initialization`  
  Contains the minimal surgical change to `TSessionDialog::Change()` with index-comparison guards for protocol and encryption combo boxes. If Task 7 reveals a proxy method issue, include that fix in the same commit.

- **Final checkpoint** (after Task 6):  
  `chore: add manual test results for FTP port preservation fix`  
  Contains the manual regression test log from Task 6. Remove any temporary `DEBUG_PRINTF` logging added during debugging (per project convention: do not leave debug logging in production code).
