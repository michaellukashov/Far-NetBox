# AGENTS.md - NetBox Development Guide

This document provides guidelines for AI coding assistants working on the NetBox project (Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager).

## Project Overview

| Attribute | Value |
|-----------|-------|
| **Language** | C++17 (no extensions) |
| **Compiler** | MSVC (Visual Studio 2022) |
| **Build System** | CMake 3.15+ / Ninja |
| **Platforms** | x86, x64, ARM64 |
| **Base** | WinSCP, PuTTY, FileZilla codebases |

## AI Agent Workflow

### Core Principles

1. **Read before writing** - Understand existing patterns before modifying code
2. **Edit, don't rewrite** - Make minimal surgical changes to existing files
3. **Don't re-read unnecessarily** - Remember files you've already read unless they may have changed
4. **Verify before declaring done** - Build and check your changes
5. **Be concise** - No fluff, no summaries, just the work
6. **User instructions override this file** - Always follow explicit user direction

### Task Execution Checklist

- [ ] Understand the task and locate relevant files
- [ ] Read existing code to match patterns and conventions
- [ ] Make minimal, focused changes
- [ ] Build succeeds with no warnings (`cmake --build ../build-RelWithDebugInfo --clean-first`)
- [ ] No trailing whitespaces introduced
- [ ] No spelling/grammar errors in comments

### Before Committing

- [ ] Clean build completes without warnings
- [ ] Changes follow naming conventions (see below)
- [ ] No modifications to third-party code in `libs/`
- [ ] Manual testing of affected functionality

## Quick Reference

### Standard Build (x64 RelWithDebugInfo)

```cmd
call "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cmake -S . -B ../build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build ../build-RelWithDebugInfo -j
```

### Debug Build

```cmd
cmake -S . -B ../build-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build ../build-Debug -j
```

### Release Build (x86, Unity)

```cmd
cmake -S . -B ../build-Release -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build ../build-Release -j
```

### Verify No Warnings

```cmd
cmake --build ../build-RelWithDebugInfo --clean-first
```

## Build Configuration

### CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug`, `Release`, `RelWithDebugInfo` | `Debug` | Build configuration |
| `PROJECT_PLATFORM` | `x86`, `x64`, `ARM64` | Auto-detected | Target architecture |
| `OPT_CREATE_PLUGIN_DIR` | `ON`, `OFF` | `OFF` | Create plugin directory structure |
| `OPT_USE_UNITY_BUILD` | `ON`, `OFF` | `ON` for x86 Release only | Faster compilation |
| `OPT_COMPILE_COMMANDS` | `ON`, `OFF` | `OFF` | Generate `compile_commands.json` |

### CMake Structure

```
CMakeLists.txt                    # Main entry (74 lines)
├── cmake/
│   ├── NetBox.cmake              # Compiler/linker flags
│   ├── Libraries.cmake           # Centralized library config
│   ├── PlatformDetection.cmake   # Platform auto-detection
│   └── Install.cmake             # Post-build installation
├── libs/*/CMakeLists.txt         # Third-party library builds
└── src/CMakeLists.txt            # Plugin target (~530 lines)
```

### Adding a New Library

1. Create `cmake/NewLib.cmake` with library configuration
2. Create `libs/newlib/CMakeLists.txt` for the build
3. Add `add_subdirectory(libs/newlib)` in root `CMakeLists.txt`
4. Link to plugin: add `newlib` to `NETBOX_LIBRARIES` in `src/CMakeLists.txt`

### Build Output

- Plugin DLLs: `Far3_<platform>/Plugins/NetBox/`
- Platforms: `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/`

## Project Structure

```
src/
├── base/        # Base classes (UnicodeString, Classes, etc.)
├── core/        # Protocol implementations (SSH, FTP, SCP, S3, WebDAV)
├── filezilla/   # FileZilla-based FTP engine
├── include/     # Public headers (nbtypes.h, rtti.hpp)
├── nbcore/      # Core utilities (strings, memory, logging)
├── NetBox/      # Far Manager plugin interface
├── PluginSDK/   # Far3 plugin SDK headers
├── resource/    # Resources (.rc, .lng, licenses)
└── windows/     # Windows-specific code (GUI, dialogs)
```

## Dependencies

Third-party libraries in `libs/` — **never modify directly**, use patches if needed:

| Library | Location | Purpose |
|---------|----------|---------|
| PuTTY | `libs/putty/` | SSH/SCP protocol |
| FileZilla | `src/filezilla/` | FTP protocol |
| OpenSSL 3 | `libs/openssl-3/` | Cryptography |
| neon | `libs/neon/` | WebDAV protocol |
| libs3 | `libs/libs3/` | S3 protocol |
| ATL/MFC | `libs/atlmfc/` | Minimal MFC subset |
| dlmalloc | `libs/dlmalloc/` | Memory allocator |
| expat | `libs/expat/` | XML parsing |
| zlib-ng | `libs/zlib-ng/` | Compression |
| tinyxml2 | `libs/tinyxml2/` | XML parser |
| fmt | `libs/fmt/` | String formatting |
| tinylog | `libs/tinylog/` | Logging |
| GSL | `libs/GSL/` | Guidelines Support Library |
| icecream-cpp | `libs/icecream-cpp/` | Debug logging |

## Code Style

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | `T` + PascalCase | `TSessionData`, `TSecureShell` |
| Methods/Functions | PascalCase | `GetSessionData()`, `Connect()` |
| Member variables | `F` + PascalCase | `FSessionData`, `FConfig` |
| Local variables | camelCase | `sessionData`, `configValue` |
| Constants/Macros | UPPER_CASE | `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT` |
| Enums | PascalCase | `SessionType`, `AuthMethod` |

### Formatting

- **Brace style**: Allman/BSD (opening brace on new line)
- **Indentation**: 2 spaces (no tabs)
- **Line endings**: CRLF (Windows)
- **Pointer/reference**: Middle alignment (`int * ptr`, `int & ref`)
- **Max line length**: 120 characters
- **No trailing whitespaces** in any source or CMake file
- **No `.clang-format`** file — follow manual rules above

### File Organization

- **Include guards**: `#pragma once`
- **Include order**: System headers → Project headers → Local headers
- **Extensions**: `.h`/`.hpp` for headers, `.cpp` for sources, `.rc`/`.lng` for resources

### Comments

- `//` for single-line comments
- `/* */` for multi-line comments
- `// TODO: description` for future work
- `// FIXME: description` for known issues
- Comments must be clear, grammatically correct English

## Code Patterns

### Error Handling

- Use exceptions for error conditions
- Log with `ADF()` macro for debug output
- Use `DebugAssert()` for invariants
- Handle network errors gracefully with meaningful messages

### Memory Management

- Prefer RAII and smart pointers
- Avoid raw `new`/`delete`
- Use `nbstr_*` functions for string memory management
- Check for null before dereferencing

### String Handling

- `UnicodeString` for wide strings (user-facing text)
- `AnsiString` for narrow strings when needed
- UTF-8 for file I/O when possible
- Handle encoding conversions explicitly

### Debugging

- `ADF()` macro for debug output
- `DebugAssert()` for assertions
- Logging via `tinylog` (see `src/nbcore/logging.cpp`)
- For VS debugging: generate solution with `-G "Visual Studio 17 2022"`, set Far.exe as debug command

## Language Files

- English: `NetBoxEng.lng` (primary, always update this)
- Other languages: `NetBoxRus.lng`, `NetBoxPol.lng`, etc.
- Use message IDs from `MsgIDs.h`
- Keep translations synchronized when modifying UI strings

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Imperative mood, under 72 chars for summary
- **Skip CI**: `[skip ci]` or `[ci skip]` in commit message
- **CI/CD**: GitHub Actions (`.github/workflows/release.yml`), AppVeyor (legacy)

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Missing `vcvarsall.bat` | Install VS2022 with "Desktop development with C++" |
| Ninja not found | `winget install Ninja-build.ninja` |
| Unity build errors | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Plugin fails to load | Check architecture match (x86/x64), verify dependencies |
| Connection failures | Check firewall, test with `ping`/`telnet`, review plugin log |

## Code Quality Checklist

- [ ] No compiler warnings (MSVC W4)
- [ ] No memory leaks (use RAII)
- [ ] Exception safety
- [ ] Unicode correctness
- [ ] No spelling/grammar errors in comments
- [ ] Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`
