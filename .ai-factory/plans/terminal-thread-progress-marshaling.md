# Plan: Add OnProgress/OnFinished Marshaling to TTerminalThread (COMPLETE — Source Already Has Implementation)

> Created: 2026-05-10
> Status: **COMPLETE** (verified 2026-05-12 — implementation already exists in source)
> Priority: ~~Critical~~ N/A — resolved: TProgressUserAction, TFinishedUserAction, TerminalProgress(), TerminalFinished() already present in Queue.cpp

## Problem

`TTerminalThread::InitTerminalThread()` marshals 10 UI callbacks to the main thread but does NOT marshal `OnProgress` and `OnFinished`. These callbacks are set by `TWinSCPFileSystem::Connect()` and called on the worker thread, where they invoke Far Manager APIs:

```
TWinSCPFileSystem::OperationProgress()  (called from worker thread)
  -> GetWinSCPPlugin()->SaveScreen()           [Far API ⚠️]
  -> GetWinSCPPlugin()->RestoreScreen()        [Far API ⚠️]
  -> GetWinSCPPlugin()->ClearConsoleTitle()    [Far API ⚠️]
  -> GetWinSCPPlugin()->Message()              [Far API ⚠️ — DIALOG from worker!]
  -> GetWinSCPPlugin()->ShowConsoleTitle()     [Far API ⚠️]
  -> GetWinSCPPlugin()->UpdateConsoleTitleProgress()
     -> FarAdvControl(ACTL_SETPROGRESSSTATE)   [Far API ⚠️]

TWinSCPFileSystem::OperationFinished()  (called from worker thread)
  -> PanelItem->SetSelected(false)             [Far API ⚠️]
  -> FSynchronizeController->LogOperation()    [may call Far APIs]
```

## Scope

| In scope | Out of scope |
|----------|-------------|
| `OnProgress` marshaling in `TTerminalThread` | Local file callbacks (TerminalDeleteLocalFile etc.) — lower risk, separate work |
| `OnFinished` marshaling in `TTerminalThread` | Queue `TTerminalItem` progress (already safe — no Far APIs) |
| Style fix: `TPluginIdleThread` → use `PostMainThreadSynchro()` wrapper | Sleep-based polling cleanup (separate issue) |
| Save/restore original callbacks | OpenSSL thread safety (separate issue) |

## Implementation Steps

### Task 1: Add TProgressUserAction and TFinishedUserAction classes

**Files**: `src/core/Queue.h`, `src/core/Queue.cpp`

Create two new `TUserAction` subclasses following the existing pattern (TInformationUserAction, TQueryUserAction, etc.):

```cpp
// TProgressUserAction — marshals OnProgress callback to main thread
class TProgressUserAction : public TUserAction
{
public:
  TProgressUserAction(TProgressEvent && OnProgress, TFileOperationProgressType & ProgressData);
  virtual void Execute(TObject * Sender) override;
private:
  TProgressEvent FOnProgress;
  TFileOperationProgressType & FProgressData;
};

// TFinishedUserAction — marshals OnFinished callback to main thread
class TFinishedUserAction : public TUserAction
{
public:
  TFinishedUserAction(TFinishedEvent && OnFinished, TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void Execute(TObject * Sender) override;
private:
  TFinishedEvent FOnFinished;
  TFileOperation FOperation;
  TOperationSide FSide;
  bool FTemp;
  UnicodeString FFileName;
  bool FSuccess;
  bool FNotCancelled;
  TOnceDoneOperation & FOnceDoneOperation;
};
```

### Task 2: Add TerminalProgress and TerminalFinished to TTerminalThread

**Files**: `src/core/Queue.h`, `src/core/Queue.cpp`

Add two new methods to `TTerminalThread`:

```cpp
// In Queue.h, add to TTerminalThread:
void TerminalProgress(TFileOperationProgressType & ProgressData);
void TerminalFinished(TFileOperation Operation, TOperationSide Side,
  bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
  TOnceDoneOperation & OnceDoneOperation);
```

Implementation:

```cpp
// In Queue.cpp:
void TTerminalThread::TerminalProgress(TFileOperationProgressType & ProgressData)
{
  TProgressUserAction Action(std::forward<TProgressEvent>(FOnProgress), ProgressData);
  WaitForUserAction(&Action);
}

void TTerminalThread::TerminalFinished(TFileOperation Operation, TOperationSide Side,
  bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
  TOnceDoneOperation & OnceDoneOperation)
{
  TFinishedUserAction Action(std::forward<TFinishedEvent>(FOnFinished),
    Operation, Side, Temp, FileName, Success, NotCancelled, OnceDoneOperation);
  WaitForUserAction(&Action);
}
```

Also add member variables to store original callbacks:

```cpp
// In Queue.h, add to TTerminalThread:
TProgressEvent FOnProgress;
TFinishedEvent FOnFinished;
```

### Task 3: Update InitTerminalThread() to marshal OnProgress/OnFinished

**File**: `src/core/Queue.cpp` — `TTerminalThread::InitTerminalThread()`

Add two new marshal lines:

```cpp
void TTerminalThread::InitTerminalThread()
{
  TSignalThread::InitSignalThread(false);

  // Save originals before overriding (already done for other callbacks)
  FOnProgress = std::move(FTerminal->GetOnProgress());
  FOnFinished = std::move(FTerminal->GetOnFinished());

  FTerminal->SetOnInformation(nb::bind(&TTerminalThread::TerminalInformation, this));
  FTerminal->SetOnQueryUser(nb::bind(&TTerminalThread::TerminalQueryUser, this));
  FTerminal->SetOnPromptUser(nb::bind(&TTerminalThread::TerminalPromptUser, this));
  FTerminal->SetOnShowExtendedException(nb::bind(&TTerminalThread::TerminalShowExtendedException, this));
  FTerminal->SetOnDisplayBanner(nb::bind(&TTerminalThread::TerminalDisplayBanner, this));
  FTerminal->SetOnChangeDirectory(nb::bind(&TTerminalThread::TerminalChangeDirectory, this));
  FTerminal->SetOnReadDirectory(nb::bind(&TTerminalThread::TerminalReadDirectory, this));
  FTerminal->SetOnStartReadDirectory(nb::bind(&TTerminalThread::TerminalStartReadDirectory, this));
  FTerminal->SetOnReadDirectoryProgress(nb::bind(&TTerminalThread::TerminalReadDirectoryProgress, this));
  FTerminal->SetOnInitializeLog(nb::bind(&TTerminalThread::TerminalInitializeLog, this));
  // NEW: marshal progress and finished to main thread
  FTerminal->SetOnProgress(nb::bind(&TTerminalThread::TerminalProgress, this));
  FTerminal->SetOnFinished(nb::bind(&TTerminalThread::TerminalFinished, this));

  Start();
}
```

### Task 4: Restore originals in destructor

**File**: `src/core/Queue.cpp` — `TTerminalThread::~TTerminalThread()`

Restore original callbacks before destruction:

```cpp
TTerminalThread::~TTerminalThread() noexcept
{
  TSimpleThread::Close();

  // ... existing assertions ...

  FTerminal->SetOnInformation(std::forward<TInformationEvent>(FOnInformation));
  FTerminal->SetOnQueryUser(std::forward<TQueryUserEvent>(FOnQueryUser));
  // ... existing restores ...
  FTerminal->SetOnProgress(std::forward<TProgressEvent>(FOnProgress));
  FTerminal->SetOnFinished(std::forward<TFinishedEvent>(FOnFinished));
}
```

### Task 5: Fix style issue in TPluginIdleThread

**File**: `src/NetBox/FarPlugin.cpp`

Replace direct `FarAdvControl(ACTL_SYNCHRO, 0, nullptr)` with `PostMainThreadSynchro(nullptr)`:

```cpp
// Before:
FPlugin->FarAdvControl(ACTL_SYNCHRO, 0, nullptr);

// After:
FPlugin->PostMainThreadSynchro(nullptr);
```

### Task 6: Build and verify

1. Build with `cmd /c build-x64.bat` — must have zero /W4 warnings
2. Run `python scripts/verify_lng_alignment.py` — no .lng changes needed
3. Manual test: connect via SFTP, transfer a file, observe progress dialog
4. Verify no crashes, no UI corruption, progress updates appear normally

## Tradeoffs

| Aspect | Current | After fix |
|--------|---------|-----------|
| Progress update latency | Instant (worker thread) | ~50-100ms round-trip to main thread |
| Thread safety | ❌ Far APIs from worker thread | ✅ All Far APIs on main thread |
| UI corruption risk | High (dialog from worker) | None |
| Code complexity | Low (callbacks bypass marshal) | Medium (2 new action classes) |

The progress marshaling adds latency (~50-100ms per progress tick). This is acceptable because:
- Progress updates are infrequent (every ~500ms or per chunk)
- The latency is imperceptible to the user
- Thread safety is more important than micro-optimization

## Success Criteria

- [ ] Zero build warnings under /W4
- [ ] `OnProgress` and `OnFinished` callbacks called on main thread
- [ ] No Far Manager API calls from worker thread in `OperationProgress`/`OperationFinished`
- [ ] Progress dialog shows normally during file transfer
- [ ] Cancel button works correctly
- [ ] No crashes in 48hr stress test
