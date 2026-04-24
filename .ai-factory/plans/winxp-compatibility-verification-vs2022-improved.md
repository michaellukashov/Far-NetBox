# Implementation Plan: Verify Windows XP Compatibility with VS2022 v143 Toolset (Improved)

**Branch:** feature/winxp-compat  
**Created:** 2026-04-24  
**Improved:** 2026-04-24  
**Mode:** Full (with branch)

---

## Problem Statement

**UPDATED 2026-04-24:** NetBox already has Windows XP compatibility layer ported from Far3 (via `vc_crt_fix_impl.cpp`). However, when building with VS2022 using the v143 toolset, XP compatibility may be broken. Need to verify and ensure that plugins built with modern VS2022 remain compatible with Windows XP.

**Reference:** PR#500 - Restore support for work under winXP when build with MSVC  
https://github.com/michaellukashov/Far-NetBox/pull/500

**Current state:**
- XP compatibility layer EXISTS: `src/NetBox/vc_crt_fix_impl.cpp` (identical to Far3)
- OS version detection EXISTS: `src/base/Common.cpp` (`GetWindowsVersion()`)
- Build config EXISTS: `_WIN32_WINNT=0x0501` in `cmake/NetBox.cmake`
- API shims EXIST: `InitializeCriticalSectionEx`, `CompareStringEx`, `LCMapStringEx`
- VS2022 v143 toolset: May break XP compatibility (needs verification)

**Current behavior:**
- NetBox plugin built with VS2022 v143 toolset may fail on Windows XP
- Modern CRT functions may not have proper XP fallbacks
- XP compatibility layer may not be properly linked/initialized

**Expected behavior:**
- NetBox plugin built with VS2022 v143 toolset runs on Windows XP SP3
- All XP compatibility shims work correctly
- No crashes or missing API errors on XP

---

## Settings

- **Testing:** Yes (manual testing on Windows XP VM required)
- **Logging:** Verbose (log XP shim usage and API calls)
- **Docs:** Yes (document XP compatibility status and build requirements)

---

## Reference Sources

**Primary reference:**
- NetBox existing implementation: `src/NetBox/vc_crt_fix_impl.cpp` (already ported from Far3)
- PR#500: https://github.com/michaellukashov/Far-NetBox/pull/500
- Far Manager 3 source code: `D:\Projects\Far3\far3-master`
- Repository: https://github.com/FarGroup/FarManager.git

**Key files to analyze:**
- `src/NetBox/vc_crt_fix_impl.cpp` - Existing XP API shims
- `src/NetBox/vc_crt_fix.asm` - Assembly wrappers
- `src/base/Common.cpp` - OS version detection
- `cmake/NetBox.cmake` - Build configuration
- `src/windows/GUITools.cpp` - XP-aware code patterns (GetTickCount vs GetTickCount64)

---

## Tasks

### Phase 1: Verification & Analysis

- [ ] **task-1:** Review PR#500 and existing XP compatibility implementation
  - **Files to examine:**
    - `src/NetBox/vc_crt_fix_impl.cpp` (existing XP shims)
    - `src/NetBox/vc_crt_fix.asm` (assembly wrappers)
    - `src/base/Common.cpp` (OS version detection)
    - `cmake/NetBox.cmake` (build configuration)
    - `src/windows/GUITools.cpp` (XP-aware code patterns)
    - PR#500 changes: https://github.com/michaellukashov/Far-NetBox/pull/500
  - **Understand:**
    1. What XP compatibility shims already exist
    2. How they are linked and initialized
    3. What PR#500 fixed or added
    4. Current build configuration for XP support
    5. Existing XP-aware code patterns in the codebase
  - **DONE when:** Documented:
    1. Complete list of existing XP shims and their status
    2. PR#500 changes summary
    3. Current vs required build configuration
    4. Gap analysis: what's missing for VS2022 v143 compatibility
    5. Catalog of XP-aware code patterns for reference
  - **LOGGING:**
    - Log existing shims found with file:line references
    - Use format: `[XPCompat.Review] Found shim: {API} in {file}:{line}`
    - Use INFO level

- [ ] **task-2:** Verify VS2022 v143 toolset XP compatibility
  - **Build test:**
    1. Build NetBox with VS2022 v143 toolset (current default)
    2. Check if `vc_crt_fix_impl.cpp` is properly linked
    3. Verify no linker errors for XP-incompatible CRT functions
    4. Check generated DLL dependencies (no Vista+ APIs)
    5. Verify linker subsystem version (should be 5.01 for XP, not 6.00 for Vista+)
  - **Tools:**
    - `dumpbin /dependents NetBox.dll` - check dependencies
    - `dumpbin /imports NetBox.dll` - check imported functions
    - `dumpbin /headers NetBox.dll` - check subsystem version
    - Dependency Walker or similar tool
  - **DONE when:**
    1. Build completes successfully with v143 toolset
    2. DLL dependencies documented (kernel32, user32, etc.)
    3. No Vista+ API imports detected (or all are shimmed)
    4. `vc_crt_fix_impl.cpp` confirmed linked into DLL
    5. Linker subsystem version verified (should be 5.01 for XP, not 6.00 for Vista+)
    6. Subsystem version checked via `dumpbin /headers NetBox.dll`
  - **LOGGING:**
    - Log build configuration and toolset version
    - Log DLL dependencies and imports
    - Log subsystem version from PE header
    - Use format: `[XPCompat.Build] Toolset: {version}, Subsystem: {version}, Dependencies: {list}`
    - Use INFO level

- [ ] **task-3:** Test on Windows XP SP3 VM
  - **Test environment:**
    - Windows XP SP3 VM (32-bit or 64-bit)
    - Far Manager 3 installed
    - NetBox plugin built with VS2022 v143 toolset
  - **Test scenarios:**
    1. Plugin loads without errors (F11 → Plugins → NetBox)
    2. Check for missing API errors in Event Viewer
    3. Connect to SFTP server (basic SSH functionality)
    4. Connect to FTP server (basic FTP functionality)
    5. Connect to WebDAV server (basic WebDAV functionality)
    6. Connect to S3-compatible storage (if S3 support is XP-compatible)
    7. File transfer operations (upload, download, delete)
    8. Check plugin log for XP shim usage
  - **Pass criteria:** All core functionality works without crashes
  - **DONE when:**
    1. Test matrix shows ≥90% pass rate
    2. No missing API errors (ordinal not found, entry point not found)
    3. Log shows XP shims being used correctly
    4. S3 protocol tested (or documented as XP-incompatible if it fails)
  - **LOGGING:**
    - Log test results with pass/fail status
    - Log any missing API errors from XP
    - Use format: `[XPCompat.Test] Scenario {name}: {pass/fail} - {details}`
    - Use INFO level for pass, ERROR for failures

### Phase 2: Fixes & Improvements (if needed)

- [ ] **task-4:** Fix VS2022 v143 XP compatibility issues (conditional)
  - **Only execute if task-3 found failures**
  - **Potential fixes:**
    1. Add missing API shims to `vc_crt_fix_impl.cpp`
    2. Update CMake to force XP-compatible CRT linking
    3. Add `/SUBSYSTEM:CONSOLE,5.01` linker flag for XP
    4. Ensure `vc_crt_fix.asm` properly exports all wrappers
    5. Review existing XP-aware code: `src/windows/GUITools.cpp:2132` (GetTickCount vs GetTickCount64 pattern)
  - **Reference:** PR#500 for patterns and solutions
  - **DONE when:**
    1. All missing APIs identified in task-3 have shims
    2. Build configuration updated for XP compatibility
    3. Re-test on XP VM shows 100% pass rate
  - **LOGGING:**
    - Log each fix applied with rationale
    - Use format: `[XPCompat.Fix] Applied: {description}`
    - Use INFO level

- [ ] **task-5:** Verify no regression on modern Windows
  - **Test environment:**
    - Windows 10/11 (modern build)
    - Far Manager 3 installed
    - NetBox plugin with XP fixes applied
  - **Test scenarios:** Same as task-3
  - **Pass criteria:** 100% pass rate, no performance degradation
  - **DONE when:**
    1. All tests pass on Windows 10/11
    2. No performance regression vs pre-fix builds
    3. Log shows native API usage (not fallbacks) on modern Windows
  - **LOGGING:**
    - Log test results
    - Use format: `[XPCompat.Regression] Modern Windows: {scenario} - {pass/fail}`
    - Use INFO level

### Phase 3: Documentation & Automation

- [ ] **task-6:** Document XP compatibility status
  - **Files to update:**
    - `AGENTS-Workflows.md` (add XP build verification steps)
    - `AGENTS-Structure.md` (document vc_crt_fix module)
    - Create `docs/windows-xp-compatibility.md` (user-facing)
  - **Content:**
    1. XP compatibility status (supported/tested)
    2. Build requirements (VS2022 v143 toolset works)
    3. List of shimmed APIs
    4. Known limitations on XP (TLS 1.2+, IPv6, etc.)
    5. Troubleshooting XP-specific issues
    6. Reference to PR#500
  - **DONE when:**
    1. Documentation complete and reviewed
    2. XP support status clearly communicated
    3. Build/test procedures documented
  - **LOGGING:**
    - Log documentation files updated
    - Use format: `[XPCompat.Docs] Updated: {file}`
    - Use INFO level

- [ ] **task-7:** Create automated XP compatibility check (optional enhancement)
  - **Purpose:** Catch XP compatibility regressions early in CI/CD
  - **Implementation:**
    1. Create script: `scripts/check-xp-compat.ps1`
    2. Parse `dumpbin /imports NetBox.dll` output
    3. Check for Vista+ APIs (InitializeCriticalSectionEx, CompareStringEx, etc.)
    4. Verify all Vista+ APIs are in the shim list
    5. Fail build if unshimmed Vista+ API detected
    6. Check subsystem version is 5.01 (XP) not 6.00 (Vista+)
  - **Integration:** Can be added to CI later (doesn't block this plan)
  - **DONE when:**
    1. Script created and tested locally
    2. Script correctly identifies Vista+ APIs
    3. Script passes for current NetBox build
    4. Documentation added to AGENTS-Workflows.md
  - **LOGGING:**
    - Log APIs checked and results
    - Use format: `[XPCompat.AutoCheck] Checked {count} imports, found {violations} violations`
    - Use INFO level for pass, ERROR for violations

---

## Commit Plan

- **Commit 1** (after tasks 1-3): `test(xp): verify VS2022 v143 toolset XP compatibility (ref PR#500)`
- **Commit 2** (after tasks 4-5, if needed): `fix(xp): ensure VS2022 v143 builds work on Windows XP`
- **Commit 3** (after task 6): `docs(xp): document XP compatibility status and build requirements`
- **Commit 4** (after task 7, optional): `ci(xp): add automated XP compatibility check script`

---

## Technical Notes

**Existing XP compatibility implementation:**
- `src/NetBox/vc_crt_fix_impl.cpp` - API shims (ported from Far3)
- `src/NetBox/vc_crt_fix.asm` - Assembly wrappers
- `src/base/Common.cpp` - OS version detection
- `cmake/NetBox.cmake` - Build configuration (_WIN32_WINNT=0x0501)
- `src/windows/GUITools.cpp` - XP-aware code patterns (line 2132: GetTickCount vs GetTickCount64)

**Already shimmed APIs:**
- `InitializeCriticalSectionEx` (Vista+) → fallback to `InitializeCriticalSection`
- `CompareStringEx` (Vista+) → fallback to `CompareStringW`
- `LCMapStringEx` (Vista+) → fallback to `LCMapStringW`
- `SetThreadStackGuarantee` (Vista+) → fallback implementation

**VS2022 v143 toolset considerations:**
- Default CRT may link Vista+ functions
- Need `/SUBSYSTEM:CONSOLE,5.01` or `/SUBSYSTEM:WINDOWS,5.01` for XP
- May need additional shims for newer CRT functions
- Subsystem version in PE header must be 5.01 (XP) not 6.00 (Vista+)

**XP-aware code patterns in codebase:**
- `src/windows/GUITools.cpp:2132` - Runtime check: GetTickCount vs GetTickCount64
- Pattern: Check OS version at runtime, use XP-compatible API on old Windows

**Reference:**
- PR#500: https://github.com/michaellukashov/Far-NetBox/pull/500
- Far3 implementation: `D:\Projects\Far3\far3-master\far\vc_crt_fix_impl.cpp`

---

## Edge Cases

- **VS2022 v143 vs v141_xp:** v143 is modern toolset, may need extra configuration for XP
- **XP SP2 vs SP3:** Require SP3 minimum (some APIs differ)
- **64-bit XP:** Rare, focus on 32-bit XP (x86 build)
- **API not available on XP:** Graceful degradation (log warning, disable feature)
- **Dynamic loading fails:** Fallback to XP-compatible alternative
- **TLS/SSL on XP:** Limited to TLS 1.0/1.1 (modern servers may reject)
- **S3 protocol on XP:** May require newer APIs (test and document limitations)
- **Subsystem version mismatch:** PE header shows 6.00 but should be 5.01 for XP

---

## Success Criteria

- [ ] Existing XP compatibility implementation reviewed and understood
- [ ] PR#500 changes analyzed and documented
- [ ] Plugin built with VS2022 v143 toolset loads on Windows XP SP3
- [ ] Core protocols (SFTP, FTP, WebDAV, S3) functional on XP (or limitations documented)
- [ ] No missing API errors on XP (all APIs shimmed or available)
- [ ] Subsystem version verified as 5.01 (XP-compatible)
- [ ] No regression on Windows 10/11
- [ ] XP compatibility status documented (build + user-facing)
- [ ] Automated XP compatibility check script created (optional)
- [ ] Code follows NetBox conventions (CRLF, naming, no warnings)

---

## Improvements Applied (via /aif-improve)

**Date:** 2026-04-24

**Changes:**
1. **Task 2 enhancement:** Added subsystem version verification (5.01 vs 6.00)
2. **Task 3 enhancement:** Added S3 protocol testing to scenario list
3. **Task 4 enhancement:** Added reference to existing XP-aware code patterns
4. **New task 7:** Added optional automated XP compatibility check script
5. **Technical notes:** Added XP-aware code pattern documentation
6. **Edge cases:** Added subsystem version mismatch scenario
7. **Success criteria:** Added subsystem version and S3 protocol verification

**Rationale:**
- Subsystem version is critical for XP compatibility (often overlooked)
- S3 protocol is mentioned in DESCRIPTION.md but missing from test plan
- Existing XP-aware code patterns provide good examples for fixes
- Automated checks prevent future regressions (CI/CD integration)

---

## Next Steps

To start implementation:
```
/aif-implement
```

To view tasks:
```
/tasks
```
