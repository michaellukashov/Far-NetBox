# System Patterns

## Threading Conventions

### Lock Acquisition Order

Observed hierarchy (from leaf to root):

1. `m_SpeedLimitSync` (`CFtpControlSocket`) — leaf lock; never held with other locks.
2. `TTerminalQueue::FItemsSection` — queue container lock.
3. `TQueueItem::FSection` — item state lock; may be acquired under `FItemsSection`.
4. `TFileOperationProgressType::FSection` — progress lock; recursive.

Cross-lock rule: `FItemsSection` may call `Item->GetStatus()` which acquires `FSection`. No reverse ordering has been observed. `TCriticalSection` wraps a Windows `CRITICAL_SECTION` and is recursive.

### Main-Thread-Only Far API Rule

All Far Manager API calls must originate from the main thread, with one explicit exception: `ACTL_SYNCHRO` (`PostMainThreadSynchro`) is the sanctioned cross-thread synchronization primitive and may be called from any thread.

Marshal mechanisms in use:
- `TFarDialog::Synchronize()` — semaphore+event blocking marshal used by `TFarDialogIdleThread`.
- `TTerminalThread::RunAction()` / `WaitForUserAction()` — event-based callback marshal for queue UI actions.
- `TCustomFarPlugin::PostMainThreadSynchro()` — thin wrapper around `ACTL_SYNCHRO`.

### Event-Driven Wait Preference

Prefer `WaitForSingleObject` / `WaitForMultipleObjects` over `Sleep()` polling for thread synchronization.

Known exceptions:
- `FileOperationProgress.cpp` line ~485: `SleepEx(100, true)` is an intentional alertable wait for APC completion.

### RAII Guard Usage

Always use `TGuard` / `TUnguard` for `TCriticalSection` scope management. Never call `Enter`/`Leave` manually.

### Thread-Local Logging Context

Debug logging uses thread IDs in trace output. Static/global mutable state requires explicit protection.
