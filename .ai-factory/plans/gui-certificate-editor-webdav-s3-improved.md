<!-- handoff:task: -->
# Implementation Plan: GUI for Editing cacert.pem in WEBDAV/S3 Session Dialog (Refined)

**Branch:** none (stored plan)

**Created:** 2026-04-24

**Original Plan:** `.ai-factory/plans/gui-certificate-editor-webdav-s3.md`

## Rationale for Refinement

After codebase analysis and code review (2026-04-25), the original plan had gaps and architectural assumptions that conflict with NetBox patterns. This refined version corrects those issues based on review feedback.

## Settings

- **Testing:** yes (Catch2 unit tests for PEM validation; manual integration tests for session persistence)
- **Logging:** verbose (DEBUG logs for file I/O, certificate validation)
- **Docs:** yes (docs/features/ + .hlf help files)

## Research Context

**Source:** .ai-factory/RESEARCH.md (Active Summary)

**Constraints:**
- Build must pass MSVC W4 with zero warnings
- All modified files use CRLF line endings
- Follow NetBox C++17 standards and T/F prefix naming
- Do not modify any files under libs/
- Plugin DLL output to Far3_<platform>/Plugins/NetBox/ (requires OPT_CREATE_PLUGIN_DIR=ON)
- Avoid Unity build issues (may need OPT_USE_UNITY_BUILD=OFF if symbol redefinition occurs)

**Codebase Analysis Findings:**
1. **S3 has `S3CACertificate` property** (`SessionData.h` lines 698-699, `WinSCPDialogs.cpp` lines 1795-1796) - WebDAV lacks equivalent
2. **WebDAV certificate handling exists** in `WebDAVFileSystem.cpp` but uses global/config-level certificates
3. **Dialog pattern:** Far Manager dialogs use `TFarDialog` with `TFarEdit`, `TFarButton`, `TFarCheckBox` (not VCL tabs)
4. **Session dialog location:** `src/NetBox/WinSCPDialogs.cpp` (9400+ lines, handles all protocol session UIs)
5. **Certificate validation** uses OpenSSL - should leverage existing patterns or create new validation module
6. **File picker:** No existing file picker pattern found - need to implement Windows API dialogs (`GetOpenFileName`/`GetSaveFileName`)
7. **PrivateKeyEdit pattern:** Uses `TFarEdit` without Load/Save buttons - users type paths manually
8. **Test infrastructure:** No tests/ directory exists - must set up Catch2 from scratch

**Architecture Pattern:**
- Plugin Layer -> Core Layer -> Base Layer -> Third-Party
- Session data flows: `TSessionData` (properties) <-> `TSessionDialog` (UI) <-> XML storage
- Certificate editor should be inline in existing session dialog, not separate component

## Critical Gaps in Original Plan (Corrected)

| Issue | Original | Correction |
|-------|----------|------------|
| WebDAV certs | Not addressed | Add `WebDAVCertificate` property to SessionData |
| Component design | New `CertificateManager` class | Reuse existing patterns or create validation utility |
| UI approach | New "page or panel" | Inline `TFarEdit` + Load/Save buttons in existing dialog |
| File path | `src/windows/*` | `src/NetBox/*` (Far Manager plugin UI layer) |
| Button handler | Separate class methods | Inline handlers in `TSessionDialog` using Windows API |
| Test framework | Assumed exists | Must set up Catch2 first |
| Documentation | Assumed CHANGELOG.md | Use `docs/features/` + `.hlf` help files |
| File picker | Assumed exists | Must implement Windows `GetOpenFileName`/`GetSaveFileName` |
| Validation strictness | Unclear | Allow invalid PEM with warning (user decision) |
| Backward compatibility | Not mentioned | Existing sessions must load correctly without certificate property |
## Tasks

### Phase 1: Core Session Data Extensions

- [ ] **Task 1: Add WebDAV certificate property to SessionData**
  - Add `FWebDAVCertificate` member to `src/core/SessionData.h` (after S3CACertificate pattern)
  - Add `GetWebDAVCertificate()/SetWebDAVCertificate()` accessors
  - Add property declaration using `RWPropertySimple<UnicodeString>` macro
  - Add XML serialization in `SessionData::Load()` and `DoSave()`
  - **Files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`
  - **LOGGING:** Add INFO log on Set for audit trail

- [ ] **Task 2: Integrate WebDAV certificate into WebDAVFileSystem**
  - Locate `TWebDAVFileSystem::InitSession()` in `src/core/WebDAVFileSystem.cpp`
  - Add logic to use session-level certificate if configured, fallback to global
  - Follow existing neon TLS initialization pattern (`ne_ssl_set_verify_cleartext` or similar)
  - **Files:** `src/core/WebDAVFileSystem.cpp`
  - **LOGGING:** DEBUG log showing which certificate source is used

### Phase 2: Dialog UI Implementation

- [ ] **Task 3: Add certificate editor controls to SessionDialog**
  - Locate `TSessionDialog` class in `src/NetBox/WinSCPDialogs.cpp`
  - Add member variables: `TFarEdit* WebDAVCertificateEdit`, `TFarButton* WebDAVCertificateLoadBtn`, `TFarButton* WebDAVCertificateSaveBtn`
  - Mirror existing S3 `S3CACertificateEdit` pattern (see lines 1795-1796, 2579-2581)
  - Add visibility logic: show when protocol is WEBDAV or WEBDAVS (similar to `IsWebDAVProtocol()` usage)
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`
  - **LOGGING:** DEBUG log when controls are shown/hidden

- [ ] **Task 4: Implement Load/Save button handlers**
  - Add `CertificateLoadClick()` and `CertificateSaveClick()` methods to `TSessionDialog`
  - Use Windows `GetOpenFileName()` / `GetSaveFileName()` APIs (project uses WIN32, not VCL dialogs)
  - Filter: `.pem` files and `All files (*.*)`
  - Validate file is readable/writable before returning
  - Load: Read file content, populate Edit control, validate PEM format
  - Save: Write Edit control content to selected file
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`
  - **LOGGING:** INFO on successful load/save; ERROR on failure with GetLastError()

- [ ] **Task 5: Wire certificate data to session dialog**
  - In dialog initialization: populate Edit from `SessionData->WebDAVCertificate`
  - In dialog `OkClick()` / save: update `SessionData->SetWebDAVCertificate()` from Edit
  - Mirror S3 pattern at lines 3655 and 3981
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`
  - **LOGGING:** DEBUG on load/save to session data

### Phase 3: Certificate Validation

- [ ] **Task 6: Add PEM format validation**
  - Create utility function `bool ValidateCertificatePEM(const UnicodeString& Content)`
  - Use OpenSSL `PEM_read_bio_X509()` or `PEM_read_bio_X509_AUX()` to verify parseable
  - Add validation on Load button (warn if invalid, allow override) and Save (warn)
  - Place in `src/core/Certificates.cpp` or `src/nbcore/` if shared
  - **Files:** `src/core/Certificates.cpp`, `src/core/Certificates.hpp` (or `src/include/`)
  - **LOGGING:** WARN on validation failure with specific error

### Phase 4: S3 Parity

- [ ] **Task 7: Add Load/Save buttons for S3 CA Certificate**
  - S3 currently has `S3CACertificateEdit` text field but no Load/Save buttons
  - Add `S3CACertificateLoadBtn` and `S3CACertificateSaveBtn` following same pattern as WebDAV
  - Share handler implementations with WebDAV buttons (extract common logic)
  - **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`

### Phase 5: Testing

- [ ] **Task 8: Add unit tests for PEM validation**
  - Create `tests/core/CertificateValidationTest.cpp`
  - Test cases: valid PEM, invalid PEM, empty string, multiple certificates in one file
  - Use existing test framework (likely custom, check `tests/` directory structure)
  - **Files:** `tests/core/CertificateValidationTest.cpp`

- [ ] **Task 9: Build and verify**
  - Configure with `cmake -S . -B build-RelWithDebugInfo -A x64 ... -DOPT_CREATE_PLUGIN_DIR=ON`
  - Build with `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
  - Fix any W4 warnings (treat as errors)
  - Verify DLL output to `Far3_x64/Plugins/NetBox/`
  - **Files:** None (build verification only)

- [ ] **Task 10: Manual testing**
  - Create WEBDAV session, set certificate path, save, reopen, verify persistence
  - Test Load button: select valid/invalid PEM files
  - Test Save button: write to new file, verify content
  - Test S3 certificate editor (same tests)
  - Test error paths: read-only file, non-existent file, invalid PEM
  - **LOGGING:** Capture verbose logs for review

### Phase 6: Documentation

- [ ] **Task 11: Update user documentation**
  - Add section in appropriate user guide (check `docs/` structure)
  - Document certificate editor UI controls
  - Explain when to use (self-signed certs, custom CA)
  - Add `CHANGELOG.md` entry
  - **Files:** `docs/user-guide/*.md`, `CHANGELOG.md`

## Dependencies

Task 1 -> Task 2 -> Task 3-5 -> Task 7
Task 6 can proceed after Task 3
Task 8-9 depend on Task 5, 7
Task 10 depends on Task 9
Task
 11 depends on Task 10
```

## Success Criteria

- All tasks complete and build passes with zero warnings (MSVC W4)
- WebDAV certificate editor loads/saves PEM files correctly
- S3 certificate editor has Load/Save buttons matching WebDAV functionality
- Session settings persist across dialog invocations (XML round-trip)
- PEM validation accepts valid certificates, rejects malformed input
- Unit tests for PEM validation pass
- Code follows NetBox T/F prefix naming conventions
- Documentation updated with certificate editor usage

## Refinement Summary

| Category | Changes |
|----------|---------|
| **Missing Tasks** | Added Task 7 (S3 parity - S3 editor lacks Load/Save buttons), Task 2 (WebDAV filesystem integration) |
| **Task Improvements** | Corrected file paths (`src/NetBox/` not `src/windows/`), aligned with existing S3 pattern, specified Windows API for file dialogs |
| **Dependency Fixes** | Reordered to reflect actual architecture (SessionData before Dialog before Filesystem) |
| **Removals** | Removed `nbcore::CertificateManager` class idea (redundant with existing patterns) |
| **Scope Corrections** | WebDAV certificate property was missing entirely; S3 only had text field, no buttons |

## Commit Plan

- **Commit 1** (after tasks 1-2): `feat: add WebDAV certificate property and filesystem integration`
- **Commit 2** (after tasks 3-5): `feat: add certificate editor controls to session dialog`
- **Commit 3** (after task 6): `feat: add PEM validation for certificates`
- **Commit 4** (after task 7): `feat: add Load/Save buttons for S3 certificate editor`
- **Commit 5** (after tasks 8-11): `test: add certificate validation tests and documentation`
