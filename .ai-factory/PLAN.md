# Plan: Fix worker-thread OperationProgress guard

**Created:** 2026-05-10
**Branch:** lmv/dev (current)
**Description:** Add thread-id guard to `TWinSCPFileSystem::OperationProgress` to prevent Far Manager API calls from worker threads.

## Settings

| Setting | Value |
|---------|-------|
| Testing | No |
| Logging | Verbose |
| Docs | No (warn-only) |
| Build type | RelWithDebugInfo |
| Platform | x64 |

## Roadmap Linkage

Milestone: "none"
Rationale: "Skipped by user"

## Context

### Problem

`TWinSCPFileSystem::OperationProgress` makes direct Far Manager API calls (`SaveScreen`, `RestoreScreen`, `Message`, `ShowConsoleTitle`, `UpdateConsoleTitleProgress`, `CheckForEsc`) without verifying it runs on the main thread. Far Manager requires all plugin-side UI work on the main thread; worker-thread calls corrupt internal state or crash.

### Existing Protection

- `TTerminalThread` marshals `OnProgress`/`OnFinished` callbacks to the main thread for background queue operations (`Queue.cpp:2817-2818`, `3247-3260`).
- `TTunnelUI` uses `GetCurrentThreadId() == FTerminalThreadID` guard pattern (`Terminal.cpp:276-354`).
- `TKeepAliveThread` already uses `PostMainThreadSynchro()` for cross-thread idle work.

### Risk Assessment

| Scenario | Thread | Marshaled? | Risk |
|----------|--------|------------|------|
| Foreground file ops | Main | N/A | Safe |
| Queue item transfer | Worker | TTerminalThread replaces callbacks | Safe (marshal handles) |
| Background sync (`TSynchronizeController`) | Worker? | Unknown | **Potential gap** |
| Terminal reconnect | Worker | TTerminalThread if queue | Safe |

### Decision

Defensive guard: check thread ID at entry. If worker thread, log warning and return early. This matches `TTunnelUI` pattern and prevents crashes if an un-marshaled path calls `OperationProgress`.

## Tasks

### Phase 1: Add thread-id guard to OperationProgress

**Task 1.1:** Add `FMainThreadId` member and initialization to `TWinSCPFileSystem`

- **Target:** `src/NetBox/WinSCPFileSystem.h` (class declaration)
- **Changes:**
  - Add `DWORD FMainThreadId{0};` private member
- **Target:** `src/NetBox/WinSCPFileSystem.cpp` (constructor)
- **Changes:**
  - Initialize `FMainThreadId = ::GetCurrentThreadId();` in constructor
- **Verify:** Compile check ✅

**Task 1.2:** Add thread-id guard to `OperationProgress`

- **Target:** `src/NetBox/WinSCPFileSystem.cpp` line 3878 (`OperationProgress`)
- **Changes:**
  - Add guard at top of method:
    ```cpp
    if (::GetCurrentThreadId() != FMainThreadId)
    {
      TINYLOG_WARNING(g_tinylog) << TLogContext::Format()
          << "OperationProgress skipped: worker thread "
          << std::to_string(::GetCurrentThreadId()) << " != main "
          << std::to_string(FMainThreadId);
      return;
    }
    ```
- **Logging:** Log at WARN level when bypass fires (detects un-marshaled paths)
- **Verify:** Compile check ✅

### Phase 2: Build and verify

**Task 2.1:** Build with MSVC x64 RelWithDebugInfo

- **Target:** Run `cmd /c build-x64.bat`
- **Changes:** None — verification only
- **Verify:** Zero warnings, zero errors ✅
**Task 2.2:** Verify plugin DLL location

- **Target:** Check `Far3_x64/Plugins/NetBox/NetBox.dll` exists
- **Verify:** DLL is in correct location ✅
## Commit Plan

Single commit:

```
fix(netbox): guard OperationProgress against worker-thread Far API calls

- Add FMainThreadId member to TWinSCPFileSystem, initialized in constructor
- Add thread-id check at entry of OperationProgress()
- Log warning when bypassed to detect un-marshaled paths
- Matches TTunnelUI pattern used elsewhere in codebase
```

## Anti-Patterns

- **DO NOT** modify files in `libs/` (use patches)
- **DO NOT** combine shell commands with `&&` or `||`
- **DO NOT** use Unix-style redirections (`>/dev/null`, `2>/dev/null`)
- **DO NOT** rewrite entire files — make surgical edits
- **DO** verify CRLF line endings on all modified files
- **DO** verify UTF-8 without BOM
- **DO** verify zero trailing whitespace

## Next Steps

```
/aif-implement

CONTEXT FROM /aif-plan:
- Plan file: .ai-factory/PLAN.md
- Testing: no
- Logging: verbose
- Docs: no (warn-only)
```
