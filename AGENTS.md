# AGENTS.md - NetBox Development Guide

This document provides guidelines for AI agents working on the NetBox project (Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager).

## Build Commands
```

**Manual CMake build:**
```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build
```

### Build Options

- `PROJECT_PLATFORM`: `x86`, `x64`, or `ARM64` (defaults to x86)
- `CMAKE_BUILD_TYPE`: `Debug`, `Release`, or `RelWithDebugInfo`
- `OPT_CREATE_PLUGIN_DIR`: Create plugin directory structure (`ON`/`OFF`)
- `OPT_USE_UNITY_BUILD`: Enable unity builds for faster compilation (x86 Release only)

### Code Formatting

### Environment Setup

```cmd
"%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
```

## Code Style Guidelines

### Compiler Requirements

- **C++ Standard**: C++17 (required, no extensions)
- **Compiler**: Visual Studio 2022 (MSVC)
- **Build System**: CMake 3.15 or later

### Formatting

- **Brace style**: Allman/BSD (opening brace on new line)
- **Indentation**: 2 spaces (not tabs)
- **Line endings**: Windows CRLF
- **Pointer alignment**: Middle (`int * ptr`)
- **Reference alignment**: Middle (`int & ref`)
- **Max line length**: 120 characters

### Naming Conventions

- **Classes**: PascalCase with `T` prefix (`TSessionData`, `TSecureShell`)
- **Functions/Methods**: PascalCase (`GetSessionData()`, `ConnectToServer()`)
- **Member variables**: PascalCase with `F` prefix (`FSessionData`, `FConfig`)
- **Local variables**: camelCase (`sessionData`, `configValue`)
- **Constants/Macros**: UPPER_CASE (`MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`)
- **Enums**: PascalCase (`SessionType`, `AuthMethod`)

### Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Mark TODOs: `// TODO: description`
- Mark FIXMEs: `// FIXME: description`

### File Organization

**Include order in headers:**
1. System headers (`<...>`)
2. Project headers (`"..."`)
3. Local headers

**Include guards:** Use `#pragma once`

**File extensions:**
- Headers: `.h` or `.hpp`
- Sources: `.cpp`
- Resources: `.rc`, `.lng`

## Error Handling

- Use exceptions for error conditions
- Provide meaningful error messages
- Log errors appropriately with `ADF()` macro
- Use `DebugAssert()` for invariants
- Handle network errors gracefully

## Memory Management

- Prefer RAII and smart pointers
- Avoid raw `new`/`delete` where possible
- Check for null pointers before dereferencing
- Use `nbstr_*` functions for string memory management

## String Handling

- Use `UnicodeString` for wide strings (user-facing text)
- Use `AnsiString` for narrow strings when needed
- Handle encoding correctly
- Use UTF-8 for file I/O when possible

## Dependencies

Third-party libraries are in `libs/`:
- PuTTY (`libs/putty/`) - SSH/SCP
- FileZilla (in `src/filezilla/`) - FTP
- OpenSSL 3 (`libs/openssl-3/`) - Cryptography
- neon (`libs/neon/`) - WebDAV
- libs3 (`libs/libs3/`) - S3
- expat, zlib-ng, tinyxml2, fmt, tinylog

**Do not modify third-party library code directly.** Use patches if needed.

## Project Structure

```
src/
├── base/          # Base classes and utilities
├── core/          # Core functionality (SSH, FTP, etc.)
├── filezilla/     # FileZilla-based FTP implementation
├── include/       # Public headers
├── nbcore/        # NetBox core utilities
├── NetBox/        # Plugin implementation
├── PluginSDK/     # Far Manager plugin SDK
├── resource/      # Resources (RC, LNG)
└── windows/       # Windows-specific code
```

## Testing

There are no automated unit tests. Manual testing is required:
1. Build compiles without warnings
2. Plugin loads correctly in Far Manager
3. Test SFTP/FTP/SCP/WebDAV/S3 connections
4. Test file operations (upload, download, delete, rename)
5. Test authentication methods

## Language Files

- English: `NetBoxEng.lng` (primary)
- Other languages: `NetBoxRus.lng`, `NetBoxPol.lng`, etc.
- Use message IDs from `MsgIDs.h`
- Keep translations synchronized when modifying UI strings

## Build Output

- Plugin location: `Far3_<platform>/Plugins/NetBox/`
- Platforms: `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/`

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Clear, descriptive, 50 chars or less for summary
- **Skip CI**: Include `[skip appveyor]` in commit message

### CI/CD
- **AppVeyor**: Automated builds on Windows
- **GitHub Actions**: Release workflows

## Code Quality Requirements

- No compiler warnings
- No memory leaks (use RAII)
- Exception safety
- Unicode correctness
- Thread safety documentation
- Protocol specification compliance

## Special Considerations

- This project is based on WinSCP, PuTTY, and FileZilla codebases
- Maintain compatibility with original design patterns
- Support Windows XP+ (`_WIN32_WINNT=0x0501`)
- Test on all target platforms (x86, x64, ARM64)
