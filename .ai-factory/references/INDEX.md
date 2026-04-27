# References Index

Available knowledge references for AI agents.

## Feature Documentation

| Reference | Topic | Audience | Updated |
|-----------|-------|----------|---------|
| [remote-to-remote-copy](remote-to-remote-copy.md) | Remote-to-remote file copy support | End-user + internal | 2026-04-26 |
| [editor-external-modification](editor-external-modification.md) | External file modification detection | End-user + internal | 2026-04-26 |

## Research References

| Reference | Topic | Sources | Updated |
|-----------|-------|---------|---------|
| [message-loading-system](message-loading-system.md) | GetMsg/FmtLoadStr resolution, mapping tables, missing ID bug | FarPluginStrings.cpp, MsgIDs.h, NetBoxEng.lng | 2026-04-27 |
| [about-dialog-version-fix](about-dialog-version-fix.md) | About dialog version substitution bug | NetBoxEng.lng, TextsCore.h, WinSCPDialogs.cpp | 2026-04-27 |
| [sftp-binary-dump-log-protocol](sftp-binary-dump-log-protocol.md) | SFTP binary dump at Log Protocol level 3 | SftpFileSystem.cpp, WinSCPDialogs.cpp, MsgIDs.h | 2026-04-27 |
| [crash-second-file-open-analysis](crash-second-file-open-analysis.md) | Crash when opening files twice without refresh | Code analysis | 2026-04-26 |
| [foundation-stability-research](foundation-stability-research.md) | Foundation stability research | Internal research | 2026-04-22 |

## Implementation Plans

| Plan | Topic | Status | Updated |
|------|-------|--------|---------|
| [fix-crash-second-file-open](fix-crash-second-file-open.md) | Fix crash on second file open without refresh | Planned | 2026-04-26 |
| [silent-mode-research](silent-mode-research.md) | Silent mode research | Internal research | 2026-04-22 |
| [winscp-source-architecture](winscp-source-architecture.md) | WinSCP source code architecture and integration guide | D:\Projects\WinSCP-work\winscp-master\source | 2026-04-22 |
| [winscp-sessiondata-encryption-settings](winscp-sessiondata-encryption-settings.md) | WinSCP encryption field definitions, property macros, and TLS defaults | WinSCP source: SessionData.h, serialization macros | 2026-04-26 |
| [netbox-s3filesystem-session-mapping](netbox-s3filesystem-session-mapping.md) | NetBox S3 session data fields, serialization gaps, and TLS setup | NetBox core/SessionData.h, core/S3FileSystem.cpp | 2026-04-26 |
| [netbox-ui-dialogs-s3-config](netbox-ui-dialogs-s3-config.md) | NetBox S3 configuration UI controls and Load/Save patterns | NetBox WinSCPDialogs.cpp/h, FarDialog | 2026-04-26 |
| [ssh-authentication-exploration](ssh-authentication-exploration.md) | SSH authentication and OpenSSH certificate support | PuTTY internals, WinSCP | 2026-04-26 |
| [session-config-ui-patterns](session-config-ui-patterns.md) | UI patterns for session configuration dialogs | TFarEdit, WinSCPDialogs.cpp | 2026-04-26 |
| [logging-subsystem](logging-subsystem.md) | Logging subsystem architecture | tinylog, nbcore/logging.cpp | 2026-04-26 |

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
