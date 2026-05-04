# WinSCP vs NetBox Feature Gap Analysis

> Exploration date: 2026-05-04  
> WinSCP source: `D:\Projects\WinSCP-work\winscp-master\source`  
> NetBox source: `D:\Projects\NetBox\NetBox-other\src`  
> Scope: Identify missing WinSCP features in NetBox for Far Manager plugin context

---

## Executive Summary

NetBox is far more feature-complete than commonly assumed. The vast majority of WinSCP's core file-transfer, synchronization, queue, editor, bookmark, and session-management functionality is already implemented. The genuine gaps are in secondary productivity features, security hardening, and advanced UX refinements that require Far Manager text-mode adaptations.

---

## Already Implemented (No Action Needed)

These major WinSCP features are fully present in NetBox:

| Feature | NetBox Evidence | Notes |
|---------|----------------|-------|
| **Synchronization** | `TFullSynchronizeDialog`, `TSynchronizeChecklistDialog`, `SynchronizeCollect`, `SynchronizeApply` in `src/NetBox/WinSCPDialogs.cpp` and `src/core/Terminal.cpp` | All modes: remote, local, both. Checklist preview with per-item action selection |
| **Background Queue** | `TQueueDialog`, `TTerminalQueue`, `TQueueItemProxy` in `src/core/Queue.cpp` and `src/NetBox/WinSCPDialogs.cpp` | Pause/resume/execute/delete/move. Parallel transfers via `TParallelOperation`. Auto-popup on user action |
| **Custom Commands** | `ApplyCommandDialog`, `TFarInteractiveCustomCommand`, `CustomCommandOnFiles` in `src/NetBox/WinSCPDialogs.cpp` | Remote and local command execution with parameter substitution |
| **Editor Integration** | `TTransferEditorConfigurationDialog`, `ProcessEditorEvent`, `UploadFromEditor` in `src/NetBox/WinSCPFileSystem.cpp` | Upload-on-save, multi-edit tracking, external modification detection |
| **File Find** | `FilesFind`, `DoFilesFind`, `TFilesFindParams` in `src/core/Terminal.cpp` | Recursive remote file search with pattern matching |
| **Bookmarks** | `TBookmarks`, `TBookmarkList`, `OpenDirectoryDialog` in `src/core/Bookmarks.cpp` and `src/NetBox/WinSCPFileSystem.cpp` | Per-session and shared bookmarks with directory+shortcut support |
| **Session Import/Export** | `ImportSessions`, `ExportSession` in `src/NetBox/WinSCPFileSystem.cpp` | OpenSSH config, FileZilla sitemanager.xml, PuTTY registry, INI files |
| **Remote-to-Remote Copy** | Protocol-specific implementations in `src/core/` | SFTP (extensions), WebDAV (HTTP COPY), S3 (CopyObject), SCP (`cp -r`) |
| **Parallel Transfers** | `TParallelTransferQueueItem`, `TParallelOperation` in `src/core/Queue.cpp` | Multi-connection file splitting for faster transfers |
| **Keep-Alive** | `TKeepAliveThread` in `src/NetBox/WinSCPFileSystem.cpp` | Configurable ping interval with NOOP/echo |

---

## Genuine Gaps (Actionable)

The following WinSCP features are missing or incomplete in NetBox:

| # | Feature | WinSCP Source | Gap Severity | Far Feasibility | Implementation Notes |
|---|---------|--------------|--------------|----------------|---------------------|
| 1 | **Generate URL / Code** | `forms/GenerateUrl.cpp` | Medium | High | Generate connection URLs, .NET assembly code, PowerShell scripts, batch files from session data. Straightforward dialog adaptation |
| 2 | **Workspace Save/Restore** | `windows/TerminalManager.cpp:SaveWorkspace` | Medium | Medium | Save/restore open sessions and their directories across Far restarts. Serialization exists (`SaveWorkspaceData` stubbed) |
| 3 | **SSH Key Generation UI** | `core/KeyGen.cpp`, `windows/Tools.cpp` | Low | High | Wrapper dialog to launch `puttygen.exe` with session parameters. Button on Authentication tab |
| 4 | **Directory Comparison** | `forms/SynchronizeChecklist.cpp` (visual diff) | Medium | Medium | Side-by-side panel comparison using Far's panel highlighting (color/symbol marks) |
| 5 | **Copy Parameter Presets** | `forms/CopyParamPreset.cpp` | Low | High | Named transfer presets ("ASCII text", "Binary all", "No preserve time") |
| 6 | **Location Profiles** | `forms/LocationProfiles.cpp` | Low | Medium | Saved local+remote directory pairs per session for quick switching |
| 7 | **Configuration Cleanup** | `forms/Cleanup.cpp` | Low | High | Dialog to clean orphaned session configs, temp files, cached host keys |
| 8 | **Advanced Session Info** | `forms/FileSystemInfo.cpp` | Low | High | Display remote filesystem capabilities (protocol version, extensions, charset, server software) |
| 9 | **Master Password** | `core/Security.cpp`, `windows/WinConfiguration.cpp` | Medium | Low | Encrypt stored passwords with user-provided master password. Requires security audit |
| 10 | **Dialog UX Refinements** | `forms/SiteAdvanced.cpp`, `forms/Login.cpp` | Medium | High | Tab ordering, control alignment, enablement logic per WinSCP patterns. See [winscp-dialog-alignment](../plans/winscp-dialog-alignment.md) |
| 11 | **WinSCP Upstream Bug Fixes** | `core/Terminal.cpp`, `core/SecureShell.cpp` | High | High | Port fixes from WinSCP 6.5.6 not yet in NetBox (timestamp, retry, cancellation, auth flow) |
| 12 | **Background Synchronize** | `core/Terminal.cpp` (continuous sync) | Medium | Medium | "Keep Local Directory Up to Date" mode using `TSynchronizeController` with periodic scan |

---

## Out of Scope (GUI-Only)

These WinSCP features cannot be replicated in a Far Manager text-mode plugin:

- Explorer vs Commander interface modes
- Drag-and-drop support
- Rich progress dialogs with speed graphs (NetBox correctly uses Far's progress + `TQueueDialog`)
- Built-in text editor with syntax highlighting (NetBox correctly uses Far's editor)
- Windows shell integration / jump lists / SendTo shortcuts
- Visual glyphs, animation resources, VCL visual components
- Console window (`TConsole`) — Far itself is the console
- Tree view navigation panel
- Color scheme / visual theme customization

---

## WinSCP Source Structure Mapping

For reference when porting features:

```
WinSCP source/                          NetBox equivalent
----------------                        ------------------
core/Terminal.cpp                       src/core/Terminal.cpp
core/SecureShell.cpp                    src/core/SecureShell.cpp
core/SessionData.cpp                    src/core/SessionData.cpp
core/Queue.cpp                          src/core/Queue.cpp
core/Script.cpp                         NOT IMPLEMENTED (deferred)
core/KeyGen.cpp                         NOT IMPLEMENTED (wrapper only)
forms/SiteAdvanced.cpp                  src/NetBox/WinSCPDialogs.cpp
forms/Login.cpp                         src/NetBox/WinSCPDialogs.cpp
forms/GenerateUrl.cpp                   NOT IMPLEMENTED
forms/Cleanup.cpp                       NOT IMPLEMENTED
forms/FileSystemInfo.cpp                NOT IMPLEMENTED
forms/LocationProfiles.cpp              NOT IMPLEMENTED
forms/CopyParamPreset.cpp               NOT IMPLEMENTED
forms/SynchronizeChecklist.cpp          src/NetBox/WinSCPDialogs.cpp
windows/TerminalManager.cpp             src/NetBox/WinSCPFileSystem.cpp
windows/Tools.cpp                       src/windows/GUITools.cpp
windows/WinConfiguration.cpp            src/windows/WinConfiguration.cpp
```

---

## Methodology

1. **WinSCP feature catalog** — Scanned `core/Terminal.cpp`, `forms/*.cpp`, `windows/*.cpp` for major functionality classes
2. **NetBox code search** — Used structural grep for `Synchronize`, `Queue`, `CustomCommand`, `Editor`, `FileFind`, `Bookmark`, `ImportSession`, `ExportSession`, `ParallelOperation`, `GenerateUrl`, `Workspace`, `Cleanup`, `KeyGen`, `Script`
3. **Cross-reference** — Verified presence/absence by checking class definitions, dialog GUIDs, menu items, and implementation completeness
4. **Far feasibility filter** — Eliminated GUI-only features (drag-drop, explorer mode, visual themes)
5. **Severity scoring** — Based on user value, implementation complexity, and risk

---

## Related Plans

- [WinSCP Feature Alignment Master Roadmap](../plans/winscp-feature-alignment-roadmap.md) — Implementation plan for all 12 gaps
- [WinSCP Dialog Alignment](../plans/winscp-dialog-alignment.md) — Detailed S3/TLS tab control fixes
- [S3 Encryption Options](../plans/s3-session-token-role-arn-editor.md) — S3-specific dialog and encryption alignment
