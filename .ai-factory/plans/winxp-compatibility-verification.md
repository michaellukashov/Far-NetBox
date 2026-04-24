# Windows XP Compatibility Verification Plan

**Branch:** N/A (verification task)  
**Created:** 2026-04-24  
**Mode:** Fast

## Settings

- **Testing:** Yes — verification tests required
- **Logging:** Verbose — detailed analysis and findings
- **Docs:** No — verification report only

## Overview

Comprehensive verification of NetBox plugin compatibility with Windows XP SP3. The plugin is built with Visual Studio 2022 (v143 toolset) targeting Windows XP (_WIN32_WINNT=0x0501) with static CRT linking.

**Plugin Details:**
- **Type:** Far Manager 3.0 plugin DLL (NetBox.dll)
- **Host Application:** Far Manager 3.0 (x86/x64 versions)
- **Build Configuration:** 
  - Compiler: MSVC 2022 (v143 toolset)
  - Runtime: Static CRT (MultiThreaded)
  - Target: _WIN32_WINNT=0x0501 (Windows XP)
  - Platforms: x86, x64, ARM64
  - Standard: C++17

## Tasks

### Phase 1: Binary Analysis

#### Task 1: PE Header and Subsystem Verification
**Files:** `build-RelWithDebugInfo/src/NetBox.dll`, `Far3_x86/Plugins/NetBox/NetBox.dll`, `Far3_x64/Plugins/NetBox/NetBox.dll`

Analyze PE headers to verify Windows XP compatibility markers:
1. Extract PE header information using available tools (objdump, PE analysis tools)
2. Verify `MajorOperatingSystemVersion` and `MinorOperatingSystemVersion` fields
3. Check subsystem version (should be 5.01 for Windows XP)
4. Verify subsystem type (WINDOWS)
5. Document linker version and timestamp

**Expected Results:**
- Subsystem version: 5.01 (Windows XP)
- Subsystem type: IMAGE_SUBSYSTEM_WINDOWS_GUI or IMAGE_SUBSYSTEM_WINDOWS_CUI
- No Vista+ (6.0+) subsystem requirements

**Logging:**
- Log all PE header fields to verification report
- Flag any version mismatches with ERROR level
- Document findings in structured format

#### Task 2: Dependency Analysis (Import Table)
**Files:** `build-RelWithDebugInfo/src/NetBox.dll`

Extract and analyze all DLL dependencies:
1. Parse import table to list all imported DLLs
2. For each DLL, verify availability in Windows XP SP3:
   - kernel32.dll, user32.dll, advapi32.dll (core Windows APIs)
   - ws2_32.dll (networking)
   - crypt32.dll (cryptography)
   - shlwapi.dll (shell utilities)
   - oleaut32.dll (COM/OLE)
3. Check for problematic dependencies:
   - api-ms-win-*.dll (API sets, Vista+)
   - ucrtbase.dll (Universal CRT, Windows 10+)
   - msvcp140.dll, vcruntime140.dll (VS2015+ runtime, requires KB updates on XP)
4. Analyze delay-loaded DLLs (ws2_32.dll, oleaut32.dll, shlwapi.dll, crypt32.dll per cmake config)

**Expected Results:**
- Static CRT linking → no msvcr*.dll or msvcp*.dll dependencies
- Only Windows XP-compatible system DLLs
- Delay-loaded DLLs properly configured

**Logging:**
- List all imported DLLs with XP compatibility status
- Flag Vista+ DLLs with ERROR level
- Document delay-load configuration

#### Task 3: API Function Compatibility Check
**Files:** `build-RelWithDebugInfo/src/NetBox.dll`

Analyze imported API functions for Windows XP compatibility:
1. Extract all imported functions from each DLL
2. Cross-reference against Windows XP API availability:
   - Check for Vista+ APIs (GetTickCount64, InitOnceExecuteOnce, etc.)
   - Check for Windows 7+ APIs (SetThreadDescription, etc.)
   - Check for Windows 8+ APIs (GetSystemTimePreciseAsFileTime, etc.)
3. Verify cryptography APIs (crypt32.dll) are XP-compatible
4. Check networking APIs (ws2_32.dll) for Vista+ additions
5. Analyze OpenSSL 3.x compatibility with XP (known issue: may require XP-specific patches)

**Expected Results:**
- All imported functions exist in Windows XP SP3
- No Vista+ API usage
- OpenSSL 3.x functions compatible or properly abstracted

**Logging:**
- List all imported functions by DLL
- Flag incompatible APIs with ERROR level and suggest alternatives
- Document OpenSSL 3.x compatibility findings

### Phase 2: Runtime Library Analysis

#### Task 4: CRT and C++ Runtime Verification
**Files:** `cmake/NetBox.cmake`, `build-RelWithDebugInfo/src/NetBox.dll`

Verify static CRT linking and C++17 runtime compatibility:
1. Confirm CMAKE_MSVC_RUNTIME_LIBRARY is set to MultiThreaded (static)
2. Verify no dynamic CRT dependencies (msvcr*.dll, msvcp*.dll, vcruntime*.dll)
3. Check C++17 standard library usage:
   - std::filesystem (requires Vista+ or XP with KB updates)
   - std::optional, std::variant, std::string_view (header-only, should be OK)
   - std::shared_mutex (requires Vista+)
4. Analyze exception handling model (SEH vs C++ exceptions)
5. Check RTTI usage (enabled/disabled)

**Expected Results:**
- Static CRT successfully linked
- No dynamic runtime dependencies
- C++17 features limited to XP-compatible subset
- Exception handling compatible with XP

**Logging:**
- Document CRT configuration
- List C++17 features used and their XP compatibility
- Flag problematic runtime dependencies with ERROR level

#### Task 5: Third-Party Library Compatibility
**Files:** `libs/openssl-3/`, `libs/putty/`, `libs/filezilla/`, `libs/neon/`, `libs/libs3/`

Analyze third-party library Windows XP compatibility:
1. **OpenSSL 3.x:**
   - Check if XP patches are applied (0001-openssl-apply-NetBox-patches.patch)
   - Verify no Vista+ crypto APIs used
   - Check for GetTickCount64 usage (Vista+, common OpenSSL issue)
2. **PuTTY 0.81:**
   - Verify XP compatibility (PuTTY officially supports XP)
   - Check for Vista+ networking APIs
3. **FileZilla 2.2.32:**
   - Verify XP compatibility (old version, should be compatible)
4. **neon (WebDAV):**
   - Check SSL/TLS implementation compatibility
5. **libs3 (S3):**
   - Verify no modern Windows APIs used

**Expected Results:**
- OpenSSL 3.x patches applied and XP-compatible
- All third-party libraries XP-compatible or patched
- No Vista+ API usage in dependencies

**Logging:**
- Document each library's XP compatibility status
- List applied patches and their purpose
- Flag unpatched Vista+ API usage with ERROR level

### Phase 3: Build Configuration Review

#### Task 6: Compiler and Linker Flags Audit
**Files:** `cmake/NetBox.cmake`, `CMakeLists.txt`

Review build configuration for XP compatibility:
1. Verify _WIN32_WINNT=0x0501 is set globally
2. Check linker subsystem version: /SUBSYSTEM:WINDOWS,5.01
3. Review NODEFAULTLIB directives (ensure no dynamic CRT sneaks in)
4. Verify delay-load configuration for optional DLLs
5. Check for /MANIFEST:NO (avoid Vista+ manifest requirements)
6. Review optimization flags (/GL, /LTCG) for XP compatibility

**Expected Results:**
- _WIN32_WINNT=0x0501 consistently applied
- Linker subsystem version explicitly set to 5.01
- No conflicting runtime library directives
- Delay-load properly configured

**Logging:**
- Document all XP-related compiler/linker flags
- Flag missing or incorrect flags with WARN level
- Suggest corrections for any issues

### Phase 4: Test Plan Development

#### Task 7: Virtual Machine Test Environment Setup
**Files:** N/A (documentation task)

Create detailed VM setup instructions for Windows XP SP3 testing:
1. **VM Configuration:**
   - OS: Windows XP Professional SP3 (x86)
   - RAM: 512 MB minimum, 1 GB recommended
   - Disk: 10 GB
   - Network: NAT or Bridged
2. **Prerequisites Installation:**
   - Far Manager 3.0 (x86 version for XP)
   - Visual C++ 2022 redistributables (if any dynamic dependencies found)
   - Windows XP hotfixes (KB2813430 for TLS 1.2 support if needed)
3. **Deployment Steps:**
   - Copy NetBox.dll to Far Manager plugins directory
   - Copy language files (.lng)
   - Copy any required configuration files
4. **Test Data Preparation:**
   - SFTP test server credentials
   - FTP test server credentials
   - WebDAV test endpoint
   - S3 test bucket (optional)

**Expected Results:**
- Complete VM setup guide
- Deployment checklist
- Test data configuration

**Logging:**
- Document VM configuration in verification report
- List all prerequisites and installation steps

#### Task 8: Functional Test Checklist
**Files:** N/A (documentation task)

Create comprehensive functional test checklist:
1. **Plugin Loading:**
   - Far Manager starts without errors
   - NetBox appears in F11 plugin menu
   - Plugin info displays correctly
2. **Session Management:**
   - Create new SFTP session
   - Create new FTP session
   - Create new WebDAV session
   - Save and load session configurations
3. **File Operations:**
   - Connect to remote server
   - List directory contents
   - Upload file
   - Download file
   - Delete file
   - Rename file
   - Create directory
4. **Error Handling:**
   - Invalid credentials
   - Network timeout
   - Connection refused
   - File not found
5. **Stability:**
   - No crashes during normal operation
   - No memory leaks (monitor with Task Manager)
   - Graceful disconnect

**Expected Results:**
- Complete test checklist with pass/fail criteria
- Expected behavior for each test case
- Error handling verification steps

**Logging:**
- Document test checklist in verification report
- Define pass/fail criteria for each test

### Phase 5: Compatibility Assessment

#### Task 9: Compatibility Report Generation
**Files:** `.ai-factory/reports/winxp-compatibility-report.md`

Synthesize all findings into comprehensive compatibility report:
1. **Executive Summary:**
   - Overall compatibility verdict: Compatible / Partially Compatible / Incompatible
   - Critical issues count
   - Recommended actions
2. **Binary Analysis Results:**
   - PE header findings
   - Dependency analysis summary
   - API compatibility summary
3. **Runtime Analysis Results:**
   - CRT configuration
   - C++17 feature usage
   - Third-party library status
4. **Build Configuration Review:**
   - Compiler/linker flags assessment
   - Configuration recommendations
5. **Test Plan:**
   - VM setup guide
   - Functional test checklist
6. **Risk Assessment:**
   - High-risk issues (blockers)
   - Medium-risk issues (workarounds available)
   - Low-risk issues (cosmetic)
7. **Recommendations:**
   - Required fixes before XP deployment
   - Optional improvements
   - Testing strategy

**Expected Results:**
- Structured compatibility report
- Clear verdict with justification
- Actionable recommendations

**Logging:**
- Generate final report with all findings
- Include evidence for each conclusion
- Provide fix recommendations with priority levels

#### Task 10: Remediation Plan (if needed)
**Files:** `.ai-factory/plans/winxp-remediation.md`

If incompatibilities are found, create remediation plan:
1. **Critical Issues:**
   - Vista+ API usage → find XP alternatives or conditional compilation
   - Missing DLL dependencies → static linking or bundling
   - OpenSSL 3.x incompatibilities → apply additional patches
2. **Build Configuration Fixes:**
   - Add /SUBSYSTEM:WINDOWS,5.01 if missing
   - Fix _WIN32_WINNT inconsistencies
   - Adjust linker flags
3. **Code Modifications:**
   - Replace Vista+ APIs with XP equivalents
   - Add runtime version checks for optional features
   - Implement fallback mechanisms
4. **Testing Validation:**
   - Re-test on Windows XP VM after fixes
   - Verify no regressions on modern Windows

**Expected Results:**
- Prioritized fix list
- Implementation guidance for each fix
- Validation criteria

**Logging:**
- Document each remediation step
- Track fix implementation status
- Log validation results

## Commit Plan

Single commit after verification complete:
```
docs(compat): add Windows XP compatibility verification report

- Analyze PE headers and subsystem version
- Verify DLL dependencies and API compatibility
- Review CRT configuration and C++17 usage
- Assess third-party library XP compatibility
- Create VM test plan and functional checklist
- Generate comprehensive compatibility report
- Provide remediation plan if issues found
```

## Success Criteria

1. **Binary Analysis Complete:**
   - PE headers analyzed
   - All dependencies documented
   - API compatibility verified
2. **Runtime Analysis Complete:**
   - CRT configuration verified
   - C++17 features assessed
   - Third-party libraries evaluated
3. **Test Plan Ready:**
   - VM setup guide created
   - Functional test checklist complete
4. **Compatibility Report Generated:**
   - Clear verdict provided
   - Evidence-based conclusions
   - Actionable recommendations
5. **Remediation Plan (if needed):**
   - Critical issues identified
   - Fix guidance provided
   - Validation criteria defined

## Notes

- This is a verification and analysis task, not implementation
- Focus on evidence-based assessment using binary analysis tools
- No assumptions about code — analyze compiled binaries only
- Provide clear, actionable recommendations
- If incompatibilities found, create separate implementation plan for fixes
