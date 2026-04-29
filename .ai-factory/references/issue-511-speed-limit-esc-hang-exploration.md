# Issue #511 Exploration: Speed Limit & Esc Hang

> Date: 2026-04-29
> Reference: https://github.com/michaellukashov/Far-NetBox/issues/511

## Bug 1: Download Speed Limit Has No Effect (SSH/SFTP/SCP)

### Root Cause

In `TParallelTransferQueueItem::DoExecute` (`src/core/Queue.cpp:2543`), the child `TFileOperationProgressType` is started with `CPSLimit = 0`:

```cpp
OperationProgress.Start(
    Operation, FParallelOperation->GetSide(), -1, Temp,
    FParallelOperation->GetTargetDir(), 0, odoIdle);
```

The comment claims "CPS limit inherited from parent OperationProgress", but this is only true for `GetCPSLimit()` which traverses `FParent`. However, `AdjustToCPSLimit` (`src/core/FileOperationProgress.cpp:463`) reads `FCPSLimit` directly:

```cpp
if (FCPSLimit > 0)
{
    ...
}
```

Since `FCPSLimit == 0` in the child progress object, `AdjustToCPSLimit` returns the full requested block size without throttling.

### Why "Maybe Only for SSH"

- **FTP**: Uses `OperationProgress->GetCPSLimit()` in `TFTPFileSystem::SetCPSLimit` (`src/core/FtpFileSystem.cpp:1650`), which traverses the parent. So FTP works.
- **SFTP/SCP/WebDAV/S3**: Use `AdjustToCPSLimit` or `ThrottleToCPSLimit` which read `FCPSLimit` directly. These are broken for parallel transfers.
- SSH protocols (SFTP/SCP) are the most commonly affected because they rely on `AdjustToCPSLimit`.

### Affected Files

- `src/core/Queue.cpp` - `TParallelTransferQueueItem::DoExecute`
- `src/core/FileOperationProgress.cpp` - `AdjustToCPSLimit`

### Fix Options

1. **Pass parent CPS limit**: Change the `0` to `FParallelOperation->GetMainOperationProgress()->GetCPSLimit()` in `TParallelTransferQueueItem::DoExecute`.
2. **Make AdjustToCPSLimit use GetCPSLimit()**: Replace direct `FCPSLimit` access with `GetCPSLimit()` call. This is more robust but slightly more expensive (requires lock on parent).

Recommended: Option 1 (minimal, targeted fix).

---

## Bug 2: Esc Causes Plugin Hang After Transfer Begins

### Observed Behavior

After a file transfer starts, pressing the Esc key causes the Far Manager plugin to become unresponsive (hang).

### Analysis

The hang occurs in the interaction between:

1. `TWinSCPFileSystem::ShowOperationProgress` (`src/NetBox/WinSCPFileSystem.cpp:3757`) - called every 500ms or on `Force`
2. `TWinSCPFileSystem::CancelConfiguration` (`src/NetBox/WinSCPFileSystem.cpp:4028`) - triggered when Esc is detected
3. `TFileOperationProgressType::Suspend()` (`src/core/FileOperationProgress.cpp:248`) - called inside `CancelConfiguration`
4. `TFileOperationProgressType::Resume()` (`src/core/FileOperationProgress.cpp:261`) - called when dialog closes

Both `Suspend()` and `Resume()` call `DoProgress()`, which invokes the `OnProgress` callback (`ShowOperationProgress`). This creates **reentrancy** into the progress display code while the cancel dialog is being processed.

Additionally, `CheckForEsc()` (`src/NetBox/FarPlugin.cpp:1733`) uses `PeekConsoleInput` followed by `ReadConsoleInput` to consume the Esc event. However, if the dialog's message loop also processes keyboard input, there may be race conditions or double-consumption of the Esc key.

### Potential Hang Scenarios

1. **Reentrant progress callback**: `Suspend()` → `DoProgress()` → `ShowOperationProgress()` → `Message()` is called while the outer `ShowOperationProgress` is still active. Far Manager's screen state may be corrupted.

2. **Unconsumed Esc key in dialog**: If the Esc key event is not properly consumed before `MoreMessageDialog` opens, the dialog's message loop may immediately process it as "Cancel dialog", causing the dialog to close with `qaCancel`. The `CancelConfiguration` switch statement does not handle `qaCancel` (falls through to `default: ACancel = csContinue`), so the transfer continues. The user may perceive this as a hang if the transfer is blocking.

3. **Background thread violation**: For queue/background transfers, `ShowOperationProgress` is NOT called from the background thread (the queue intercepts `OnProgress`). However, if there are direct transfers or if the queue UI itself calls `CheckForEsc()`, there may be threading issues.

### Affected Files

- `src/NetBox/WinSCPFileSystem.cpp` - `ShowOperationProgress`, `CancelConfiguration`
- `src/core/FileOperationProgress.cpp` - `Suspend`, `Resume`, `DoProgress`
- `src/NetBox/FarPlugin.cpp` - `CheckForEsc`

### Fix Directions

1. **Add reentrancy guard in CancelConfiguration**: Check if already inside a progress callback before showing the cancel dialog.
2. **Guard DoProgress during Suspend/Resume**: Skip `DoProgress()` if we're already inside an `OnProgress` callback.
3. **Handle qaCancel in CancelConfiguration**: Add explicit handling for `qaCancel` result from `MoreMessageDialog`.
4. **Consume Esc before dialog**: Ensure the Esc key is fully consumed from the console input buffer before opening the cancel dialog.

---

## Testing Notes

- Build with `OPT_CREATE_PLUGIN_DIR=ON`
- Test with local SFTP server (OpenSSH)
- Set speed limit to 100 KB/s in transfer dialog
- Verify actual speed is throttled (use large file)
- Press Esc during active transfer and verify no hang
- Check log at `%LOCALAPPDATA%\NetBox\netbox.log`
