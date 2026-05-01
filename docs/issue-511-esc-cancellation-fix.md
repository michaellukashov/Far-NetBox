# Fix: SCP Esc Cancellation — Speed Limit & Hang (Issue #511)

Reference: https://github.com/michaellukashov/Far-NetBox/issues/511

## Problem

Two independent bugs in SCP transfer handling:

1. **Speed limit had no effect** for SSH/SFTP/SCP parallel transfers. `TParallelTransferQueueItem::DoExecute` passed `CPSLimit = 0` instead of the parent's limit. FTP was unaffected.

2. **Pressing Esc during transfer caused a hang.** The cancel dialog appeared but clicking "Yes" froze the panel or the plugin entirely. Multiple root causes contributed: reentrancy in `ShowOperationProgress`, `qaCancel` falling through to `csContinue`, fatal exception conversion during stack unwinding, and post-cancel session corruption.

## Root Causes — Five Independent Layers

### Layer 1 — Console Input Buffer Bug (Esc Never Detected)

**File:** `src/NetBox/FarPlugin.cpp`

`CheckForEsc()` and `FlushEscBuffer()` only peeked at the **first event** in the console input buffer. If preceded by a mouse move, key release, or window event, Esc was permanently undetectable.

**Fix:** Read the entire buffer at once, filter Esc key-down events, write all non-Esc events back in FIFO order.

```
1. GetNumberOfConsoleInputEvents → EventCount
2. ReadConsoleInput → read all events
3. Filter: Esc key-down → discard; all other events → preserve
4. WriteConsoleInput → restore non-Esc events
```

### Layer 2 — Cancel State Semantics (csCancel vs csCancelTransfer)

**File:** `src/core/ScpFileSystem.cpp`

SCP transfer loops only checked `csCancelTransfer` during active file transfer. `csCancel` (set by OK/Cancel dialogs) was ignored while transferring, so most user cancellations never aborted an in-progress SCP transfer.

**Fix:** Check both `csCancel` and `csCancelTransfer` in all SCP loop cancellation checks and fatal-error guards.

```cpp
// Before (broken):
if (OperationProgress->GetCancel() == csCancelTransfer)  // upload
if ((GetCancel() == csCancelTransfer) ||                 // upload binary
    (GetCancel() == csCancel && !GetTransferringFile()))  // ← csCancel ignored during transfer

// After (fixed):
if (OperationProgress->GetCancel() == csCancelTransfer ||
    OperationProgress->GetCancel() == csCancel)           // upload
if ((GetCancel() == csCancelTransfer) ||
    (GetCancel() == csCancel))                            // upload binary
```

### Layer 3 — Exception Conversion (Panel Close vs Silent Abort)

**Files:** `src/core/Terminal.cpp`, `src/NetBox/WinSCPFileSystem.cpp`

Catch blocks in `TTerminal::CopyToRemote` and `CopyToLocal` re-threw `EAbort(EXCEPTION_MSG_REPLACED)`. This magic string (`L"[replaced]"`) is treated as a fatal error by `HandleException`, causing it to set `DoClose = true` and close the entire NetBox panel. Every user cancellation closed the panel.

**Fix:** Throw `EAbort("")` (empty message) for user-initiated cancellation. `HandleException` checks `E->Message == EXCEPTION_MSG_REPLACED` — an empty message does not match, so the silent-abort path is taken.

Additionally, invert the cancel-state logic: only throw `EAbort("")` for `csCancel` and `csCancelTransfer` (user-initiated). `csRemoteAbort` and other states fall through to `CommandError` for proper error display.

```cpp
// After (fixed):
if (OperationProgress.GetCancel() == csCancel ||
    OperationProgress.GetCancel() == csCancelTransfer)
{
    OnceDoneOperation = odoIdle;
    throw EAbort("");           // Silent abort — panel stays open
}
// csContinue, csCancelFile, csRemoteAbort → CommandError
```

### Layer 4 — Cleanup Hang (ReadCommandOutput Blocking)

**File:** `src/core/ScpFileSystem.cpp`

After `USER_TERMINATED` is thrown, the `__finally` cleanup called `ReadCommandOutput(coWaitForLastLine)` which loops `ReceiveLine()` until the SCP exit status. The remote SCP process may have **megabytes** of file data still in its send buffer, making this appear to hang.

**Fix:** Guard `ReadCommandOutput` with a `!GetCancel()` check — skip when user cancelled. Send the error byte to the remote side and let the exception propagate immediately.

```cpp
SCPSendError((OperationProgress->GetCancel() ? L"Terminated by user." : L"Exception"), true);
if (OperationProgress->GetCancel() == csContinue)
{
    ReadCommandOutput(coOnlyReturnCode | coWaitForLastLine);  // Only for non-cancelled errors
}
```

### Layer 5 — Post-Cancel Session Corruption (Two Phases)

**Files:** `src/core/ScpFileSystem.cpp`, `src/core/ScpFileSystem.h`, `src/core/SecureShell.h`, `src/core/SecureShell.cpp`

Skipping `ReadCommandOutput` (Layer 4) left the kernel socket buffer full of pipelined file data. Additionally, the remote SSH server **closes the TCP connection** after the SCP process terminates — no drain can revive a dead socket.

**Nine attempts failed before finding the correct approach.** See the [comprehensive fix reference](../.ai-factory/references/esc-cancellation-comprehensive-fix.md) for details.

**Final fix (5j) — Three components:**

**1. `FLastDirectory` tracking** — Persistent member, updated in both `CachedChangeDirectory()` and `ReadCurrentDirectory()`. Unlike `FCurrentDirectory` (only set after `pwd` completes), `FLastDirectory` is always available.

**2. Flag in `__finally`** — Set `FNeedsSessionReset = true` after cleanup. No drain or close during exception unwinding.

**3. Reconnect in `ChangeDirectory()` BEFORE `EnsureLocation()`** — This is the critical insight. `ChangeDirectory`'s `EnsureLocation()` try-catch silently catches dead-session exceptions and sets `ecNoEnsureLocation`, bypassing any reconnect. The reconnect MUST happen first.

```cpp
// In ChangeDirectory (before EnsureLocation try-catch):
if (FNeedsSessionReset)
{
    FNeedsSessionReset = false;
    try { FSecureShell->Close(); } catch (...) {}
    FSecureShell->Open();
    if (!FLastDirectory.IsEmpty())
        FCachedDirectoryChange = FLastDirectory;
}
// Then: EnsureLocation() → cd /home/mikhail/Dropbox/sec (restored)
// Then: ExecCommand("cd ..") → /home/mikhail/Dropbox ✓
```

## Implementation Summary

### Files Modified

| File | Changes |
|------|---------|
| `src/NetBox/FarPlugin.cpp` | Rewrite `CheckForEsc()`, `FlushEscBuffer()`; use `nb::vector_t<INPUT_RECORD>` |
| `src/NetBox/WinSCPFileSystem.cpp` | Reentrancy guards, `CancelConfiguration` qaCancel fix, `EAbort` handling |
| `src/NetBox/WinSCPFileSystem.h` | `FInShowOperationProgress`, `FInCancelDialog` members |
| `src/core/ScpFileSystem.cpp` | csCancel parity (5 locations), `ReadCommandOutput` guard, Layer 5 reconnect |
| `src/core/ScpFileSystem.h` | `FNeedsSessionReset`, `FLastDirectory` members |
| `src/core/Terminal.cpp` | `EAbort("")` + cancel-state logic + diagnostic logging |
| `src/core/SecureShell.h` | `ClearPending()`, `DrainSocket()` declarations |
| `src/core/SecureShell.cpp` | `ClearPending()`, `DrainSocket()` implementations; hex-log for binary data |
| `src/core/Queue.cpp` | CPS limit propagation, assert ordering fix |

### Key Members Added

```cpp
// TSCPFileSystem (ScpFileSystem.h)
bool FNeedsSessionReset{false};        // Set in __finally, consumed in ChangeDirectory
UnicodeString FLastDirectory;          // Tracked in CachedChangeDirectory + ReadCurrentDirectory

// TSecureShell (SecureShell.h)
void ClearPending();                   // Free Pending buffer, reset PendLen/PendSize
void DrainSocket();                    // Non-blocking socket drain (unused, for future use)
```

## Correct Integration Patterns

### Cancellation Flow (End-to-End)

```
User presses Esc during SCP download:
  1. CheckForEsc() → full-buffer scan → Esc detected ✓
  2. CancelConfiguration() → qaCancel → csCancel ✓
  3. ShowOperationProgress → guarded (< csCancel, !FInCancelDialog) ✓
  4. SCP loop → csCancel + csCancelTransfer → throw USER_TERMINATED ✓
  5. __finally cleanup → send error byte, skip ReadCommandOutput ✓
  6. TTerminal catch → EAbort("") → HandleException → silent ✓
  7. Panel stays open, user navigates:
     ChangeDirectory → FNeedsSessionReset → Close+Open → restore dir ✓
```

### Diagnostic Log Sequence (Correct)

```
1. Transfer progress: Transferred: ..., Left: ...
2. SCP download cancelling: cancel=3
3. Sending SCP error (2) to remote side
4. CopyToLocal catch: cancel=3, exception=Terminated by user.
5. Closing connection.           ← Close()
6. Layer 5: restoring dir to /home/mikhail/Dropbox/sec
7. Raw command: cd .. ; echo "NetBox: this is end-of-file:$?"
8. /home/mikhail/Dropbox         ← pwd response ✓
```

## Anti-Patterns to Avoid

| # | Anti-Pattern | Why It's Wrong |
|---|-------------|----------------|
| 1 | `PeekConsoleInput(&Rec, 1, ...)` + `break` on non-Esc | Only checks first event; non-Esc events block permanently |
| 2 | `case qaCancel: ACancel = csContinue` (implicit via default) | User clicking Cancel continues transfer |
| 3 | `throw EAbort(EXCEPTION_MSG_REPLACED)` for user cancel | Magic string triggers panel close |
| 4 | `ReadCommandOutput(coWaitForLastLine)` during user cancel | Blocks on remote buffered file data |
| 5 | Checking only `csCancelTransfer` in SCP loops | Ignores `csCancel` from dialog buttons |
| 6 | `GetCancel() == csContinue` instead of `< csCancel` | `csCancelFile` used internally; would break skip-file |
| 7 | Throwing new exception in `catch` before setting cleanup state | Unreachable `OnceDoneOperation = odoIdle` |
| 8 | Using `== csContinue` for error-vs-cancel gating | Loses diagnostics for `csRemoteAbort` |
| 9 | Guard mismatch (`> csContinue` vs `< csCancel`) | Esc silently discarded during `csCancelFile` window |
| 10 | `EventSelectLoop` for socket drain | Edge-triggered — only reads one block, leaves rest |
| 11 | `ReadCommandOutput` in `__finally` during exception unwinding | Blocks + throwing during unwind = `std::terminate()` |
| 12 | `Close()` in `__finally` during exception unwinding | Kills panel |
| 13 | Reconnect in `SendCommand` (too late) | `ChangeDirectory.EnsureLocation` already handled dead session |

## Verification Results

| # | Test | Expected | Result |
|---|------|----------|--------|
| 1 | Press Esc during SCP upload | Cancel dialog appears | ✓ |
| 2 | Press Esc during SCP download | Cancel dialog appears | ✓ |
| 3 | Click "Yes" in cancel dialog | Transfer aborts | ✓ |
| 4 | Panel after cancel | Panel stays open | ✓ |
| 5 | `cd ".."` after cancel | Navigate to parent dir | ✓ `/home/mikhail/Dropbox` |
| 6 | Esc detection with mouse events in buffer | Esc still detected | ✓ |
| 7 | Speed limit on parallel SFTP/SCP | Throttled to ~limit | ✓ |
| 8 | No hex garbage after cancel | Clean text responses | ✓ |
| 9 | No 15-second timeout after cancel | Response < 1s | ✓ |

## Related Documentation

- [Issue #511 Plan](../.ai-factory/plans/issue-511-speed-limit-esc-hang.md)
- [Comprehensive Fix Reference](../.ai-factory/references/esc-cancellation-comprehensive-fix.md)
- [Code Review Report](../.ai-factory/REVIEW-esc-cancellation.md)
- [AGENTS.md](AGENTS.md)
- [ARCHITECTURE.md](architecture.md)