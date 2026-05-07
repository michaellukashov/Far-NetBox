# Research

Updated: 2026-05-10
Status: active

## Active Summary (input for /aif-plan)

Updated: 2026-05-10

Goal: Fix 4 issues found in WinSCP UX Parity Phase 2 verification

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- WinXP compatibility (_WIN32_WINNT=0x0501)
- Incremental evolution — no architectural rewrites
- C++17 standard only (no std::filesystem, std::variant)

Decisions:
- Location Profiles "Open": store selected bookmark paths in dialog, expose via getters, navigate after Execute()
- Hardcoded English: add 2 new MsgIDs (NB_LOCATION_BOOKMARK_NAME_PROMPT, NB_LOCATION_BOOKMARK_RENAME_PROMPT) and update all 5 .lng files
- Remove confirmation: add MessageDialog before deletion, add 1 new MsgID (NB_LOCATION_PROFILES_REMOVE_CONFIRM)
- Null check: add defensive null guards in TGenerateUrlDialog for FScriptFormatCombo, FTransferModeBtn, and related pointers
- SCP Esc cancellation: Four-layer fix adopted (console buffer scan, csCancel semantics, EAbort("") dispatch, deferred reconnect). Post-cancel session reset via FNeedsSessionReset + FLastDirectory.
- SSH/SCP buffer corruption: Default SendBuf=0, SshSimple=false to disable dynamic TCP send backlog query.
- Multithreading: Far API marshaling via Synchronize/PostMainThreadSynchro; event-driven waits replace Sleep polling.
- CWE-134: EscapeFmtChars() utility for all untrusted args to FMTLOAD.
- S3 TLS alignment: Add S3CACertificate serialization + UI; expose Min/Max TLS version on S3 tab.
- SFTP Log Protocol: Level 3 added for binary dump (separate from Level 2 headers).
- CMake: 97% reduction in main CMakeLists.txt via modular orchestrator pattern.
- OpenSSH certificate auth (WinSCP-aligned): Silent pre-connect conversion inside StoreToConfig(). Effective key file resolved before CONF_keyfile. Passphrase encryption uses effective key file path. Temp PPK cleaned in TSecureShell destructor. No interactive dialogs (Far plugin context).
- Master password infrastructure: Effectively complete. All security-critical gaps (TSecureString, atomic counters, rate limiting, error reporting) are implemented. Active-terminal recryption gap is non-issue in Far plugin context (no TTerminalManager registry, per-panel terminal ownership).
- UX parity scope: "Structural parity" only — same dialogs must exist as in WinSCP. Behavioral parity (keyboard shortcuts, menu structure, workflow sequencing) is out of scope. Interaction differences due to Far text-mode (no drag-drop, no tree view, no MDI tabs) are accepted.
Open questions:
- Silent mode file operations: Error collection mechanism designed but not implemented.
- ~~Stack overflow (#497): Symlink cycle detection needed in CalculateFilesSize (mirrors FilesFind pattern).~~ **FIXED** (commit `2689164e6`, 2026-05-03).
- ~~DST timestamp (#391): Remove erroneous DST subtraction in ConvertTimestampToUnix for dstmWin on Win7+.~~ **FIXED** (commit `17a50dfdc`, 2026-05-02).
- ~~Private key auth (#392): Passphrase prompt misclassification or path encoding issue suspected; needs diagnostic logging.~~ **FIXED** (commit `e41274cd7`, 2026-05-02).
- ~~Second file open crash: Dangling TRemoteFile pointers after directory refresh; Duplicate(false) recommended in CreateFileList.~~ **FIXED** (commits `311e2c8fc` + `8539f9963`, 2026-04-26).
Success signals:
- Zero build warnings under /W4
- Plugin DLL in Far3_<platform>/Plugins/NetBox/
- Manual test protocol: connect, transfer, cancel, navigate for each protocol
- No crashes in 48hr stress test

Next step:
- Implement Phase 2 fixes: 4 issues found in verification (see Session 2026-05-10 below).
- Prioritize remaining open question: silent mode file operations (designed, not implemented).

## Sessions

### 2026-05-10 — WinSCP UX Parity Phase 2 Verification & Planning

What changed:
- Systematic verification of all 5 Phase 2 features (GenerateUrl, Cleanup, LocationProfiles, CopyParamCustom, Generate Key)
- 5 parallel agent investigations + alignment script verification
- Verified all features exist, are wired into plugin menu, and have correct message ID alignment
- Alignment script PASSED: 1309 entries in all 5 .lng files, zero misalignments
- Found 4 issues (1 functional gap, 2 minor, 1 defensive coding)

Key findings:
- All 5 features properly implemented and wired
- Message IDs correctly aligned across all 5 .lng files (verified by verify_lng_alignment.py)
- Location Profiles "Open" button logs paths but doesn't navigate (functional gap)
- Location Profiles has hardcoded English in InputBox prompts (2 instances)
- Location Profiles has no confirmation before removing bookmarks
- TGenerateUrlDialog has unchecked pointer accesses (FScriptFormatCombo, FTransferModeBtn)

Plan for fixes:
- Issue 1: Add FOpenedLocalDir/FOpenedRemoteDir/FOpened members to TLocationProfilesDialog, set in OpenBookmarkClick, expose via getters, navigate in wrapper after Execute() returns true. Key APIs: FTerminal->ChangeDirectory(), FileSystem->UpdatePanel(), FileSystem->RedrawPanel().
- Issue 2: Add 2 new MsgIDs (NB_LOCATION_BOOKMARK_NAME_PROMPT, NB_LOCATION_BOOKMARK_RENAME_PROMPT), update all 5 .lng files, replace hardcoded L-strings with GetMsg().
- Issue 3: Add MessageDialog() confirmation before Bookmarks->Delete(), add 1 new MsgID (NB_LOCATION_PROFILES_REMOVE_CONFIRM), update all 5 .lng files.
- Issue 4: Add null guards at entry of GenerateScript(), UpdateScriptResult(), UpdateUrlResult().

Links (paths):
- src/NetBox/WinSCPDialogs.cpp (dialog implementations, lines 2480-2795)
- src/NetBox/WinSCPFileSystem.cpp (navigation patterns, OpenDirectory)
- src/NetBox/WinSCPPlugin.cpp (menu wiring, dispatch)
- src/base/MsgIDs.h (message IDs)
- src/NetBox/NetBox{Eng,Rus,Fr,Pol,Spa}.lng (language files)

### 2026-05-07 — WinSCP Structural UX Parity Audit

What changed:
- Systematic comparison of WinSCP `forms/*.cpp` (33 forms) against NetBox `src/NetBox/WinSCPDialogs.cpp` (17 dialog classes)
- Identified exact structural parity gap: 4 missing dialogs + 1 missing button
- Scoped UX parity to "structural" only — same dialogs must exist, behavioral parity (shortcuts, menu structure, workflow) is out of scope
- Far text-mode limitations accepted (no drag-drop, tree view, MDI tabs, animations, glyphs)

Key findings:
- NetBox already has 26 of 33 WinSCP form equivalents (79% structural parity)
- 7 forms are GUI-only (Explorer, Console, Animations, Glyphs, NonVisual) — out of scope
- 4 dialogs missing entirely: GenerateUrl, Cleanup, LocationProfiles, CopyParamCustom
- 1 button missing: "Generate Key" on Authentication tab
- TFileSystemInfoDialog already exists (was listed as gap in roadmap but already implemented)
- CopyParamPreset just added in Phase 1 — but lacks per-session custom override (CopyParamCustom)

Structural parity gap (complete list):
- TGenerateUrlDialog: Generate session URL, .NET assembly code, PowerShell script, batch file. Format combo, password checkbox, copy-to-clipboard. WinSCP ref: `forms/GenerateUrl.cpp`. ~200-300 lines.
- TCleanupDialog: CheckboxList of cleanup categories (orphaned sessions, temp files, cached host keys, old logs). Select All / Deselect All. Delete button. WinSCP ref: `forms/Cleanup.cpp`. ~150-200 lines.
- TLocationProfilesDialog: ListBox of saved local+remote directory pairs per session. Add/Edit/Delete/Open buttons. WinSCP ref: `forms/LocationProfiles.cpp`. ~200-250 lines.
- CopyParamCustom page: Per-session override of copy parameter preset. "Custom" option in TCopyDialog that opens full copy param editor for this session. Currently presets are global only. WinSCP ref: `forms/CopyParamCustom.cpp`. ~150-200 lines.
- "Generate Key" button: On Authentication tab. Launches puttygen.exe with session params. Only visible when PuttygenPath is configured. WinSCP ref: `forms/SiteAdvanced.cpp:1681`. ~30-50 lines. **Analyzed in detail below.**
- Total estimate: ~730-1030 lines of new dialog code in WinSCPDialogs.cpp

Decision: Structural parity is sufficient. Behavioral parity (same shortcuts, same menu structure, same workflow sequencing) is out of scope.

#### Deep Dive: "Generate Key" Button

WinSCP has two key-related UI elements:
1. **"Tools" dropdown** next to PrivateKeyEdit on Auth tab (`SiteAdvanced.cpp:1672-1678`): "Generate Key" + "Upload Key" menu items. "Generate" launches `puttygen.exe` via `ExecuteTool(PuttygenTool)` + monitors for new `.ppk` files via `ReadDirectoryChangesW` to auto-fill PrivateKeyEdit. "Upload" installs key on SFTP server (requires active SFTP session).
2. **"Run PuTTYgen" link** on Login dialog (`Login.cpp:116`): simple launch, no monitoring.

NetBox current state:
- ✅ `PuttygenPath` in `TFarConfiguration` (auto-set to `putty\puttygen.exe` relative path)
- ✅ Preferences > Integration tab has PuttygenPath edit (`WinSCPDialogs.cpp:1021-1044`)
- ✅ Commands menu (F11) has "PuTTYgen" item, launches via `ExecuteShellChecked` (`WinSCPPlugin.cpp:714-720`)
- ❌ No button on Authentication tab to generate/upload keys
- ❌ No directory monitoring (VCL-specific, not portable to Far)

Recommended implementation: **Simple "Generate Key" TFarButton** on Auth tab, next to PrivateKeyEdit. On click: `ExecuteShellChecked(PuttygenPath)`. Enabled only when PuttygenPath is configured and file exists. ~30 lines. Directory monitor is VCL-specific overkill that doesn't fit Far text-mode; user can browse for the new key manually via existing PrivateKeyBrowseBtn. "Upload Key" deferred (requires active SFTP connection at session-edit time, complex).

Key source references:
- WinSCP: `forms/SiteAdvanced.cpp:1672-1688` (Tools button + Generate/Upload items)
- WinSCP: `forms/Login.cpp:115-117` (Run Puttygen action)
- WinSCP: `windows/GUITools.cpp:40` (`PuttygenTool = L"puttygen.exe"`)
- NetBox: `src/NetBox/WinSCPDialogs.cpp:1021-1044` (Integration tab, PuttygenPath edit)
- NetBox: `src/NetBox/WinSCPPlugin.cpp:614-720` (Commands menu, PuTTYgen launch)
- NetBox: `src/NetBox/FarConfiguration.cpp:68-69` (PuttygenPath default)
- NetBox: `src/windows/GUITools.h:223` (`PuttygenTool` constant)

Links (paths):
- `.ai-factory/references/winscp-netbox-gap-analysis.md`
- `.ai-factory/plans/winscp-feature-alignment-roadmap.md`
- `src/NetBox/WinSCPDialogs.cpp` (existing dialogs, UpdateControls logic)
- `D:\Projects\WinSCP-work\winscp-master\source\forms\GenerateUrl.cpp`
- `D:\Projects\WinSCP-work\winscp-master\source\forms\Cleanup.cpp`
- `D:\Projects\WinSCP-work\winscp-master\source\forms\LocationProfiles.cpp`
- `D:\Projects\WinSCP-work\winscp-master\source\forms\CopyParamCustom.cpp`
- `D:\Projects\WinSCP-work\winscp-master\source\forms\SiteAdvanced.cpp`

#### Deep Dive: TGenerateUrlDialog

WinSCP source: `forms/GenerateUrl.cpp` (~1020 lines, but 70% is RTF formatting).

Structure: 3-tab dialog (URL / Script / .NET Assembly). Heavily VCL-dependent (TRichEditWithLinks for clickable URLs in output, RTF color-coded code generation). Two entry points:
- `DoGenerateUrlDialog(Data, Paths)` — from session manager, generates URL for session or selected files
- `DoGenerateTransferCodeDialog(ToRemote, Move, CopyParamAttrs, Data, ...)` — from file panel, generates script/assembly for transfer operation

URL tab: checkboxes for user/password/hostkey/remote-directory/raw-settings/WinSCP-specific/save-extension. Calls `TSessionData::GenerateSessionUrl()` with flags. Simple — the core logic already exists in NetBox's `TSessionData`.

Script tab: format combo (ScriptFile/BatchFile/CommandLine/PowerShell). Generates WinSCP scripting-language commands (`open`, `put`/`get`, `exit`). Output is batch/PS/command-line script text.

Assembly tab: language combo (C#/VB.NET/PowerShell). Generates .NET assembly code calling `WinSCP.Session`. Calls `TSessionData::GenerateAssemblyCode()` and `TCopyParamType::GenerateAssemblyCode()`.

Far text-mode adaptation:
- RTF/RichEdit with clickable links: **Not portable**. Use plain `TFarText` output.
- URL generation: Core `TSessionData::GenerateSessionUrl()` already exists — **no new backend needed**.
- Script generation: All logic is string formatting — straightforward to port as plain text. But it generates *WinSCP* scripting language, not NetBox. NetBox's command-line is `netbox.com` not `winscp.com`. Need to adjust exe names and command-line switches.
- .NET assembly code: Generates code referencing `WinSCP.Session` class. **Not applicable to NetBox** — NetBox has no .NET assembly. This tab should be **omitted** or replaced with something NetBox-specific.
- Clipboard copy button: Easy — `FarCopyToClipboard()`.

Implementation estimate: ~250-300 lines for URL tab + Script tab (CommandLine/BatchFile/PowerShell). Omit Assembly tab. The heavy RTF formatting code (500+ lines) is entirely dropped.

#### Deep Dive: TCleanupDialog

WinSCP source: `forms/Cleanup.cpp` (~175 lines). Simplest dialog.

Structure: ListView with checkboxes showing cleanup categories. "Check All" toggle button. OK/Cancel.

Cleanup categories detected at init:
- `CLEANUP_CONFIG` — registry: ConfigurationSubKey → `Configuration->CleanupConfiguration()`
- `CLEANUP_SESSIONS` — registry: StoredSessionsSubKey → `StoredSessions->Cleanup()`
- `CLEANUP_CACHES` — registry: caches → `Configuration->CleanupCaches()`
- `CLEANUP_INIFILE` — file: INI path → `Configuration->CleanupIniFile()`
- `CLEANUP_SEEDFILE` — file: random seed → `Configuration->CleanupRandomSeedFile()`
- `CLEANUP_TEMP_FOLDERS` — directory: temp → `WinConfiguration->CleanupTemporaryFolders()`

Execution: On OK, iterates checked items, calls each `TCleanupEvent` callback, catches exceptions per-item.

Far text-mode adaptation:
- ListView with checkboxes → `TFarListBox` with checkmark items (same pattern as `TOpenDirectoryDialog`).
- NetBox already has `TConfiguration::CleanupConfiguration`, `CleanupCaches`, etc. — all backend exists.
- Registry locations → show as text labels in Far list.
- "Check All" → keyboard shortcut (Space or Ins to toggle).
- Very straightforward port. **Simplest of all 5 gaps.**

Implementation estimate: ~150-180 lines. Backend already exists.
