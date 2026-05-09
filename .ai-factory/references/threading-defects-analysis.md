# Threading Defects Analysis — Deep Codebase Audit

> Generated: 2026-05-10
> Based on parallel exploration of `src/NetBox/WinSCPFileSystem.cpp`, `src/filezilla/FtpControlSocket.cpp`, `src/core/FileOperationProgress.cpp`, `src/core/Terminal.cpp`, `src/base/Global.cpp`, `src/core/CoreMain.cpp`, `src/core/Queue.cpp`, `src/NetBox/FarPlugin.cpp`, `src/NetBox/FarDialog.cpp`, `src/windows/ConsoleRunner.cpp`, `src/windows/GUITools.cpp`, `src/core/HierarchicalStorage.cpp`, `src/core/NeonIntf.cpp`, `src/core/SessionData.cpp`, `src/base/Classes.cpp`, `src/filezilla/MainThread.cpp`

---

## Summary

Three categories of threading violations were found in the NetBox codebase:

1. **Far Manager API callbacks invoked from worker threads** — one confirmed workaround (`OperationProgress` bail-out) and several unguarded suspects (`TerminalReadDirectoryProgress`, `TerminalInformation`).
2. **TOCTOU races** in `FtpControlSocket` where a critical section is released, the thread sleeps on an event, then the section is re-acquired.
3. **Callbacks** in `FileOperationProgress` and `Queue` that access shared mutable state without holding the corresponding lock.

Additionally: several `Sleep()`-based polling loops in core and windows layers, plus a handful of unsynchronized global/static variables accessed across threads.

---

## 1. CRITICAL — Far Manager API Called from Worker Thread

### 1.1 `src/NetBox/WinSCPFileSystem.cpp`

#### `OperationProgress` — already guarded (proves the problem exists)

**Location:** line ~3901

```cpp
void TWinSCPFileSystem::OperationProgress(
  TFileOperationProgressType & ProgressData)
{
  if (::GetCurrentThreadId() != FMainThreadId)
  {
    TINYLOG_WARNING(g_tinylog) << TLogContext::Format()
        << "OperationProgress skipped: worker thread "
        << std::to_string(::GetCurrentThreadId()) << " != main "
        << std::to_string(FMainThreadId);
    return;
  }
  // ... Far API calls: SaveScreen, RestoreScreen, Message, GetMsg ...
}
```

The log warning proves the terminal fires `OnProgress` from worker threads. The guard is a **defensive bail-out**, not a proper main-thread marshal.

#### `TerminalReadDirectoryProgress` — **unguarded**

**Location:** line ~3691

```cpp
void TWinSCPFileSystem::TerminalReadDirectoryProgress(
  TObject * /*Sender*/, int32_t Progress, int32_t /*ResolvedLinks*/, bool & Cancel)
{
  if (Progress < 0)
  {
    if (!FNoProgress && (Progress == -2))
    {
      MoreMessageDialog(GetMsg(NB_DIRECTORY_READING_CANCELLED), nullptr,
        qtWarning, qaOK);          // <-- Far API dialog from potential worker thread
    }
  }
  else
  {
    if (GetWinSCPPlugin()->CheckForEsc())
    {
      Cancel = true;
    }
    if (!FNoProgress)
    {
      GetWinSCPPlugin()->UpdateConsoleTitle(
        FORMAT("%s (%d)", GetMsg(NB_READING_DIRECTORY_TITLE), Progress));
    }
  }
}
```

Calls `MoreMessageDialog`, `CheckForEsc`, `UpdateConsoleTitle` **without any thread guard**. If dispatched from a background thread (e.g., during `TTerminalItem::ProcessEvent`), this violates Far Manager's threading contract.

#### `TerminalInformation` — **unguarded**

**Location:** line ~3623

```cpp
void TWinSCPFileSystem::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & AStr, int32_t Phase, const UnicodeString & /*Additional*/)
{
  if (Phase != 0)
  {
    // ... calls UpdateConsoleTitle, MoreMessageDialog ...
  }
}
```

Registered on `FTerminal` alongside the guarded `OperationProgress`. Same concern — needs audit.

**Callback registration point:** line ~3444

```cpp
FTerminal->SetOnReadDirectoryProgress(nb::bind(&TWinSCPFileSystem::TerminalReadDirectoryProgress, this));
FTerminal->SetOnInformation(nb::bind(&TWinSCPFileSystem::TerminalInformation, this));
FTerminal->SetOnProgress(nb::bind(&TWinSCPFileSystem::OperationProgress, this));
```

### 1.2 `src/NetBox/FarPlugin.cpp` — idle thread is correct, but shows awareness

**Location:** lines ~42–64

`TPluginIdleThread::Execute` marshals idle work to the main thread via `PostMainThreadSynchro` / `ACTL_SYNCHRO`. This is the **correct pattern**.

**Anti-deadlock pattern (not a violation, but notable):**

`TCustomFarPlugin::HandleException` (~1734) and `TWinSCPPlugin::HandleException` (`WinSCPPlugin.cpp:807`) use `TUnguard` to release the global `FCriticalSection` before showing modal dialogs. The codebase is aware that holding a plugin lock while calling Far API is dangerous because Far may re-enter plugin exports from the same thread.

### 1.3 `src/NetBox/FarDialog.cpp` — dialog idle thread is correct

**Location:** lines ~18–47

`TFarDialogIdleThread::Execute` correctly marshals `TFarDialog::Idle` to the main thread via `Synchronize` (semaphore/event pair at line ~824). No violation here; it demonstrates the proper pattern.

The fact that a separate idle thread exists for dialogs shows dialog code is at risk of being called off the main thread and requires active marshaling.

---

## 2. HIGH — Race Conditions

### 2.1 `src/filezilla/FtpControlSocket.cpp` — TOCTOU race in speed-limit throttling

**Location:** `CFtpControlSocket::GetAbleToUDSize()` lines ~6030–6062

```cpp
// m_SpeedLimitSync is a CCriticalSectionWrapper (static, shared across all FTP sockets)
m_SpeedLimitSync.Unlock();
::WaitForSingleObject(m_SpeedLimitEvent, 100);   // sleep 100ms
m_SpeedLimitSync.Lock();
```

**Problem:** Between `Unlock()` and `Lock()`, another thread can:
- Erase the current entry from `m_InstanceList` via `RemoveActiveTransfer`
- Modify iterators
- Making the `iter` parameter (passed by reference from caller) stale

The code re-searches the list after re-locking (lines 6066–6070), but the race window is real.

**Caller context:** `GetAbleToTransferSize()` (line ~6127–6144) holds the outer lock while calling `GetAbleToUDSize`, which drops the lock internally — this is a **re-entrant lock release**.

**Related:** `SpeedLimitAddTransferredBytes` (lines 6168–6186) sets `m_SpeedLimitEvent` while holding the lock, which can wake the waiting thread just before it unlocks, but the sleeping thread still loses atomicity.

### 2.2 `src/core/FileOperationProgress.cpp` — callbacks without lock

#### `DoProgress()` calls `FOnProgress` without `FSection`

**Location:** lines 418–434

```cpp
void TFileOperationProgressType::DoProgress()
{
  SystemRequired();
  // Reentrancy guard: FOnProgress may call back into FileOperationProgress
  // methods (e.g., Suspend). TCriticalSection is recursive, so locks are safe,
  // but nested callbacks can recurse infinitely if the caller isn't careful.
  if (!FInCallback)
  {
    TValueRestorer<bool> InCallbackRestorer(FInCallback);
    FInCallback = true;
    FOnProgress(*this);   // <-- NO FSection lock held!
  }
}
```

Many callers modify fields (`SetFile` ~484, `AddTransferred` ~878, `SetTransferSize` ~622, `ChangeTransferSize` ~792, `AddLocallyUsed` ~515) then call `DoProgress()` without any lock.

If `FOnProgress` executes on a different thread (main thread UI update), it reads `FFileName`, `FTransferredSize`, `FTransferSize`, `FFileInProgress` while they are in the middle of being updated.

#### `GetCancel()` reads `FCancel` without lock

**Location:** lines 681–694

```cpp
TCancelStatus TFileOperationProgressType::GetCancel() const
{
  TCancelStatus Result = FCancel;          // <-- reads without lock
  if (FParent != nullptr)
  {
    const TGuard Guard(FSection);            // <-- only locks for parent lookup
    const TCancelStatus ParentCancel = FParent->GetCancel();
    if (ParentCancel > Result)
    {
      Result = ParentCancel;
    }
  }
  return Result;
}
```

`SetCancel()` (line 656) and `SetCancelAtLeast()` (line 667) **write** `FCancel` under `FSection`. `GetCancel()` reads it without a lock — data race on cancel status.

### 2.3 `src/core/Queue.cpp` — unprotected virtual callback

**Location:** `TQueueItem::SetProgress` lines 1885–1903

```cpp
// Acquires FSection to copy progress data
// ... but drops the lock before calling:
ProgressUpdated();           // virtual callback, unprotected
FQueue->DoQueueItemUpdate(this);   // unprotected
```

The virtual callback is dispatched without holding the section lock.

**Also:** `TQueueItemProxy::GetProgressData` (lines 2020–2023) reads `FProgressData->GetOperation()` without holding `FSection`, while `SetProgress` writes `FProgressData` under `FSection`.

---

## 3. MEDIUM — Busy-Waiting with `Sleep()`

### 3.1 `src/core/Terminal.cpp`

#### `TParallelOperation::WaitFor()` — `Sleep(200)` fallback

**Location:** lines ~830–862

```cpp
do {
  // polls FClients == 0
  // when FClientsZeroEvent is null, falls back to:
  Sleep(200);
} while (FClients > 0);
```

Variables: `FClients`, `FClientsZeroEvent`, `FProbablyEmpty`.

#### `TTerminal::CopyParallel()` — `Sleep(100)` polling

**Location:** lines ~7817–7843

```cpp
do {
  if (CopyToParallel(...) == 0) {
    Sleep(100);   // polling when no parallel work available
  }
} while (Continue && !GotNext);
```

Variables: `Continue`, `GotNext`, `ParallelOperation`, `AOperationProgress`.

### 3.2 `src/core/HierarchicalStorage.cpp` — defensive retry loop

**Location:** lines ~1832–1852

```cpp
do {
  Handle = CreateFile(...);
  Error = GetLastError();
  Retry = (Error == ERROR_SHARING_VIOLATION);
  if (Retry) {
    Sleep(Step);   // Step = 100ms, Trying += Step until 2000ms
  }
} while (Retry && (Trying < 2000));
```

Variables: `Handle`, `Error`, `Retry`, `Trying`.

Acceptable defensive retry; should be documented with a comment explaining the intentional polling.

### 3.3 `src/windows/ConsoleRunner.cpp` — console input polling

- `TOwnConsole` input polling loop: `Sleep(50)` (line ~432)
- `WaitBeforeExit` console-input polling: `Sleep(50)` (line ~500)
- `TNullConsole::Choice`: `Sleep(Timer)` (line ~1080)
- Script choice handler: `Sleep(Timer)` (line ~1720)
- Callstack dump wait loop: `Sleep(1000)` (lines ~2795–2810, line ~2801)

### 3.4 `src/windows/GUITools.cpp` — cleanup thread busy-wait

- `TPuttyCleanupThread::Finalize()` while-true loop: `Sleep(100)` (line ~218)
- `TPuttyCleanupThread::Execute()` do-while loop: `Sleep(400)` (line ~276)
- `TPuttyPasswordThread::Execute()` while loop: `DoSleep(50)` (lines ~367, ~412, ~419)

These are cleanup threads waiting for child process exit. `WaitForSingleObject` on the process handle is the proper replacement.

---

## 4. MEDIUM — Unsynchronized Global/Static Mutable State

### 4.1 `src/base/Global.cpp` — debug tracing globals

**Location:** lines ~69–76

```cpp
static HANDLE TraceFile;           // line 69
extern bool IsTracing;             // line 70
extern uint32_t CallstackTls;      // line 72
extern TCriticalSection* TracingCriticalSection;  // line 73
extern bool TracingInMemory;       // line 75
extern HANDLE TracingThread;         // line 76
```

`DoDirectTrace`/`DoTrace` read `TraceFile` and `IsTracing` without locks. The file already contains a thread-safety inventory comment acknowledging this.

**Fix:** Use `TracingCriticalSection` (which already exists) to guard reads.

### 4.2 `src/core/CoreMain.cpp` — session globals

```cpp
extern TApplicationLog* ApplicationLog;   // line 22
extern bool AnySession;                   // line 23
static bool StoredSessionsInitialized;    // line 170
```

- `AnySession` is written from `Terminal.cpp:1564` (`TTerminal::Open`) and read from `CoreMain.cpp:328` (`CoreUpdateFinalStaticUsage`) with **no lock**.
- `StoredSessionsInitialized` is toggled in `GetStoredSessions`/`DeleteStoredSessions` without a lock.

### 4.3 `src/base/Classes.cpp` — global functions pointer

```cpp
static TGlobals* GlobalFunctions;   // line 11
```

Set once during single-threaded plugin initialization; comment asserts no synchronization required. No other mutable globals found.

### 4.4 `src/core/SessionData.cpp` — properly locked

- `TPasswordFilesCache PasswordFilesCache` (line ~2058) is protected by `PasswordFilesCacheSection` (line ~2056).
- `TIntMapping ProxyMethodMapping` (line ~51) is initialized once and effectively read-only.

No unsynchronized mutable globals detected.

### 4.5 `src/core/NeonIntf.cpp` — properly locked

- `static nb::set_t<TTerminal*> NeonTerminals` (line ~428) is protected by `static TCriticalSection DebugSection` (line ~426).
- Neon callbacks use per-session private data (`SESSION_TERMINAL_KEY`, `SESSION_PROXY_AUTH_KEY`, `SESSION_TLS_INIT_KEY`) rather than global state.

No unsynchronized mutable globals detected.

---

## 5. Architecture Notes

The plugin has a main Far thread (`TCustomFarPlugin` records `FFarThreadId`) and several worker thread families:

1. **TPluginIdleThread / TFarDialogIdleThread** — marshal idle events back to the main thread via `ACTL_SYNCHRO` or `Synchronize` semaphore pairs. These are **correct**.
2. **TTerminalItem / TBackgroundTerminal** — worker threads run queued file operations. Their UI callbacks (`QueryUser`, `PromptUser`, `ShowExtendedException`) are marshaled to the main thread via `WaitForUserAction`/`RunAction` event pairs. However, the `OnProgress` callback path was **not fully marshaled** for the direct-terminal case, leading to the `OperationProgress` worker-thread guard.
3. **FileZilla CFtpControlSocket** — uses a global `CCriticalSectionWrapper m_SpeedLimitSync` with an explicit unlock/sleep/lock anti-pattern for throttling.
4. **TFileOperationProgressType** — uses `TCriticalSection FSection`, but `DoProgress()` deliberately omits it (comment notes recursive lock safety) and callers frequently modify state without locking before invoking the callback, relying on single-threaded usage of the progress object. The `FOnProgress` callback itself may run on the main thread when marshaled, so the lack of lock around the callback dispatch is a cross-thread data race.

---

## Severity Matrix

| Severity | Count | Categories |
|---|---|---|
| CRITICAL | 2 | `TerminalReadDirectoryProgress`, `TerminalInformation` — no worker-thread guard |
| HIGH | 3 | `FtpControlSocket` TOCTOU, `FileOperationProgress` callback race, `FileOperationProgress` `GetCancel` race |
| MEDIUM | 6+ | `Sleep(200)` parallel wait, `Sleep(100)` copy parallel, `Sleep(50/100/400)` cleanup threads, debug tracing globals, `AnySession`, `StoredSessionsInitialized` |
