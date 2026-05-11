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

- `TracingCriticalSection` (`Global.cpp`): Protects `IsTracing` and `WriteTraceBuffer` access. `DoAssert()` reads `IsTracing` without a lock, which is acceptable as `bool` reads are atomic on x86/x64 platforms.
- `TTerminalQueue::FItemsSection` (`Queue.h`): Protects the `TTerminalQueue`'s internal list of `TQueueItem` objects.
- `TQueueItem::FSection` (`Queue.h`): Protects the internal state of individual `TQueueItem` objects.
- `TFileOperationProgressType::FSection` (`FileOperationProgress.h`): Protects progress reporting data for file operations.
- `FSection` (within `TPuttyCleanupThread` in `GUITools.cpp`): Protects the internal state of the `TPuttyCleanupThread` instance.
## 6. Progress Callbacks

Progress callbacks (`FOnProgress`) must be invoked without holding locks that the callback might reacquire. Use recursive `TCriticalSection` with an `FInCallback` guard to detect and suppress unexpected reentrant calls.

## 7. Lock Hierarchy

The following lock acquisition order must be respected. Cycles are forbidden.

| Lock | File | Level | Notes |
|------|------|-------|-------|
| `m_SpeedLimitSync` | `FtpControlSocket.h` | Leaf | Controls FTP speed limit; never held with other locks |
| `TTerminalQueue::FItemsSection` | `Queue.h` | Queue container | Guards access to the queue's item list; may acquire `TQueueItem::FSection` |
| `TQueueItem::FSection` | `Queue.h` | Item state | Guards individual queue item state; acquired under `FItemsSection` |
| `TFileOperationProgressType::FSection` | `FileOperationProgress.h` | Progress | Guards file operation progress state; recursive; acquired under `FItemsSection` |
| `FSection` (TPuttyCleanupThread) | `GUITools.cpp` | Cleanup | Guards `TPuttyCleanupThread` state; isolated; never held with other locks |
| `TracingCriticalSection` | `Global.cpp` | Leaf | Guards tracing buffers; never held with other locks |

Cross-lock rule: `FItemsSection` → `Item->GetStatus()` → `FSection`. No reverse ordering has been observed. `TCriticalSection` wraps a Windows `CRITICAL_SECTION` and is recursive.

## 8. Event Objects

| Event | File | Type | Purpose |
| Event | File | Type | Purpose |
|-------|------|------|---------|
| `m_SpeedLimitEvent` | `FtpControlSocket.cpp` | Auto-reset | Signals changes in speed limit, replacing `Sleep(100)` in speed limit wait |
| `FClientsZeroEvent` | `Terminal.cpp` | Manual-reset | Signals completion of parallel transfers, replacing `Sleep(200)` |
| `FDirectoryCreatedEvent` | `Terminal.cpp` | Manual-reset | Signals completion of directory creation, replacing `Sleep(100)` |
| `m_hStartedEvent` | `MainThread.cpp` | Manual-reset | Signals thread startup, replacing `Sleep(10)` |
| `FDoneEvent` | `GUITools.cpp` | Auto-reset | Signals `TPuttyCleanupThread` finalization, replacing `Sleep(100)` |
| `FTimerEvent` | `GUITools.cpp` | Auto-reset | Signals `TPuttyCleanupThread` timer expiration, replacing `Sleep(400)` |


All events are created via `CreateEvent(nullptr, manual_reset, initial_state, nullptr)`. Auto-reset events are used for one-shot signals. Manual-reset events are used for persistent state signals.
