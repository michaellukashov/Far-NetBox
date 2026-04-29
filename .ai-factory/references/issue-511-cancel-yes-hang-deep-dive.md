# Issue #511 Deep Dive: Hang After Pressing "Yes" in Cancel Dialog

> Date: 2026-04-29
> Reference: https://github.com/michaellukashov/Far-NetBox/issues/511
> Related: [issue-511-speed-limit-esc-hang-exploration](issue-511-speed-limit-esc-hang-exploration.md)

## Problem

During SCP direct transfer (copy from server), pressing Esc shows a fatal cancel dialog (`NB_CANCEL_OPERATION_FATAL2`) with "Yes"/"No"/"Cancel". Pressing "Yes" causes the transfer progress dialog to hang — the plugin becomes unresponsive.

## Initial Fix and Its Limitations

The first fix applied:
1. `FInShowOperationProgress` boolean guard in `ShowOperationProgress`
2. `!ProgressData.GetSuspended()` check
3. Explicit `qaCancel = csContinue` handling (later reverted)

However, analysis revealed that `FInShowOperationProgress` is **insufficient** to block the post-dialog `ShowOperationProgress` call.

### Why `FInShowOperationProgress` Is Not Enough

Call sequence when Esc is pressed:

```
Outer ShowOperationProgress()
  → CheckForEsc() returns true
  → CancelConfiguration(ProgressData)
    → ProgressData.Suspend()
      → DoProgress()
        → ShowOperationProgress() [inner] — BLOCKED by FInShowOperationProgress
    → MoreMessageDialog(..., qaYes|qaNo|qaCancel) shown
    → User presses "Yes"
    → ACancel = csCancelTransfer
    → ProgressData.SetCancel(csCancelTransfer)
    → SCOPE_EXIT → Resume()
      → DoProgress()
        → ShowOperationProgress() [post-dialog] — NOT BLOCKED!
```

`FInShowOperationProgress` is reset by `SCOPE_EXIT` when the **inner** `ShowOperationProgress` (called from `Suspend()`) returns. By the time `Resume()` fires after the dialog closes, the flag is already `false`. Therefore, the post-dialog progress call runs **un guarded**.

## Root Cause of the Hang

After "Yes" is pressed:

1. `csCancelTransfer` is set on `OperationProgress`
2. `SCPSink()` detects cancel and throws `Exception(USER_TERMINATED)`
3. Exception propagates up through `SinkRobust()`, `Sink()`, `TSCPFileSystem::CopyToLocal()`
4. `TTerminal::CommandError()` catches it, converts to `EAbort` via `TryReplaceAndThrow<EFatal, EAbort>()`
5. Stack unwinding triggers `__finally` blocks:
   - `OperationStop()` → `Progress.Stop()` → `DoProgress()` → `ShowOperationProgress()`
6. This post-exception `ShowOperationProgress()` runs with:
   - `Ticks - LastTicks > 500` likely true (dialog was open >500ms)
   - `GetSuspended() == false` ( Resume() already completed )
   - `FInShowOperationProgress == false` (reset after inner call returned)
7. So it passes ALL guards and executes:
   - `Message()` — updates Far Manager status display
   - `CheckForEsc()` — drains console input buffer, may find another Esc event
   - If `CheckForEsc()` returns true → `CancelConfiguration()` AGAIN

### `CheckForEsc()` Input Buffer Interaction

`CheckForEsc()` (`src/NetBox/FarPlugin.cpp:1733`) uses `PeekConsoleInput`/`ReadConsoleInput` in a loop to drain the console input buffer looking for `VK_ESCAPE`. After the dialog closes, keyboard events may still be in the buffer (or the keypress that dismissed the dialog may leave trailing events). If `CheckForEsc()` finds another Esc during the unwinding `ShowOperationProgress()`, it calls `CancelConfiguration()` again, potentially showing a second dialog or corrupting Far Manager's dialog state while the fatal exception is already in flight.

### `Message()` During Stack Unwinding

`Message(0, Title, Message, nullptr, nullptr)` (`src/NetBox/WinSCPFileSystem.cpp:3863`) calls `FStartupInfo.Message()` with `FMSG_ALLINONE | FMSG_LEFTALIGN` and zero buttons — a non-blocking status display. Calling this during exception unwinding, while Far Manager may still be cleaning up the closed modal dialog, can corrupt the screen/dialog state.

## Final Fix

### Guard 1: `ShowOperationProgress` Skip When Cancelled

Added condition `ProgressData.GetCancel() < csCancel` to the main progress display block:

```cpp
if ((Ticks - LastTicks > 500 || Force) && !ProgressData.GetSuspended() &&
    (ProgressData.GetCancel() < csCancel))
{
    // ... progress display, Message(), CheckForEsc(), CancelConfiguration()
}
```

This allows normal progress (`csContinue`, `csCancelFile`) but suppresses the entire block once actual cancellation is initiated (`csCancel`, `csCancelTransfer`, `csRemoteAbort`). No `Message()`, no `CheckForEsc()`, no `CancelConfiguration()` during unwinding.

**Important:** The initial attempt used `GetCancel() == csContinue`, which caused a **regression** — progress was never shown because `csCancelFile` is used internally during normal file operations for skip-file handling. The correct condition is `< csCancel` (allow `csContinue` and `csCancelFile`, block `csCancel` and above).

### Guard 2: `CancelConfiguration` Early Return

Added early return if already cancelled:

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

### Guard 3: Reentrancy Guard (`FInShowOperationProgress`)

The `FInShowOperationProgress` guard remains useful for blocking the inner `ShowOperationProgress` call from `Suspend()` during normal operation, even though it does not block the post-`Resume()` call.

## Files Changed

| File | Change |
|------|--------|
| `src/NetBox/WinSCPFileSystem.cpp` | Added `GetCancel() < csCancel` to `ShowOperationProgress`; added early return in `CancelConfiguration`; added `FInShowOperationProgress` guard; added `!GetSuspended()` check; added Esc press logging |
| `src/NetBox/WinSCPFileSystem.h` | Added `bool FInShowOperationProgress{false}` |
| `src/core/Queue.cpp` | Fixed CPS limit propagation in `TParallelTransferQueueItem::DoExecute` |

## Call Stack During Hang (Hypothesized)

```
TWinSCPFileSystem::ShowOperationProgress() [outer]
  TWinSCPFileSystem::CheckForEsc() → true
  TWinSCPFileSystem::CancelConfiguration()
    TFileOperationProgressType::Suspend()
      TFileOperationProgressType::DoProgress()
        TWinSCPFileSystem::ShowOperationProgress() [inner] — blocked by FInShowOperationProgress
    MoreMessageDialog(..., qtWarning, qaYes|qaNo|qaCancel) → qaYes
    ACancel = csCancelTransfer
    ProgressData.SetCancel(csCancelTransfer)
    ~TSuspendFileOperationProgress (SCOPE_EXIT)
      TFileOperationProgressType::Resume()
        TFileOperationProgressType::DoProgress()
          TWinSCPFileSystem::ShowOperationProgress() [post-dialog] — NOT blocked!
            Message() ← Corrupts Far dialog state during unwinding
            CheckForEsc() ← May find Esc, triggers CancelConfiguration again
```

## Testing

- Build with `OPT_CREATE_PLUGIN_DIR=ON`
- Test protocol: SCP direct transfer (not queued)
- Press Esc after transfer starts → fatal cancel dialog appears
- Press "Yes" → transfer cancels without hang
- Verify normal progress display still works (no regression)
- Check log at `%LOCALAPPDATA%\NetBox\netbox.log` for "Esc pressed during transfer"

## Lessons Learned

1. **Reentrancy guards with `SCOPE_EXIT` can be insufficient** when callbacks are triggered both on entry (Suspend) and exit (Resume) of a guarded region — the flag is reset before the second callback fires.

2. **Console input buffer state persists across modal dialogs** — `PeekConsoleInput`/`ReadConsoleInput` can consume events that were generated while the dialog was open, causing unexpected behavior after dialog dismissal.

3. **Progress callbacks during exception unwinding are dangerous** — `__finally` blocks and destructors may trigger progress updates while the stack is unwinding from a fatal error, and UI calls during this phase can corrupt application state.

4. **Cancel status enums are not binary** — `csCancelFile` (value 1) is used during normal operation for skip-file logic; only `csCancel` (value 2) and above indicate actual user-initiated cancellation. Guards must use `< csCancel`, not `== csContinue`.
