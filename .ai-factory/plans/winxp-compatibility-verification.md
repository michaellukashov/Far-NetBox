# Windows XP Compatibility Verification Plan

**Branch:** N/A (implementation + verification task)
**Created:** 2026-04-24
**Updated:** 2026-04-25
**Mode:** Fast

## Settings

- **Testing:** Yes — verification tests required
- **Logging:** Verbose — detailed analysis and findings
- **Docs:** No — verification report only

## Overview

Comprehensive Windows XP compatibility implementation and verification for NetBox plugin. Based on Far3's proven `vc_crt_fix_impl.cpp` pattern, this plan implements a compatibility layer for 18 Vista+ APIs, then verifies the plugin works on Windows XP SP3.

The plugin is built with Visual Studio 2022 (v143 toolset) targeting Windows XP (_WIN32_WINNT=0x0501) with static CRT linking.

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

### Phase 0: Implementation

#### Task 0.1: Verify OpenSSL XP Patches Applied
**Files:** `libs/openssl-3/`, `libs/openssl-3/0001-openssl-apply-NetBox-patches.patch`

Verify OpenSSL 3.x patches are applied before implementing compatibility layer:
1. Check if patch file exists: `libs/openssl-3/0001-openssl-apply-NetBox-patches.patch`
2. Verify patch is applied: `git -C libs/openssl-3 log --oneline | grep -i netbox`
3. Search for GetTickCount64 usage: `grep -r GetTickCount64 libs/openssl-3/`
4. Search for other Vista+ APIs:
   - InitOnceExecuteOnce
   - SRWLock functions (AcquireSRWLockExclusive, etc.)
   - Condition variable functions
5. Document patch contents and what it fixes
6. If patch not applied, apply it: `git -C libs/openssl-3 apply -p3 ../0001-openssl-apply-NetBox-patches.patch`

**Expected Results:**
- Patch file exists and is applied
- No GetTickCount64 usage in OpenSSL code
- No Vista+ synchronization primitives
- OpenSSL builds without Vista+ API dependencies

**Logging:**
- Document patch application status
- List Vista+ APIs found (if any) with ERROR level
- Document patch purpose and changes

**Blocks:** Task 0.2 (need to know which APIs require stubs)

#### Task 0.2: Create Windows XP Compatibility Layer Implementation
**Files:** `src/NetBox/vc_crt_fix_impl.cpp`, `src/NetBox/vc_crt_fix.asm`

Implement XP compatibility stubs for 18 Vista+ APIs following Far3's proven pattern:

**Pattern to follow (from Far3 vc_crt_fix_impl.cpp):**
```cpp
template<typename T>
static T GetFunctionPointer(const wchar_t* ModuleName, const char* FunctionName, T Replacement)
{
    const auto Module = GetModuleHandleW(ModuleName);
    const auto Address = Module? GetProcAddress(Module, FunctionName) : nullptr;
    return Address? reinterpret_cast<T>(reinterpret_cast<void*>(Address)) : Replacement;
}

#define CREATE_FUNCTION_POINTER(ModuleName, FunctionName)\
    static const auto Function = GetFunctionPointer(ModuleName, #FunctionName, &implementation::FunctionName)

extern "C" RETURN_TYPE WINAPI FunctionNameWrapper(PARAMS)
{
    struct implementation
    {
        static RETURN_TYPE WINAPI FunctionName(PARAMS)
        {
            // XP fallback implementation or ERROR_CALL_NOT_IMPLEMENTED
        }
    };
    CREATE_FUNCTION_POINTER(modules::kernel32, FunctionName);
    return Function(args...);
}
```

**APIs to implement (18 total):**

1. **File System APIs:**
   - `CreateSymbolicLinkW` → return FALSE, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)
   - `GetFileInformationByHandleEx` → return FALSE, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)
   - `SetFileInformationByHandle` → return FALSE, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)

2. **Threadpool APIs (8 functions):**
   - `CreateThreadpoolWait` → return NULL, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)
   - `SetThreadpoolWait` → no-op stub
   - `CloseThreadpoolWait` → no-op stub
   - `CreateThreadpoolTimer` → return NULL, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)
   - `SetThreadpoolTimer` → no-op stub
   - `WaitForThreadpoolTimerCallbacks` → no-op stub
   - `CloseThreadpoolTimer` → no-op stub
   - `FreeLibraryWhenCallbackReturns` → no-op stub

3. **Time APIs:**
   - `GetTickCount64` → fallback to GetTickCount() (32-bit, wraps every 49.7 days)

4. **Processor APIs:**
   - `GetCurrentProcessorNumber` → return 0 (single processor fallback)
   - `FlushProcessWriteBuffers` → call FlushInstructionCache(GetCurrentProcess(), NULL, 0)

5. **Synchronization APIs:**
   - `CreateSemaphoreExW` → fallback to CreateSemaphoreW(NULL, initial, max, name)
   - `CreateEventExW` → fallback to CreateEventW(NULL, manual, initial, name)
   - `InitOnceExecuteOnce` → return FALSE, SetLastError(ERROR_CALL_NOT_IMPLEMENTED)

6. **Locale APIs:**
   - `GetLocaleInfoEx` → fallback to GetLocaleInfoW(LOCALE_USER_DEFAULT, ...)

**Implementation steps:**
1. Add wrapper functions to `src/NetBox/vc_crt_fix_impl.cpp`
2. Follow exact pattern from Far3 reference code
3. Use `modules::kernel32` namespace for module names
4. Implement XP-compatible fallbacks where possible (GetTickCount64 → GetTickCount)
5. Return ERROR_CALL_NOT_IMPLEMENTED for features that can't be emulated
6. Add comments explaining each stub's behavior

**Expected Results:**
- 18 wrapper functions implemented
- All functions follow GetFunctionPointer pattern
- XP fallbacks used where possible
- ERROR_CALL_NOT_IMPLEMENTED for unimplementable features
- Code compiles without warnings

**Logging:**
- Document each API stub and its behavior
- Note which APIs have fallbacks vs. stubs
- Log any implementation challenges

**Blocks:** Task 0.3, Task 0.4

#### Task 0.3: Update ASM Export Definitions
**Files:** `src/NetBox/vc_crt_fix.asm`

Add ASM export declarations for all 18 new wrapper functions:

**Pattern (from existing vc_crt_fix.asm):**
```asm
HOOK MACRO name, size, args:VARARG
    ifndef X64
        @CatStr(name, Wrapper) proto stdcall args
        @CatStr(__imp__, name, @, size) dd @CatStr(name, Wrapper)
        public @CatStr(__imp__, name, @, size)
    else
        @CatStr(name, Wrapper) proto stdcall
        @CatStr(__imp_, name) dq @CatStr(name, Wrapper)
        public @CatStr(__imp_, name)
    endif
ENDM
```

**Add HOOK declarations for:**
1. File APIs (3): CreateSymbolicLinkW, GetFileInformationByHandleEx, SetFileInformationByHandle
2. Threadpool APIs (8): CreateThreadpoolWait, SetThreadpoolWait, CloseThreadpoolWait, CreateThreadpoolTimer, SetThreadpoolTimer, WaitForThreadpoolTimerCallbacks, CloseThreadpoolTimer, FreeLibraryWhenCallbackReturns
3. Time APIs (1): GetTickCount64
4. Processor APIs (2): GetCurrentProcessorNumber, FlushProcessWriteBuffers
5. Sync APIs (3): CreateSemaphoreExW, CreateEventExW, InitOnceExecuteOnce
6. Locale APIs (1): GetLocaleInfoEx

**Calculate stack sizes for x86 (stdcall convention):**
- Pointer = 4 bytes
- DWORD/BOOL = 4 bytes
- HANDLE = 4 bytes
- Example: `HOOK CreateSymbolicLinkW, 12, :dword, :dword, :dword` (3 pointers = 12 bytes)

**Expected Results:**
- 18 HOOK declarations added
- Correct stack sizes for x86
- x64 declarations without size parameter
- ASM file assembles without errors

**Logging:**
- Document each HOOK declaration
- Verify stack size calculations

**Blocks:** Task 0.5

#### Task 0.4: Test Compatibility Layer with Sample Program
**Files:** `tests/xp_compat_test.cpp` (new)

Create minimal test program to verify all 18 stubs work correctly:

```cpp
// Test program that calls all stubbed APIs
#include <windows.h>
#include <stdio.h>

int main() {
    // Test each API and verify it doesn't crash
    ULONGLONG tick64 = GetTickCount64();
    printf("GetTickCount64: %llu\n", tick64);
    
    DWORD proc = GetCurrentProcessorNumber();
    printf("GetCurrentProcessorNumber: %u\n", proc);
    
    // Test threadpool APIs (should return NULL/fail gracefully)
    PTP_WAIT wait = CreateThreadpoolWait(NULL, NULL, NULL);
    printf("CreateThreadpoolWait: %p (expected NULL)\n", wait);
    
    // ... test remaining 15 APIs ...
    
    return 0;
}
```

**Test steps:**
1. Create test program that calls all 18 APIs
2. Build test program with same flags as NetBox
3. Run on Windows 10/11 (should delegate to real APIs)
4. Verify no crashes, appropriate return values
5. Check that ERROR_CALL_NOT_IMPLEMENTED is set where expected

**Expected Results:**
- Test program compiles and links
- All 18 APIs callable without crashes
- Fallback APIs return sensible values (GetTickCount64 → GetTickCount)
- Stub APIs return NULL/FALSE with ERROR_CALL_NOT_IMPLEMENTED
- No undefined behavior

**Logging:**
- Document test results for each API
- Log return values and error codes
- Verify behavior matches expectations

**Blocks:** Task 7 (VM testing)

#### Task 0.5: Add Linker Subsystem Version Flag
**Files:** `cmake/NetBox.cmake`

Add explicit subsystem version to linker flags:

**Current (line 223):**
```cmake
/SUBSYSTEM:WINDOWS
```

**Change to:**
```cmake
/SUBSYSTEM:WINDOWS,5.01
```

**Verification steps:**
1. Edit `cmake/NetBox.cmake` line 223
2. Reconfigure CMake: `cmake -S . -B build-RelWithDebugInfo ...`
3. Build: `cmake --build build-RelWithDebugInfo`
4. Extract linker command from build log
5. Verify `/SUBSYSTEM:WINDOWS,5.01` appears in link command
6. Use `dumpbin /HEADERS build-RelWithDebugInfo/src/NetBox.dll` to verify PE header shows subsystem version 5.01

**Expected Results:**
- Linker flag updated in cmake/NetBox.cmake
- Build succeeds
- PE header shows subsystem version 5.01

**Logging:**
- Document linker flag change
- Log PE header verification results

**Blocks:** Task 1 (PE header verification)

#### Task 0.6: Add CMake Build Verification Step
**Files:** Build output, CMake cache

Verify build system produces XP-compatible binaries:

1. **Verify _WIN32_WINNT propagation:**
   - Check CMakeCache.txt for NETBOX_WIN32_WINNT=0x0501
   - Extract compile commands: `cmake -S . -B build-RelWithDebugInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
   - Verify all .cpp files compiled with `-D_WIN32_WINNT=0x0501`
   - Sample check: `grep '_WIN32_WINNT' build-RelWithDebugInfo/compile_commands.json | head -5`

2. **Verify subsystem version in link command:**
   - Build with verbose output: `cmake --build build-RelWithDebugInfo --verbose`
   - Extract link.exe command line
   - Verify `/SUBSYSTEM:WINDOWS,5.01` is present

3. **Verify static CRT linking:**
   - Check for `/MT` (release) or `/MTd` (debug) in compile commands
   - Verify no `/MD` or `/MDd` flags
   - Check CMakeCache.txt: `CMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded`

4. **Document build configuration:**
   - Compiler version: `cl.exe /?` (first line)
   - CMake version: `cmake --version`
   - Platform: x86, x64, or ARM64
   - Build type: Release, Debug, RelWithDebInfo

**Expected Results:**
- _WIN32_WINNT=0x0501 in all compilation units
- /SUBSYSTEM:WINDOWS,5.01 in link command
- Static CRT (/MT or /MTd)
- Build configuration documented

**Logging:**
- Log compiler/linker command samples
- Document CMake configuration
- Flag any misconfigurations with ERROR level

**Blocks:** Task 6 (compiler/linker audit)

#### Task 0.7: Document XP Compatibility Architecture
**Files:** `.ai-factory/docs/xp-compatibility.md` (new)

Create comprehensive documentation for the compatibility layer:

**Document structure:**

1. **Overview**
   - Purpose: Enable NetBox to run on Windows XP SP3
   - Approach: Runtime API detection + fallback stubs
   - Based on: Far3's proven vc_crt_fix pattern

2. **Architecture**
   - GetFunctionPointer template pattern
   - How runtime detection works
   - Why this approach (vs. conditional compilation)
   - Wrapper function naming convention

3. **Implemented Stubs (18 APIs)**
   - Table: API name | Module | XP Behavior | Vista+ Behavior
   - Example: GetTickCount64 | kernel32 | Returns GetTickCount() | Returns real GetTickCount64()
   - Document which APIs have fallbacks vs. ERROR_CALL_NOT_IMPLEMENTED

4. **XP Limitations**
   - GetTickCount wraps every 49.7 days (GetTickCount64 fallback)
   - No threadpool support (CreateThreadpoolWait returns NULL)
   - No symbolic links (CreateSymbolicLinkW fails)
   - Single processor assumed (GetCurrentProcessorNumber returns 0)
   - No InitOnceExecuteOnce (must use critical sections)

5. **Adding New Stubs**
   - Step-by-step guide
   - Code template
   - ASM export declaration
   - Testing checklist

6. **Testing**
   - How to test on modern Windows
   - How to test on XP VM
   - Expected behavior for each stub

7. **Troubleshooting**
   - Common issues
   - Linker errors
   - Runtime crashes

**Expected Results:**
- Complete architecture documentation
- Clear explanation of each stub
- Maintainer guide for future changes

**Logging:**
- Document creation in commit message


## Task Dependencies

**Phase 0 (Implementation) dependencies:**
- Task 0.1 (Verify OpenSSL Patches) → blocks Task 0.2 (need to know which APIs require stubs)
- Task 0.2 (Create Compatibility Layer) → blocks Task 0.3, 0.4, 0.5
- Task 0.3 (Update ASM Exports) → blocks Task 0.5 (linker needs exports)
- Task 0.4 (Test Compatibility Layer) → blocks Task 7 (VM testing)
- Task 0.5 (Add Linker Subsystem Flag) → blocks Task 1 (PE header verification)
- Task 0.6 (CMake Build Verification) → blocks Task 6 (compiler/linker audit)

**Cross-phase dependencies:**
- Task 0.2 (Create Compatibility Layer) → blocks Task 7 (can't test on XP VM without stubs)
- Task 0.4 (Test Compatibility Layer) → blocks Task 8 (unit test before functional test)
- Task 5 (Third-Party Library Compatibility) → informs Task 0.2 (which APIs need stubs)

**Recommended execution order:**
1. Phase 0: Tasks 0.1 → 0.2 → 0.3 → 0.4 → 0.5 → 0.6 → 0.7 (implementation)
2. Phase 1: Tasks 1 → 2 → 3 (binary analysis)
3. Phase 2: Tasks 4 → 5 (runtime analysis)
4. Phase 3: Task 6 (build config review)
5. Phase 4: Tasks 7 → 8 (test plan)
6. Phase 5: Tasks 9 → 10 (assessment)

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

## Notes

- This plan combines implementation and verification
- Phase 0 implements the compatibility layer based on Far3's proven pattern
- Phases 1-5 verify the implementation works correctly
- Focus on evidence-based assessment using binary analysis tools
- No assumptions about code — analyze compiled binaries after implementation
- Provide clear, actionable recommendations
- Track implementation status in remediation plan (Task 10)

## Success Criteria

1. **Implementation Complete (Phase 0):**
   - All 18 Vista+ API stubs implemented in vc_crt_fix_impl.cpp
   - ASM export declarations added for all wrappers
   - Compatibility layer unit tests pass
   - Linker subsystem version set to 5.01
   - Build verification confirms XP-compatible configuration
   - Architecture documentation complete

2. **Binary Analysis Complete (Phase 1):**
   - PE headers analyzed and show subsystem version 5.01
   - All dependencies documented and XP-compatible
   - API compatibility verified (all Vista+ APIs have stubs)

3. **Runtime Analysis Complete (Phase 2):**
   - CRT configuration verified (static linking)
   - C++17 features assessed for XP compatibility
   - Third-party libraries evaluated (OpenSSL patches verified)

4. **Build Configuration Verified (Phase 3):**
   - _WIN32_WINNT=0x0501 consistently applied
   - /SUBSYSTEM:WINDOWS,5.01 in linker command
   - Static CRT linking confirmed

5. **Test Plan Ready (Phase 4):**
   - VM setup guide created
   - Functional test checklist complete
   - Test environment documented

6. **Compatibility Report Generated (Phase 5):**
   - Clear verdict provided (Compatible/Partially Compatible/Incompatible)
   - Evidence-based conclusions
   - Actionable recommendations
   - Implementation status tracked

7. **Quality Gates:**
   - Clean build with zero warnings
   - All compatibility layer stubs tested
   - PE header shows subsystem 5.01
   - No Vista+ API dependencies without stubs
   - Documentation complete and accurate