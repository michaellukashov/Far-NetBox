# OpenSSH Certificate-Based Authentication Integration

**Branch:** `feature/openssh-certificate-auth`
**Created:** 2026-04-26
**Updated:** 2026-04-26 (post-aif-improve — deep codebase analysis)
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
- **Option A (recommended):** Convert OpenSSH private key → PPK format at runtime using PuTTY's `import_ssh2()` + `save_ssh2_privatekey()` before passing to `CONF_keyfile`
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
4. **Runtime conversion** (Option A) — OpenSSH key → PPK via PuTTY's `import_ssh2()` before `CONF_keyfile`
5. **No CA validation in v1** — PuTTY already validates certs; we just log the results
6. **Simple TFarEdit controls** — no browse buttons (matches existing patterns in WinSCPDialogs.cpp)

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
5. Add storage serialization in `DoLoad()` and `DoSave()` using `Storage->ReadString()`/`Storage->WriteString()` and `Storage->ReadBool()`/`Storage->WriteBool()`
6. Add field to `Assign()` copy method and `IsSame()` comparison method
7. Add field to `ImportFromOpenssh()` for ssh_config import support

**Logging:** Log every property read/write during session load/save with `FTerminal->LogEvent(L"SessionData: %s = %s", ...)`

**Dependencies:** TASK-0 (already resolved — findings documented above)

**Done when:** New fields compile, serialize/deserialize correctly, and appear in stored session configs.

---

#### TASK-2: Add OpenSSH Certificate Controls to Session Dialog UI

**Description:** Add controls to `tabAuthentication` in TSessionDialog for OpenSSH certificate and private key file paths. Follow existing simple TFarEdit patterns (no browse buttons — matches WinSCPDialogs.cpp conventions).

**File paths:**
- `src/NetBox/WinSCPDialogs.cpp` — add controls to tabAuthentication `Init()`, wire up in `Execute()`

**Changes:**
1. In `Init()` for tabAuthentication, below the GSSAPI group:
   - Add checkbox: "Use OpenSSH certificate authentication" (`FUseOpensshCertCheck`)
   - Add label: "Certificate file:" + TFarEdit for `FDetachedCertificate` path (`FOpensshCertEdit`)
   - Add label: "Private key file (OpenSSH format):" + TFarEdit for `FOpensshPrivateKeyFile` path (`FOpensshKeyEdit`)
   - Use existing UI patterns: `MakeOwnedObject<TFarCheckBox>()`, `MakeOwnedObject<TFarEdit>()`, `SetNextItemPosition()`, `SetDefaultGroup(tabAuthentication)`
2. Wire enable/disable: checkbox state controls edit field enabled state
3. In `Execute()` after modal result (IDOK):
   - `SessionData->SetUseOpensshCertificate(UseOpensshCertCheck->GetChecked())`
   - `SessionData->SetDetachedCertificate(FOpensshCertEdit->GetText())`
   - `SessionData->SetOpensshPrivateKeyFile(FOpensshKeyEdit->GetText())`
4. In `Init()` for dialog population:
   - `FOpensshCertEdit->SetText(SessionData->DetachedCertificate)`
   - `FOpensshKeyEdit->SetText(SessionData->GetOpensshPrivateKeyFile())`
   - `UseOpensshCertCheck->SetChecked(SessionData->GetUseOpensshCertificate())`
5. Follow spacing/separator patterns from existing checkbox groups on tabAuthentication
6. Add validation: if cert auth enabled but paths empty, show error via `MessageDlg()`

**Note:** No browse buttons — matches existing patterns (S3CACertificateEdit, PrivateKeyEdit, RecycleBinPathEdit all use simple TFarEdit). User types or pastes the path.

**Logging:** Log UI state changes: `LogEvent(L"Dialog: OpenSSH cert auth %s", UseOpensshCertificate ? L"enabled" : L"disabled")`

**Dependencies:** TASK-1 (TSessionData fields must exist)

**Done when:** Dialog displays controls, paths can be entered, values persist through Apply/OK, validation prevents empty paths when enabled.

---

### Phase 2: Core SSH Integration

#### TASK-3: Add OpenSSH → PPK Key Conversion for PuTTY Authentication

**Description:** **CRITICAL TASK** — PuTTY only accepts PPK format for private keys. Implement runtime conversion from OpenSSH format to PPK so `~/.ssh/id_*` keys work with certificate authentication.

**File paths:**
- `src/core/SecureShell.cpp` — add key conversion before `StoreToConfig()`
- `src/core/PuttyTools.h` — add conversion function declaration
- `src/core/PuttyTools.cpp` — implement OpenSSH → PPK conversion

**Changes:**
1. Add `ConvertOpenSSHKeyToPPK()` function:
   - Read OpenSSH private key file (PEM or new OpenSSH format)
   - Use PuTTY's `import_ssh2()` (`libs/putty/import.c:144`) to parse OpenSSH format into `struct ssh2_userkey`
   - Use PuTTY's `save_ssh2_privatekey()` to write PPK format to a temporary file
   - Handle passphrase-protected keys — pass passphrase to `import_ssh2()`
   - Return path to temporary PPK file
2. In `StoreToConfig()` or before `CONF_keyfile` is set:
   - If `FUseOpensshCertificate` is true and `FOpensshPrivateKeyFile` is set:
     - Convert OpenSSH key → temporary PPK file
     - Set `CONF_keyfile` to temporary PPK path
     - Store temporary file path for cleanup on disconnect
3. Cleanup temporary PPK file in `TSecureShell::Close()` or destructor
4. Handle conversion errors: log detailed error, fall back to standard key auth if `FPublicKeyFile` is also set
5. Handle passphrase-protected keys using existing `FPassphrase` flow

**Key reference points:**
- `import_ssh2()` at `libs/putty/import.c:144` — converts OpenSSH to internal key struct
- `ppk_load_f()` at `libs/putty/ssh/sshpubk.c` — loads PPK format (what PuTTY normally uses)
- `save_ssh2_privatekey()` — writes PPK format from internal key struct
- `key_type()` at `libs/putty/ssh/sshpubk.c:2014` — detects key format (OPENSSH_PEM, OPENSSH_NEW, SSHCOM)

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

**Description:** Add logging for certificate authentication results. PuTTY already validates certificates — we just need to surface the results in NetBox's log output.

**File paths:**
- `src/core/SecureShell.cpp` — add logging around authentication flow
- `src/core/PuttyIntf.h` — review ScpSeat callbacks for auth result hooks

**Changes:**
1. Log when certificate authentication is attempted
2. Log PuTTY's certificate validation results (errors are surfaced through PuTTY's error callbacks)
3. Log certificate principals and validity info if available from PuTTY's cert parsing
4. Capture and log PuTTY's certificate-related error messages (expired cert, unknown CA, principal mismatch, etc.)
5. Use existing error callback patterns from `ScpSeat` for auth failure messages

**Note:** PuTTY's certificate validation (openssh-certs.c) already checks:
- Signature validity
- CA key validity (not a certificate itself)
- Certificate type (user vs host)
- Time bounds (not before / not after)
- Principal matching (username)
- Critical options support

We just need to log the outcomes, not re-implement validation.

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

#### TASK-7: Add XML Storage Support for OpenSSH Certificate Settings

**Description:** Ensure OpenSSH certificate fields are properly saved/loaded in XML session storage format.

**File paths:**
- `src/core/SessionData.cpp` — verify `DoLoad()`/`DoSave()` handle XML storage correctly
- `src/NetBox/XmlStorage.cpp` — review XML storage patterns for reference

**Changes:**
1. Verify that `DoSave()` writes OpenSSH certificate fields to XML storage with correct element names
2. Verify that `DoLoad()` reads OpenSSH certificate fields from XML storage
3. Ensure password-protected private keys use `LOAD_PASSWORD`/`SAVE_PASSWORD` for encryption
4. Add XML import/export compatibility with existing session XML files
5. Handle backward compatibility: older XML files without these fields should load with defaults (disabled)

**Logging:**
```
LogEvent(L"XML: Saving opensshkey=%s", KeyPath)
LogEvent(L"XML: Loading opensshkey=%s", KeyPath)
```

**Dependencies:** TASK-1

**Done when:** OpenSSH certificate settings persist correctly in XML session files and load without errors.

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
6. **Verify new .cpp files are added to CMakeLists.txt** — per project rules, any new source files must be added to the build

**Logging:**
```
LogEvent(L"Plugin: Creating SFTP session with OpenSSH certificate auth")
LogEvent(L"Plugin: Certificate auth %s for session %s", Enabled ? L"enabled" : L"disabled", SessionName)
```

**Dependencies:** TASK-2, TASK-5

**Done when:** Far Manager plugin correctly initiates and handles OpenSSH certificate authentication for both SFTP and SCP sessions.

---

#### TASK-9: Add Tunnel Support for OpenSSH Certificate Authentication

**Description:** Extend tunnel (SSH tunneling) configuration to support OpenSSH certificate authentication.

**File paths:**
- `src/core/SessionData.h` — add tunnel certificate fields (FTunnelOpensshPrivateKeyFile, FTunnelUseOpensshCertificate)
- `src/core/SessionData.cpp` — add getters/setters and serialization for tunnel certificate fields
- `src/NetBox/WinSCPDialogs.cpp` — add UI controls to tunnel configuration tab

**Changes:**
1. Add tunnel-specific OpenSSH certificate fields mirroring the main session fields:
   - `FTunnelOpensshPrivateKeyFile` (UnicodeString)
   - `FTunnelUseOpensshCertificate` (bool)
   - **Reuse `FTunnelDetachedCertificate`** if it exists, otherwise add it
2. Implement getters/setters and storage serialization
3. Add UI controls to the tunnel configuration tab in TSessionDialog
4. Ensure tunnel connection uses certificate auth when configured
5. Follow existing tunnel key file patterns (FTunnelPublicKeyFile, FTunnelPassphrase)
6. Apply same OpenSSH → PPK conversion for tunnel keys

**Logging:**
```
LogEvent(L"SSH: Tunnel using OpenSSH certificate authentication")
LogEvent(L"SSH: Tunnel certificate: %s", CertPath)
```

**Dependencies:** TASK-3, TASK-4

**Done when:** SSH tunnels can use OpenSSH certificate authentication with the same workflow as primary sessions.

---

## Commit Plan

Given 10 tasks (TASK-0 through TASK-9, TASK-0 already resolved) across 5 phases, use the following commit checkpoints:

### Commit 1: Data Model & UI (after TASK-2)
```
feat(session): add OpenSSH certificate fields and UI controls

- Add FOpensshPrivateKeyFile and FUseOpensshCertificate to TSessionData
- Reuse existing FDetachedCertificate for certificate file path
- Add certificate auth controls to tabAuthentication in TSessionDialog
- Wire up Apply handler for new controls
- Add storage serialization for new fields

Related: #509, #36
```

### Commit 2: Core SSH Integration (after TASK-5)
```
feat(ssh): integrate OpenSSH certificate authentication with PuTTY engine

- Add OpenSSH → PPK key conversion using import_ssh2() (TASK-3)
- Wire certificate and converted key to PuTTY CONF_detached_cert/CONF_keyfile (TASK-4)
- Add verbose logging throughout certificate authentication flow (TASK-5)
- Handle passphrase-protected keys and conversion errors
- PuTTY handles cert validation — we log the results

Note: PuTTY only accepts PPK format for private keys; OpenSSH keys are converted at runtime

Related: #263, #323, #509, #36
```

### Commit 3: Configuration & Compatibility (after TASK-7)
```
feat(config): add OpenSSH certificate support to URL parsing and XML storage

- Extend ApplyRawSettings() for cert and opensshkey URL parameters
- Add XML storage serialization for certificate fields
- Ensure backward compatibility with older session files
- Support ssh_config import with certificate references

Related: #36, #388
```

### Commit 4: Integration & Tunnel Support (after TASK-9)
```
feat(plugin): integrate OpenSSH cert auth with Far Manager plugin and tunnels

- Wire certificate settings through Far Manager plugin session lifecycle
- Add error messages in Far Manager UI for certificate auth failures
- Extend tunnel configuration with OpenSSH certificate fields and UI
- Support certificate auth for both SFTP and SCP protocols
- Verify CMakeLists.txt includes all new source files

Related: #156, #263, #323
```

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `import_ssh2()` incompatible with modern OpenSSH key formats | Medium | High | Test with OpenSSH 8.8+ generated keys; fallback to manual PPK conversion with user warning |
| Temporary PPK file security (left on disk after crash) | Medium | Medium | Use `GetTempPath()` + unique filename; cleanup in destructor; mark as delete-on-close if possible |
| Passphrase handling for OpenSSH new format keys | Medium | Medium | Test with passphrase-protected OpenSSH keys; reuse existing `FPassphrase` flow |
| `import_ssh2()` or `save_ssh2_privatekey()` not publicly accessible | Low | High | May need to wrap in PuttyTools; check visibility of these functions in PuTTY headers |
| Backward compatibility with older session files | Low | Medium | Default new fields to disabled/empty; test with existing saved sessions |
| UI conflicts with existing auth options | Low | Low | Follow existing checkbox enable/disable patterns; test all auth combinations |

## Edge Cases

1. **Expired certificate:** PuTTY rejects with error — capture and display meaningful message to user
2. **Revoked certificate:** No CRL support; PuTTY doesn't check CRLs — log warning if CA fingerprint unknown
3. **Unknown CA:** PuTTY validates cert signature independently — log CA fingerprint for debugging
4. **Missing private key:** Error before connection attempt; prompt user to select valid key
5. **Certificate/key mismatch:** PuTTY validates match during auth — capture and log the error
6. **OpenSSH key format detection:** Use `key_type()` to detect OPENSSH_PEM vs OPENSSH_NEW vs SSHCOM
7. **Certificate with multiple principals:** PuTTY matches against username — log all principals
8. **Tunnel + main session both using certs:** Ensure independent configuration and temp file cleanup for each
9. **Both PPK and OpenSSH key configured:** Prefer OpenSSH key when cert auth enabled; log warning
10. **Temp PPK file left after crash:** Cleanup in destructor; consider `FILE_FLAG_DELETE_ON_CLOSE`

## Changelog

### 2026-04-26 — aif-improve (Deep Codebase Analysis)

**TASK-0 resolved — Critical findings:**
- PuTTY 0.81 has full OpenSSH certificate support — no cert field needed, reuse `FDetachedCertificate`
- **BLOCKER:** PuTTY only accepts PPK format for private keys (`ppk_load_f` at userauth2-client.c:1282)
- `import_ssh2()` exists for format conversion but is NOT used in auth flow — must be wired up
- **Added TASK-3:** OpenSSH → PPK key conversion (critical path)
- **Added TASK-4:** Wire cert + converted key to PuTTY (split from old TASK-3)
- **Split old TASK-3** into TASK-3 (conversion) + TASK-4 (wiring) + TASK-5 (logging)
- **Reordered:** TASK-5 (logging) now depends on TASK-4 (wiring)
- **Reordered:** TASK-8 depends on TASK-5 instead of TASK-3
- **Reordered:** TASK-9 depends on TASK-3 and TASK-4

**UI improvements:**
- No browse buttons in WinSCPDialogs.cpp — use simple TFarEdit controls (matches existing patterns)
- Space available on tabAuthentication below GSSAPI group

**Risk assessment updated:**
- Added `import_ssh2()` compatibility risk
- Added temporary PPK file security risk
- Added PuTTY function visibility risk

**Edge cases expanded:**
- Added #6 (key format detection via `key_type()`)
- Added #10 (temp file cleanup with FILE_FLAG_DELETE_ON_CLOSE)

**CMakeLists.txt verification** added to TASK-8 (per project rules from skill-context)

**Commit plan updated** to reflect new task structure (10 tasks, 4 commits)
