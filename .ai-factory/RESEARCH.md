# Research

Updated: 2026-05-14
Status: active

## Active Summary (input for /aif-plan)

Updated: 2026-05-12

Goal:
| Threading safety audit exploration — identify remaining gaps after multithreading-review-fix.md completion.
| `TPuttyCleanupThread` still uses `Sleep()` in 2 places (Finalize: Sleep(100), Execute: Sleep(400)).
| `Global.cpp` tracing globals: ALREADY guarded by `TracingCriticalSection` — no fix needed.
| `.ai-factory/rules/threading.md`: exists but incomplete — missing actual lock hierarchy.
| **NEXT: Expand `.ai-factory/rules/threading.md` with complete TCriticalSection inventory, full event-object catalog, Sleep() pattern taxonomy, and TGuard usage patterns.**
Constraints:
| No modifications to `libs/` — use patches only
| Far Manager API calls on main thread only
| MSVC /W4 zero warnings
| Follow existing threading rules in `.ai-factory/rules/threading.md`
Decisions:
| Plan `multithreading-review-fix.md` is COMPLETE (all 14 tasks done, build verified).
| Plan `threading-safety-audit-and-fixes.md` is STALE — identifies same issues already fixed.
| `TPuttyCleanupThread` should use `WaitForSingleObject` on events instead of `Sleep()`.
| `Global.cpp` tracing: `WriteTraceBuffer()` holds `TracingCriticalSection`; `DoDirectTrace()`/`DoTrace()` are already guarded. `DoAssert()` reads `IsTracing` without lock but `bool` is atomic on x86/x64 — acceptable.
| `.ai-factory/rules/threading.md` needs expansion: add actual lock hierarchy (m_SpeedLimitSync -> FItemsSection -> FSection -> FileOperationProgress::FSection), event objects (m_SpeedLimitEvent, FClientsZeroEvent, FDirectoryCreatedEvent, m_hStartedEvent), and state inventory.
| **Ground-truth verified: TPuttyCleanupThread ALREADY uses WaitForSingleObject in both Finalize() and Execute() — the Sleep(400) is only a defensive null-event fallback. Source audit was based on outdated snapshot.**
Open questions:
| Should `TPuttyCleanupThread::Finalize()` use INFINITE wait or a reasonable timeout?
| Should `TPuttyCleanupThread::Execute()` use a manual-reset event or a waitable timer?
| **RESOLVED via source read: Current implementation is correct. No code change needed.**
Success signals:
| `TPuttyCleanupThread` Sleep calls replaced with WaitForSingleObject
| `.ai-factory/rules/threading.md` expanded with lock hierarchy and event objects
| `threading-safety-audit-and-fixes.md` plan marked as STALE/COMPLETE
| **NEW: threading.md duplicate header fixed, full TCriticalSection inventory added, all 16+ event objects catalogued, Sleep() pattern taxonomy documented**
Next step:
| Create plan for TPuttyCleanupThread fix + threading rules expansion (OR implement directly if user prefers)
| **REVISED: No TPuttyCleanupThread code fix needed — only documentation expansion in threading.md. Transition to /aif-plan to apply.**
## Sessions

### 2026-05-12 — Stale Plan & Roadmap Cleanup

What changed:
- Marked `terminal-thread-progress-marshaling.md` plan as COMPLETE — implementation already exists in source.
- Updated ROADMAP.md v1.2 milestone to COMPLETE.
- Verified no source changes were needed (TProgressUserAction, TFinishedUserAction already in Queue.cpp).

Key decisions:
- Stale plan preservation: kept plan file for history, updated header/status to reflect actual state.
- Roadmap v1.2 now accurately reflects completed background copy & progress UI.

Links (paths):
|- `.ai-factory/plans/terminal-thread-progress-marshaling.md` (marked COMPLETE)
|- `.ai-factory/ROADMAP.md` (v1.2 marked COMPLETE)

### 2026-05-12 — TerminalThread Marshaling & Background Queue Verification

What changed:
- Verified TTerminalThread OnProgress/OnFinished marshaling is ALREADY in source:
  TProgressUserAction (Queue.cpp:255), TFinishedUserAction (Queue.cpp:278),
  TerminalProgress() / TerminalFinished() methods (Queue.cpp:3247),
  InitTerminalThread() save/override/restore (Queue.cpp:2802-2818),
  destructor restore (Queue.cpp:2852-2853).
- Plan `.ai-factory/plans/terminal-thread-progress-marshaling.md` is stale — marked for update.
- Verified Background copy & progress UI (roadmap v1.2) is COMPLETE:
  TQueueDialog exists with Execute/Delete/MoveUp/MoveDown controls (WinSCPDialogs.cpp:10833)
  3-line progress display per item: operation+direction+source, dest+%, filename+%
  TBackgroundTerminal runs transfers in worker thread (Queue.cpp:1352)
  Queue beep on empty (WinSCPFileSystem.cpp:4188) and pending action (4194)
  Queue auto-popup = auto-process user actions (passwords, errors) at 4143
  Far taskbar progress updated during queue ops (ProcessQueue:4117)
- Roadmap v1.2 "Remaining: minor polish items (queue auto-popup, beep on completion)" is stale —
  both features exist and are wired in transfer dialog + config storage.

Key decisions:
- No implementation needed for threads 1 or 3 — source is complete.
- Next functional work: Phase 3 Workspace save/restore (POSTPONED) or Phase 4 Directory comparison.

Links (paths):
- src/core/Queue.cpp (TTerminalThread, TProgressUserAction, TFinishedUserAction, TBackgroundTerminal)
- src/NetBox/WinSCPDialogs.cpp (TQueueDialog, FillQueueItemLine, QueueItemNeedsFrequentRefresh)
- src/NetBox/WinSCPFileSystem.cpp (ProcessQueue, QueueShow, QueueAddItem, QueueEvent)
- src/NetBox/FarConfiguration.cpp/h (QueueBeep, QueueAutoPopup settings)
- .ai-factory/plans/terminal-thread-progress-marshaling.md (stale — needs marking DONE)
- .ai-factory/ROADMAP.md (v1.2 status needs updating to COMPLETE)

### 2026-05-10 (evening) — Phase 2 Verification Fixes + Gap Resolution

What changed:
- All 4 Phase 2 verification issues fixed and committed (`9b5f49b01`)
- Gap A resolved: Local dir navigation via `SynchronizeBrowsing()` in Location Profiles wrapper
- Gap B resolved: Clipboard button null guards added to TGenerateUrlDialog
- Plan `phase2-verification-fixes.md` updated with all issues marked DONE
- 1312 .lng entries aligned, zero build warnings, CRLF verified

Key decisions:
- Local dir navigation uses existing `TWinSCPFileSystem::SynchronizeBrowsing()` (FarControl FCTL_SETPANELDIRECTORY + PANEL_PASSIVE)
- Clipboard null guards use early return pattern (consistent with other null guards in TGenerateUrlDialog)

Links (paths):
- src/NetBox/WinSCPDialogs.cpp (local dir nav + null guards)
- .ai-factory/plans/phase2-verification-fixes.md (updated plan)

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
### 2026-05-12 — Threading Safety Audit Exploration

What changed:
- Traced full threading safety landscape: two plans exist (`multithreading-review-fix.md` = COMPLETE, `threading-safety-audit-and-fixes.md` = STALE)
- All CRITICAL and HIGH issues from the audit plan are already fixed in the codebase
- Verified `Global.cpp` tracing globals: `WriteTraceBuffer()` holds `TracingCriticalSection`; `DoDirectTrace()`/`DoTrace()` are guarded. `DoAssert()` reads `IsTracing` without lock but `bool` is atomic on x86/x64 — acceptable.
- Identified `TPuttyCleanupThread` as the only remaining `Sleep()` user that should use `WaitForSingleObject`:
  - `Finalize()` line 219: `Sleep(100)` polls for thread self-destruction → should wait on `HANDLE FDoneEvent`
  - `Execute()` line 278: `Sleep(400)` polls until cleanup timer expires → should wait on `HANDLE FTimerEvent`
- `.ai-factory/rules/threading.md` exists but is incomplete: missing actual lock hierarchy, event objects, state inventory

Key decisions:
- `TPuttyCleanupThread::Finalize()` → replace `Sleep(100)` with `WaitForSingleObject(FDoneEvent, INFINITE)`
- `TPuttyCleanupThread::Execute()` → replace `Sleep(400)` with `WaitForSingleObject(FTimerEvent, 400)`, add `DoSchedule()` to set event
- `Global.cpp`: no fix needed — already correctly guarded
- `.ai-factory/rules/threading.md` needs expansion with lock hierarchy and event objects
- `threading-safety-audit-and-fixes.md` should be marked as STALE/COMPLETE

Links (paths):
- `src/windows/GUITools.cpp` (TPuttyCleanupThread: lines 166-303)
- `src/base/Global.cpp` (tracing globals: lines 69-188)
- `.ai-factory/rules/threading.md` (threading rules: lines 1-42)
- `.ai-factory/plans/multithreading-review-fix.md` (implementation plan: all 14 tasks [x])
- `.ai-factory/plans/threading-safety-audit-and-fixes.md` (audit plan: quality gates unchecked)
- `.ai-factory/references/multithreading-review-fix-results.md` (fix results)

### 2026-05-14 — Complete Threading Rules Expansion Audit

What changed:
| Comprehensive audit of ALL TCriticalSection instances, event objects, and Sleep() patterns across src/.
| Ground-truth verified against live source: TPuttyCleanupThread already correctly uses WaitForSingleObject.
| Current threading.md has: duplicate header row in Event Objects table, incomplete inventory (5 of ~30 locks, 6 of 16 events), no Sleep() taxonomy, no TGuard patterns.

Key findings — Current threading.md defects:
| 1. **Duplicate markdown header**: Event Objects table has two identical header rows (renders badly).
| 2. **Section numbering**: "5" appears twice (Static/Global Mutable State AND Thread-Safe State Inventory), then 6, 7, 8 — should be 5, 5.1, 5.2, 6, 7, 8, 9, 10.
| 3. **Incomplete TCriticalSection inventory**: Only 5 locks listed; actual count is ~30 across all layers.
| 4. **Incomplete event catalog**: Only 6 events listed; actual count is 16 CreateEvent() calls.
| 5. **Missing Sleep() taxonomy**: No documentation of which Sleep() calls are legitimate vs replaced.
| 6. **Missing TGuard/TUnguard patterns**: No documentation of RAII lock wrapper usage.
| 7. **Missing marshaling primitives**: No reference to Synchronize/PostMainThreadSynchro/WaitForUserAction patterns.

Complete TCriticalSection inventory (verified by source search):

| Layer | Lock | File | Protected State | Notes |
|-------|------|------|----------------|-------|
| Plugin | TCustomFarPlugin::FCriticalSection | FarPlugin.h:235 | Plugin callback state | Entry-point guards |
| Plugin | TFarDialog::FCriticalSection | FarPlugin.h:370 | Dialog internal state | Idle thread synchronization |
| Plugin | TWinSCPFileSystem::FQueueStatusSection | WinSCPFileSystem.h:322 | Queue status, event pending flags | Main thread only |
| Base | TracingCriticalSection | Global.cpp:73/125 | TraceFile, IsTracing, WriteTraceBuffer | DoAssert reads IsTracing lockless (atomic bool) |
| Base | DateTimeParamsSection | Common.cpp:2463 | YearlyDateTimeParams | Lazy init, read-heavy |
| Base | IgnoredExceptionsCriticalSection | Exceptions.cpp:17 | IgnoredExceptions set | Exception-type deduplication |
| Core | CoreMainCriticalSection | CoreMain.cpp:24 | AnySession, StoredSessionsInitialized, StoredSessions | Session lifecycle |
| Core | TConfiguration::FCriticalSection | Configuration.h:249 | Config values, OnChange dispatch | Heavily contended |
| Core | TFileOperationProgressType::FSection | FileOperationProgress.h:172 | Progress data (size, speed, cancel) | Recursive; address-ordered in Assign() |
| Core | TFileOperationProgressType::FUserSelectionsSection | FileOperationProgress.h:173 | User selection state | Secondary lock |
| Core | TTerminalQueue::FItemsSection | Queue.h:155 | Queue item list | Parent of TQueueItem::FSection |
| Core | TQueueItem::FSection | Queue.h:248 | Individual queue item state | Child of FItemsSection |
| Core | TTerminalThread::FSection | Queue.h:596 | Terminal thread state | Marshaling handshake |
| Core | TParallelOperation::FSection | Terminal.h:1205 | Parallel transfer client count | Never held with FItemsSection |
| Core | TFTPFileSystem::FQueueCriticalSection | FtpFileSystem.h:239 | FTP message queue | Signals via FQueueEvent |
| Core | TFTPFileSystem::FTransferStatusCriticalSection | FtpFileSystem.h:240 | FTP transfer progress | Protocol-layer progress |
| Core | TSecureShell::FSocketEvent | SecureShell.cpp:95 | Socket readiness | WSAEventSelect integration |
| Core | TRemoteDirectoryCache::FSection | RemoteFiles.h:420 | Cached directory entries | Navigation invalidation |
| Core | LibS3Section | S3FileSystem.cpp:80 | libs3 init/deinit refcount | Global singleton pattern |
| Core | PasswordFilesCacheSection | SessionData.cpp:2056 | Password file cache map | Per-file cache |
| Core | DebugSection | NeonIntf.cpp:426 | NeonTerminals set | neon callback registration |
| Core | PuttyStorageSection | PuttyIntf.cpp:550 | PuttyStorage redirect | PuTTY config marshaling |
| Core | TSessionLog::FCriticalSection | SessionInfo.h:341 | Log file/stream | I/O bound |
| Core | TActionLog::FCriticalSection | SessionInfo.h:414 | Action log entries | I/O bound |
| Core | TCallStackLog::FCriticalSection | SessionInfo.h:456 | Callstack entries | Debug only |
| Core | TUsage::FCriticalSection | Usage.h:37 | Usage counters | Analytics |
| Core | TWebDAVFileSystem::FNeonLockStoreSection | WebDAVFileSystem.h:178 | neon lock store | WebDAV locking |
| filezilla | CAsyncSslSocketLayer::m_sCriticalSection | AsyncSslSocketLayer.h:197 | SSL layer list (static) | Cross-instance SSL state |
| filezilla | CAsyncSslSocketLayer::m_CriticalSection | AsyncSslSocketLayer.h:198 | Per-instance SSL state | Connection-specific |
| filezilla | CFtpControlSocket::m_SpeedLimitSync | FtpControlSocket.h:179 | Speed limit active list | Leaf lock; unlock-sleep-relock pattern replaced with event |
| Windows | TPuttyCleanupThread::FSection | GUITools.cpp:188 | Singleton instance pointer | Self-destructing thread |
| Windows | SystemRequiredThreadSection | GUITools.cpp:2550 | SystemRequiredThread singleton | Power management |
| Windows | TOwnConsole::FSection | ConsoleRunner.cpp:88 | Console singleton instance | Console runner state |
| Windows | TConsoleCommStruct::FSection | ConsoleRunner.cpp:566 | Console communication struct | Inter-thread console I/O |
| Windows | TTerminalManager::FQueueSection | TerminalManager.h:118 | Terminal-manager queue events | Session event dispatch |
| Windows | TTerminalManager::FChangeSection | TerminalManager.h:121 | Configuration change counter | Config change batching |
| Windows | StackTraceCriticalSection | WinInterface.cpp:615 | StackTraceMap (TLS->TStrings) | Debug-only callstack capture |

Complete Event Objects inventory (verified by source search):

| Event | File | Type | Purpose | Replaced Sleep |
|-------|------|------|-------|---------------|
| TFarDialogIdleThread::FEvent | FarDialog.cpp:33 | Auto-reset | Dialog idle processing signal | N/A (new) |
| TFarDialog::FSynchronizeObjects[1] | FarDialog.cpp:820 | Auto-reset | Dialog Synchronize() completion | N/A (new) |
| TKeepAliveThread::FEvent | WinSCPFileSystem.cpp:276 | Auto-reset | Keepalive heartbeat signal | N/A (new) |
| TFTPFileSystem::FQueueEvent | FtpFileSystem.cpp:327 | Manual-reset | FTP message queue non-empty | N/A (new) |
| TSignalThread::FEvent | Queue.cpp:477 | Auto-reset | Generic signal thread wake | N/A (new) |
| TTerminalThread::FActionEvent | Queue.cpp:2771 | Auto-reset | User action completion signal | N/A (new) |
| TSecureShell::FSocketEvent | SecureShell.cpp:95 | Auto-reset | WSA socket readiness | N/A (new) |
| TSecureShell connect Event | SecureShell.cpp:656 | Auto-reset | Async socket connect completion | N/A (new) |
| TParallelOperation::FClientsZeroEvent | Terminal.cpp:750 | Manual-reset | All parallel clients finished | Sleep(200) |
| TParallelOperation::FDirectoryCreatedEvent | Terminal.cpp:751 | Manual-reset | Directory created in parallel op | Sleep(100) |
| CFtpControlSocket::m_SpeedLimitEvent | FtpControlSocket.cpp:6059 | Auto-reset | Speed limit recalculation ready | Sleep(100) |
| CMainThread::m_hStartedEvent | MainThread.cpp:27 | Manual-reset | Main thread startup complete | Sleep(10) |
| TPuttyCleanupThread::FTimerEvent | GUITools.cpp:173 | Auto-reset | Cleanup timer expiration | Sleep(400) |
| TPuttyCleanupThread::FDoneEvent | GUITools.cpp:194 | Auto-reset | Cleanup thread finalization | Sleep(100) |
| TSynchronizeController::FStopEvent | SynchronizeController.cpp:74 | Manual-reset | Sync poller shutdown | N/A (new) |
| TCallstackThread named event | WinInterface.cpp:1598 | Auto-reset (named) | Cross-process callstack dump | N/A (new) |

Sleep() Pattern Taxonomy (all Sleep() calls in src/):

| Category | Count | Files | Rationale |
|----------|-------|-------|-----------|
| **Thread sync — REPLACED with events** | 0 active | — | All sync Sleep()s replaced in multithreading-review-fix.md |
| **Thread sync — defensive fallback** | 2 | GUITools.cpp:290 (null FTimerEvent), Terminal.cpp:7833 (null FDirectoryCreatedEvent) | Dead code under normal conditions; event creation always succeeds |
| **I/O retry backoff** | 1 | HierarchicalStorage.cpp:1850 | File create SHARING_VIOLATION retry; legitimate |
| **Intentional pacing/delay** | 2 | CoreMain.cpp:373 (benchmark), Queue.cpp:1396 (session reopen), Setup.cpp:736 (shell default) | Time-based behavior, not synchronization |
| **Console input polling** | 3 | ConsoleRunner.cpp:433, 502, 1083 | Console-mode message loop; no event alternative for null console |
| **External process wait** | 2 | ConsoleRunner.cpp:2806 (dump file), Tools.cpp:433 (shell wait) | ProcessMessages()+Sleep pattern; could use WaitForInputIdle but acceptable |
| **Pipe/IPC polling** | 1 | GUITools.cpp:395 | TPuttyPasswordThread named pipe client wait; timeout-based |

TGuard / TUnguard Patterns:
| Pattern | Usage | Notes |
|---------|-------|-------|
| `const TGuard Guard(section)` | 95% of locks | RAII Enter/Leave; exception-safe |
| `TGuard Guard(section)` (non-const) | 5% (e.g., S3FileSystem) | Same semantics; style inconsistency only |
| `const TGuard Guard1(*first); const TGuard Guard2(*second);` | FileOperationProgress.cpp:187 | Address-ordered dual lock to prevent deadlock in Assign() |
| `TUnguard` | Rare | Explicit early unlock (inverse of TGuard); used in legacy code |
| `TGuard Guard(*section.get())` | Static `unique_ptr<TCriticalSection>` | Dereference pattern for heap-allocated sections |
| `TGuard Guard(FSection.get())` | ConsoleRunner.cpp:128 | Raw pointer variant (same semantics) |

Marshaling Primitives:
| Primitive | File | Pattern | Usage |
|-----------|------|---------|-------|
| `TFarDialog::Synchronize(TThreadMethod)` | FarDialog.cpp:811 | Semaphore+event pair; worker posts, main executes | Dialog idle processing |
| `TCustomFarPlugin::PostMainThreadSynchro()` | FarPlugin.cpp | ACTL_SYNCHRO wrapper | General idle event posting |
| `TTerminalThread::WaitForUserAction()` | Queue.cpp:2845 | Event wait loop on worker; main calls RunAction() | Queue user-action marshaling |
| `TTerminalThread::RunAction()` | Queue.cpp:2870 | Sets action, signals event, executes on main | Main-thread action execution |
| `TTunnelUI` thread-ID guard | WinSCPFileSystem.cpp | `GetCurrentThreadId() != FMainThreadId` -> return | Defensive bail-out for callback safety |

Proposed threading.md restructuring:
```
1. Far Manager API Calls (unchanged)
2. Worker Thread UI Work (unchanged)
3. No Sleep Polling for Synchronization (unchanged)
4. Lock Ordering (expand with full hierarchy)
5. Static / Global Mutable State (unchanged)
  5.1 Thread-Safe State Inventory (expand to ~30 entries)
  5.2 TraceInitPtr Convention (new: document TraceInitPtr macro for static init)
6. Progress Callbacks (unchanged)
7. TGuard / TUnguard Patterns (new section)
8. Lock Hierarchy (expand with all locks, add cross-lock rules)
9. Event Objects (fix duplicate header; expand to 16 entries)
10. Sleep() Pattern Taxonomy (new section)
11. Marshaling Primitives (new section)
```

Key decisions:
| No code changes needed to TPuttyCleanupThread — source already correct.
| threading.md needs structural fix (duplicate header, numbering) + content expansion.
| All Sleep() remaining in codebase are legitimate (I/O backoff, pacing, console polling, defensive fallbacks).

Links (paths):
| `.ai-factory/rules/threading.md` (current rules with defects)
| `src/base/Global.cpp` (TGuard/TUnguard definitions, TracingCriticalSection)
| `src/base/System.SyncObjs.cpp` (TCriticalSection implementation)
| `src/windows/GUITools.cpp` (TPuttyCleanupThread: lines 166-330 — verified correct)
| `src/core/Queue.cpp` (TSignalThread, TTerminalThread marshaling)
| `src/core/Terminal.cpp` (TParallelOperation events)
| `src/filezilla/FtpControlSocket.cpp` (m_SpeedLimitEvent, m_SpeedLimitSync)
| `src/NetBox/FarDialog.cpp` (Synchronize, idle thread)
| `src/NetBox/WinSCPFileSystem.cpp` (KeepAlive thread, TTunnelUI guard)
