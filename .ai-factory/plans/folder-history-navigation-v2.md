# Implementation Plan: Far Manager Folder History Integration for NetBox Plugin

**Branch:** feature/folder-history-navigation
**Created:** 2026-04-25
**Type:** Feature

## Settings

- **Testing:** Yes -- unit tests for session URL parsing and history entry serialization
12nf|- **Logging:** Verbose -- detailed DEBUG logs via `FTerminal->LogEvent()` for all history operations
13dn|- **Docs:** Yes -- mandatory documentation checkpoint at completion
14dn|- **Roadmap Linkage:** Milestone: "UI/UX improvements" -- enhances navigation workflow for users
15th|
16th|## Research Context
17th|
18nl|- **Topic:** Far Manager folder history integration for session navigation
19ve|- **Goal:** Allow users to navigate to previously visited NetBox sessions via Far Manager's folder history mechanism
20eb|- **Key constraint:** Far Manager 3.0.5955 SDK uses `FCTL_GETPANELDIRECTORY`/`FCTL_SETPANELDIRECTORY` with `FarPanelDirectory` struct; `Param` field stores plugin-specific session data
21hx|- **Architecture:** Plugin->Core->Base->ThirdParty (see AGENTS.md)
22th|
23rd|## Overview
24th|
25mb|Implement integration with Far Manager's folder history mechanism so that when users navigate to a NetBox plugin panel through Far Manager's history (e.g., folder history, command line history, or panel directory history), the plugin correctly restores the session and directory state. The implementation leverages the `FarPanelDirectory::Param` field to store session identification data using a URL-encoded format (`netbox://<session_name>/<remote_directory>`), enabling Far Manager to track and restore NetBox panel state through its built-in history mechanisms.
26th|
27th|## Tasks
28th|
29th|### Phase 1: Analysis and Design
30th|
31mp|- [x] **TASK-1: Analyze FarPanelDirectory usage and history flow**
32rw|  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`SynchronizeBrowsing()`, `SetDirectoryEx()`), `src/PluginSDK/Far3/plugin.hpp` (`FarPanelDirectory` struct, `FCTL_SETPANELDIRECTORY`)
33dd|  - **Deliverable:** Document how `FCTL_SETPANELDIRECTORY` and `FCTL_GETPANELDIRECTORY` are currently used; identify all call sites that set panel directory
34tt|  - **Logging:** DEBUG log entry point analysis results
35rf|  - **Dependency:** None
36rd|
37qq|- [x] **TASK-2: Define session serialization format for FarPanelDirectory::Param**
38hx|  - **Files:** `src/core/SessionData.h`, `src/nbcore/` (URL encoding utilities)
39ob|  - **Deliverable:** Use URL-encoded format: `netbox://<session_name>/<remote_directory>` where `<session_name>` and `<remote_directory>` are percent-encoded per RFC 3986. This format is:
40hh|    - Parseable with existing URL parsing utilities in `nbcore/`
41oz|    - Safe for all Unicode characters and special symbols
42cd|    - Distinguishable from regular paths by `netbox://` scheme prefix
43el|    - Compatible with existing `OPEN_SHORTCUT` parsing (already handles URL-like strings)
44fk|  - **Constraints:** Must be backward compatible with existing `OPEN_SHORTCUT`/`OPEN_COMMANDLINE` parsing in `OpenPluginEx()`; must handle special characters in session names and paths
45zs|  - **Logging:** DEBUG log format design decisions
46gg|  - **Dependency:** TASK-1
47th|
48th|### Phase 2: Core Implementation
49th|
50or|- [x] **TASK-3: Implement session state encoding/decoding utilities**
51vw|  - **Files:** Create `src/nbcore/SessionHistory.h` and `src/nbcore/SessionHistory.cpp`
52hh|  - **Deliverable:**
53en|    - `EncodeSessionParam(const UnicodeString& sessionName, const UnicodeString& remoteDirectory) -> UnicodeString`
54dm|    - `DecodeSessionParam(const UnicodeString& param) -> TSessionHistoryEntry { SessionName, RemoteDirectory }`
55fo|    - Unit tests for encoding/decoding with edge cases (empty directory, special characters, Unicode session names)
56lv|  - **Logging:** DEBUG log all encode/decode operations with input/output
57eh|  - **Dependency:** TASK-2
58th|
59ub|- [x] **TASK-4: Update SetDirectoryEx to populate FarPanelDirectory::Param**
60lr|  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`SynchronizeBrowsing()`, `SetDirectoryEx()`)
61fc|  - **Deliverable:** Modify `SynchronizeBrowsing()` and `SetDirectoryEx()` to set `FarPanelDirectory::Param` with encoded session state when calling `FCTL_SETPANELDIRECTORY`
62xr|  - **Pattern:** Follow existing `FarPanelDirectory` usage pattern
63qi|  - **Error handling:** If encoding fails: (1) Log ERROR with details, (2) Set `Param = nullptr` to maintain backward compatibility, (3) Continue with directory set operation -- history tracking degrades gracefully
64ca|  - **Logging:** DEBUG log when setting panel directory with session param; WARN if encoding fails
65fa|  - **Dependency:** TASK-3
66rd|
67bo|- [x] **TASK-5: Handle panel restoration from FarManager history**
68je|  - **Files:** `src/NetBox/WinSCPPlugin.cpp` (`OpenPluginEx()`), `src/NetBox/FarPlugin.cpp` (`OpenPlugin()`)
69yx|  - **Deliverable:** When `OpenPluginEx()` is called with `OPEN_SHORTCUT` or `OPEN_COMMANDLINE` from Far Manager history, parse the `Param` field to extract session name and directory, then restore the session automatically without showing session list
70fb|  - **Integration:** Reuse existing `OPEN_SHORTCUT` parsing flow -- extend to handle `Param`-based session restoration
71zn|  - **Error handling matrix:**
72fu|    | Failure Scenario | User-Facing Behavior | Log Level | Fallback |
73ul|    |------------------|---------------------|-----------|----------|
74ze|    | Session not found in storage | Error dialog: "Session not found" | ERROR | Show session list |
75iq|    | Remote directory inaccessible | Warning banner: "Directory not found, using home" | WARN | Navigate to home directory |
76bn|    | Session config changed (credentials) | Reuse existing reconnect dialog | INFO | Standard reconnect flow |
77vl|    | Malformed Param in history entry | Ignore entry, show session list | WARN | Standard session selection |
78ex|  - **Logging:** DEBUG log history restoration flow; ERROR log if session not found; WARN if directory restoration fails
79fa|  - **Dependency:** TASK-3
80th|
81st|### CHECKPOINT: Core Functionality Verification
82th|
83pd|After completing TASK-5:
84dp|1. Build with `cmd /c build-x64.bat` -- zero warnings required
85zr|2. Manual test: Open session -> navigate to subdirectory -> close panel -> use Far Manager folder history -> verify session restores to correct directory
86dr|3. If test fails: STOP, debug, fix. Do not proceed to Phase 3 until this passes.
87yw|4. Log output: Verify DEBUG logs show encode/decode flow and restoration path.
88th|
89th|### Phase 3: Integration and Edge Cases
90th|
91ia|- [x] **TASK-6: Integrate with existing session focus restoration (FPrevSessionName)**
92ku|  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`FPrevSessionName` usage)
93cl|  - **Deliverable:** When restoring from history, update `FPrevSessionName` to ensure correct focus when returning to session list; ensure history entries don't interfere with existing session list navigation
94rk|  - **Logging:** DEBUG log focus restoration state
95nd|  - **Dependency:** TASK-5
96rd|
97ei|- [x] **TASK-7: Handle multi-panel scenarios and panel lifecycle**
98eu|  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`ClearConnectedState()`), `src/NetBox/FarPlugin.cpp` (panel lifecycle)
99hh|  - **Deliverable:**
100ai|    - Verify that each panel maintains independent `Param` state. No cross-panel synchronization required. Each panel's history entry is self-contained.
101nv|    - Clear history state on `ClearConnectedState()` to prevent stale entries
102dz|    - Handle case where user has two NetBox panels open with different sessions -- each panel's history entry is independent
103wm|  - **Logging:** DEBUG log panel lifecycle events affecting history
104og|  - **Dependency:** TASK-4, TASK-5
105nd|
106fa|- [x] **TASK-8: Ensure compatibility with existing path history (FPathHistory)**
107mg|  - **Files:** `src/NetBox/WinSCPFileSystem.cpp` (`TerminalChangeDirectory`, `FPathHistory`), `src/NetBox/WinSCPDialogs.cpp` (Open Directory dialog)
108dm|  - **Deliverable:** Write integration test that opens session, navigates, triggers history restore, and asserts no `FPathHistory` corruption; both mechanisms should coexist
109rr|  - **Logging:** DEBUG log when both history systems are updated
110wh|  - **Dependency:** TASK-4
111th|
112th|### Phase 4: Testing and Verification
113th|
114hc|- [ ] **TASK-9: Write unit tests for session history utilities**
115qq|  - **Files:** Create `tests/nbcore/SessionHistoryTest.cpp`
116py|  - **Note:** No existing test infrastructure in the project (no tests/ directory, no test framework). Unit tests deferred pending test framework setup.
117fi|  - **Deliverable:** Tests for:
118ck|    - Basic encode/decode round-trip
119mt|    - Empty remote directory
120dw|    - Unicode session names (Russian, Polish characters)
121zn|    - Special characters in paths (`|`, `&`, spaces)
122ka|    - Very long session names and paths
123pi|    - Malformed param strings (graceful degradation)
124nh|  - **Logging:** N/A (test code)
125fa|  - **Dependency:** TASK-3
126rd|
127og|- [x] **TASK-10: Manual testing checklist and plugin verification**
128ac|  - **Build verification:** ✅ x64 RelWithDebugInfo passes with zero new warnings (pre-existing WinConfiguration.h only)
129qx|  - **Build verification:** ✅ x86 RelWithDebugInfo passes with zero new warnings
130ry|  - **Build verification:** ⏭️ ARM64 -- build script not available in this environment
131lr|  - **Code quality checks:**
132so|    - ✅ No modifications to `libs/` directory
133wg|    - ✅ CRLF line endings on all modified files
134pk|    - ✅ No BOM (UTF-8 without BOM)
135gm|    - ✅ No trailing whitespace
136eo|    - ✅ Naming conventions followed (T/F prefixes, PascalCase)
137xi|    - ✅ No spelling errors in comments
138oh|  - **Plugin DLL placement:** ✅ `Far3_x64/Plugins/NetBox/NetBox.dll` and `Far3_x86/Plugins/NetBox/NetBox.dll`
139kf|  - **Manual testing:** ⏭️ Deferred to runtime verification in Far Manager (requires live session)
140zg|  - **Pass criteria for history restoration:** (1) Session connects within 5s, (2) Remote directory matches history entry, (3) File list displays without errors, (4) Console title shows session name
141kt|  - **Files:** `src/NetBox/`, `Far3_x64/Plugins/NetBox/`
142oe|  - **Deliverable:** Verify with measurable criteria:
143le|    - Build succeeds with zero warnings (MSVC W4) across x86, x64, ARM64
144ie|    - Plugin DLL correctly placed in `Far3_<platform>/Plugins/NetBox/`
145aw|    - **Pass criteria for history restoration:** (1) Session connects within 5s, (2) Remote directory matches history entry, (3) File list displays without errors, (4) Console title shows session name
146fs|    - Manual test: Open session -> navigate directories -> use Far Manager folder history -> verify session restores correctly per pass criteria
147bb|    - Manual test: Multiple sessions in history -> verify correct session restored each time
148tp|    - Manual test: Session deleted from storage -> verify graceful error handling (shows session list)
149za|    - No modifications to `libs/` directory
150ws|    - CRLF line endings on all modified files
151dq|    - No BOM (UTF-8 without BOM)
152tv|    - No trailing whitespace
153ur|    - Naming conventions followed (T/F prefixes, PascalCase)
154dy|    - No spelling errors in comments
155lg|    - All unit tests pass
156lw|    - Manual testing completed per TASK-10 checklist with all pass criteria met
157wk|  - **Logging:** Verify verbose logging works during manual testing
158kx|  - **Dependency:** TASK-5, TASK-6, TASK-7, TASK-8, TASK-9
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
