# Fix: Issue #511 — Speed Limit & Esc Hang

Reference: https://github.com/michaellukashov/Far-NetBox/issues/511

## Settings

- **Testing:** no
- **Logging:** verbose
- **Docs:** yes

## Research Context

See [issue-511-speed-limit-esc-hang-exploration](.ai-factory/references/issue-511-speed-limit-esc-hang-exploration.md) for initial root-cause analysis.
See [issue-511-cancel-yes-hang-deep-dive](.ai-factory/references/issue-511-cancel-yes-hang-deep-dive.md) for detailed analysis of the post-dialog "Yes" hang, reentrancy guard failure, and exception unwinding hazards.

## Bugs Description

1. **Queue: Suspend, Cancel not work.** When Esc is pressed during a queued or direct transfer, `ShowOperationProgress` calls `CancelConfiguration`, which triggers `ProgressData.Suspend()` → `DoProgress()` → reentrant `ShowOperationProgress()`. This reentrancy, combined with Far Manager dialog/message display, corrupts screen state or deadlocks the plugin. Additionally, after pressing "Yes" in the cancel dialog, exception unwinding (`OperationStop()` → `DoProgress()` → `ShowOperationProgress()`) runs unguarded, calling `Message()` and `CheckForEsc()` while the stack unwinds, which can trigger `CancelConfiguration()` again and cause a hang.

2. **`qaCancel` falls through to `default: ACancel = csContinue;`.** In `CancelConfiguration`, the `switch (Result)` statement does not explicitly handle `qaCancel`. When the user presses Esc (or clicks the Cancel button) inside the cancel confirmation dialog, `MoreMessageDialog` returns `qaCancel`, which falls through to `default:` and sets `ACancel = csContinue`. This **continues the transfer** instead of cancelling it. This is a bug — the research identified it, and the fix is to add explicit `case qaCancel: ACancel = csCancel; break;`.

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
- Add explicit `case qaCancel: ACancel = csCancel; break;` in `CancelConfiguration`
- Add Esc press logging

## Tasks

- [x] Task 1: Fix CPS limit propagation for parallel transfers
- [x] Task 2: Add reentrancy guards to ShowOperationProgress
- [x] Task 3: Add cancel-state guards to CancelConfiguration and ShowOperationProgress
- [x] Task 4: Build and verify both fixes
- [x] Task 5: Fix assert ordering and misleading comment in Queue.cpp
- [x] Task 6: Update documentation and changelog

### Task 1: Fix CPS limit propagation for parallel transfers

**File:** `src/core/Queue.cpp`

**Change:**
In `TParallelTransferQueueItem::DoExecute` (around line 2543), replace the hard-coded `0` CPS limit with the parent progress object's limit:

```cpp
  // CPS limit explicitly propagated from parent (not truly "inherited" via FCPSLimit).
  DebugAssert(FParallelOperation->GetMainOperationProgress() != nullptr);
  Terminal->LogEvent(FORMAT("Parallel transfer CPS limit: %u",
    FParallelOperation->GetMainOperationProgress()->GetCPSLimit()));
  OperationProgress.Start(
    Operation, FParallelOperation->GetSide(), -1, Temp,
    FParallelOperation->GetTargetDir(),
    FParallelOperation->GetMainOperationProgress()->GetCPSLimit(),
    odoIdle);
```

**Acceptance:**

- Parallel SFTP/SCP/WebDAV/S3 download with 100 KB/s limit stays below ~120 KB/s.
- No regression for single-threaded (non-parallel) transfers.
- `DebugAssert` precedes any dereference of `GetMainOperationProgress()`.
- Comment no longer claims "inherited" behavior (it is explicitly passed).

### Task 2: Add reentrancy guard to ShowOperationProgress

**Files:** `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPFileSystem.h`

**Change:**
In `ShowOperationProgress` (around line 3772), add three guards to the main time-check condition. The `GetCancel() < csCancel` guard is the **primary** fix for the post-dialog / exception-unwinding hang, while `FInShowOperationProgress` (a `SCOPE_EXIT`-managed boolean) only blocks reentrancy from `Suspend()` → `DoProgress()` during normal operation:

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

### Task 3: Add cancel-state guards and fix qaCancel handling

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

**Change 2:** In the same `CancelConfiguration` function, add explicit `qaCancel` handling in the switch statement:

```cpp
    switch (Result)
    {
    case qaYes:
      ACancel = csCancelTransfer;
      break;
    case qaOK:
      ACancel = csCancel;
      break;
    case qaNo:
      ACancel = csContinue;
      break;
    case qaCancel:
      ACancel = csCancel;
      break;
    default:
      ACancel = csContinue;
      break;
    }
```

Previously `qaCancel` (returned when the user presses Esc or the Cancel button in the cancel confirmation dialog) fell through to `default: ACancel = csContinue;`, which continued the transfer instead of cancelling it. The explicit `case qaCancel: ACancel = csCancel;` fixes this bug.

**Acceptance:**

- Dialog closes cleanly when user presses Cancel or Esc inside the cancel confirmation dialog, and the transfer is cancelled (not continued).
- No compiler warnings.
- No duplicate cancel dialogs after pressing "Yes".
- No regression for "No" or "OK" button behavior.

### Task 4: Build and verify both fixes

**Command:**

```cmd
rmdir /s /q build-RelWithDebugInfo
cmd /c build-x64.bat
```

**Acceptance:**

- Zero warnings (MSVC W4).
- Plugin DLL present in `Far3_x64/Plugins/NetBox/`.
- Manual test (direct transfer): connect to local SFTP, download 10 MB file with 500 KB/s limit, verify speed is throttled.
- Manual test (direct transfer): press Esc during active transfer, verify dialog appears and can be dismissed without hang.
- Manual test (queue transfer): add a background SFTP download, press Esc, verify no hang and progress updates correctly.
- Regression test: non-parallel FTP/SFTP/SCP transfers with and without speed limit.

### Task 5: Fix assert ordering and misleading comment in Queue.cpp

**File:** `src/core/Queue.cpp`

**Problem 1 — Assert ordering:**
The initial implementation placed `LogEvent` (which dereferences `GetMainOperationProgress()`) before the `DebugAssert` null check, making the assert ineffective. The corrected order places the `DebugAssert` first, followed by the log line.

**Problem 2 — Misleading comment:**
The comment "CPS limit inherited from parent OperationProgress" is incorrect because `AdjustToCPSLimit` reads `FCPSLimit` directly; it does not traverse the parent chain. The comment should state that the limit is explicitly propagated.

**Acceptance:**

- `DebugAssert` precedes any dereference.
- Comment accurately describes explicit propagation, not inheritance.
- No compiler warnings.

### Task 6: Update documentation and changelog

**Docs task:**

- Update ROADMAP/changelog with issue #511 fixes: CPS limit propagation, reentrancy guards, cancel-state guards, and `qaCancel` explicit handling.
- Document the `qaCancel` fix: previously pressing Esc in the cancel dialog continued the transfer; now it correctly cancels.

**Acceptance:**

- Changelog/ROADMAP entry exists referencing issue #511.
- No missing documentation for `Docs: yes` setting.

