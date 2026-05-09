# Plan: TSyncPoller — Keep Local Directory Up To Date

> **Branch:** feature/tsyncpoller-keep-uptodate
> **Created:** 2026-05-12
> **Status:** Draft — pending approval
> **Priority:** High — user-visible feature currently crashes (ThrowNotImplemented)
> **Refined:** 2026-05-12 (9 improvements applied)

## Context

The "Keep Local Directory Up To Date" feature (Commands → Synchronize in Far Manager) opens a dialog, accepts settings, but crashes with `ThrowNotImplemented(256)` when the user presses **Start**.

Root cause: `TSynchronizeController` references `Discmon::TDiscMonitor`, a VCL component from WinSCP that was never ported to NetBox. All TDiscMonitor code is commented out with `// FIXME`.

## Goal

Replace the non-existent `Discmon::TDiscMonitor` with a lightweight Win32 `FindFirstChangeNotification`-based poller (`TSyncPoller`) that:
- Monitors the local directory tree for file/directory changes
- Triggers remote synchronization via the existing `TSynchronizeController::SynchronizeChange()` callback
- Marshals all UI-affecting work to the main thread using existing `TFarDialog::Synchronize()` plumbing
- Starts and stops cleanly without handle leaks
- Logs sync events via `SynchronizeLog()`

## Scope

| In Scope | Out of Scope |
|----------|-------------|
| `TSyncPoller` class implementation | Task 4.2 (sync progress dialog enhancement) — separate plan |
| Integration into `TSynchronizeController` | TDiscMonitor full port (VCL component) |
| Removal of stale `Discmon::TDiscMonitor` references | UI/UX redesign of sync dialog |
| Build with zero `/W4` warnings | |
| Manual test: start/stop/change detection | |

## Architecture

```
User presses START in TSynchronizeDialog
              ↓
TSynchronizeController::StartStop(Start=true)
              ↓
    ┌─────────────────────┐
    │   TSyncPoller::Start │  Creates thread + notification handle
    └──────────┬────────────┘
               ↓
    [Background Thread Loop]
               ↓
    WaitForMultipleObjects([ChangeHandle, StopEvent], timeout)
               ↓
    ChangeHandle signaled → Directory changed!
               ↓
    OnSynchronizeThreads(RootDirCallback)  ← marshal to main thread
               ↓
    [Main Thread]
               ↓
    TSynchronizeController::SynchronizeChange()
               ↓
    Terminal::SynchronizeCollect() → apply changes
               ↓
    FindNextChangeNotification()  ← re-arm immediately
               ↓
    Loop...
```

## Design Decisions

| Decision | Rationale |
|----------|-----------|
| **FindFirstChangeNotification** (not ReadDirectoryChangesW) | Simpler API, no buffer management, sufficient for coarse-grained "something changed" detection. WinXP-compatible. |
| **1-2 second timeout cap** | Prevents indefinite hangs on thread shutdown. Uses `min(KeepUpToDateChangeDelay, 2000)`. |
| **Re-arm immediately after signal** | Simpler state machine. Risk of missed changes during sync window is **accepted** per user decision. |
| **Always sync from RootDirectory** | `SynchronizeChange` receives the root dir, not the specific changed path. Less efficient but correct and avoids partial-tree sync bugs. |
| **Separate thread (not Far idle)** | `FindFirstChangeNotification` requires a waiting thread. Cannot be done in Far's idle callback without blocking. |
| **StopPoller() does NOT WaitFor** | Avoids deadlock when thread is blocked in `TFarDialog::Synchronize()`. Destructor waits instead. |

## Implementation Steps

### Task 1: Create TSyncPoller class

**File:** `src/windows/SynchronizeController.cpp` (add before existing code)
**File:** `src/windows/SynchronizeController.h` (add declaration)

```cpp
class TSyncPoller final : public TSimpleThread
{
  NB_DISABLE_COPY(TSyncPoller)
public:
  TSyncPoller() = delete;
  explicit TSyncPoller(const UnicodeString & Directory, bool Recursive,
    TSynchronizeThreadsEvent OnSynchronizeThreads,
    TThreadMethod OnChange, TSynchronizeInvalidEvent OnInvalid) noexcept;
  virtual ~TSyncPoller() noexcept override;

  void StartPoller();
  void StopPoller();

protected:
  virtual void Execute() override;
  virtual void Terminate() override;

private:
  UnicodeString FDirectory;
  bool FRecursive{false};
  TSynchronizeThreadsEvent FOnSynchronizeThreads;
  TThreadMethod FOnChange;         // zero-arg delegate, called on main thread
  TSynchronizeInvalidEvent FOnInvalid; // (controller, dir, error)
  HANDLE FChangeHandle{INVALID_HANDLE_VALUE};
  HANDLE FStopEvent{INVALID_HANDLE_VALUE};
  bool FStarted{false};

  bool OpenNotification();
  void CloseNotification();
};
```

Implementation:

```cpp
TSyncPoller::TSyncPoller(const UnicodeString & Directory, bool Recursive,
  TSynchronizeThreadsEvent OnSynchronizeThreads,
  TThreadMethod OnChange, TSynchronizeInvalidEvent OnInvalid) noexcept :
  TSimpleThread(OBJECT_CLASS_TSyncPoller),
  FDirectory(Directory),
  FRecursive(Recursive),
  FOnSynchronizeThreads(OnSynchronizeThreads),
  FOnChange(OnChange),
  FOnInvalid(OnInvalid)
{
}

TSyncPoller::~TSyncPoller() noexcept
{
  StopPoller();
}

void TSyncPoller::StartPoller()
{
  DebugAssert(!FStarted);
  FStopEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
  TSimpleThread::InitSimpleThread("NetBox Sync Poller");
  Start();
  FStarted = true;
}

void TSyncPoller::StopPoller()
{
  if (FStarted)
  {
    // Set stop event but do NOT WaitFor here.
    // WaitFor would deadlock if the thread is blocked in
    // TFarDialog::Synchronize() waiting for the main thread.
    // The thread will exit naturally when it sees the stop event.
    ::SetEvent(FStopEvent);
    FStarted = false;
  }
}

bool TSyncPoller::OpenNotification()
{
  FChangeHandle = ::FindFirstChangeNotification(
    FDirectory.c_str(),
    FRecursive,
    FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_DIR_NAME |
    FILE_NOTIFY_CHANGE_LAST_WRITE);
  return FChangeHandle != INVALID_HANDLE_VALUE;
}

void TSyncPoller::CloseNotification()
{
  if (FChangeHandle != INVALID_HANDLE_VALUE)
  {
    ::FindCloseChangeNotification(FChangeHandle);
    FChangeHandle = INVALID_HANDLE_VALUE;
  }
}

void TSyncPoller::Execute()
{
  if (!OpenNotification())
  {
    if (FOnInvalid)
      FOnInvalid(nullptr, FDirectory, L"Failed to open directory notification");
    return;
  }

  const DWORD PollInterval = nb::Min(
    static_cast<DWORD>(GUIConfiguration->GetKeepUpToDateChangeDelay()), 2000u);

  HANDLE Handles[] = { FChangeHandle, FStopEvent };

  while (!IsFinished())
  {
    const DWORD WaitResult = ::WaitForMultipleObjects(
      _countof(Handles), Handles, FALSE, PollInterval);

    if (WaitResult == WAIT_OBJECT_0)  // Change detected
    {
      // Marshal to main thread for sync
      if (FOnSynchronizeThreads)
      {
        FOnSynchronizeThreads(this, FOnChange);
      }

      // Re-arm immediately (per design decision)
      if (!::FindNextChangeNotification(FChangeHandle))
      {
        // Handle invalidated (e.g., directory deleted) — reopen
        CloseNotification();
        if (!OpenNotification())
        {
          if (FOnInvalid)
            FOnInvalid(nullptr, FDirectory, L"Lost directory notification");
          break;
        }
      }
    }
    else if (WaitResult == WAIT_OBJECT_0 + 1)  // Stop event
    {
      break;
    }
    // WAIT_TIMEOUT — loop and re-wait
  }

  CloseNotification();
}
```

**Notes:**
- `OBJECT_CLASS_TSyncPoller` must be added to `ObjIDs.h` / `ObjIDs.cpp`
- `FOnChange` is a `TThreadMethod` (zero-arg delegate), not a `TNotifyEvent`
- `FOnInvalid` is a `TSynchronizeInvalidEvent` (3-arg delegate), not a `TNotifyEvent`
- `StopPoller()` only sets the stop event — does NOT call `WaitFor()` (avoids deadlock)
- The destructor calls `StopPoller()` which sets the event, then `TSimpleThread::Close()` waits for the thread
- Re-arm is immediate after signal (per decision)

### Task 2: Integrate TSyncPoller into TSynchronizeController

**File:** `src/windows/SynchronizeController.cpp`

1. **Remove stale references:**
   - Remove `#include <DiscMon.hpp>` (line 8)
   - Remove `namespace Discmon { class TDiscMonitor; }` forward declaration from header (lines 42-45)
   - Replace `Discmon::TDiscMonitor * FSynchronizeMonitor` with `TSyncPoller * FSyncPoller` in header (line 70)
   - Add `bool FSubdirsChanged{false}` member to `TSynchronizeController` (for `SynchronizeChange` output parameter)

2. **Add public helper for SynchronizeChange callback:**
   `SynchronizeChange` is private. Add a public method that TSyncPoller can call:

```cpp
// In TSynchronizeController header, add to public section:
void NotifySynchronizeChange(const UnicodeString & Directory);

// In .cpp:
void TSynchronizeController::NotifySynchronizeChange(const UnicodeString & Directory)
{
  FSubdirsChanged = false;
  SynchronizeChange(this, Directory, FSubdirsChanged);
}
```

3. **Update `StartStop()`:**
   - Remove `ThrowNotImplemented(256)` at line 82
   - Remove commented-out TDiscMonitor block (lines 83-108)
   - Add `FOnSynchronizeThreads = OnSynchronizeThreads;` (store for later use)
   - Create `TThreadMethod` callback that calls `NotifySynchronizeChange` with root directory
   - Add logging:

```cpp
// Replace lines 82-108:
FOnSynchronizeThreads = OnSynchronizeThreads;

// Create callback for polling
TThreadMethod ChangeCallback = nb::bind(&TSynchronizeController::NotifySynchronizeChange, this,
  FSynchronizeParams.LocalDirectory);

FSyncPoller = new TSyncPoller(
  FSynchronizeParams.LocalDirectory,
  FLAGSET(FSynchronizeParams.Options, soRecurse),
  OnSynchronizeThreads,
  ChangeCallback,
  nb::bind(&TSynchronizeController::SynchronizeInvalid, this));

// Log start
SynchronizeLog(slStart, FMTLOAD(SYNCHRONIZE_START, 1));

try
{
  FSyncPoller->StartPoller();
}
catch (...)
{
  SAFE_DESTROY(FSyncPoller);
  ThrowNotImplemented(256);
  throw;
}
```

4. **Update `SynchronizeAbort()`:**
   - Remove `ThrowNotImplemented(258)`
   - Replace `// FIXME FSynchronizeMonitor->Close()` with:

```cpp
if (FSyncPoller != nullptr)
{
  FSyncPoller->StopPoller();
  SAFE_DESTROY(FSyncPoller);
}
```

5. **Update destructor:**
   - Change `DebugAssert(FSynchronizeMonitor == nullptr)` to `DebugAssert(FSyncPoller == nullptr)`

6. **Update catch block in `StartStop()`:**
   - Remove `ThrowNotImplemented(257)`
   - Add cleanup:

```cpp
catch(...)
{
  if (FSyncPoller != nullptr)
  {
    FSyncPoller->StopPoller();
    SAFE_DESTROY(FSyncPoller);
  }
  throw;
}
```

### Task 3: Update header file

**File:** `src/windows/SynchronizeController.h`

1. Remove:
```cpp
namespace Discmon
{
class TDiscMonitor;
}
```

2. Add forward declaration for `TSyncPoller` (after existing forward declarations):
```cpp
class TSyncPoller;
```

3. Change member declaration:
```cpp
// Before:
Discmon::TDiscMonitor * FSynchronizeMonitor{nullptr};

// After:
TSyncPoller * FSyncPoller{nullptr};
```

4. Add member for output parameter:
```cpp
bool FSubdirsChanged{false};
```

5. Add public method:
```cpp
void NotifySynchronizeChange(const UnicodeString & Directory);
```

### Task 4: Add ObjID for TSyncPoller

**Files:** `src/base/ObjIDs.h`, `src/base/ObjIDs.cpp`

Add `OBJECT_CLASS_TSyncPoller` entry following existing pattern (near line 163 in ObjIDs.cpp, after `OBJECT_CLASS_TKeepAliveThread`):

```cpp
// In ObjIDs.h:
extern const TObjectClassId OBJECT_CLASS_TSyncPoller;

// In ObjIDs.cpp:
const TObjectClassId OBJECT_CLASS_TSyncPoller = static_cast<TObjectClassId>(nb::counter_id());
```

### Task 5: Verify no remaining TDiscMonitor references

Search and remove any remaining references to `TDiscMonitor`, `DiscMon.hpp`, or `Discmon::` namespace. Use:
```
grep -r "TDiscMonitor\|DiscMon\|Discmon" src/ libs/
```

### Task 6: Build verification

```cmd
cmd /c build-x64.bat
```

- Zero warnings under `/W4`
- Plugin DLL in `Far3_x64/Plugins/NetBox/`

### Task 7: Manual test protocol

1. Open Far Manager, connect to SFTP session
2. Navigate to a local directory paired with a remote directory
3. Commands → Synchronize (Alt-Shift-F12)
4. Check "Synchronize files", set local/remote dirs
5. Press **Start** — should NOT crash
6. Dialog title changes to "Synchronizing...", Stop button enables
7. In Windows Explorer, create a new file in the local directory
8. Within ~2 seconds, observe file appears on remote side
9. Press **Stop** — monitoring stops, dialog returns to idle state
10. Close dialog, disconnect — no crashes

**Regression test:**
- Full synchronize (non-keep-up-to-date) still works
- Compare directories still works
- Queue operations still work

## Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| FindFirstChangeNotification fails on network drives | Medium | Medium | Log error, show message dialog, stop poller gracefully |
| Rapid changes cause sync thrashing | Low | Low | 1-2s timeout + sync operation duration naturally throttles |
| Handle leak if thread killed unexpectedly | Low | High | RAII in destructor, `StopPoller()` in all exit paths |
| Marshaling fails if Far dialog closed during sync | Low | High | `SynchronizeChange` already has exception handling; poller thread catches and aborts |
| Build breaks due to missing includes | Low | High | Use existing Windows headers (already included via `<Common.h>` → `<vcl.h>`) |
| **Deadlock in StopPoller** | **Medium** | **High** | **StopPoller() does NOT call WaitFor(); destructor handles cleanup** |

## Commit Checkpoint

```
feat(sync): implement TSyncPoller for "Keep Local Directory Up To Date"

- Replace non-existent Discmon::TDiscMonitor with Win32
  FindFirstChangeNotification-based TSyncPoller
- Directory changes trigger sync via existing marshal plumbing
- 1-2s timeout cap, immediate re-arm, recursive subtree support
- Remove all ThrowNotImplementeds from sync controller
- Add SynchronizeLog() calls for polling start/stop
- Fix deadlock: StopPoller() does not WaitFor
- Zero /W4 warnings
```

## Success Criteria

- [x] `TSyncPoller` class exists with proper thread lifecycle
- [ ] Start button in sync dialog works (no crash)
- [ ] Local file changes trigger remote sync within ~2 seconds
- [ ] Stop button stops monitoring cleanly (no deadlock)
- [x] Zero build warnings
- [x] No TDiscMonitor/Discmon references remain
- [ ] Regression: full sync, compare dirs, queue all work
