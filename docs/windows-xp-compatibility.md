# Windows XP Compatibility

> NetBox supports Windows XP SP3 (x86) via a compatibility layer ported from Far Manager 3.
>
> **Validation status:** Runtime-validated on Windows XP SP3 x86 (2026-05-24).
> See `.ai-factory/reports/winxp-compatibility-report.md` for full assessment.

## Overview

NetBox is built with Visual Studio 2022 (v143 toolset) and targets Windows XP through a dual-layer compatibility approach:

1. **Build-time**: `_WIN32_WINNT=0x0503`, static CRT (`/MT`), subsystem `5.01`
2. **Run-time**: API shims intercept Vista+ kernel32 calls and fall back to XP-compatible alternatives

## Architecture

### Compatibility Layer Files

| File | Purpose |
|------|---------|
| `src/NetBox/vc_crt_fix_impl.cpp` | 50+ API wrapper implementations (primary) |
| `src/NetBox/vc_crt_fix.asm` | ASM export table (x86/x64 HOOK macros) |
| `src/NetBox/vc_crt_fix_ulink.cpp` | Delay-load fallback hooks (`DliFailureHook2`) |

### How It Works

The MSVC linker builds the DLL against a modern import library (kernel32.lib from Windows SDK). At load time on Windows XP, the import table references functions that do not exist in XP's kernel32.dll. The compatibility layer resolves this via two redundant mechanisms:

1. **HOOK macros** (`vc_crt_fix.asm`): The ASM file defines `__imp_<FunctionName>` symbols that redirect to `Wrapper_<FunctionName>`. Each wrapper uses the `CREATE_AND_RETURN` macro:
   - On Vista+: `GetProcAddress` finds the real API, calls it directly
   - On XP: real API missing, executes the fallback `implementation::impl()`

2. **Delay-load hooks** (`vc_crt_fix_ulink.cpp`): Per-function `/delayload:kernel32.<Function>` pragmas catch `dliFailGetProc` events. `__pfnDliFailureHook2` returns a fallback function pointer. This is a safety net that activates if HOOK entries are removed.

Both mechanisms reuse the same fallback implementations without code duplication.

## API Shim Inventory

### Synchronization Primitives (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `InitializeSRWLock` | Zero init | |
| `AcquireSRWLockExclusive` | Spin-lock with `InterlockedCompareExchange` | |
| `ReleaseSRWLockExclusive` | `InterlockedExchangePointer` | |
| `TryAcquireSRWLockExclusive` | Non-blocking CAS | |
| `AcquireSRWLockShared` | Same as exclusive (spin-lock) | Coarse-grained but safe |
| `ReleaseSRWLockShared` | `InterlockedExchangePointer` | |
| `TryAcquireSRWLockShared` | Non-blocking CAS | |
| `InitializeConditionVariable` | Zero init | |
| `SleepConditionVariableSRW` | `ERROR_CALL_NOT_IMPLEMENTED` | No emulation |
| `SleepConditionVariableCS` | `ERROR_CALL_NOT_IMPLEMENTED` | No emulation |
| `WakeConditionVariable` | `ERROR_CALL_NOT_IMPLEMENTED` | No-op stub |
| `WakeAllConditionVariable` | `ERROR_CALL_NOT_IMPLEMENTED` | No-op stub |

### One-Time Initialization (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `InitOnceBeginInitialize` | Critical-section-based state machine | `XP_INIT_ONCE_NEW/BUSY/DONE/FAILED` |
| `InitOnceComplete` | Sets state, leaves CS | |
| `InitOnceExecuteOnce` | Orchestrates begin/execute/complete | Calls wrapped begin + complete |

### Fiber Local Storage (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `FlsAlloc` | `TlsAlloc()` | |
| `FlsGetValue` | `TlsGetValue()` | |
| `FlsSetValue` | `TlsSetValue()` | |
| `FlsFree` | `TlsFree()` | |

### Thread Pool (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `CreateThreadpoolTimer` | Returns `NULL` | Thread pools unavailable on XP |
| `SetThreadpoolTimer` | No-op | |
| `WaitForThreadpoolTimerCallbacks` | No-op | |
| `CloseThreadpoolTimer` | No-op | |
| `CreateThreadpoolWait` | Returns `NULL` | |
| `SetThreadpoolWait` | No-op | |
| `CloseThreadpoolWait` | No-op | |
| `FreeLibraryWhenCallbackReturns` | No-op | |

### Locale (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `CompareStringEx` | `CompareStringW` with LCID mapping | `locale_name_to_lcid()` helper |
| `LCMapStringEx` | `LCMapStringW` with LCID mapping | |
| `GetLocaleInfoEx` | `GetLocaleInfoW` with LCID mapping | |

### Critical Section (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `InitializeCriticalSectionEx` | `InitializeCriticalSection` + spin count manual | Stores `SpinCount` in struct field |

### File System (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `CreateSymbolicLinkW` | `FALSE`, `ERROR_CALL_NOT_IMPLEMENTED` | Symlinks not supported on XP |
| `GetFileInformationByHandleEx` | `FALSE`, `ERROR_CALL_NOT_IMPLEMENTED` | |
| `SetFileInformationByHandle` | `FALSE`, `ERROR_CALL_NOT_IMPLEMENTED` | |

### Time / System Info (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `GetTickCount64` | `GetTickCount()` (32-bit) | Wraps every 49.7 days |
| `GetCurrentProcessorNumber` | Returns `0` | Single-processor assumption |
| `FlushProcessWriteBuffers` | No-op | Memory barriers handled differently on XP |
| `GetSystemTimePreciseAsFileTime` | `GetSystemTimeAsFileTime()` | Lower precision |

### Module / Pointer (VC2010-2012)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `EncodePointer` | `XorPointer()` | Thread-creation-time cookie XOR |
| `DecodePointer` | `XorPointer()` | |
| `GetModuleHandleExW` | `VirtualQuery` / `LoadLibraryW` | `GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS` handled |

### S-List (VC2015, x86 only)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `InitializeSListHead` | Zero init | |
| `InterlockedFlushSList` | `InterlockedCompareExchange64` loop | |
| `InterlockedPushEntrySList` | `InterlockedCompareExchange64` loop | |

### NUMA / Processor (VC2017)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `GetNumaHighestNodeNumber` | Returns `0` | Single-node assumption |
| `GetLogicalProcessorInformation` | Single-core mask | `ProcessorMask = 1` |

### Thread Stack (VC2019)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `SetThreadStackGuarantee` | `FALSE`, `ERROR_CALL_NOT_IMPLEMENTED` | |

### Events / Semaphores (Vista+)

| API | XP Fallback | Notes |
|-----|-------------|-------|
| `CreateEventExW` | `NULL`, `ERROR_CALL_NOT_IMPLEMENTED` | Use `CreateEventW` instead |
| `CreateSemaphoreExW` | `NULL`, `ERROR_CALL_NOT_IMPLEMENTED` | Use `CreateSemaphoreW` instead |

## Build Configuration

| Flag | Value | Location |
|------|-------|----------|
| `_WIN32_WINNT` | `0x0503` | `cmake/NetBox.cmake:21` |
| Subsystem (x86) | `/SUBSYSTEM:WINDOWS,5.01` | `cmake/NetBox.cmake:194` |
| CRT | Static (`/MT`) | `TargetConfiguration.cmake` |
| Delay-load DLLs | `ws2_32`, `oleaut32`, `shlwapi`, `crypt32` | `cmake/NetBox.cmake:224-227` |

## Known Limitations on Windows XP

| Feature | Status | Workaround |
|---------|--------|------------|
| Symbolic links | Not supported | No workaround |
| Thread pools | Not supported | Use threads directly |
| Condition variable sleep | Not supported | Use events + critical sections |
| `GetTickCount64` | 32-bit wraparound | Monitor for 49.7-day uptime |
| TLS 1.2+ | Requires KB2813430 | Install XP hotfix |
| `std::filesystem` | Partial | Avoid `std::filesystem::symlink_status` |

## Testing on Modern Windows

All wrappers are exercised on every build — simply running NetBox on Windows 10/11 calls the real APIs through the wrapper layer. To verify fallback code paths (the `implementation::impl()` functions), build with `/DWRAPPER_DEBUG` to trace each call.

## Adding a New Stub

1. **Implement fallback** in `vc_crt_fix_impl.cpp`:
   ```cpp
   extern "C" RETURN_TYPE WINAPI WRAPPER(FunctionName)(PARAMS)
   {
     struct implementation
     {
       static RETURN_TYPE WINAPI impl(PARAMS)
       {
         // XP fallback or ERROR_CALL_NOT_IMPLEMENTED
       }
     };
     CREATE_AND_RETURN(modules::kernel32, args...);
   }
   ```

2. **Add ASM export** in `vc_crt_fix.asm`:
   ```asm
   HOOK FunctionName, SIZE, :dword, ...
   ```

3. **Add delayload pragma** in `vc_crt_fix_ulink.cpp` (optional safety net):
   ```cpp
   #pragma comment(linker, "/delayload:kernel32.FunctionName")
   ```

4. **Add `DliFailureHook2` entry** in `vc_crt_fix_ulink.cpp` (if delayload added):
   ```cpp
   if(!lstrcmpA(pdli->dlp.szProcName, "FunctionName"))
     return (FARPROC)Wrapper_FunctionName;
   ```

5. **Add forward declaration** in `vc_crt_fix_ulink.cpp` (if delayload added):
   ```cpp
   extern "C" RETURN_TYPE WINAPI Wrapper_FunctionName(PARAMS);
   ```

## References

- Far Manager 3 `vc_crt_fix_impl.cpp` — original pattern
- Raymond Chen, "The Old New Thing" — per-function delayload rationale
- PR#500 — Restore support for work under WinXP when build with MSVC
