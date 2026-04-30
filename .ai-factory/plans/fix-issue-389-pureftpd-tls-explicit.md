# Plan: Fix Pure‑FTPd TLS Explicit Encryption (Issue #389)

> **GitHub Issue:** [#389](https://github.com/michaellukashov/Far-NetBox/issues/389)  
> **Related:** [#29](https://github.com/FarGroup/Far-NetBox/issues/29)  
> **Created:** 2026-04-30  
> **Mode:** fast  
> **Settings:** Testing=no, Logging=verbose, Docs=yes

## Problem Statement

Connecting to a Pure‑FTPd server with “TLS/SSL Explicit encryption” fails because the server rejects the `AUTH SSL` command with “500 This security scheme is not implemented”. The server expects `AUTH TLS`. The log shows:

```
. … TLS layer changed state from connecting to connected
. … Connected with pureftpd.example.org, negotiating TLS connection...
< … 220---------- Welcome to Pure-FTPd [privsep] [TLS] ----------
> … AUTH SSL
< … 500 This security scheme is not implemented
```

The client sends `AUTH SSL` because the session’s FTP encryption mode is mapped to `ftpsExplicitSsl` (`SERVER_FTP_SSL_EXPLICIT`), which triggers `FtpControlSocket.cpp` line 624 to send SSL instead of TLS. Pure‑FTPd supports `AUTH TLS` but not `AUTH SSL`.

## Root Cause Analysis

1. **UI mapping:** The dialog combo “TLS/SSL Explicit encryption” (index 2) maps to `ftpsExplicitSsl` via `TSessionDialog::IndexToFtps()`. There is no separate UI option for TLS‑explicit vs SSL‑explicit.
2. **Protocol mapping:** `ftpsExplicitSsl` → `SERVER_FTP_SSL_EXPLICIT` → `FZ_SERVERTYPE_LAYER_SSL_EXPLICIT`.  
   `ftpsExplicitTls` → `SERVER_FTP_TLS_EXPLICIT` → `FZ_SERVERTYPE_LAYER_TLS_EXPLICIT`.
3. **AUTH command selection:** `FtpControlSocket.cpp` lines 624‑636:
   - If `FZ_SERVERTYPE_LAYER_SSL_EXPLICIT` → send `AUTH SSL`, state `CONNECT_SSL_NEGOTIATE`.
   - Otherwise → send `AUTH TLS`, state `CONNECT_TLS_NEGOTIATE`.
4. **Fallback:** Only `CONNECT_TLS_NEGOTIATE` has a fallback to SSL (lines 645‑652). `CONNECT_SSL_NEGOTIATE` fails immediately.
5. **Pure‑FTPd compatibility:** The server announces `[TLS]` in its banner but the client sends SSL because the UI selected SSL‑explicit.

## Solution Approach

Modify `FtpControlSocket.cpp` to treat `FZ_SERVERTYPE_LAYER_SSL_EXPLICIT` the same as `FZ_SERVERTYPE_LAYER_TLS_EXPLICIT` for the purpose of AUTH command selection: send `AUTH TLS` first, and if the server replies with a non‑2xx/3xx code, fall back to `AUTH SSL`. This preserves compatibility with SSL‑only servers while fixing Pure‑FTPd and other TLS‑only servers.

**No UI changes required.** Existing saved sessions keep their `Ftps` value (`ftpsExplicitSsl`), but the new logic will try TLS first anyway.

## Tasks

### Phase I. Investigation & Logging

#### Task 1: Add diagnostic logging to `FtpControlSocket.cpp`
- File: `src/filezilla/FtpControlSocket.cpp`
- Insert at the start of the `CONNECT_SSL_INIT` handler (line 622, before the AUTH branch):
  ```cpp
  ShowStatus(FORMAT("Trying AUTH TLS for explicit encryption (serverType=0x%04X)",
    m_CurrentServer.nServerType), FZ_LOG_INFO);
  ```
- In the fallback path (line 647, inside `if (m_Operation.nOpState == CONNECT_TLS_NEGOTIATE)` before `SendAuthSsl()`):
  ```cpp
  ShowStatus(FORMAT("AUTH TLS rejected (code=%d), falling back to AUTH SSL", res), FZ_LOG_INFO);
  ```
- Use `ShowStatus()` with `FZ_LOG_INFO` level (maps to NetBox verbose logging).

#### Task 2: Verify UI mapping and session data
- File: `src/core/FtpFileSystem.cpp` (around line 525)
- Add a log statement before the `switch (Data->Ftps)` block in `TFTPFileSystem::Connect()`:
  ```cpp
  LogEvent(FORMAT("FTP encryption mode: Ftps=%d → ServerType=0x%04X",
    nb::ToInt32(Data->Ftps), ServerType));
  ```
- Verify that `ftpsExplicitSsl` (value 2) maps to `SERVER_FTP_SSL_EXPLICIT` (0x1200) and `ftpsExplicitTls` (value 3) maps to `SERVER_FTP_TLS_EXPLICIT` (0x1400).
- Confirm there is no hidden setting that could switch between SSL‑explicit and TLS‑explicit.

### Phase II. Core Fix

#### Task 3: Modify AUTH command selection logic
- File: `src/filezilla/FtpControlSocket.cpp` (lines 624‑636)
- **Simplification:** Remove the `if/else` branch entirely. Always send `AUTH TLS` and set state to `CONNECT_TLS_NEGOTIATE` for any explicit encryption server type (`SSL_EXPLICIT` or `TLS_EXPLICIT`). The existing fallback (lines 645‑652) already tries `AUTH SSL` when TLS fails.
- **Before:**
  ```cpp
  if (m_CurrentServer.nServerType & FZ_SERVERTYPE_LAYER_SSL_EXPLICIT)
  {
    if (!SendAuthSsl()) return;
  }
  else
  {
    if (!Send("AUTH TLS")) return;
    m_Operation.nOpState = CONNECT_TLS_NEGOTIATE;
  }
  ```
- **After:**
  ```cpp
  // TLS‑first for all explicit encryption modes. SSL‑only servers are handled
  // by the fallback in the CONNECT_TLS_NEGOTIATE handler (issue #389).
  if (!Send("AUTH TLS"))
    return;
  m_Operation.nOpState = CONNECT_TLS_NEGOTIATE;
  ```
- **Verification:** Confirm that `SendAuthSsl()` (called as fallback) correctly sets `m_Operation.nOpState = CONNECT_SSL_NEGOTIATE` (line 1202).
- **Regression check:** Ensure `ftpsExplicitTls` (index 3, `SERVER_FTP_TLS_EXPLICIT`) still works—this change is a no‑op for that path.
#### Task 4: Verify `ftpsExplicitTls` path compatibility
- Confirm that `SERVER_FTP_TLS_EXPLICIT` (0x1400, set when `ftpsExplicitTls` is selected programmatically or via legacy sessions) still enters `CONNECT_SSL_INIT` and receives `AUTH TLS`.
- This change is a no‑op for that path, but a quick code review or unit test should confirm the state machine is unchanged.

#### Task 5: Verify `SendAuthSsl()` fallback state transition
- Review `CFtpControlSocket::SendAuthSsl()` (line 1198) to confirm it sets `m_Operation.nOpState = CONNECT_SSL_NEGOTIATE` (line 1202).
- Ensure no early return path skips the state assignment.
- This is the critical safety check that makes the unconditional‑TLS approach work for SSL‑only servers.

### Phase III. Verification

#### Task 6: Build verification
- Build `RelWithDebugInfo` for x64 using `build-x64.bat`.
- Ensure zero MSVC W4 warnings.
- If unity‑build symbol conflicts arise, disable with `-DOPT_USE_UNITY_BUILD=OFF`.

#### Task 7: Manual test scenario
- Create a new FTP session with “TLS/SSL Explicit encryption”.
- Connect to a Pure‑FTPd server (or a mock server that rejects `AUTH SSL`).
- Verify that the client sends `AUTH TLS` (check logs).
- Verify that the connection proceeds successfully.
- Verify that an SSL‑only server still works (TLS fails, fallback to SSL succeeds).

### Phase IV. Documentation

#### Task 8: Update knowledge references
- File: `.ai-factory/references/INDEX.md`
- Add a link to this plan and a short summary of the fix.
- If a Pure‑FTPd‑specific note is needed, add a troubleshooting entry.

#### Task 9: Commit changes
- Use conventional commit message:
  ```
  fix(ftp): try AUTH TLS first for explicit SSL encryption
  
  Pure‑FTPd servers reject AUTH SSL with “500 This security scheme is not
  implemented”. Change FtpControlSocket.cpp to treat SSL‑explicit server
  type the same as TLS‑explicit for the initial AUTH command, sending
  AUTH TLS first and falling back to AUTH SSL only when TLS fails.
  
  This preserves compatibility with SSL‑only servers while fixing
  TLS‑only servers like Pure‑FTPd (GitHub issue #389).
  ```

## Architecture Notes

- **Layer:** FileZilla‑derived FTP core (`src/filezilla/`), not WinSCP facade.
- **No changes to `libs/`** — the fix is entirely within NetBox’s copy of FileZilla code.
- **Thread safety:** The FTP control socket runs on its own thread; changes are confined to that thread’s state machine.
- **Backward compatibility:** Saved sessions retain their `Ftps` value (`ftpsExplicitSsl`). The new behavior is a superset (tries TLS, then SSL), so existing SSL‑only servers continue to work.

## Edge Cases

1. **Server supports both TLS and SSL:** The TLS attempt will succeed, no fallback needed.
2. **Server supports only SSL:** TLS fails (likely 500 or 534), fallback to SSL succeeds.
3. **Server supports neither:** Both attempts fail, connection closes as before.
4. **Implicit SSL (`ftpsImplicit`):** Not affected; uses a different code path (SSL layer before any AUTH).
5. **UI combo index 3 (`ftpsExplicitTls`):** Currently unreachable via UI; if ever used, behavior unchanged (TLS first, SSL fallback).

## Acceptance Criteria

- [ ] Pure‑FTPd with explicit encryption connects successfully.
- [ ] Logs show `AUTH TLS` sent when server type is `SSL_EXPLICIT`.
- [ ] Logs show fallback to `AUTH SSL` when TLS fails (for SSL‑only servers).
- [ ] Existing SSL‑only servers still work (no regression).
- [ ] `ftpsExplicitTls` path verified (no regression for index 3).
- [ ] `SendAuthSsl()` fallback state transition verified.
- [ ] Build passes with zero warnings.
- [ ] Commit message follows conventional format.

## Changelog

| Date       | Change                                      | Reason |
|------------|---------------------------------------------|--------|
| 2026-04-30 | Initial plan                                | Issue #389 analysis |
| 2026-04-30 | Refined: unconditional TLS, exact log specs   | Code review (aif-improve) |
| 2026-04-30 | Added Tasks 4 & 5 (compatibility & fallback) | Safety verification gaps |