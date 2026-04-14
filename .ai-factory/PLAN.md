# Plan: Embed cacert.pem into C++ source

**Description:** Move the `cacert.pem` certificate bundle into C++ source as an embedded default, update configuration to use embedded content, and remove the external file from git tracking.

**Branch:** N/A (fast mode)

**Created:** 2025-04-12

## Settings

- **Testing:** No (skipped per user request)
- **Logging:** Standard (infrastructure changes don't require verbose logging)
- **Docs:** Not required (self-documenting code change)

## Research Context

Current implementation:
- `cacert.pem` file resides in `src/NetBox/`
- `cmake/Install.cmake` copies it to plugin directory
- `TConfiguration::GetCertificateStorageExpanded()` falls back to `cacert.pem` in plugin directory if no custom path configured
- Used by FTPS (FileZilla) and HTTPS/WebDAV (neon) via `CertificateStorageExpanded` property

## Tasks

- [x] Task 1: Create embedded certificate source files
- [x] Task 2: Modify configuration to use embedded certificates
- [x] Task 3: Update CMake installation rules
- [x] Task 4: Remove cacert.pem from git
- [ ] Task 5: Build verification

### Task 5: Build verification

**Deliverable:** Clean build with no warnings, and verification that SSL/TLS connections still work.

**Steps:**
1. Clean build: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Ensure zero warnings (MSVC W4 level)
3. Manual test: Connect to an FTPS server and HTTPS/WebDAV server to verify certificate validation works
   - Use existing test sessions or create new ones
   - Verify that server certificates are properly validated
   - Check that no errors appear in logs

**Logging:** Verify debug messages show certificate file being created from embedded data

**Dependencies:** Task 4 must be complete

---

**Status:** Pending - Ready for build verification.

### Task 2: Modify configuration to use embedded certificates

**Deliverable:** Update `src/core/Configuration.cpp` - `GetCertificateStorageExpanded()` method to create a temporary file from embedded certificate data when no custom path is configured.

**Implementation:**
- If `FCertificateStorage` is set by user, use it (preserve custom override)
- If empty, check for existing `cacert.pem` file (backward compatibility)
- If no file exists, create a temporary file from `EmbeddedCacertPem`
- Cache the temp file path to avoid recreating on every call
- Log certificate source usage at debug level

**File paths modified:**
- `src/core/Configuration.h` (include Certificates.hpp)
- `src/core/Configuration.cpp` (GetCertificateStorageExpanded implementation)
- `src/core/NeonIntf.cpp` (include Certificates.hpp, simplified logging)
- `src/core/FtpFileSystem.cpp` (include Certificates.hpp)

**Logging:** 
- Log when temp file is created from embedded data
- Log certificate storage path being used

**Dependencies:** Task 1 must be complete

---

✅ **Task 2 completed** - Configuration now uses embedded certificates via temp file.

---

### Task 3: Update CMake installation rules

**Deliverable:** Remove `cacert.pem` from distribution files in `cmake/Install.cmake`.

**Current (line 46):**
```cmake
set(DIST_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/*.lng
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NetBox/cacert.pem
  ...
)
```

**Action:** Delete the line referencing `cacert.pem`. No replacement needed since data is now embedded.

**File paths:**
- `cmake/Install.cmake`

**Logging:** N/A (build script)

**Dependencies:** Task 2 must be complete (to ensure embedded data is used)

---

### Task 4: Remove cacert.pem from git

**Deliverable:** `cacert.pem` is no longer tracked by git.

**Steps:**
```bash
git rm --cached src/NetBox/cacert.pem
```

**Verify:** Check that `.gitignore` (if exists) doesn't need update; the file should be ignored if present in project but untracked.

**Note:** The physical file can remain in working directory temporarily for backward compatibility during transition, but should be deleted from repository history. After this commit, the file is no longer part of git.

**File paths affected:** None (git operation only)

**Dependencies:** Task 3 must be complete

---

### Task 5: Build verification

**Deliverable:** Clean build with no warnings, and manual verification that SSL/TLS connections still succeed.

**Steps:**
1. Clean build: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Ensure zero warnings (MSVC W4 level)
3. Manual test: Connect to an FTPS server and HTTPS/WebDAV server to verify certificate validation works
   - Use existing test sessions or create new ones
   - Verify that server certificates are properly validated
   - Check that no errors appear in logs

**Logging:** Verify `ADF()` messages in debug log show embedded certificate source

** rollback:** If issues found, fix before proceeding

**Dependencies:** Task 4 must be complete

---

## Commit Plan

**Single commit** (5 tasks but logically atomic refactor):

```
refactor(netbox): embed cacert.pem certificate bundle as C++ source

- Add Certificates.hpp/cpp with embedded PEM data
- Modify configuration to use embedded certificates by default
- Update consumers (FtpFileSystem, NeonIntf) to use embedded data
- Remove cacert.pem from CMake distribution and git tracking
- Preserve backward compatibility: custom CertificateStorage still overrides

BREAKING CHANGE: cacert.pem file removed from repository; custom certificate
overrides via configuration still supported. Embedded certificates are now
compiled into the plugin binary.
```

This is a single-commit change since all tasks are tightly coupled and must land together.
