# Multithreading Review and Fix Results

> Date: 2026-04-29
> Scope: Implementation of multithreading-review-fix.md plan across all codebase layers
> Result: 14 tasks completed across 4 phases

---

## Summary

All planned multithreading fixes were implemented. This reference documents the changes applied and the real bugs discovered during implementation.

---

## Critical Fixes

### Far Manager API Thread Affinity (Phase II)

| File | Original Defect | Fix Applied |
|---|---|---|
| `src/NetBox/FarDialog.cpp` | `TFarDialogIdleThread::Execute` called `FDialog->Idle()` from worker thread | Replaced with `FDialog->Synchronize(nb::bind(&TFarDialog::Idle, FDialog))` — marshals to main thread via existing semaphore+event mechanism |
| `src/NetBox/FarPlugin.cpp` | `TPluginIdleThread::Execute` called `FarAdvControl(ACTL_SYNCHRO)` directly from worker thread | Introduced `TCustomFarPlugin::PostMainThreadSynchro()` wrapper; worker thread calls this instead |
| `src/NetBox/FarPlugin.h` | — | Declared `PostMainThreadSynchro(void * Param = nullptr) const` |
| `src/NetBox/WinSCPDialogs.cpp` | `TQueueDialog::Idle()` called `FarAdvControl(ACTL_SYNCHRO)` from worker thread | Now calls `GetFarPlugin()->PostMainThreadSynchro(&SynchroParams)`; because Task 4 already marshals `Idle()` to main thread, this executes on main thread |
| `src/core/Queue.cpp` | `TTerminalThread::RunAction` executed `FUserAction->Execute` without verifying main thread | Added `DebugAssert(GetCurrentThreadId() == FMainThread)` before callback execution; documented marshal handshake between `RunAction` and `WaitForUserAction` |

---

## Race Condition Fixes

### FtpControlSocket Speed Limit Race (Task 8)

| File | Change |
|---|---|
| `src/filezilla/FtpControlSocket.h` | Added `static HANDLE m_SpeedLimitEvent` |
| `src/filezilla/FtpControlSocket.cpp` | Replaced `Unlock(); Sleep(100); Lock()` with `Unlock(); WaitForSingleObject(m_SpeedLimitEvent, 100); Lock()`; added `::SetEvent(m_SpeedLimitEvent)` in `SpeedLimitAddTransferredBytes()` after `nBytesAvailable` update |

### FileOperationProgress Callback Reentrancy (Task 10)

| File | Change |
|---|---|
| `src/core/FileOperationProgress.h` | Added `bool FInCallback{false}` |
|| `src/core/FileOperationProgress.cpp` | Wrapped `FOnProgress(*this)` invocation with `if (!FInCallback)` guard using `TValueRestorer<bool>`; reentrant calls are suppressed with a comment (replaced undefined `LOG_WARN` macro with a comment during verification).

---

## Busy-Wait Elimination (Task 9)

| File | Original Pattern | Replacement |
|---|---|---|
| `src/filezilla/MainThread.cpp` | `ResumeThread()` then `Sleep(10)` polling loop waiting for `m_Started` | Added `m_hStartedEvent`; `ResumeThread()` now waits on event with `INFINITE`; signaled in `Run()` when `m_Started = true` |
| `src/core/Terminal.cpp` | `TParallelOperation::WaitFor()` `Sleep(200)` poll on `FClients == 0` | Added `FClientsZeroEvent`; signaled in `RemoveClient()`; `WaitFor()` waits on event |
| `src/core/Terminal.cpp` | `CopyParallel()` `Sleep(100)` poll on directory creation | Added `FDirectoryCreatedEvent`; signaled in `Done()` when `Exists` becomes `true`; `CopyParallel()` waits on event |
| `src/core/Terminal.h` | — | Added `FClientsZeroEvent` and `FDirectoryCreatedEvent` declarations |

### Intentionally Preserved Sleep

`src/core/FileOperationProgress.cpp` line ~485: `SleepEx(100, true)` is an intentional alertable wait for APC completion. Documented in comments; excluded from conversion.

---

## Static State & Thread Safety Hardening (Phase I)

| File | State / Defect | Resolution |
|---|---|---|
| `src/base/Classes.cpp` | `GlobalFunctions` pointer | Documented single-threaded init safety (set once before worker threads) |
| `src/base/Global.cpp` | `TraceFile`, `IsTracing`, `CallstackTls`, `TracingCriticalSection` | Hardened `WriteTraceBuffer()` and `DoTrace()` to acquire `TracingCriticalSection` when non-null; added thread-safety inventory comments |
| `src/base/Common.cpp` | `DateTimeParamsSection` / `YearlyDateTimeParams` | Documented existing `TCriticalSection` protection |
| `src/base/Exceptions.cpp` | `IgnoredExceptionsCriticalSection` | Documented existing protection |
| `src/core/NeonIntf.cpp` | `DebugSection` / `NeonTerminals` | Documented existing protection |
| `src/core/S3FileSystem.cpp` | `LibS3Section` | Documented; **fixed real bug**: `TGuard Guard(LibS3Section.get())` → `TGuard Guard(*LibS3Section.get())` (pointer-to-reference mismatch) |
| `src/core/SessionData.cpp` | `ProxyMethodMapping` | Documented MSVC C++11 thread-safe static initialization |
| `src/nbcore/nbstring.cpp` | `nbstr_getNil()` lazy-init race | Replaced global `m_nil` with function-local `static CNilMStringData * nil = new CNilMStringData();` (C++11 thread-safe static init); documented `nbstr_lock`/`nbstr_unlock` non-atomic design intent |

---

## OpenSSL Initialization (Task 11)

| File | Change |
|---|---|
| `src/core/Cryptography.cpp` | Wrapped `InitOpenssl()` in `std::call_once(OpenSSLInitOnce, ...)` to guarantee single-threaded initialization regardless of which thread calls it first; added `#include <mutex>` |

---

## Lock Ordering Documentation (Task 3)

| File | Documentation Added |
|---|---|
| `src/core/Queue.h` | Documented hierarchy `TTerminalQueue::FItemsSection` -> `TQueueItem::FSection`; noted `TCriticalSection` recursion |
| `src/core/FileOperationProgress.h` | Documented dual-acquisition in `Assign()` and recursive safety |
| `src/filezilla/FtpControlSocket.h` | Documented `m_SpeedLimitSync` as leaf lock; warned against `Unlock/Sleep/Lock` anti-pattern |

---

## Documentation Artifacts Created

| File | Purpose |
|---|---|
| `.ai-factory/rules/threading.md` | Enforceable threading conventions: Far API main-thread rule, event-driven waits, lock ordering, static state protection, progress callback rules |

---

## Known Limitations

- **Build verification** cannot be performed on the Linux workstation; MSVC W4 compilation must be verified on a Windows environment.
- `FileOperationProgress::Assign()` now uses address-based ordering to prevent deadlock under concurrent bidirectional assignment. See `.ai-factory/PLAN.md` Task 2.

---

## Related References

- [Multithreading Audit Exploration](multithreading-audit-exploration.md) — Pre-implementation audit that identified these defects
- [Multithreading Review and Fix Plan](../plans/multithreading-review-fix.md) — The plan this reference implements
- [Threading Rules](../rules/threading.md) — Enforceable conventions derived from this work
