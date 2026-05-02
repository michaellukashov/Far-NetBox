# References Index

Available knowledge references for AI agents.

Features
--------

* **Remote-to-Remote Copy** — Copy files directly between remote directories without downloading them locally ([details](features/remote-to-remote-copy.md))
* **Editor External Modification Detection** — Automatic reload when files are modified externally ([details](features/editor-external-modification.md))

## Research References

| Reference | Topic | Sources | Updated |
|-----------|-------|---------|---------|
| [combo-box-dropdown-keyboard-exploration](combo-box-dropdown-keyboard-exploration.md) | Terminal host interception of Ctrl+Down, Alt+Down fallback, DefaultItemProc approach | FarDialog.cpp, FarPlugin.h, WinSCPDialogs.cpp, Far Manager dialog.cpp | 2026-04-29 |
| [message-loading-system](message-loading-system.md) | GetMsg/FmtLoadStr resolution, mapping tables, missing ID bug | FarPluginStrings.cpp, MsgIDs.h, NetBoxEng.lng | 2026-04-27 |
| [about-dialog-version-fix](about-dialog-version-fix.md) | About dialog version substitution bug | NetBoxEng.lng, TextsCore.h, WinSCPDialogs.cpp | 2026-04-27 |
| [sftp-binary-dump-log-protocol](sftp-binary-dump-log-protocol.md) | SFTP binary dump at Log Protocol level 3 | SftpFileSystem.cpp, WinSCPDialogs.cpp, MsgIDs.h | 2026-04-27 |
| [crash-second-file-open-analysis](crash-second-file-open-analysis.md) | Crash when opening files twice without refresh | Code analysis | 2026-04-26 |
| [foundation-stability-research](foundation-stability-research.md) | Foundation stability research | Internal research | 2026-04-22 |
qt|| [cmake-refactoring-plan-exploration](cmake-refactoring-plan-exploration.md) | CMake modularization plan vs actual codebase reconciliation, missing modules, line-count corrections | CMakeLists.txt, cmake/*.cmake, libs/*/CMakeLists.txt | 2026-04-29 |
| [issue-501-ssh-scp-buffer-corruption-exploration](issue-501-ssh-scp-buffer-corruption-exploration.md) | Root-cause analysis of slow SSH/SCP transfers and file corruption caused by dynamic TCP send buffer resizing | SessionData.cpp, SecureShell.cpp, WinSCPDialogs.cpp | 2026-04-29 |
## Implementation Plans

| Plan | Topic | Status | Updated |
|------|-------|--------|---------|
| [fix-crash-second-file-open](fix-crash-second-file-open.md) | Fix crash on second file open without refresh | Planned | 2026-04-26 |
| [silent-mode-research](silent-mode-research.md) | Silent mode research | Internal research | 2026-04-22 |
| [winscp-source-architecture](winscp-source-architecture.md) | WinSCP source code architecture and integration guide | D:\Projects\WinSCP-work\winscp-master\source | 2026-04-22 |
| [fix-issue-389-pureftpd-tls-explicit](../plans/fix-issue-389-pureftpd-tls-explicit.md) | **FIXED** — Pure‑FTPd explicit TLS: `AUTH TLS`→SSL fallback. Full investigation: OpenSSL `bn_div_words` asm crash, session log debugging, state machine traces (Issue [#389](https://github.com/michaellukashov/Far-NetBox/issues/389)). See also: [exploration](fix-issue-389-pureftpd-tls-explicit.md) | FtpControlSocket.cpp, AsyncSslSocketLayer.cpp, Cryptography.cpp, FtpFileSystem.cpp | 2026-05-02 |
| [winscp-sessiondata-encryption-settings](winscp-sessiondata-encryption-settings.md) | WinSCP encryption field definitions, property macros, and TLS defaults | WinSCP source: SessionData.h, serialization macros | 2026-04-26 |
| [netbox-s3filesystem-session-mapping](netbox-s3filesystem-session-mapping.md) | NetBox S3 session data fields, serialization gaps, and TLS setup | NetBox core/SessionData.h, core/S3FileSystem.cpp | 2026-04-26 |
| [netbox-ui-dialogs-s3-config](netbox-ui-dialogs-s3-config.md) | NetBox S3 configuration UI controls and Load/Save patterns | NetBox WinSCPDialogs.cpp/h, FarDialog | 2026-04-26 |
| [ssh-authentication-exploration](ssh-authentication-exploration.md) | SSH authentication and OpenSSH certificate support | PuTTY internals, WinSCP | 2026-04-26 |
| [session-config-ui-patterns](session-config-ui-patterns.md) | UI patterns for session configuration dialogs | TFarEdit, WinSCPDialogs.cpp | 2026-04-26 |
| [logging-subsystem](logging-subsystem.md) | Logging subsystem architecture | tinylog, nbcore/logging.cpp | 2026-04-26 |
| [far-dialog-api-text-retrieval](far-dialog-api-text-retrieval.md) | Far Manager dialog API for retrieving text from edit controls | DM_GETTEXT, DM_GETDLGITEM, CreateDirectoryDialog | 2026-04-27 |
| [issue-511-speed-limit-esc-hang-exploration](issue-511-speed-limit-esc-hang-exploration.md) | Speed limit ineffective for SSH/SFTP and Esc key hang during transfer | Queue.cpp, FileOperationProgress.cpp, WinSCPFileSystem.cpp, FarPlugin.cpp | 2026-04-29 |
| [issue-511-cancel-yes-hang-deep-dive](issue-511-cancel-yes-hang-deep-dive.md) | Post-dialog hang after "Yes" in cancel dialog, reentrancy guard failure, CheckForEsc input buffer, exception unwinding | WinSCPFileSystem.cpp, FileOperationProgress.cpp, FarPlugin.cpp | 2026-04-29 |
## Internal Documentation (development)

| Document | Purpose | Path |
|----------|---------|------|
| Architecture | Project structure and dependency flow | [docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md) |
| Dependencies | Third-party library inventory | [DEPENDENCIES.md](../DEPENDENCIES.md) |
| Source Organization | Codebase structure and module boundaries | [SOURCE_ORGANIZATION.md](../SOURCE_ORGANIZATION.md) |
| AGENTS Guide | Main AI agent entry point | [AGENTS.md](../../AGENTS.md) |

## User Documentation (end-user)

| Document | Purpose | Path |
|----------|---------|------|
| README | Project overview and links | [../../docs/README.md](../../docs/README.md) |
| Features | Feature guides | [../../docs/features/](../../docs/features/) |
| Internationalization | Translated READMEs | [../../docs/i18n/](../../docs/i18n/) |


## Threading & Concurrency

| Reference | Topic | Sources | Updated |
|-----------|-------|---------|---------|
|| [multithreading-audit-exploration](multithreading-audit-exploration.md) | Comprehensive threading audit: Far API violations, race conditions, busy-waiting, static state | src/core/Queue.cpp, src/NetBox/*.cpp, src/filezilla/*.cpp, src/base/*.cpp | 2026-04-29 |
| [multithreading-review-fix-results](multithreading-review-fix-results.md) | Implementation results of the multithreading review and fix plan: critical Far API fixes, race condition elimination, busy-wait replacement, static state hardening, OpenSSL once-init | src/core/Queue.cpp, src/NetBox/*.cpp, src/filezilla/*.cpp, src/base/*.cpp, src/core/Cryptography.cpp | 2026-04-29 |

## Security

| Reference | Topic | Sources | Updated |
|-----------|-------|---------|---------|
| [cwe134-fmtload-vulnerability-scan](cwe134-fmtload-vulnerability-scan.md) | CWE-134 format string vulnerability scan: 6 vulnerable FMTLOAD call sites with untrusted server data, EscapeFmtChars utility proposal, risk assessment | FtpControlSocket.cpp, FtpFileSystem.cpp, NeonIntf.cpp, ScpFileSystem.cpp, FormatUtils.cpp, TextsCore1.rc | 2026-04-29 |
| [esc-cancellation-comprehensive-fix](esc-cancellation-comprehensive-fix.md) | Four-layer root-cause fix for SCP Esc cancellation, console input buffer, cancel-state semantics, exception conversion, and cleanup hang | ScpFileSystem.cpp, Terminal.cpp, FarPlugin.cpp, WinSCPFileSystem.cpp | 2026-05-01 |
| [fix-issue-389-pureftpd-tls-explicit](fix-issue-389-pureftpd-tls-explicit.md) | **BUG FOUND** — OpenSSL 3 `bn_div_words` division-by-zero in NASM assembly affecting RSA/EC operations during TLS handshake (Issue [#389](https://github.com/michaellukashov/Far-NetBox/issues/389)). **Not a NetBox bug.** Permanent fix: rebuild OpenSSL with `no-asm` configure flag or update NASM to compatible version. See also: [plan](../plans/fix-issue-389-pureftpd-tls-explicit.md), [ARCHITECTURE.md](../ARCHITECTURE.md) | bn_asm.c, bn_div.c, ecp_mont.c, rsa_sig.c, statem_clnt.c, FtpControlSocket.cpp, AsyncSslSocketLayer.cpp, Cryptography.cpp | 2026-05-02 |