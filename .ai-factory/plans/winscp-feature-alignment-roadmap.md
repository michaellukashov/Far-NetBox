# Implementation Plan: WinSCP Feature Alignment Master Roadmap

**Branch:** feature/winscp-alignment-roadmap  
**Created:** 2026-05-04  
**Plan Type:** full (master roadmap with phased milestones)  
**Scope:** Align NetBox functionality with WinSCP, prioritizing feasible Far-plugin adaptations

---

## Settings

- **Testing:** yes — test plans included per milestone
- **Logging:** verbose — detailed DEBUG logs for all new features
- **Docs:** yes — mandatory docs checkpoint at each milestone completion
- **Roadmap Linkage:** Integrated with existing Version 1.2 / 2.0 milestones

---

## Research Context

**Source:** Parallel codebase analysis of WinSCP 6.5.6 (`D:\Projects\WinSCP-work\winscp-master\source`) and NetBox current state.

> **Full exploration record:** See [winscp-netbox-gap-analysis](../references/winscp-netbox-gap-analysis.md) for detailed methodology, source structure mapping, and feasibility assessment.
### Critical Discovery: NetBox Core Is Largely Complete

Contrary to initial assumptions, NetBox has already ported most of WinSCP's core file-transfer functionality from the WinSCP/PuTTY/FileZilla codebases. The following major WinSCP features are **already fully implemented** in NetBox:

| Feature | NetBox Status | Evidence |
|---------|--------------|----------|
| Synchronization (collect, checklist, apply, mirror) | **COMPLETE** | `TFullSynchronizeDialog`, `TSynchronizeChecklistDialog`, `SynchronizeCollect`, `SynchronizeApply` |
| Background transfer queue | **COMPLETE** | `TQueueDialog`, `TTerminalQueue`, `TQueueItemProxy`, pause/resume/execute/delete |
| Custom commands (remote/local) | **COMPLETE** | `ApplyCommandDialog`, `TFarInteractiveCustomCommand`, `CustomCommandOnFiles` |
| Editor integration (upload-on-save, multi-edit) | **COMPLETE** | `TTransferEditorConfigurationDialog`, `ProcessEditorEvent`, `UploadFromEditor` |
| File find (remote search) | **COMPLETE** | `FilesFind`, `DoFilesFind`, `TFilesFindParams` |
| Bookmarks (per-session + shared) | **COMPLETE** | `TBookmarks`, `TBookmarkList`, `OpenDirectoryDialog` |
| Session import/export | **COMPLETE** | `ImportSessions`, `ExportSession`, OpenSSH/FZ/PuTTY import |
| Remote-to-remote copy | **COMPLETE** | All protocols (SFTP/WebDAV/S3/SCP) per ROADMAP.md v1.1 |
| Parallel transfers | **COMPLETE** | `TParallelOperation`, `TParallelTransferQueueItem` |
| Keep-alive thread | **COMPLETE** | `TKeepAliveThread` in `WinSCPFileSystem.cpp` |

### Genuine Gaps Identified

The following WinSCP features are **missing or incomplete** in NetBox and represent real alignment opportunities:

| # | Feature | WinSCP Source | Gap Severity | Far Feasibility | Notes |
|---|---------|--------------|--------------|-----------------|-------|
| 1 | **Generate URL / Code** | `forms/GenerateUrl.cpp` | Medium | High | Generate connection URLs, scripts, .NET assembly code from session data |
| 2 | **Workspace Save/Restore** | `windows/TerminalManager.cpp:SaveWorkspace` | Medium | Medium | Save/restore open sessions and their state across Far restarts |
| 3 | **Scripting Engine** | `core/Script.cpp` | High | Low | Batch command execution — fundamentally GUI-oriented; **deferred to v2.0** |
| 4 | **SSH Key Generation UI** | `core/KeyGen.cpp`, `windows/Tools.cpp` | Low | High | Wrapper for puttygen.exe with dialog integration |
| 5 | **Directory Comparison** | `forms/SynchronizeChecklist.cpp` (visual diff) | Medium | Medium | Side-by-side panel comparison with color highlighting |
| 6 | **Copy Parameter Presets** | `forms/CopyParamPreset.cpp` | Low | High | Named transfer setting presets (e.g., "ASCII text", "Binary all") |
| 7 | **Location Profiles** | `forms/LocationProfiles.cpp` | Low | Medium | Saved local+remote directory pairs per session |
| 8 | **Master Password** | `core/Security.cpp`, `windows/WinConfiguration.cpp` | Medium | Low | Encrypt stored passwords with master password — requires security audit |
| 9 | **Configuration Cleanup** | `forms/Cleanup.cpp` | Low | High | Dialog to clean registry/XML config leftovers |
| 10 | **Advanced Session Info** | `forms/FileSystemInfo.cpp` | Low | High | Display remote filesystem capabilities dialog |
| 11 | **Dialog UX Refinements** | `forms/SiteAdvanced.cpp`, `forms/Login.cpp` | Medium | High | Tab ordering, control alignment, enablement logic per WinSCP patterns |
| 12 | **WinSCP Bug Fixes Port** | `core/Terminal.cpp`, `core/SecureShell.cpp` | High | High | Port upstream WinSCP fixes not yet in NetBox |

### WinSCP Features Intentionally Out of Scope

These are GUI-specific and cannot be replicated in a Far Manager text-mode plugin:

- Explorer vs Commander interface modes
- Drag-and-drop support
- Rich progress dialogs with speed graphs (NetBox uses Far's progress + `TQueueDialog`)
- Built-in text editor with syntax highlighting (NetBox uses Far's editor — this is correct)
- Windows shell integration / jump lists / SendTo shortcuts
- Visual glyphs and animation resources
- Console window (`TConsole`) — Far itself is the console

---

## Roadmap Linkage

**Milestone: Version 1.2 — Background Copy & Progress UI**
- Status: Effectively COMPLETE (`TQueueDialog` provides full queue UI)
- Remaining: Minor polish items (queue auto-popup, beep on completion)

**Milestone: Version 1.3 — Win32/KiTTY Input Mode, WinXP Compatible Builds**
- Status: In progress (see existing `win32-kitty-input-mode` plan)
- Not covered by this roadmap — orthogonal concern

**Milestone: Version 2.0 — Full Plugin Refactor, Modular Architecture**
- This roadmap feeds into v2.0 by identifying stable features and porting targets
- Scripting engine (gap #3) is deferred to v2.0 as it requires architectural changes

---

## Phased Implementation Plan

### Phase 1: Dialog UX & Upstream Fix Port (Foundation)

**Goal:** Complete dialog UX alignment with WinSCP patterns and proactively port upstream bug fixes. Previously blocking crash bugs (#497 stack overflow, #391 DST, #392 auth, #508 second-file-open) are **already fixed** — see RESEARCH.md audit session 2026-05-04. Phase 1 is now low-risk UX polish + proactive maintenance, not urgent fire-fighting.

**Affected files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPFileSystem.cpp`, `src/core/Terminal.cpp`, `src/core/SecureShell.cpp`

- [ ] **Task 1.1: Proactively port WinSCP upstream bug fixes**
  - Audit WinSCP 6.5.6 `core/Terminal.cpp` and `core/SecureShell.cpp` for fixes not yet in NetBox
  - Focus on: transfer retry logic, edge-case auth flows, protocol handshake robustness
  - Cross-reference with NetBox's existing fix history; skip already-ported fixes (#391, #392, #511, etc.)
  - Files: `src/core/Terminal.cpp`, `src/core/SecureShell.cpp`
  - LOG: `FTerminal->LogEvent(L"WinSCP upstream fix port: <description>")`
  - Blocked by: none

- [ ] **Task 1.2: Complete dialog control alignment**
  - Finish `winscp-dialog-alignment.md` plan items (S3 tab, TLS tab, auth enablement)
  - Ensure all `UpdateControls()` methods match WinSCP enablement logic
  - Fix any remaining duplicate control leaks (`MakeOwnedObject` overwritten)
  - Files: `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`
  - LOG: `FTerminal->LogEvent(L"Dialog: control <name> enabled=%d")`
  - Blocked by: none

- [ ] **Task 1.3: Copy parameter presets**
  - Add `TCopyParamPreset` storage and UI for named transfer presets
  - Presets: "Default", "Text files (ASCII)", "Binary all", "No preserve time"
  - Files: `src/NetBox/WinSCPDialogs.cpp` (preset selection combo), `src/core/CopyParam.cpp` (preset storage)
  - LOG: `FTerminal->LogEvent(L"Copy param preset loaded: %s", PresetName)`
  - Blocked by: none

- [ ] **Task 1.4: Build verification & regression test**
  - Run `build-x64.bat` with zero warnings
  - Manual test: connect SFTP/FTP/WebDAV/S3, transfer, sync, queue, custom command
  - Files: all modified files
  - Blocked by: Tasks 1.1–1.3

**Commit checkpoint:**
```
feat(ui): align dialogs with WinSCP and port upstream fixes

- Port WinSCP 6.5.6 bug fixes for Terminal/SecureShell
- Complete S3/TLS tab control alignment
- Add copy parameter presets (Default, Text, Binary, NoPreserve)
```

---

### Phase 2: Structural UX Parity (Generate URL, KeyGen, Cleanup, LocationProfiles, CopyParamCustom)

**Goal:** Achieve complete structural UX parity with WinSCP — all dialogs that exist in WinSCP must exist in NetBox. Detailed plan: `.ai-factory/plans/winscp-structural-ux-parity.md`

**Affected files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPPlugin.cpp`, `src/core/SessionData.cpp`, `src/base/MsgIDs.h`, `.lng` files

- [x] **Task 2.1: Generate URL / Script dialog**
  - Two-tab dialog: URL (checkboxes for user/pass/hostkey/remote-dir) + Script (batch/PS/cmd-line format)
  - Omit .NET Assembly tab (NetBox has no .NET assembly)
  - Use `netbox.com` not `winscp.com` in script output
  - Core `TSessionData::GenerateSessionUrl()` already exists
  - Files: `src/NetBox/WinSCPDialogs.cpp` (new `TGenerateUrlDialog`), `src/NetBox/WinSCPPlugin.cpp` (menu wiring)
  - LOG: `AppLogFmt(L"GenerateUrl: ...")`

- [x] **Task 2.2: SSH key generation button**
  - Add "Generate Key" TFarButton on Authentication tab next to PrivateKeyBrowseBtn
  - Launch puttygen.exe when PuttygenPath is configured and file exists
  - Files: `src/NetBox/WinSCPDialogs.cpp` (button + handler)
  - LOG: `AppLogFmt(L"GenerateKey: ...")`

- [x] **Task 2.3: Configuration cleanup dialog**
  - TFarListBox with checkmark items for cleanup categories (config, sessions, caches, INI, seed)
  - Temporary folders skipped (VCL-only method)
  - Files: `src/NetBox/WinSCPDialogs.cpp` (new `TCleanupDialog`), `src/NetBox/WinSCPPlugin.cpp` (menu)

- [x] **Task 2.4: Location profiles dialog**
  - TTabbedDialog with Session/Shared tabs, TFarListBox for bookmarks
  - CRUD: add/remove/rename bookmarks (local+remote directory pairs)
  - Reuses existing `TBookmarkList`/`TBookmarks` data model
  - Files: `src/NetBox/WinSCPDialogs.cpp` (new `TLocationProfilesDialog`), `src/NetBox/WinSCPPlugin.cpp` (menu)

- [x] **Task 2.5: CopyParamCustom — custom copy param option**
  - Added "Custom" option to copy dialog preset combo
  - Selecting "Custom" opens the existing copy-param editor dialog
  - Files: `src/NetBox/WinSCPDialogs.cpp` (combo option + handler)

- [x] ~~Task 2.4 (old): Advanced session info dialog~~ — **Already exists** as `TFileSystemInfoDialog` in WinSCPDialogs.cpp.

**Commit checkpoint:**
```
feat(ui): achieve WinSCP structural UX parity

- Generate URL/Script dialog (URL + batch/PS/cmd-line output)
- Puttygen launcher button on auth tab
- Configuration cleanup dialog for orphaned data
- Location profiles dialog with CRUD for session/shared bookmarks
- Per-session custom copy parameter override
-```
---

### Phase 3: Workspace & Session Management

**Goal:** Implement workspace save/restore and location profiles for session state persistence.

**Affected files:** `src/NetBox/WinSCPFileSystem.cpp`, `src/core/SessionData.cpp`, `src/NetBox/FarConfiguration.cpp`

- [ ] **Task 3.1: Workspace save/restore infrastructure**
  - Add `TStoredSessionList::SaveWorkspaceData()` and `LoadWorkspaceData()` serialization
  - Store: open sessions, current directories, panel layouts
  - Files: `src/core/SessionData.cpp`, `src/core/SessionData.h`
  - LOG: `FTerminal->LogEvent(L"Workspace: saved %d sessions", Count)`
  - Blocked by: Phase 2 completion

- [ ] **Task 3.2: Workspace UI in plugin menu**
  - Add "Save Workspace" and "Restore Workspace" menu items
  - Dialog: workspace name input, list of saved workspaces
  - Auto-save on Far exit / restore on Far start (optional config)
  - Files: `src/NetBox/WinSCPPlugin.cpp` (menu), `src/NetBox/WinSCPFileSystem.cpp` (workspace operations)
  - LOG: `FTerminal->LogEvent(L"Workspace: restored '%s' with %d sessions", Name, Count)`
  - Blocked by: Task 3.1

- [ ] **Task 3.3: Location profiles**
  - Add saved local+remote directory pairs per session
  - Quick-switch menu in plugin commands (Alt-Shift-F12 extension)
  - Files: `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPDialogs.cpp` (profile dialog)
  - LOG: `FTerminal->LogEvent(L"Location profile: switched to local=%s remote=%s", ...)`
  - Blocked by: Task 3.2

**Commit checkpoint:**
```
feat(session): workspace save/restore and location profiles

- Serialize/deserialize open session state
- Workspace menu with save/restore/auto-save
- Per-session location profiles for quick directory switching
```

---

### Phase 4: Directory Comparison & Advanced Sync

**Goal:** Add directory comparison UI and enhance synchronization with visual feedback.

**Affected files:** `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPDialogs.cpp`, `src/core/Terminal.cpp`

- [ ] **Task 4.1: Directory comparison mode**
  - Add "Compare Directories" command (Ctrl-F12 or menu)
  - Mark differing files in Far panel with color/symbol (using Far's panel highlighting)
  - Criteria: size, timestamp, or both
  - Files: `src/NetBox/WinSCPFileSystem.cpp` (comparison logic), `src/NetBox/WinSCPDialogs.cpp` (criteria dialog)
  - LOG: `FTerminal->LogEvent(L"Compare: %d files differ by timestamp", Count)`
  - Blocked by: Phase 3 completion

- [ ] **Task 4.2: Synchronize progress dialog enhancement**
  - Enhance existing `TSynchronizeDialog` with real-time statistics
  - Show: files scanned, files to transfer, bytes transferred, ETA
  - Files: `src/NetBox/WinSCPDialogs.cpp` (`TSynchronizeDialog` improvements)
  - LOG: `FTerminal->LogEvent(L"Sync progress: %d/%d files, %s bytes", ...)`
  - Blocked by: Task 4.1

- [ ] **Task 4.3: Background synchronize (keep local dir up to date)**
  - Add "Keep Local Directory Up to Date" mode (continuous sync)
  - Use `TSynchronizeController` with periodic scan + auto-apply
  - Files: `src/NetBox/WinSCPFileSystem.cpp` (controller wiring), `src/core/Terminal.cpp` (periodic sync)
  - LOG: `FTerminal->LogEvent(L"Auto-sync: scanning local dir '%s'", LocalDir)`
  - Blocked by: Task 4.2

**Commit checkpoint:**
```
feat(sync): directory comparison and background synchronization

- Panel-level directory comparison with Far highlighting
- Real-time sync progress statistics
- Continuous "keep local dir up to date" mode
```

---

### Phase 5: Security & Configuration Hardening

**Goal:** Implement master password and security-related features. **This phase requires security audit before release.**

**Affected files:** `src/core/Security.cpp`, `src/windows/WinConfiguration.cpp`, `src/NetBox/WinSCPDialogs.cpp`

- [ ] **Task 5.1: Master password infrastructure**
  - Add `TWinSCPPlugin` master password prompt and storage
  - Encrypt sensitive session data (passwords, passphrases) with AES-256-GCM
  - Use Windows DPAPI or key derivation (PBKDF2) for key protection
  - Files: `src/core/Security.cpp` (encryption helpers), `src/NetBox/FarConfiguration.cpp` (storage)
  - LOG: `FTerminal->LogEvent(L"Security: master password set/verified")`
  - Blocked by: Phase 4 completion

- [ ] **Task 5.2: Master password UI**
  - Add "Set Master Password" to plugin configuration menu
  - Prompt on first access to encrypted data, lock/unlock workflow
  - Files: `src/NetBox/WinSCPDialogs.cpp` (password dialog), `src/NetBox/WinSCPPlugin.cpp` (menu)
  - LOG: `FTerminal->LogEvent(L"Security: master password dialog result=%d", Result)`
  - Blocked by: Task 5.1

- [ ] **Task 5.3: Security audit & hardening**
  - Audit all password/passphrase handling for plaintext leaks
  - Ensure encrypted values are not logged (mask in `LogEvent`)
  - Review `src/core/SecureShell.cpp` key handling for side-channel resistance
  - Files: all security-related files
  - Blocked by: Task 5.2

**Commit checkpoint:**
```
feat(security): master password and encrypted configuration storage

- AES-256-GCM encryption for stored credentials
- Master password dialog with lock/unlock workflow
- Security audit: mask secrets in logs, hardened key handling
```

---

### Phase 6: Scripting Engine (Deferred to v2.0)

**Goal:** Batch command execution for automation. **Explicitly deferred** because it requires architectural changes (command interpreter, batch file parser) that conflict with v1.x stability goals.

**Rationale:**
- WinSCP scripting is heavily tied to its GUI (`TScript` interacts with `TTerminal` via GUI callbacks)
- Far Manager already provides macro/scripting capabilities
- NetBox's `CustomCommand` feature covers most automation use cases
- Implementing a full scripting engine would introduce significant complexity and risk

**When to revisit:** As part of Version 2.0 modular architecture refactor.

---

## Dependencies

```
Phase 1 ──→ Phase 2 ──→ Phase 3 ──→ Phase 4 ──→ Phase 5
   │           │           │           │           │
   └─ independent tasks within each phase can proceed in parallel
```

- Phase 2 depends on Phase 1 (foundational dialog infrastructure)
- Phase 3 depends on Phase 2 (workspace builds on stable UI)
- Phase 4 depends on Phase 3 (sync enhancements need workspace stability)
- Phase 5 depends on Phase 4 (security is last to minimize code churn during audit)
- Phase 6 is deferred to v2.0, no dependency chain

---

## Success Criteria

Per phase:

| Phase | Success Criteria |
|-------|----------------|
| 1 | Build zero warnings; all dialog tabs match WinSCP enablement; no regressions in connect/transfer/sync/queue |
| 2 | URL generation produces valid WinSCP-compatible URLs; puttygen launches correctly; cleanup removes orphans |
| 3 | Workspace round-trip (save → restart Far → restore) preserves sessions and directories |
| 4 | Directory comparison marks differing files; background sync runs without panel lockups |
| 5 | Master password encrypts/decrypts credentials; no plaintext in logs; security audit passes |

Global:
- [ ] All modified files use CRLF line endings
- [ ] UTF-8 without BOM encoding
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase methods)
- [ ] No modifications to `libs/`
- [ ] Plugin DLL output to `Far3_x64/Plugins/NetBox/`

---

## Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| WinSCP source structure changes in future upstream sync | Medium | Medium | Document all WinSCP-derived code with source references; maintain patch notes |
| Far Manager API limitations block UI feature | Medium | Medium | Use `TFarDialog` extensions; fall back to simpler menu-based UI if needed |
| Master password implementation has crypto flaw | Low | Critical | External security audit; use only Windows DPAPI + standard KDF (PBKDF2) |
| Workspace serialization breaks on session format changes | Medium | Medium | Version workspace format; graceful downgrade on load |
| Unity build conflicts with large dialog additions | Medium | Low | Disable `OPT_USE_UNITY_BUILD` when adding new dialog classes |
| Phase 6 scripting deferred indefinitely | High | Low | Document in ROADMAP.md v2.0; point users to Far macros for automation |

---

## Testing Strategy

### Per-Milestone Manual Test Protocol

1. **Build verification:** `build-x64.bat` → zero warnings → DLL in `Far3_x64/Plugins/NetBox/`
2. **Protocol smoke test:** Connect SFTP → FTP → WebDAV → S3 (one file each)
3. **Feature-specific test:** Exercise all new UI paths, error paths, cancel paths
4. **Regression test:** Synchronize, queue, custom command, editor, bookmarks
5. **48-hour stress test:** No crashes during normal usage

### Automated Testing (where feasible)

- Serialization round-trip tests for workspace and presets (Catch2)
- URL generation validation against known-good outputs
- Encryption/decryption unit tests for master password (mock DPAPI)

---

## Documentation Checkpoints

After each phase:
- [ ] Update `docs/user-guide.md` with new feature descriptions
- [ ] Update `.hlf` help files (EN, RU, PL, FR, ES) with new menu items and dialogs
- [ ] Update `NetBoxEng.lng` / `NetBoxRus.lng` with new message strings (sync `MsgIDs.h`)
- [ ] Add CHANGELOG entry

---

## Changelog

- 2026-05-04: Initial roadmap created based on comprehensive WinSCP vs NetBox gap analysis
- 2026-05-04: Discovered NetBox already has sync, queue, custom commands, editor, bookmarks, import/export — removed from plan
- 2026-05-04: Scoped 12 genuine gaps across 5 phases + deferred scripting to v2.0
- 2026-05-04: Post-creation audit discovered 4 crash/data-integrity bugs (#497, #391, #392, #508) already fixed in commits Apr 26–May 03. Reframed Phase 1 from "urgent bug fixes" to "proactive upstream porting + UX polish". Only silent mode remains genuinely open.
- 2026-05-04: Integrated with existing ROADMAP.md milestones (v1.2 complete, v1.3 orthogonal, v2.0 deferred)
