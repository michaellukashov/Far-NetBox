# Research

Updated: 2026-05-04 15:45
Status: active

## Active Summary (input for /aif-plan)

Updated: 2026-05-04 15:45

Goal: Capture institutional knowledge from deep codebase investigations to inform future planning and prevent regression

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
Open questions:
cr|- Silent mode file operations: Error collection mechanism designed but not implemented.
bw|- ~~Stack overflow (#497): Symlink cycle detection needed in CalculateFilesSize (mirrors FilesFind pattern).~~ **FIXED** (commit `2689164e6`, 2026-05-03).
qa|- ~~DST timestamp (#391): Remove erroneous DST subtraction in ConvertTimestampToUnix for dstmWin on Win7+.~~ **FIXED** (commit `17a50dfdc`, 2026-05-02).
iz|- ~~Private key auth (#392): Passphrase prompt misclassification or path encoding issue suspected; needs diagnostic logging.~~ **FIXED** (commit `e41274cd7`, 2026-05-02).
xm|- ~~Second file open crash: Dangling TRemoteFile pointers after directory refresh; Duplicate(false) recommended in CreateFileList.~~ **FIXED** (commits `311e2c8fc` + `8539f9963`, 2026-04-26).
na|Success signals:
lu|- Zero build warnings under /W4
cq|- Plugin DLL in Far3_<platform>/Plugins/NetBox/
pv|- Manual test protocol: connect, transfer, cancel, navigate for each protocol
fo|- No crashes in 48hr stress test
zb|
ho|Next step:
ma|- Prioritize remaining open question: silent mode file operations (designed, not implemented).
cq|- Resume WinSCP feature alignment roadmap: Phase 1 dialog UX refinements + upstream bug fix porting (no longer blocked by crash bugs).

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

kx|What changed:
id|- Audited RESEARCH.md "Open questions" against actual git commit history (2026-04-01 to 2026-05-04)
gs|- Discovered 4 of 5 listed bugs were already fixed; only silent mode remains genuinely open
ru|- Cross-referenced with Github-Issues.md tracker which correctly marked all four as FIXED
an|
gb|Key findings:
it|- #497: Fixed by `2689164e6` (2026-05-03) — symlink cycle detection + parent-dir defense in CalculateFilesSize
it|- #391: Fixed by `17a50dfdc` (2026-05-02) — removed DST subtraction in ConvertTimestampToUnix for Win7+
it|- #392: Fixed by `e41274cd7` (2026-05-02) — FRememberedPasswordKind guard + inverted Result check in PromptUser
it|- #508: Fixed by `311e2c8fc` + `8539f9963` (2026-04-26) — Duplicate(false) in CreateFileList + FFileList.reset()
it|- Silent mode: Still open — no commits; TFileOperationErrorLog designed but not implemented
it|
it|Action taken:
it|- Updated RESEARCH.md Active Summary: closed 4 bugs with commit references, updated Next step
it|- Reframes WinSCP feature alignment roadmap Phase 1 from "urgent bug fixes" to "proactive upstream porting + UX polish"
it|
zp|Links (paths):
vz|- .ai-factory/Github-Issues.md (correct tracker)
ub|- .ai-factory/plans/winscp-feature-alignment-roadmap.md (roadmap to resume)
up|- git log --since="2026-04-01" --until="2026-05-05"

### 2026-05-04 — WinSCP Upstream Contribution Audit

kx|What changed:
id|- Audited 4 NetBox fixes for upstream contribution potential to WinSCP 6.5.6 master
gs|- Examined WinSCP source at `D:\Projects\WinSCP-work\winscp-master\source` for each fix's current state
ru|- Ranked candidates by effort, WinSCP user value, and adaptation complexity
an|
gb|Key findings:
ke|it|- #391 DST timestamp (ConvertTimestampToUnix): WinSCP STILL HAS THE BUG. `!UsesDaylightHack()` branch subtracts DaylightDifferenceSec for dstmWin on Win7+ where FILETIME is pure UTC. HIGH upstream value — affects all SCP uploads during DST.
np|it|- #497 Symlink cycle detection: WinSCP ALREADY FIXED. Has TLoopDetector in TCalculateSizeParams and uses it in CalculateFileSize callback. NetBox fix was porting WinSCP's pattern — nothing to upstream.
qh|it|- #501 SSH/SCP buffer corruption: WinSCP has same dynamic send buffer code but this is a workaround (disable by default), not root cause fix. Better as issue report than upstream PR.
pl|it|- CWE-134 FMTLOAD escaping: WinSCP has same vulnerable call sites (FTP_RESPONSE_ERROR, REQUEST_REDIRECTED, INVALID_OUTPUT_ERROR). NO EscapeFmtChars() utility exists. Security hardening contribution — medium effort.
fy|it|
sf|it|Upstream candidate ranking:
it|it|1. #391 DST timestamp — small effort, high value, clean adaptation (same file `core/Common.cpp`)
it|it|2. CWE-134 FMTLOAD escaping — medium effort, security value, requires new utility function + 5 call sites
it|it|3. #501 buffer corruption — large effort (needs root cause), better as issue than PR
it|
xs|zp|Links (paths):
mw|vz|- `D:\Projects\WinSCP-work\winscp-master\source\core\Common.cpp` (ConvertTimestampToUnix:2218-2228)
vo|ub|- `D:\Projects\WinSCP-work\winscp-master\source\core\Terminal.cpp` (TLoopDetector, CalculateFileSize)
cq|up|- `D:\Projects\WinSCP-work\winscp-master\source\core\SecureShell.cpp` (SendBuffer, SIO_IDEAL_SEND_BACKLOG_QUERY)
sr|it|- `D:\Projects\WinSCP-work\winscp-master\source\core\FtpFileSystem.cpp`, `NeonIntf.cpp`, `ScpFileSystem.cpp` (FMTLOAD vulnerable sites)
it|- NetBox fix: `git show 17a50dfdc` (src/base/Common.cpp)
