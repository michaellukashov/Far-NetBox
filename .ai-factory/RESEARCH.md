# Research

Updated: 2026-05-07
Status: active

## Active Summary (input for /aif-plan)

Updated: 2026-05-07

Goal: Achieve structural UX parity with WinSCP — all dialogs that exist in WinSCP must exist in NetBox

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- WinXP compatibility (_WIN32_WINNT=0x0501)
- Incremental evolution — no architectural rewrites
- C++17 standard only (no std::filesystem, std::variant)

Decisions:
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
- Implement structural UX parity Phase 2: 4 missing dialogs + 1 missing button (see Session 2026-05-07 below for details).
- Prioritize remaining open question: silent mode file operations (designed, not implemented).

## Sessions

### 2026-05-02 — SSH Private Key Auth & DST Timestamp

What changed:
- Deep analysis of Issue #392 (private key auth failure) and Issue #391 (DST timestamp bug)
- Identified four suspected root causes for key auth: passphrase misclassification, path encoding, missing upstream fix, auto-conversion interference
- Mapped full auth flow from TSessionData through PuTTY userauth2-client.c
- Discovered ConvertTimestampToUnix subtracts DaylightDifferenceSec on Win7+ where FILETIME is already pure UTC

Key notes:
- PuTTY 0.81 natively supports OpenSSH keys but auth flow only accepts PPK format for private keys
- WinSCP Bug 1974 addressed related DST issue but different code path
- Fix scope: ConvertTimestampToUnix only; UnixToDateTime already correct; DateTimeToFileTime intentionally unchanged for download compatibility

Links (paths):
- .ai-factory/references/issue-392-private-key-auth-exploration.md
- .ai-factory/references/issue-391-dst-timestamp-exploration.md
- src/core/SecureShell.cpp, src/core/SessionData.cpp, src/base/Common.cpp

### 2026-04-29 — CWE-134 FMTLOAD Vulnerability Scan

What changed:
- Extended Issue #513 fix from 1 to 6 total vulnerable call sites
- Identified 5 additional FMTLOAD calls passing untrusted server/user data with % characters
- Designed EscapeFmtChars() reusable utility

Key notes:
- FTP_RESPONSE_ERROR in FtpFileSystem.cpp (3 sites) — HIGH risk
- REQUEST_REDIRECTED in NeonIntf.cpp — MEDIUM risk (URLs with %20)
- INVALID_OUTPUT_ERROR in ScpFileSystem.cpp — MEDIUM risk (remote shell output)
- INVALID_URL in NeonIntf.cpp — LOW risk (user-typed)
- Original fix in FtpControlSocket.cpp used 12-line inline escaping — should be refactored to EscapeFmtChars()

Links (paths):
- .ai-factory/references/cwe134-fmtload-vulnerability-scan.md
- src/core/FtpFileSystem.cpp, src/core/NeonIntf.cpp, src/core/ScpFileSystem.cpp

### 2026-04-29 — Multithreading Audit & Fix

What changed:
- Comprehensive threading review across src/NetBox/, src/core/, src/filezilla/, src/base/
- Found 5 critical Far Manager API thread affinity violations
- Found 3 race conditions, 4 busy-wait patterns, 7 static mutable state sites
- Implemented 14 tasks across 4 phases

Key notes:
- Critical: TFarDialogIdleThread, TPluginIdleThread, TQueueDialog::Idle, TTerminalThread::RunAction all called Far APIs from worker threads
- Race: FtpControlSocket Sleep(100) between unlock/relock on speed limit sync
- Real bug found: TGuard Guard(LibS3Section.get()) pointer-to-reference mismatch in S3FileSystem.cpp
- OpenSSL init wrapped in std::call_once for thread safety

Links (paths):
- .ai-factory/references/multithreading-audit-exploration.md
- .ai-factory/references/multithreading-review-fix-results.md
- .ai-factory/rules/threading.md

### 2026-04-29 — Issue #511 Esc Cancellation Deep Dive

What changed:
- Four-layer root cause analysis of SCP Esc cancellation hang
- Discovered Layer 5 (post-cancel session corruption) during verification
- 7 warning findings + 3 info findings from code review

Key notes:
- Layer 1: PeekConsoleInput only checked first event — mouse moves blocked Esc forever
- Layer 2: csCancel (from OK button) was ignored in SCP loops; only csCancelTransfer worked
- Layer 3: EAbort(EXCEPTION_MSG_REPLACED) triggered panel close via HandleException
- Layer 4: ReadCommandOutput during cleanup blocked on remote's buffered file data
- Layer 5: FNeedsSessionReset + FLastDirectory enables deferred reconnect before EnsureLocation
- 12 anti-patterns catalogued for prevention

Links (paths):
- .ai-factory/references/esc-cancellation-comprehensive-fix.md
- .ai-factory/references/issue-511-cancel-yes-hang-deep-dive.md
- .ai-factory/references/issue-511-speed-limit-esc-hang-exploration.md
- .ai-factory/references/issue-511-review-findings.md

### 2026-04-29 — CMake Refactoring Reconciliation

What changed:
- Audited REFACTORING_PLAN.md against actual codebase state
- Found 4 undocumented modules, significant line-count discrepancies

Key notes:
- Main CMakeLists.txt: 81 lines (not 102); src/CMakeLists.txt: 91 lines (not 340-530)
- Missing: Libraries.cmake, PlatformDetection.cmake, SourceGroups.cmake, TargetConfiguration.cmake
- Actual reduction: 97% (2478 -> 81 lines)
- baselib target was removed; all base sources merged into NetBox SHARED
- libs/tinylog.backup/ stale directory should be removed

Links (paths):
- .ai-factory/references/cmake-refactoring-plan-exploration.md
- CMakeLists.txt, src/CMakeLists.txt, cmake/*.cmake

### 2026-04-29 — Issue #501 SSH/SCP Buffer Corruption

What changed:
- Root cause: dynamic TCP send buffer resizing via SIO_IDEAL_SEND_BACKLOG_QUERY
- Corruption + 100% CPU when SendBuf > 0; ~6 MB/s vs ~30 MB/s when disabled

Key notes:
- Fix: Set SendBuf=0 and SshSimple=false by default
- WSALoctl polling every 1000ms interacts poorly with PuTTY's internal channel buffering
- NetBoxRus.lng:1104 already hints at disabling this setting

Links (paths):
- .ai-factory/references/issue-501-ssh-scp-buffer-corruption-exploration.md
- src/core/SecureShell.cpp, src/core/SessionData.cpp

### 2026-04-27 — SFTP Binary Dump & Log Protocol

What changed:
- Investigated Log Protocol levels 0-2 across all protocols
- Designed Level 3 for SFTP binary packet dump

Key notes:
- Current: Level 2 dumps binary for SFTP; proposed Level 3 for binary dump, Level 2 for headers only
- Requires MsgIDs.h + all .lng files update
- SessionInfo.cpp display logic needs "Debug 3" case

Links (paths):
- .ai-factory/references/sftp-binary-dump-log-protocol.md
- src/core/SftpFileSystem.cpp, src/core/SessionInfo.cpp, src/base/MsgIDs.h

### 2026-04-27 — Far Dialog API Text Retrieval

What changed:
- Investigated DM_GETTEXT vs DM_GETDLGITEM for autocomplete synchronization bug (#515)
- Recommended DM_GETDLGITEM for complete item state retrieval

Key notes:
- TFarEdit::GetText() returns cached Data; autocomplete updates Far control without firing DN_EDITCHANGE
- Proposed TFarEdit::GetTextFromDialog() method for fresh text retrieval
- Fallback to cached GetData() if dialog not shown or API fails

Links (paths):
- .ai-factory/references/far-dialog-api-text-retrieval.md
- src/NetBox/FarDialog.cpp, src/NetBox/WinSCPDialogs.cpp

### 2026-04-26 — Crash Analysis: Second File Open

What changed:
- Identified dangling TRemoteFile pointer bug after directory refresh
- Panel stores TRemoteFile* in UserData; refresh deletes objects; second file open crashes

Key notes:
- TRemoteDirectory::Reset() deletes owned objects when OwnsObjects=true
- CreateFileList() extracts stale pointers from TFarPanelItem::GetUserData()
- Recommended fix: Duplicate(false) in CreateFileList() for self-contained file list
- Workaround: Ctrl+R refresh before second open

Links (paths):
- .ai-factory/references/crash-second-file-open-analysis.md
- src/NetBox/WinSCPFileSystem.cpp, src/core/Terminal.cpp

### 2026-04-26 — SFTP Directory Size Stack Overflow (#497)

What changed:
- Root cause: recursive directory size calculation lacks symlink cycle detection
- 98,648 files in directory triggered STATUS_STACK_OVERFLOW

Key notes:
- FilesFind already has TLoopDetector pattern with AbsolutePath() — works correctly
- CalculateFilesSize/DoCalculateDirectorySize has no visited-directory tracking
- Fix: Add TStringList* VisitedDirs to TCalculateSizeParams, check before recursion

Links (paths):
- .ai-factory/references/sftp-directory-size-stack-overflow-497.md
- src/core/Terminal.cpp

### 2026-04-26 — SSH Authentication & OpenSSH Certs

What changed:
- Deep analysis of PuTTY 0.81 certificate support and NetBox wiring
- Mapped all auth-related GitHub issues (#263, #323, #156, #509, #388, #36)

Key notes:
- PuTTY 0.81 has full openssh-certs.c implementation; NetBox already wires FDetachedCertificate to CONF_detached_cert
- Critical gap: PuTTY auth flow only accepts PPK private keys; OpenSSH PEM/New formats detected but not auto-converted
- Must convert OpenSSH keys to PPK at runtime using import_ssh2() + save_ssh2_privatekey() before CONF_keyfile
- UI needed: OpensshPrivateKeyFile edit, UseOpensshCertificate checkbox on tabAuthentication

Links (paths):
- .ai-factory/references/ssh-authentication-exploration.md
- src/core/SecureShell.cpp, src/core/SessionData.h, libs/putty/ssh/userauth2-client.c

### 2026-04-26 — S3 Session Data & Encryption Alignment

What changed:
- Mapped WinSCP SessionData encryption fields against NetBox implementation
- Identified 4 gaps in S3 TLS/CA certificate handling

Key notes:
- S3CACertificate declared but not serialized (missing from DoLoad/DoSave)
- S3CACertificate read but not applied (TODO in S3FileSystem.cpp)
- TLS version UI missing for S3 tab (Min/Max TLS combos needed)
- WinSCP does not expose per-session CA for S3 either — this is a NetBox-specific enhancement

Links (paths):
- .ai-factory/references/netbox-s3filesystem-session-mapping.md
- .ai-factory/references/winscp-sessiondata-encryption-settings.md
- src/core/SessionData.cpp, src/core/S3FileSystem.cpp, src/NetBox/WinSCPDialogs.cpp

### 2026-04-26 — Foundation Stability Research

What changed:
- Catalogued RAII patterns, exception safety, smart pointer migration paths
- Documented 5 common pitfalls for Far Manager plugins

Key notes:
- TGuard RAII wrapper for critical sections — verified pattern
- FILE_OPERATION_LOOP_BEGIN/END macros for retry logic
- ExitFAR ordered cleanup: threads -> connections -> config -> globals
- WinXP: _WIN32_WINNT=0x0501, v141_xp toolset, avoid std::filesystem

Links (paths):
- .ai-factory/references/foundation-stability-research.md

### 2026-04-22 — Silent Mode File Operations

What changed:
- Designed silent mode architecture for non-interactive file operations
- Mapped confirmation architecture in Terminal.cpp

Key notes:
- Existing flags: cpNoConfirmation=0x08, spNoConfirmation=0x02
- EffectiveBatchOverwrite() returns boAll when silent mode active
- TFileOperationErrorLog class designed for error collection
- Global configuration (not per-session) for automation use cases

Links (paths):
- .ai-factory/references/silent-mode-research.md
- src/core/Terminal.cpp, src/core/Configuration.h

### 2026-04-22 — Logging Subsystem Reference

What changed:
- Documented tinylog architecture: async double-buffer, mutex + condition variable
- Three TinyLog instances: g_tinylog (global debug), TSessionLog (per-session), TActionLog (per-session XML)

Key notes:
- Buffer size: 16 KB; flush timeout: 3 sec
- _wfsopen with SH_DENYWR; setvbuf unbuffered with 4 KB buffer
- Log path placeholders: &S (session), &Y/&M/&D/&T (datetime), &P (PID), &@ (hostname)

Links (paths):
- .ai-factory/references/logging-subsystem.md
- libs/tinylog/, src/core/SessionInfo.cpp, src/NetBox/WinSCPPlugin.cpp

### 2026-05-03 — OpenSSH Certificate Auth: WinSCP Alignment

What changed:
- Cross-referenced NetBox implementation against WinSCP master source at D:\Projects\WinSCP-work\winscp-master\source
- Discovered WinSCP master has NO OpensshPrivateKeyFile/UseOpensshCertificate fields — these are NetBox-specific additions
- WinSCP converts keys INTERACTIVELY before StoreToConfig() via VerifyAndConvertKey() in GUI layer (SiteAdvanced.cpp, ImportSessions.cpp, TerminalManager.cpp)
- NetBox must convert SILENTLY inside StoreToConfig() or Open() because Far plugin has no interactive conversion dialogs during connect
- Confirmed: PuTTY 0.81 auth flow (userauth2-client.c:1282) only accepts PPK format for CONF_keyfile

Key notes:
- WinSCP passphrase encryption: always uses PublicKeyFile as key; re-encrypts when PublicKeyFile changes (SetPublicKeyFile handles this)
- NetBox gap: ResolvePublicKeyFile() never checks UseOpensshCertificate/OpensshPrivateKeyFile — always returns PublicKeyFile
- NetBox gap: Passphrase encrypted with PublicKeyFile even when cert mode uses OpensshPrivateKeyFile as the actual key
- NetBox gap: No silent conversion utility — only interactive ConvertKey() in Tools.cpp
- WinSCP temp key pattern: FTemporaryKeyFile in TWinConfiguration for embedded session keys, not for converted SSH keys
- Recommended approach: Add ResolveEffectiveKeyFile() to TSessionData; add silent ConvertKeyToTemporaryPPK() in PuttyIntf.cpp; wire in StoreToConfig(); cleanup in ~TSecureShell()

Links (paths):
- D:\Projects\WinSCP-work\winscp-master\source\core\SecureShell.cpp (StoreToConfig:251)
- D:\Projects\WinSCP-work\winscp-master\source\core\SessionData.cpp (SetPassphrase:3272, SetPublicKeyFile:3237)
- D:\Projects\WinSCP-work\winscp-master\source\windows\Tools.cpp (ConvertKey:1299, VerifyAndConvertKey:1444)
- D:\Projects\WinSCP-work\winscp-master\source\forms\SiteAdvanced.cpp (DetachedCertificateEdit UI)
- src/core/SecureShell.cpp, src/core/SessionData.h, src/core/PuttyIntf.cpp, src/NetBox/WinSCPDialogs.cpp

### 2026-05-04 — Open Bug Audit: RESEARCH.md Stale Status

What changed:
- Audited RESEARCH.md "Open questions" against actual git commit history (2026-04-01 to 2026-05-04)
- Discovered 4 of 5 listed bugs were already fixed; only silent mode remains genuinely open
- Cross-referenced with Github-Issues.md tracker which correctly marked all four as FIXED

Key findings:
- #497: Fixed by `2689164e6` (2026-05-03) — symlink cycle detection + parent-dir defense in CalculateFilesSize
- #391: Fixed by `17a50dfdc` (2026-05-02) — removed DST subtraction in ConvertTimestampToUnix for Win7+
- #392: Fixed by `e41274cd7` (2026-05-02) — FRememberedPasswordKind guard + inverted Result check in PromptUser
- #508: Fixed by `311e2c8fc` + `8539f9963` (2026-04-26) — Duplicate(false) in CreateFileList + FFileList.reset()
- Silent mode: Still open — no commits; TFileOperationErrorLog designed but not implemented

Action taken:
- Updated RESEARCH.md Active Summary: closed 4 bugs with commit references, updated Next step
- Reframes WinSCP feature alignment roadmap Phase 1 from "urgent bug fixes" to "proactive upstream porting + UX polish"

Links (paths):
- .ai-factory/Github-Issues.md (correct tracker)
- .ai-factory/plans/winscp-feature-alignment-roadmap.md (roadmap to resume)
- git log --since="2026-04-01" --until="2026-05-05"

### 2026-05-04 — WinSCP Upstream Contribution Audit

What changed:
- Audited 4 NetBox fixes for upstream contribution potential to WinSCP 6.5.6 master
- Examined WinSCP source at `D:\Projects\WinSCP-work\winscp-master\source` for each fix's current state
- Ranked candidates by effort, WinSCP user value, and adaptation complexity

Key findings:
- #391 DST timestamp (ConvertTimestampToUnix): WinSCP STILL HAS THE BUG. `!UsesDaylightHack()` branch subtracts DaylightDifferenceSec for dstmWin on Win7+ where FILETIME is pure UTC. HIGH upstream value — affects all SCP uploads during DST.
- #497 Symlink cycle detection: WinSCP ALREADY FIXED. Has TLoopDetector in TCalculateSizeParams and uses it in CalculateFileSize callback. NetBox fix was porting WinSCP's pattern — nothing to upstream.
- #501 SSH/SCP buffer corruption: WinSCP has same dynamic send buffer code but this is a workaround (disable by default), not root cause fix. Better as issue report than upstream PR.
- CWE-134 FMTLOAD escaping: WinSCP has same vulnerable call sites (FTP_RESPONSE_ERROR, REQUEST_REDIRECTED, INVALID_OUTPUT_ERROR). NO EscapeFmtChars() utility exists. Security hardening contribution — medium effort.

Upstream candidate ranking:
1. #391 DST timestamp — small effort, high value, clean adaptation (same file `core/Common.cpp`)
2. CWE-134 FMTLOAD escaping — medium effort, security value, requires new utility function + 5 call sites
3. #501 buffer corruption — large effort (needs root cause), better as issue than PR

Links (paths):
- `D:\Projects\WinSCP-work\winscp-master\source\core\Common.cpp` (ConvertTimestampToUnix:2218-2228)
- `D:\Projects\WinSCP-work\winscp-master\source\core\Terminal.cpp` (TLoopDetector, CalculateFileSize)
- `D:\Projects\WinSCP-work\winscp-master\source\core\SecureShell.cpp` (SendBuffer, SIO_IDEAL_SEND_BACKLOG_QUERY)
- `D:\Projects\WinSCP-work\winscp-master\source\core\FtpFileSystem.cpp`, `NeonIntf.cpp`, `ScpFileSystem.cpp` (FMTLOAD vulnerable sites)
- NetBox fix: `git show 17a50dfdc` (src/base/Common.cpp)

### 2026-05-06 — Master Password Infrastructure Audit & Cleanup

What changed:
- Audited `master-password-infrastructure-research.md` gaps against actual codebase. Verified which gaps are closed, which are non-issues, and which remain theoretical.

Key findings:
- **Gap 3 (TSecureString): CLOSED** — `TSecureString` exists in `src/base/SecureString.cpp` with `VirtualLock`, `SecureZeroMemory`, move-only semantics. Already used for `FPlainMasterPasswordEncrypt`/`Decrypt`.
- **Gap 4 (Thread-unsafe counters): CLOSED** — `FMasterPasswordSession` is `std::atomic<int32_t>`, `FMasterPasswordSessionAsked` is `std::atomic<bool>`. `TValidationAttemptTracker` uses `std::atomic<uint32_t>`.
- **Gap 5 (No rate limiting): CLOSED** — `ValidateMasterPassword` implements 5-failure → 30s lockout with `GetTickCount()` (WinXP-compatible wrap-safe subtraction). `CountAttempt` parameter allows dialog inline validation without counting.
- **Gap 6 (Hardcoded strings): CLOSED** — All dialog strings use `GetMsg(NB_MASTER_PASSWORD_*)` MsgIDs. No literals in `WinSCPDialogs.cpp`.
- **Gap 7 (Recryption errors discarded): CLOSED** — Errors are logged via `AppLogFmt` and shown to user via `MessageDialog` with recrypt error count. Minor UX gap vs WinSCP's `MoreMessageDialog`, but not silent discard.
- **Gap 2 (Exception safety): NON-ISSUE** — `try__finally` / `__finally` macros guarantee cleanup on unwind. Theoretical stale-decrypt-key window during exception is acceptable risk.
- **Gap 1 (RecryptPasswords stub): NON-ISSUE** — NetBox has no central `TTerminalManager` registry of active terminals. Each Far panel owns its own `TTerminal` via `TWinSCPFileSystem::FTerminal`. Active terminals do not persist remembered passwords across reconnect in the Far plugin context, so the missing active-terminal walk is not reachable in practice.

Action taken:
- Closed 6 of 7 gaps as resolved or non-issues. Master password infrastructure is effectively complete; no implementation work required. Removed from active concerns.

Links (paths):
- `.ai-factory/references/master-password-infrastructure-research.md` (legacy assessment, now partially stale)
- `src/base/SecureString.cpp`
- `src/windows/MasterPassword.cpp`
- `src/windows/WinConfiguration.h` (atomic counters, TValidationAttemptTracker)
- `src/NetBox/WinSCPDialogs.cpp` (dialog validation flow)

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

#### Deep Dive: TLocationProfilesDialog

WinSCP source: `forms/LocationProfiles.cpp` (~1102 lines). Most complex of the missing dialogs.

Structure: Two TreeViews (Session profiles / Shared profiles) on tab pages. Each tree has folders and bookmarks (local+remote dir pairs). Add/Remove/Rename/MoveTo/Up/Down buttons per tree. Local + Remote directory edit fields. OK/Cancel.

Key data model already exists in NetBox:
- `TBookmarkList` — fully implemented in `src/core/Bookmarks.cpp`, supports folders, load/save, shortcuts
- `TBookmarks` — per-session + shared, used by `TFarConfiguration`
- `TWinConfiguration::UseLocationProfiles`, `UseSharedBookmarks` — config flags exist
- `TWinSCPFileSystem::OpenDirectoryDialog()` — existing Far dialog that manages bookmarks as a flat menu with Delete/Copy shortcuts

So NetBox already has a simplified bookmark manager in `OpenDirectoryDialog`. The gap is: WinSCP has a *full CRUD editor* with folders, reorder, rename, shared profiles, and local+remote pairs. NetBox has a *flat menu* with add/delete only.

Far text-mode adaptation:
- Two TreeViews → Two `TFarListBox` lists (Session/Shared tabs or single list with separator).
- Folders → collapsible in Far via [+] / [-] prefixes (like Far's folder tree in panels).
- Add/Remove/Rename/MoveTo/Up/Down → Far menu items or dialog buttons.
- Local + Remote directory edits → Two `TFarEdit` fields.
- Drag-drop reordering → **Not portable**. Use Up/Down buttons or reorder via menu.
- TreeViewScrollOnDragOver → **Not portable**. Omit.

Implementation estimate: ~200-250 lines. Data model exists; only UI shell is new.

#### Deep Dive: CopyParamCustom page

WinSCP source: `forms/CopyParamCustom.cpp` (~74 lines!). It's just a thin wrapper around the existing `CopyParamsFrame`.

Structure: A form that embeds `TCopyParamsFrame` (the full copy-param editor with all tabs: Transfer/Exclude/Include/Log/Other). It's opened from `TCopyDialog` when user selects "Custom" instead of a named preset.

Current NetBox state:
- CopyParamPreset dialog was added in Phase 1 — but it only manages *global* presets.
- When copying, user selects a preset name. No per-session "Custom" override.
- The underlying `TCopyParamType` editor (`TCopyParamsFrame` equivalent) already exists inside `TSessionDialog` on the "Directories" and "Environment" tabs.

Far text-mode adaptation:
- In WinSCP, "Custom" is a radio option next to the preset combo in CopyDialog. Selecting it opens the full param editor, and the custom params are stored per-session.
- For NetBox: Add a "Custom" option to the Copy dialog's preset combo. When selected, open the existing copy-param editor dialog. Store custom params on `TSessionData` (needs new field).
- The editor already exists — just need the wiring: "Custom" → open editor → store result.

Implementation estimate: ~100-150 lines. The editor dialog already exists; only wiring and a new `TSessionData` field needed.

#### Summary: Revised Implementation Estimates

| # | Dialog | Est. Lines | Complexity | Key Risk |
|---|--------|-----------|------------|----------|
| 1 | TGenerateUrlDialog | 250-300 | Medium | Script gen needs exe-name adjustment; Assembly tab omitted |
| 2 | TCleanupDialog | 150-180 | Low | Simplest; all backend exists |
| 3 | TLocationProfilesDialog | 200-250 | Medium | Data model exists; UI is new but well-patterned |
| 4 | CopyParamCustom | 100-150 | Low | Editor exists; just wiring |
| 5 | "Generate Key" button | 30-50 | Low | Plumbing exists; just add button |
| **Total** | | **730-930** | | |
