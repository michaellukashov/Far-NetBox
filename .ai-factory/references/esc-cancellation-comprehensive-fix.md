# SCP Esc Cancellation — Comprehensive Fix Reference

Reference: [Issue #511 Plan](.ai-factory/plans/issue-511-speed-limit-esc-hang.md) | GitHub Issue: https://github.com/michaellukashov/Far-NetBox/issues/511

## Overview

The Esc key cancellation for SCP transfers was broken across **four independent layers**, each requiring a
separate fix. The original plan addressed reentrancy guards and dialog button mapping, but three additional
root-cause bugs were discovered during verification.

---

## Layer 1 — Console Input Buffer Bug (Esc Never Detected)

### Known Bug

`CheckForEsc()` and `FlushEscBuffer()` in `src/NetBox/FarPlugin.cpp` only peeked at the **first event** in the
console input buffer. If the first event was a mouse move, key release, window resize, or any other non-Esc
event, both functions `break`d immediately without checking subsequent events. Since `PeekConsoleInput` does
not remove events, the non-Esc event stayed at the front **forever**, making Esc permanently undetectable.

The bug was introduced by a prior fix that changed from consuming all events (losing non-Esc keystrokes) to
`break`ing on the first non-Esc event — the correct approach is to scan the entire buffer and preserve
non-Esc events.

### Correct Integration Pattern

Read the entire console input buffer, filter out Esc key-down events, and write all non-Esc events back:

```
1. GetNumberOfConsoleInputEvents(FConsoleInput, &EventCount)
2. if no events → return false
3. ReadConsoleInput(FConsoleInput, buffer, EventCount, &ReadCount)
4. Iterate all events:
   - Esc key-down (bKeyDown==true) → FoundEsc = true
   - Everything else → push to NonEscEvents vector
5. If NonEscEvents not empty → WriteConsoleInput(FConsoleInput, NonEscEvents, ...)
6. return FoundEsc
```

**Key invariant:** Non-Esc events must be preserved in FIFO order. All Esc key-down events must be removed
to prevent the cancel dialog from immediately closing.

### Implementation

| File | Function | Change |
|------|----------|--------|
| `src/NetBox/FarPlugin.cpp` | `CheckForEsc()` | Rewrote from single-event peek to full-buffer scan |
| `src/NetBox/FarPlugin.cpp` | `FlushEscBuffer()` | Rewrote from single-event peek to full-buffer scan |
| `src/NetBox/FarPlugin.cpp` | `#include <vector>` | Added for `INPUT_RECORD` buffer |

**Verification:** Esc keypress is detected even when mouse moves, key releases, or other events precede it
in the console input buffer.

---

## Layer 2 — Cancel State Semantics (csCancel vs csCancelTransfer)

### Known Bug

The SCP transfer loops in `src/core/ScpFileSystem.cpp` only checked for `csCancelTransfer` during active
file transfer. `csCancel` was ignored while `OperationProgress->GetTransferringFile()` was true. But
`CancelConfiguration` maps **both** the OK/Cancel dialog and the Cancel button in the Yes/No/Cancel
dialog to `csCancel` — meaning most user-initiated cancellations never aborted an in-progress SCP transfer.

```cpp
// BEFORE (broken):
if (OperationProgress->GetCancel() == csCancelTransfer)                    // ASCII upload
if ((GetCancel() == csCancelTransfer) ||                                   // BINARY upload
    (GetCancel() == csCancel && !GetTransferringFile()))
if (OperationProgress->GetCancel() == csCancelTransfer)                    // download
```

### Correct Integration Pattern

Check for **both** `csCancel` and `csCancelTransfer` in all SCP in-flight cancellation checks. Also update
the surrounding fatal-error guards so `csCancel` is recognized as user-initiated (not a fatal protocol error):

```cpp
// AFTER (fixed):
if (OperationProgress->GetCancel() == csCancelTransfer ||
    OperationProgress->GetCancel() == csCancel)                            // ASCII upload
if ((GetCancel() == csCancelTransfer) || (GetCancel() == csCancel))        // BINARY upload
if (OperationProgress->GetCancel() == csCancelTransfer ||
    OperationProgress->GetCancel() == csCancel)                            // download
```

### Implementation

| Location | File | Change |
|----------|------|--------|
| Line ~2189 | `src/core/ScpFileSystem.cpp` | ASCII upload: added `\|\| == csCancel` |
| Line ~2214 | `src/core/ScpFileSystem.cpp` | BINARY upload: removed `&& !GetTransferringFile()` guard |
| Line ~2896 | `src/core/ScpFileSystem.cpp` | Download: added `\|\| == csCancel` |
| Line ~2262 | `src/core/ScpFileSystem.cpp` | Upload fatal-error guard: added `&& != csCancel` |
| Line ~2906 | `src/core/ScpFileSystem.cpp` | Download fatal-error guard: added `&& != csCancel` |

**Why `csCancel` semantics is safe here:** SCP has no graceful mid-file abort — the loop reads/sends data
in fixed-size blocks. Once in the block loop, the only way to stop is to throw. SFTP and other protocols
handle `csCancel` differently (via async abort), but for SCP, `csCancel` and `csCancelTransfer` are
semantically equivalent during active data transfer.

**Verification:** User pressing Esc → clicking OK (csCancel=2) or Yes (csCancelTransfer=3) → both
immediately throw `USER_TERMINATED` in the SCP loop.

---

## Layer 3 — Exception Conversion (Panel Close vs Silent Abort)

### Known Bug

The catch blocks in `TTerminal::CopyToRemote` and `TTerminal::CopyToLocal` (`src/core/Terminal.cpp`)
converted the caught `USER_TERMINATED` exception to `EAbort(EXCEPTION_MSG_REPLACED)`. However,
`EXCEPTION_MSG_REPLACED` (`L"[replaced]"`) is a **magic string** that `TWinSCPFileSystem::HandleException`
treats as a **fatal error** — it sets `DoClose = true` and closes the entire NetBox panel. Every user
cancellation would close the panel instead of silently aborting the transfer.

### Correct Integration Pattern

Throw `EAbort("")` (empty message) for user-initiated cancellation. `HandleException` checks
`E->Message == EXCEPTION_MSG_REPLACED` — an empty message does not match, so the silent-abort path
is taken:

```cpp
// In HandleException (WinSCPFileSystem.cpp):
if (E->Message == EXCEPTION_MSG_REPLACED)   { DoClose = true; }       // fatal
else                                        { return; }               // silent abort
```

**Exception state machine:**

```
USER_TERMINATED (ScpFileSystem)
  ↓ caught by catch(Exception & E) in Terminal.cpp
  ↓ if GetCancel() != csContinue → throw EAbort("")   ← empty message
  ↓
EAbort("") propagates through GetFilesRemote/GetFilesEx/GetFiles
  ↓ caught by catch(Exception & E) in TCustomFarPlugin::GetFiles/PutFiles
  ↓ HandleFileSystemException → HandleException
  ↓ nb::isa<EAbort>(E) is true, Message="" ≠ EXCEPTION_MSG_REPLACED
  ↓ return (silent, no panel close)
```

### Implementation

| File | Line | Change |
|------|------|--------|
| `src/core/Terminal.cpp` | ~7928 | `throw EAbort("")` (was `EAbort(EXCEPTION_MSG_REPLACED)`) |
| `src/core/Terminal.cpp` | ~8518 | `throw EAbort("")` (was `EAbort(EXCEPTION_MSG_REPLACED)`) |

**Verification:** User cancels transfer → panel stays open, no error dialog, transfer silently aborted.

---

## Layer 4 — Cleanup Hang (ReadCommandOutput Blocking)

### Known Bug

After the SCP transfer loop throws `USER_TERMINATED`, exception unwinding reaches the `__finally` block in
`TSCPFileSystem::CopyToLocal`. This block attempts a **graceful SCP session teardown**:

1. `ReceiveLine()` — reads a line from the remote side
2. `SCPSendError()` — sends error byte (\x02) to remote SCP
3. `ReadCommandOutput(coOnlyReturnCode | coWaitForLastLine)` — **waits for remote side to finish**

The remote SCP process may have **megabytes** of file data still in its send buffer. `ReadCommandOutput`
calls `ReceiveLine()` repeatedly, reading this buffered file data, until it eventually receives the SCP
termination line. This makes it appear the transfer never stopped.

**Log evidence:**

```
SCP download cancelling: cancel=3     ← exception thrown (correct)
Read 62 bytes (12989 pending)         ← cleanup starts reading buffered data
Sending SCP error (2) to remote side  ← error sent to remote
Read 261 bytes (12728 pending)        ← STILL reading remote's buffered file data
Read 100 bytes (12628 pending)        ← remote hasn't acknowledged error yet
Read 197 bytes (12431 pending)        ← user sees this as "not stopping"
```

### Correct Integration Pattern

When `OperationProgress->GetCancel()` is non-zero (user cancelled), **skip** the `ReadCommandOutput` wait.
Just send the error to the remote side and let the original exception propagate immediately. The remote
SCP process will exit on its own schedule; we don't need to wait for acknowledgment.

```cpp
SCPSendError((OperationProgress->GetCancel() ? L"Terminated by user." : L"Exception"), true);
if (!OperationProgress->GetCancel())
{
    // Only wait for remote ack on non-user-cancelled errors
    ReadCommandOutput(coOnlyReturnCode | coWaitForLastLine);
}
// Note: When user cancelled, skip ReadCommandOutput to avoid blocking on remote data.
```

### Implementation

| File | Line | Change |
|------|------|--------|
| `src/core/ScpFileSystem.cpp` | ~2528-2535 | Guard `ReadCommandOutput` with `!GetCancel()` check |

The upload path (`CopyToRemote` at line ~1999) has similar `ReadCommandOutput(coWaitForLastLine)` but is
guarded by a `catch(Exception & E)` that catches exceptions from the teardown. Since the upload direction
involves a shorter teardown handshake (`SendLine("E")` → `SCPResponse()` → `ReadCommandOutput`), it's
less likely to block for extended periods.

**Verification:** User cancels download → error sent to remote → exception propagates immediately → transfer
stops without waiting for remote acknowledgment.

**Post-verification discovery:** After cancellation, navigating the panel (`cd ".."`) would freeze because the pending buffer (`PendLen`/`Pending` in `TSecureShell`) still contained 13KB+ of buffered file data. The next `ReceiveLine()` returned garbled binary instead of a shell prompt, corrupting the SCP session state. See Layer 5 below.

---

## Layer 5 — Pending Buffer Corruption After Cancel (Post-Panel Navigation)

### Known Bug

After the four-layer fix, cancelling an SCP download mid-file **still** caused a hang when navigating the panel. The transfer dialog closed correctly, but pressing Backspace or clicking `..` to change directory read garbled binary data instead of a shell response, freezing the panel.

**Root cause:** The SCP download loop calls `FSecureShell->Receive(BlockBuf, BlockSize)` (inline low-level reads). The remote SCP process pumps file data through TCP into the local socket buffer. `Receive()` stores excess data beyond what the loop consumes into a **pending buffer** (`PendLen` / `Pending` in `TSecureShell`). When `USER_TERMINATED` is thrown mid-transfer, the pending buffer still holds 13KB+ of buffered file data. The `__finally` cleanup sends the error byte, but **`PendLen` is never cleared**. The next `ReceiveLine()` (for `cd ".."`) reads from this stale pending buffer, getting binary garbage.

**Log evidence:**

```
01:51:59.232  SCP download cancelling: cancel=3         ← Layer 2: cancel detected
01:51:59.232  Read 148 bytes (12903 pending)            ← 13KB of file data still buffered
01:51:59.233  Sending SCP error (2) to remote side     ← cleanup sends abort byte
01:51:59.233  CopyToLocal catch: cancel=3, exception=... ← Layer 3: outer catch reached
...
01:52:01.183  Changing directory to "..".             ← user navigates panel
01:52:01.183  cd ".." ; echo "NetBox: this is end-of-file:$?"
01:52:01.183  Read 82 bytes (12821 pending)            ← READS BUFFERED FILE DATA!
01:52:01.183  [hex] 57AB88D4D9C1895F...                ← GARBAGE — corrupts shell state
```

### Correct Integration Pattern

After SCP mid-file cancellation, clear the pending buffer to discard leftover file data before the next shell command is issued:

```cpp
// In TSecureShell (SecureShell.h + SecureShell.cpp):
void TSecureShell::ClearPending()
{
  if (Pending != nullptr)
  {
    sfree(Pending);
    Pending = nullptr;
  }
  PendLen = 0;
  PendSize = 0;
}
```

Call from `TSCPFileSystem::CopyToLocal` `__finally` block when user cancelled:

```cpp
SCPSendError(...);
if (OperationProgress->GetCancel() == csContinue)
{
    ReadCommandOutput(coOnlyReturnCode | coWaitForLastLine);
}
else
{
    FSecureShell->ClearPending();  // Layer 5
}
```

### Implementation

| File | Line | Change |
|------|------|--------|
| `src/core/SecureShell.h` | ~178 | Add `void ClearPending();` declaration |
| `src/core/SecureShell.cpp` | ~1353 | Implement `ClearPending()` — free pending buffer, reset lengths |
| `src/core/ScpFileSystem.cpp` | ~2535 | Call `FSecureShell->ClearPending()` in `else` branch when cancelled |

**Verification:** User cancels download → navigates panel → shell commands execute normally without corruption.
---

## Diagnostic Logging

Added trace points for future debugging:

| File | Log Message | Purpose |
|------|-------------|---------|
| `src/core/ScpFileSystem.cpp` | `SCP upload/download cancelling: cancel=N` | Confirms cancel state detected in loop |
| `src/core/Terminal.cpp` | `CopyToRemote/CopyToLocal catch: cancel=N, exception=...` | Confirms exception reached outer catch |

**Expected log sequence for successful cancellation:**

```
1. Transfer progress: Transferred: ..., Left: ...
2. SCP download cancelling: cancel=3          ← Layer 2 fix
3. Sending SCP error (2) to remote side       ← cleanup runs
4. CopyToLocal catch: cancel=3, exception=... ← Layer 3 fix (reaches outer handler)
```

Note: After Layer 4 fix, steps 2→4 have minimal delay (no ReadCommandOutput wait).

---

## Code Review — Post-Fix Findings

After the four-layer fix was deployed and verified, a formal code review against all 5 modified files
surfaced 7 warnings and 3 info-level findings. None are critical (crash/security/data-loss), but several
represent robustness gaps and maintainability issues that should be addressed before the next release.

### WR-006 — WriteConsoleInput partial-write not checked

**File:** `src/NetBox/FarPlugin.cpp` (lines 1785–1787 and 1825–1827)

Both `CheckForEsc()` and `FlushEscBuffer()` call `WriteConsoleInput` to restore non-Esc events but ignore
the `Written` count. If the console input buffer is full or the call partially fails, user keystrokes
consumed by `ReadConsoleInput` are silently discarded — the guarantee of preserving non-Esc events
becomes best-effort rather than robust.

**Fix:** Check `Written < NonEscEvents.size()` and log a warning on shortfall.

```cpp
if (!NonEscEvents.empty())
{
    DWORD Written = 0;
    ::WriteConsoleInput(FConsoleInput, NonEscEvents.data(),
        static_cast<DWORD>(NonEscEvents.size()), &Written);
    if (Written < NonEscEvents.size())
    {
        DebugCheck(false);
        // Lost keystrokes — buffer full or API partial failure
    }
}
```

### WR-002/WR-005 — OnceDoneOperation unreachable after throw EAbort

**File:** `src/core/Terminal.cpp`
- `CopyToRemote` catch (lines 7927–7930)
- `CopyToLocal` catch (lines 8521–8523)

The `else` branch unconditionally throws `EAbort("")`, making the subsequent
`OnceDoneOperation = odoIdle` unreachable on the cancellation path. If `EAbort` were ever caught by an
outer handler that continues execution, the stale `OnceDoneOperation` value could trigger unintended
`CloseOnCompletion`.

**Fix:** Move `OnceDoneOperation = odoIdle` before the throw — set it in both branches:

```cpp
if (OperationProgress.GetCancel() == csContinue)
{
    CommandError(&E, MainInstructions(LoadStr(TOREMOTE_COPY_ERROR)));
}
else
{
    OnceDoneOperation = odoIdle;
    throw EAbort("");
}
```

### WR-004 — Cancel condition narrowing silently discards csRemoteAbort

**File:** `src/core/Terminal.cpp` (lines 7924–7929 and 8516+)

The condition was narrowed from `GetCancel() != csCancel` to `GetCancel() == csContinue`. This expands
the `throw EAbort` path to include `csCancelFile`, `csCancelTransfer`, and `csRemoteAbort`. `csRemoteAbort`
is a legitimate state (remote side closed connection mid-transfer) that the original code would show
a `CommandError` for — the new code silently aborts, discarding diagnostics. If a genuine `EFatal`
(connection loss) coincides with a non-Continue cancel state, the fatal error is silently swallowed.

**Fix:** Use a more nuanced condition:

```cpp
// csCancel and csCancelTransfer = user-initiated → silent abort
// csRemoteAbort and csCancelFile = system/remote → show error dialog
if (OperationProgress.GetCancel() == csCancel || OperationProgress.GetCancel() == csCancelTransfer)
{
    throw EAbort("");
}
else
{
    CommandError(&E, MainInstructions(LoadStr(TOREMOTE_COPY_ERROR)));
}
```

### WR-001 — CancelConfiguration guard inconsistent with ShowOperationProgress

**File:** `src/NetBox/WinSCPFileSystem.cpp` (lines 4128–4131)

`CancelConfiguration` uses `GetCancel() > csContinue` as its early-return guard, while
`ShowOperationProgress` uses `GetCancel() < csCancel` as its entry condition. During the transient
`csCancelFile` state, `CheckForEsc()` consumes Esc from the input buffer, but `CancelConfiguration`
returns early without showing a dialog — the Esc press is silently lost. Low practical impact because
`csCancelFile` is transient, but a latent UX gap.

**Fix:** Align the guards. Change `CancelConfiguration` to:
```cpp
if (ProgressData.GetCancel() >= csCancel) return;
```

### WR-003 — CopyToLocal catch block indentation broken

**File:** `src/core/Terminal.cpp` (lines 8510–8528)

The `catch` at 2-space pairs with a `try` at 6-space. Body braces at 8-space vs statements at 4-space.
Reading control flow is needlessly difficult. `CopyToRemote`'s catch is correctly indented.

**Fix:** Re-indent to match CopyToRemote pattern.

### Info-Level Findings

| ID | File | Finding | Fix |
|----|------|---------|-----|
| IN-001 | FarPlugin.cpp:1745 | `static uint32_t LastTicks` not atomic (latent data race) | Change to `std::atomic<uint32_t>` with relaxed ordering |
| IN-002 | Terminal.cpp:7930/8521 | Potential MSVC C4702 unreachable code under /W4 | Same fix as WR-002/WR-005 (restructure) |
| IN-003 | Terminal.cpp:7930/8521 | `throw EAbort("")` uses unnamed literal vs codebase convention | Define `EXCEPTION_MSG_USER_CANCEL` constant |

---

## Anti-Patterns to Avoid

| # | Anti-Pattern | Why It's Wrong | Found In |
|---|-------------|----------------|----------|
| 1 | `PeekConsoleInput(&Rec, 1, ...)` to check for Esc | Only checks first event; non-Esc events block permanently | Layer 1 bug |
| 2 | `break` on non-Esc without consuming it | Leaves blocking event in buffer forever | Layer 1 bug |
| 3 | `case qaCancel: ACancel = csContinue` | User clicking Cancel in dialog continues transfer | Original bug |
| 4 | `throw EAbort(EXCEPTION_MSG_REPLACED)` for user cancel | Magic string triggers panel close via HandleException | Layer 3 bug |
| 5 | `ReadCommandOutput(coWaitForLastLine)` during user cancel | Blocks on remote side's buffered file data | Layer 4 bug |
| 6 | Checking only `csCancelTransfer` in SCP loops | Ignores `csCancel` set by OK/Cancel dialog buttons | Layer 2 bug |
| 7 | Using `GetCancel() == csContinue` instead of `< csCancel` | `csCancelFile` is used internally; would break normal skip-file | ShowOperationProgress guard fix |
| 8 | Ignoring `WriteConsoleInput` return count | Lost keystrokes if buffer full — silent data loss | WR-006 |
| 9 | Throwing new exception in catch block before setting cleanup state | Unreachable cleanup code, stale state if exception caught by outer handler | WR-002/WR-005 |
| 10 | Using `== csContinue` for error-vs-cancel gating | Loses diagnostics for `csRemoteAbort`; swallows genuine fatals | WR-004 |
| 11 | Guard mismatch between caller and callee (`> csContinue` vs `< csCancel`) | Esc press silently discarded during `csCancelFile` window | WR-001 |
| 12 | Unnamed magic values vs named constants for exception dispatch | Intent is opaque; harder to audit HandleException dispatch | IN-003 |
---


## Files Modified (Complete List)

| # | File | Changes | Review Status |
|---|------|---------|---------------|
| 1 | `src/NetBox/FarPlugin.cpp` | `#include <vector>`, rewrite `CheckForEsc()`, rewrite `FlushEscBuffer()` | ⚠ 2 findings (WR-006, IN-001) |
| 2 | `src/core/ScpFileSystem.cpp` | Add `csCancel` to 5 cancel checks + 1 cleanup guard + 3 diagnostic logs | ✓ Clean |
| 3 | `src/core/Terminal.cpp` | Change 2 throws to `EAbort("")` + 2 diagnostic logs | ⚠ 5 findings (WR-002,003,004,005, IN-002,003) |
| 4 | `src/NetBox/WinSCPFileSystem.cpp` | (prior fix) reentrancy guard, CancelConfiguration, condition guards | ⚠ 1 finding (WR-001) |
| 5 | `src/NetBox/WinSCPFileSystem.h` | (prior fix) FInShowOperationProgress, FInCancelDialog members | ✓ Clean |

---

## Verification Results

### Functional Verification

| # | Test | Expected | Actual |
|---|------|----------|--------|
| 1 | Press Esc during SCP upload | Cancel dialog appears | ✓ Verified |
| 2 | Press Esc during SCP download | Cancel dialog appears | ✓ Verified |
| 3 | Click "Yes" in fatal cancel dialog | Transfer aborts immediately | ✓ Verified |
| 4 | Click "OK" in simple cancel dialog | Transfer aborts immediately (csCancel now treated same as csCancelTransfer) | ✓ Verified |
| 5 | Panel after cancel | Panel stays open, no error dialog | ✓ Verified |
| 6 | Transfer log after cancel | No lengthy `ReadCommandOutput` wait | ✓ Verified |
| 7 | Esc detection with mouse events in buffer | Esc still detected (full-buffer scan) | ✓ Verified |
| 8 | Esc detection after key release in buffer | Esc still detected (not blocked by non-Esc events) | ✓ Verified |

### Code Review

| Metric | Result |
|--------|--------|
| Files reviewed | 5 |
| Clean files | 2 (`ScpFileSystem.cpp`, `WinSCPFileSystem.h`) |
| Critical findings | 0 |
| Warning findings | 7 |
| Info findings | 3 |
| Review depth | standard |
| Review output | [.ai-factory/REVIEW-esc-cancellation.md](.ai-factory/REVIEW-esc-cancellation.md) |

## Related Documentation

- [Issue #511 Plan — Speed Limit & Esc Hang](.ai-factory/plans/issue-511-speed-limit-esc-hang.md)
- [Code Review Report — Esc Cancellation](.ai-factory/REVIEW-esc-cancellation.md)
- [AGENTS.md — NetBox Development Guide](AGENTS.md)
- [ARCHITECTURE.md — Layered plugin architecture](.ai-factory/ARCHITECTURE.md)

