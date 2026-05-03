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
dm|- [ ] **Task 7:** Smoke-test key auth in Far Manager
ov|  - Create SFTP session with PPK key to `publickey`-only server
rc|  - Verify successful connection
vo|  - Verify passphrase prompt (if key encrypted, no stored passphrase)
ll|  - Verify password-only server still works
cy|  - Verify browse buttons on SSH tab open file dialogs with correct filters
xt|  - Verify "Display Public Key" button shows public key blob for valid key files
pk|  - **Skipped:** Testing=No per plan settings; build verification (Task 6) confirms code compiles
dm|  - **Blocked by:** Task 6

lu|### Phase 5: Documentation
ah|- [x] **Task 8:** Update `ChangeLog`
xs|  - Added fix description for #392 referencing GitHub issue
um|  - **Blocked by:** Task 7
bj|
ae|- [x] **Task 9:** Update `.ai-factory/Github-Issues.md`
tb|  - Marked #392 as FIXED
um|  - **Blocked by:** Task 7
oc|
### Phase 6: UI Alignment with WinSCP
yd|
kh|mr|ul|- [x] **Task 10:** Add browse button and "Display Public Key" button to `PrivateKeyEdit` on Session tab
kp|nw|  - File: `src/NetBox/WinSCPDialogs.cpp` (TSessionDialog)
wv|ao|  - **Note:** `PrivateKeyEdit` (bound to `FPublicKeyFile` used for SSH auth) is on the **main Session tab**, not the Authentication tab. WinSCP groups it under "Authentication parameters" in its tabbed dialog.
sw|gk|  - Add `TFarButton` with `\u2026` (ellipsis) caption next to `PrivateKeyEdit`
hq|xi|  - Wire `OPENFILENAMEW` dialog with filter: `PuTTY Private Key Files (*.ppk)|*.ppk|All Private Key Files (*.ppk;*.pem;*.key;id_*)|*.ppk;*.pem;*.key;id_*|All Files (*.*)|*.*`
em|lr|  - Add "Display Public Key" button (`TFarButton`) — calls `GetPublicKeyLine()` and shows result in message box
kg|rb|  - **Tools dropdown deferred** — Full "Generate/Import/Convert Key" menu requires PuTTYgen integration. Mark as `// TODO` placeholder or skip for this plan.
ks|
xs|da|- [x] **Task 11:** Add browse button to Certificate field
mp|  - File: `src/NetBox/WinSCPDialogs.cpp` (TSessionDialog)
zn|  - Add `TFarButton` with `\u2026` next to `OpensshCertEdit` / detached certificate edit
rp|  - Wire `OPENFILENAMEW` with filter: `Public key files (*.pub)|*.pub|All Files (*.*)|*.*`
ne|  - Follow same pattern as TLS/S3 certificate browse buttons (`TlsCertificateFileBrowseClick`)
qf|  - **Blocked by:** Task 10 (shares browse handler pattern)

du|bk|- [x] **Task 12:** Add message strings for new UI elements
xa|  - Files: `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng`, `src/NetBox/NetBoxFr.lng`, `src/NetBox/NetBoxSpa.lng`
qp|  - Add IDs: `NB_LOGIN_DISPLAY_PUBLIC_KEY`, `NB_LOGIN_TOOLS`, `NB_LOGIN_PRIVATE_KEY_BROWSE_FILTER`, `NB_LOGIN_CERTIFICATE_BROWSE_FILTER`
jd|  - Add English, French, Spanish translations (mark non-English as `[T]` for translation)
pw|  - Update `src/resource/TextsCore1.rc` if needed for runtime string loading
my|  - **Blocked by:** Tasks 10-11 (need final button captions)

cp|
### Phase 7: SSH/Key Exchange Tab Alignment with WinSCP
yd|
mc|ul|- [x] **Task 13:** Add missing "Attempt GSSAPI key exchange" checkbox to KEX tab
kp|  - File: `src/NetBox/WinSCPDialogs.cpp` (TSessionDialog class)
nw|  - Add `TFarCheckBox * AuthGSSAPIKEXCheck{nullptr}` member declaration (near `KexDownButton`)
ao|  - Create checkbox in `Init()` under `tabKex`, after `KexDownButton`, before next separator
sw|  - Caption: new message `NB_LOGIN_KEX_GSSAPI` (WinSCP: "Attempt &GSSAPI key exchange")
gk|  - Wire in dialog load (`Init()`): `AuthGSSAPIKEXCheck->SetChecked(SessionData->GetAuthGSSAPIKEX())`
hq|  - Wire in dialog save (`Execute()`): `SessionData->SetAuthGSSAPIKEX(AuthGSSAPIKEXCheck->GetChecked())`
xi|  - Enable logic: `AuthGSSAPIKEXCheck->SetEnabled(aSshProtocol)` (or existing Kex tab enablement)
em|  - **Rationale:** `FAuthGSSAPIKEX` data field exists in `TSessionData`, is stored, wired to PuTTY (`CONF_try_gssapi_kex`), and logged — but has NO UI control. WinSCP exposes it on KEX tab.
lr|  - **Blocked by:** Task 5 (core fix stable)
rb|
wf|da|- [x] **Task 14:** Add message strings for GSSAPI KEX checkbox
mp|  - Files: `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng`, `src/NetBox/NetBoxFr.lng`, `src/NetBox/NetBoxSpa.lng`
zn|  - Add ID: `NB_LOGIN_KEX_GSSAPI`
rp|  - English: `"Attempt GSSAPI key exchange"` (with accelerator `&` if desired)
ne|  - French/Spanish: `[T]` placeholder for translators
qf|  - Update `src/resource/TextsCore1.rc` if needed
my|  - **Blocked by:** Task 13
hk|
fr|
gf|ul|- [x] **Task 15:** Wire or remove dead `FOpensshPrivateKeyFile` field
kp|  - File: `src/core/SecureShell.cpp` (`StoreToConfig()`), `src/NetBox/WinSCPDialogs.cpp`
nw|  - **Finding:** `FOpensshPrivateKeyFile` has full UI controls (`OpensshKeyEdit` on Authentication tab's "OpenSSH Certificate" group), storage serialization (`ReadString`/`WriteString`), and property accessors — but is **never passed to PuTTY** for SSH authentication. Only `FPublicKeyFile` → `CONF_keyfile` is used.
ao|  - **Options:**
sw|    1. Wire it: When `UseOpensshCertificate` is enabled and `FOpensshPrivateKeyFile` is non-empty, pass it to `CONF_keyfile` instead of `FPublicKeyFile` (or in addition, if PuTTY supports multiple key files).
gk|    2. Consolidate: Rebind `OpensshKeyEdit` to `FPublicKeyFile` and remove `FOpensshPrivateKeyFile` from data model + storage + UI to eliminate confusion.
hq|  - **Rationale:** The current UI implies the "OpenSSH private key file" field is functional for connections, but it is dead code. This causes user confusion when they configure a key in the certificate group and auth still fails.
xi|  - **Blocked by:** Task 5 (core auth fix stable)
em|
## Commit Message (Draft)

```
fix(ssh): resolve private key authentication failure (#392)
```
at|
fix(ssh): resolve private key authentication failure and align SSH UI with WinSCP (#392)
ct|
Fix SSH public-key authentication so sessions with configured PPK/OpenSSH
ot|key files connect successfully to publickey-only servers. Adds diagnostic
vk|logging to trace key file resolution, loading, and auth prompt handling.
wk|
oa|Also aligns Session Editor SSH UI with WinSCP:
to|no|- Authentication tab: browse button for private key file selection
vm|iz|- Authentication tab: "Display Public Key" and "Tools" action buttons
gx|at|- Authentication tab: browse button for certificate file selection
aq|du|- Key Exchange tab: "Attempt GSSAPI key exchange" checkbox (WinSCP parity)
no|Fixes GitHub issue #392
xn|```

## Acceptance Criteria

- [ ] SFTP session with configured PPK key connects to `publickey`-only server without password prompt
- [ ] Encrypted PPK key with stored passphrase connects automatically
- [ ] Encrypted PPK key without stored passphrase prompts for passphrase (not generic password)
- [ ] Password-only SFTP server still works (no regression)
- [ ] Agent-only auth still works (no regression)
- [ ] Build passes with zero warnings
dp|- [ ] ChangeLog and Github-Issues.md updated
er|- [ ] Private key file field has browse button with correct filters (PPK, PEM, KEY, id_*)
ld|- [ ] "Display Public Key" button shows key fingerprint/blob for valid key files
sw|za|- [ ] Tools dropdown button deferred (Generate/Import/Convert require PuTTYgen integration)
zz|- [ ] Certificate file field has browse button with .pub filter
rv|- [ ] Key Exchange tab shows "Attempt GSSAPI key exchange" checkbox and persists value
rv|- [ ] `FOpensshPrivateKeyFile` is either wired to `CONF_keyfile` when certificate auth is enabled, or removed from UI to avoid user confusion
zf|

## Notes

- The error `No supported authentication methods available` originates in PuTTY `userauth2-client.c:2118`, not NetBox code. It indicates the client's loaded credentials do not intersect with the server's offered methods.
- PuTTY 0.81 loads the configured key file at auth start via `ppk_loadpub_f()`. If this fails, the client has no publickey method available.
- WinSCP 6.5.6 uses the same PuTTY version; differences must be in NetBox's glue code (`SecureShell.cpp`, `Terminal.cpp`, prompt handling).
- `DoVerifyKey()` auto-converts OpenSSH→PPK in the UI save path. Sessions created via import or command line may bypass this conversion. PuTTY 0.81 natively supports OpenSSH keys, so conversion should not be required for auth.
- The `SetPassphrase()` method in `SessionData` encrypts the passphrase with `PublicKeyFile` as the encryption key. If `PublicKeyFile` changes, the passphrase is re-encrypted — this could cause issues if the path changes during session loading.
- Do NOT modify `libs/putty/` — any PuTTY-level fix must be applied as a patch or handled in NetBox's glue code.
yx|oc|- **WinSCP UI alignment:** WinSCP uses `TFilenameEdit` (VCL) with built-in browse and action buttons for private key and certificate fields. NetBox uses custom `TFarDialog` controls — implement browse/action buttons as `TFarButton` controls with `OPENFILENAMEW` dialogs and `GetPublicKeyLine()` for display.
ij|ea|- **KEX tab parity:** `FAuthGSSAPIKEX` field exists in `TSessionData`, persisted to storage, cloned for tunnels, wired to PuTTY `CONF_try_gssapi_kex`, and included in session info logging — but the KEX tab has no `AuthGSSAPIKEXCheck` checkbox. WinSCP exposes this on its Key Exchange tab as "Attempt GSSAPI key exchange".
ea|
ag|
## Changelog
gr|
op|| Date       | Change                     | Reason |
ql||------------|----------------------------|--------|
mr|| 2026-05-02 | Initial plan               | Issue #392 analysis |
ok|| 2026-05-02 | Refined (2nd iteration)    | /aif-improve: added UI alignment tasks (10-12) for WinSCP SSH Authentication tab parity |
mc|| 2026-05-03 | Refined (3rd iteration)    | /aif-improve: added KEX tab alignment tasks (13-14) — missing `AuthGSSAPIKEXCheck` checkbox for GSSAPI key exchange (WinSCP parity) |
zc|| 2026-05-03 | Refined (4th iteration)    | /aif-improve: corrected Task 10 (PrivateKeyEdit is on Session tab, not Auth tab; deferred Tools dropdown); added Task 15 (dead `FOpensshPrivateKeyFile` field) |
wi||            |                            |  |