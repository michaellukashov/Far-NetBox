<!-- aif-plan -->
# Windows Certificate Store Authentication (Refined)

**Branch:** feature/windows-cert-auth
**Created:** 2026-04-23
**Type:** Feature
**Priority:** Critical
**Refined:** 2026-04-23 (via /aif-improve)

## Settings

- **Testing:** Yes (Unit + Integration tests required)
- **Logging:** Verbose (DEBUG level for certificate operations)
- **Docs:** Yes (User guide + API documentation)
- **Parallel:** No (fast mode, sequential execution)

## Roadmap Linkage

Milestone: N/A (user skipped linkage or no roadmap present)

## Research Context

Reference: GitHub Issue #509 - User-provided auth certificate
Users need to authenticate using Windows Certificate Store instead of file-based certificates only.
Current implementation only supports DetachedCertificate file path. Need to add store-based selection with thumbprint storage.

## Architecture Context

Dependency flow: Plugin Layer (NetBox/) → Core Layer (core/) → Base Layer (base/) → Third-Party (libs/)
- Changes touch: Core (SecureShell.cpp), SessionData (SessionData.cpp/h), UI (WinSCPDialogs.cpp), Windows-specific (windows/)
- No modifications to libs/ directory (use patches if needed)
- Build with CMake, Unity builds enabled for x86 Release
- RAII pattern required for Win32 certificate handles

## Tasks

### Phase 0: Foundation - Schema & Interfaces

**Task 0.1:** Define session configuration XSD schema
- File: `docs/session-schema.xsd` (new)
- Add `CertSourceType` enum (0=None, 1=File, 2=WindowsStore)
- Add `CertThumbprint` field (pattern `[0-9A-F]{40}`)
- Add `CertStoreLocation` enum (CurrentUser, LocalMachine)
- Version attribute set to 2.0
- Deliverable: XSD file validates with sample XML
- Logging: INFO on schema load, ERROR if validation fails

**Task 0.2:** Create certificate store abstraction header
- File: `src/WindowsCertStore.hpp` (new)
- Class `TWindowsCertStore`: EnumerateCertificates, FindByThumbprint, GetPrivateKeyHandle, ReleaseContext
- Struct `TCertInfo`: Subject, Issuer, ValidFrom, ValidTo, HasPrivateKey, KeyUsage, ThumbprintSHA1
- RAII wrapper `TCertContext` for PCCERT_CONTEXT
- RAII wrapper `TNCryptKeyHandle` for NCRYPT_KEY_HANDLE
- Deliverable: Header compiles, all methods declared
- Logging: DEBUG on enumeration, WARNING on filtered certs
- Depends on: Task 0.1

### Phase 1: Certificate Store Access

**Task 1.0:** CMake integration for new source files
- File: `src/CMakeLists.txt`
- Add `WindowsCertStore.cpp` to NETBOX_SOURCES
- Link `ncrypt.lib` and `crypt32.lib` for Windows Certificate Store APIs
- Deliverable: CMake builds new files without errors
- Logging: N/A
- Depends on: Task 0.2

**Task 1.1:** Implement certificate enumeration
- Files: `src/WindowsCertStore.cpp` (new)
- Open store with `CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, L"MY")`
- Iterate `CertEnumCertificatesInStore`, filter: has private key (`CERT_KEY_PROV_INFO_PROP_ID`) AND Client Authentication EKU (`szOID_PKIX_KP_CLIENT_AUTH`)
- Extract SHA1 thumbprint via `CryptHashCertificate`
- Return vector<TCertInfo>, handle empty store with message "No certificates with private keys found"
- Deliverable: Enumeration returns correct count from test store, <100ms for 100+ certs
- Logging: DEBUG each cert processed, INFO count of filtered/eligible certs
- Depends on: Task 1.0

**Task 1.2:** Implement thumbprint lookup
- File: `src/WindowsCertStore.cpp`
- Method `FindByThumbprint(std::wstring thumbprint)` validates format (40 uppercase hex), converts to byte array
- Use `CertFindCertificateInStore` with `CERT_FIND_SHA1_HASH`
- Return PCCERT_CONTEXT wrapped in TCertContext or null
- Log error `CERT_NOT_FOUND` with thumbprint if not found
- Deliverable: Lookup finds exact match, invalid format throws EConvertError
- Logging: WARNING on not found, ERROR on invalid format
- Depends on: Task 1.1

**Task 1.3:** Implement private key handle acquisition
- File: `src/WindowsCertStore.cpp`
- Method `GetPrivateKeyHandle(PCCERT_CONTEXT cert)` returns NCRYPT_KEY_HANDLE
- Try `NCryptOpenKey` first (CNG), fallback to `CryptAcquireCertificatePrivateKey` (CryptoAPI)
- Handle smart card PIN prompt via `NCRYPT_PIN_PROPERTY`
- Errors: `NTE_BAD_KEYSET`, `NTE_NO_KEY`, `SCARD_E_NO_SMARTCARD` → specific error codes
- Deliverable: Returns valid handle for test cert, retry logic for transient errors (3x 500ms)
- Logging: DEBUG on key open, WARNING on PIN retry, ERROR on permanent failures
- Depends on: Task 1.2

**Task 1.4:** Implement RAII wrappers and error policy
- File: `src/WindowsCertStore.cpp`
- `TCertContext` destructor calls `CertFreeCertificateContext`
- `TNCryptKeyHandle` destructor calls `NCryptFreeKey`
- Enum `ECertStoreError` mapping Win32/NCrypt errors to user-friendly messages
- Fallback behavior spec: store access failure → error, not silent fallback to file (user must config)
- Deliverable: All test cases clean with Valgrind/Dr.Memory (no leaks)
- Logging: INFO on cleanup, ERROR on double-free attempts
- Depends on: Task 1.3

**Task 1.5:** Error handling for store access failures
- Files: `src/WindowsCertStore.cpp`
- Handle `CERT_E_*`, `NCrypt_*`, `SCARD_E_*` error codes gracefully
- Implement fallback behavior spec: store access failure → error, not silent fallback to file
- Return user-friendly error messages via `ECertStoreError` enum
- Deliverable: All known error codes handled with appropriate user messages
- Logging: ERROR on unhandled errors, WARNING on recoverable failures
- Depends on: Task 1.4

### Phase 2: Session Data & Configuration

**Task 2.1:** Extend TSessionData with certificate source fields
- Files: `src/SessionData.h`, `src/SessionData.cpp`
- Add members: `FCertSourceType: TCertSourceType` (default cstNone), `FCertThumbprint: UnicodeString`, `FCertStoreLocation: TCertStoreLocation` (default CurrentUser)
- Properties with validation: `SetCertThumbprint` enforces length=40, hex-only, uppercase; `SetCertSourceType` enforces consistency
- **Mutual exclusivity:** Setting `CertThumbprint` clears `DetachedCertificate`, and vice versa (implementation detail)
- Deliverable: Session loads with defaults, invalid thumbprint throws EConvertError
- Logging: DEBUG on property set, WARNING on validation failure
- Depends on: Task 1.5

**Task 2.2:** Update session save/load with version handling
- File: `src/SessionData.cpp` (SaveToStorage, LoadFromStorage)
- Save: write `<CertSourceType>`, `<CertThumbprint>`, `<CertStoreLocation>` to XML section
- Load: read with defaults (0, empty, CurrentUser); if `DetachedCertificate` present and CertThumbprint empty, set CertSourceType=File (migration)
- Version attribute: if version < 2, infer CertSourceType from presence of DetachedCertificate
- Deliverable: Round-trip preserves all fields, legacy sessions load correctly
- Logging: INFO on migration, WARNING on unknown version
- Depends on: Task 2.1

**Task 2.4:** Legacy session migration validation
- Files: `src/SessionData.cpp` (existing test or manual test)
- Verify old sessions with `DetachedCertificate` load correctly with `CertSourceType=File`
- Verify sessions with both `DetachedCertificate` and `CertThumbprint` set use proper precedence
- Test edge case: corrupted thumbprint values in existing sessions
- Deliverable: All legacy formats load without crash, migration logs correctly
- Logging: INFO on legacy format detected
- Depends on: Task 2.2

**Task 2.3:** Update protocol logging and UI strings
- Files: `src/SessionInfo.cpp`, `src/FarPluginStrings.cpp`
- Add `MSG_CERT_SOURCE_TYPE`, `MSG_CERT_THUMBPRINT` constants
- Format log line: include cert source and thumbprint (masked? no, thumbprint is not secret)
- **Internationalization:** Follow existing FarPluginStrings.cpp pattern for new string IDs
- Deliverable: Session info shows correct certificate info
- Logging: INFO on session start with certificate details
- Depends on: Task 2.2

### Phase 3: UI - Certificate Selection

**Task 3.1:** Create certificate picker dialog
- File: `src/windows/WinSCPDialogs.cpp`
- Modal dialog with list view: columns Subject, Issuer, Valid To, Key Usage
- Filter text box (subject substring, case-insensitive)
- Buttons: Select (stores thumbprint in session), Cancel, Details (opens details dialog)
- On show: call `TWindowsCertStore::EnumerateCertificates`, filter by ClientAuth EKU
- Double-click = Select
- Deliverable: Picker shows only eligible certs, selection populates session thumbprint
- Logging: DEBUG on dialog open, INFO on certificate selected
- Depends on: Task 2.3, Task 1.5

**Task 3.2:** Create certificate details dialog
- File: `src/windows/WinSCPDialogs.cpp`
- Show: Version, Serial Number, Subject (all RDNs), Issuer, Validity (from/to), Key Algorithm, Key Length, EKU list
- "Copy Thumbprint" button copies SHA1 to clipboard
- Deliverable: All X.509 fields display correctly
- Logging: DEBUG on open, INFO on copy to clipboard
- Depends on: Task 3.1

**Task 3.3:** Modify session options dialog - certificate source radio buttons
- File: `src/windows/WinSCPDialogs.cpp`
- Radio buttons: "Use certificate file" (existing) vs "Use Windows Certificate Store" (new)
- Logic: File → enable file picker, store path in DetachedCertificate, set CertSourceType=File; Store → disable file, enable picker button, set CertSourceType=WindowsStore on selection
- Validation on OK: if neither selected, show `MSG_NEED_CERTIFICATE` error
- Deliverable: Radio switching works, session config saves correctly
- Logging: DEBUG on radio change, INFO on save
- Depends on: Task 3.2

**Task 3.4:** Implement PIN prompt dialog for smart cards
- File: `src/windows/WinSCPDialogs.cpp`
- Dialog: "Enter PIN for certificate [Subject]" with masked password box, Show PIN checkbox optional
- Cache: store PIN in `TSessionData->FPassword` with `FPasswordIsPin=true` flag (new bool), clear on session logout
- Retry: up to 3 attempts, show "X attempts remaining"; 3rd failure aborts with `MSG_CERT_PIN_FAILED`
- Smart card removal: detect `SCARD_E_NO_SMARTCARD` during auth, show "Insert smart card and retry"
- Deliverable: PIN prompt appears for cert with key protection, retry works
- Logging: WARNING on wrong PIN, ERROR on final failure, INFO on success
- Depends on: Task 3.3

### Phase 4: SSH Authentication Integration

**Task 4.1:** Modify SSH authentication to branch on certificate source
- File: `src/core/SecureShell.cpp`
- In `prepare_publickey` callback: check `SessionData->CertSourceType`
- If `cstWindowsStore`: call `TWindowsCertStore::FindByThumbprint(SessionData->CertThumbprint)`
- If cert not found: log `SESSION_CERT_NOT_FOUND`, fail auth (no fallback to file unless explicitly configured)
- If found: obtain `NCRYPT_KEY_HANDLE`, convert to `EVP_PKEY*`
- Deliverable: Authentication succeeds with store cert, fails with clear error if missing
- Logging: DEBUG on store lookup, INFO on cert found, ERROR on not found
- Depends on: Task 3.4, Task 1.5

**Task 4.2:** Implement EVP_PKEY conversion from NCrypt key handle
- File: `src/PuttyIntf.cpp`
- Function `EVP_PKEY* CreateOpenSSLEVPFromNCrypt(NCRYPT_KEY_HANDLE hKey)`
- Extract algorithm via `NCRYPT_ALGORITHM_PROPERTY`
- Build OpenSSL key: RSA → populate RSA struct with n,e,d,p,q,dmp1,dmq1,iqmp; ECDSA → EC_KEY with curve and scalar
- Return `EVP_PKEY*` with correct type; caller frees with `EVP_PKEY_free`
- Zero all temporary buffers with `SecureZeroMemory`
- **Thread-safety:** NCrypt calls must be serialized (use critical section if called from multiple threads)
- Deliverable: Conversion produces valid EVP_PKEY that can sign test data
- Logging: DEBUG on conversion, ERROR on unsupported algorithm
- Depends on: Task 1.5

**Task 4.3:** Wire up smart card PIN caching and retry
- Files: `src/core/SecureShell.cpp`, `src/SessionData.h`
- If `NCryptOpenKey` returns `NTE_SILENT_CONTEXT` or needs PIN, trigger UI via `Terminal->QueryUser()` (or equivalent)
- Cache PIN in `TSessionData->FPassword` with `FPasswordIsPin=true`; clear on `Disconnect()`
- Detect `SCARD_E_NO_SMARTCARD` during signing → show error, prompt to re-authenticate
- Deliverable: Smart card auth works with PIN caching, removal detected
- Logging: INFO on PIN cache set/clear, WARNING on card removal
- Depends on: Task 4.2, Task 3.4

**Task 4.4:** Integration testing with OpenSSH server
- File: `tests/integration/ssh_cert_auth_test.cpp` (new)
- Spin up OpenSSH server in Docker with test CA; import test cert into Windows store via script
- Test: NetBox connects using thumbprint → success within 5 seconds
- Test: Wrong thumbprint → fails with `SESSION_CERT_NOT_FOUND`
- Test: File-based cert still works (regression)
- Deliverable: Integration tests pass on Windows 10/11, Server 2019
- Logging: Test output with RESULTS: PASS/FAIL
- Depends on: Task 4.3

### Phase 5: Testing & Validation

**Task 5.1:** Unit tests for certificate store wrapper
- Files: `tests/unit/WindowsCertStore_test.cpp`, `tests/unit/SessionDataCert_test.cpp`
- Test enumeration filtering (private key + client auth EKU)
- Test FindByThumbprint valid/invalid format
- Test GetPrivateKeyHandle success and errors (no key, access denied)
- Test SessionData thumbprint validation
- Test migration from DetachedCertificate
- Deliverable: All unit tests pass on CI (x86/x64/ARM64)
- Logging: Test framework output (gtest)
- Depends on: Task 4.4

**Task 5.2:** Performance benchmarks
- File: `tests/performance/cert_store_perf.cpp` (new)
- Measure enumeration with 100+ certs: target <100ms
- Measure auth latency overhead vs file-based: target <50ms
- Report results in test output
- Deliverable: Performance meets targets on Windows 10/11
- Logging: Benchmark timings to stdout
- Depends on: Task 5.1

**Task 5.3:** Security validation
- Manual test: use ProcMon to verify no private key material written to disk during auth
- Memory scan after session logout: verify PIN and key handles cleared
- **PIN timeout:** Verify PIN is cleared after session timeout (configurable, default 30 minutes)
- Fuzz test thumbprint input with malformed strings (nulls, non-hex, 41 chars) → no crash
- Deliverable: Security checklist signed off
- Logging: Test report in docs/security-validation.md
- Depends on: Task 5.2

**Task 5.4:** Documentation updates
- File: `docs/user-guide/certificate-auth.md` (new)
- Screenshots of certificate picker dialog
- Steps to configure Windows Certificate Store auth
- Troubleshooting section (common errors, PIN issues)
- Update changelog with feature flag `FEATURE_WINDOWS_CERT_STORE`
- Deliverable: User guide section complete, screenshots in docs/assets/
- Logging: N/A
- Depends on: Task 5.3

## Commit Plan

**Commit 1:** Schema & interfaces (Tasks 0.1, 0.2)
- Add XSD, update CMake to install it
- Add WindowsCertStore.hpp with RAII wrappers and error enum

**Commit 2:** CMake integration & certificate enumeration (Tasks 1.0, 1.1)
- Add WindowsCertStore.cpp to CMake
- Link ncrypt.lib, crypt32.lib
- Implement certificate enumeration

**Commit 3:** Thumbprint lookup & key acquisition (Tasks 1.2, 1.3)
- Implement thumbprint lookup
- Implement private key handle acquisition

**Commit 4:** RAII & error handling (Tasks 1.4, 1.5)
- RAII wrappers, error mapping
- Link ncrypt.lib, crypt32.lib

**Commit 5:** Session data changes (Tasks 2.1, 2.2, 2.4, 2.3)
- Extend TSessionData, update save/load
- Add protocol logging
- Legacy migration validation

**Commit 6:** UI - picker and details dialogs (Tasks 3.1, 3.2, 3.3)
- Certificate picker, details dialog, radio buttons in session options

**Commit 7:** PIN prompt and caching (Task 3.4)
- PIN dialog, retry logic, cache management

**Commit 8:** SSH auth integration & conversion (Tasks 4.1, 4.2, 4.3)
- SecureShell branching, CreateOpenSSLEVPFromNCrypt, PIN caching wire-up

**Commit 9:** Integration & performance tests (Tasks 4.4, 5.1, 5.2)
- OpenSSH test server setup script, unit tests, benchmarks

**Commit 10:** Security validation & documentation (Tasks 5.3, 5.4)
- Security tests, user guide, changelog

## Verification Checklist

- [ ] All unit tests pass on Windows (x86/x64/ARM64)
- [ ] Integration tests pass against OpenSSH 8.0+ server with certificate auth
- [ ] Performance: enumeration <100ms with 100+ certs, auth overhead <50ms
- [ ] Memory analysis: no private key material leakage (Dr. Memory clean)
- [ ] Fuzzing: malformed thumbprint does not crash
- [ ] Legacy file-based sessions load and connect without regression
- [ ] Session import/export preserves new fields exactly
- [ ] Smart card PIN prompt appears when needed, retry works, 3rd failure aborts
- [ ] Documentation builds without errors, screenshots present
- [ ] Code review: all SecureZeroMemory placements correct, no warnings with /W4

## References

- Issue: https://github.com/michaellukashov/Far-NetBox/issues/509
- Windows CNG: https://learn.microsoft.com/en-us/windows/win32/seccng/cng-portal
- Certificate Store: https://learn.microsoft.com/en-us/windows/win32/api/wincrypt/
- X.509 EKU OIDs: https://learn.microsoft.com/en-us/windows/win32/seccrypto/object-identifiers
- OpenSSL EVP: Existing PuttyIntf.cpp patterns
- Smart Card: NIST SP 800-73-4
- Architecture: .ai-factory/ARCHITECTURE.md