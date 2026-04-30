# KeepAlive Far 3 Fix Plan (Option B)

**GitHub Issue:** #329 — KeepAlive broken since FAR 3  
**Date:** 2026-04-29  
**Status:** Pending Implementation

## Problem

The per-session `TKeepAliveThread` calls `Terminal->Idle()` directly from a background worker thread. Far Manager 3 requires all plugin API interactions to happen on the main thread. This violates the threading contract and causes crashes/hangs.

## Root Cause

`TKeepAliveThread::Execute()` in `src/NetBox/WinSCPFileSystem.cpp` runs on a dedicated background thread and directly invokes:

```cpp
FFileSystem->KeepaliveThreadCallback();
```

which acquires a filesystem-level critical section and then calls `FTerminal->Idle()`. The global plugin idle thread (`TPluginIdleThread`) already uses the sanctioned `ACTL_SYNCHRO` / `PostMainThreadSynchro` path to marshal `FE_IDLE` events to the main thread, where `Terminal->Idle()` is safely invoked. The per-session thread bypasses this mechanism.

## Solution (Option B)

Adapt `TKeepAliveThread` to marshal keepalive processing to the main thread using the sanctioned `ACTL_SYNCHRO` mechanism (`PostMainThreadSynchro`), instead of calling `Idle()` directly from the worker thread.

## Implementation Tasks

### Task 1 — Modify `TKeepAliveThread::Execute()` to use `PostMainThreadSynchro`

**File:** `src/NetBox/WinSCPFileSystem.cpp`  
**Lines:** ~291–293 (inside `Execute()` loop)

**Before:**
```cpp
    if ((::WaitForSingleObject(FEvent, nb::ToDWord(FInterval.GetValue() * MSecsPerDay)) != WAIT_FAILED) &&
      !IsFinished())
    {
      FFileSystem->KeepaliveThreadCallback();
    }
```

**After:**
```cpp
    if ((::WaitForSingleObject(FEvent, nb::ToDWord(FInterval.GetValue() * MSecsPerDay)) != WAIT_FAILED) &&
      !IsFinished())
    {
      // Marshal keepalive processing to the main thread via the sanctioned synchro path.
      // Far Manager 3 requires all plugin-side idle work to happen on the main thread.
      // We post nullptr (triggers FE_IDLE for both panels) rather than a custom
      // TSynchroParams because FSynchroParams is shared mutable state and is not
      // thread-safe for concurrent background posts. FE_IDLE is harmless:
      // Terminal::Idle() checks Connected() and PingInterval internally, so extra
      // calls at the keepalive interval are no-ops if the interval hasn't elapsed.
      FFileSystem->GetPlugin()->PostMainThreadSynchro(nullptr);
    }
```

### Task 2 — Remove obsolete `KeepaliveThreadCallback()` method and header declaration

**Files:** `src/NetBox/WinSCPFileSystem.cpp` and `src/NetBox/WinSCPFileSystem.h`
**Lines:** ~350–365 (cpp), ~124 (h)

Delete the entire `TWinSCPFileSystem::KeepaliveThreadCallback()` method body in the `.cpp` file, and remove its declaration in the `.h` file. No other call sites exist.


### Task 3 — Build verification

Run `build-x64.bat` (or equivalent) and verify:
- Build succeeds
- Zero new warnings
- No link errors after removing `KeepaliveThreadCallback`

### Task 4 — Code review

- [ ] Confirm `FE_IDLE` handler (`ProcessPanelEventEx`) still calls `FTerminal->Idle()` when `Connected()`
- [ ] Verify no remaining direct background-thread calls to `Terminal->Idle()`
- [ ] Check that `PostMainThreadSynchro` is the only Far API call from the worker thread
- [ ] Verify `WinSCPFileSystem.h` no longer declares `KeepaliveThreadCallback()`

## Risks

- **Low:** `FE_IDLE` already runs every 400 ms via the global idle thread; posting it at the keepalive interval (typically ≥ 30 s) is harmless.
- **Low:** No functional change in keepalive behavior; only the thread executing the work changes.

## Verification

1. Open an SSH/SFTP session with `PingInterval` > 0.
2. Leave the panel idle for longer than the keepalive interval.
3. Confirm session stays alive (no NAT/firewall timeout).
4. Verify no crashes or hangs during idle periods.
