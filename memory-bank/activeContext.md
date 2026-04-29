# Active Context

## Current Focus

Multithreading review and fix plan is fully implemented. All 14 tasks across 4 phases have been addressed.

## Key Decisions

- `ACTL_SYNCHRO` is the sanctioned exception to the "main-thread-only Far API" rule. It is explicitly wrapped in `PostMainThreadSynchro()`.
- `TGuard`/`TUnguard` are the standard RAII primitives for `TCriticalSection`.
- `TCriticalSection` is backed by Windows `CRITICAL_SECTION` and is recursive.
- Event-driven waits are preferred over `Sleep()` polling. New events introduced:
  - `CFtpControlSocket::m_SpeedLimitEvent`
  - `CMainThread::m_hStartedEvent`
  - `TParallelOperation::FClientsZeroEvent`
  - `TParallelOperation::FDirectoryCreatedEvent`

## Open Items

- Build verification (MSVC W4, x86/x64, `RelWithDebugInfo`) requires a Windows environment.
- Verify no `Sleep`-based polling remains except the documented `SleepEx` alertable wait.
- Consider adding address-based lock ordering for `FileOperationProgress::Assign()` dual-section acquisition.
