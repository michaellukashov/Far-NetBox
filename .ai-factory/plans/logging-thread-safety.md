# NetBox Logging & Thread-Safety Improvement Plan

**Branch:** N/A (no branch created)  
**Created:** 2026-04-23  
**Mode:** Full  

## Settings

- **Testing:** Yes — unit tests for tinylog, integration tests for NetBox
- **Logging:** Verbose — detailed logging during implementation
- **Docs:** Yes — mandatory documentation checkpoint after Phase 2

## Roadmap Linkage

**Milestone:** none  
**Rationale:** Skipped — internal infrastructure improvement, not user-facing feature

## Overview

This plan fixes critical thread-safety issues in the tinylog logging library and improves NetBox's logging patterns to enable better debugging of multi-threaded operations.

**Problems addressed:**
1. Race conditions and data corruption in `libs/tinylog/` buffer management
2. Missing structured logging context in NetBox (session ID, operation type, file paths)
3. Insufficient logging coverage in critical threading code paths

**Approach:**
- Phase 1: Fix tinylog thread-safety with atomic operations and TLS buffering
- Phase 2: Add structured logging helpers and instrument NetBox threading code
- Phase 3: Comprehensive testing and verification

## Tasks

### Phase 1: Fix tinylog Thread-Safety

#### Task 1.1: Add Thread-Local Staging Buffers
**Status:** [x]
**File:** `libs/tinylog/tinylog/LogStream.h`  
**File:** `libs/tinylog/src/LogStream.cpp`

Add `thread_local` staging buffers to reduce mutex contention.

**Changes:**
1. In `LogStream.h` after line 58 (after `bool & drain_buffer_;`), add to private section:
   ```cpp
   static thread_local std::array<char, 4096> tls_buffer_;
   static thread_local size_t tls_buffer_used_;
   ```

2. In `LogStream.cpp` after line 12 (after namespace open), add:
   ```cpp
   thread_local std::array<char, 4096> LogStream::tls_buffer_{};
   thread_local size_t LogStream::tls_buffer_used_{0};
   ```

3. Modify `FormattedWrite` to write to TLS buffer first, flush to shared buffer only when full

**Acceptance Criteria:**
- Code compiles with zero warnings
- `nm build-RelWithDebugInfo/libs/tinylog/libtinylog.a | grep tls_buffer` shows symbols
- Unit test `test_tls_buffer_isolation` passes (verifies each thread has independent buffer)

**Logging:**
- DEBUG: "TLS buffer allocated for thread %d, size %zu"
- DEBUG: "TLS buffer flush to shared buffer, used %zu bytes"

**Blocked by:** None

---

#### Task 1.2: Make drain_buffer_ Atomic
**Status:** [x]
**File:** `libs/tinylog/tinylog/LogStream.h`  
**File:** `libs/tinylog/src/LogStream.cpp`  
**File:** `libs/tinylog/src/TinyLog.cpp`

Fix buffer swap race by making `drain_buffer_` atomic with proper memory ordering.

**Changes:**
1. In `LogStream.h` line 57, replace:
   ```cpp
   bool & drain_buffer_;
   ```
   with:
   ```cpp
   std::atomic<bool> & drain_buffer_;
   ```

2. In `LogStream.cpp` line 179, replace:
   ```cpp
   drain_buffer_ = true;
   ```
   with:
   ```cpp
   drain_buffer_.store(true, std::memory_order_release);
   ```

3. In `TinyLog.cpp` line 135, replace:
   ```cpp
   while (run_ && !drain_buffer_)
   ```
   with:
   ```cpp
   while (run_ && !drain_buffer_.load(std::memory_order_acquire))
   ```

4. Ensure all swap operations happen ONLY in `TinyLogImpl::MainLoop` (background thread)

**Acceptance Criteria:**
- Producer threads never call `SwapBuffer()` directly
- ThreadSanitizer reports zero data races on `drain_buffer_`
- Multi-threaded stress test (10 threads × 10K entries) shows no corruption

**Logging:**
- DEBUG: "drain_buffer_ signaled by thread %d"
- DEBUG: "Background thread acquired drain signal, swapping buffers"

**Blocked by:** Task 1.1

---

#### Task 1.3: Replace Timestamp with Atomic
**Status:** [x]
**File:** `libs/tinylog/tinylog/LogStream.h`  
**File:** `libs/tinylog/src/LogStream.cpp`

Replace `tv_base_` and `tm_base_` with atomic timestamp to prevent races.

**Changes:**
1. In `LogStream.h` lines 53-54, replace:
   ```cpp
   timeval tv_base_{};
   struct tm tm_base_{};
   ```
   with:
   ```cpp
   std::atomic<uint64_t> timestamp_us_{0};
   ```

2. In `LogStream.cpp`, replace entire `UpdateBaseTime()` function:
   ```cpp
   void LogStream::UpdateBaseTime() {
     struct timeval tv;
     gettimeofday(&tv, nullptr);
     uint64_t us = tv.tv_sec * 1000000ULL + tv.tv_usec;
     timestamp_us_.store(us, std::memory_order_relaxed);
   }
   ```

3. Update `FormattedWrite` to read timestamp with:
   ```cpp
   uint64_t ts = timestamp_us_.load(std::memory_order_relaxed);
   ```

4. Convert `ts` to `tm` structure for formatting

**Acceptance Criteria:**
- All timestamp reads use atomic load
- Multi-threaded test shows monotonic timestamps per-thread
- ThreadSanitizer reports zero races on timestamp

**Logging:**
- DEBUG: "Timestamp updated: %llu microseconds"

**Blocked by:** Task 1.2

---

#### Task 1.4: Measure Mutex Hold Time
**Status:** [x]
**File:** `libs/tinylog/src/LogStream.cpp`

Measure current `fwrite` time under lock to determine if buffer copy optimization is needed.

**Changes:**
1. In `WriteBuffer()` function, add timing around `Flush`:
   ```cpp
   auto start = std::chrono::steady_clock::now();
   front_buff_->Flush(file_);
   auto elapsed = std::chrono::steady_clock::now() - start;
   auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
   if (ms > 10) {
     // Log warning: mutex held too long
   }
   ```

2. Run stress test and collect measurements

3. **Decision point:** If average hold time <10ms, SKIP buffer copy optimization (not needed). If >10ms, create follow-up task for copy-then-write.

**Acceptance Criteria:**
- Measurement code compiles and runs
- Stress test produces timing data
- Decision documented: skip or proceed with optimization

**Logging:**
- INFO: "Mutex hold time during flush: %lld ms (threshold: 10ms)"
- WARN: "Mutex held for %lld ms, exceeds threshold" (if >10ms)

**Blocked by:** Task 1.3

---

#### Task 1.5: Add Buffer Overflow Handling
**Status:** [x]
**File:** `libs/tinylog/tinylog/Buffer.h`  
**File:** `libs/tinylog/src/Buffer.cpp`

Add ring buffer semantics to drop oldest entries when buffer is full.

**Changes:**
1. In `Buffer.h`, add to Buffer class:
   ```cpp
   std::atomic<size_t> read_pos_{0};
   std::atomic<size_t> write_pos_{0};
   std::atomic<size_t> dropped_count_{0};
   ```

2. In `Buffer.cpp`, modify `TryAppend`:
   - When buffer is full, advance `read_pos_` to next newline (scan for `\n`)
   - Increment `dropped_count_`
   - Log warning with dropped count

3. Add `GetDroppedCount()` method for testing

**Acceptance Criteria:**
- Stress test with slow disk (simulated delay) shows `dropped_count_ > 0`
- Log file contains "WARNING: Log buffer overflow, dropped N entries"
- Log file contains all non-dropped entries with correct format (timestamp + prefix)
- No crashes or corruption when buffer overflows

**Logging:**
- WARN: "Log buffer overflow, dropped %zu entries"
- DEBUG: "Buffer full, advancing read_pos from %zu to %zu"

**Blocked by:** Task 1.4

---

#### Task 1.6: Create tinylog Unit Tests
**Status:** [x]
**File:** `libs/tinylog/tests/test_tinylog.cpp` (new)  
**File:** `libs/tinylog/CMakeLists.txt`

Create comprehensive unit tests for tinylog thread-safety.

**Tests to implement:**
1. `test_tls_buffer_isolation` — verify each thread has independent TLS buffer
2. `test_atomic_drain_signal` — verify no races on `drain_buffer_`
3. `test_timestamp_monotonic` — verify timestamps increase monotonically per-thread
4. `test_buffer_overflow` — verify graceful degradation when buffer full
5. `test_stress_multithread` — 10 threads × 10K entries, verify all present

**Acceptance Criteria:**
- All tests pass with exit code 0
- ThreadSanitizer reports zero data races
- Test output format: `PASS: test_name` or `FAIL: test_name: <reason>`
- Test log file `./test.log` exists and is non-empty after tests complete
- Stress test verifies:
  - All 100K entries present (sequence numbers)
  - No partial log lines (every entry ends with `\n`)
  - Test completes in <10 seconds (no deadlock)

**Logging:**
- INFO: "Running test: %s"
- INFO: "Test %s: PASS"
- ERROR: "Test %s: FAIL - %s"

**Blocked by:** Task 1.5

---

### Phase 1 Verification Gate

**STOP:** Before proceeding to Phase 2, run:
```cmd
cmd /c build-x64.bat
cd build-RelWithDebugInfo
ctest -R tinylog
```

**Gate passes when:**
- All tinylog tests exit with code 0
- No ThreadSanitizer warnings
- No compiler warnings
- Log file `test.log` exists in the current test working directory and contains expected log entries

**If gate fails:** Debug and fix before proceeding. Do NOT start Phase 2 with broken tinylog.

**Backup:** After gate passes, create backup:
```cmd
xcopy /E /I /Y libs\tinylog libs\tinylog.backup
```

---

### Phase 2: Improve NetBox Logging Patterns

#### Task 2.1: Create TLogContext Helper
**Status:** [x]
**File:** `src/base/LogContext.h` (new)  
**File:** `src/base/LogContext.cpp` (new)

Create RAII helper for structured logging context.

**Implementation:**
```cpp
// LogContext.h
class NB_CORE_EXPORT TLogContext {
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TLogContext)
public:
  TLogContext(const char* key, const UnicodeString& value);
  ~TLogContext() noexcept;
  
  static UnicodeString Format(); // Returns "[key1=val1][key2=val2]"
  
private:
  static thread_local std::vector<std::pair<const char*, UnicodeString>> context_;
  const char* key_;
};
```

**Acceptance Criteria:**
- Code compiles with zero warnings
- Unit test verifies context is thread-local (different threads see different contexts)
- Unit test verifies RAII cleanup (context removed on scope exit)

**Logging:**
- DEBUG: "LogContext pushed: [%s=%s]"
- DEBUG: "LogContext popped: [%s]"

**Blocked by:** Phase 1 Verification Gate

---

#### Task 2.2: Add Structured Logging Macros
**Status:** [x]
**File:** `src/base/Global.h`

Add convenience macros for structured logging.

**Macros to add:**
```cpp
#define LOG_THREAD_START(name) \
  TINYLOG_INFO(g_tinylog) << TLogContext::Format() << " Thread started: " << (name)

#define LOG_THREAD_STOP(name) \
  TINYLOG_INFO(g_tinylog) << TLogContext::Format() << " Thread stopped: " << (name)

#define LOG_OPERATION(op, path) \
  TLogContext __ctx_op("op", op); TLogContext __ctx_path("path", path);

#define LOG_QUEUE_EVENT(event) \
  TINYLOG_DEBUG(g_tinylog) << TLogContext::Format() << " Queue event: " << (event)
```

**Acceptance Criteria:**
- Macros compile without warnings
- Example usage compiles and produces expected log output

**Logging:**
- N/A (this task defines logging macros)

**Blocked by:** Task 2.1

---

#### Task 2.3: Instrument TSimpleThread
**Status:** [x]
**File:** `src/core/Queue.cpp`

Add logging to `TSimpleThread::ThreadProc` and `TSimpleThread::Execute`.

**Changes:**
1. In `ThreadProc` (line 305), add at start:
   ```cpp
   TLogContext ctx_thread("thread_id", IntToStr(GetCurrentThreadId()));
   LOG_THREAD_START(SimpleThread->ClassName());
   ```

2. Add at end (before return):
   ```cpp
   LOG_THREAD_STOP(SimpleThread->ClassName());
   ```

3. In `Execute` override of each thread class, add context:
   ```cpp
   TLogContext ctx_class("class", ClassName());
   ```

**Acceptance Criteria:**
- Logs show thread start/stop with thread ID
- Logs show class name for each thread type
- No performance degradation (logging overhead <1%)

**Logging:**
- INFO: "[thread_id=1234] Thread started: TPluginIdleThread"
- INFO: "[thread_id=1234] Thread stopped: TPluginIdleThread"

**Blocked by:** Task 2.2

---

#### Task 2.4: Instrument TTerminalQueue
**Status:** [x]
**File:** `src/core/Queue.cpp`

Add logging to `TTerminalQueue::ProcessEvent` and queue item processing.

**Changes:**
1. In `ProcessEvent`, add at start:
   ```cpp
   TLogContext ctx_queue("queue", "terminal");
   TINYLOG_DEBUG(g_tinylog) << TLogContext::Format() << " ProcessEvent called";
   ```

2. When processing queue item, add:
   ```cpp
   TLogContext ctx_item("item", Item->GetInfo()->Source);
   LOG_QUEUE_EVENT("processing");
   ```

3. On completion/error, log outcome:
   ```cpp
   LOG_QUEUE_EVENT(Success ? "completed" : "failed");
   ```

**Acceptance Criteria:**
- Logs show queue processing with item source path
- Logs show success/failure outcome
- Context includes queue name and item path

**Logging:**
- DEBUG: "[queue=terminal] ProcessEvent called"
- DEBUG: "[queue=terminal][item=/path/to/file] Queue event: processing"
- DEBUG: "[queue=terminal][item=/path/to/file] Queue event: completed"

**Blocked by:** Task 2.3

---

#### Task 2.5: Instrument TQueueItem
**Status:** [x]
**File:** `src/core/Queue.cpp`

Add logging to `TQueueItem::Execute` and status changes.

**Changes:**
1. In `Execute`, add at start:
   ```cpp
   TLogContext ctx_op("op", GetInfo()->Operation == foNone ? "unknown" : OperationName(GetInfo()->Operation));
   TLogContext ctx_src("src", GetInfo()->Source);
   TLogContext ctx_dst("dst", GetInfo()->Destination);
   TINYLOG_INFO(g_tinylog) << TLogContext::Format() << " QueueItem execute start";
   ```

2. In `SetStatus`, log status changes:
   ```cpp
   TINYLOG_DEBUG(g_tinylog) << TLogContext::Format() << " Status: " << StatusName(FStatus) << " -> " << StatusName(Status);
   ```

3. On completion, log result:
   ```cpp
   TINYLOG_INFO(g_tinylog) << TLogContext::Format() << " QueueItem execute complete";
   ```

**Acceptance Criteria:**
- Logs show operation type, source, destination
- Logs show status transitions
- Context persists across entire operation

**Logging:**
- INFO: "[op=copy][src=/local/file][dst=/remote/file] QueueItem execute start"
- DEBUG: "[op=copy][src=/local/file][dst=/remote/file] Status: qsPending -> qsProcessing"
- INFO: "[op=copy][src=/local/file][dst=/remote/file] QueueItem execute complete"

**Blocked by:** Task 2.4

---

#### Task 2.6: Add Lock Contention Logging (Debug Only)
**Status:** [x]
**File:** `src/base/System.SyncObjs.cpp`

Add optional lock contention logging to `TCriticalSection` (debug builds only).

**Changes:**
1. In `Enter()`, add timing (debug only):
   ```cpp
   #ifdef DODEBUGGING
   auto start = std::chrono::steady_clock::now();
   #endif
   ::EnterCriticalSection(&FSection);
   #ifdef DODEBUGGING
   auto elapsed = std::chrono::steady_clock::now() - start;
   auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
   if (ms > 100) {
     TINYLOG_WARN(g_tinylog) << "Lock contention: waited " << ms << "ms";
   }
   #endif
   ```

**Acceptance Criteria:**
- Debug builds log warnings when lock wait >100ms
- Release builds have zero overhead (no timing code)
- No false positives (only real contention logged)

**Logging:**
- WARN: "Lock contention: waited 150ms" (debug only)

**Blocked by:** Task 2.5

---

### Phase 3: Testing & Verification

#### Task 3.1: Integration Test with Far Manager
**Status:** [ ]  
**File:** `tests/integration/test_logging.md` (new, manual test plan)

Create manual test plan for integration testing with Far Manager.

**Test scenarios:**
1. Launch Far Manager, load NetBox plugin
2. Connect to SFTP server
3. Perform file operations (copy, move, delete)
4. Verify logs contain structured context
5. Check for lock contention warnings (debug build)

**Acceptance Criteria:**
- All test scenarios documented
- Log files exist at expected paths:
  - `%TEMP%\netbox-dbglog.txt` (global debug log)
  - `%TEMP%\<sessionname>.log` (session protocol log)
  - `%TEMP%\<sessionname>.xml` (action log)
- Logs contain structured context
- No crashes or deadlocks during testing
- Lock contention <5% of operations (debug build)

**Logging:**
- N/A (this task verifies logging output)

**Blocked by:** Task 2.6

---

#### Task 3.2: Performance Benchmark
**Status:** [ ]  
**File:** `tests/benchmark/bench_logging.cpp` (new)

Create benchmark to measure logging overhead.

**Benchmarks:**
1. Baseline: no logging
2. With TLS buffering: measure overhead
3. With structured context: measure overhead
4. Multi-threaded: 10 threads, measure contention

**Acceptance Criteria:**
- Logging overhead <1% CPU in normal operation
- Mutex contention <5% of time blocked
- Benchmark results documented

**Logging:**
- INFO: "Benchmark: %s - overhead: %.2f%%"

**Blocked by:** Task 3.1

---

#### Task 3.3: Final Verification Checklist
**Status:** [ ]  
**File:** `.ai-factory/plans/logging-thread-safety.md` (this file)

Run final verification checklist and document results.

**Checklist:**
- [ ] tinylog multi-threaded stress test passes (10 threads × 10K entries, zero loss)
- [ ] No data corruption in logs (verify with checksum or sequence numbers)
- [ ] Log file created and contains entries — `test.log` in the test working directory
- [ ] Log files created at all 3 expected NetBox paths: `%TEMP%\netbox-dbglog.txt`, `%TEMP%\&S.log`, `%TEMP%\&S.xml`
- [ ] Logging overhead <1% CPU (measure with profiler)
- [ ] Lock contention <5% (measure with debug logging)
- [ ] NetBox builds cleanly with zero warnings
- [ ] Far Manager plugin loads and operates normally
- [ ] Logs contain structured context (session ID, operation, paths)
- [ ] No crashes or deadlocks under load

**Acceptance Criteria:**
- All checklist items pass
- Results documented in this plan file

**Logging:**
- INFO: "Verification checklist: %d/%d passed"

**Blocked by:** Task 3.2

---

## Commit Plan

**Commit 1:** Phase 1 Changes (Tasks 1.1-1.6)
```
fix(tinylog): add thread-safety with atomic operations and TLS buffering

- Add thread-local staging buffers to reduce mutex contention
- Make drain_buffer_ atomic with proper memory ordering
- Replace timestamp with atomic to prevent races
- Add buffer overflow handling with ring buffer semantics
- Create comprehensive unit tests for thread-safety

Fixes race conditions and data corruption in tinylog library.
```

**Commit 2:** Phase 2 Changes (Tasks 2.1-2.6)
```
feat(logging): add structured logging context to NetBox

- Create TLogContext RAII helper for structured logging
- Add convenience macros for common logging patterns
- Instrument TSimpleThread, TTerminalQueue, TQueueItem
- Add lock contention logging (debug builds only)

Improves debuggability of multi-threaded operations.
```

**Commit 3:** Phase 3 Changes (Tasks 3.1-3.3)
```
test(logging): add integration tests and benchmarks

- Create integration test plan for Far Manager
- Add performance benchmarks for logging overhead
- Document verification checklist results

Verifies logging improvements meet performance and correctness goals.
```

---

## Rollback Plan

If Phase 1 causes crashes or test failures:

1. **Immediate rollback:**
   ```cmd
   git revert <commit-sha>
   ```

2. **Restore backup:**
   ```cmd
   rmdir /S /Q libs\tinylog
   xcopy /E /I /Y libs\tinylog.backup libs\tinylog
   ```

3. **Verify:**
   ```cmd
   cmd /c build-x64.bat
   Far3_x64\Far.exe
   ```
   Plugin must load without crash.

4. **Root cause:** Before retry, identify which change caused failure

5. **Incremental retry:** Apply changes one at a time with verification between each

---

## Notes

- **DO NOT modify `libs/tinylog/` without reading existing code carefully** — it's third-party
- **Use `TGuard` for all `TCriticalSection` locking** — RAII prevents leaks
- **Test with Far Manager** — plugin DLL must load and operate correctly
- **Measure before optimizing** — profile to find real bottlenecks
- **Keep logging optional** — add `OPT_ENABLE_DETAILED_LOGGING` CMake flag for debug builds

## References

- `AGENTS.md` — NetBox development guide, build commands, conventions
- `AGENTS-Standards.md` — C++ coding standards, naming conventions
- `libs/tinylog/tinylog/README.md` — tinylog documentation
- `docs/logging-subsystem.md` — NetBox logging architecture reference (tinylog internals, log file paths, configuration)
- Windows `CRITICAL_SECTION` docs — https://learn.microsoft.com/en-us/windows/win32/sync/critical-section-objects
