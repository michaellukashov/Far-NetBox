# Fix: Issue #511 — Speed Limit & Esc Hang

Reference: https://github.com/michaellukashov/Far-NetBox/issues/511

## Settings

- **Testing:** no
- **Logging:** verbose
- **Docs:** yes

## Research Context

See [issue-511-speed-limit-esc-hang-exploration](.ai-factory/references/issue-511-speed-limit-esc-hang-exploration.md) for initial root-cause analysis.
See [issue-511-cancel-yes-hang-deep-dive](.ai-factory/references/issue-511-cancel-yes-hang-deep-dive.md) for detailed analysis of the post-dialog "Yes" hang, reentrancy guard failure, and exception unwinding hazards.
### Bug 1: Download Speed Limit Has No Effect (SSH/SFTP/SCP)

`TParallelTransferQueueItem::DoExecute` (`src/core/Queue.cpp:2543`) creates child `TFileOperationProgressType` with `CPSLimit = 0`:

```cpp
OperationProgress.Start(
    Operation, FParallelOperation->GetSide(), -1, Temp,
    FParallelOperation->GetTargetDir(), 0, odoIdle);
```

The comment claims "CPS limit inherited from parent" but this only works for `GetCPSLimit()`, not for `AdjustToCPSLimit()` which reads `FCPSLimit` directly. FTP is unaffected because it calls `GetCPSLimit()`. SFTP/SCP/WebDAV/S3 are broken because they rely on `AdjustToCPSLimit`/`ThrottleToCPSLimit`.

**Fix:** Pass the parent operation progress CPS limit into `OperationProgress.Start`.

### Bug 2: Esc Causes Plugin Hang After Transfer Begins

When Esc is pressed during transfer, `ShowOperationProgress` calls `CancelConfiguration`, which calls `ProgressData.Suspend()` → `DoProgress()` → reentrant `ShowOperationProgress()`. This reentrancy combined with Far Manager dialog/message display can corrupt screen state or deadlock.

Additionally, after pressing "Yes" in the cancel dialog, the exception unwinding path (`OperationStop()` → `DoProgress()` → `ShowOperationProgress()`) runs without any guard, calling `Message()` and `CheckForEsc()` while the stack is unwinding. `CheckForEsc()` may consume a second Esc event from the console input buffer, triggering `CancelConfiguration()` again and causing a hang.

**Fix directions:**
- Add `FInShowOperationProgress` reentrancy guard in `ShowOperationProgress`
- Add `!ProgressData.GetSuspended()` check to skip progress during dialog
- Add `ProgressData.GetCancel() < csCancel` to suppress progress display once cancellation is initiated
- Add early return in `CancelConfiguration` if `GetCancel() > csContinue`
- Add Esc press logging
## Tasks

- [x] Task 1: Fix CPS limit propagation for parallel transfers
- [x] Task 2: Add reentrancy guards to ShowOperationProgress
- [x] Task 3: Add cancel-state guards to CancelConfiguration and ShowOperationProgress
- [x] Task 4: Build and verify both fixes
### Task 1: Fix CPS limit propagation for parallel transfers

**File:** `src/core/Queue.cpp`

**Change:**
In `TParallelTransferQueueItem::DoExecute` (around line 2543), replace the hard-coded `0` CPS limit with the parent progress object's limit:

```cpp
DebugAssert(FParallelOperation->GetMainOperationProgress() != nullptr);
OperationProgress.Start(
    Operation, FParallelOperation->GetSide(), -1, Temp,
    FParallelOperation->GetTargetDir(),
    FParallelOperation->GetMainOperationProgress()->GetCPSLimit(),
    odoIdle);
```

**Logging:** Add `FTerminal->LogEvent(FORMAT(L"Parallel transfer CPS limit: %u", ...))` before `OperationProgress.Start` to confirm the limit value.

**Acceptance:**
- Parallel SFTP download with 100 KB/s limit stays below ~120 KB/s.
- No regression for single-threaded (non-parallel) transfers.

### Task 2: Add reentrancy guard to ShowOperationProgress

**Files:** `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPFileSystem.h`

**Change:**
In `ShowOperationProgress` (around line 3772), add three guards to the main time-check condition:

```cpp
if ((Ticks - LastTicks > 500 || Force) && !ProgressData.GetSuspended() &&
    (ProgressData.GetCancel() < csCancel))
{
    LastTicks = Ticks;
    ...
}
```

- `!ProgressData.GetSuspended()` — skips progress while cancel dialog is open
- `GetCancel() < csCancel` — allows normal progress (`csContinue`, `csCancelFile`) but suppresses display once actual cancel is initiated (`csCancel`, `csCancelTransfer`, `csRemoteAbort`)

**Important:** An initial attempt used `GetCancel() == csContinue`, which caused a regression where progress was never shown because `csCancelFile` is used internally for skip-file handling during normal operation. The correct condition is `< csCancel`.

Additionally, add a `FInShowOperationProgress` boolean guard to prevent reentrant execution of `ShowOperationProgress` from the `Suspend()` callback in `CancelConfiguration`.

**Logging:** Add `FTerminal->LogEvent(L"Esc pressed during transfer")` inside the `CheckForEsc` block.

**Acceptance:**
- Pressing Esc during direct transfer shows cancel dialog once; no reentrant dialog cascade.
- Pressing "Yes" in cancel dialog cancels transfer without hang.
- Normal progress display still works during transfer (no regression).
### Task 3: Add cancel-state guards

**File:** `src/NetBox/WinSCPFileSystem.cpp`

**Change 1:** In `CancelConfiguration` (around line 4040), add early return if already cancelled:

```cpp
void TWinSCPFileSystem::CancelConfiguration(TFileOperationProgressType & ProgressData)
{
  if (ProgressData.GetCancel() > csContinue)
  {
    return;
  }
  // ... existing dialog logic
}
```

This prevents re-showing the fatal cancel dialog if the operation is already being cancelled.

**Note:** Explicit `qaCancel` handling (`case qaCancel: ACancel = csContinue; break;`) was considered but omitted per user request. The default case (`csContinue`) preserves existing behavior.

**Acceptance:**
- Dialog closes cleanly when user presses Cancel or Esc inside the cancel confirmation dialog.
- No compiler warnings.
- No duplicate cancel dialogs after pressing "Yes".
### Task 4: Build and verify both fixes

**Command:**
```cmd
rmdir /s /q build-RelWithDebugInfo
cmd /c build-x64.bat
```

**Acceptance:**
- Zero warnings (MSVC W4).
- Plugin DLL present in `Far3_x64/Plugins/NetBox/`.
- Manual test: connect to local SFTP, download 10 MB file with 500 KB/s limit, verify speed is throttled.
- Manual test: press Esc during active transfer, verify dialog appears and can be dismissed without hang.
