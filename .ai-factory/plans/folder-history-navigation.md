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

### Active Summary
- **Topic:** Far Manager folder history integration for session navigation
- **Goal:** Allow users to navigate to previously visited NetBox sessions via Far Manager's folder history mechanism
- **Key constraint:** Far Manager 3.0.5955 SDK does not expose `FCTL_GETFOLDERHISTORY`/`FCTL_SETFOLDERHISTORY`; instead uses `FCTL_GETPANELDIRECTORY`/`FCTL_SETPANELDIRECTORY` with `FarPanelDirectory` struct that includes a `Param` field for plugin-specific data
- **Architecture:** Plugin Layer (NetBox/) → Core Layer (core/) → Base Layer (base/, nbcore/) → Third-Party (libs/)

## Overview

Implement integration with Far Manager's folder history mechanism so that when users navigate to a NetBox plugin panel through Far Manager's history (e.g., folder history, command line history, or panel directory history), the plugin correctly restores the session and directory state. The implementation leverages the `FarPanelDirectory::Param` field to store session identification data, enabling Far Manager to track and restore NetBox panel state through its built-in history mechanisms.

## Tasks

### Phase 1: Analysis and Design

- [ ] **TASK-1: Analyze FarPanelDirectory usage and history flow**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (lines 2215-2246), `src/PluginSDK/Far3/plugin.hpp` (lines 1125-1132, 1158, 1171)
  - **Deliverable:** Document how `FCTL_SETPANELDIRECTORY` and `FCTL_GETPANELDIRECTORY` are currently used in `SynchronizeBrowsing()` and identify all call sites that set panel directory
  - **Logging:** DEBUG log entry point analysis results
  - **Dependency:** None

- [ ] **TASK-2: Design session serialization format for FarPanelDirectory::Param**
  - **Files:** `src/core/SessionData.h`, `src/NetBox/WinSCPFileSystem.h`
  - **Deliverable:** Define a compact string format for `FarPanelDirectory::Param` that encodes session name and remote directory (e.g., `session_name|remote_directory` or URL format compatible with existing `OPEN_SHORTCUT` parsing)
  - **Constraints:** Must be backward compatible with existing `OPEN_SHORTCUT`/`OPEN_COMMANDLINE` parsing in `OpenPluginEx()` (lines 307-376 of `WinSCPPlugin.cpp`); must handle special characters in session names and paths
  - **Logging:** DEBUG log format design decisions
  - **Dependency:** TASK-1

### Phase 2: Core Implementation

- [ ] **TASK-3: Implement session state encoding/decoding utilities**
  - **Files:** Create `src/nbcore/SessionHistory.h` and `src/nbcore/SessionHistory.cpp`
  - **Deliverable:** 
    - `EncodeSessionParam(sessionName, remoteDirectory)` → returns `UnicodeString` for `FarPanelDirectory::Param`
    - `DecodeSessionParam(param)` → returns struct with `SessionName` and `RemoteDirectory`
    - Unit tests for encoding/decoding with edge cases (empty directory, special characters, Unicode session names)
  - **Logging:** DEBUG log all encode/decode operations with input/output
  - **Dependency:** TASK-2

- [ ] **TASK-4: Update SetDirectoryEx to populate FarPanelDirectory::Param**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (lines 2215-2246, 2249-2350)
  - **Deliverable:** Modify `SynchronizeBrowsing()` and `SetDirectoryEx()` to set `FarPanelDirectory::Param` with encoded session state when calling `FCTL_SETPANELDIRECTORY`
  - **Pattern:** Follow existing `FarPanelDirectory` usage pattern at line 2215-2219
  - **Logging:** DEBUG log when setting panel directory with session param; WARN if encoding fails
  - **Dependency:** TASK-3

- [ ] **TASK-5: Handle panel restoration from FarManager history**
  - **Files:** `src/NetBox/WinSCPPlugin.cpp` (lines 284-413), `src/NetBox/FarPlugin.cpp` (lines 428-472)
  - **Deliverable:** When `OpenPluginEx()` is called with `OPEN_SHORTCUT` or `OPEN_COMMANDLINE` from Far Manager history, parse the `Param` field to extract session name and directory, then restore the session automatically without showing session list
  - **Integration:** Reuse existing `OPEN_SHORTCUT` parsing flow (lines 307-376) — extend to handle `Param`-based session restoration
  - **Edge cases:** Session no longer exists in storage; session configuration changed; remote directory no longer accessible
  - **Logging:** DEBUG log history restoration flow; ERROR log if session not found; WARN if directory restoration fails
  - **Dependency:** TASK-3, TASK-4

### Phase 3: Integration and Edge Cases

- [ ] **TASK-6: Integrate with existing session focus restoration (FPrevSessionName)**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (lines 571-580, 3258)
  - **Deliverable:** When restoring from history, update `FPrevSessionName` to ensure correct focus when returning to session list; ensure history entries don't interfere with existing session list navigation
  - **Logging:** DEBUG log focus restoration state
  - **Dependency:** TASK-5

- [ ] **TASK-7: Handle multi-panel scenarios and panel lifecycle**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (lines 2115, 3369-3387), `src/NetBox/FarPlugin.cpp` (lines 474-510)
  - **Deliverable:** 
    - Ensure history entries are correctly maintained when panels are closed/reopened
    - Clear history state on `ClearConnectedState()` (line 2115) to prevent stale entries
    - Handle case where user has two NetBox panels open with different sessions
  - **Logging:** DEBUG log panel lifecycle events affecting history
  - **Dependency:** TASK-4, TASK-5

- [ ] **TASK-8: Ensure compatibility with existing path history (FPathHistory)**
  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (lines 3369-3387), `src/NetBox/WinSCPDialogs.cpp` (lines 6990-7139)
  - **Deliverable:** Verify that Far Manager folder history integration does not conflict with existing per-connection `FPathHistory` used in Open Directory dialog; both mechanisms should coexist
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
  - **Deliverable:** Verify:
    - Build succeeds with zero warnings (MSVC W4) across x86, x64, ARM64
    - Plugin DLL correctly placed in `Far3_<platform>/Plugins/NetBox/`
    - Manual test: Open session → navigate directories → use Far Manager folder history → verify session restores correctly
    - Manual test: Multiple sessions in history → verify correct session restored each time
    - Manual test: Session deleted from storage → verify graceful error handling
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
- [ ] Manual testing completed per TASK-10 checklist
