# NetBox Project Rules and Guidelines

This document outlines the rules, conventions, and guidelines for contributing to the NetBox project (Far-NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager).

## Table of Contents

1. [General Principles](#general-principles)
2. [Code Style and Formatting](#code-style-and-formatting)
3. [File Organization](#file-organization)
4. [Naming Conventions](#naming-conventions)
5. [Build System](#build-system)
6. [Dependencies and Libraries](#dependencies-and-libraries)
7. [Version Control](#version-control)
8. [Testing and Quality](#testing-and-quality)
9. [Documentation](#documentation)
10. [License and Copyright](#license-and-copyright)

## General Principles

### Project Overview

- **Name**: Far-NetBox (NetBox)
- **Purpose**: SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager 3.0
- **Platforms**: Windows (x86, x64, ARM64)
- **License**: GNU General Public License v3 (GPL-3.0)

### Codebase Heritage

This project is based on:

- **WinSCP** version 6.5.6 (Copyright (c) 2000-2025 Martin Prikryl)
- **PuTTY** 0.81 (Copyright (c) 1997-2024 Simon Tatham) - for SSH/SCP
- **FileZilla** 2.2.32 (Copyright (c) 2001-2007 Tim Kosse) - for FTP

When modifying code from these sources, maintain compatibility and respect original design patterns.

### Core Requirements

- **C++ Standard**: C++17 (required, no extensions)
- **Compiler**: Visual Studio 2022 (MSVC)
- **Build System**: CMake 3.15 or later
- **Unicode**: Full Unicode support required (`_UNICODE`, `UNICODE` defined)

## Code Style and Formatting

### Formatting Rules

- **Style**: Allman/BSD style braces
- **Indentation**: 2 spaces (not tabs)
- **Line Endings**: Windows (CRLF)
- **Pointer Alignment**: Middle (`int * ptr`, not `int* ptr` or `int *ptr`)
- **Reference Alignment**: Middle (`int & ref`, not `int& ref` or `int &ref`)

### Astyle Configuration

Use the provided astyle configuration:

```bash
scripts/run_astyle.bat [files]
```

Astyle settings:

- `--style=allman`
- `--indent=spaces=2`
- `--convert-tabs`
- `--align-pointer=middle`
- `--align-reference=middle`
- `--mode=c`
- `--keep-one-line-statements`
- `--keep-one-line-blocks`
- `--indent-switches`
- `--suffix=none`
- `--lineend=windows`
- `--pad-oper`

### Code Structure

#### Header Files

- Use `#pragma once` for include guards
- Include order:
  1. System headers (`<...>`)
  2. Project headers (`"..."`)
  3. Local headers

#### Source Files

- Include `vcl.h` and `#pragma hdrstop` for compatibility
- Use Borland C++ compatibility blocks where needed:

```cpp
  #if defined(__BORLANDC__)
  // Borland-specific code
  #endif
```

#### Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Mark TODOs with `// TODO: description`
- Mark FIXMEs with `// FIXME: description`
- Use `// Note:` for important implementation notes

### Compiler Flags and Defines

#### Required Defines

- `NOMINMAX` - Prevent Windows.h from defining min/max macros
- `MPEXT` - NetBox extensions
- `WINSCP` - WinSCP compatibility
- `FARPLUGIN` - Far Manager plugin
- `_UNICODE`, `UNICODE` - Unicode support
- `_WIN32_WINNT=0x0501` - Windows XP+ compatibility
- `_LIB` - Static library
- `_WINDOWS`, `WIN32` - Windows platform

#### Warning Suppressions

- `_SCL_SECURE_NO_WARNINGS` - Suppress secure CRT warnings
- `_CRT_SECURE_NO_WARNINGS` - Suppress secure CRT warnings
- `_WINSOCK_DEPRECATED_NO_WARNINGS` - Suppress Winsock deprecation
- `_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING` - Suppress iterator warnings
- `_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS` - Suppress MS extension warnings
- `_DISABLE_VECTOR_ANNOTATION` - Disable vector annotations

#### Platform-Specific Defines

- **x64**: `WIN64`, `_WIN64`, `_AMD64_`
- **x86**: (no additional defines)
- **ARM64**: (handled by CMake)

## File Organization

### Directory Structure

```txt
src/
├── base/          # Base classes and utilities
├── core/          # Core functionality (SSH, FTP, etc.)
├── filezilla/     # FileZilla-based FTP implementation
├── include/       # Public headers
├── nbcore/        # NetBox core utilities
├── NetBox/        # NetBox plugin implementation
├── PluginSDK/     # Far Manager plugin SDK
├── resource/      # Resources (RC files, strings)
└── windows/       # Windows-specific code
```

### File Naming

- **Headers**: `.h` or `.hpp` (use `.hpp` for C++ headers)
- **Sources**: `.cpp`
- **Resources**: `.rc`, `.lng` (language files)
- Use PascalCase for class files: `SessionData.h`, `SecureShell.cpp`
- Use lowercase for utility files: `common.h`, `utils.cpp`

### Module Organization

- Each major feature should be in its own module/directory
- Keep related files together (header and source)
- Separate interface from implementation

## Naming Conventions

### Classes

- **PascalCase**: `TSessionData`, `TSecureShell`, `TFarDialog`
- Prefix with `T` for types/classes (Delphi-style convention from WinSCP heritage)

### Functions and Methods

- **PascalCase**: `GetSessionData()`, `ConnectToServer()`
- Use descriptive, verb-based names

### Variables

- **PascalCase** for member variables: `FSessionData`, `FConfiguration`
- Prefix with `F` for fields (Delphi-style convention)
- **camelCase** for local variables: `sessionData`, `configValue`
- **UPPER_CASE** for constants: `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`

### Types and Enums

- **PascalCase**: `SessionType`, `AuthMethod`
- Use `enum class` for type-safe enums in C++11+

### Macros

- **UPPER_CASE**: `SET_SESSION_PROPERTY`, `NETBOX_DEFS`
- Use descriptive names, avoid common names

### Properties

- Use getter/setter pattern:

  ```cpp
  bool GetTryAgent() const { return FTryAgent; }
  void SetTryAgent(bool value) { SET_SESSION_PROPERTY(TryAgent); }
  ```

## Build System

### CMake Requirements

- **Minimum Version**: CMake 3.15
- **Build Type**: `Debug`, `Release`, or `RelWithDebugInfo` (default)
- **Generator**: Ninja (recommended) or Visual Studio 17 2022

For complete build instructions, including environment setup and detailed steps, see [Developer Guide](./DEVELOPER.md).

### CMake Options

- `OPT_CREATE_PLUGIN_DIR`: Create plugin directory structure (default: OFF)
- `OPT_USE_UNITY_BUILD`: Use unity builds for faster compilation (x86 Release only)
- `PROJECT_PLATFORM`: Target platform (`x86`, `x64`, `ARM64`)



### Output Structure

- Plugin output: `Far3_<platform>/Plugins/NetBox/`
- Libraries: Built as static libraries
- DLLs: Required runtime DLLs (lua, sqlite3, etc.) in plugin directory

## Dependencies and Libraries

### External Libraries

Libraries are included in `libs/` directory:

- **PuTTY** (`libs/putty/`) - SSH/SCP implementation
- **FileZilla** (code in `src/filezilla/`) - FTP implementation
- **OpenSSL 3** (`libs/openssl-3/`) - Cryptography
- **neon** (`libs/neon/`) - WebDAV client
- **libs3** (`libs/libs3/`) - S3 client
- **expat** (`libs/expat/`) - XML parsing
- **zlib-ng** (`libs/zlib-ng/`) - Compression
- **tinyxml2** (`libs/tinyxml2/`) - XML parsing
- **fmt** (`libs/fmt/`) - Formatting library
- **tinylog** (`libs/tinylog/`) - Logging

### Library Rules

- **Do not modify** third-party library code directly
- Use patches in library directories if modifications needed
- Document any patches or modifications
- Keep library versions compatible with upstream when possible

### Linking

- Link statically where possible
- Use `_LIB` define for static library builds
- Minimize external DLL dependencies

## Version Control

### Git Workflow

- **Main Branch**: `main` (protected)
- Use feature branches for development
- Squash commits before merging to main
- Write clear, descriptive commit messages

### Commit Messages

Format:

```txt
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what and why, not how.

- Bullet points for multiple changes
- Reference issues if applicable
```

### Branch Naming

- `feature/description` - New features
- `fix/description` - Bug fixes
- `refactor/description` - Code refactoring
- `docs/description` - Documentation updates

### Ignored Files

- Build directories (`build/`, `Far3_*/`)
- IDE files (`.user`, `.suo`, etc.)
- Generated files
- Binary files (`.exe`, `.dll`, `.pdb`)

### CI/CD

- **AppVeyor**: Automated builds on Windows
- **GitHub Actions**: Release workflows
- Builds triggered on push to `main`
- Use `[skip appveyor]` in commit message to skip CI

## Testing and Quality

### Code Quality

- **No compiler warnings** (suppress known false positives with defines)
- **No memory leaks** - Use RAII and smart pointers
- **Exception safety** - Handle exceptions appropriately
- **Unicode correctness** - Always use wide strings for user-facing text

### Debugging

- Use debug builds for development (`RelWithDebugInfo` recommended)
- Include debug logging with `ADF()` macro
- Use assertions (`DebugAssert()`) for invariants
- Enable debug output in session configuration

### Error Handling

- Use exceptions for error conditions
- Provide meaningful error messages
- Log errors appropriately
- Handle network errors gracefully

### Performance

- Avoid unnecessary allocations
- Use move semantics where appropriate
- Profile before optimizing
- Consider memory usage (especially for x86 builds)

## Documentation

### Code Documentation

- Document public APIs
- Explain complex algorithms
- Include usage examples for non-obvious code
- Keep comments up-to-date with code changes

### User Documentation

- Update language files (`.lng`) for UI changes
- Document new features in changelog
- Keep README.md current

### Internal Documentation

- Document design decisions
- Explain workarounds and hacks
- Note compatibility requirements
- Document protocol implementations

### Language Files

- English: `NetBoxEng.lng` (primary)
- Other languages: `NetBoxRus.lng`, `NetBoxPol.lng`, etc.
- Use message IDs from `MsgIDs.h`
- Keep translations synchronized

## License and Copyright

### License

- **NetBox Code**: GNU General Public License v3 (GPL-3.0)
- **Third-party Libraries**: Respect their individual licenses
  - PuTTY: MIT License
  - OpenSSL: Apache License 2.0 / OpenSSL License
  - FileZilla: GPL
  - Others: Check individual library licenses

### Copyright Headers

When adding new files, include appropriate copyright notice:

```cpp
/*
 * Far-NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager
 * Copyright (c) [Year] [Your Name]
 *
 * Based on WinSCP (c) 2000-2025 Martin Prikryl
 * Based on PuTTY (c) 1997-2024 Simon Tatham
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
```

### Attribution

- Maintain attribution to original authors
- Document code origins
- Respect license requirements
- Include license files

## Additional Guidelines

### String Handling

- Use `UnicodeString` for wide strings
- Use `AnsiString` for narrow strings when needed
- Always handle encoding correctly
- Use UTF-8 for file I/O when possible

### Memory Management

- Prefer RAII and smart pointers
- Use `nbstr_*` functions for string memory management
- Avoid raw `new`/`delete` where possible
- Check for null pointers before dereferencing

### Thread Safety

- Document thread-safety requirements
- Use synchronization primitives appropriately
- Avoid shared mutable state
- Use thread-local storage when appropriate

### Platform Compatibility

- Support Windows XP+ (`_WIN32_WINNT=0x0501`)
- Test on all target platforms (x86, x64, ARM64)
- Handle platform-specific code with `#ifdef`
- Document platform requirements

### Protocol Implementation

- Follow protocol specifications (SSH, FTP, etc.)
- Maintain compatibility with standard implementations
- Handle protocol version negotiation
- Implement proper error handling

### UI Guidelines

- Use Far Manager plugin API correctly
- Follow Far Manager UI conventions
- Support keyboard navigation
- Provide accessible interfaces

## Enforcement

### Pre-commit Checks

- Run astyle on modified files
- Ensure code compiles without warnings
- Run basic functionality tests
- Check for obvious bugs

### Code Review

- All changes require review
- Review for style compliance
- Check for security issues
- Verify license compliance

### Continuous Integration

- Automated builds must pass
- Tests must pass (when available)
- No regressions allowed
- Maintain build performance

## Getting Help

### Resources

- **Project Page**: <https://github.com/michaellukashov/Far-NetBox>
- **Far Manager Forum**: <http://forum.farmanager.com/>
- **Discussions**:
  - Russian: <http://forum.farmanager.com/viewtopic.php?f=5&t=6317>
  - English: <http://forum.farmanager.com/viewtopic.php?f=39&t=6638>

### Questions

- Check existing documentation first
- Search issue tracker
- Ask on forum
- Review similar code in codebase

---

**Last Updated**: 2026-04-13 **Version**: 1.1
