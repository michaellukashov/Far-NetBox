# OpenSSH Certificate-Based Authentication Integration

**Branch:** `feature/openssh-certificate-auth`
**Created:** 2026-04-26
**Updated:** 2026-04-26 (post-aif-improve — second pass deep codebase analysis)
**Mode:** Full Plan

## Settings

- **Testing:** No — NetBox has no unit test infrastructure; rely on manual Far Manager testing
- **Logging:** Verbose — detailed `FTerminal->LogEvent()` calls throughout certificate loading, parsing, and authentication flow
- **Docs:** Yes — mandatory documentation checkpoint at completion; update AGENTS.md and user-facing changelog
- **Roadmap Linkage:** Milestone: "SSH Protocol Modernization" — extends existing OpenSSH 8.8+ compatibility work (issues #263, #323)

**Reference files:**

- [SSH Authentication Exploration](../references/ssh-authentication-exploration.md) — code paths, PuTTY cert support details, key format limitations, extension approach
- [Session Configuration UI Patterns](../references/session-config-ui-patterns.md) — dialog architecture, tab UI patterns, control creation, data binding, extension code examples

### Codebase Findings (Deep Analysis)

**PuTTY certificate support — CONFIRMED working:**
- PuTTY 0.81 includes full OpenSSH certificate support (`libs/putty/crypto/openssh-certs.c`)
- Validates: signature, CA key validity, cert type (user/host), time bounds, principal matching
- Rejects unsupported critical options with clear error messages
- Certificate file format: RFC4716 or OpenSSH public key format with `*-cert-v01@openssh.com` algorithm
- `CONF_detached_cert` is defined in `libs/putty/conf.h` and already wired in `SecureShell.cpp:291-293`

**CRITICAL BLOCKER — Private key format:**
- PuTTY's `ssh2_userauth_new()` (userauth2-client.c:499-530) **only loads PPK format** (`SSH_KEYTYPE_SSH2`) for private keys
- `ppk_load_f()` at line 1282 handles private key loading — does NOT auto-convert OpenSSH format
- `import_ssh2()` exists in `libs/putty/import.c:144` for format conversion but is **NOT used** in the authentication flow
- **`key_type()`** (`libs/putty/ssh/sshpubk.c:2014`) can detect OPENSSH_PEM, OPENSSH_NEW, SSHCOM formats but detection ≠ conversion
- **Conclusion: OpenSSH-format private keys (~/.ssh/id_*) will NOT work directly with PuTTY's auth flow**

**Resolution approach:** Two viable paths:
- **Option A (recommended):** Convert OpenSSH private key → PPK format at runtime using existing `GetKeyType()` + `LoadKey()` + `ppk_save_f()` before passing to `CONF_keyfile`
- **Option B:** Prompt user to convert keys manually using PuTTYgen; only accept PPK files
- **Option A is preferred** for UX — transparent conversion with no user burden

**UI findings:**
- `tabAuthentication` contains only checkboxes (SshNoUserAuthCheck, TryAgentCheck, AuthKICheck, etc.) separated by TFarSeparator groups
- **No file path controls with browse buttons exist anywhere in WinSCPDialogs.cpp**
- File path controls (PrivateKeyEdit, S3CACertificateEdit) are simple TFarEdit without browse buttons
- Space available below GSSAPI group on tabAuthentication for new controls
- Data binding happens in `Execute()` after modal result — controls read via `GetText()`, written via property setters

**Data flow (confirmed):**
```
TSessionData → StoreToConfig() → PuTTY Conf → backend_init() → ssh2_userauth_new()
  FDetachedCertificate ──────→ CONF_detached_cert ─→ cert param
  FPublicKeyFile ────────────→ CONF_keyfile ───────→ keyfile param
```

### Key Decisions (Locked)

1. **`FDetachedCertificate` REUSED** for certificate file path — no new cert field needed
2. **`FOpensshPrivateKeyFile`** added for OpenSSH-format private key path (user specifies ~/.ssh/id_*)
3. **`FUseOpensshCertificate`** added as enable/disable flag
4. **Runtime conversion** (Option A) — OpenSSH key → PPK via existing `GetKeyType()` + `LoadKey()` + `ppk_save_f()` before `CONF_keyfile`
5. **No CA validation in v1** — PuTTY already validates certs; we just log the results
6. **Simple TFarEdit controls** — no browse buttons (matches existing patterns in WinSCPDialogs.cpp)
7. **No new source files needed** — key conversion uses existing `LoadKey()`/`SaveKey()` in PuttyIntf.cpp
8. **TASK-9 (Tunnel Support) deferred to v2** — scope too large for initial delivery

## Related GitHub Issues

- [#263](https://github.com/michaellukashov/Far-NetBox/issues/263) — Error when connecting to new OpenSSH servers
- [#323](https://github.com/michaellukashov/Far-NetBox/issues/323) — SFTP key auth is broken for OpenSSH 8.7p1 & 8.8p1
- [#156](https://github.com/michaellukashov/Far-NetBox/issues/156) — Diffie-Hellman group exchange KEX is obsolete
- [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — User-provided auth certificate
- [#388](https://github.com/michaellukashov/Far-NetBox/issues/388) — RSA-SHA256 support
- [#36](https://github.com/michaellukashov/Far-NetBox/issues/36) — Support for Unix-style SSH keys as well as PuTTY-style keys

## Tasks

### Phase 0: Investigation (RESOLVED)

#### TASK-0: Verify FDetachedCertificate Reuse for OpenSSH Certificates ✅ RESOLVED

**Status:** COMPLETED — deep codebase analysis confirmed findings

**Findings:**
- `FDetachedCertificate` is already wired to `CONF_detached_cert` in `SecureShell.cpp:291-293`
- PuTTY 0.81 has full OpenSSH certificate support (`openssh-certs.c`)
- Certificate validation (signature, CA, time bounds, principals) handled by PuTTY
- `import_ssh2()` in `libs/putty/import.c:144` can convert OpenSSH → PPK but is NOT used in auth flow
- **Blocker:** PuTTY only accepts PPK format for private keys — OpenSSH keys need conversion
- **No UI** currently exists for `FDetachedCertificate`

**Decision:** Reuse `FDetachedCertificate`, add runtime OpenSSH→PPK conversion.

---

### Phase 1: Data Model & UI

#### TASK-1: Extend TSessionData with OpenSSH Certificate Fields

**Description:** Add new fields to `TSessionData` for OpenSSH certificate-based authentication. Reuse existing `FDetachedCertificate` for certificate path; add OpenSSH private key field with enable/disable flag.

**File paths:**
- `src/core/SessionData.h` — add new member variables, properties, getters/setters
- `src/core/SessionData.cpp` — implement getters/setters, serialization in `DoLoad()`/`DoSave()`

**Changes:**
1. Add `FOpensshPrivateKeyFile` (UnicodeString) — path to OpenSSH private key file (`~/.ssh/id_*` format)
2. Add `FUseOpensshCertificate` (bool) — enable/disable certificate authentication for this session
3. **Reuse existing `FDetachedCertificate`** for certificate file path — no new cert field needed
4. Add corresponding `__property` declarations and getter/setter methods following naming conventions:
   - `SetOpensshPrivateKeyFile()`, `GetOpensshPrivateKeyFile()`
   - `SetUseOpensshCertificate()`, `GetUseOpensshCertificate()`
5. Add new fields to the PROPERTY macro block (near line 435 in SessionData.cpp, where `PROPERTY2(DetachedCertificate)` and `PROPERTY_HANDLER(Passphrase, F)` are defined). Use `PROPERTY2(OpensshPrivateKeyFile)` and `PROPERTY(UseOpensshCertificate)` to auto-generate getters/setters.
6. Add storage serialization in `DoLoad()` (line ~878, near `DetachedCertificate = Storage->ReadString(...)`) and `DoSave()` (line ~1250, near `WRITE_DATA_EX(String, "DetachedCertificate", FDetachedCertificate, )`) using `Storage->ReadString()`/`Storage->WriteString()` and `Storage->ReadBool()`/`Storage->WriteBool()`.
7. Add field to `IsSame()` comparison method (at SessionData.cpp:769) — compare new fields against defaults
8. Add field to `ImportFromOpenssh()` for ssh_config import support

**Logging:** Log every property read/write during session load/save with `FTerminal->LogEvent(L"SessionData: %s = %s", ...)`

**Dependencies:** TASK-0 (already resolved — findings documented above)

**Done when:** New fields compile, serialize/deserialize correctly, and appear in stored session configs.

---

#### TASK-2: Add OpenSSH Certificate Controls to Session Dialog UI

**Description:** Add controls to `tabAuthentication` in TSessionDialog for OpenSSH certificate and private key file paths. Follow existing simple TFarEdit patterns (no browse buttons — matches WinSCPDialogs.cpp conventions).

**File paths:**
- `src/NetBox/WinSCPDialogs.cpp` — add controls to tabAuthentication `Init()`, wire up in `Execute()`

**Changes:**
1. **Declare member variables in TSessionDialog class** (near existing control declarations like `SshNoUserAuthCheck`, `TryAgentCheck`):
   - `TFarCheckBox * FUseOpensshCertCheck`
   - `TFarText * FOpensshCertLabel`
   - `TFarEdit * FOpensshCertEdit`
   - `TFarText * FOpensshKeyLabel`
   - `TFarEdit * FOpensshKeyEdit`
2. In `Init()` for tabAuthentication, below the GSSAPI group:
   - Add checkbox: "Use OpenSSH certificate authentication" (`FUseOpensshCertCheck`)
   - Add label: "Certificate file:" + TFarEdit for `FDetachedCertificate` path (`FOpensshCertEdit`)
   - Add label: "Private key file (OpenSSH format):" + TFarEdit for `FOpensshPrivateKeyFile` path (`FOpensshKeyEdit`)
   - Use existing UI patterns: `MakeOwnedObject<TFarCheckBox>()`, `MakeOwnedObject<TFarEdit>()`, `MakeOwnedObject<TFarText>()`, `SetNextItemPosition()`, `SetDefaultGroup(tabAuthentication)`
3. Wire enable/disable: checkbox state controls edit field and label enabled state
4. In `Execute()` after modal result (IDOK):
   - `SessionData->SetUseOpensshCertificate(FUseOpensshCertCheck->GetChecked())`
   - `SessionData->SetDetachedCertificate(FOpensshCertEdit->GetText())`
   - `SessionData->SetOpensshPrivateKeyFile(FOpensshKeyEdit->GetText())`
5. In `Init()` for dialog population:
   - `FOpensshCertEdit->SetText(SessionData->DetachedCertificate)`
   - `FOpensshKeyEdit->SetText(SessionData->GetOpensshPrivateKeyFile())`
   - `FUseOpensshCertCheck->SetChecked(SessionData->GetUseOpensshCertificate())`
   - Set initial enabled state: `FOpensshCertEdit->SetChecked(SessionData->GetUseOpensshCertificate())`
6. Follow spacing/separator patterns from existing checkbox groups on tabAuthentication
7. Add validation: if cert auth enabled but paths empty, show error via `MessageDlg()`

**Note:** No browse buttons — matches existing patterns (S3CACertificateEdit, PrivateKeyEdit, RecycleBinPathEdit all use simple TFarEdit). User types or pastes the path.

**Logging:** Log UI state changes: `LogEvent(L"Dialog: OpenSSH cert auth %s", UseOpensshCertificate ? L"enabled" : L"disabled")`

**Dependencies:** TASK-1 (TSessionData fields must exist)

**Done when:** Dialog displays controls, paths can be entered, values persist through Apply/OK, validation prevents empty paths when enabled.

---

### Phase 2: Core SSH Integration

#### TASK-3: Add OpenSSH → PPK Key Conversion for PuTTY Authentication

**Description:** **CRITICAL TASK** — PuTTY only accepts PPK format for private keys. Implement runtime conversion from OpenSSH format to PPK so `~/.ssh/id_*` keys work with certificate authentication.

**Key finding from deep analysis:** NetBox **already has** the key conversion infrastructure:
- `GetKeyType()` at `PuttyIntf.cpp:796` — detects key format (ktOpenSSHPEM, ktOpenSSHNew, ktSSHCom, ktSSH2)
- `LoadKey()` at `PuttyIntf.cpp:855-900` — calls `import_ssh2()` for OpenSSH formats, `ppk_load_f()` for PPK
- `SaveKey()` at `PuttyIntf.cpp:1040-1082` — calls `ppk_save_f()` to write PPK format
- `FreeKey()` at `PuttyIntf.cpp:1084-1090` — cleans up `ssh2_userkey` struct

**File paths:**
- `src/core/SecureShell.cpp` — add key conversion before `StoreToConfig()`
- `src/core/PuttyTools.h` — verify `GetKeyType()`/`LoadKey()`/`SaveKey()`/`FreeKey()` are declared and accessible

**Changes:**
1. **Leverage existing infrastructure** — do NOT create new conversion functions:
   - Call `GetKeyType(FOpensshPrivateKeyFile)` to detect format
   - If `ktOpenSSHPEM`, `ktOpenSSHNew`, or `ktSSHCom`: call `LoadKey()` to parse OpenSSH key into `ssh2_userkey`
   - Call `ppk_save_f(tempPPKPath, ssh2_userkey, passphrase, &ppk_save_default_parameters)` to write PPK to temp file
   - Call `FreeKey()` to clean up the in-memory key struct
2. In `StoreToConfig()` or before `CONF_keyfile` is set:
   - If `FSessionData->GetUseOpensshCertificate()` is true and `FOpensshPrivateKeyFile` is set:
     - Generate temp PPK path using `GetTempPath()` + unique filename
     - Convert OpenSSH key → temp PPK file using steps above
     - Set `CONF_keyfile` to temp PPK path
     - Store temp file path in `FTempPPKFile` member variable (new field on `TSecureShell`) for cleanup
3. **Temp PPK file lifecycle:**
   - Add `UnicodeString FTempPPKFile` member to `TSecureShell`
   - Delete temp file in `TSecureShell::Close()` and destructor
   - Use `DeleteFile()` for cleanup; on Windows, consider `FILE_FLAG_DELETE_ON_CLOSE` via `CreateFile()`
4. Handle conversion errors: log detailed error, fall back to standard key auth if `FPublicKeyFile` is also set
5. Handle passphrase-protected keys using existing `FPassphrase` flow (already passed to `LoadKey()`)

**Key reference points:**
- `import_ssh2()` at `libs/putty/import.c:144` — converts OpenSSH to internal `ssh2_userkey` struct
- `ppk_load_f()` at `libs/putty/ssh/sshpubk.c:1068` — loads PPK format
- `ppk_save_f()` at `libs/putty/ssh/ssh.h:1579` — writes PPK format from `ssh2_userkey` struct
- `ppk_save_default_parameters` at `libs/putty/ssh/ssh.h:1575` — default save parameters
- `GetKeyType()` at `src/core/PuttyIntf.cpp:796` — detects key format
- `LoadKey()` at `src/core/PuttyIntf.cpp:855-900` — loads key with format-specific dispatch
- `SaveKey()` at `src/core/PuttyIntf.cpp:1040-1082` — saves key as PPK
- `FreeKey()` at `src/core/PuttyIntf.cpp:1084-1090` — frees `ssh2_userkey`

**Logging:**
```
LogEvent(L"SSH: Converting OpenSSH key %s to PPK format", KeyPath)
LogEvent(L"SSH: Key conversion %s", Success ? L"succeeded" : L"failed")
LogEvent(L"SSH: Temporary PPK file: %s", TempPath)
```

**Dependencies:** TASK-1

**Done when:** OpenSSH-format private keys are converted to PPK at runtime and accepted by PuTTY's authentication flow.

---

#### TASK-4: Wire Certificate + Converted Key to PuTTY Authentication

**Description:** Ensure `StoreToConfig()` correctly passes the certificate path and (converted) key path to PuTTY when OpenSSH certificate auth is enabled.

**File paths:**
- `src/core/SecureShell.cpp` — modify `StoreToConfig()` method (around lines 287-293)

**Changes:**
1. In `StoreToConfig()`, after existing `CONF_keyfile` setup (line 287-289):
   - If `FSessionData->GetUseOpensshCertificate()` is true:
     - `conf_set_filename(conf, CONF_detached_cert, ...)` with `FSessionData->DetachedCertificate` (already at line 291 — verify correct)
     - If `FOpensshPrivateKeyFile` is set and conversion (TASK-3) succeeded:
       - `conf_set_filename(conf, CONF_keyfile, ...)` with converted PPK path (override existing `CONF_keyfile`)
     - Else if `FPublicKeyFile` is also set:
       - Log warning: "Both PPK and OpenSSH key configured; using OpenSSH key for certificate auth"
2. Verify that `CONF_detached_cert` at SecureShell.cpp:291 uses the correct variable (should be `Data->DetachedCertificate` — confirm it's being used)
3. Ensure `ExpandEnvironmentVariables()` is applied to certificate path (matching existing pattern at line 291)
4. No changes needed to PuTTY — `ssh2_userauth_new()` already handles detached certificates (confirmed in libs/putty/ssh/ssh.c:259-260)

**Existing code to verify/modify:**
```cpp
// SecureShell.cpp:287-293 (existing)
Filename * AFileName = filename_from_utf8(UTF8String(Data->ResolvePublicKeyFile()).c_str());
conf_set_filename(conf, CONF_keyfile, AFileName);
filename_free(AFileName);

AFileName = filename_from_utf8(UTF8String(ExpandEnvironmentVariables(Data->DetachedCertificate)).c_str());
conf_set_filename(conf, CONF_detached_cert, AFileName);
filename_free(AFileName);
```

**Logging:**
```
LogEvent(L"SSH: Certificate auth enabled, cert=%s key=%s", CertPath, KeyPath)
LogEvent(L"SSH: CONF_detached_cert set to %s", CertPath)
LogEvent(L"SSH: CONF_keyfile set to %s (converted from OpenSSH)", PpkPath)
```

**Dependencies:** TASK-3

**Done when:** PuTTY receives correct certificate and key paths, authentication succeeds with OpenSSH certificate + key pair.

---

#### TASK-5: Log Certificate Authentication Results

**Description:** Add logging for certificate authentication results. PuTTY already validates certificates — we just need to surface the results in NetBox's log output through existing ScpSeat callbacks.

**File paths:**
- `src/core/SecureShell.cpp` — add logging around authentication flow
- `src/core/PuttyIntf.cpp` — review `ScpSeat` class callbacks for auth result hooks; cert validation errors surface through Seat notification callbacks

**Changes:**
1. Log when certificate authentication is attempted (before `StoreToConfig()` is called)
2. **Capture cert errors via ScpSeat callbacks:** Review `ScpSeat` in `PuttyIntf.cpp` for Seat interface methods that receive error notifications (e.g., `notify_userpass`, `connection_fatal`). Certificate validation errors (expired cert, unknown CA, principal mismatch) are surfaced through these callbacks — add logging to capture them.
3. Log certificate principals and validity info if available from PuTTY's cert parsing
4. Capture and log PuTTY's certificate-related error messages via Seat callbacks
5. Use existing error callback patterns from `ScpSeat` for auth failure messages

**Note:** PuTTY's certificate validation (openssh-certs.c) already checks:
- Signature validity
- CA key validity (not a certificate itself)
- Certificate type (user vs host)
- Time bounds (not before / not after)
- Principal matching (username)
- Critical options support

We just need to log the outcomes via ScpSeat, not re-implement validation.

**Logging:**
```
LogEvent(L"SSH: OpenSSH certificate authentication attempt")
LogEvent(L"SSH: Certificate file: %s", CertPath)
LogEvent(L"SSH: Key file (converted): %s", PpkPath)
LogEvent(L"SSH: Certificate auth result: %s", Result)
```

**Dependencies:** TASK-4

**Done when:** All certificate authentication attempts and results are logged with meaningful detail for debugging.

---

### Phase 3: Configuration & Compatibility

#### TASK-6: Add OpenSSH Certificate Support to URL/Session String Parsing

**Description:** Enable OpenSSH certificate configuration via session URL strings and command-line parameters.

**File paths:**
- `src/core/SessionData.cpp` — extend `ApplyRawSettings()` and URL parsing
- `src/core/SessionData.h` — review URL parameter parsing patterns

**Changes:**
1. Add URL parameters: `cert=<path>` (reuses `FDetachedCertificate`) and `opensshkey=<path>`
2. Extend `ApplyRawSettings()` to recognize and apply OpenSSH certificate settings
3. Extend `GetProtocolUrl()` to include OpenSSH certificate parameters in the URL
4. Ensure `ImportFromOpenssh()` can detect and import OpenSSH certificate references from ssh_config files
5. Test URL round-trip: parse → apply → export → parse should produce identical settings

**Logging:**
```
LogEvent(L"Session: Parsing cert=%s from URL", CertPath)
LogEvent(L"Session: Parsing opensshkey=%s from URL", KeyPath)
```

**Dependencies:** TASK-1

**Done when:** OpenSSH certificate settings can be configured via session URLs and survive round-trip parsing.

---

#### ~~TASK-7: Add XML Storage Support for OpenSSH Certificate Settings~~ MERGED INTO TASK-1

**Reason:** `DoLoad()`/`DoSave()` in SessionData.cpp handle BOTH registry storage (TFar3Storage) AND XML storage (TXmlStorage) through the unified `THierarchicalStorage` abstraction. There is no separate XML-specific save/load path — the same `Storage->ReadString()`/`WriteString()` calls work for both backends. TASK-1's serialization changes automatically cover XML storage.

**No separate implementation needed.** Acceptance criteria moved to TASK-1: "Test with both registry and XML storage backends".

---

### Phase 4: Integration & Testing

#### TASK-8: Integrate OpenSSH Certificate Auth with Far Manager Plugin Flow

**Description:** Ensure the Far Manager plugin properly uses OpenSSH certificate authentication throughout the session lifecycle.

**File paths:**
- `src/NetBox/WinSCPFileSystem.cpp` — review session creation and connection flow
- `src/NetBox/WinSCPFileSystem.h` — review session management
- `src/core/SftpFileSystem.h` — review SFTP session creation
- `src/core/ScpFileSystem.h` — review SCP session creation

**Changes:**
1. Ensure `TWinSCPFileSystem` passes OpenSSH certificate settings to `TSecureShell` during connection
2. Verify that SFTP and SCP file system implementations correctly inherit the certificate settings from TSessionData
3. Add error messages in Far Manager UI when certificate authentication fails (missing file, expired cert, etc.)
4. Ensure the plugin correctly handles certificate re-authentication on connection retry
5. Test with both SFTP (fsSFTP) and SCP (fsSCP) protocols

**Logging:**
```
LogEvent(L"Plugin: Creating SFTP session with OpenSSH certificate auth")
LogEvent(L"Plugin: Certificate auth %s for session %s", Enabled ? L"enabled" : L"disabled", SessionName)
```

**Dependencies:** TASK-2, TASK-5

**Done when:** Far Manager plugin correctly initiates and handles OpenSSH certificate authentication for both SFTP and SCP sessions.

---

#### ~~TASK-9: Add Tunnel Support for OpenSSH Certificate Authentication~~ DEFERRED TO V2

**Reason:** Tunnel support doubles the scope (mirror fields + UI + conversion for tunnel). The main feature is already complex with key conversion, cert UI, and auth wiring. The extension pattern is identical to main session and can be implemented after v1 delivery.

**V2 extension notes (for future reference):**
- Add `FTunnelOpensshPrivateKeyFile` (UnicodeString) and `FTunnelUseOpensshCertificate` (bool) to TSessionData
- Reuse `FTunnelDetachedCertificate` if it exists, otherwise add it
- Add UI controls to tunnel configuration tab in TSessionDialog
- Apply same OpenSSH → PPK conversion flow as TASK-3
- Follow existing tunnel key file patterns (FTunnelPublicKeyFile, FTunnelPassphrase)

**Dependencies:** TASK-3, TASK-4 (when implemented in v2)

---

### Phase 5: Temp PPK File Lifecycle

#### TASK-10: Manage Temporary PPK File Lifecycle

**Description:** Ensure temporary PPK files created during OpenSSH → PPK conversion are properly managed throughout the connection lifecycle and cleaned up reliably.

**File paths:**
- `src/core/SecureShell.h` — add `FTempPPKFile` member variable
- `src/core/SecureShell.cpp` — add cleanup in `Close()` and destructor

**Changes:**
1. Add `UnicodeString FTempPPKFile` private member to `TSecureShell` class
2. In `StoreToConfig()` (or pre-connection setup), after converting OpenSSH key → temp PPK:
   - Store temp file path in `FTempPPKFile`
3. In `TSecureShell::Close()`:
   - If `!FTempPPKFile.IsEmpty()`: call `DeleteFile(FTempPPKFile)`, clear `FTempPPKFile`
   - Log cleanup: `LogEvent(L"SSH: Cleaned up temporary PPK file %s", FTempPPKFile)`
4. In `TSecureShell::~TSecureShell()`:
   - Same cleanup as `Close()` as a safety net for crash scenarios
5. Consider `FILE_FLAG_DELETE_ON_CLOSE` on Windows for additional crash safety (via `CreateFile()` when creating temp PPK, then `ppk_save_f()` with that handle)
6. Ensure temp file path is unique per session to avoid conflicts between concurrent connections

**Dependencies:** TASK-3

**Done when:** Temporary PPK files are reliably cleaned up on normal disconnect, explicit close, and destructor invocation; no temp files remain after crash.

---

## Commit Plan

Given 9 active tasks (TASK-0 resolved, TASK-7 merged, TASK-9 deferred, TASK-10 added) across 4 phases, use the following commit checkpoints:

### Commit 1: Data Model & UI (after TASK-2)
```
feat(session): add OpenSSH certificate fields and UI controls

- Add FOpensshPrivateKeyFile and FUseOpensshCertificate to TSessionData
- Reuse existing FDetachedCertificate for certificate file path
- Add PROPERTY macro entries for automatic getter/setter generation
- Add serialization in DoLoad()/DoSave() (covers both registry and XML storage)
- Add IsSame() comparison for new fields
- Add certificate auth controls to tabAuthentication in TSessionDialog
- Wire up dialog population and Apply handler for new controls

Related: #509, #36
```

### Commit 2: Core SSH Integration (after TASK-5)
```
feat(ssh): integrate OpenSSH certificate authentication with PuTTY engine

- Leverage existing GetKeyType()/LoadKey()/ppk_save_f() for OpenSSH → PPK conversion
- Wire certificate and converted key to PuTTY CONF_detached_cert/CONF_keyfile
- Add temp PPK file lifecycle management (creation, cleanup in Close/destructor)
- Add verbose logging throughout certificate authentication flow
- Capture certificate validation errors via ScpSeat callbacks
- Handle passphrase-protected keys and conversion errors

Note: PuTTY only accepts PPK format for private keys; OpenSSH keys are converted
at runtime using existing NetBox key infrastructure (PuttyIntf.cpp)

Related: #263, #323, #509, #36
```

### Commit 3: Configuration & Plugin Integration (after TASK-8 + TASK-10)
```
feat(config): add OpenSSH certificate support to URL parsing and plugin flow

- Extend ApplyRawSettings() for cert and opensshkey URL parameters
- Extend GetProtocolUrl() to include certificate parameters in session URLs
- Support ssh_config import with certificate references
- Wire certificate settings through Far Manager plugin session lifecycle
- Add error messages in Far Manager UI for certificate auth failures
- Support certificate auth for both SFTP and SCP protocols
- Ensure reliable temp PPK file cleanup across connection lifecycle

Related: #36, #388, #156
```

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `import_ssh2()` incompatible with modern OpenSSH key formats | Medium | High | Test with OpenSSH 8.8+ generated keys; fallback to manual PPK conversion with user warning |
| Temporary PPK file security (left on disk after crash) | Medium | Medium | Use `GetTempPath()` + unique filename; cleanup in Close() and destructor; consider `FILE_FLAG_DELETE_ON_CLOSE` |
| Passphrase handling for OpenSSH new format keys | Medium | Medium | Test with passphrase-protected OpenSSH keys; reuse existing `FPassphrase` flow through `LoadKey()` |
| `ppk_save_f()` parameters change in future PuTTY versions | Low | Medium | Use `ppk_save_default_parameters`; pin to PuTTY 0.81 API surface |
| Backward compatibility with older session files | Low | Medium | Default new fields to disabled/empty; test with existing saved sessions |
| UI conflicts with existing auth options | Low | Low | Follow existing checkbox enable/disable patterns; test all auth combinations |

## Edge Cases

1. **Expired certificate:** PuTTY rejects with error — capture and display meaningful message to user
2. **Revoked certificate:** No CRL support; PuTTY doesn't check CRLs — log warning if CA fingerprint unknown
3. **Unknown CA:** PuTTY validates cert signature independently — log CA fingerprint for debugging
4. **Missing private key:** Error before connection attempt; prompt user to select valid key
5. **Certificate/key mismatch:** PuTTY validates match during auth — capture and log the error
6. **OpenSSH key format detection:** Use existing `GetKeyType()` to detect OPENSSH_PEM vs OPENSSH_NEW vs SSHCOM
7. **Certificate with multiple principals:** PuTTY matches against username — log all principals
8. **Temp PPK file left after crash:** Cleanup in destructor; consider `FILE_FLAG_DELETE_ON_CLOSE`
9. **Both PPK and OpenSSH key configured:** Prefer OpenSSH key when cert auth enabled; log warning
10. **Concurrent connections with cert auth:** Each session gets its own temp PPK file with unique name

## Changelog

### 2026-04-26 — aif-improve (Second Pass — Deep Codebase Analysis)

**CRITICAL corrections:**
- `save_ssh2_privatekey()` does NOT exist in PuTTY — corrected to `ppk_save_f()` (libs/putty/ssh.h:1579)
- NetBox **already has** complete key conversion infrastructure: `GetKeyType()`, `LoadKey()`, `SaveKey()`, `FreeKey()` in PuttyIntf.cpp (lines 796-1090)
- TASK-3 rewritten to leverage existing infrastructure — no new conversion functions needed
- Removed `PuttyTools.cpp` from TASK-3 file paths — no new source files needed

**TASK improvements:**
- TASK-1: Added explicit PROPERTY macro block reference (SessionData.cpp:435), `IsSame()` location (line 769), `DoLoad()` (line 878), `DoSave()` (line 1250)
- TASK-2: Added member variable declaration step for TSessionDialog class
- TASK-5: Added ScpSeat callback references for cert error capture

**TASK-7 removed:** Merged into TASK-1 — `DoLoad()`/`DoSave()` handle both registry and XML storage via THierarchicalStorage abstraction; no separate XML path exists

**TASK-9 deferred to v2:** Tunnel support doubles scope; extension pattern is identical and can be implemented after v1 delivery

**TASK-10 added:** Temp PPK file lifecycle management (FTempPPKFile member, cleanup in Close/destructor, crash safety)

**Commit plan reduced:** 4 commits → 3 commits (TASK-7 merged, TASK-9 deferred)

**Risk assessment updated:**
- Removed `import_ssh2()` visibility risk (already used in PuttyIntf.cpp:871)
- Removed `save_ssh2_privatekey()` risk (function doesn't exist)
- Added `ppk_save_f()` parameter stability risk

**Edge cases updated:**
- #6: Changed from `key_type()` to `GetKeyType()` (existing NetBox function)
- #8: Removed tunnel-related edge case (TASK-9 deferred)
- #10: Added concurrent connection edge case
