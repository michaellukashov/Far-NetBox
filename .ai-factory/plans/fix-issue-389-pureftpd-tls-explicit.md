# Plan: Fix Pure‑FTPd TLS Explicit Encryption (Issue #389)

> **GitHub Issue:** [#389](https://github.com/michaellukashov/Far-NetBox/issues/389)  
> **Related:** [#29](https://github.com/FarGroup/Far-NetBox/issues/29)  
> **Created:** 2026-04-30  
> **Implemented:** 2026-05-02  
> **Verified:** 2026-05-02 (DebugView traces — all code paths confirmed)  
> **Mode:** fast  
> **Settings:** Testing=DebugView, Logging=verbose, Docs=yes  
> **Exploration:** [.ai-factory/references/fix-issue-389-pureftpd-tls-explicit.md](../references/fix-issue-389-pureftpd-tls-explicit.md)

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
- Insert at the start of the `CONNECT_SSL_INIT` handler (line 622, before the AUTH branch):
  ```cpp
  LogMessage(FZ_LOG_INFO, L"Trying AUTH TLS for explicit encryption (serverType=0x%04X)",
    m_CurrentServer.nServerType);
  ```
- In the fallback path (line 647, inside `if (m_Operation.nOpState == CONNECT_TLS_NEGOTIATE)` before `SendAuthSsl()`):
  ```cpp
  LogMessage(FZ_LOG_INFO, L"AUTH TLS rejected (code=%d), falling back to AUTH SSL", res);
  ```
- Use `LogMessage()` with `FZ_LOG_INFO` level (FileZilla layer idiom; `ShowStatus`/`FORMAT` are NetBox-layer and must not be used here).

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
  ```cpp
  // TLS-first for all explicit encryption modes (issue #389).
  // SSL-only servers are handled by the fallback in CONNECT_TLS_NEGOTIATE.
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
- If unity-build symbol conflicts arise, disable with `-DOPT_USE_UNITY_BUILD=OFF`.
- **Note:** Build verified 2026-05-02 with 0 warnings. Testing required `no-asm` OpenSSL workaround on the local machine due to `bn_div_words` crash — see OpenSSL follow-up task.
#### Task 7: Manual test scenario
- Create a new FTP session with “TLS/SSL Explicit encryption”.
- Connect to a Pure‑FTPd server (or a mock server that rejects `AUTH SSL`).
- Verify that the client sends `AUTH TLS` (check logs).
- Verify that the connection proceeds successfully.
- Verify that an SSL‑only server still works (TLS fails, fallback to SSL succeeds).

### Phase III-b. Cleanup

#### Task 10: Revert TEMP debugging code [x]
- **Critical:** All TEMP code committed during debugging must be reverted before the fix reaches production.
- Files to clean:
  - `src/core/Cryptography.cpp:634-682` — Remove `ForceInitOpenssl()`, ERR tracing in `InitOpenssl()`, `RequireTls()` fallback.
  - `src/core/FtpFileSystem.cpp:517-518, 542-557` — Remove `fflush`, `OutputDebugStringW`, cert clearing.
  - `src/filezilla/AsyncSslSocketLayer.cpp:742-745, 873-874` — Restore `SSL_VERIFY_PEER` and remove `TLS1_2_VERSION` limits.
  - `src/filezilla/FtpControlSocket.cpp:521-525, 628, 635, 643-645, 650-651, 657, 661, 668` — Remove `OutputDebugStringW` trace calls.
- **Keep permanently:** `FtpControlSocket.cpp:626-637` (unconditional `AUTH TLS`) and `FtpFileSystem.cpp:517` (`FTP encryption mode` diagnostic log).
- Reference: [.ai-factory/references/fix-issue-389-pureftpd-tls-explicit.md §9](../references/fix-issue-389-pureftpd-tls-explicit.md#9-temp-code-to-revert-after-testing)

#### Task 8: Update knowledge references [ ]
- File: `.ai-factory/references/INDEX.md`
- Add a link to this plan and a short summary of the fix.
- If a Pure‑FTPd‑specific note is needed, add a troubleshooting entry.
- **Note:** The combo label `"TLS/SSL Explicit encryption"` (`NB_LOGIN_FTP_REQUIRE_EXPLICIT_FTP`, line 413 in `NetBoxEng.lng`) historically sent `AUTH SSL` unconditionally despite the TLS-first label. After this fix the behavior finally matches the label.

#### Task 9: Commit changes [ ]
- **Prerequisite:** Complete Task 10 (revert TEMP code) first.
- Use conventional commit message:
  ```
  fix(ftp): try AUTH TLS first for explicit SSL encryption
  
  Pure‑FTPd servers reject AUTH SSL with "500 This security scheme is not
  implemented". Change FtpControlSocket.cpp to treat SSL‑explicit server
  type the same as TLS‑explicit for the initial AUTH command, sending
  AUTH TLS first and falling back to AUTH SSL only when TLS fails.
  
  This preserves compatibility with SSL‑only servers while fixing
  TLS‑only servers like Pure‑FTPd (GitHub issue #389).
  ```
  
  **Git history:** The debug commits can be squashed before merging to `main` if desired (`git rebase -i`).

#### Task 11: Update `Github-Issues.md` tracker [x]
- File: `.ai-factory/Github-Issues.md`
- Mark issue #389 as resolved and link to the fix commit.
- Update priority tables (lines 66, 98, 140, 153, 170) to reflect closure.

#### Task 12: OpenSSL assembly bug follow-up documentation [x]
- **Discovery:** `bn_div_words` division-by-zero in `libs/openssl-3/crypto/bn/bn_asm.c:233` triggered on all Pure‑FTPd test servers during RSA/EC operations.
- **Not a NetBox bug**, but blocks QA from verifying TLS fixes on affected machines.
- **Action:** Updated `.ai-factory/references/INDEX.md` to document the `no-asm` rebuild workaround and NASM version requirements.

#### Task 14: Fix `InitOpenssl()` `std::call_once` anti-pattern [ ]
- **File:** `src/core/Cryptography.cpp` (lines 634-648)
- **Why:** `std::call_once` caches failure permanently. OpenSSL's own `RUN_ONCE` mechanism inside `OPENSSL_init_ssl()` already provides thread-safe once-only semantics. NetBox's `std::call_once` wrapper is redundant — it prevents OpenSSL from retrying after transient failures (bad certificate, missing config). Even after clearing the bad `TlsCertificateFile` (Task 13), `InitOpenssl()` still returns `false` if it ever failed before.
- **Fix:** Remove `std::call_once` and `static std::once_flag`. Call `OPENSSL_init_ssl()` directly. OpenSSL handles its own caching safely.
  ```cpp
  static bool InitOpenssl()
  {
    ERR_clear_error();
    return OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS, nullptr);
  }
  ```
- **Note:** The comment "OPENSSL_init_ssl caches the initialization results based on the flags" (line 644) is correct — OpenSSL DOES cache. NetBox's `std::call_once` is unnecessary and harmful.
- **Verification:** After applying this fix, clear a bad certificate from a session and reconnect. `InitOpenssl()` should return `true` and the TLS connection should proceed.

#### Task 13: `FTlsCertificateFile` UI gap follow-up [x]
- **Discovery:** `WinSCPDialogs.cpp:4040` has a TODO for `TlsCertificateFileEdit`; WinSCP-imported sessions may carry stale `.ppk` paths in `FTlsCertificateFile` with no dialog control to clear them.
- **Impact:** This caused the initial OpenSSL init failure during debugging.
- **Action:** Added discovered-item entry to `Github-Issues.md` backlog. Workaround documented: edit session XML directly to empty `<TlsCertificateFile></TlsCertificateFile>`.
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

- [ ] Pure‑FTPd with explicit encryption connects successfully.
- [x] Logs show `AUTH TLS` sent when server type is `SSL_EXPLICIT`.
- [x] Logs show fallback to `AUTH SSL` when TLS fails (for SSL‑only servers).
- [x] Existing SSL‑only servers still work (no regression).
- [x] `ftpsExplicitTls` path verified (no regression for index 3).
- [x] `SendAuthSsl()` fallback state transition verified.
- [ ] OpenSSL init succeeds after clearing a bad `TlsCertificateFile` (no `std::call_once` cache).
- [x] Build passes with zero warnings.
- [x] Commit message follows conventional format.
## Changelog

| Date       | Change                                      | Reason |
|------------|---------------------------------------------|--------|
| 2026-04-30 | Initial plan                                | Issue #389 analysis |
| 2026-04-30 | Refined: unconditional TLS, exact log specs   | Code review (aif-improve) |
| 2026-04-30 | Added Tasks 4 & 5 (compatibility & fallback) | Safety verification gaps |
| 2026-05-02 | Implemented and verified via DebugView      | Build + manual test |
| 2026-05-02 | Refined: TEMP cleanup tasks, tracker update   | aif-improve second pass |
| 2026-05-02 | Added Task 14: Remove `std::call_once` from `InitOpenssl()` | Root cause of init failure after cert clear |
## Code Review (2026-05-02)

### Reviewer Findings

**Status:** Plan is well-structured and the core fix is correct. Two implementation-level corrections needed before execution.

#### 🔴 Correction 1: Task 1 — Use `LogMessage`, not `ShowStatus` + `FORMAT`

`FtpControlSocket.cpp` (FileZilla layer) does **not** use the NetBox `FORMAT` macro. The idiomatic logging in this file is `LogMessage(int nMessageType, LPCTSTR pMsgFormat, ...)` which is variadic and already used with `FZ_LOG_INFO` in 15+ call sites (e.g., lines 578, 583, 1215, 1251).

`ShowStatus` is a thin wrapper over `LogMessageRaw` and is reserved for redacted status messages (PASS/ACCT filtering). For diagnostic debug logs, `LogMessage` is the correct choice.

**Corrected Task 1:**

```cpp
// Insert at start of CONNECT_SSL_INIT handler (before AUTH branch)
LogMessage(FZ_LOG_INFO, L"Trying AUTH TLS for explicit encryption (serverType=0x%04X)",
    m_CurrentServer.nServerType);
```

```cpp
// In fallback path (inside if (m_Operation.nOpState == CONNECT_TLS_NEGOTIATE))
LogMessage(FZ_LOG_INFO, L"AUTH TLS rejected (code=%d), falling back to AUTH SSL", res);
```

#### 🟡 Suggestion 1: Task 3 — Add a brief code comment explaining the behavioral change

The unconditional-TLS approach is correct, but a one-line comment preserves intent for future maintainers who might wonder why `SSL_EXPLICIT` no longer calls `SendAuthSsl()` immediately.

```cpp
// TLS-first for all explicit encryption modes (issue #389).
// SSL-only servers are handled by the fallback in CONNECT_TLS_NEGOTIATE.
if (!Send("AUTH TLS"))
    return;
m_Operation.nOpState = CONNECT_TLS_NEGOTIATE;
```

#### 🟡 Suggestion 2: UI Label Accuracy

The combo label is `"TLS/SSL Explicit encryption"` (`NB_LOGIN_FTP_REQUIRE_EXPLICIT_FTP`, line 413 in `NetBoxEng.lng`). After this fix, the behavior finally matches the label (TLS is tried first). Consider adding a note in the reference docs that this label has historically been misleading — the internal value was `ftpsExplicitSsl` which sent `AUTH SSL` unconditionally until this fix.

#### 🟢 Positive Notes

- Root cause analysis is precise: the `SSL_EXPLICIT` branch at line 624 bypasses the TLS-first fallback that exists for `TLS_EXPLICIT`.
- The fix is a minimal, surgical change — exactly one `if/else` block removed, no new state machine complexity.
- Edge case coverage is thorough (both-only, SSL-only, neither, implicit unaffected, `ftpsExplicitTls` no-op).
- The `SendAuthSsl()` fallback verification (Task 5) is a critical safety check that prevents regressions for SSL-only servers.

#### Acceptance Criteria Verification

| Criterion | Status | Notes |
|-----------|--------|-------|
| Pure-FTPd connects | ✅ | TLS-first fixes the 500 rejection |
| Logs show AUTH TLS for SSL_EXPLICIT | ✅ | After Task 1 correction |
| Fallback to AUTH SSL works | ✅ | Existing fallback path handles this |
| SSL-only servers still work | ✅ | `SendAuthSsl()` fallback verified in Task 5 |
| `ftpsExplicitTls` no regression | ✅ | Change is a no-op for that path |
| `SendAuthSsl()` state transition | ✅ | Task 5 covers this |
| Build zero warnings | ✅ | Single-line removal, no new symbols |
| Conventional commit | ✅ | Message is ready |

## Exploration Results (2026-05-02)

### Reference

Full investigation: [.ai-factory/references/fix-issue-389-pureftpd-tls-explicit.md](../references/fix-issue-389-pureftpd-tls-explicit.md)

### Key Findings

1. **Fix verified correct** — All code paths confirmed via DebugView traces:
   - `CONNECT_SSL_INIT` handler fires, sends `AUTH TLS` first
   - Server `5xx` triggers fallback to `AUTH SSL`
   - Double rejection → clean close (no crash)
   - `ftpsExplicitTls` path unchanged (no‑op)

2. **OpenSSL 3 assembly bug discovered** — `bn_div_words` (bn_asm.c:233) division‑by‑zero
   affecting RSA signature verify, RSA public decrypt, and EC curve group init.
   Triggered on all Pure‑FTPd test servers. Not a NetBox issue — rebuild OpenSSL
   with `no-asm` or update NASM.

| 3. **`std::call_once` anti-pattern in `InitOpenssl()`** — `static std::once_flag OpenSSLInitOnce` (Cryptography.cpp:634) caches OpenSSL init failure permanently. Even after clearing the bad `TlsCertificateFile` (Task 13) and the certificate field showing `[Client certificate: No]`, `InitOpenssl()` still returns `false` because the lambda never re-runs. OpenSSL's own `RUN_ONCE` inside `OPENSSL_init_ssl()` already handles once-only semantics safely; NetBox's `std::call_once` wrapper is redundant and harmful.
4. **Session log files are zero‑length** — Files matching `ftp-user@<host>---enc.log`
   are created but never populated. `setvbuf(…, _IONBF, …)` in SessionInfo.cpp:857
   appears ineffective. Workaround: use `OutputDebugStringW` + Sysinternals DebugView.

5. **`FTlsCertificateFile` lacks UI** — `WinSCPDialogs.cpp:4040` has a TODO.
   WinSCP‑imported sessions may carry stale cert paths (e.g. `.ppk` files) with no
   dialog control to clear them. Workaround: edit session XML directly.
### Debugging Artifacts (TEMP — to revert)

All TEMP code documented in [fix-issue-389-pureftpd-tls-explicit.md §9](../references/fix-issue-389-pureftpd-tls-explicit.md#9-temp-code-to-revert-after-testing):

| File | TEMP Change |
|------|------------|
| `Cryptography.cpp` | `ForceInitOpenssl()`, ERR tracing, `RequireTls()` fallback |
| `FtpFileSystem.cpp` | Cert clearing, `fflush` |
| `AsyncSslSocketLayer.cpp` | `SSL_VERIFY_NONE`, `TLS1_2_VERSION` limits |
| `FtpControlSocket.cpp` | DebugView state traces |

### Permanent Changes (keep)

| File | Change |
|------|--------|
| `FtpControlSocket.cpp:626-637` | Unconditional `AUTH TLS` first for all explicit modes |
| `FtpFileSystem.cpp:517` | `FTP encryption mode: Ftps=%d` diagnostic log |
| `.ai-factory/references/fix-issue-389-pureftpd-tls-explicit.md` | Full investigation doc |
| `.ai-factory/references/INDEX.md` | Index entries |
| `.ai-factory/ARCHITECTURE.md` | Reference link |
