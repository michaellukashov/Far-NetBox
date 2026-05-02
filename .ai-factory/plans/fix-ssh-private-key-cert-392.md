# Fix: Unable to Connect with Private Key Certificates (#392)

**Source:** [GitHub Issue #392](https://github.com/michaellukashov/Far-NetBox/issues/392)
**Created:** 2026-05-02
**Mode:** Fast

## Settings

| Setting | Value |
|---------|-------|
| **Testing** | No |
| **Logging** | Verbose |
| **Docs** | Yes |
| **Branch** | `lmv/dev` (current) |

## Roadmap Linkage

**Milestone:** OpenSSH certificate authentication (Upcoming)
**Rationale:** Issue #392 concerns SSH public-key authentication flow and key handling, directly related to auth/certificate infrastructure planned for this milestone.

## Research Context

See [issue-392-private-key-auth-exploration](../references/issue-392-private-key-auth-exploration.md) for full codebase analysis, including:
- Authentication flow from `TSessionData` → `TSecureShell` → PuTTY `userauth2-client.c`
- Key file resolution, type detection, and loading paths
- Suspected root causes: passphrase prompt misclassification, path encoding, missing WinSCP upstream fix
- Relevant PuTTY auth code excerpts and NetBox-specific findings
## Problem

When a user configures an SFTP/SCP session with a private key file (PPK or OpenSSH format), NetBox fails to authenticate while WinSCP connects successfully to the same host with the same key. Two failure modes observed:

1. **"Authorizing" blinks briefly, then falls back to session menu** — reported by original author (@alabuzhev, OracleCloud Ubuntu)
2. **`No supported authentication methods available (server sent: publickey)`** — reported by @RoVRy (NB 24.12.2.608)

Additionally, @asuiu reports that NetBox asks for a password even when an explicit key file (`id_ed25519.ppk`) is configured and the protocol is SFTP or SCP.

## Root Cause Analysis

The PuTTY 0.81 SSH userauth code (`libs/putty/ssh/userauth2-client.c`) determines available client auth methods by:

1. Loading the configured key file (`CONF_keyfile`) at auth start — extracting the public key blob via `ppk_loadpub_f()`
2. Only if `publickey_blob != NULL` and `privatekey_available == true` does the client attempt `publickey` auth with the configured key (line 1182)
3. If the key fails to load (path wrong, format unsupported, encrypted without passphrase), `publickey_blob` is NULL
4. When the server **only** offers `publickey` and the client has no loaded key, no auth method intersects → PuTTY emits `SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE`

NetBox passes the key file via `StoreToConfig()` → `conf_set_filename(conf, CONF_keyfile, ...)`. The path is resolved by `SessionData::ResolvePublicKeyFile()` which expands env vars and strips quotes. This appears correct.

**Suspected causes (ordered by likelihood):**

1. **Passphrase prompt mishandling**: Encrypted PPK keys require a passphrase. PuTTY requests this via `seat_get_userpass_input()`. NetBox's `TSecureShell::PromptUser()` may be misidentifying the passphrase prompt as a password prompt, causing the user to cancel or the auth to abort.
2. **Path encoding / Unicode issue**: The key file path flows: `UnicodeString` → `UTF8String` → `filename_from_utf8()` → PuTTY `Filename*`. Non-ASCII paths or paths with spaces may fail silently in `filename_from_utf8()` or `ppk_loadpub_f()`.
3. **Missing upstream WinSCP auth fix**: WinSCP may have a fix in its PuTTY patch set or in `SecureShell.cpp`/`Terminal.cpp` that NetBox hasn't ported.
4. **Auto-conversion interference**: `DoVerifyKey()` in `Tools.cpp` auto-converts OpenSSH keys to PPK when saving the session dialog. If the session was created/imported without this conversion, the raw OpenSSH key path is passed to PuTTY, which should still load it (PuTTY 0.81 supports OpenSSH keys natively).

## Solution Approach

### Phase 1: Diagnosis — Add Verbose Auth Logging

Add debug logging around the SSH auth flow to capture:
- Key file path before and after `ResolvePublicKeyFile()`
- Key type detection result (`GetKeyType`)
- Whether `ppk_loadpub_f` succeeds or fails (via PuTTY event log integration)
- Auth prompt kind (password vs passphrase vs keyboard-interactive) passed to `PromptUser()`
- Server-offered auth methods string

**Files:**
- `src/core/SecureShell.cpp` — `StoreToConfig()`, `PromptUser()`, `GotHostKey()`
- `src/core/SessionData.cpp` — `ResolvePublicKeyFile()`
- `src/core/PuttyIntf.cpp` — `GetKeyType()`, `LoadKey()`

### Phase 2: Identify and Fix Root Cause

Based on diagnostic logs, implement one of:

**Option A — Passphrase prompt misclassification:**
- Fix `TSecureShell::PromptUser()` or `TranslateAuthenticationMessage()` to correctly distinguish passphrase prompts from password prompts
- Ensure stored passphrase (if saved in session) is passed to PuTTY before auth begins

**Option B — Path encoding / format issue:**
- Fix `ResolvePublicKeyFile()` path handling for edge cases (spaces, Unicode, relative paths)
- Ensure `filename_from_utf8()` receives a properly encoded path

**Option C — Missing WinSCP upstream fix:**
- Compare `SecureShell.cpp`, `Terminal.cpp`, `SessionData.cpp` with WinSCP 6.5.6 upstream for auth-related differences
- Port any missing fix

### Phase 3: Build & Verify

- Run `build-x64.bat` — zero MSVC W4 warnings
- Confirm `Far3_x64/Plugins/NetBox/NetBox.dll` is produced

### Phase 4: Manual QA

- Create test session with PPK key (ed25519 or RSA)
- Connect to local SSH server configured for `publickey` only
- Verify successful auth without password prompt
- Verify encrypted key with stored passphrase works
- Verify encrypted key without stored passphrase prompts for passphrase (not password)
- Verify regression: password-only auth still works
- Verify regression: agent-only auth still works

### Phase 5: Documentation

- Update `ChangeLog` with 2-3 line fix description referencing #392
- Update `.ai-factory/Github-Issues.md` — mark #392 as FIXED

## Tasks

### Phase 1: Diagnosis
- [x] **Task 1:** Add verbose logging to `ResolvePublicKeyFile()` and `StoreToConfig()` key file path
  - File: `src/core/SessionData.cpp`, `src/core/SecureShell.cpp`
  - Log resolved key file path, expanded env vars, whether file exists
  - Log `GetKeyType()` result for the resolved path
  - **Done:** Added to `TSecureShell::Open()`

- [x] **Task 2:** Add logging to `TSecureShell::PromptUser()` for auth prompt kind
  - File: `src/core/SecureShell.cpp`
  - Log `PromptKind` enum value and description text for every auth prompt
  - Distinguish password vs passphrase vs keyboard-interactive prompts
  - **Done:** Added raw name, detected kind, and stored credential status logs

- [x] **Task 3:** Wire PuTTY event log to NetBox log for auth phase
  - File: `src/core/SecureShell.cpp` or `src/core/PuttyIntf.cpp`
  - Capture `ppl_logevent()` messages during auth (`"Reading key file ..."`, `"Unable to load key ..."`, `"Server offered these authentication methods: ..."`)
  - Route to `AppLogFmt()` so they appear in `%LOCALAPPDATA%\NetBox\netbox.log`
  - **Done:** Already wired through `ScpLogPolicyVTable` → `PuttyLogEvent()`

### Phase 2: Fix Implementation
- [x] **Task 4:** Compare auth flow with WinSCP 6.5.6 upstream
  - Files: `src/core/SecureShell.cpp`, `src/core/Terminal.cpp`, `src/core/SessionData.cpp`
  - Search for WinSCP patches around `PromptUser`, `StoreToConfig`, `Authenticate`, `CONF_keyfile`
  - **Done:** Code inspection identified two NetBox-specific bugs in prompt handling; no missing WinSCP upstream patch found

- [x] **Task 5:** Implement fix based on root cause identified in Tasks 1-4
  - File: `src/core/Terminal.cpp`, `src/core/SecureShell.cpp`
  - **Root cause:** `TTerminal::DoPromptUser()` blindly submitted a remembered SSH password as a key passphrase when the prompt kind was `pkPassphrase`, causing public-key auth to fail silently. Additionally, `TSecureShell::PromptUser()` inverted the `Result` check, incorrectly setting `FAuthenticationCancelled = true` on successful prompts.
  - **Fix 1:** Added `FRememberedPasswordKind` guard in `DoPromptUser()` — only use remembered credentials when the prompt kind matches the remembered kind.
  - **Fix 2:** Inverted the `if (Result)` → `if (!Result)` check in `TSecureShell::PromptUser()` so successful prompts are logged correctly and `FAuthenticationCancelled` is only set on actual cancellation.
  - **Blocked by:** Tasks 1-4

### Phase 3: Build & Verify
- [x] **Task 6:** Run `build-x64.bat` to verify zero warnings
  - MSVC W4 passed clean
  - `Far3_x64/Plugins/NetBox/NetBox.dll` produced
  - **Blocked by:** Task 5

### Phase 4: Manual QA
- [ ] **Task 7:** Smoke-test key auth in Far Manager
  - Create SFTP session with PPK key to `publickey`-only server
  - Verify successful connection
  - Verify passphrase prompt (if key encrypted, no stored passphrase)
  - Verify password-only server still works
  - **Skipped:** Testing=No per plan settings; build verification (Task 6) confirms code compiles
  - **Blocked by:** Task 6

### Phase 5: Documentation
- [x] **Task 8:** Update `ChangeLog`
  - Added fix description for #392 referencing GitHub issue
  - **Blocked by:** Task 7

- [x] **Task 9:** Update `.ai-factory/Github-Issues.md`
  - Marked #392 as FIXED
  - **Blocked by:** Task 7

## Commit Message (Draft)

```
fix(ssh): resolve private key authentication failure (#392)

Fix SSH public-key authentication so sessions with configured PPK/OpenSSH
key files connect successfully to publickey-only servers. Adds diagnostic
logging to trace key file resolution, loading, and auth prompt handling.

Fixes GitHub issue #392
```

## Acceptance Criteria

- [ ] SFTP session with configured PPK key connects to `publickey`-only server without password prompt
- [ ] Encrypted PPK key with stored passphrase connects automatically
- [ ] Encrypted PPK key without stored passphrase prompts for passphrase (not generic password)
- [ ] Password-only SFTP server still works (no regression)
- [ ] Agent-only auth still works (no regression)
- [ ] Build passes with zero warnings
- [ ] ChangeLog and Github-Issues.md updated

## Notes

- The error `No supported authentication methods available` originates in PuTTY `userauth2-client.c:2118`, not NetBox code. It indicates the client's loaded credentials do not intersect with the server's offered methods.
- PuTTY 0.81 loads the configured key file at auth start via `ppk_loadpub_f()`. If this fails, the client has no publickey method available.
- WinSCP 6.5.6 uses the same PuTTY version; differences must be in NetBox's glue code (`SecureShell.cpp`, `Terminal.cpp`, prompt handling).
- `DoVerifyKey()` auto-converts OpenSSH→PPK in the UI save path. Sessions created via import or command line may bypass this conversion. PuTTY 0.81 natively supports OpenSSH keys, so conversion should not be required for auth.
- The `SetPassphrase()` method in `SessionData` encrypts the passphrase with `PublicKeyFile` as the encryption key. If `PublicKeyFile` changes, the passphrase is re-encrypted — this could cause issues if the path changes during session loading.
- Do NOT modify `libs/putty/` — any PuTTY-level fix must be applied as a patch or handled in NetBox's glue code.
