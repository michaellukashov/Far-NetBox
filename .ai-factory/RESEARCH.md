# Research

Updated: 2026-05-03 14:30
Status: active

## Active Summary (input for /aif-plan)

Topic: Far-NetBox plugin stability, protocol correctness, and modernization — consolidated findings from reference explorations

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
- Silent mode file operations: Error collection mechanism designed but not implemented.
- Stack overflow (#497): Symlink cycle detection needed in CalculateFilesSize (mirrors FilesFind pattern).
- DST timestamp (#391): Remove erroneous DST subtraction in ConvertTimestampToUnix for dstmWin on Win7+.
- Private key auth (#392): Passphrase prompt misclassification or path encoding issue suspected; needs diagnostic logging.
- Second file open crash: Dangling TRemoteFile pointers after directory refresh; Duplicate(false) recommended in CreateFileList.
Success signals:
- Zero build warnings under /W4
- Plugin DLL in Far3_<platform>/Plugins/NetBox/
- Manual test protocol: connect, transfer, cancel, navigate for each protocol
- No crashes in 48hr stress test

Next step:
- Prioritize open issues by severity: crash bugs (#497, second-file-open) > data integrity (#391 DST) > auth (#392) > features (silent mode, S3 TLS UI)

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
