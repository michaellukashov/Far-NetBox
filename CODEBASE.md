# NetBox Codebase Map

## Documentation Index

This directory contains comprehensive documentation about the NetBox codebase structure, architecture, and implementation details.

### Documents

| Document | Description |
|----------|-------------|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | High-level architecture overview, build system, component relationships |
| [SOURCE_ORGANIZATION.md](./SOURCE_ORGANIZATION.md) | Source code organization, directory structure, module responsibilities |
| [DEPENDENCIES.md](./DEPENDENCIES.md) | Third-party libraries, their purposes, and integration details |
| [PLUGIN_STRUCTURE.md](./PLUGIN_STRUCTURE.md) | Far Manager plugin implementation, entry points, and lifecycle |
| [PROTOCOL_IMPLEMENTATIONS.md](./PROTOCOL_IMPLEMENTATIONS.md) | Protocol implementations (SFTP, FTP, SCP, WebDAV, S3) |

## Quick Reference

### Project Summary
- **Name**: NetBox (Far-NetBox)
- **Type**: Far Manager 3.x Plugin
- **Purpose**: SFTP/FTP/SCP/WebDAV/S3 client
- **Based On**: WinSCP 6.5.1, PuTTY 0.81, FileZilla 2.2.32
- **Language**: C++17
- **Build System**: CMake
- **Platforms**: Windows (x86, x64, ARM64)

### Directory Structure

```
Far-NetBox/
├── src/                    # Source code
│   ├── base/              # Foundation classes
│   ├── core/              # Protocol implementations
│   ├── filezilla/         # FTP engine
│   ├── include/           # Public headers
│   ├── nbcore/            # Core utilities
│   ├── NetBox/            # Plugin implementation
│   ├── PluginSDK/         # Far Manager SDK
│   ├── resource/          # Resources
│   └── windows/           # Windows-specific code
├── libs/                   # Third-party libraries
├── cmake/                  # CMake configuration
├── .planning/              # Planning and documentation
└── .planning/codebase/     # This documentation
```

### Key Components

**Protocols**:
- SFTP (via PuTTY)
- FTP/FTPS (via FileZilla)
- SCP (via PuTTY)
- WebDAV (via neon)
- S3 (via libs3)

**Core Libraries**:
- OpenSSL 3 (cryptography)
- tinyxml2 (XML parsing)
- fmt (string formatting)
- tinylog (logging)
- GSL (C++ guidelines)

### Build Commands

**Quick Start**:
```cmd
call "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

**Platform Options**:
- `PROJECT_PLATFORM`: x86, x64, ARM64
- `CMAKE_BUILD_TYPE`: Debug, Release, RelWithDebugInfo
- `OPT_CREATE_PLUGIN_DIR`: ON (creates plugin directory structure)

### Code Style

- **C++ Standard**: C++17
- **Brace Style**: Allman/BSD
- **Indentation**: 2 spaces
- **Naming**: PascalCase with `T` prefix for classes
- **Members**: `F` prefix for fields

## Navigation Guide

### For New Developers
1. Start with [ARCHITECTURE.md](./ARCHITECTURE.md) for overview
2. Read [SOURCE_ORGANIZATION.md](./SOURCE_ORGANIZATION.md) for code layout
3. Review [DEPENDENCIES.md](./DEPENDENCIES.md) for third-party libraries
4. Study [PLUGIN_STRUCTURE.md](./PLUGIN_STRUCTURE.md) for plugin integration
5. Examine [PROTOCOL_IMPLEMENTATIONS.md](./PROTOCOL_IMPLEMENTATIONS.md) for protocol details

### For Protocol Developers
1. Review [PROTOCOL_IMPLEMENTATIONS.md](./PROTOCOL_IMPLEMENTATIONS.md)
2. Examine `src/core/Terminal.h` for base interface
3. Study existing protocol implementations in `src/core/`
4. Understand [DEPENDENCIES.md](./DEPENDENCIES.md) for library integration

### For Plugin Developers
1. Read [PLUGIN_STRUCTURE.md](./PLUGIN_STRUCTURE.md)
2. Examine `src/NetBox/` directory
3. Study Far Manager SDK in `src/PluginSDK/`
4. Review [ARCHITECTURE.md](./ARCHITECTURE.md) for component relationships

### For Build Engineers
1. Review [ARCHITECTURE.md](./ARCHITECTURE.md) build system section
2. Examine `CMakeLists.txt` and `cmake/` directory
3. Study [DEPENDENCIES.md](./DEPENDENCIES.md) for library configuration
4. Check AGENTS.md for build instructions

## File Naming Patterns

| Pattern | Example | Description |
|---------|---------|-------------|
| `<Class>.h` | `SessionData.h` | Header file |
| `<Class>.cpp` | `SessionData.cpp` | Implementation |
| `<Module>.hpp` | `UnicodeString.hpp` | Alternative header |
| `<Lib>.cmake` | `PuTTY.cmake` | CMake configuration |
| `<Lang>.lng` | `NetBoxEng.lng` | Language file |

## Key Files Reference

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main build configuration |
| `src/NetBox/NetBox.cpp` | Plugin entry point |
| `src/core/Terminal.h` | Base filesystem interface |
| `src/core/SessionData.h` | Session configuration |
| `src/base/UnicodeString.hpp` | String handling |
| `src/include/nbtypes.h` | Type definitions |

## Contact & Resources

- **Repository**: https://github.com/michaellukashov/Far-NetBox
- **Far Manager**: https://www.farmanager.com/
- **Documentation**: See individual protocol and component documentation

## Version Information

This documentation reflects the codebase as of the current working directory state. For version-specific information, check the git history or release tags.

---

*Generated for NetBox codebase mapping - 2026-02-27*
