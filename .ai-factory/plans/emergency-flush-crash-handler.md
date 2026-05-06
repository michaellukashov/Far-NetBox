# Implementation Plan: Emergency Log Flush on Crash

Branch: feature/emergency-flush-crash-handler
Created: 2026-05-06

## Settings
- Testing: no (crash handler testing requires manual fault injection; unit tests not applicable)
- Logging: verbose (all flush attempts, filter installation/removal, timeouts logged via AppLogFmt)
- Docs: yes — update `.ai-factory/references/logging-subsystem.md` with crash-resilience section

## Research Context
Source: `.ai-factory/RESEARCH.md` (Active Summary)

Goal: Reduce log data loss when NetBox crashes during protocol operations by flushing async tinylog buffers from an unhandled exception filter.

Constraints:
- No modifications to `libs/` directory (use patches only)
- Far Manager API calls on main thread only (exception filter must NOT call Far API)
- MSVC /W4 zero warnings
- WinXP compatibility (`GetTickCount`, not `GetTickCount64`)
- Incremental evolution — no architectural rewrites
- C++17 standard only

Decisions:
- Flush via `SetUnhandledExceptionFilter` installed in `TWinSCPPlugin` constructor
- Use `pthread_mutex_trylock` (non-blocking) to avoid deadlock if crashing thread holds the mutex
- Chain to previous filter after flush (preserve existing handlers)
- TLS buffer of the crashing thread is lost if mutex cannot be acquired; acceptable tradeoff
- `TApplicationLog` (sync `fwrite`) requires no emergency flush — already safe

Open questions:
- Tinylog worker thread crash: `pthread_join` with timeout; if worker is the crashed thread, join will never return. Plan: `WaitForSingleObject` with 100ms timeout, then abandon.
- Multiple `TinyLog` instances: need instance registry. Plan: add to `TinyLog` class itself.

## Architecture

```
Crash Flow:
  SEH Unhandled Exception
         |
         v
  NetBoxExceptionFilter()
         |
         +-- Flush g_tinylog (DEBUG builds)
         +-- Flush all TSessionLog TinyLog instances
         +-- Flush all TActionLog TinyLog instances
         +-- Chain to previous filter (or terminate)
```

Tinylog modifications (patch-based):
```
TinyLog::EmergencyFlush():
  1. Trylock mutex (non-blocking)
     └─ fail → return false (skip shared buffers)
  2. WriteBuffer() on front buffer
  3. If drain_buffer_ pending → SwapBuffer() + WriteBuffer()
  4. Clear drain_buffer_
  5. Signal worker (run_=false, cond_broadcast)
  6. Unlock mutex
  7. WaitForSingleObject(thrd_, 100ms)
  8. Return true
```

TinyLog instance registry:
```
TinyLogRegistry:
  - Register(TinyLog*)   [called from TinyLog ctor]
  - Unregister(TinyLog*) [called from TinyLog dtor]
  - EmergencyFlushAll()    [called from exception filter]
```

## Commit Plan

- **Commit 1** (after tasks 0-3): `feat(tinylog): add EmergencyFlush and instance registry`
- **Commit 2** (after tasks 4-5): `feat(plugin): install unhandled exception filter for log flush on crash`
- **Commit 3** (after task 6): `docs(logging): document crash-resilient logging architecture`

## Tasks

### Phase 1: Tinylog Core Changes (libs/tinylog/)

- [x] **Task 0: Make `run_` std::atomic<bool>**
- [x] **Task 1: Add TinyLog instance registry**
- [x] **Task 2: Implement LogStream::EmergencyFlush()**
- [x] **Task 3: Implement TinyLog::EmergencyFlush() and registry EmergencyFlushAll()**
  - Add namespace-level registry in `TinyLog.cpp` (not a new class/header — keep it simple)
  - `static std::mutex g_registry_mutex; static std::vector<TinyLog *> g_registry;`
  - Methods: `Register(TinyLog*)`, `Unregister(TinyLog*)`, `EmergencyFlushAll(uint32_t TimeoutMs)`
  - Call `Register(this)` from `TinyLog` constructor, `Unregister(this)` from destructor
  - `EmergencyFlushAll`: lock `g_registry_mutex`, iterate, call `EmergencyFlush(uint32_t TimeoutMs)` per instance
  - Timeout per instance: fixed 50ms, max total 300ms, early-exit on failure
  - Never throw from `EmergencyFlushAll` — wrap iteration body in `try { ... } catch (...) { OutputDebugStringA(...); }`
  - LOGGING: `OutputDebugStringA` for registration count and flush results
  - Files: `libs/tinylog/tinylog/TinyLog.h`, `libs/tinylog/src/TinyLog.cpp`
  - Depends on: Task 0

- [ ] **Task 2: Implement LogStream::EmergencyFlush()**
  - Add `bool EmergencyFlush()` to `LogStream` class
  - Use `pthread_mutex_trylock` on `mutex_` (return false immediately on busy)
  - If acquired:
    - `WriteBuffer()` on current front buffer
    - If `drain_buffer_` is set: `SwapBuffer()` then `WriteBuffer()`
    - Clear `drain_buffer_` flag
  - Unlock mutex
  - Do NOT attempt to join worker thread from here (leave that to `TinyLog` level)
  - LOGGING: `OutputDebugStringA` on success/failure and bytes flushed
  - Files: `libs/tinylog/tinylog/LogStream.h`, `libs/tinylog/src/LogStream.cpp`

- [ ] **Task 3: Implement TinyLog::EmergencyFlush() and registry EmergencyFlushAll()**
  - `TinyLog::EmergencyFlush(uint32_t TimeoutMs)`:
    - Call `logstream_->EmergencyFlush()`
    - If successful, signal worker thread exit (`run_.store(false, std::memory_order_release)`, `cond_broadcast`)
    - Unlock mutex (released by LogStream::EmergencyFlush)
    - `WaitForSingleObject((HANDLE)thrd_, TimeoutMs)` — cast `pthread_t` to `HANDLE`
      (valid because `platform_win32.h` defines `typedef HANDLE pthread_t`)
    - If timeout: log warning via `OutputDebugStringA`, do NOT block process termination
    - Return true if data reached disk, false otherwise
  - `EmergencyFlushAll(TimeoutMs)`:
    - Iterate registered instances under `g_registry_mutex`
    - Call `EmergencyFlush(50)` on each (fixed 50ms per instance)
    - Stop iterating after total timeout exceeds 300ms
    - Wrap entire body in `try { ... } catch (...) { OutputDebugStringA(...); }` — never throw from crash handler context
  - Create patch file: `.ai-factory/patches/tinylog-emergency-flush.patch`
  - LOGGING: `OutputDebugStringA` for each flushed instance and total
  - Files: `libs/tinylog/src/TinyLog.cpp`, `.ai-factory/patches/tinylog-emergency-flush.patch`
  - Depends on: Task 1, Task 2

### Phase 2: Plugin Crash Handler (src/NetBox/)

- [x] **Task 4: Add exception filter to TWinSCPPlugin**
- [x] **Task 5: Ensure TApplicationLog is also flushed**

### Phase 3: Verification & Documentation

- [x] **Task 6: Build verification and docs update**

## Risk Notes

1. **Data race on `run_` (FIXED by Task 0)**: `TinyLogImpl::run_` was a plain `bool` written by the crash handler and read by the worker thread. Now `std::atomic<bool>` with `memory_order_release/acquire`.

2. **Deadlock in shared buffer flush**: `pthread_mutex_trylock` mitigates this. If the crashing thread holds the mutex, shared buffers (Layer 2) are skipped. TLS buffer of crashing thread (Layer 1) is also skipped — it requires the mutex via `InternalWrite`.

3. **Deadlock in TApplicationLog flush**: `TCriticalSection::TryEnter()` mitigates this. If the crashing thread holds `FCriticalSection`, we skip the `fflush`. The `fwrite` already completed, so only the CRT stream buffer (typically 4KB) is lost.

4. **Crash handler recursion**: `AppLogFmt` (heap alloc + critical section + `fwrite`) is banned from the filter. Only `OutputDebugStringA` with a stack buffer is used. The entire filter body is wrapped in `__try/__except` — if the flush itself crashes, we silently chain to the previous filter.

5. **Worker thread crash**: If the tinylog worker thread itself crashed, `WaitForSingleObject` on `thrd_` will timeout after 50ms. The process terminates with unflushed front buffer. This is unrecoverable.

6. **Far Manager filter chaining**: Far may install its own exception filter. We chain via `SetUnhandledExceptionFilter` return value, never replace. The previous filter always runs after ours.

7. **libs/ modification**: Tinylog is a bundled library with no upstream. The patch file is created per project rules, but the build system does not auto-apply patches. The patch must be applied manually (`git apply`) or integrated into the CMake build. Documented in Task 6.

8. **Multiple TinyLog instances**: Each `TSessionLog` and `TActionLog` creates its own `TinyLog`. The registry tracks all instances. In a crash, `EmergencyFlushAll(200)` distributes 50ms per instance, max 300ms total. If a user has 10+ concurrent session logs, later instances may be skipped.