# Far-NetBox

## What This Is

Far-NetBox is an SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager 3.0 that provides reliable file transfer capabilities across multiple protocols. It's built on proven technologies including WinSCP, PuTTY, and FileZilla, offering system administrators, developers, and power users a comprehensive file management solution integrated within the Far Manager ecosystem.

## Core Value

Reliable file transfers across all supported protocols with consistent, error-free performance.

## Requirements

### Validated

- ✓ SFTP file transfers — existing
- ✓ FTP file transfers — existing  
- ✓ SCP file transfers — existing
- ✓ WebDAV file transfers — existing
- ✓ S3 file transfers — existing
- ✓ Far Manager integration — existing
- ✓ Multi-platform support (x86/x64/ARM64) — existing

### Active

- [ ] Bug fixes and stability improvements
- [ ] Performance optimization
- [ ] Codebase modernization
- [ ] UI/UX improvements
- [ ] Maintain WinXP compatibility

### Out of Scope

- Major architectural rewrites — preserves existing codebase structure
- New protocol support (in v1) — focus on stability first
- Cross-platform UI redesign — maintains current Far Manager integration

## Context

This is a mature C++ codebase built on proven file transfer technologies:
- Based on WinSCP version 6.5.6
- SSH/SCP code based on PuTTY 0.81
- FTP code based on FileZilla 2.2.32
- Extensive protocol library integrations (OpenSSL, zlib, etc.)
- Established Far Manager plugin architecture
- Multi-architecture support (x86/x64/ARM64)

## Constraints

- **Compatibility**: Must remain compatible with Far Manager ecosystem
- **Platform**: Windows-focused development with WinXP compatibility
- **Architecture**: Preserve existing C++ codebase structure
- **Performance**: Maintain or improve current performance levels
- **Backward Compatibility**: Support existing configurations and workflows

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Incremental improvements approach | Minimize risk while improving existing functionality | — Pending |
| Focus on reliability first | Addresses core value of consistent file transfers | — Pending |
| Preserve WinXP compatibility | Maintains user base and backward compatibility | — Pending |

---
*Last updated: 2026-04-13*