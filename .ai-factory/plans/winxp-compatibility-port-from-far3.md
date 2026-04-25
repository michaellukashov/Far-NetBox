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

**Reference implementations:**
- **Far Manager 3** (runtime pattern): `D:\Projects\Far3\far3-master` — OS version detection, GetProcAddress wrappers
  - Repository: https://github.com/FarGroup/FarManager.git
- **YY-Thunks** (static library pattern): https://github.com/Chuyu-Team/YY-Thunks — Pre-built XP stub implementations
  - License: MIT
  - Use for: Vista+ API stub implementations that work on XP (kernel32, syncapi, libloaderapi modules)

---

## Settings

- **Testing:** Yes (manual testing on Windows XP VM required)
- **Logging:** Verbose (log OS version detection and compatibility shim usage)
- **Docs:** Yes (document XP compatibility requirements and limitations)

---

## Reference Sources

**Primary reference (runtime pattern):**
- Local: `D:\Projects\Far3\far3-master`
- Remote: https://github.com/FarGroup/FarManager.git
- **Key files to analyze:**
  - `far/platform.*.cpp` — OS version detection
  - `far/imports.*.cpp` — API imports (GetProcAddress wrappers)
  - `far/common/os_version.cpp` — Version checks
  - Build scripts — XP toolset configuration

**Secondary reference (stub implementations):**
- Remote: https://github.com/Chuyu-Team/YY-Thunks
- License: MIT (allows linking or copying implementations)
- **Key files to analyze:**
  - `src/Thunks/kernel32/*.cpp` — API shims for kernel32.dll (InitializeCriticalSectionEx, EncodePointer, etc.)
  - `src/Thunks/syncapi/*.cpp` — Synchronization primitives (condition variables, SRW locks)
  - `src/Thunks/libloaderapi/*.cpp` — DLL loading compatibility (AddDllDirectory, etc.)
  - `CMakeLists.txt` — How to integrate thunks into CMake build
- **Key YY-Thunks patterns:**
  - Static library approach: compile once, link into any target
  - Conditional compilation per API based on target Windows version
  - Tested implementations for XP (SP2/SP3), Vista, Win7 missing APIs
  - See issues for specific API coverage: #83 (TLS), #89 (EncodePointer), #90 (Chrome APIs), #117 (kernel32 version fixes)


**Tertiary reference (per-function delayload pattern):**
- FarManager commit `6e5828a657` — `#pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockExclusive")`
- Repository: https://github.com/FarGroup/FarManager
- Pattern: MSVC linker per-function delayload syntax (`/delayload:dllname.functionname`)
- Why not entire kernel32: Raymond Chen ("The Old New Thing", 2010-02-01) — delayloading all of kernel32.dll creates a Catch-22 because the delay-load helper itself needs kernel32 functions (LoadLibrary, GetProcAddress)
- Key files in FarManager to study: look for `#pragma comment(linker, "/delayload:kernel32.")` across the codebase
**Approach comparison:**

|Aspect|Far3 Pattern|YY-Thunks Pattern|Per-Function Delayload|Recommended for NetBox|
|---|---|---|---|
|Resolution|Runtime GetProcAddress|Link-time static library|Linker-generated stubs + failure hook|Hybrid: Delayload for kernel32 syncapi, YY-Thunks for others|
|Toolset|Requires XP toolset (v141_xp/v142_xp)|Works with modern VS2022 toolset|Works with modern VS2022 toolset|YY-Thunks / Delayload (avoids VS2019 requirement)|
|Maintenance|Must write each shim manually|Upstream maintained|Manual (pragma + hook)|Copy relevant stubs from YY-Thunks / use delayload pragmas|
|Overhead|Per-API lazy loading|Zero overhead (linked at compile time)|One-time load, then direct call|YY-Thunks / Delayload per API|
|Call site changes|Yes (macro redirect)|No|**No** (transparent)|Delayload where possible|
|kernel32 compat|Yes|Yes|**Yes** (per-function only, not entire DLL)|Delayload for syncapi functions|

## Tasks

### Phase 1: Discovery & Analysis

- [ ] **task-1:** Analyze Far3 and YY-Thunks XP compatibility architectures
  - **Far3 files to examine:**
    - `D:\Projects\Far3\far3-master\far\platform.*.cpp` (OS detection)
    - `D:\Projects\Far3\far3-master\far\imports.*.cpp` (API imports)
    - `D:\Projects\Far3\far3-master\far\common\os_version.cpp` (version checks)
    - CMakeLists.txt or build scripts (XP toolset configuration)
  - **YY-Thunks files to examine:**
    - `src/Thunks/kernel32/*.cpp` — API shims (InitializeCriticalSectionEx, EncodePointer, DecodePointer)
    - `src/Thunks/syncapi/*.cpp` — Condition variables, SRW locks emulation
    - `src/Thunks/libloaderapi/*.cpp` — LoadLibrary, AddDllDirectory compatibility
    - `CMakeLists.txt` — Build integration patterns
  - **Search for (Far3):**
    - `IsWindowsXP`, `IsWindowsVersionOrGreater`, `OSVERSIONINFO`
    - `GetProcAddress`, `LoadLibrary` patterns for dynamic API loading
    - `_WIN32_WINNT` preprocessor checks
    - XP-specific workarounds (e.g., `InitializeCriticalSectionEx` fallback)
  - **Search for (YY-Thunks):**
    - List of APIs already stubbed (check `src/Thunks/` directory structure)
    - CMake integration patterns (how thunks are built and linked)
    - License verification (MIT — confirm terms allow copying/linking)
  - **DONE when:** Documented:
    1. Far3's OS version detection mechanism (function names, file locations)
    2. YY-Thunks stub inventory (which APIs are already implemented)
    3. Conditional compilation strategy (macros, build flags)
    4. Decision: which APIs from YY-Thunks to reuse vs implement from Far3
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
    | NetBox File | API Used | XP Available? | YY-Thunks Has It? | Far3 Shim Location | Action Required |
    |-------------|----------|---------------|-------------------|-------------------|-----------------|
    | ... | ... | Yes/No | Yes/No | file:line | Port/Adapt/Skip/Copy from YY-Thunks |
  - **LOGGING:**
    - Log each incompatible API found with usage count
    - Use format: `[XPCompat.Audit] API {name} used {count} times in {file} - XP: {yes/no} - YY-Thunks: {yes/no}`
    - Use WARN level for XP-incompatible APIs

- [ ] **task-3:** Design NetBox XP compatibility layer architecture
  - **Design decisions:**
    1. OS version detection: Where to initialize? (plugin startup, session init, per-operation?)
    2. API shim location: New file `src/base/WinXPCompat.h/cpp` or integrate into existing `src/base/Common.cpp`?
    3. Stub strategy: Copy YY-Thunks implementations into NetBox vs link YY-Thunks.lib
    4. Build configuration: Single binary with runtime detection (recommended)
  - **API shim pattern (from Far3 — runtime GetProcAddress):**
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
  - **API shim pattern (from YY-Thunks — static stub):**
    ```cpp
    // YY-Thunks approach: Provide stub that's always linked
    // On XP: stub executes directly (no GetProcAddress overhead)
    // On Vista+: stub calls native API (transparent)
    // Example: YY-Thunks kernel32 InitializeCriticalSectionEx stub
    extern "C" BOOL WINAPI InitializeCriticalSectionEx(
        LPCRITICAL_SECTION lpCriticalSection,
        DWORD dwSpinCount,
        DWORD Flags)
    {
        // YY-Thunks implements proper fallback logic inline
        // No runtime detection needed — stub is always available
        if (dwSpinCount == 0 && Flags == 0)
            return ::InitializeCriticalSection(lpCriticalSection), TRUE;
        // ... full stub implementation from YY-Thunks source
    }
    ```
  - **API shim pattern (from MSVC per-function delayload — recommended for kernel32 syncapi):**
    - Reference: FarManager commit `6e5828a657` — `#pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockExclusive")`
    - Uses MSVC linker's per-function delayload syntax (`/delayload:dllname.functionname`)
    - Avoids the Catch-22 of delayloading all of kernel32.dll (Raymond Chen, "The Old New Thing", 2010-02-01)
    - When function doesn't exist on XP, the delay-load helper raises structured exception (`dliFailGetProc`)
    - Custom `__pfnDliFailureHook2` intercepts the failure and returns pointer to fallback implementation
    ```cpp
    // In WinXPCompat.h — one pragma per function:
    #pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockExclusive")
    #pragma comment(linker, "/delayload:kernel32.AcquireSRWLockExclusive")

    // In WinXPCompat.cpp — fallback implementations + failure hook:
    VOID WINAPI ReleaseSRWLockExclusive_Fallback(PSRWLOCK SRWLock)
    {
        LeaveCriticalSection((LPCRITICAL_SECTION)SRWLock);
    }

    FARPROC WINAPI DliFailureHook2(unsigned dliNotify, PDelayLoadInfo pdli)
    {
        if (dliNotify == dliFailGetProc &&
            _stricmp(pdli->szDll, "kernel32.dll") == 0)
        {
            if (strcmp(pdli->dlp.szProcName, "ReleaseSRWLockExclusive") == 0)
                return (FARPROC)ReleaseSRWLockExclusive_Fallback;
            // ... more fallbacks for other syncapi functions
        }
        return nullptr; // let default handler raise exception
    }

    // Register hook at plugin startup:
    PfnDliHook __pfnDliFailureHook2 = DliFailureHook2;
    ```
    - **Advantage over Far3/YY-Thunks for kernel32 syncapi:**
      - Zero call-site changes (transparent to existing code)
      - One pragma + one fallback function per API (minimal boilerplate)
      - MSVC linker generates all stub code automatically
      - Call sites use native API name — delayload hook only activates when API is missing
    - **Requires:** Link `delayimp.lib` (MSVC delay-load helper library)
    - **NetBox already uses delayload for:** `ws2_32.dll`, `oleaut32.dll`, `shlwapi.dll`, `crypt32.dll` (`cmake/NetBox.cmake` lines 218-221)
  - **DONE when:** Design document added to plan with:
    1. File structure (new files to create)
    2. API shim pattern (code template — choose Far3 vs YY-Thunks vs Per-Function Delayload per API)
    3. Initialization sequence (when to detect OS, when to load shims, when to register DliFailureHook2)
    4. Build configuration changes (CMake, compiler flags, YY-Thunks integration, delayimp.lib linkage)
    5. Per-API strategy: which APIs use delayload pragmas vs YY-Thunks stubs vs Far3 GetProcAddress
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

- [ ] **task-5:** Port API compatibility shims
  - **File:** `src/base/WinXPCompat.cpp`, `src/base/WinXPCompat.h`
  - **APIs to port (based on task-2 audit):**
    - Threading: `InitializeCriticalSectionEx`, `InitializeConditionVariable`, `SleepConditionVariableCS`
    - SyncAPI (kernel32 delayload candidates): `AcquireSRWLockExclusive`, `ReleaseSRWLockExclusive`, `InitializeSRWLock`, `SleepConditionVariableSRW`, `WakeConditionVariable`, `WakeAllConditionVariable`
    - File system: `GetFinalPathNameByHandle`, `CreateSymbolicLink`
    - Network: IPv6 functions if used
    - Security: `CreateWellKnownSid` (if used), `EncodePointer`, `DecodePointer`
    - Process: `FlsAlloc`, `FlsFree`, `FlsGetValue`, `FlsSetValue` (fiber local storage)
  - **Source selection per API (three-tier strategy):**
    |API|Approach|Rationale|
    |---|---|---|
    |`AcquireSRWLockExclusive`|**Per-function delayload**|Transparent fallback via DliFailureHook2|
    |`ReleaseSRWLockExclusive`|**Per-function delayload**|FarManager commit 6e5828a657 pattern|
    |`InitializeSRWLock`|**Per-function delayload**|Same as above|
    |`SleepConditionVariableSRW`|**Per-function delayload**|Same as above|
    |`WakeConditionVariable`|**Per-function delayload**|Same as above|
    |`WakeAllConditionVariable`|**Per-function delayload**|Same as above|
    |`InitializeCriticalSectionEx`|YY-Thunks|Complex fallback with spin count handling|
    |`InitializeConditionVariable`|YY-Thunks|Event-based emulation already tested|
    |`SleepConditionVariableCS`|YY-Thunks|Pairs with above|
    |`GetFinalPathNameByHandle`|Far3 or YY-Thunks|Both have implementations|
    |`EncodePointer`/`DecodePointer`|YY-Thunks|Issue #89 has XP RTM fix|
    |`Fls*` functions|YY-Thunks|Issue #83 has TLS support|
    |`CreateSymbolicLink`|Far3|Simple wrapper, no YY-Thunks equivalent|
  - **Pattern (Per-Function Delayload — preferred for kernel32 syncapi):**
    1. Add `#pragma comment(linker, "/delayload:kernel32.<FunctionName>")` to `WinXPCompat.h`
    2. Implement `<FunctionName>_Fallback` in `WinXPCompat.cpp`
    3. In `DliFailureHook2`, map function name to fallback pointer
    4. Register hook: `PfnDliHook __pfnDliFailureHook2 = DliFailureHook2;`
    5. Zero call-site changes needed — existing code calls native API name directly
  - **Pattern (YY-Thunks approach — for non-kernel32 APIs):**
    1. Copy stub implementation from YY-Thunks source (MIT license)
    2. Wrap in `#ifndef NETBOX_WINXP_COMPAT` guard for non-XP builds
    3. Add to `src/base/WinXPCompat.cpp`
    4. Declare in `src/base/WinXPCompat.h` with `extern "C"` linkage
  - **Pattern (Far3 approach — for APIs not in YY-Thunks):**
  - **DONE when:**
    1. All APIs from task-2 compatibility matrix have shims
    2. Each shim has XP fallback that preserves functionality (or gracefully degrades)
    3. Shims tested on modern Windows (should use native API or transparent stub)
    4. YY-Thunks source attribution included in comments for each copied stub
  - **LOGGING:**
    - Log each API shim initialization (success/fallback)
    - Use format: `[XPCompat.Shim] API {name}: {native/fallback}`
    - Use DEBUG level

- [ ] **task-6:** Integrate compatibility layer with NetBox codebase
  - **Files to modify:** (from task-2 audit)
  - **Changes:**
    1. Add `#include "WinXPCompat.h"` to files using shimmed APIs
    2. Replace direct API calls with shim macros (if using macro redirection — not needed for delayload APIs)
    3. Add `TWinXPCompat::Initialize()` call to plugin entry point (`src/NetBox/NetBox.cpp` or similar)
    4. Register `__pfnDliFailureHook2` at plugin startup (for per-function delayload fallback)
    5. Add `delayimp.lib` to link dependencies (required for delayload support)
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
    2. **Important:** Keep `_WIN32_WINNT >= 0x0600` even for XP builds (APIs must be declared)
       - Do NOT set `_WIN32_WINNT=0x0501` — this hides Vista+ API declarations
       - Use runtime resolution (GetProcAddress) or static stubs (YY-Thunks pattern)
    3. Add preprocessor define: `add_definitions(-DNETBOX_WINXP_COMPAT)` when enabled
    4. When `OPT_WINXP_COMPAT=ON`:
       - Option A: Add YY-Thunks stubs to build target directly (copy source approach)
       - Option B: Link against pre-built `yythunks.lib` (if available)
       - Option C: Add per-function delayload pragmas to `WinXPCompat.h` (zero build changes — linker handles stubs)
    5. **DO NOT** use XP toolset (`v141_xp`/`v142_xp`) — use modern VS2022 toolset with stubs
    6. Add `delayimp.lib` to link libraries (required for per-function delayload support)
    7. Verify existing `/DELAYLOAD` entries in `cmake/NetBox.cmake` (ws2_32, oleaut32, shlwapi, crypt32)
  - **DONE when:**
    1. CMake generates XP-compatible build when `OPT_WINXP_COMPAT=ON`
    2. Default build (OFF) remains unchanged (modern Windows target)
    3. Build works with VS2022 toolset (no XP toolset dependency)
    4. Build script `build-x64-xp.bat` created for XP builds
  - **LOGGING:**
    - Log build configuration at CMake configure time
    - Use CMake `message(STATUS "XP compatibility: ${OPT_WINXP_COMPAT}")`
    - Use CMake `message(STATUS "XP stub source: ${XP_STUB_SOURCE}")`

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
    2. XP compatibility layer architecture (Far3 + YY-Thunks + Per-Function Delayload hybrid)
    3. List of shimmed APIs with source attribution (YY-Thunks / Far3 / MSVC delayload)
    4. Known limitations on XP
  - **DONE when:** Documentation merged and reviewed

- [ ] **task-12:** Create XP compatibility notes for users
  - **File:** `docs/windows-xp-compatibility.md` (new file)
  - **Content:**
    1. System requirements (XP SP3 minimum)
    2. Supported protocols on XP (SFTP, FTP, WebDAV, S3)
    3. Known limitations (e.g., no TLS 1.3, no IPv6)
    4. Troubleshooting XP-specific issues
    5. **Performance notes:** SRW lock fallback uses critical sections — may have higher contention under heavy threading on XP. SRW locks are reader-writer optimized; critical sections are exclusive. For typical NetBox usage (file transfers, not highly concurrent), the difference is negligible.
  - **DONE when:** User-facing documentation complete

---

## Commit Plan

- **Commit 1** (after tasks 1-3): `feat(xp): analyze Far3 and YY-Thunks compatibility, design NetBox layer`
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

**YY-Thunks reference files (to clone/study):**
- Repository: https://github.com/Chuyu-Team/YY-Thunks
- `src/Thunks/kernel32/*.cpp` — API shims (InitializeCriticalSectionEx, EncodePointer, etc.)
- `src/Thunks/syncapi/*.cpp` — Condition variables emulation
- `src/Thunks/libloaderapi/*.cpp` — DLL loading compatibility
- `CMakeLists.txt` — Build integration

**NetBox files to modify (estimated):**
- `src/base/WinXPCompat.h` (new) — Compatibility layer header + per-function delayload pragmas
- `src/base/WinXPCompat.cpp` (new) — Shim implementations (from Far3 + YY-Thunks) + `DliFailureHook2`
- `src/NetBox/NetBox.cpp` — Initialize compatibility layer + register `__pfnDliFailureHook2`
- Files from task-2 audit — Integrate shims

**XP-incompatible APIs (common examples):**
- `ReleaseSRWLockExclusive` (Vista+) → **Per-function delayload** + fallback (FarManager commit 6e5828a657 pattern)
- `AcquireSRWLockExclusive` (Vista+) → **Per-function delayload** + fallback
- `InitializeSRWLock` (Vista+) → **Per-function delayload** + fallback
- `InitializeCriticalSectionEx` (Vista+) → YY-Thunks stub (spin count handling)
- `InitializeConditionVariable` / `SleepConditionVariableCS` (Vista+) → YY-Thunks event-based emulation
- `GetFinalPathNameByHandle` (Vista+) → Far3 fallback or YY-Thunks stub
- `CreateSymbolicLink` (Vista+) → return error on XP (no good fallback)
- `EncodePointer` / `DecodePointer` (XP RTM missing) → YY-Thunks stub (issue #89)
- `FlsAlloc` / `FlsFree` / `FlsGetValue` / `FlsSetValue` → YY-Thunks TLS support (issue #83)
- `AddDllDirectory` / `RemoveDllDirectory` / `SetDefaultDllDirectories` → YY-Thunks stub (issue #92)

**Per-Function Delayload reference:**
- FarManager commit `6e5828a657` — `#pragma comment(linker, "/delayload:kernel32.ReleaseSRWLockExclusive")`
- Raymond Chen, "The Old New Thing" (2010-02-01): Why you can't `/DELAYLOAD:kernel32.dll` but CAN delayload individual functions
- MSVC docs: `/delayload:dllname.functionname` syntax generates per-function stubs
- Requires: `delayimp.lib` (MSVC delay-load helper) + `__pfnDliFailureHook2` for XP fallback

**Build toolset decision:**
- **Approach:** Use modern VS2022 toolset + compatibility stubs (no XP toolset needed)
- **Why:** VS2022 does not include `v143_xp`; requiring VS2019 is burdensome
- **How:** YY-Thunks pattern — stubs provide missing symbols at link time
- **Preprocessor:** Keep `_WIN32_WINNT >= 0x0600` so API declarations are visible

---

## Edge Cases

- **XP SP2 vs SP3:** Require SP3 minimum (some APIs differ)
- **64-bit XP:** Rare, focus on 32-bit XP (x86 build)
- **API not available on XP:** Graceful degradation (log warning, disable feature)
- **Dynamic loading fails:** Fallback to XP-compatible alternative (YY-Thunks stub)
- **Per-function delayload hook fails:** If `__pfnDliFailureHook2` is not registered or returns null, the delay-load helper raises structured exception. Must ensure hook is registered before any delayloaded API is called.
- **Structured exception handler conflicts:** DliFailureHook uses SEH (`__try`/`__except`). If calling code also uses SEH, exceptions may be caught by the wrong handler. Use `/EHa` if C++ exceptions and SEH are mixed.
- **Performance on XP:** Acceptable degradation for legacy support
- **TLS/SSL on XP:** Limited to TLS 1.0/1.1 (modern servers may reject)
- **OpenSSL version:** Modern OpenSSL 3.x dropped XP support — may need older OpenSSL for XP builds
- **YY-Thunks license compliance:** MIT license allows copying — include attribution in source comments

---

## Success Criteria

- [ ] Build passes with zero warnings (XP and modern builds)
- [ ] Plugin loads and runs on Windows XP SP3 VM
- [ ] Core protocols (SFTP, FTP, WebDAV) functional on XP
- [ ] No regression on Windows 10/11
- [ ] XP mode correctly detected at runtime
- [ ] All shimmed APIs logged in debug mode
- [ ] YY-Thunks source attribution included for copied stubs
- [ ] Per-function delayload pragmas present for kernel32 syncapi functions in `WinXPCompat.h`
- [ ] `__pfnDliFailureHook2` registered and returns correct fallbacks on XP
- [ ] `delayimp.lib` linked when `OPT_WINXP_COMPAT=ON`
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
