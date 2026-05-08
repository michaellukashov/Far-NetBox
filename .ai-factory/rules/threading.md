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

## 6. Progress Callbacks

Progress callbacks (`FOnProgress`) must be invoked without holding locks that the callback might reacquire. Use recursive `TCriticalSection` with an `FInCallback` guard to detect and suppress unexpected reentrant calls.
