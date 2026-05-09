# Plan: Threading Safety Audit and Fixes

**Branch:** lmv/dev (current)
**Date:** 2026-05-10
**Mode:** Fast

## Description

Address threading defects identified across the NetBox codebase. Issues span CRITICAL (Far Manager API invoked from worker threads), HIGH (race conditions in speed-limit throttling and progress callbacks without locks), and MEDIUM (busy-waiting polling loops and unsynchronized global mutable state).

## Scope & Affected Files

| Severity | Defect | File(s) |
|---|---|---|
| CRITICAL | Far Manager API called from worker thread | `src/NetBox/WinSCPFileSystem.cpp` |
| HIGH | UNLOCK→Sleep→RELOCK race (TOCTOU) | `src/filezilla/FtpControlSocket.cpp` |
| HIGH | Callback without lock | `src/core/FileOperationProgress.cpp` |
| MEDIUM | Busy-waiting with `Sleep()` | `src/core/Terminal.cpp`, `src/core/HierarchicalStorage.cpp`, `src/windows/ConsoleRunner.cpp`, `src/windows/GUITools.cpp` |
| MEDIUM | Static/global mutable state without sync | `src/base/Global.cpp`, `src/core/CoreMain.cpp` |

## Constraints

- **Main thread rule:** All Far Manager API calls (`DialogInit`, `Message`, `SaveScreen`, `UpdateConsoleTitle`, etc.) MUST run on the main Far thread only.
- **No modifications to `libs/`** — use patches if third-party code is affected.
- **MSVC /W4 zero warnings.**
- **CRLF endings, UTF-8 without BOM.**
- Prefer event-based waiting over `Sleep()` polling where feasible.
- Preserve existing thread-marshaling patterns (`Synchronize`, `PostMainThreadSynchro`, `WaitForUserAction`).

---

## Phase 1: CRITICAL — Far Manager API from Worker Thread

### Task 1.1: Add worker-thread guard to `TerminalReadDirectoryProgress`

**Target:** `src/NetBox/WinSCPFileSystem.cpp` line ~3691

**Problem:** `TerminalReadDirectoryProgress` calls `MoreMessageDialog`, `CheckForEsc`, `UpdateConsoleTitle` without a thread guard. `OperationProgress` (~3901) already has a `GetCurrentThreadId() != FMainThreadId` bail-out proving the terminal fires callbacks from worker threads.

**Fix:** Add the same `GetCurrentThreadId() != FMainThreadId` guard at the top of `TerminalReadDirectoryProgress`:
```cpp
void TWinSCPFileSystem::TerminalReadDirectoryProgress(...)
{
  if (::GetCurrentThreadId() != FMainThreadId)
  {
    return;   // skip Far API calls from worker thread
  }
  // ... existing body ...
}
```

**Edge case:** When called from the main thread, behavior is unchanged.

### Task 1.2: Audit `TerminalInformation` for worker-thread dispatch

**Target:** `src/NetBox/WinSCPFileSystem.cpp` line ~3623

**Problem:** `TerminalInformation` is registered on `FTerminal` alongside the guarded `OperationProgress`. It calls `UpdateConsoleTitle` and potentially `MoreMessageDialog`.

**Audit step:** Trace `TTerminal::Information()` dispatch paths to confirm whether it fires from worker threads:
- `TTerminalItem::ProcessEvent` → `FTerminal->Information()` → callback
- `TSecureShell::Information()` → `FUI->Information()` → `TTunnelUI::Information()` → marshals to main thread ✓
- Direct `TTerminal::Information()` → `FOnInformation` → may fire from any thread

**Decision after audit:**
- If confirmed from worker threads: add same `GetCurrentThreadId()` guard.
- If only from main thread: document in comment why no guard is needed.

**Note:** `TTunnelUI::Information()` already marshals to `FTerminal->Information()` (line ~274-280), so the tunnel path is safe. The direct-terminal path is the concern.

---

## Phase 2: HIGH — Race Conditions

### Task 2.1: Fix TOCTOU race in `CFtpControlSocket::GetAbleToUDSize()`

**Target:** `src/filezilla/FtpControlSocket.cpp` lines ~6030–6062

**Problem:** The function releases `m_SpeedLimitSync`, sleeps on `m_SpeedLimitEvent` for 100 ms, then re-acquires `m_SpeedLimitSync`. Between unlock and lock, another thread can modify `m_InstanceList` via `RemoveActiveTransfer`, invalidating the `iter` parameter.

**Fix:** Refactor to avoid passing iterator reference across the sleep window. Return the limit value directly; caller re-searches if needed after the call. `CCriticalSectionWrapper` is recursive-safe (Windows `CRITICAL_SECTION`), so re-entrant lock/unlock is safe.

**Current code (lines 6060–6062):**
```cpp
m_SpeedLimitSync.Unlock();
::WaitForSingleObject(m_SpeedLimitEvent, 100);
m_SpeedLimitSync.Lock();
```

**Alternative approach:** Copy the lookup key (`this` pointer + `direction`) before unlocking. After re-locking, re-search the list from scratch. The code already re-searches at lines 6066–6070, but the `iter` reference passed by the caller may still be used afterward at line ~6143.

**Edge case:** `GetAbleToTransferSize()` (line ~6127) holds the outer lock and calls `GetAbleToUDSize`, which drops the lock internally — this is a re-entrant lock release. `CCriticalSectionWrapper` supports recursive lock/unlock on the same thread (Windows `CRITICAL_SECTION` behavior).

### Task 2.2: Fix `FileOperationProgress` callback/data race

**Target:** `src/core/FileOperationProgress.cpp`

**Problem A — callers modify fields without lock:**
`SetFile()` (line ~484-500) modifies `FFullFileName`, `FFileName`, `FFileInProgress`, `FFileStartTime` without `FSection`, then calls `DoProgress()`.
`AddTransferred()` (line ~878-898) modifies `FTransferredSize`, `FTransferSize` without `FSection`, then calls `DoProgress()`.
Many other callers have the same pattern.

**Problem B — `DoProgress()` calls `FOnProgress` without `FSection`:**
Lines 418-434. If `FOnProgress` executes on a different thread (main thread UI update), it reads fields while worker threads modify them.

**Fix (two-part):**

**Part 1 — Lock field modifications in callers:**
Add `TGuard Guard(FSection)` at the start of methods that modify shared state before calling `DoProgress()`:
- `SetFile()`
- `AddTransferred()`
- `SetLocalSize()`
- `SetTransferSize()`
- `ChangeTransferSize()`
- `AddLocallyUsed()`
- etc.

**Part 2 — Lock `DoProgress()` body:**
Hold `FSection` around the entire `DoProgress()` body. `TCriticalSection` is recursive, so re-entrant callbacks are safe. The existing `FInCallback` guard already prevents infinite recursion.

```cpp
void TFileOperationProgressType::DoProgress()
{
  SystemRequired();
  if (!FInCallback)
  {
    TValueRestorer<bool> InCallbackRestorer(FInCallback);
    FInCallback = true;
    const TGuard Guard(FSection);   // <-- ADD
    FOnProgress(*this);
  }
}
```

**Problem C — `GetCancel()` reads `FCancel` without lock:**
Lines 681-694. `SetCancel()` (line 656) and `SetCancelAtLeast()` (line 667) write `FCancel` under `FSection`. `GetCancel()` reads it without a lock.

**Fix:** Add `TGuard Guard(FSection)` at the top of `GetCancel()` (before reading `FCancel`). The parent lookup path already acquires the lock conditionally; unify so the local read is always locked. Note: `GetCancel()` is `const`; `FSection` must be `mutable` or the lock must use a non-const reference.

**Verify:** Check if `FSection` is declared `mutable` or if the `const` can be removed from `GetCancel()` (check all callers).

---

## Phase 3: MEDIUM — Busy-Waiting with `Sleep()`

### Task 3.1: Verify `TParallelOperation::WaitFor()` event path

**Target:** `src/core/Terminal.cpp` lines ~827–864

**Finding from code read:** `FClientsZeroEvent` is **always** created in the constructor (line 750): `FClientsZeroEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);`. The `Sleep(200)` fallback at line 853 is **dead code** under normal conditions — the null check is purely defensive.

**Fix:** Remove the defensive null check and the `Sleep(200)` fallback, OR add a `DebugAssert(FClientsZeroEvent != nullptr)` to document the invariant. Simplify `WaitFor()`:

```cpp
void TParallelOperation::WaitFor()
{
  if (FSection != nullptr)
  {
    bool Done;
    do
    {
      {
        const TGuard Guard(*FSection.get());
        Done = (FClients == 0);
      }
      if (!Done)
      {
        FMainOperationProgress->Progress();
        ::WaitForSingleObject(FClientsZeroEvent, 200);
        ::ResetEvent(FClientsZeroEvent);
      }
    }
    while (!Done);
    FProbablyEmpty = true;
  }
  else
  {
    DebugAssert(FClients == 0);
  }
}
```

**Rationale:** The event is always created; defensive null checks add unnecessary branching and hide the real design intent.

### Task 3.2: Verify `TTerminal::CopyParallel()` event path

**Target:** `src/core/Terminal.cpp` lines ~7820–7847

**Finding from code read:** The code **already uses event-based waiting first**:
```cpp
if (!ParallelOperation->WaitForDirectoryCreated(100))
{
  Sleep(100);
}
```

`WaitForDirectoryCreated(100)` uses `WaitForSingleObject(FDirectoryCreatedEvent, 100)` (line 866-872). The `Sleep(100)` is only reached when the event times out (no directory created within 100ms). This is already the correct pattern — event primary, Sleep degraded fallback.

**Fix:** No code change needed. Document with a comment explaining the intentional fallback:
```cpp
// Event-based wait first; Sleep only if no directory was created within 100ms
if (!ParallelOperation->WaitForDirectoryCreated(100))
{
  Sleep(100);
}
```

### Task 3.3: Audit remaining `Sleep()` loops

**Targets:**
- `src/core/HierarchicalStorage.cpp` ~1832–1852 — `CreateFile` retry on `ERROR_SHARING_VIOLATION`. Acceptable defensive retry; document with comment.
- `src/windows/ConsoleRunner.cpp` — `TOwnConsole` input polling (`Sleep(50)`), `WaitBeforeExit` (`Sleep(50)`), `TNullConsole::Choice` (`Sleep(Timer)`). Console-mode loops; evaluate if `WaitForSingleObject` on console input handle (`GetStdHandle(STD_INPUT_HANDLE)`) can replace them.
- `src/windows/GUITools.cpp` — `TPuttyCleanupThread` (`Sleep(100)`, `Sleep(400)`). These are cleanup threads waiting for child process exit. **Replace with `WaitForSingleObject` on the process handle.**

**Decision:** Only fix `GUITools.cpp` cleanup threads (clear event-based alternative). Document the rest with `// busy-wait fallback: <reason>`.

---

## Phase 4: MEDIUM — Unsynchronized Global Mutable State

### Task 4.1: Protect debug tracing globals in `Global.cpp`

**Target:** `src/base/Global.cpp` lines ~69–76

**Globals:** `TraceFile`, `IsTracing`, `CallstackTls`, `TracingCriticalSection`, `TracingInMemory`, `TracingThread`

**Problem:** `DoDirectTrace`/`DoTrace` read `TraceFile` and `IsTracing` without locks. The file already contains a thread-safety inventory comment acknowledging this.

**Fix:** Use `TracingCriticalSection` (which already exists) to guard reads of `TraceFile` and `IsTracing`. Ensure the lock is held for the shortest duration (just the write-to-file call).

**Note:** `CallstackTls` is a TLS index (`uint32_t`), set once and read-only after init. `TracingThread` is a thread handle, set once. These do not need ongoing synchronization.

### Task 4.2: Protect `AnySession` and `StoredSessionsInitialized` in `CoreMain.cpp`

**Target:** `src/core/CoreMain.cpp` lines ~22–23, ~170

**Problem:**
- `AnySession` is written from `Terminal.cpp:1564` (`TTerminal::Open`) and read from `CoreMain.cpp:328` (`CoreUpdateFinalStaticUsage`) with no lock.
- `StoredSessionsInitialized` is toggled in `GetStoredSessions`/`DeleteStoredSessions` without a lock.

**Fix:** Add a lightweight `TCriticalSection` or `std::atomic<bool>` for `AnySession`. For `StoredSessionsInitialized`, use the same critical section that protects `StoredSessions` access (if one exists) or add a dedicated one.

**Verify:** Check if `StoredSessions` already has a protecting lock (search for `StoredSessions` access patterns in `CoreMain.cpp` and `SessionData.cpp`).

---

## Verification

### Build
- `cmd /c build-x64.bat` → zero warnings

### .lng Alignment
- `python scripts/verify_lng_alignment.py` → exit 0

### Static Analysis (Recommended)
- Re-run the same threading audit that produced the original severity table. Confirm each fixed issue is resolved or downgraded.

### Manual Regression
- Test SSH/SFTP transfer with progress dialog (validates `OperationProgress`, `FileOperationProgress` locking).
- Test FTP transfer with speed limit enabled (validates `FtpControlSocket` race fix).
- Test directory listing on slow server (validates `TerminalReadDirectoryProgress` guard).

### Quality Gates
- [ ] Zero build warnings under MSVC `/W4`
- [ ] No modifications to `libs/` directory
- [ ] CRLF line endings on all modified files
- [ ] UTF-8 without BOM in all text files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No new `Sleep()` loops introduced
- [ ] Final verification: `Run /aif-verify` to validate the implementation against the plan
