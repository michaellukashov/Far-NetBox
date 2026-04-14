# Project: Far-NetBox

## Overview
Far-NetBox is a Far Manager 3.0 plugin providing SFTP/FTP/SCP/WebDAV/S3 client capabilities. Built on proven codebases (WinSCP 6.5.1, PuTTY 0.81, FileZilla 2.2.32), it delivers reliable file transfer across multiple protocols integrated within the Far Manager ecosystem.

## Core Features
- SFTP file transfers (via PuTTY SSH library)
- FTP/FTPS file transfers (via FileZilla engine)
- SCP file transfers (via PuTTY SSH library)
- WebDAV/HTTPS file transfers (via neon library)
- Amazon S3 compatible storage (via libs3)
- Far Manager 3.0 integration (x86/x64/ARM64)
- Session configuration with XML storage
- Multi-language support (English, Russian, Polish)

## Tech Stack
- **Language:** C++17 (no extensions)
- **Compiler:** MSVC (Visual Studio 2022)
- **Build System:** CMake 3.15+ / Ninja
- **Platforms:** Windows (x86, x64, ARM64)
- **Database:** N/A (file-based XML configuration)
- **ORM:** N/A
- **Integrations:** Far Manager 3.0 Plugin API

## Key Dependencies
| Library | Purpose |
|---------|---------|
| PuTTY 0.81 | SSH/SCP/SFTP protocol |
| FileZilla 2.2.32 | FTP/FTPS protocol |
| OpenSSL 3 | Cryptography/SSL/TLS |
| neon | WebDAV/HTTP protocol |
| libs3 | Amazon S3 protocol |
| tinyxml2 | XML parsing |
| fmt | String formatting |
| tinylog | Logging framework |
| GSL | C++ Core Guidelines utilities |
| zlib-ng | Compression |

## Architecture Notes
See [.ai-factory/ARCHITECTURE.md](./ARCHITECTURE.md) for detailed architecture guidelines.
- Pattern: Plugin Architecture (Layered)
- Layered architecture: Plugin Layer → Core Layer → Base Layer → Third-Party Libraries
- All protocol implementations inherit from `TCustomFileSystem` interface
- Factory pattern for protocol-specific filesystem creation
- RAII pattern for resource management
- Exception-based error handling with debug logging via `ADF()` macro
- Unity builds enabled for x86 Release builds (faster compilation)

## Non-Functional Requirements
- **Logging:** Configurable via tinylog integration, debug output via `ADF()` macro
- **Error handling:** Exception-based with structured error responses
- **Security:** OpenSSL 3 for cryptography, regular security updates for PuTTY/OpenSSL
- **Performance:** Maintain or improve current performance levels
- **Compatibility:** Far Manager ecosystem compatibility, WinXP support

## Constraints
- Must preserve existing codebase structure (incremental improvements)
- No modifications to third-party code in `libs/` (use patches)
- Windows-focused development with WinXP compatibility
- Multi-architecture support (x86/x64/ARM64)
- Backward compatibility with existing configurations

## Active Development Focus
- Bug fixes and stability improvements
- Performance optimization
- Codebase modernization (C++17 features adoption)
- UI/UX improvements
- Maintain WinXP compatibility

## Out of Scope
- Major architectural rewrites
- New protocol support (in v1)
- Cross-platform UI redesign
