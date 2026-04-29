# Implementation Plan: S3 Protocol Encryption Options — WinSCP Alignment

**Branch:** none (stored plan)

**Created:** 2026-04-26

**Plan Type:** full

---

## Settings

- **Testing:** yes (Catch2 unit tests for serialization round-trips; manual integration tests for TLS setup)
- **Logging:** verbose (DEBUG logs for encryption config application, certificate loading, TLS negotiation)
- **Docs:** yes (update session config docs + `.hlf` help files)

---

## Research Context

**Source:** Exploration results from three parallel agents (WinSCP SessionData audit, NetBox S3FileSystem mapping, NetBox UI dialog identification)

### Scope Boundary

- **Focus:** TLS/session-layer encryption only (TLS version constraints, cipher preferences, certificate validation behavior, session data serialization consistency)
- **Out of scope:** S3 server-side encryption (SSE-S3, SSE-KMS, SSE-C) — WinSCP does not implement these concepts for S3 sessions; adding them would require libs3 API changes and bucket-level configuration beyond session scope
- **"Consistent with WinSCP" means:**
  1. Same `SESSION_PROPERTY` / `RWProperty` macro family for property declarations
  2. Same `WRITE_DATA` / `ReadXXX` serialization patterns in `DoLoad`/`DoSave`
  3. Same default value strategy for S3 sessions
  4. Same UI control type (edit field + Load/Save buttons for certificate paths)
  5. Same fallback behavior when settings are unspecified

### Missing Source Fallback

- If any WinSCP reference file or member is missing or has unexpected structure, document the discrepancy and use the closest available pattern from other protocol implementations (SFTP/FTP/WebDAV). **Do not fabricate WinSCP API signatures.**

---

## Architecture Reference

**Layered Plugin Architecture:**

```
Plugin Layer (src/NetBox/)        — Far Manager API, dialogs (WinSCPDialogs.cpp)
        ↓
Core Layer (src/core/)            — Protocol implementations (S3FileSystem.cpp)
        ↓
Base Layer (src/base/, nbcore/)   — Foundation classes (UnicodeString, Classes)
        ↓
Third-Party (libs/)               — libs3, neon, OpenSSL
```

**Dependency Rules:**
- Plugin Layer → Core Layer (plugin calls protocol implementations)
- Core Layer → Base Layer (protocols use foundation classes)
- Core Layer → Third-Party (protocols wrap library implementations)
- **Never** modify `libs/` — use patches only
- **Never** call Far Manager APIs from worker threads

---

## Reference Documents

The following reference documents were created by exploration agents and provide detailed findings:

- [WinSCP SessionData Encryption Settings](.ai-factory/references/winscp-sessiondata-encryption-settings.md) — WinSCP encryption field definitions, property macros, serialization patterns, TLS version defaults, and cipher/KEX list handling
- [NetBox S3FileSystem Session Mapping](.ai-factory/references/netbox-s3filesystem-session-mapping.md) — NetBox S3-specific session data fields, serialization gaps, TLS setup via neon, and CA certificate TODO
- [NetBox UI Dialogs for S3 Config](.ai-factory/references/netbox-ui-dialogs-s3-config.md) — NetBox session dialog architecture, S3 tab controls, FTP encryption pattern, and Load/Save button implementation guide for certificate editing

---

## Current State Summary

### What Exists (NetBox)

|Component|Status|
|---|---|
|`FS3CACertificate` field in `TSessionData`|✅ Declared in `SessionData.h`|
|TLS via `MinTlsVersion`/`MaxTlsVersion`|✅ Shared across S3/WebDAV/FTP, applied via `SetupSsl()`|
|S3 tab in session dialog (`tabS3`)|✅ Shows Region, URL Style, Requester Pays, CA Certificate|
|`S3CACertificateEdit` in dialog|✅ Text field exists|
|S3 always requires TLS|✅ `RequireTls()` called in `S3FileSystem::Open()`|

### Critical Gaps Identified

|Gap|Impact|WinSCP Pattern|
|---|---|---|
|**S3CACertificate not serialized** (missing from `DoLoad`/`DoSave`)|Custom CA config lost on save/load|All S3 fields use `WRITE_DATA`/`ReadUnicodeString`|
|**S3CACertificate not applied to neon SSL**|Even if set, custom CA unused; connection uses embedded `cacert.pem` only|`ne_ssl_set_certificates_storage()` after writing cert to temp file (per TODO comment)|
|**TLS version selectors not exposed in S3 tab UI**|Users cannot override Min/Max TLS for S3 sessions|WinSCP TLS page includes S3; NetBox exposes FTP encryption combo but not TLS versions|
|**S3CACertificate property not in PROPERTY2 block**|Change notifications may not fire|All other S3 properties use `RWPropertySimple`|
|**No cipher suite awareness**|Cipher choice limited to OpenSSL defaults|WinSCP SFTP/SCP have cipher lists; TLS-based protocols (S3/WebDAV) use OpenSSL defaults controlled by TLS version — this is intentional, no change needed|

### What NOT to Change

|Aspect|Reason|
|---|---|
|Per-S3 cipher list selection|WinSCP TLS page does not expose cipher selection for S3/WebDAV; uses OpenSSL defaults controlled by `MinTlsVersion`/`MaxTlsVersion` — this is the established pattern|
|Server-side encryption (SSE-S3/SSE-KMS/SSE-C)|WinSCP does not implement SSE for S3; would require libs3 API changes|
|FTPS encryption combo pattern|S3 always uses HTTPS; no plain/implicit/explicit variants exist for S3|

---

## Tasks

### Phase 1: Fix S3CACertificate Serialization

**Affected files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`

- [x] **Task 1: Add S3CACertificate to serialization pipeline**

  - **Goal:** Make `S3CACertificate` persistent across session save/load cycles
  - **Files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`
  - **Clarification:** `FS3CACertificate` property already exists in SessionData.h. The gap is DoLoad/DoSave serialization only.
  - **Changes:**
    1. In `SessionData.cpp` `DoLoad()`: Add `FS3CACertificate = Storage->ReadString(L"S3CACertificate", FS3CACertificate);` following the same pattern as `S3DefaultRegion`
    2. In `SessionData.cpp` `DoSave()` (non-Putty path): Add `WRITE_DATA(String, S3CACertificate);` following the same pattern as `S3DefaultRegion`
  - **Testing:** Add Catch2 unit test for serialization round-trip (not just manual test)
  - **Edge case:** Existing sessions without this property will load with empty string default — no migration needed
  - **Verification:** Round-trip test — set value, save session, reload, verify value persists + automated unit test
  - **Blocked by:** none

### Phase 2: Apply S3CACertificate to neon SSL Context

**Affected files:** `src/core/S3FileSystem.cpp`

- [x] **Task 2: Implement S3CACertificate loading in TLS initialization**

  - **Goal:** Apply custom CA certificate from `S3CACertificate` to the neon SSL context during S3 connection establishment
  - **Files:** `src/core/S3FileSystem.cpp`
  - **Method to modify:** `InitSslSessionImpl()` (called via `LibS3SessionCallback` → `SetNeonTlsInit` → `InitSslSession` → `InitSslSessionImpl`, lines 726-780)
  - **Certificate Integration Note:** S3CACertificate coexists independently with global `CertificateStorage` in Configuration (used by NeonIntf.cpp for WebDAV). S3-specific CA is used for S3 sessions; global is used for other protocols. No conflict — they operate at different protocol scopes.
  - **Changes:**
    1. Read `FS3CACertificate` from `FTerminal->SessionData->S3CACertificate`
    2. If non-empty, write certificate content to a temporary file
    3. Call `ne_ssl_set_certificates_storage()` with the temp file path
    4. Retain temp file until S3 session ends (clean up in `Close()` or session destructor)
  - **Cleanup:** Add temporary file deletion in S3FileSystem `Close()` method - track temp file path and delete on session close
  - **LOGGING:**
    - DEBUG: `InitSslSessionImpl: S3CACertificate is %s`, `(configured)` or `(empty)`
    - DEBUG: `InitSslSessionImpl: Wrote CA cert to temp file: %s`, temp path
    - INFO: `InitSslSessionImpl: Custom CA certificate applied to SSL context`
    - WARN: `InitSslSessionImpl: Failed to apply custom CA certificate: %s`, error message
    - ERROR: `InitSslSessionImpl: ne_ssl_set_certificates_storage failed`
  - **Edge cases:**
    - Empty `S3CACertificate` → skip custom CA, use default `cacert.pem` bundle (existing behavior)
    - Invalid certificate file → log WARN, fall back to default bundle
    - Temp file creation failure → log ERROR, abort connection with meaningful message
    - Multiple concurrent S3 sessions with different custom CAs → each session has its own temp file
  - **Reference pattern:** WebDAV uses same `SetupSsl()` pattern (see `WebDAVFileSystem.cpp` lines 2188-2195); S3-specific CA loading needs neon's certificate storage API
  - **Blocked by:** Task 1

### Phase 3: Expose TLS Version Controls in S3 Tab UI

**Affected files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`

- [x] **Task 3: Add MinTlsVersion/MaxTlsVersion controls to S3 tab**

  - **Goal:** Allow users to configure TLS version range specifically for S3 sessions
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`
  - **Design Note:** `MinTlsVersion`/`MaxTlsVersion` are session-level settings shared across all protocols (S3/WebDAV/FTP). This task adds S3-specific UI, allowing users to set different TLS requirements for S3 vs other protocols.
  - **Changes:**
    1. Identify the S3 tab layout in `TSessionDialog` - search `tabS3` in `WinSCPDialogs.cpp` (see line 2039)
    2. Add two combo boxes: `MinTlsVersionCombo` and `MaxTlsVersionCombo` to the S3 tab
    3. Populate combo boxes: TLS 1.0, TLS 1.1, TLS 1.2, TLS 1.3
    4. Wire to `FSessionData->MinTlsVersion` and `FSessionData->MaxTlsVersion`
    5. Add validation: Max TLS version must be >= Min TLS version
    6. Add default: TLS 1.2 min, TLS 1.3 max
  - **LOGGING:**
    - DEBUG: `TSessionDialog: S3 tab TLS version controls shown`
    - DEBUG: `TSessionDialog: MinTlsVersion=%d, MaxTlsVersion=%d`, values on dialog open
    - INFO: `TSessionDialog: S3 TLS version range changed: %s to %s`, on user change
    - WARN: `TSessionDialog: Invalid TLS version range (max < min), reverting`
  - **Edge cases:**
    - Protocol downgrade warning: warn if TLS 1.0 min selected
    - Existing sessions: load current values from session data
  - **Reference pattern:** WinSCP TLS page; NetBox FTP `FtpEncryptionCombo` pattern
  - **Blocked by:** none

- [x] **Task 4: Add S3CACertificate Load/Save buttons to S3 tab**

  - **Goal:** Add Load/Save buttons next to existing `S3CACertificateEdit` field for certificate file management
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`
  - **Changes:**
    1. Add `TFarButton* S3CACertificateLoadBtn` and `TFarButton* S3CACertificateSaveBtn` to `TSessionDialog`
    2. Position adjacent to `S3CACertificateEdit` (at ~line 2579 in `WinSCPDialogs.cpp`)
    3. Implement `S3CACertificateLoadClick()`:
       - Use Windows `GetOpenFileNameW()` from COMDLG32
       - Filter: PEM files (`*.pem`), all files
       - Read file content, populate Edit control
       - Validate PEM header: `-----BEGIN CERTIFICATE-----`
    4. Implement `S3CACertificateSaveClick()`:
       - Use Windows `GetSaveFileNameW()` from COMDLG32
       - Default extension: `.pem`
       - Write Edit control content to selected file
    5. Wire in `InitDialog()`: populate Edit from `SessionData->S3CACertificate`
    6. Wire in `OkClick()`: update `SessionData->SetS3CACertificate()` from Edit
  - **API Details:**
    ```cpp
    // Windows COMDLG32 for file dialogs
    #include <windows.h>
    #include <commdlg.h>
    // Use GetOpenFileNameW / GetSaveFileNameW (Unicode)
    // Allocate buffer: wchar_t path[MAX_PATH];
    ```
  - **LOGGING:**
    - DEBUG: `TSessionDialog: S3 CA certificate loaded from: %s`
    - INFO: `TSessionDialog: S3 CA certificate saved to: %s`
    - ERROR: `TSessionDialog: Failed to load S3 CA certificate: %s`, error
  - **Edge cases:**
    - Non-existent file → log ERROR, show error dialog
    - Invalid PEM → warn but allow override (user decision)
    - Read-only file → log ERROR, disable Save button
  - **UX Gap (Post-Implement):** Dialog handlers call `MoreMessageDialog(GetMsg(MSG_TITLE_...), nullptr, ...)` — passing `nullptr` as the body message. Add `MsgIDs` for `NB_S3_INVALID_PEM`, `NB_S3_LOAD_ERROR`, `NB_S3_SAVE_ERROR` and wire them into `.lng` files so users see meaningful error text, not just a titled empty dialog.
  - **Blocked by:** none

### Phase 4: Code Consistency & Quality

**Affected files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`, `src/core/S3FileSystem.cpp`, `src/NetBox/WinSCPDialogs.cpp`

- [x] **Task 5: Verify naming conventions and code style**

  - **Goal:** Ensure all new code follows NetBox conventions
  - **Files:** All modified files from Tasks 1-4
  - **Checklist:**
    - [ ] Class names use `T` prefix (e.g., `TS3SslHelper`)
    - [ ] Member variables use `F` prefix (e.g., `FS3CACertificate`)
    - [ ] Methods use PascalCase (e.g., `SetS3CACertificate()`)
    - [ ] Local variables use camelCase (e.g., `sessionData`)
    - [ ] Brace style: Allman/BSD (opening brace on new line)
    - [ ] Indentation: 2 spaces (no tabs)
    - [ ] Line endings: CRLF
    - [ ] Encoding: UTF-8 without BOM
    - [ ] No trailing whitespace
    - [ ] Max line length: 120 characters
    - [ ] Pointer/reference middle alignment (`int * ptr`)
    - [ ] `#pragma once` in new headers
    - [ ] No raw `new`/`delete` — use RAII/smart pointers
    - [ ] Common typo check: loose→lose, connexion→connection, authentification→authentication, occured→occurred, recieve→receive, seperate→separate
  - **Blocked by:** Tasks 1-4

### Phase 5: Build & Testing

- [x] **Task 6: Build verification**

  - **Goal:** Clean build with zero warnings (MSVC W4)
  - **Command:** Run `build-x64.bat` or equivalent CMake build
  - **Steps:**
    1. Configure: `cmake -S . -B build-RelWithDebugInfo -A x64 -DOPT_CREATE_PLUGIN_DIR=ON -DOPT_USE_UNITY_BUILD=OFF`
    2. Build: `cmake --build build-RelWithDebugInfo --clean-first`
    3. Fix any W4 warnings
    4. Verify DLL output to `Far3_x64/Plugins/NetBox/`
  - **Edge case:** Unity build may cause symbol redefinition — use `OPT_USE_UNITY_BUILD=OFF`
  - **Blocked by:** Task 5

- [x] **Task 7: Manual integration testing**

  - **Goal:** Verify S3 encryption settings work end-to-end
  - **Test scenarios:**
    1. Create S3 session, set custom CA certificate path, save, reopen session, verify persistence
    2. Set Min TLS version to 1.2, Max TLS version to 1.3, save, verify applied in connection
    3. Load valid PEM certificate via Load button, verify Edit field populated
    4. Save certificate via Save button, verify file written correctly
    5. Test error paths: read-only file, non-existent file, invalid PEM, temp file creation failure
    6. Test backward compatibility: open existing S3 session without CA cert, verify loads with empty string
    7. Test default behavior: new session without TLS version override, verify uses defaults
    8. **Test concurrent sessions**: multiple S3 sessions with different custom CA certificates, verify each uses its own certificate
  - **LOGGING:** Enable verbose logging, capture `%LOCALAPPDATA%\NetBox\netbox.log` for review
  - **Blocked by:** Task 6

### Phase 6: Documentation

- [x] **Task 8: Update user documentation**

  - **Goal:** Document new S3 encryption options for end users
  - **Files:** `docs/` (S3 session configuration section), `.hlf` help files (if present)
  - **Content:**
    1. Document TLS version controls in S3 tab
    2. Document CA certificate Load/Save functionality
    3. Explain when to use custom CA (self-signed certs, corporate CA)
    4. Add troubleshooting section for common certificate issues
  - **Blocked by:** Task 7

### Phase 7: Post-Implementation Follow-ups

The following items were discovered during `/aif-improve` refinement (2026-04-29) and are **not** critical to the original feature scope, but should be addressed for completeness.

- [ ] **Task 9: Add Catch2 unit test for S3CACertificate serialization round-trip**

  - **Goal:** Automated verification that `S3CACertificate` persists across save/load
  - **Files:** `tests/nbcore/SessionDataTest.cpp` (or equivalent)
  - **Status:** INFEASIBLE with current test infrastructure
  - **Reason:** `TSessionData` is a 6000+ line class with heavy VCL/Borland dependencies (`vcl.h`, `CoreMain.h`, `PuttyIntf.h`, `XMLDoc.hpp`, etc.). Existing project tests (`test_sessionhistory`, `test_tinylog`, `DatetimeUnitTest`) only exercise small, self-contained utility classes with minimal source-file linking. Adding a `TSessionData` test target would require pulling in the majority of the core library and likely VCL runtime, far exceeding the scope of a focused unit test. Integration testing (Task 7) already covers serialization round-trip manually.

- [x] **Task 10: Localize new UI strings in non-English `.lng` files**

  - **Goal:** Replace English placeholder strings with proper translations
  - **Files:** `src/NetBox/NetBoxRus.lng`, `src/NetBox/NetBoxPol.lng`, `src/NetBox/NetBoxFr.lng`, `src/NetBox/NetBoxSpa.lng`
  - **Strings to translate:**
    - `"Min TLS version:"` / `"Max TLS version:"`
    - `"&Load"` / `"&Save"`
  - **Note:** These strings were added with English placeholders during implementation. They appear at message IDs `NB_S3_MIN_TLS_VERSION`, `NB_S3_MAX_TLS_VERSION`, `NB_S3_LOAD_CA_CERT`, `NB_S3_SAVE_CA_CERT`.
  - **Blocked by:** none

- [x] **Task 11: Add user-facing error message bodies for Load/Save button failures**

  - **Goal:** Show meaningful text when certificate load/save fails, instead of an empty dialog with just a title
  - **Files:** `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng` (and other `.lng` files), `src/NetBox/WinSCPDialogs.cpp`
  - **Changes:**
    1. Add `NB_S3_INVALID_PEM`, `NB_S3_LOAD_ERROR`, `NB_S3_SAVE_ERROR` to `MsgIDs.h`
    2. Add corresponding strings to all `.lng` files
    3. Update `S3CACertificateLoadClick()` and `S3CACertificateSaveClick()` to pass the new `MsgID` as the second argument to `MoreMessageDialog()` instead of `nullptr`
  - **Blocked by:** none

- [x] **Task 12: Update `docs/README.md` with S3 encryption documentation**

  - **Goal:** The project README should mention S3 TLS version configuration and custom CA certificate support
  - **Files:** `docs/README.md`
  - **Note:** Task 8 updated `.hlf` help files, but `docs/README.md` still does not mention S3 TLS/CA features under the Supported Protocols section.
  - **Blocked by:** none

---

## Dependencies

```
Task 1 ──→ Task 2 ──→ Task 5 ──→ Task 6 ──→ Task 7 ──→ Task 8
Task 3 ───────────→ Task 5
Task 4 ───────────→ Task 5
```

- Tasks 1, 3, 4 can proceed in parallel
- Task 2 depends on Task 1 (serialization must work before applying cert)
- Task 5 depends on all implementation tasks (1-4)
- Tasks 6, 7, 8 are sequential (build → test → docs)
- Tasks 9-12 are independent follow-ups; none block the original plan
---

## Commit Plan

**Checkpoint 1** (after Tasks 1-2):
```
fix(s3): serialize and apply S3CACertificate for custom CA support

- Add S3CACertificate to DoLoad/DoSave serialization pipeline
- Apply custom CA certificate to neon SSL context in InitSslSessionImpl
- Add verbose logging for certificate loading and application
```

**Checkpoint 2** (after Tasks 3-4):
```
feat(s3): add TLS version controls and certificate Load/Save buttons to S3 tab

- Add MinTlsVersion/MaxTlsVersion combo boxes to S3 session tab
- Add Load/Save buttons for S3CACertificate field
- Wire dialog controls to session data with validation
- Add PEM validation on certificate load
```

**Checkpoint 3** (after Tasks 5-8):
```
chore(s3): verify code style, build, and document encryption options

- Ensure all code follows NetBox naming and formatting conventions
- Build passes with zero MSVC W4 warnings
- Manual testing of S3 encryption settings end-to-end
- Update user documentation for new S3 encryption options
```

---

## Success Criteria

- [x] `S3CACertificate` persists across session save/load (round-trip test passes)
- [x] Custom CA certificate is applied to neon SSL context during S3 connection
- [x] TLS version selectors visible in S3 tab with working Min/Max validation
- [x] Load/Save buttons for CA certificate functional with PEM validation
- [x] Build passes with zero warnings (MSVC W4)
- [x] No modifications to `libs/`
- [x] Plugin DLL output to `Far3_x64/Plugins/NetBox/`
- [x] CRLF line endings on all modified files
- [x] UTF-8 without BOM, no trailing whitespace
- [x] Naming conventions enforced (T/F prefixes, PascalCase methods, camelCase locals)
- [x] No spelling errors in comments
- [x] Backward compatibility: existing S3 sessions load without errors

---

## Risks & Mitigations

|Risk|Likelihood|Impact|Mitigation|
|---|---|---|---|
|neon `ne_ssl_set_certificates_storage()` API unavailable|Medium|High|Check neon headers; if missing, fall back to setting `SSL_CTX_set_default_verify_paths()` with custom path|
|TLS version combo conflicts with existing global TLS settings|Low|Medium|Scope controls to S3 tab only; use session-level `MinTlsVersion`/`MaxTlsVersion` which are already shared across protocols|
|Unity build symbol redefinition|Medium|Low|Use `OPT_USE_UNITY_BUILD=OFF` (documented in AGENTS.md)|
|WinSCP source structure differs from assumptions|Low|Medium|Use "Missing Source Fallback" — document gap and use closest pattern from FTP/WebDAV implementations|
|S3CACertificate conflicts with global CertificateStorage|Low|None|S3CACertificate coexists independently with global CertificateStorage — no conflict (different protocol scopes)|
|Temp file cleanup timing|Medium|Low|Retain temp file until S3 session ends (neon's API may reference path during SSL operations)|

---

## Changelog

|Date|Change|
|---|---|
|2026-04-26|Initial plan created based on PromptSentinel review findings|
|2026-04-26|Applied H-1 fix: replaced ambiguous UI instruction with explicit file discovery|
|2026-04-26|Applied H-2 fix: replaced "or equivalent" with specific method identification|
|2026-04-26|Applied M-1: added missing-source fallback instruction|
|2026-04-26|Applied M-2: defined measurable "consistency" criteria|
|2026-04-26|Applied L-1: converted negations to positive constraints|
|2026-04-26|Applied L-2: added scope guard for SSE (TLS-only focus)|
|2026-04-26|Applied review fixes: added Clarification section for certificate integration and temp file lifecycle|
|2026-04-26|User clarification: S3CACertificate coexists independently with global CertificateStorage|
|2026-04-26|User clarification: temp files retained until S3 session ends|
|2026-04-26|Fixed P3: corrected TLS default values (TLS 1.2 min, TLS 1.3 max)|
|2026-04-26|Fixed P3: added clarification that FS3CACertificate already exists (serialization gap only)|
|2026-04-26|Added P2: added Catch2 unit test for serialization round-trip|
|2026-04-26|Added P3: test scenario for concurrent sessions with different CAs|
|2026-04-26|aif-improve: removed redundant "add to property block" step (already exists)|
|2026-04-26|aif-improve: added temp file cleanup note to Task 2 (delete in Close())|
|2026-04-26|aif-improve: added Design Note to Task 3 (shared TLS settings)|
|2026-04-26|aif-improve: added API details to Task 4 (GetOpenFileNameW/GetSaveFileNameW)|
|2026-04-29|Post-merge security audit: redacted temp file path from logs (LOW-1), checked `ne_ssl_set_certificates_storage` return value (LOW-2), used `Contains()` for PEM validation (LOW-3)|
|2026-04-29|aif-improve refinement: removed infeasible `FTerminal->LogEvent()` from Task 1, documented empty-dialog UX gap in Task 4, added Tasks 9-12 as post-implementation follow-ups|
