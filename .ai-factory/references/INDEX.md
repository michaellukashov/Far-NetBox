# References Index

Available knowledge references for AI agents.

Features
--------

* **Remote-to-Remote Copy** — Copy files directly between remote directories without downloading them locally ([details](features/remote-to-remote-copy.md))
* **Editor External Modification Detection** — Automatic reload when files are modified externally ([details](features/editor-external-modification.md))

## Research References

| Reference | Topic | Sources | Updated |
|---|---|---|---|
| [combo-box-dropdown-keyboard-exploration](combo-box-dropdown-keyboard-exploration.md) | Terminal host interception of Ctrl+Down, Alt+Down fallback, DefaultItemProc approach | FarDialog.cpp, FarPlugin.h, WinSCPDialogs.cpp, Far Manager dialog.cpp | 2026-04-29 |
| [message-loading-system](message-loading-system.md) | GetMsg/FmtLoadStr resolution, mapping tables, missing ID bug | FarPluginStrings.cpp, MsgIDs.h, NetBoxEng.lng | 2026-04-27 |
| [about-dialog-version-fix](about-dialog-version-fix.md) | About dialog version substitution bug | NetBoxEng.lng, TextsCore.h, WinSCPDialogs.cpp | 2026-04-27 |
| [sftp-binary-dump-log-protocol](sftp-binary-dump-log-protocol.md) | SFTP binary dump at Log Protocol level 3 | SftpFileSystem.cpp, WinSCPDialogs.cpp, MsgIDs.h | 2026-04-27 |
| [crash-second-file-open-analysis](crash-second-file-open-analysis.md) | Crash when opening files twice without refresh | Code analysis | 2026-04-26 |
| [foundation-stability-research](foundation-stability-research.md) | Foundation stability research | Internal research | 2026-04-22 |
| [issue-392-private-key-auth-exploration](issue-392-private-key-auth-exploration.md) | SSH private key auth failure: key file loading, passphrase prompt classification, PuTTY auth flow | SecureShell.cpp, SessionData.cpp, PuttyIntf.cpp, userauth2-client.c | 2026-05-02 |
| [fix-issue-388-rsa-sha256-support](../plans/fix-issue-388-rsa-sha256-support.md) | **FIXED** — RSA-SHA2 host key cache fallback: when `rsa-sha2-256`/`rsa-sha2-512` lookup fails, retry with `ssh-rsa`. Also fixes `HaveHostKey` session-configured key comparison for RSA-SHA2 equivalence. (Issue [#388](https://github.com/michaellukashov/Far-NetBox/issues/388)) | SecureShell.cpp | 2026-05-02 |
| [issue-501-ssh-scp-buffer-corruption-exploration](issue-501-ssh-scp-buffer-corruption-exploration.md) | Root-cause analysis of slow SSH/SCP transfers and file corruption caused by dynamic TCP send buffer resizing | SessionData.cpp, SecureShell.cpp, WinSCPDialogs.cpp | 2026-04-29 |
| [issue-391-dst-timestamp-exploration](issue-391-dst-timestamp-exploration.md) | **BUG FOUND** — Daylight Saving Time bug: `ConvertTimestampToUnix()` subtracts DST offset for `dstmWin` on Win7+ where FILETIME is already pure UTC, causing SCP upload timestamps to be 1 hour behind during DST (Issue [#391](https://github.com/michaellukashov/Far-NetBox/issues/391)). **Fix:** remove erroneous DST subtraction in `!UsesDaylightHack()` branch. See also: [plan](../plans/issue-391-daylight-saving-time-dst-bug.md) | `src/base/Common.cpp` (`ConvertTimestampToUnix`, `UnixToDateTime`, `DateTimeToFileTime`), `src/core/Terminal.cpp` (`OpenLocalFile`, `GetLocalFileTime`), `src/core/ScpFileSystem.cpp` (`SCPSource`), `src/NetBox/WinSCPFileSystem.cpp` (editor upload) | 2026-05-02 |
| [openssh-certificate-auth-exploration](openssh-certificate-auth-exploration.md) | OpenSSH certificate auth: PuTTY cert validation, runtime OpenSSH→PPK conversion, passphrase encryption with effective key file, WinSCP master alignment | SecureShell.cpp, SessionData.cpp, PuttyIntf.cpp, ssh-authentication-exploration.md | 2026-05-03 |
|| [fix-issue-389-tlscertificatefile-dialog-control](../plans/fix-issue-389-tlscertificatefile-dialog-control.md) | **IMPLEMENTED** — Added TLS client certificate file UI control (label + edit + browse button) to the session dialog for FTPS and WebDAV protocols. Resolves stale `.ppk` paths in `TlsCertificateFile` causing OpenSSL initialization failures. (Issue [#389](https://github.com/michaellukashov/Far-NetBox/issues/389)) | `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/*.lng` | 2026-05-04 |
| [winscp-certificate-editor-patterns](winscp-certificate-editor-patterns.md) | WinSCP TLS certificate editor patterns: single TlsCertificateFile property, TFilenameEdit browse control, VerifyCertificate/CheckCertificate validation flow. Analysis of SiteAdvanced.dfm/cpp, SessionData, Terminal.cpp for NetBox alignment of the certificate editor UI. | `forms/SiteAdvanced.dfm`, `forms/SiteAdvanced.cpp`, `core/SessionData.h`, `core/Terminal.cpp`, `windows/Tools.cpp` (WinSCP) | 2026-05-04 |
| [winscp-netbox-gap-analysis](winscp-netbox-gap-analysis.md) | Comprehensive WinSCP vs NetBox feature gap analysis: 12 actionable gaps, 10 already-implemented features, GUI-only exclusions, source structure mapping | `core/Terminal.cpp`, `forms/*.cpp`, `windows/*.cpp` (WinSCP); `src/core/Terminal.cpp`, `src/NetBox/WinSCPDialogs.cpp`, `src/core/Queue.cpp` (NetBox) | 2026-05-04 |
|| [master-password-infrastructure-research](master-password-infrastructure-research.md) | Master password infrastructure: WinSCP vs NetBox RecryptPasswords gap, exception safety, thread safety, rate limiting, i18n. Full source cross-reference and dependency graph. | `windows/WinConfiguration.cpp`, `windows/MasterPassword.cpp`, `core/Terminal.cpp`, `core/SessionData.cpp`, `NetBox/WinSCPDialogs.cpp` | 2026-05-05 |
|| [silent-mode-ui-and-refinements-exploration](silent-mode-ui-and-refinements-exploration.md) | Silent mode UI and refinements: add Preferences checkbox, change boAll→boOlder, suppress all plugin-layer confirmations, write error reports to .errors file. Complete implementation audit of existing silent mode code. | `Terminal.cpp`, `WinSCPDialogs.cpp`, `WinSCPFileSystem.cpp`, `FileOperationProgress.h/cpp`, `MsgIDs.h`, `.lng` files | 2026-05-08 |
