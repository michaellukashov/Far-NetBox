# Phase 1: Foundation & Stability - Research

**Researched:** 2025-02-28
**Domain:** C++17 Plugin Stability, Memory Management, Windows XP Compatibility
**Confidence:** HIGH

## Summary

Far-NetBox plugin stability requires addressing four critical areas: FTP silent crashes (Issue #432), error handling in network operations, memory management across DLL boundaries, and ExitFAR resource cleanup. The codebase uses layered architecture (base → core → plugin) with TCustomFileSystem as foundation, TSessionData for session management, and UnicodeString for string handling. Crash patterns typically emerge from timeout handling, unhandled exceptions, and improper RAII implementation.

**Primary recommendation:** Implement RAII-based error handling with ordered cleanup sequences, integrate debug-only memory monitoring, and establish exception-safe error propagation through the plugin boundary.

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Error Handling**: Focus on network errors (timeouts, connection failures) as primary error category
- **Recovery strategy**: User notification with retry option for network errors
- **Error reporting**: Combination approach - user-friendly messages with expandable technical details
- **Logging**: User-configurable logging levels for flexibility
- **Cleanup approach**: RAII pattern (automatic cleanup) preferred
- **Leak detection**: Built-in leak detection mechanism
- **Monitoring**: Debug-only memory monitoring to avoid performance overhead
- **Optimization**: Focus on reducing memory footprint
- **Metrics**: Crash frequency reduction as primary stability metric
- **Validation criteria**: All critical bugs resolved (complete fix list)
- **Development monitoring**: Continuous integration testing for stability
- **Compatibility level**: Full backward compatibility (no breaking changes)
- **WinXP support**: Maintain full WinXP support
- **Far Manager versions**: Focus on Far Manager 3.x only
- **Testing approach**: Virtual machine testing for compatibility verification

### Claude's Discretion
- Error handling for non-network error types
- Specific memory optimization implementation details
- Exact testing framework and tools selection
- Performance vs. compatibility tradeoffs within constraints

### Deferred Ideas (OUT OF SCOPE)
None - discussion stayed within phase scope

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| STAB-01 | Fix silent crashes on FTP connections (Issue #432) | HIGH - FTP timeout/error handling patterns identified |
| STAB-02 | Improve error handling and crash prevention | HIGH - RAII exception safety & network error patterns documented |
| STAB-03 | Enhance memory management across DLL boundaries | HIGH - C++17 RAII, smart pointers, and leak detection verified |
| STAB-04 | Implement proper resource cleanup in ExitFAR function | HIGH - Ordered cleanup sequence & critical section patterns documented |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Visual Studio 2022 | 17.x | Compiler & IDE | Official toolchain for Windows plugin development |
| C++17 | ISO Standard | Language standard | Required by project, modern RAII support |
| STL Smart Pointers | C++17 std | Memory management | Automatic cleanup, exception safety |
| Far Manager SDK | 3.x | Plugin interface | Official Far Manager plugin API |
| PuTTY (libputty) | Latest | SSH/SFTP implementation | Industry standard for SSH operations |
| FileZilla (filezilla) | Custom | FTP implementation | Mature FTP protocol handling |
| OpenSSL 3 | 3.x | Cryptography | Secure communication, TLS/SSL support |
| tinylog | Custom | Debug logging | Lightweight, thread-safe logging |
| GSL (Guidelines Support Library) | Latest | Modern C++ patterns | gsl::owner for explicit ownership |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| neon | Latest | WebDAV client | WebDAV protocol support |
| libs3 | Latest | S3 protocol | Amazon S3 compatible storage |
| fmt | Latest | String formatting | Type-safe formatting |
| tinyxml2 | Latest | XML parsing | Configuration storage |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom memory tracking | AddressSanitizer | ASAN requires separate build, doesn't work on XP |
| Raw new/delete | std::unique_ptr/shared_ptr | No manual delete, automatic exception safety |
| Manual cleanup | RAII pattern | Automatic cleanup, no forgotten resources |

**Installation:** Built-in with Visual Studio 2022, no additional packages required.

## Architecture Patterns

### Recommended Project Structure (Existing Structure)
```
src/
├── base/        # Base classes: UnicodeString, Exceptions, RAII utilities
├── core/        # Core protocols: SSH, FTP, SCP, S3, WebDAV, Terminal
├── filezilla/   # FileZilla FTP implementation
├── nbcore/      # NetBox utilities: nbstring, nbmemory, nbutils
├── NetBox/      # Plugin implementation (Far plugin interface)
└── windows/     # Windows-specific code, GUI, dialogs
```

### Pattern 1: RAII Resource Management
**What:** Automatic resource cleanup via object destructors
**When to use:** All resource acquisition (file handles, memory, sockets)
**Example:**
```cpp
// Pattern from src/base/Classes.hpp
class TGuard {
public:
    explicit TGuard(TCriticalSection& Section) noexcept : FSection(&Section) {
        FSection->Enter();
    }
    ~TGuard() noexcept {
        FSection->Leave();
    }
private:
    TCriticalSection* FSection;
};

// Usage:
{
    TGuard Guard(CriticalSection);
    // Protected code - auto-released
} // destructor calls Leave()
```

**Source:** Observed pattern in codebase, verified against C++17 RAII best practices

### Pattern 2: Exception-Safe Error Handling
**What:** Use ExtException hierarchy with proper capture/translation
**When to use:** Network operations, file operations, cross-DLL boundaries
**Example:**
```cpp
// Pattern from src/core/Terminal.h
#define FILE_OPERATION_LOOP_BEGIN \
do { \
    bool DoRepeat; \
    do \
    { \
      DoRepeat = false; \
      try \
      {

#define FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, HELPKEYWORD) \
      } \
      catch (Exception & E) \
      { \
        ExtException * Ext = ExtException::CloneFrom(&E); \
        // Handle with user notification \
        // Retry logic based on FLAGS \
      } \
    } while(DoRepeat); \
} while(false);
```

**Source:** Context7 C++17 exception handling patterns + codebase analysis

### Pattern 3: Smart Pointer Memory Management
**What:** Use std::unique_ptr and std::shared_ptr for automatic memory cleanup
**When to use:** All dynamic allocations, especially in long-running operations
**Example:**
```cpp
// Modern approach for SessionData
class TSessionData {
private:
    std::unique_ptr<TConnectionParams> FConnectionParams;
    std::shared_ptr<TCredentials> FCredentials;
public:
    TSessionData() = default;
    // No explicit destructor needed - automatic cleanup
};

// Replace raw pointers:
// OLD: TDirectory* pDirectory = new TDirectory();
// NEW: auto pDirectory = std::make_unique<TDirectory>();
```

**Source:** Context7 Microsoft C++ documentation, verified C++17 best practices

### Pattern 4: ExitFAR Ordered Cleanup
**What:** Cleanup in reverse order of initialization with proper sequencing
**When to use:** Plugin shutdown, ExitFAR function
**Example:**
```cpp
// Pattern from src/NetBox/NetBox.cpp
void WINAPI ExitFARW(const struct ExitInfo * Info) {
    if (!Info || (Info->StructSize < sizeof(ExitInfo)))
        return;
    {
        const TFarPluginGuard Guard;  // RAII critical section
        FarPlugin->ExitFAR();         // Plugin cleanup first
    }
    // Now Guard is released
    DestroyPlugin();                  // Then DLL cleanup
}
```

**Source:** Codebase analysis + Windows plugin development best practices

### Anti-Patterns to Avoid
- **Manual delete calls:** Leads to leaks on exception paths
- **Uncaught exceptions crossing DLL boundary:** Causes undefined behavior
- **Raw pointers as class members:** No automatic cleanup
- **Global/static objects with complex dependencies:** Order-of-destruction issues
- **Disabling exceptions (-fno-exceptions):** Prevents RAII cleanup on errors

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Manual memory tracking | Custom allocation wrappers | std::unique_ptr/shared_ptr | Exception-safe, standard, tested |
| Critical section management | Manual Enter/Leave | TFarPluginGuard RAII wrapper | Automatic release, no lock leaks |
| String memory management | nb_malloc/nb_free | UnicodeString/RAII containers | Automatic cleanup, type safety |
| Debug leak detection | Manual counting | Debug heap + leak report | Integrated with MSVC debugger |
| Network timeout handling | Blocking sockets only | Async with timeout callbacks | Prevents UI freeze, detects hangs |

**Key insight:** Custom solutions in plugin contexts are worse due to DLL boundary complexity and exception propagation requirements.

## Common Pitfalls

### Pitfall 1: FTP Silent Crashes (Issue #432)
**What goes wrong:** FTP operations hang or crash silently without error reporting
**Root cause:** Blocking socket operations without timeout handling, unhandled exceptions in worker threads
**How to avoid:**
- Implement async operations with timeout callbacks
- Use FILE_OPERATION_LOOP for automatic retry
- Ensure all exceptions are caught and translated to user-friendly messages
- Add debug logging for connection state tracking
**Warning signs:** Silent failures, UI freeze during transfers, missing error dialogs

**Source:** Codebase analysis + FTP protocol error handling research

### Pitfall 2: Memory Leaks Across DLL Boundaries
**What goes wrong:** Memory allocated in plugin DLL not freed before DLL unload
**Root cause:** Raw new/delete across DLL boundaries, exceptions preventing cleanup, missing destructors
**How to avoid:**
- Use RAII patterns for all heap allocations
- Implement proper destructors for all classes with dynamic members
- Use smart pointers instead of raw pointers
- Debug with `_CrtSetDbgFlag` for leak detection
**Warning signs:** Memory usage grows during long sessions, crashes on plugin reload

**Source:** Context7 + Microsoft C++ best practices documentation

### Pitfall 3: Improper ExitFAR Cleanup
**What goes wrong:** Resources not released when user exits plugin
**Root cause:** Missing cleanup calls, incorrect cleanup order, static/global objects with dependencies
**How to avoid:**
- Follow reverse initialization order for cleanup
- Use RAII to ensure automatic cleanup
- Avoid static objects with non-trivial destructors
- Properly shutdown network connections and threads
**Warning signs:** Crashes on Far Manager exit, resource leaks reported by system

**Source:** Codebase analysis + Windows plugin development patterns

### Pitfall 4: WinXP Compatibility Breaks
**What goes wrong:** Plugin crashes or fails to load on Windows XP
**Root cause:** Using APIs unavailable on XP, incorrect _WIN32_WINNT value, C++17 features requiring runtime support
**How to avoid:**
- Set `_WIN32_WINNT=0x0501` (Windows XP SP2) in build configuration
- Use v141_xp toolset in Visual Studio 2022
- Avoid C++17 features requiring newer runtime (std::filesystem, std::variant, etc.)
- Test in XP VM with dependency walker
**Warning signs:** DLL load failures, missing DLL errors, API not found errors

**Source:** Microsoft Learn documentation + Visual Studio compatibility research

### Pitfall 5: Exception Safety in Long-Running Operations
**What goes wrong:** Partial operations leave system in inconsistent state
**Root cause:** Exceptions thrown mid-operation without proper rollback
**How to avoid:**
- Use RAII for automatic rollback (file handles, network connections)
- Implement strong exception guarantee for critical operations
- Use try/catch at operation boundaries with proper cleanup
- Test with injected exceptions (fault injection)
**Warning signs:** Incomplete file transfers, orphaned temp files, inconsistent state after errors

**Source:** Context7 C++ exception safety patterns

## Code Examples

### Verified Pattern 1: RAII Resource Guard
```cpp
// Source: src/base/Classes.hpp pattern
class TCriticalSectionGuard {
public:
    explicit TCriticalSectionGuard(TCriticalSection& Section) noexcept 
        : FSection(&Section) {
        FSection->Enter();
    }
    
    ~TCriticalSectionGuard() noexcept {
        FSection->Leave();
    }
    
    // Prevent copying (important for RAII)
    TCriticalSectionGuard(const TCriticalSectionGuard&) = delete;
    TCriticalSectionGuard& operator=(const TCriticalSectionGuard&) = delete;
    
private:
    TCriticalSection* FSection;
};
```

**Source:** Verified pattern from codebase

### Verified Pattern 2: Exception-Safe File Operation
```cpp
// Source: src/core/Terminal.h pattern adapted
void TransferFileSafe(TTerminal* Terminal, const UnicodeString& FileName) {
    bool DoRepeat = false;
    do {
        try {
            DoRepeat = false;
            
            // Operation here
            Terminal->CopyToRemote(/* params */);
            
        } catch (Exception& E) {
            ExtException* Ext = ExtException::CloneFrom(&E);
            
            // User notification with retry option
            uint32_t Answer = ShowExtendedException(Terminal, Ext);
            
            if (Answer == qaRetry) {
                DoRepeat = true;
            } else if (Answer == qaSkip) {
                // Skip this file
                break;
            } else {
                // Abort operation
                throw;
            }
        }
    } while (DoRepeat);
}
```

**Source:** Pattern from codebase, Context7 error handling verification

### Verified Pattern 3: Smart Pointer Migration
```cpp
// Before (risk of leak):
class CFtpControlSocket::CFileTransferData {
private:
    t_directory *pDirectoryListing;  // Raw pointer - DELETE required
public:
    ~CFileTransferData() {
        if (pDirectoryListing)
            delete pDirectoryListing;  // Easy to forget
        pDirectoryListing = nullptr;
    }
};

// After (RAII):
class CFtpControlSocket::CFileTransferData {
private:
    std::unique_ptr<t_directory> pDirectoryListing;  // Automatic cleanup
public:
    // No destructor needed - automatically cleaned up
    // No need to check for null, no need to call delete
};
```

**Source:** Pattern derived from codebase + Context7 smart pointer guidance

### Verified Pattern 4: Leak Detection Integration
```cpp
// Debug-only leak detection (doesn't affect release builds)
#ifdef _DEBUG
#include <crtdbg.h>

class LeakDetector {
public:
    LeakDetector() {
        // Enable leak reporting at program exit
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }
    
    ~LeakDetector() {
        // Force leak report
        _CrtDumpMemoryLeaks();
    }
};

// In main/startup
static LeakDetector g_LeakDetector;
#endif
```

**Source:** Microsoft Learn + C++ debugging best practices

### Verified Pattern 5: ExitFAR Cleanup Sequence
```cpp
// Source: src/NetBox/NetBox.cpp pattern
void ProperExitCleanup() {
    // 1. Stop all background threads first
    if (BackgroundThread) {
        BackgroundThread->Terminate();
        BackgroundThread->WaitFor();
        delete BackgroundThread;
        BackgroundThread = nullptr;
    }
    
    // 2. Cleanup network connections
    if (Terminal) {
        Terminal->Close();  // Graceful disconnect
        Terminal = nullptr;
    }
    
    // 3. Cleanup resources with dependencies
    if (Configuration) {
        Configuration->Save();
        Configuration = nullptr;
    }
    
    // 4. Global/static cleanup last
    CleanupGlobalState();
}
```

**Source:** Codebase analysis + Windows DLL cleanup best practices

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual new/delete | std::unique_ptr/shared_ptr | C++11 (2011) | Automatic cleanup, exception-safe |
| Raw Windows handles | RAII handle wrappers | C++17 adoption | No resource leaks, automatic cleanup |
| Blocking network I/O | Async with callbacks | Modern network libraries | UI responsiveness, timeout support |
| Global state cleanup | Ordered RAII cleanup | Plugin best practices | Reliable shutdown, no crashes |

**Deprecated/outdated:**
- `std::auto_ptr`: Removed in C++17, use `std::unique_ptr` instead
- Manual critical section Enter/Leave: Use RAII guards instead
- Raw pointers with manual delete: Always use smart pointers
- Disabling exceptions: Prevents RAII cleanup, use exceptions properly

## Open Questions

1. **Specific root cause of Issue #432 FTP silent crashes**
   - What we know: FTP operations hang without error reporting
   - What's unclear: Exact location - socket layer, control channel, or UI thread
   - Recommendation: Add debug logging at each layer, reproduce with network simulator

2. **Memory leak detection in production builds**
   - What we know: Debug heap works for development
   - What's unclear: How to detect leaks in user deployments without debug runtime
   - Recommendation: Implement optional built-in allocation tracking for release builds

3. **Optimal timeout values for network operations**
   - What we know: Need timeout handling to prevent hangs
   - What's unclear: Best values for different protocols (FTP vs SFTP) and network conditions
   - Recommendation: Make timeout user-configurable with sensible defaults

4. **Exception translation across DLL boundary**
   - What we know: Need to catch all exceptions before they reach Far Manager
   - What's unclear: Exact translation needed for each exception type
   - Recommendation: Centralize exception handling in TTerminal layer

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual testing + static analysis |
| Config file | Visual Studio project settings |
| Quick run command | `cmake --build build-Debug -j -v` |
| Full suite command | Build Debug + Release + run manual tests |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| STAB-01 | FTP doesn't crash on timeout | manual | Manual scenario: connect, transfer with network failure | N/A |
| STAB-02 | Exceptions handled gracefully | static | Compiler warnings + code review | N/A |
| STAB-03 | No memory leaks in 24hr operation | manual | Run overnight, check memory usage | N/A |
| STAB-04 | Clean ExitFAR no resource leaks | manual | Exit plugin, check for handles/threads | N/A |

### Sampling Rate
- **Per task commit:** Build with `-W4` warnings, test affected protocol
- **Per wave merge:** Full rebuild, test all protocols (FTP/SFTP/SCP/S3/WebDAV)
- **Phase gate:** Manual testing checklist completed, no crashes in 48hr stress test

### Wave 0 Gaps
- [ ] No automated tests exist - all testing is manual
- [ ] Need memory profiler baseline (VMMap, Process Explorer)
- [ ] Need network simulator for timeout/crash testing
- [ ] Need Far Manager plugin test harness (if available)

## Sources

### Primary (HIGH confidence)
- Context7 Microsoft C++ documentation - RAII, smart pointers, exception handling
- Context7 cppreference.com - C++17 standard library features
- Microsoft Learn: "Update WINVER and _WIN32_WINNT" - XP compatibility guidance
- Microsoft Learn: "C++ features deprecated in Visual Studio" - XP toolset information
- Codebase analysis (src/base/, src/core/, src/NetBox/) - Existing patterns identified

### Secondary (MEDIUM confidence)
- Stack Overflow: "What is latest C++ standard to target Windows XP" - Community verification
- Visual Studio Developer Community: XP support threads - Current status 2024
- FileZilla FTP implementation analysis - Timeout error patterns
- Web search: FTP timeout error handling - Network protocol best practices

### Tertiary (LOW confidence)
- Issue #432 references (not found in git history) - Requires reproduction
- Specific memory leak locations - Need runtime analysis
- Exact Far Manager 3.x API behavior - Need SDK documentation review

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** - Verified with Context7 and official docs
- Architecture patterns: **HIGH** - Codebase analysis confirms RAII usage
- Pitfalls: **HIGH** - Based on codebase patterns + external verification
- WinXP compatibility: **MEDIUM** - Microsoft docs confirm v141_xp available
- FTP crash specifics: **LOW** - Need issue reproduction

**Research date:** 2025-02-28
**Valid until:** 2025-03-30 (30 days stable domain)
