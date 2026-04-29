# Project Roadmap

## Milestones

- **Version 1.0** – Core functionality stable
- **Version 1.1** – ✅ Remote‑to‑Remote copy for all protocols (SFTP, WebDAV, S3, SCP) — **COMPLETE**
  - Status: Already implemented (2011-2023)
  - SCP: Uses `cp -r` command (added 2011-2014)
  - SFTP: Uses SFTP protocol extensions (added 2023-06-11)
  - WebDAV: Uses HTTP COPY method (added 2017-03-19)
  - S3: Uses S3 CopyObject API (added 2018-01-19)
- **Version 1.2** – Background copy & progress UI
- **Version 1.3** – Win32/KiTTY input mode, WinXP compatible builds
- **Version 2.0** – Full plugin refactor, modular architecture

## Upcoming Features

- DateTime format functions (custom tokens, locale support)
- FTP heartbeat / NOOP keep‑alive
- Inactive session crash fix (WinSCP #2426)
- Static RTTI improvements
- **Issue #511** – Fix parallel transfer CPS limit propagation + Esc hang during cancel dialog (2026-04-29)

## Technical Debt / Refactoring

- Consolidate `TODO` items into tracked tasks (`/aif-implement`)
- Review and improve static RTTI usage
- Clean up OpenSSL sync scripts
- Add comprehensive documentation for all new features

## Process

- Each milestone is tracked via a feature plan in `.ai-factory/plans/`.
- When a plan is completed, update the milestone status in this file.
- Use `/aif-commit` for conventional commits after each checkpoint.

