# Fast Plan: Add Comprehensive Documentation for New Features

## Settings
- **Mode**: Fast
- **Testing**: No
- **Logging**: Verbose
- **Docs**: Yes (this IS the docs work)
- **Milestone**: Technical Debt / Refactoring

## Roadmap Linkage
- **Milestone**: "Technical Debt / Refactoring"
- **Rationale**: User-facing documentation is essential for adoption of 2026-04 features.

## Context
The 2026-04 release cycle delivered ~15 features/fixes. `docs/user-guide.md` currently documents only **Folder History Navigation**. Multiple user-facing capabilities lack documentation:

| Feature | User Impact | Current Doc Status |
|---------|-------------|-------------------|
| FTP heartbeat / NOOP keep-alive | Connection stability | ❌ Missing |
| CPS speed limit + Esc cancellation | Transfer control, UI hang fix | ❌ Missing |
| SSH/SCP buffer settings (`SendBuf`) | Performance tuning | ❌ Missing |
| S3 TLS version + custom CA cert | Security configuration | Partial (protocol table only) |
| Combo box keyboard shortcuts | Daily UX (Alt+Down/Ctrl+Down) | ❌ Missing |
| WebDAV overwrite/refresh fix | File operation reliability | ❌ Missing |
| SFTP remote folder creation | File operations | ❌ Missing |
| FTP port preservation | Session configuration | ❌ Missing |
| DateTime format functions | Scripting/automation | ❌ Missing |

## Tasks

### Task 1: Expand user-guide.md with Feature Reference sections
- **Files**: `docs/user-guide.md`
- **Action**: Add documented sections for user-facing 2026-04 features
- **Sections to add**:
  1. **Connection Keep-Alive** — FTP NOOP heartbeat interval, SSH keep-alive via PuTTY settings
  2. **Speed Limits** — CPS limit per session, global limit propagation, Esc-cancel behavior
  3. **Security Settings** — S3 TLS version selection, custom CA certificate path, per-session overrides
  4. **Keyboard Shortcuts** — Alt+Down / Ctrl+Down to open combo box dropdowns
  5. **Session Configuration** — Non-default FTP ports, SSH send buffer (`SendBuf=0` default), SCP simplicity mode (`SshSimple`)
  6. **File Operations** — WebDAV overwrite/refresh reliability, SFTP remote folder creation
  7. **Scripting & Automation** — `FormatDateTime` token reference, `ISO8601ToDate` parsing
- **Requirements**: Use existing document style (markdown tables, code blocks for examples, cross-links to Getting Started)
- **Note**: Keep Folder History Navigation section as-is; add new sections after it

### Task 2: Update getting-started.md with new build/prerequisite notes
- **Files**: `docs/getting-started.md`
- **Action**: Add notes for any new prerequisites or build behaviors introduced in 2026-04
- **Checklist**:
  - Verify NASM prerequisite is still accurate (OpenSSL 3.3.7)
  - Note WinXP compatibility status if changed
  - Add "Troubleshooting" subsection for common 2026-04 issues (Esc hang, buffer corruption)

### Task 3: Mark ROADMAP.md documentation item complete
- **Files**: `.ai-factory/ROADMAP.md`
- **Action**: Mark `Add comprehensive documentation for all new features` as complete (date: 2026-05-04)
- **Reference**: Link to updated `docs/user-guide.md`

## Commit Plan
- Single commit after Task 1 + Task 2 + Task 3
- Commit message: `docs(user-guide): document 2026-04 features`

## Next Step
Run `/aif-implement` to execute this plan.
