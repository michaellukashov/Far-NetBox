# Implementation Plan: Port Windows XP Compatibility from Far3 to NetBox Plugin

**Branch:** feature/winxp-compat  
**Created:** 2026-04-24  
**Mode:** Full (with branch)

---

## Problem Statement

NetBox plugin currently lacks Windows XP compatibility layer that exists in Far Manager 3. Far3 has proven XP compatibility implementation that should be ported to the plugin to enable NetBox to run on legacy Windows XP systems.

**Current behavior:**
- NetBox plugin may fail or crash on Windows XP due to missing API compatibility shims
- Modern Windows APIs used in NetBox are not available on XP
- No runtime detection of OS version to conditionally use XP-compatible code paths

**Expected behavior:**
- NetBox plugin detects Windows XP at runtime
- Uses compatibility shims for APIs not available on XP
- Falls back to XP-compatible alternatives where needed
- Maintains full functionality on modern Windows (Vista+)

**Reference implementation:**
- Far Manager 3 source code: `D:\Projects\Far3\far3-master`
- Repository: https://github.com/FarGroup/FarManager.git
- Focus areas: OS version detection, API compatibility layer, conditional compilation

---

## Settings

- **Testing:** Yes (manual testing on Windows XP VM required)
- **Logging:** Verbose (log OS version detection and compatibility shim usage)
- **Docs:** Yes (document XP compatibility requirements and limitations)

---

## Reference Sources

**Primary reference:**
- Local: `D:\Projects\Far3\far3-master`
- Remote: https://github.com/FarGroup/FarManager.git

**Key files to analyze:**
- OS version detection logic
- API compatibility shims (GetProcAddress wrappers)
- Conditional compilation macros
- XP-specific workarounds
- Build configuration for XP target

---

## Tasks

### Phase 1: Discovery & Analysis

- [ ] **task-1:** Analyze Far3 XP compatibility architecture
  - **Files to examine:**
    - `D:\Projects\Far3\far3-master\far\platform.*.cpp` (OS detection)
    - `D:\Projects\Far3\far3-master\far\imports.*.cpp` (API imports)
    - `D:\Projects\Far3\far3-master\far\common\os_version.cpp` (version checks)
    - CMakeLists.txt or build scripts (XP toolset configuration)
  - **Search for:**
    - `IsWindowsXP`, `IsWindowsVersionOrGreater`, `OSVERSIONINFO`
    - `GetProcAddress`, `LoadLibrary` patterns for dynamic API loading
    - `_WIN32_WINNT` preprocessor checks
    - XP-specific workarounds (e.g., `InitializeCriticalSectionEx` fallback)
  - **DONE when:** Documented:
    1. Far3's OS version detection mechanism (function names, file locations)
    2. List of APIs that need XP compatibility shims
    3. Conditional compilation strategy (macros, build flags)
    4. Dynamic loading patterns for Vista+ APIs
  - **LOGGING:**
    - Log discovered compatibility patterns with file:line references
    - Use format: `[XPCompat.Discovery] Found pattern: {description} in {file}:{line}`
    - Use INFO level

- [ ] **task-2:** Identify NetBox code requiring XP compatibility
  - **Files to examine:**
    - `src/core/*.cpp` (SSH/FTP/WebDAV protocol implementations)
    - `src/windows/*.cpp` (Far Manager GUI integration)
    - `src/base/*.cpp` (base classes, utilities)
    - `src/nbcore/*.cpp` (logging, memory management)
  - **Search for:**
    - Modern Windows APIs not available on XP (Vista+ APIs)
    - Direct API calls that need dynamic loading on XP
    - File system operations (extended attributes, symbolic links)
    - Network APIs (IPv6, modern TLS)
    - Threading primitives (condition variables, SRW locks)
  - **DONE when:** Created compatibility matrix:
    | NetBox File | API Used | XP Available? | Far3 Shim Location | Action Required |
    |-------------|----------|---------------|-------------------|-----------------|
    | ... | ... | Yes/No | file:line | Port/Adapt/Skip |
  - **LOGGING:**
    - Log each incompatible API found with usage count
    - Use format: `[XPCompat.Audit] API {name} used {count} times in {file} - XP: {yes/no}`
    - Use WARN level for XP-incompatible APIs

- [ ] **task-3:** Design NetBox XP compatibility layer architecture
  - **Design decisions:**
    1. OS version detection: Where to initialize? (plugin startup, session init, per-operation?)
    2. API shim location: New file `src/base/WinXPCompat.h/cpp` or integrate into existing `src/base/Common.cpp`?
    3. Macro strategy: `#if (_WIN32_WINNT < 0x0600)` or runtime checks?
    4. Build configuration: Separate XP build target or single binary with runtime detection?
  - **API shim pattern (from Far3):**
    ```cpp
    // Example: InitializeCriticalSectionEx (Vista+)
    typedef BOOL (WINAPI *PInitializeCriticalSectionEx)(LPCRITICAL_SECTION, DWORD, DWORD);
    static PInitializeCriticalSectionEx pInitializeCriticalSectionEx = nullptr;
    
    BOOL XP_InitializeCriticalSectionEx(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount, DWORD Flags)
    {
        if (!pInitializeCriticalSectionEx)
        {
            HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
            pInitializeCriticalSectionEx = (PInitializeCriticalSectionEx)GetProcAddress(hKernel32, "InitializeCriticalSectionEx");
        }
        
        if (pInitializeCriticalSectionEx)
            return pInitializeCriticalSectionEx(lpCriticalSection, dwSpinCount, Flags);
        else
            return InitializeCriticalSection(lpCriticalSection), TRUE; // XP fallback
    }
    ```
  - **DONE when:** Design document added to plan with:
    1. File structure (new files to create)
    2. API shim pattern (code template)
    3. Initialization sequence (when to detect OS, when to load shims)
    4. Build configuration changes (CMake, compiler flags)
  - **LOGGING:**
    - Log design decisions and rationale
    - Use format: `[XPCompat.Design] Decision: {description}`
    - Use INFO level

### Phase 2: Core Implementation

- [ ] **task-4:** Implement OS version detection
  - **File:** `src/base/WinXPCompat.cpp` (new file)
  - **Implementation:**
    ```cpp
    class TWinXPCompat
    {
    public:
        static bool IsWindowsXP();
        static bool IsWindowsVistaOrGreater();
        static void Initialize(); // Call at plugin startup
    private:
        static bool FIsXP;
        static bool FInitialized;
    };
    ```
  - **Pattern from Far3:** Use `GetVersionEx` or `VerifyVersionInfo` (XP-compatible)
  - **DONE when:**
    1. `TWinXPCompat::Initialize()` correctly detects XP vs Vista+
    2. Detection result cached in static variable
    3. Unit test verifies detection logic (mock `GetVersionEx` if needed)
  - **LOGGING:**
    - Log OS version detection result at plugin startup
    - Use format: `[XPCompat.Init] OS detected: {version} (XP mode: {yes/no})`
    - Use INFO level

- [ ] **task-5:** Port Far3 API compatibility shims
  - **File:** `src/base/WinXPCompat.cpp`, `src/base/WinXPCompat.h`
  - **APIs to port (based on task-2 audit):**
    - Threading: `InitializeCriticalSectionEx`, `InitializeConditionVariable`, `SleepConditionVariableCS`
    - File system: `GetFinalPathNameByHandle`, `CreateSymbolicLink`
    - Network: IPv6 functions if used
    - Security: `CreateWellKnownSid` (if used)
  - **Pattern:** For each API:
    1. Define function pointer typedef
    2. Implement `XP_<APIName>` wrapper with dynamic loading
    3. Provide XP fallback implementation
    4. Add macro to redirect calls: `#define APIName XP_APIName`
  - **DONE when:**
    1. All APIs from task-2 compatibility matrix have shims
    2. Each shim has XP fallback that preserves functionality (or gracefully degrades)
    3. Shims tested on modern Windows (should use native API via GetProcAddress)
  - **LOGGING:**
    - Log each API shim initialization (success/fallback)
    - Use format: `[XPCompat.Shim] API {name}: {native/fallback}`
    - Use DEBUG level

- [ ] **task-6:** Integrate compatibility layer with NetBox codebase
  - **Files to modify:** (from task-2 audit)
  - **Changes:**
    1. Add `#include "WinXPCompat.h"` to files using shimmed APIs
    2. Replace direct API calls with shim macros (if using macro redirection)
    3. Add `TWinXPCompat::Initialize()` call to plugin entry point (`src/NetBox/NetBox.cpp` or similar)
  - **Verification:** Ensure no direct calls to Vista+ APIs remain (grep for API names)
  - **DONE when:**
    1. All incompatible API calls routed through compatibility layer
    2. Plugin initializes compatibility layer at startup
    3. No build warnings about undefined APIs on XP toolset
  - **LOGGING:**
    - Log compatibility layer initialization at plugin startup
    - Use format: `[XPCompat] Compatibility layer initialized (XP mode: {yes/no})`
    - Use INFO level

### Phase 3: Build Configuration

- [ ] **task-7:** Configure CMake for XP compatibility
  - **File:** `CMakeLists.txt` (root and `src/CMakeLists.txt`)
  - **Changes:**
    1. Add option: `option(OPT_WINXP_COMPAT "Enable Windows XP compatibility" OFF)`
    2. Set `_WIN32_WINNT=0x0501` when `OPT_WINXP_COMPAT=ON`
    3. Use Visual Studio XP toolset: `set(CMAKE_GENERATOR_TOOLSET "v141_xp")` (VS2017) or `v142_xp` (VS2019)
    4. Add preprocessor define: `add_definitions(-DNETBOX_WINXP_COMPAT)` when enabled
  - **DONE when:**
    1. CMake generates XP-compatible build when `OPT_WINXP_COMPAT=ON`
    2. Default build (OFF) remains unchanged (modern Windows target)
    3. Build script `build-x64-xp.bat` created for XP builds
  - **LOGGING:**
    - Log build configuration at CMake configure time
    - Use CMake `message(STATUS "XP compatibility: ${OPT_WINXP_COMPAT}")`

- [ ] **task-8:** Create XP build script
  - **File:** `build-x64-xp.bat` (new file)
  - **Content:**
    ```bat
    @echo off
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    cmake -S . -B build-x64-xp -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_WINXP_COMPAT=ON -DOPT_CREATE_PLUGIN_DIR=ON
    cmake --build build-x64-xp --clean-first -- -j4
    ```
  - **DONE when:**
    1. Script successfully builds XP-compatible plugin DLL
    2. DLL loads on Windows XP (test in VM)
    3. Script documented in `AGENTS-Workflows.md`

### Phase 4: Testing & Validation

- [ ] **task-9:** Test on Windows XP VM
  - **Test environment:**
    - Windows XP SP3 VM
    - Far Manager 3 installed
    - NetBox plugin XP build deployed to `Far3_x64/Plugins/NetBox/`
  - **Test scenarios:**
    1. Plugin loads without errors (check Far Manager plugin list)
    2. Connect to SFTP server (basic SSH functionality)
    3. Connect to FTP server (basic FTP functionality)
    4. Connect to WebDAV server (basic WebDAV functionality)
    5. File transfer operations (upload, download, delete)
    6. Interactive SSH session (if supported)
    7. Check plugin log for compatibility shim usage
  - **Pass criteria:** All core functionality works on XP without crashes
  - **DONE when:**
    1. Test matrix shows ≥90% pass rate (some features may degrade gracefully)
    2. No crashes or unhandled exceptions on XP
    3. Log shows correct XP mode detection and shim usage
  - **LOGGING:**
    - Log test results with pass/fail status
    - Use format: `[XPCompat.Test] Scenario {name}: {pass/fail} - {details}`
    - Use INFO level

- [ ] **task-10:** Verify no regression on modern Windows
  - **Test environment:**
    - Windows 10/11 (modern build)
    - Far Manager 3 installed
    - NetBox plugin standard build (XP compat OFF)
  - **Test scenarios:** Same as task-9
  - **Pass criteria:** 100% pass rate, no performance degradation
  - **DONE when:**
    1. All tests pass on Windows 10/11
    2. No performance regression vs pre-XP-compat builds
    3. Log shows native API usage (not fallbacks)
  - **LOGGING:**
    - Log test results
    - Use format: `[XPCompat.Test] Modern Windows: {scenario} - {pass/fail}`
    - Use INFO level

### Phase 5: Documentation

- [ ] **task-11:** Update build documentation
  - **Files:**
    - `AGENTS-Workflows.md` (add XP build instructions)
    - `AGENTS-Structure.md` (document WinXPCompat module)
  - **Content:**
    1. How to build XP-compatible plugin (`build-x64-xp.bat`)
    2. XP compatibility layer architecture
    3. List of shimmed APIs
    4. Known limitations on XP
  - **DONE when:** Documentation merged and reviewed

- [ ] **task-12:** Create XP compatibility notes for users
  - **File:** `docs/windows-xp-compatibility.md` (new file)
  - **Content:**
    1. System requirements (XP SP3 minimum)
    2. Supported protocols on XP (SFTP, FTP, WebDAV, S3)
    3. Known limitations (e.g., no TLS 1.3, no IPv6)
    4. Troubleshooting XP-specific issues
  - **DONE when:** User-facing documentation complete

---

## Commit Plan

- **Commit 1** (after tasks 1-3): `feat(xp): analyze Far3 XP compatibility and design NetBox layer`
- **Commit 2** (after tasks 4-6): `feat(xp): implement OS detection and API compatibility shims`
- **Commit 3** (after tasks 7-8): `build(xp): add CMake XP build configuration and script`
- **Commit 4** (after tasks 9-10): `test(xp): validate XP compatibility on VM and modern Windows`
- **Commit 5** (after tasks 11-12): `docs(xp): document XP compatibility layer and build process`

---

## Technical Notes

**Far3 reference files (estimated):**
- `far/platform.*.cpp` — OS version detection
- `far/imports.*.cpp` — Dynamic API loading
- `far/common/os_version.cpp` — Version checks
- Build scripts — XP toolset configuration

**NetBox files to modify (estimated):**
- `src/base/WinXPCompat.h` (new) — Compatibility layer header
- `src/base/WinXPCompat.cpp` (new) — Shim implementations
- `src/NetBox/NetBox.cpp` — Initialize compatibility layer
- `CMakeLists.txt` — XP build option
- Files from task-2 audit — Integrate shims

**XP-incompatible APIs (common examples):**
- `InitializeCriticalSectionEx` (Vista+) → fallback to `InitializeCriticalSection`
- `GetFinalPathNameByHandle` (Vista+) → fallback to `GetMappedFileName` or skip
- `CreateSymbolicLink` (Vista+) → return error on XP
- Condition variables (Vista+) → emulate with events + critical sections

**Build toolset:**
- Visual Studio 2017: `v141_xp`
- Visual Studio 2019: `v142_xp`
- Visual Studio 2022: No official XP toolset (use VS2019 or earlier)

---

## Edge Cases

- **XP SP2 vs SP3:** Require SP3 minimum (some APIs differ)
- **64-bit XP:** Rare, focus on 32-bit XP (x86 build)
- **API not available on XP:** Graceful degradation (log warning, disable feature)
- **Dynamic loading fails:** Fallback to XP-compatible alternative
- **Performance on XP:** Acceptable degradation for legacy support
- **TLS/SSL on XP:** Limited to TLS 1.0/1.1 (modern servers may reject)

---

## Success Criteria

- [ ] Build passes with zero warnings (XP and modern builds)
- [ ] Plugin loads and runs on Windows XP SP3 VM
- [ ] Core protocols (SFTP, FTP, WebDAV) functional on XP
- [ ] No regression on Windows 10/11
- [ ] XP mode correctly detected at runtime
- [ ] All shimmed APIs logged in debug mode
- [ ] Documentation complete (build + user-facing)
- [ ] Code follows NetBox conventions (CRLF, naming, no warnings)

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
