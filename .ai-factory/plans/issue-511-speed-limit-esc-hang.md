# Fix: Issue #511 — Speed Limit & Esc Hang

Reference: https://github.com/michaellukashov/Far-NetBox/issues/511

## Settings

- **Testing:** no
- **Logging:** verbose
- **Docs:** yes

## Research Context

See [issue-511-speed-limit-esc-hang-exploration](.ai-factory/references/issue-511-speed-limit-esc-hang-exploration.md) for full root-cause analysis.

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

When Esc is pressed during transfer, `ShowOperationProgress` calls `CancelConfiguration`, which calls `ProgressData.Suspend()` → `DoProgress()` → reentrant `ShowOperationProgress()`. This reentrancy combined with Far Manager dialog/message display can corrupt screen state or deadlock. Additionally, `qaCancel` is not handled in `CancelConfiguration`'s switch statement.

**Fix directions:**
- Guard `CancelConfiguration` against reentrancy
- Handle `qaCancel` explicitly in `CancelConfiguration`
- Add `FSuspended` check in `ShowOperationProgress` to skip `CancelConfiguration` during reentrant callbacks

## Tasks

- [x] Task 1: Fix CPS limit propagation for parallel transfers
- [x] Task 2: Add reentrancy guard to ShowOperationProgress
- [x] Task 3: Handle qaCancel in CancelConfiguration
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
In `ShowOperationProgress` (around line 3763), add `!ProgressData.GetSuspended()` to the main time-check condition. This skips the entire progress display — including `Message()`, `CheckForEsc()`, and `CancelConfiguration()` — while suspended, making the reentrancy protection intentional rather than relying on the accidental `LastTicks` guard:

```cpp
if ((Ticks - LastTicks > 500 || Force) && !ProgressData.GetSuspended())
{
    LastTicks = Ticks;
    ...
```

Additionally, add a `FInShowOperationProgress` boolean guard to prevent reentrant execution of `ShowOperationProgress` from `Suspend()`/`Resume()` callbacks in `CancelConfiguration`. This prevents nested progress display calls that can corrupt Far Manager screen state or cause dialog hangs when 'Yes' is pressed in the cancel confirmation dialog.

**Logging:** Add `FTerminal->LogEvent(L"Esc pressed during transfer")` inside the `CheckForEsc` block.

**Acceptance:**
- Pressing Esc during direct transfer shows cancel dialog once; no reentrant dialog cascade.
- Pressing Esc during queue transfer does not hang the plugin.

### Task 3: Handle qaCancel in CancelConfiguration

**File:** `src/NetBox/WinSCPFileSystem.cpp`

**Change:**
In `CancelConfiguration` (around line 4050), add explicit `qaCancel` handling to replace the implicit `default` fallthrough:

```cpp
case qaCancel:
    ACancel = csContinue;
    break;
```

This preserves existing behavior. The cancel confirmation dialog text (`NetBoxEng.lng:131`) explicitly states: "Press 'Cancel' to continue operation." The explicit case improves code clarity and prevents accidental behavioral changes if the `switch` is extended in future.

**Acceptance:**
- Dialog closes cleanly when user presses Cancel or Esc inside the cancel confirmation dialog.
- No compiler warnings.

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
