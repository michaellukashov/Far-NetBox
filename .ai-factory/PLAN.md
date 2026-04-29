# Enable TKeepAliveThread for Session Keepalives

**Branch:** none (fast plan)
**Created:** 2026-04-29
**Plan Type:** fast

## Settings

- **Testing:** yes — verify no crash on connect/disconnect, no deadlock during idle
- **Logging:** standard — key lifecycle events only
- **Docs:** no — warn-only

## Overview

The `TKeepAliveThread` class (`src/NetBox/WinSCPFileSystem.cpp` lines 247-296) is fully implemented but never instantiated. The `TODO` at line 3323 marks it as intentionally deferred ("Not finished nor used."). Enabling it moves keepalive pings to a background thread, matching WinSCP behavior and preventing protocol stalls during idle detection.

**Problems addressed:**
1. Keepalive pings are currently only done synchronously within protocol threads (SFTP/SCP/FTP), risking stalls
2. `TKeepAliveThread` exists with complete `Execute()`, `Terminate()`, and `InitKeepaliveThread()` but is dead code
3. `FKeepaliveThread` member is only destroyed (`SAFE_DESTROY` in `Close()`) but never created

**Out of scope:**
- Keepalive interval UI controls (already in SessionData/Terminal)
- Ping type logic (dummy command vs TCP keepalive)

## Tasks

### Task 1: Instantiate TKeepAliveThread after successful connect ✅

**File:** `src/NetBox/WinSCPFileSystem.cpp`

1. Replace the `TODO` at line 3323 with thread creation:
   ```cpp
   const TDateTime Interval = FTerminal->GetSessionData()->GetPingIntervalDT();
   if (Interval.GetValue() > 0)
   {
     FKeepaliveThread = new TKeepAliveThread(this, Interval);
     FKeepaliveThread->InitKeepaliveThread();
   }
   ```

2. Ensure `FKeepaliveThread` is `gsl::owner<TKeepAliveThread *>` (already declared in header line 338).

**Acceptance Criteria:**
- Code compiles with zero warnings
- Thread starts after `FTerminal->GetActive()` succeeds
- No thread creation when `PingInterval == 0` (keepalives disabled)
- `FKeepaliveThread` remains `nullptr` if terminal connect fails

**Logging:**
- DEBUG: "Keepalive thread created, interval=%.1fs"
- DEBUG: "Keepalive thread skipped, interval=0"

---

### Task 2: Guard against double-start and ensure safe shutdown order ✅

**File:** `src/NetBox/WinSCPFileSystem.cpp`

1. In `Close()` (line 386), ensure `SAFE_DESTROY(FKeepaliveThread)` happens BEFORE `FQueue` / `FQueueStatus` teardown, since the thread may call `KeepaliveThreadCallback` which touches `FTerminal`.

2. Verify `TKeepAliveThread::Terminate()` signals `FEvent` and `Execute()` exits cleanly before `SAFE_DESTROY` deletes the object.

3. Add null guard before `InitKeepaliveThread()` to prevent double-start on reconnect.

**Acceptance Criteria:**
- No use-after-free when `Close()` is called while keepalive thread is active
- Thread terminates within 2× interval seconds
- No crash on rapid connect/disconnect cycles

**Logging:**
- DEBUG: "Keepalive thread terminating"
- DEBUG: "Keepalive thread destroyed"

---

### Task 3: Verify TKeepAliveThread::Execute is thread-safe ✅

**File:** `src/NetBox/WinSCPFileSystem.cpp`

1. Review `KeepaliveThreadCallback` (line 350): it acquires `GetCriticalSection()` and calls `FTerminal->Idle()`.
2. Confirm `FTerminal->Idle()` is safe from background threads (does not call Far Manager APIs).
3. If `Idle()` may throw, wrap in try/catch to prevent thread crash.

**Acceptance Criteria:**
- `Execute()` does not crash on `Idle()` exceptions
- No deadlock between keepalive thread and protocol worker threads
- ThreadSanitizer/AddressSanitizer clean (if available)

**Logging:**
- WARN: "Keepalive thread caught exception: %s"

---

### Task 4: Manual integration test ✅

**File:** `tests/integration/test_keepalive.md` (new, manual test plan)

1. Connect to SFTP server with PingInterval = 30s
2. Leave session idle for 60s
3. Verify session remains connected (no timeout/disconnect)
4. Disconnect and verify no crash
5. Repeat with PingInterval = 0 (keepalive disabled) — verify no thread created

**Acceptance Criteria:**
- Session stays alive with keepalives enabled
- No crash on disconnect
- No regression in file transfer operations

---

## Commit Plan

Single commit at completion:

```
feat(netbox): enable TKeepAliveThread for background session keepalives

- Instantiate TKeepAliveThread after successful terminal connect
  using SessionData->GetPingIntervalDT()
- Remove deferred TODO marking thread as unfinished
- Ensure SAFE_DESTROY runs before queue/terminal teardown
- Add exception guard in Execute() to prevent thread crash
- Add lifecycle logging for debug diagnostics
```
