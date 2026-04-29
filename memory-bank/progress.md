# Progress

## Multithreading Review and Fix — Completed 2026-04-29

### Phase I. Foundation — Audit & Static State Hardening

- Task 1: Audited static/global mutable state across `base/` and `core/`.
  - Added thread-safety inventory comments to `Classes.cpp`, `Global.cpp`, `Common.cpp`, `Exceptions.cpp`, `SessionData.cpp`, `NeonIntf.cpp`, `S3FileSystem.cpp`.
  - Fixed missing `*` dereference in `S3FileSystem.cpp` `TGuard Guard(LibS3Section.get())` → `TGuard Guard(*LibS3Section.get())`.
- Task 2: Hardened `nbstr_getNil()` race condition in `nbstring.cpp`.
  - Replaced lazy-init global with C++11 thread-safe function-local static.
  - Documented intentional non-atomic `nbstr_lock`/`nbstr_unlock` behavior.
- Task 3: Documented lock ordering hierarchy in `Queue.h`, `FileOperationProgress.h`, `FtpControlSocket.h`.

### Phase II. Critical — Far Manager Thread Safety

- Task 4: Fixed `TFarDialogIdleThread` calling `Idle()` from worker thread.
  - Now marshals to main thread via existing `TFarDialog::Synchronize()`.
- Task 5: Fixed `TPluginIdleThread` calling `FarAdvControl` directly.
  - Introduced `TCustomFarPlugin::PostMainThreadSynchro()` abstraction.
- Task 6: Hardened `TTerminalThread` UI callback marshaling.
  - Added `DebugAssert(GetCurrentThreadId() == FMainThread)` before `FUserAction->Execute`.
  - Documented marshal handshake in `RunAction` and `WaitForUserAction`.
- Task 7: Verified `TQueueDialog::Idle()` propagation.
  - Since Task 4 marshals `Idle()` to main thread, `TQueueDialog::Idle()` runs on main thread.
  - Replaced direct `FarAdvControl(ACTL_SYNCHRO)` call with `PostMainThreadSynchro()`.

### Phase III. Protocol — Race Conditions & Busy-Waiting

- Task 8: Fixed `FtpControlSocket` unlock-sleep-relock race.
  - Added static `m_SpeedLimitEvent` and signaled it from `SpeedLimitAddTransferredBytes`.
  - Replaced `Sleep(100)` with `WaitForSingleObject(m_SpeedLimitEvent, 100)`.
- Task 9: Replaced busy-waiting loops with event-driven waits.
  - `CMainThread::ResumeThread()`: added `m_hStartedEvent` to replace `Sleep(10)` poll.
  - `TParallelOperation::WaitFor()`: added `FClientsZeroEvent` to replace `Sleep(200)` poll.
  - `TTerminal::CopyParallel()`: added `FDirectoryCreatedEvent` to replace `Sleep(100)` poll.
  - Documented `SleepEx(100, true)` in `FileOperationProgress.cpp` as intentional alertable wait.
- Task 10: Hardened `FileOperationProgress` callback reentrancy.
  - Added `FInCallback` guard to `DoProgress()` to detect and suppress reentrant calls.
- Task 11: Fixed OpenSSL background thread initialization.
  - Wrapped `InitOpenssl()` in `std::call_once` to guarantee single-threaded initialization.

### Phase IV. Verification & Documentation

- Task 12: Build verification pending Windows/MSVC environment.
- Task 13: Memory-bank updated (`systemPatterns.md`).
- Task 14: Threading rules committed to `.ai-factory/rules/threading.md`.
