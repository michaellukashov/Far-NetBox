# Threading Rules

## 1. Far Manager API Calls

All Far Manager API calls must originate from the **main thread only**.

Exception: `ACTL_SYNCHRO` is the sanctioned cross-thread synchronization primitive. Use `TCustomFarPlugin::PostMainThreadSynchro()` rather than calling `FarAdvControl(ACTL_SYNCHRO)` directly.

## 2. Worker Thread UI Work

Worker threads must not perform UI work. They signal events; the main thread performs UI work via the marshal mechanisms:

- `TFarDialog::Synchronize()` for dialog idle processing.
- `TTerminalThread::WaitForUserAction()` / `RunAction()` for queue callbacks.
- `TCustomFarPlugin::PostMainThreadSynchro()` for general idle events.

## 3. No Sleep Polling for Synchronization

Use `WaitForSingleObject`, `WaitForMultipleObjects`, or condition-variable equivalents instead of `Sleep()` loops for thread synchronization.

## 4. Lock Ordering

Nested locks follow the documented hierarchy. Cycles are forbidden.

Document any cross-lock acquisition in a header comment near the critical section declarations.

## 5. Static / Global Mutable State

All mutable static or global variables must be one of:

- `const` after initialization (and initialization must be thread-safe, e.g., C++11 function-local statics).
- Protected by an explicit `TCriticalSection`.
- Converted to `thread_local` where appropriate.

Add a thread-safety inventory comment for every static `TCriticalSection` and the state it protects.

## 5.1 Thread-Safe State Inventory

The following table inventories every `TCriticalSection` instance across the codebase. Each row identifies the layer, lock name, source file, protected state, and notes.

| Layer | Lock | File | Protected State | Notes |
|-------|------|------|----------------|-------|
| Plugin | `TCustomFarPlugin::FCriticalSection` | `FarPlugin.h:235` | Plugin callback state | Entry-point guards |
| Plugin | `TFarDialog::FCriticalSection` | `FarPlugin.h:370` | Dialog internal state | Idle thread synchronization |
| Plugin | `TWinSCPFileSystem::FQueueStatusSection` | `WinSCPFileSystem.h:322` | Queue status, event pending flags | Main thread only |
| Base | `TracingCriticalSection` | `Global.cpp:73/125` | `TraceFile`, `IsTracing`, `WriteTraceBuffer` | `DoAssert` reads `IsTracing` lockless (atomic bool) |
| Base | `DateTimeParamsSection` | `Common.cpp:2463` | `YearlyDateTimeParams` | Lazy init, read-heavy |
| Base | `IgnoredExceptionsCriticalSection` | `Exceptions.cpp:17` | `IgnoredExceptions` set | Exception-type deduplication |
| Core | `CoreMainCriticalSection` | `CoreMain.cpp:24` | `AnySession`, `StoredSessionsInitialized`, `StoredSessions` | Session lifecycle |
| Core | `TConfiguration::FCriticalSection` | `Configuration.h:249` | Config values, `OnChange` dispatch | Heavily contended |
| Core | `TFileOperationProgressType::FSection` | `FileOperationProgress.h:172` | Progress data (size, speed, cancel) | Recursive; address-ordered in `Assign()` |
| Core | `TFileOperationProgressType::FUserSelectionsSection` | `FileOperationProgress.h:173` | User selection state | Secondary lock |
| Core | `TTerminalQueue::FItemsSection` | `Queue.h:155` | Queue item list | Parent of `TQueueItem::FSection` |
| Core | `TQueueItem::FSection` | `Queue.h:248` | Individual queue item state | Child of `FItemsSection` |
| Core | `TTerminalItem::FCriticalSection` | `Queue.cpp:337` | Terminal item state | Queue worker thread state |
| Core | `TTerminalThread::FSection` | `Queue.h:596` | Terminal thread state | Marshaling handshake |
| Core | `TParallelOperation::FSection` | `Terminal.h:1205` | Parallel transfer client count | Never held with `FItemsSection` |
| Core | `TFTPFileSystem::FQueueCriticalSection` | `FtpFileSystem.h:239` | FTP message queue | Signals via `FQueueEvent` |
| Core | `TFTPFileSystem::FTransferStatusCriticalSection` | `FtpFileSystem.h:240` | FTP transfer progress | Protocol-layer progress |
| Core | `TRemoteDirectoryCache::FSection` | `RemoteFiles.h:420` | Cached directory entries | Navigation invalidation |
| Core | `LibS3Section` | `S3FileSystem.cpp:80` | libs3 init/deinit refcount | Global singleton pattern |
| Core | `PasswordFilesCacheSection` | `SessionData.cpp:2056` | Password file cache map | Per-file cache |
| Core | `DebugSection` | `NeonIntf.cpp:426` | `NeonTerminals` set | neon callback registration |
| Core | `PuttyStorageSection` | `PuttyIntf.cpp:550` | `PuttyStorage` redirect | PuTTY config marshaling |
| Core | `TSessionLog::FCriticalSection` | `SessionInfo.h:341` | Log file/stream | I/O bound |
| Core | `TActionLog::FCriticalSection` | `SessionInfo.h:414` | Action log entries | I/O bound |
| Core | `TApplicationLog::FCriticalSection` | `SessionInfo.h:456` | Application log entries | Debug only |
| Core | `TUsage::FCriticalSection` | `Usage.h:37` | Usage counters | Analytics |
| Core | `TWebDAVFileSystem::FNeonLockStoreSection` | `WebDAVFileSystem.h:178` | neon lock store | WebDAV locking |
| filezilla | `CAsyncSslSocketLayer::m_sCriticalSection` | `AsyncSslSocketLayer.h:197` | SSL layer list (static) | Cross-instance SSL state |
| filezilla | `CAsyncSslSocketLayer::m_CriticalSection` | `AsyncSslSocketLayer.h:198` | Per-instance SSL state | Connection-specific |
| filezilla | `CFtpControlSocket::m_SpeedLimitSync` | `FtpControlSocket.h:179` | Speed limit active list | `CCriticalSectionWrapper`; leaf lock; unlock-sleep-relock replaced with event |
| Windows | `TPuttyCleanupThread::FSection` | `GUITools.cpp:188` | Singleton instance pointer | Self-destructing thread |
| Windows | `SystemRequiredThreadSection` | `GUITools.cpp:2550` | `SystemRequiredThread` singleton | Power management |
| Windows | `TOwnConsole::FSection` | `ConsoleRunner.cpp:88` | Console singleton instance | Console runner state |
| Windows | `TExternalConsole::FSection` | `ConsoleRunner.cpp:566` | Console I/O via file mapping | Inter-thread console I/O |
| Windows | `TTerminalManager::FChangeSection` | `TerminalManager.h:121` | Configuration change counter | Config change batching |
| Windows | `StackTraceCriticalSection` | `WinInterface.cpp:615` | `StackTraceMap` (TLS->TStrings) | Debug-only callstack capture |

## 5.2 TraceInitPtr Convention

Static `TCriticalSection` instances are initialized via the `TraceInitPtr` macro (defined in `Global.h`). This macro:
- In debug builds: instruments the pointer with allocation tracking
- In release builds: passes through the argument unchanged

Example:
```cpp
static std::unique_ptr<TCriticalSection> LibS3Section(TraceInitPtr(std::make_unique<TCriticalSection>()));
```

Always pair with a thread-safety inventory comment on the following line:
```cpp
// Thread-safety: LibS3Section protects [state description].
```

The macro is a no-op in release — do not rely on it for runtime behavior.

## 6. Progress Callbacks

Progress callbacks (`FOnProgress`) must be invoked without holding locks that the callback might reacquire. Use recursive `TCriticalSection` with an `FInCallback` guard to detect and suppress unexpected reentrant calls.

## 7. TGuard / TUnguard Patterns

`TGuard` (defined in `Global.h`) is the RAII lock-acquisition wrapper around `TCriticalSection`. `TUnguard` is the inverse — explicit early unlock.

| Pattern | Prevalence | Notes |
|---------|-----------|-------|
| `const TGuard Guard(section)` | 95% of locks | RAII Enter/Leave; exception-safe; preferred style |
| `TGuard Guard(section)` (non-const) | 5% | Same semantics; style inconsistency only — prefer const |
| `const TGuard Guard1(*first); const TGuard Guard2(*second);` | Address-ordered dual lock | Used in `FileOperationProgress::Assign()` to prevent deadlock under concurrent bidirectional assignment. Always lock lower address first. |
| `TUnguard` | Rare | Explicit early unlock (inverse of TGuard); used in legacy code only |
| `TGuard Guard(*section.get())` | Heap-allocated sections | Dereference pattern for `std::unique_ptr<TCriticalSection>` |
| `TGuard Guard(FSection.get())` | Raw pointer variant | Same semantics; used in `ConsoleRunner.cpp` |

**Rule:** Always use `const TGuard` for new code. Only use `TUnguard` when explicitly releasing a lock before a long operation within the same scope.

## 8. Lock Hierarchy

The following lock acquisition order must be respected. Cycles are forbidden.

### Leaf Locks (never held with other locks)

| Lock | File | Notes |
|------|------|-------|
| `m_SpeedLimitSync` | `FtpControlSocket.h` | Controls FTP speed limit; isolated leaf |
| `TracingCriticalSection` | `Global.cpp` | Guards tracing buffers; isolated leaf |
| `DateTimeParamsSection` | `Common.cpp` | Read-heavy; short critical sections only |
| `IgnoredExceptionsCriticalSection` | `Exceptions.cpp` | Exception-type deduplication; rare contention |
| `CoreMainCriticalSection` | `CoreMain.cpp` | Session lifecycle; acquired briefly |
| `SystemRequiredThreadSection` | `GUITools.cpp` | Power management; isolated |
| `TOwnConsole::FSection` | `ConsoleRunner.cpp` | Console singleton; isolated |

### Protocol / Core Layer Locks

| Lock | File | Level | Parent/Child | Notes |
|------|------|-------|-------------|-------|
| `TTerminalQueue::FItemsSection` | `Queue.h` | Queue container | Parent of `TQueueItem::FSection` | Guards queue's item list |
| `TQueueItem::FSection` | `Queue.h` | Item state | Child of `FItemsSection` | Guards individual item state |
| `TFileOperationProgressType::FSection` | `FileOperationProgress.h` | Progress | Acquired under `FItemsSection` | Recursive; guards progress data |
| `TFileOperationProgressType::FUserSelectionsSection` | `FileOperationProgress.h` | User selections | Secondary to `FSection` | Guards user selection state |
| `TFTPFileSystem::FQueueCriticalSection` | `FtpFileSystem.h` | Protocol queue | — | Guards FTP message queue |
| `TFTPFileSystem::FTransferStatusCriticalSection` | `FtpFileSystem.h` | Protocol progress | — | Guards FTP transfer progress |
| `TTerminalItem::FCriticalSection` | `Queue.cpp` | Queue worker | — | Guards terminal item worker state |
| `TWebDAVFileSystem::FNeonLockStoreSection` | `WebDAVFileSystem.h` | WebDAV locks | — | neon lock store |
| `LibS3Section` | `S3FileSystem.cpp` | S3 init | — | libs3 init/deinit refcount |
| `PuttyStorageSection` | `PuttyIntf.cpp` | PuTTY config | — | PuTTY config marshaling |
| `DebugSection` | `NeonIntf.cpp` | neon debug | — | `NeonTerminals` set |
| `PasswordFilesCacheSection` | `SessionData.cpp` | Password cache | — | Per-file cache |

### Plugin / UI Layer Locks

| Lock | File | Level | Notes |
|------|------|-------|-------|
| `TCustomFarPlugin::FCriticalSection` | `FarPlugin.h` | Plugin entry | Guards all Far API callbacks |
| `TFarDialog::FCriticalSection` | `FarPlugin.h` | Dialog state | Idle thread synchronization |
| `TWinSCPFileSystem::FQueueStatusSection` | `WinSCPFileSystem.h` | Queue status | Main thread only; no worker contention |

### Cross-Layer Rules

- **Plugin layer NEVER acquires Core layer locks** — Far API callbacks must not block on protocol operations.
- **Core layer NEVER acquires Plugin layer locks** — protocol code is Far-agnostic per `ARCHITECTURE.md` dependency rules.
- **`FItemsSection` → `Item->GetStatus()` → `FSection`** is the only observed nested lock path. No reverse ordering has been observed.
- **Address-based ordering** for `FileOperationProgress::Assign()`: lock `&FSection` with lower address first, then `&Other.FSection`.
- **TCriticalSection wraps a Windows CRITICAL_SECTION and is recursive** — same thread may re-enter its own lock safely.

## 9. Event Objects

| Event | File | Type | Purpose | Replaced Sleep |
|-------|------|------|-------|---------------|
| `TFarDialogIdleThread::FEvent` | `FarDialog.cpp:33` | Auto-reset | Dialog idle processing signal | N/A (new) |
| `TFarDialog::FSynchronizeObjects[1]` | `FarDialog.cpp:820` | Auto-reset | Dialog `Synchronize()` completion | N/A (new) |
| `TKeepAliveThread::FEvent` | `WinSCPFileSystem.cpp:276` | Auto-reset | Keepalive heartbeat signal | N/A (new) |
| `TFTPFileSystem::FQueueEvent` | `FtpFileSystem.cpp:327` | Manual-reset | FTP message queue non-empty | N/A (new) |
| `TSignalThread::FEvent` | `Queue.cpp:477` | Auto-reset | Generic signal thread wake | N/A (new) |
| `TTerminalThread::FActionEvent` | `Queue.cpp:2771` | Auto-reset | User action completion signal | N/A (new) |
| `TSecureShell::FSocketEvent` | `SecureShell.cpp:95` | Auto-reset | WSA socket readiness | N/A (new) |
| TSecureShell connect Event | `SecureShell.cpp:656` | Auto-reset | Async socket connect completion | N/A (new) |
| `TParallelOperation::FClientsZeroEvent` | `Terminal.cpp:750` | Manual-reset | All parallel clients finished | `Sleep(200)` |
| `TParallelOperation::FDirectoryCreatedEvent` | `Terminal.cpp:751` | Manual-reset | Directory created in parallel op | `Sleep(100)` |
| `CFtpControlSocket::m_SpeedLimitEvent` | `FtpControlSocket.cpp:6059` | Auto-reset | Speed limit recalculation ready | `Sleep(100)` |
| `CMainThread::m_hStartedEvent` | `MainThread.cpp:27` | Manual-reset | Main thread startup complete | `Sleep(10)` |
| `TPuttyCleanupThread::FTimerEvent` | `GUITools.cpp:173` | Auto-reset | Cleanup timer expiration | `Sleep(400)` |
| `TPuttyCleanupThread::FDoneEvent` | `GUITools.cpp:194` | Auto-reset | Cleanup thread finalization | `Sleep(100)` |
| `TSynchronizeController::FStopEvent` | `SynchronizeController.cpp:74` | Manual-reset | Sync poller shutdown | N/A (new) |
| TCallstackThread named event | `WinInterface.cpp:1598` | Auto-reset (named) | Cross-process callstack dump | N/A (new) |

All events are created via `CreateEvent(nullptr, manual_reset, initial_state, nullptr)`. Auto-reset events are used for one-shot signals. Manual-reset events are used for persistent state signals.

## 10. Sleep() Pattern Taxonomy

Not all `Sleep()` calls are threading anti-patterns. The following taxonomy distinguishes legitimate uses. Line numbers current as of 2026-05-14; verify with `grep -n 'Sleep(' src/**/*.cpp` before relying on specific locations.

| Category | Count | Files | Rationale |
|----------|-------|-------|-----------|
| **Thread sync — REPLACED with events** | 0 active | — | All synchronization `Sleep()`s replaced in `multithreading-review-fix.md` |
| **Thread sync — defensive fallback** | 2 | `GUITools.cpp:290` (null `FTimerEvent`), `Terminal.cpp:7833` (null `FDirectoryCreatedEvent`) | Dead code under normal conditions; event creation always succeeds. Kept as safe fallback. |
| **I/O retry backoff** | 1 | `HierarchicalStorage.cpp:1850` | File create `ERROR_SHARING_VIOLATION` retry; legitimate |
| **Intentional pacing/delay** | 3 | `CoreMain.cpp:373` (benchmark), `Queue.cpp:1396` (session reopen), `Setup.cpp:736` (shell default) | Time-based behavior, not synchronization |
| **Console input polling** | 3 | `ConsoleRunner.cpp:433`, `502`, `1083` | Console-mode message loop; no event alternative for null console |
| **External process wait** | 2 | `ConsoleRunner.cpp:2806` (dump file), `Tools.cpp:433` (shell wait) | `ProcessMessages()` + `Sleep()` pattern; could use `WaitForInputIdle` but acceptable |
| **Pipe/IPC polling** | 1 | `GUITools.cpp:395` | `TPuttyPasswordThread` named pipe client wait; timeout-based |

**Rule:** Before adding a new `Sleep()` call, consult this taxonomy. If the call is for thread synchronization, use `WaitForSingleObject` on an event instead.

## 11. Marshaling Primitives

All Far Manager API calls must run on the main thread. The following primitives marshal work from worker threads to the main thread.

| Primitive | File | Pattern | Usage |
|-----------|------|---------|-------|
| `TFarDialog::Synchronize(TThreadMethod)` | `FarDialog.cpp:811` | Semaphore + event pair; worker posts, main executes | Dialog idle processing |
| `TCustomFarPlugin::PostMainThreadSynchro()` | `FarPlugin.cpp` | `ACTL_SYNCHRO` wrapper | General idle event posting |
| `TTerminalThread::WaitForUserAction()` | `Queue.cpp:2845` | Event wait loop on worker; main calls `RunAction()` | Queue user-action marshaling |
| `TTerminalThread::RunAction()` | `Queue.cpp:2870` | Sets action, signals event, executes on main | Main-thread action execution |
| `TTunnelUI` thread-ID guard | `WinSCPFileSystem.cpp` | `GetCurrentThreadId() != FMainThreadId` -> return | Defensive bail-out for callback safety |

**Rule:** Worker threads must never call `FarAdvControl()`, `DialogInit()`, `Message()`, `UpdateConsoleTitle()`, or any other Far API directly. Always marshal through one of the above primitives.
