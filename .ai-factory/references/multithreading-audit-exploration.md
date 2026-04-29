# Multithreading Audit Exploration

> Date: 2026-04-29
> Scope: Comprehensive review of threading and synchronization patterns in NetBox
> Agents: ThreadingPrimitives, ThreadingProtocols, FarManagerThreading

## Summary

Three parallel explore agents analyzed `src/`, `core/`, `filezilla/`, `NetBox/`, `base/`, `nbcore/`, and `windows/` for concurrency defects. Findings are organized by severity and defect class.

---

## Critical: Far Manager API Thread Affinity Violations

The AGENTS.md rule states: *"All Far Manager API calls from main thread only."* Multiple violations were found.

| File | Violation | Context |
|------|-----------|---------|
| `src/NetBox/FarDialog.cpp` | `TFarDialogIdleThread::Execute` calls `FDialog->Idle()` from worker thread; derived dialogs call `FarAdvControl(ACTL_SYNCHRO)` | Idle thread bypasses main thread |
| `src/NetBox/FarPlugin.cpp` | `TPluginIdleThread::Execute` calls `FPlugin->FarAdvControl` directly from worker thread | Same pattern as above |
| `src/NetBox/WinSCPDialogs.cpp` | `TQueueDialog::Idle` calls `FarAdvControl(ACTL_SYNCHRO)` from worker thread | Progress dialog idle |
| `src/core/Queue.cpp` | `TTerminalThread::RunAction` executes `FUserAction->Execute` on worker thread; callbacks (`TerminalQueryUser`, `TerminalPromptUser`, etc.) invoke Far APIs off main thread | Terminal UI callbacks |
| `src/NetBox/WinSCPFileSystem.cpp` | `TerminalInformation`, `TerminalQueryUser`, `TerminalPromptUser`, `TerminalShowExtendedException`, `TerminalDisplayBanner`, `TerminalStartReadDirectory`, `TerminalReadDirectoryProgress` all invoke Far dialogs/screen/title updates | Called from `TTerminalThread` |
| `src/NetBox/Far3Storage.h` | `TFar3Storage` wraps `SettingsControl`; potential indirect calls from worker threads via `SaveSession` | Storage API crossing threads |

Architecture note: `TTerminalItem` queue correctly marshals to main thread for UI; `TTerminalThread` lacks this marshaling.

---

## High: Race Conditions

| File | Pattern | Risk |
|------|---------|------|
| `src/filezilla/FtpControlSocket.cpp` (lines 6072-6074) | `UNLOCK -> Sleep(100) -> RELOCK` on `m_SpeedLimitSync` | State change during sleep window; classic race |
| `src/core/Queue.cpp` (lines 2404-2409) | `FLastParallelOperationAdded` updated before operation added, rollback on failure | Partial ordering inconsistency |
| `src/core/FileOperationProgress.cpp` | `DoProgress()` calls `FOnProgress` without holding `FSection` lock | Callback reentrancy from multiple threads |

---

## Medium: Busy-Waiting & Inefficient Polling

| File | Pattern | Suggested Replacement |
|------|---------|----------------------|
| `src/core/Terminal.cpp` (line 833) | `Sleep(200)` in wait loop | Event-driven wait with timeout |
| `src/core/Terminal.cpp` (line 7705) | `while(Continue && !Cancel)` with `Sleep(100)` | Condition variable or signaled event |
| `src/filezilla/MainThread.cpp` (line 403) | `Sleep(10)` in idle loop | Waitable event or message pump |
| `src/core/Queue.cpp` (lines 456-474) | `WaitForEvent` with short timeouts | Event or semaphore-based blocking |

---

## Medium: Static/Global Mutable State Without Explicit Synchronization

| File | State | Current Protection |
|------|-------|-------------------|
| `src/base/Classes.cpp` (lines 11-14) | `GlobalFunctions` static pointer | Lazy init, no explicit sync |
| `src/base/Global.cpp` (lines 69-72) | `TraceFile`, `IsTracing`, `CallstackTls`, `TracingCriticalSection` | `TracingCriticalSection` present but verify coverage |
| `src/base/Common.cpp` (line 2463) | `DateTimeParamsSection` (yearly datetime params) | Protected by `TCriticalSection` |
| `src/base/Exceptions.cpp` (line 17) | `IgnoredExceptionsCriticalSection` | Protected, verify RAII usage |
| `src/core/SessionData.cpp` (line 2054) | `PasswordFilesCacheSection` | Protected, verify all paths |
| `src/core/NeonIntf.cpp` (line 425) | `DebugSection` for `NeonTerminals` set | Verify unregister path safety |
| `src/core/S3FileSystem.cpp` (line 80) | `LibS3Section` | Verify all `libs3` entry points |

---

## Low/Medium: Thread-Local Storage Gaps

Only `src/base/LogContext.h/cpp` uses explicit `thread_local` (`TLogContext::ContextStack_`). Other per-thread state (e.g., `CallstackTls`, `m_sGlobalCriticalSection` thread-local data in `AsyncSocketEx.cpp`) may benefit from `thread_local` where correctness requires it.

---

## Known Previous Issues

| Issue | File | Status |
|-------|------|--------|
| #501 — SSH/SCP buffer corruption | `src/core/SessionData.cpp`, `src/core/SecureShell.cpp` | Mitigated (`SetSendBuf(0)`, `SetSshSimple(false)`); threading root cause partially addressed |
| #511 — Speed limit / Esc hang | `src/core/Queue.cpp`, `src/core/FileOperationProgress.cpp` | Fixed 2026-04; verify no regression |

---

## Positive Patterns to Preserve

| Pattern | File | Description |
|---------|------|-------------|
| RAII lock guards | `src/base/Global.cpp` | `TGuard`, `TUnguard` around critical sections |
| Reentrancy guard | `src/core/Terminal.h` | `TTunnelUI` stores `FTerminalThreadID`, validates thread before forwarding |
| Deadlock prevention | `src/core/Queue.cpp` (lines 2790-2794) | `TTerminalThread::Idle()` uses `TryEnter` to avoid deadlock |
| Try/catch in thread proc | `src/core/Queue.cpp` (lines 254-329) | `ThreadProc` catches exceptions, logs, terminates gracefully |
| Reference counting | `src/base/include/nbstring.h` | `InterlockedIncrement`/`InterlockedDecrement` for thread-safe strings |

---

## OpenSSL Thread Safety Note

`src/core/Cryptography.cpp` (lines 658-667): `OPENSSL_init_crypto` fails when called from a background thread after `RAND_poll` in foreground. This is a known upstream limitation. NetBox previously rejected a clean fix as a regression risk. Plan should evaluate whether a thread-safe lazy-init wrapper is now acceptable.

---

## Files Requiring Detailed Review

- `src/NetBox/FarDialog.cpp`
- `src/NetBox/FarPlugin.cpp`
- `src/NetBox/WinSCPDialogs.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`
- `src/core/Queue.cpp`
- `src/core/Queue.h`
- `src/core/Terminal.cpp`
- `src/core/FileOperationProgress.cpp`
- `src/core/FtpFileSystem.cpp`
- `src/filezilla/FtpControlSocket.cpp`
- `src/filezilla/MainThread.cpp`
- `src/base/Classes.cpp`
- `src/base/Global.cpp`
- `src/core/Cryptography.cpp`
