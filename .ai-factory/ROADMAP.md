# Project Roadmap

## Milestones

- **Version 1.0** – Core functionality stable
- **Version 1.1** – ✅ Remote-to-Remote copy for all protocols (SFTP, WebDAV, S3, SCP) — **COMPLETE**
  - Status: Already implemented (2011-2023)
  - SCP: Uses `cp -r` command (added 2011-2014)
  - SFTP: Uses SFTP protocol extensions (added 2023-06-11)
  - WebDAV: Uses HTTP COPY method (added 2017-03-19)
  - S3: Uses S3 CopyObject API (added 2018-01-19)
- **Version 1.2** – Background copy & progress UI
- **Version 1.3** – Win32/KiTTY input mode, WinXP compatible builds
- **Version 2.0** – Full plugin refactor, modular architecture

## Completed Features

| Feature | Date | Notes |
|---------|------|-------|
| FTP heartbeat / NOOP keep-alive | 2026-04 | Prevents server-side timeout on FTP control connections |
|| Issue #511 — CPS limit + Esc hang | 2026-04 | Speed limit propagation and cancel dialog hang fixes |
|| Issue #501 — SSH/SCP buffer corruption | 2026-04 | Disable dynamic send buffer by default (SendBuf=0, SshSimple=false) |
| DateTime format functions | 2026-04 | `FormatDateTime` token parsing, `ISO8601ToDate` parser |
| WinSCP #2426 — Inactive session fix | 2026-04 | External file modification detection for inactive sessions |
| Static RTTI replacement | 2026-04 | Replaced `dynamic_cast` with static type checks |
| Folder history navigation | 2026-04 | Far Manager folder history integration via `FarPanelDirectory::Param` |
| FTP port preservation | 2026-04 | Fix non-default FTP port handling |
| FTP vsftpd directory listing | 2026-04 | Defensive logging for misclassified files (#507) |
| SFTP remote folder creation | 2026-04 | Fix cannot-create-remote-folder scenarios |
| Logging thread safety | 2026-04 | Safe multi-threaded logging |
| S3 TLS version + CA cert | 2026-04 | Per-session TLS and CA certificate controls |
| Combo box keyboard shortcuts | 2026-04 | Alt+Down / Ctrl+Down dropdown open |
| WebDAV panel refresh | 2026-04 | Fix overwrite and refresh issues |
| OpenSSL sync cleanup | 2026-04 | Synced with WinSCP 6.5.6 baseline, removed 300 unused files |

- Background copy & progress UI (Version 1.2)
- KiTTY keyboard protocol full implementation
- OpenSSH certificate authentication
- S3 bucket access fix
- Local background copy optimization
- Editor external modification handling improvements
- WinSCP feature alignment — see `.ai-factory/plans/winscp-feature-alignment-roadmap.md`
  - Phase 1: Dialog UX & upstream bug fix porting
  - Phase 2: Productivity features (Generate URL, KeyGen, Cleanup, FS Info)
  - Phase 3: Workspace save/restore and location profiles
  - Phase 4: Directory comparison and advanced sync
  - Phase 5: Security hardening (master password)
  - Phase 6: Scripting engine (deferred to v2.0)
## Technical Debt / Refactoring

[x] Consolidate `TODO` items into tracked tasks — **COMPLETE** (2026-05-04)
  See `.ai-factory/TODO-INVENTORY.md` and `.ai-factory/plans/implement-todos.md`
- [x] Review and improve static RTTI usage — **COMPLETE** (2026-04)
- [x] Clean up OpenSSL sync scripts — **COMPLETE** (2026-04-17)
[x] Add comprehensive documentation for all new features — **COMPLETE** (2026-05-04)
  See `docs/user-guide.md` and `docs/getting-started.md`
[x] Refactor monolithic CMakeLists.txt (see `REFACTORING_PLAN.md`) — **COMPLETE** (2026-05-04)

## Process

- Each milestone is tracked via a feature plan in `.ai-factory/plans/`.
- When a plan is completed, update the milestone status in this file.
- Use `/aif-commit` for conventional commits after each checkpoint.

