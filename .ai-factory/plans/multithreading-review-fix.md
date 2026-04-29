# Multithreading Review and Fix

> Mode: fast
> Created: 2026-04-29
> Branch: none (fast mode)

## Settings

| Setting | Value |
|---------|-------|
| Testing | no |
| Logging | verbose |
| Docs    | yes |

## Roadmap Linkage

| Milestone | Rationale |
|-----------|---------|
| Technical Debt / Refactoring | Threading defects are stability-critical technical debt. Resolving them prevents hangs, crashes, and race-related data corruption before Version 1.2 background copy work expands thread surface area. |

## Research Context
Three parallel explore agents audited `src/`, `core/`, `filezilla/`, `NetBox/`, `base/`, and `windows/` for concurrency defects. Full findings are preserved in [Multithreading Audit Exploration](../references/multithreading-audit-exploration.md).

Implementation results and applied fixes are documented in [Multithreading Review and Fix Results](../references/multithreading-review-fix-results.md).

Key discoveries:

1. **Critical:** Multiple Far Manager API calls originate from worker threads (`TFarDialogIdleThread`, `TPluginIdleThread`, `TQueueDialog::Idle`). `TTerminalThread` UI callbacks are already marshaled to the main thread via `RunAction`/`WaitForUserAction`; verify this handshake is complete.
2. **High:** `FtpControlSocket.cpp` uses an unlock-sleep-relock pattern on `m_SpeedLimitSync` that creates a race window.
3. **Medium:** Busy-waiting loops with `Sleep()` exist in `Terminal.cpp` and `MainThread.cpp` (genuine polling); `Queue.cpp` already uses event objects via `TSignalThread::WaitForEvent`.
4. **Medium:** `FileOperationProgress.cpp` invokes `FOnProgress` without holding its section lock.
5. **Medium:** Several static/global mutable objects lack explicit synchronization or rely on lazy init without once-semantics.
6. **Low/Medium:** OpenSSL `OPENSSL_init_crypto` thread-safety limitation in `Cryptography.cpp` remains unresolved.

## Architecture Notes

This plan touches all layers. Observe the dependency flow:

```
Plugin Layer (NetBox/)  <-- worker threads must NOT call Far APIs directly here
        ↓
Core Layer (core/)      <-- queue threads, progress callbacks, protocol threads
        ↓
Base Layer (base/)      <-- static/global state, string refcounts, logging context
        ↓
Third-Party (libs/)     <-- no modifications; patch if required
```

- No files in `libs/` are modified.
- Build target: `RelWithDebugInfo` with `OPT_CREATE_PLUGIN_DIR=ON`.
- Compiler: MSVC W4 (zero warnings).
- Platforms: x86, x64, ARM64.

---

## Phase I. Foundation — Audit & Static State Hardening

### [x] Task 1: Audit and document all static/global mutable state

**File(s):** `src/base/Classes.cpp`, `src/base/Global.cpp`, `src/base/Common.cpp`, `src/base/Exceptions.cpp`, `src/core/SessionData.cpp`, `src/core/NeonIntf.cpp`, `src/core/S3FileSystem.cpp`

**Change:**
- Create a thread-safety inventory table in code comments for each static `TCriticalSection` and its protected state.
- Verify that every mutable static/global variable is either (a) `const` after init, (b) protected by a `TCriticalSection`, or (c) converted to `thread_local`.
- For `GlobalFunctions` pointer in `Classes.cpp`, verify it is set once during single-threaded startup before any worker threads are created. If a race is proven, replace with `std::call_once` or `InitOnceExecuteOnce`; otherwise document the single-threaded safety in comments.

**Logging:**
- `LOG_DEBUG` in each initialization path with `GetCurrentThreadId()`.
- `FTerminal->LogEvent()` when applicable in core-layer globals.

---

### [x] Task 1.5: Fix `TGuard` dereference bug in `S3FileSystem.cpp`

**File(s):** `src/core/S3FileSystem.cpp`

**Change:**
- During Task 1 audit, a real bug was discovered: `TGuard Guard(LibS3Section.get())` passed a pointer instead of a reference to `TGuard`.
- Correct to `TGuard Guard(*LibS3Section.get())` at all four call sites (`S3ReadFile`, `S3GetAccessControl`, `InitLibS3`, `CleanupLibS3`).

**Logging:**
- N/A (defensive fix discovered during audit).

---


### [x] Task 2: Harden reference-counted string operations

**File(s):** `src/base/include/nbstring.h`, `src/base/nbstring.cpp`

**Change:**
- Audit `InterlockedIncrement` / `InterlockedDecrement` usage.
- Ensure no non-atomic read-modify-write paths exist (e.g., `if (ref == 0)` checks outside the interlocked op).
- Add `static_assert(sizeof(LONG) == sizeof(std::atomic<long>), "")` or MSVC-specific `_InterlockedIncrement` correctness checks only if a gap is found; otherwise document the existing safety in comments.

**Logging:**
- `LOG_DEBUG` on string ref count underflow/overflow detection (add defensive checks).

---

### [x] Task 3: Establish and document lock ordering hierarchy

**File(s):** `src/core/Queue.h`, `src/core/Terminal.h`, `src/core/FileOperationProgress.h`, `src/filezilla/FtpControlSocket.h`

**Change:**
- Document the acquisition order in a single header comment (e.g., `FSection` -> `FItemsSection` -> `FCriticalSection`).
- Verify that no code path acquires locks in a different order.
- If cycles exist, refactor to use a single coarse lock or introduce an explicit ordering layer (e.g., always acquire protocol lock before progress lock).

**Logging:**
- `LOG_DEBUG` when entering nested lock contexts; log lock name and thread ID.

---

### [x] Task 3.5: Evaluate address-based lock ordering for `FileOperationProgress::Assign()`

**File(s):** `src/core/FileOperationProgress.cpp`, `src/core/FileOperationProgress.h`

**Change:**
- `Assign()` acquires `this->FSection` then `Other.FSection` in fixed order. This is safe only if assignments are strictly unidirectional.
- Evaluate whether `std::lock`-style address-based ordering is needed to prevent deadlocks under concurrent cross-assignment scenarios.
- Document the decision in `FileOperationProgress.h` comments (deferred or implemented).

**Logging:**
- N/A (analysis task).

---


## Phase II. Critical — Far Manager Thread Safety

### [x] Task 4: Fix `TFarDialogIdleThread` Far API thread affinity

**File(s):** `src/NetBox/FarDialog.cpp`

**Change:**
- Reuse the existing `TFarDialog::Synchronize(TThreadMethod)` semaphore+event mechanism (FarDialog.cpp:811-822). Have `TFarDialogIdleThread` set a flag and let the main thread execute `Idle()` via `Synchronize()`. If blocking the idle thread is unacceptable, post a non-blocking flag that the main thread checks during its message loop.
- Ensure derived dialogs (e.g., `TQueueDialog`) inherit the fix automatically.

**Logging:**
- `LOG_DEBUG` on signal post (worker thread) and on idle execution (main thread).
- `FTerminal->LogEvent()` if idle processing is deferred.

---

### [x] Task 5: Fix `TPluginIdleThread` Far API thread affinity

**File(s):** `src/NetBox/FarPlugin.cpp`

**Change:**
- Same pattern as Task 4: `TPluginIdleThread::Execute` must not call `FarAdvControl` directly.
- Queue an event to the main thread and return immediately.

**Logging:**
- `LOG_DEBUG` on worker thread signal and main thread handler invocation.

---

### [x] Task 6: Fix `TTerminalThread` UI callback marshaling

**File(s):** `src/core/Queue.cpp`, `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPFileSystem.h`

**Change:**
- Verify that all callbacks use `WaitForUserAction`, which marshals to the main thread via `RunAction`'s `FActionEvent` wait loop (Queue.cpp:2845-2880). The marshaling already exists; this task hardens it.
- Add `DebugAssert(GetCurrentThreadId() == FMainThread)` before `FUserAction->Execute(nullptr)` in `RunAction`.
- Document the marshal handshake (`WaitForUserAction` on worker thread → `RunAction` wait loop on main thread) in comments.
- Ensure the `TTunnelUI` thread-ID guard pattern is extended to all callback entry points where missing.

---

### [x] Task 7: Fix `TQueueDialog::Idle` thread affinity

**File(s):** `src/NetBox/WinSCPDialogs.cpp`

**Change:**
- `TQueueDialog::Idle()` is invoked by `TFarDialogIdleThread`. If Task 4 marshals `TFarDialog::Idle()` to the main thread, this runs on the main thread automatically. Verify the propagation.
- If a standalone fix is needed, use `TFarDialog::Synchronize()` or post an `ACTL_SYNCHRO` request from the main thread only.

**Logging:**
- `LOG_DEBUG` on idle signal and handler execution.

---

## Phase III. Protocol — Race Conditions & Busy-Waiting

### [x] Task 8: Fix `FtpControlSocket` unlock-sleep-relock race

**File(s):** `src/filezilla/FtpControlSocket.cpp` (around lines 6072-6074)

**Change:**
- Replace the `m_SpeedLimitSync.Unlock(); Sleep(100); m_SpeedLimitSync.Lock();` pattern with an event-driven wait (e.g., a waitable event signaled by `AddTransferred` when `nBytesAvailable` changes, or a waitable timer for second rollover).
- Note: `src/filezilla/` is NetBox-derived code (not in `libs/`), so modification is allowed, but keep changes minimal to reduce divergence from upstream.

**Logging:**
- `FTerminal->LogEvent()` when speed limit changes and when wait resumes.

---

### [x] Task 9: Replace busy-waiting loops with event-driven waits

**File(s):** `src/core/Terminal.cpp` (lines 833, 7705), `src/filezilla/MainThread.cpp` (line 403)

**Change:**
- For each `Sleep()` polling loop, determine the event that would wake the waiter.
- Introduce or reuse an existing Windows event object (`HANDLE` event / `TEvent`) modeled after `TSignalThread` (Queue.cpp:655-676) and replace `Sleep(N)` with `WaitForSingleObject(event, N)` or `WaitForMultipleObjects`.
- **Exclude** `src/core/FileOperationProgress.cpp` line 485: `SleepEx(100, true)` is an intentional alertable wait for APC completion; converting to an event wait would break APC delivery. Document this exception in comments.
- **Exclude** `src/core/Queue.cpp`: `TSignalThread::WaitForEvent` already uses event objects with timeout.
- Preserve existing timeout behavior so that no functional timing changes are introduced.

**Logging:**
- `LOG_DEBUG` on event creation, wait entry, and wake source (event vs timeout).

---

### [x] Task 10: Harden `FileOperationProgress` callback reentrancy

**File(s):** `src/core/FileOperationProgress.cpp`, `src/core/FileOperationProgress.h`

**Change:**
- `DoProgress()` must hold `FSection` (or a dedicated callback section) while reading callback state, then release the lock before invoking `FOnProgress` to avoid deadlock.
- If `FOnProgress` can recurse into `FileOperationProgress` methods, document the reentrancy boundary and ensure no lock is held across the call.
- Add an `FInCallback` guard to detect and warn on unexpected reentrant progress calls.

**Logging:**
- `LOG_DEBUG` on `DoProgress` entry/exit and callback invocation.
- `LOG_WARN` if reentrant callback detected.

---

### [x] Task 11: Evaluate and fix OpenSSL background thread initialization

**File(s):** `src/core/Cryptography.cpp` (lines 658-667)

**Change:**
- `OPENSSL_init_crypto` is not thread-safe after `RAND_poll` on a different thread.
- Add a thread-safe lazy-init wrapper that performs the init on the first calling thread (preferably main thread) and stores a `std::once_flag` / `InitOnceExecuteOnce` so background threads block until init is complete rather than failing.
- If upstream limitation prevents a true fix, add an explicit `DebugAssert` that the init happens on the main thread, plus a comment documenting the constraint.

**Logging:**
- `LOG_INFO` on OpenSSL init path and thread ID.
- `LOG_WARN` if a background thread detects uninitialized crypto and must wait.

---

## Phase IV. Verification & Documentation

### [x] Task 12: Build verification with zero warnings (completed on Windows/MSVC)

**File(s):** All modified files, `CMakeLists.txt` (if new event handles require includes)

**Change:**
- Build `RelWithDebugInfo` for x64 using `build-x64.bat`.
- Build x86 using `build-x86.bat`.
- Verify MSVC W4 produces zero warnings.
- If `OPT_USE_UNITY_BUILD` causes symbol redefinition, disable it (`-DOPT_USE_UNITY_BUILD=OFF`) for the verification build.

**Logging:**
- `LOG_INFO` build artifacts and warning counts.

---

### [x] Task 13: Update memory-bank with threading findings and decisions

**File(s):** `./memory-bank/activeContext.md`, `./memory-bank/progress.md`, `./memory-bank/systemPatterns.md`

**Change:**
- Record every defect found, the file/line, the fix applied, and the concurrency pattern now in use.
- Add a `## Threading Conventions` section to `systemPatterns.md` covering:
  - Lock acquisition order
  - Main-thread-only Far API rule
  - Event-driven wait preference over `Sleep` polling
  - RAII guard usage (`TGuard` / `TUnguard`)
  - Thread-local logging context usage

**Logging:**
- N/A (documentation task).

---

### [x] Task 14: Add threading rules to project conventions

**File(s):** `.ai-factory/rules/threading.md`, `.ai-factory/ARCHITECTURE.md`

**Change:**
- Create `.ai-factory/rules/threading.md` with enforceable rules:
  1. All Far Manager API calls from main thread only.
  2. Worker threads signal events; main thread performs UI work.
  3. No `Sleep` polling for synchronization — use events, semaphores, or condition variables.
  4. Nested locks follow documented hierarchy; cycles are forbidden.
  5. Static/global mutable state requires explicit `TCriticalSection` or `thread_local`.
  6. Progress callbacks release all locks before invocation.
- Append a reference to this rule file in `ARCHITECTURE.md` under `## Key Principles`.

**Logging:**
- N/A (documentation task).

---

## Changelog

| Item | Task | Note |
|------|------|------|
| `m_SpeedLimitEvent` introduced | Task 8 | Replaces `Sleep(100)` in `FtpControlSocket` |
| `m_hStartedEvent` introduced | Task 9 | Replaces `Sleep(10)` poll in `MainThread.cpp` |
| `FClientsZeroEvent` introduced | Task 9 | Replaces `Sleep(200)` in `TParallelOperation::WaitFor()` |
| `FDirectoryCreatedEvent` introduced | Task 9 | Replaces `Sleep(100)` in `CopyParallel()` |
| `PostMainThreadSynchro()` created | Task 5 | Sanctioned wrapper for `ACTL_SYNCHRO` |
| `FInCallback` guard added | Task 10 | Suppresses reentrant `FOnProgress` calls |
| `std::call_once` in `InitOpenssl()` | Task 11 | Thread-safe lazy initialization |
| S3 `TGuard` dereference fixed | Task 1.5 | Real bug discovered during audit |


## Commit Plan

| Checkpoint | Tasks | Message |
|------------|-------|---------|
| After Phase I | 1-3, 1.5, 3.5 | `refactor(thread-safety): audit and harden static global state` |
| After Phase II | 4-7 | `fix(threading): marshal all Far Manager API calls to main thread` |
| After Phase III | 8-11 | `fix(threading): eliminate race conditions and busy-waiting in protocol code` |
| After Phase IV | 12-14 | `docs(threading): document threading model and verification` |

## Verification

- [x] Tasks 1-11, 1.5, 3.5, 13-14 implemented and committed.
- [x] Task 12: Build verification completed on Windows/MSVC x64 RelWithDebugInfo.
- [x] No `Sleep`-based polling remains for thread synchronization, except `SleepEx(100, true)` in `FileOperationProgress.cpp` which is an intentional alertable wait for APC completion (documented exception). Short timeouts on event waits are acceptable.
- [x] No Far Manager API calls from worker threads remain (verified by code review of all `CreateThread` / `_beginthreadex` / `std::thread` entry points).
- [x] Lock ordering documented and cycle-free.
- [x] Memory-bank updated with findings.
- [x] Threading rules committed to `.ai-factory/rules/threading.md`.

## Explorations Results

- [Multithreading Review and Fix Results](../references/multithreading-review-fix-results.md) documents every fix applied, files changed, and real bugs discovered (including the S3 `TGuard` dereference bug and `MainThread.cpp`/`Terminal.cpp` structural fixes).
- New Windows events introduced: `m_SpeedLimitEvent`, `m_hStartedEvent`, `FClientsZeroEvent`, `FDirectoryCreatedEvent`.
- New `TCustomFarPlugin::PostMainThreadSynchro()` abstraction created for sanctioned `ACTL_SYNCHRO` usage.
- `FInCallback` reentrancy guard added to `FileOperationProgress::DoProgress()`.
- `std::call_once` wrapper added to `InitOpenssl()` in `Cryptography.cpp`.
