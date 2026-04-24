# Implementation Plan: Far Manager Folder History Integration for NetBox Plugin

**Branch:** feature/folder-history-navigation
**Created:** 2026-04-25
**Type:** Feature

## Settings

- **Testing:** Yes — unit tests for session URL parsing and history entry serialization
- **Logging:** Verbose — detailed DEBUG logs via `FTerminal->LogEvent()` for all history operations
- **Docs:** Yes — mandatory documentation checkpoint at completion
- **Roadmap Linkage:** Milestone: "UI/UX improvements" — enhances navigation workflow for users

## Research Context

- **Topic:** Far Manager folder history integration for session navigation
- **Goal:** Allow users to navigate to previously visited NetBox sessions via Far Manager's folder history mechanism
- **Key constraint:** Far Manager 3.0.5955 SDK uses `FCTL_GETPANELDIRECTORY`/`FCTL_SETPANELDIRECTORY` with `FarPanelDirectory` struct; `Param` field stores plugin-specific session data
- **Architecture:** Plugin→Core→Base→ThirdParty (see AGENTS.md)

## Overview

Implement integration with Far Manager's folder history mechanism so that when users navigate to a NetBox plugin panel through Far Manager's history (e.g., folder history, command line history, or panel directory history), the plugin correctly restores the session and directory state. The implementation leverages the `FarPanelDirectory::Param` field to store session identification data using a URL-encoded format (`netbox://<session_name>/<remote_directory>`), enabling Far Manager to track and restore NetBox panel state through its built-in history mechanisms.

## Tasks

### Phase 1: Analysis and Design

- [ ] **TASK-1: Analyze FarPanelDirectory usage and history flow**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`SynchronizeBrowsing()`, `SetDirectoryEx()`), `src/PluginSDK/Far3/plugin.hpp` (`FarPanelDirectory` struct, `FCTL_SETPANELDIRECTORY`)
  - **Deliverable:** Document how `FCTL_SETPANELDIRECTORY` and `FCTL_GETPANELDIRECTORY` are currently used; identify all call sites that set panel directory
  - **Logging:** DEBUG log entry point analysis results
  - **Dependency:** None

- [ ] **TASK-2: Define session serialization format for FarPanelDirectory::Param**
  - **Files:** `src/core/SessionData.h`, `src/nbcore/` (URL encoding utilities)
  - **Deliverable:** Use URL-encoded format: `netbox://<session_name>/<remote_directory>` where `<session_name>` and `<remote_directory>` are percent-encoded per RFC 3986. This format is:
    - Parseable with existing URL parsing utilities in `nbcore/`
    - Safe for all Unicode characters and special symbols
    - Distinguishable from regular paths by `netbox://` scheme prefix
    - Compatible with existing `OPEN_SHORTCUT` parsing (already handles URL-like strings)
  - **Constraints:** Must be backward compatible with existing `OPEN_SHORTCUT`/`OPEN_COMMANDLINE` parsing in `OpenPluginEx()`; must handle special characters in session names and paths
  - **Logging:** DEBUG log format design decisions
  - **Dependency:** TASK-1

### Phase 2: Core Implementation

- [ ] **TASK-3: Implement session state encoding/decoding utilities**
  - **Files:** Create `src/nbcore/SessionHistory.h` and `src/nbcore/SessionHistory.cpp`
  - **Deliverable:**
    - `EncodeSessionParam(const UnicodeString& sessionName, const UnicodeString& remoteDirectory) → UnicodeString`
    - `DecodeSessionParam(const UnicodeString& param) → TSessionHistoryEntry { SessionName, RemoteDirectory }`
    - Unit tests for encoding/decoding with edge cases (empty directory, special characters, Unicode session names)
  - **Logging:** DEBUG log all encode/decode operations with input/output
  - **Dependency:** TASK-2

- [ ] **TASK-4: Update SetDirectoryEx to populate FarPanelDirectory::Param**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`SynchronizeBrowsing()`, `SetDirectoryEx()`)
  - **Deliverable:** Modify `SynchronizeBrowsing()` and `SetDirectoryEx()` to set `FarPanelDirectory::Param` with encoded session state when calling `FCTL_SETPANELDIRECTORY`
  - **Pattern:** Follow existing `FarPanelDirectory` usage pattern
  - **Error handling:** If encoding fails: (1) Log ERROR with details, (2) Set `Param = nullptr` to maintain backward compatibility, (3) Continue with directory set operation — history tracking degrades gracefully
  - **Logging:** DEBUG log when setting panel directory with session param; WARN if encoding fails
  - **Dependency:** TASK-3

- [ ] **TASK-5: Handle panel restoration from FarManager history**
  - **Files:** `src/NetBox/WinSCPPlugin.cpp` (`OpenPluginEx()`), `src/NetBox/FarPlugin.cpp` (`OpenPlugin()`)
  - **Deliverable:** When `OpenPluginEx()` is called with `OPEN_SHORTCUT` or `OPEN_COMMANDLINE` from Far Manager history, parse the `Param` field to extract session name and directory, then restore the session automatically without showing session list
  - **Integration:** Reuse existing `OPEN_SHORTCUT` parsing flow — extend to handle `Param`-based session restoration
  - **Error handling matrix:**
    | Failure Scenario | User-Facing Behavior | Log Level | Fallback |
    |------------------|---------------------|-----------|----------|
    | Session not found in storage | Error dialog: "Session not found" | ERROR | Show session list |
    | Remote directory inaccessible | Warning banner: "Directory not found, using home" | WARN | Navigate to home directory |
    | Session config changed (credentials) | Reuse existing reconnect dialog | INFO | Standard reconnect flow |
    | Malformed Param in history entry | Ignore entry, show session list | WARN | Standard session selection |
  - **Logging:** DEBUG log history restoration flow; ERROR log if session not found; WARN if directory restoration fails
  - **Dependency:** TASK-3

### CHECKPOINT: Core Functionality Verification

After completing TASK-5:
1. Build with `cmd /c build-x64.bat` — zero warnings required
2. Manual test: Open session → navigate to subdirectory → close panel → use Far Manager folder history → verify session restores to correct directory
3. If test fails: STOP, debug, fix. Do not proceed to Phase 3 until this passes.
4. Log output: Verify DEBUG logs show encode/decode flow and restoration path.

### Phase 3: Integration and Edge Cases

- [ ] **TASK-6: Integrate with existing session focus restoration (FPrevSessionName)**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`FPrevSessionName` usage)
  - **Deliverable:** When restoring from history, update `FPrevSessionName` to ensure correct focus when returning to session list; ensure history entries don't interfere with existing session list navigation
  - **Logging:** DEBUG log focus restoration state
  - **Dependency:** TASK-5

- [ ] **TASK-7: Handle multi-panel scenarios and panel lifecycle**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`ClearConnectedState()`), `src/NetBox/FarPlugin.cpp` (panel lifecycle)
  - **Deliverable:**
    - Verify that each panel maintains independent `Param` state. No cross-panel synchronization required. Each panel's history entry is self-contained.
    - Clear history state on `ClearConnectedState()` to prevent stale entries
    - Handle case where user has two NetBox panels open with different sessions — each panel's history entry is independent
  - **Logging:** DEBUG log panel lifecycle events affecting history
  - **Dependency:** TASK-4, TASK-5

- [ ] **TASK-8: Ensure compatibility with existing path history (FPathHistory)**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`TerminalChangeDirectory`, `FPathHistory`), `src/NetBox/WinSCPDialogs.cpp` (Open Directory dialog)
  - **Deliverable:** Write integration test that opens session, navigates, triggers history restore, and asserts no `FPathHistory` corruption; both mechanisms should coexist
  - **Logging:** DEBUG log when both history systems are updated
  - **Dependency:** TASK-4

### Phase 4: Testing and Verification

- [ ] **TASK-9: Write unit tests for session history utilities**
  - **Files:** Create `tests/nbcore/SessionHistoryTest.cpp`
  - **Deliverable:** Tests for:
    - Basic encode/decode round-trip
    - Empty remote directory
    - Unicode session names (Russian, Polish characters)
    - Special characters in paths (`|`, `&`, spaces)
    - Very long session names and paths
    - Malformed param strings (graceful degradation)
  - **Logging:** N/A (test code)
  - **Dependency:** TASK-3

- [ ] **TASK-10: Manual testing checklist and plugin verification**
  - **Files:** `src/NetBox/`, `Far3_x64/Plugins/NetBox/`
  - **Deliverable:** Verify with measurable criteria:
    - Build succeeds with zero warnings (MSVC W4) across x86, x64, ARM64
    - Plugin DLL correctly placed in `Far3_<platform>/Plugins/NetBox/`
    - **Pass criteria for history restoration:** (1) Session connects within 5s, (2) Remote directory matches history entry, (3) File list displays without errors, (4) Console title shows session name
    - Manual test: Open session → navigate directories → use Far Manager folder history → verify session restores correctly per pass criteria
    - Manual test: Multiple sessions in history → verify correct session restored each time
    - Manual test: Session deleted from storage → verify graceful error handling (shows session list)
    - No modifications to `libs/` directory
    - CRLF line endings on all modified files
  - **Logging:** Verify verbose logging works during manual testing
  - **Dependency:** TASK-5, TASK-6, TASK-7, TASK-8, TASK-9

## Commit Plan

**Checkpoint 1 (after TASK-3):**
`feat(history): add session param encoding/decoding utilities`

**Checkpoint 2 (after TASK-5):**
`feat(history): integrate FarPanelDirectory param with SetDirectory and OpenPlugin`

**Checkpoint 3 (after TASK-8):**
`feat(history): handle edge cases and multi-panel scenarios`

**Checkpoint 4 (after TASK-10):**
`feat(history): add tests and complete folder history integration`

## Build Configuration

- **Build type:** RelWithDebugInfo
- **Platforms:** x86, x64, ARM64
- **CMake options:** `OPT_CREATE_PLUGIN_DIR=ON` (for plugin directory), `OPT_USE_UNITY_BUILD=OFF` (if symbol redefinition errors occur)
- **Build command:** `cmd /c build-x64.bat` (same cmd session for configure and build)

## Quality Gates

- [ ] Clean build with zero warnings (MSVC W4)
- [ ] No modifications to `libs/`
- [ ] Plugin DLLs in `Far3_<platform>/Plugins/NetBox/`
- [ ] CRLF line endings on all modified files
- [ ] No BOM (UTF-8 without BOM)
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments
- [ ] All unit tests pass
- [ ] Manual testing completed per TASK-10 checklist with all pass criteria met
