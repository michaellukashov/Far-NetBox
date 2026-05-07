# Exploration: Silent Mode UI and Refinements

**Date:** 2026-05-08
**Topic:** Complete silent mode feature — add UI toggle, change overwrite mode, suppress all confirmations, write error reports to file
**Status:** Complete — Ready for planning

## Summary

Silent mode backend is substantially implemented (7 of 7 plan checkpoints from `.ai-factory/plans/silent-mode-file-operations.md`). What's missing is the **user-facing surface** and two behavioral refinements. This exploration identified exactly what exists, what's missing, and the precise code locations for each change.

## Current Implementation State

### Already Implemented

| Component | Location | Status |
|-----------|----------|--------|
| `FSilentMode` flag in `TConfiguration` | `Configuration.h:125`, `Configuration.cpp:243,429` | ✅ Done |
| `GetSilentMode/SetSilentMode` | `Configuration.h:284-285` | ✅ Done |
| `KEY(Bool, SilentMode)` persistence | `Configuration.cpp:429` | ✅ Done |
| `TGUIConfiguration` forwarding | `GUIConfiguration.h:394-395` | ✅ Done |
| `TFarConfiguration` forwarding | `FarConfiguration.h:84-85` | ✅ Done |
| `TFileOperationError` struct | `FileOperationProgress.h:49-63` | ✅ Done |
| `TFileOperationErrorCategory` enum | `FileOperationProgress.h:40-46` | ✅ Done |
| `TFileOperationErrorLog` class | `FileOperationProgress.h:65-86`, `FileOperationProgress.cpp:35-93` | ✅ Done |
| ErrorLog in `TFileOperationProgressType` | `FileOperationProgress.h:178,347-355` | ✅ Done |
| `EffectiveBatchOverwrite → boAll` | `Terminal.cpp:3302-3306` | ✅ Done (needs change to boOlder) |
| `cpNoConfirmation` auto-set (3 sites) | `Terminal.cpp:7888,8473` + 1 more | ✅ Done |
| `spNoConfirmation` auto-set (2 sites) | `Terminal.cpp:6422,7121` | ✅ Done |
| `FileOperationLoopQuery` continue-on-error | `Terminal.cpp:2664-2676` | ✅ Done |
| `GenerateReport()` | `FileOperationProgress.cpp:64-93` | ✅ Done |
| Error report after `CopyToLocal` | `Terminal.cpp:8020-8025` | ✅ Done (needs file output) |
| Error report after `CopyToRemote` | `Terminal.cpp:8623-8628` | ✅ Done (needs file output) |
| Unit tests (configuration) | `tests/core/test_configuration.cpp:21-39` | ✅ Done |
| Integration tests (error log) | `tests/integration/test_silent_mode.cpp:1-79` | ✅ Done |
| User documentation | `docs/silent-mode.md` | ✅ Done |

### Missing (This Plan's Scope)

| Gap | Location | Change Needed |
|-----|----------|---------------|
| **A: No UI toggle** | `WinSCPDialogs.cpp` (0 mentions of SilentMode) | Add checkbox to Confirmations dialog |
| **B: boAll is too aggressive** | `Terminal.cpp:3305` | Change `boAll` → `boOlder` |
| **C: Plugin-layer confirmations not suppressed** | `WinSCPFileSystem.cpp` (5 sites) | Add `GetSilentMode()` bypass |
| **D: Error report only to DoInformation callback** | `Terminal.cpp:8024,8627` | Write to `.errors` file + summary status |

## Detailed Findings per Change

### A: UI Toggle in Confirmations Dialog

**Current dialog:** `WinSCPDialogs.cpp:937-990` — `ConfirmationsConfigurationDialog()`
- Size: `TPoint(67, 10)` — 4 checkboxes
- Pattern: `MakeOwnedObject<TFarCheckBox>(Dialog)` + `SetCaption(GetMsg(NB_*))`
- Load: read from `GetFarConfiguration()` / `GetGUIConfiguration()`
- Save: in `try__finally` block with `BeginUpdate/EndUpdate`

**Required changes:**
- `MsgIDs.h` — add `NB_CONFIRMATIONS_SILENT_MODE` enum entry
- 5 `.lng` files — add localized string at same zero-based index
- `WinSCPDialogs.cpp:937-990` — add 5th checkbox, bump dialog height to 11-12
- Wire checkbox to `GetConfiguration()->GetSilentMode()` / `SetSilentMode()`
- Use `EnabledDependencyNegative` on the 4 existing checkboxes when SilentMode is checked

### B: boOlder Instead of boAll

**Current code:** `Terminal.cpp:3302-3306`
```cpp
if (FConfiguration->GetSilentMode())
{
  return boAll;  // ← unconditional overwrite
}
```

**Change:** Replace `boAll` with `boOlder`.

**Safety analysis:** The early return at line 3305 bypasses all downstream logic including the `!Special` downgrade at line 3336-3340 that would otherwise turn `boOlder` into `boNo`. So `boOlder` from the early return is safe — it will NOT be downgraded.

**Cross-protocol timestamp availability:** All protocols populate `TRemoteFile::FModification`:
- SFTP/SCP: from `ls` / `stat` response
- FTP: from `LIST` / `MLST` parsing
- WebDAV: from `getlastmodified` DAV property
- S3: from `Last-Modified` header / `ListObjectsResult`

`boOlder` works correctly for all protocols.

**Behavioral change note:** This changes existing behavior for any user who has manually enabled SilentMode via XML config. Their overwrite mode will change from "overwrite everything" to "overwrite only if newer." Commit message should note this.

### C: Suppress All Confirmation Dialogs

**Current coverage:** SilentMode suppresses overwrite/error confirmations at the Core layer (`Terminal.cpp`). But the Plugin layer has its own confirmation dialogs that bypass SilentMode.

**Unsuppressed confirmation points:**

1. **Delete files confirmation** — `WinSCPFileSystem.cpp:2612`
   ```cpp
   if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
       (MoreMessageDialog(Query, nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
   ```
   Need: add `GetConfiguration()->GetSilentMode() ||` before the dialog call

2. **Delete sessions confirmation** — `WinSCPFileSystem.cpp:2621`
   ```cpp
   if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
       (MoreMessageDialog(GetMsg(NB_DELETE_SESSIONS_CONFIRM), ...)))
   ```
   Need: add `GetConfiguration()->GetSilentMode() ||` before the dialog call

3. **Synchronized browsing toggle** — `WinSCPFileSystem.cpp:2224`
   ```cpp
   if (GetFarConfiguration()->GetConfirmSynchronizedBrowsing())
   ```
   Need: add `!GetConfiguration()->GetSilentMode() &&` guard

4. **Create directory dialog** — `WinSCPFileSystem.cpp:2485`
   ```cpp
   if ((OpMode & OPM_SILENT) ||
       CreateDirectoryDialog(AName, &Properties, SaveSettings))
   ```
   Need: add `GetConfiguration()->GetSilentMode() ||` as additional bypass

5. **Create session folder dialog** — `WinSCPFileSystem.cpp:2514`
   ```cpp
   if (((OpMode & OPM_SILENT) ||
       GetWinSCPPlugin()->InputBox(...)))
   ```
   Need: add `GetConfiguration()->GetSilentMode() ||` as additional bypass

**Pattern for all sites:** Add `GetConfiguration()->GetSilentMode()` as an additional OR condition alongside the existing `OPM_SILENT` check. This mirrors how `OPM_SILENT` already works — Far Manager's programmatic "don't prompt" flag — but ties it to the user's persistent configuration preference.

### D: Error Reports to File

**Current code:** `Terminal.cpp:8020-8025` and `Terminal.cpp:8623-8628`
```cpp
if (FConfiguration->GetSilentMode() && OperationProgress.GetErrorLog().HasErrors())
{
  const UnicodeString Report = OperationProgress.GetErrorLog().GenerateReport();
  LogEvent(1, L"Silent mode error report:\n" + Report);
  DoInformation(Report, 0, L"");  // ← fires FOnInformation callback → status line
}
```

**Problem:** `DoInformation()` in Far Manager context shows a brief status line — unsuitable for a multi-page error report.

**Proposed change:**
1. Derive `.errors` file path from the session log path: same directory, same base name, `.errors` extension
2. Write the full report to the `.errors` file using `_wfsopen` + write (same pattern as `logging.cpp`)
3. Replace `DoInformation(Report, 0, L"")` with `DoInformation(summary, 0, L"")` where summary = `"<N> errors — see <path>"`
4. If no log file is configured, use `%TEMP%\<session-name>.errors` as fallback
5. Keep `LogEvent(1, ...)` as-is (already logs the full report)

**Log file path resolution:**
- `FConfiguration->GetLogFileName()` returns the effective log path (with session placeholder expansion)
- Replace `.log` / `.xml` extension with `.errors`
- If `GetLogFileName()` is empty, use `GetDefaultLogFileName()` path with `.errors` extension
- Write using `_wfsopen(path, L"w", SH_DENYWR)` + `fwrite` + `fclose` (existing pattern from `SessionLog.cpp`)

## Related Issues

No specific GitHub issue number assigned. Silent mode was designed as an internal feature for automation/deadlock-prevention.

## File Impact Summary

| File | Change Type |
|------|-------------|
| `src/base/MsgIDs.h` | Add 1 enum entry |
| `src/NetBox/NetBoxEng.lng` | Add 1 string |
| `src/NetBox/NetBoxRus.lng` | Add 1 string |
| `src/NetBox/NetBoxFr.lng` | Add 1 string |
| `src/NetBox/NetBoxPol.lng` | Add 1 string |
| `src/NetBox/NetBoxSpa.lng` | Add 1 string |
| `src/NetBox/WinSCPDialogs.cpp` | Add checkbox + wire up |
| `src/core/Terminal.cpp` | Change boAll→boOlder; error file output (2 sites) |
| `src/NetBox/WinSCPFileSystem.cpp` | Add SilentMode bypass (5 sites) |
| `docs/silent-mode.md` | Update with new behavior |
