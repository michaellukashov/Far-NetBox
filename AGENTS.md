# AGENTS.md - NetBox Development Guide

This document provides guidelines for AI agents working on the NetBox project (Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager).

## Build Commands

### Manual CMake Build

```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build
```

### Build Options

- `PROJECT_PLATFORM`: `x86`, `x64`, or `ARM64` (auto-detected if not specified)
- `CMAKE_BUILD_TYPE`: `Debug`, `Release`, or `RelWithDebugInfo`
- `OPT_CREATE_PLUGIN_DIR`: Create plugin directory structure (`ON`/`OFF`)
- `OPT_USE_UNITY_BUILD`: Enable unity builds for faster compilation (auto-enabled for x86 Release)
- `OPT_COMPILE_COMMANDS`: Generate `compile_commands.json` for IDE support (`ON`/`OFF`)

### CMake Build Configuration

#### x86 Release Build (Unity)

```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build --config Release
```

#### x64 Debug Build

```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DPROJECT_PLATFORM=x64 -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build
```

#### ARM64 Build

```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DPROJECT_PLATFORM=ARM64 -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build
```

## Code Formatting

### clang-format

The project does not include a `.clang-format` file. Follow the manual formatting rules below.

### Manual Formatting Rules

When clang-format is not available, follow these rules:

- **Brace style**: Allman/BSD (opening brace on new line)
- **Indentation**: 2 spaces (not tabs)
- **Line endings**: Windows CRLF
- **Pointer alignment**: Middle (`int * ptr`)
- **Reference alignment**: Middle (`int & ref`)
- **Max line length**: 120 characters

## Development Environment

### Environment Setup

Configure Visual Studio 2022 build environment:

```cmd
call "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
```

### Required Tools

- CMake 3.15+
- Ninja build system
- Visual Studio 2022 (MSVC)
- Git

## Code Style Guidelines

### Compiler Requirements

- **C++ Standard**: C++17 (required, no extensions)
- **Compiler**: Visual Studio 2022 (MSVC)
- **Build System**: CMake 3.15 or later

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
- ATL/MFC (`libs/atlmfc/`) - Minimal MFC subset
- dlmalloc (`libs/dlmalloc/`) - Memory allocator
- expat (`libs/expat/`) - XML parsing
- zlib-ng (`libs/zlib-ng/`) - Compression
- tinyxml2 (`libs/tinyxml2/`) - XML parser
- fmt (`libs/fmt/`) - String formatting
- tinylog (`libs/tinylog/`) - Logging
- GSL (`libs/GSL/`) - Guidelines Support Library
- icecream-cpp (`libs/icecream-cpp/`) - Debug logging

**Do not modify third-party library code directly.** Use patches if needed.

## Project Structure

```
src/
├── base/        # Base classes and utilities (UnicodeString, Classes, etc.)
├── core/        # Core functionality (SSH, FTP, SCP, S3, WebDAV, Terminal)
├── filezilla/   # FileZilla-based FTP implementation
├── include/     # Public headers (nbtypes.h, rtti.hpp, etc.)
├── nbcore/      # NetBox core utilities (string, memory, utils)
├── NetBox/      # Plugin implementation (Far plugin interface)
├── PluginSDK/   # Far Manager plugin SDK (Far3 headers)
├── resource/    # Resources (RC, LNG, licenses)
└── windows/     # Windows-specific code (GUI, dialogs, tools)
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

## CMake Structure

The NetBox project uses a modular CMake structure:

```
CMakeLists.txt (main - 100 lines)
├── cmake/NetBox.cmake (compiler/linker flags)
├── cmake/Install.cmake (post-build installation)
├── cmake/[Library].cmake (library configurations)
├── cmake/README.md (documentation)
│
├── libs/*/CMakeLists.txt (library builds)
└── src/CMakeLists.txt (plugin build - 530 lines)
```

### Key CMake Files

| File | Purpose |
|------|---------|
| `cmake/NetBox.cmake` | Centralized compiler flags and defines |
| `cmake/Install.cmake` | Plugin installation and post-build commands |
| `cmake/[Library].make` | Library-specific configurations |

### Adding/Modifying Libraries

1. **Create library config**: `cmake/NewLib.cmake`
2. **Create library build**: `libs/newlib/CMakeLists.txt`
3. **Add to main build**: `add_subdirectory(libs/newlib)` in `CMakeLists.txt`
4. **Link to plugin**: Add `newlib` to `NETBOX_LIBRARIES` in `src/CMakeLists.txt`

### Build Verification

```cmd
# Configure (check for CMake errors)
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Build (check for compilation errors)
cmake --build build

# Clean build
cmake --build build --clean-first
```

## CMake Library Build Template

Library configurations are in `cmake/[Library].cmake` files. Each third-party library in `libs/` has its own `CMakeLists.txt`.

### Library Configuration Files

| File | Purpose |
|------|---------|
| `cmake/NetBox.cmake` | Centralized compiler flags and defines |
| `cmake/Install.cmake` | Plugin installation and post-build commands |
| `cmake/ucm.cmake` | CMake utilities (from UCM project) |
| `cmake/[Library].cmake` | Library-specific configurations (OpenSSL, PuTTY, etc.) |

### Adding a New Library

1. **Create library build**: `libs/newlib/CMakeLists.txt`
2. **Add to main build**: `add_subdirectory(libs/newlib)` in `CMakeLists.txt`
3. **Link to plugin**: Add `newlib` to target_link_libraries in `src/CMakeLists.txt`

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Clear, descriptive, 50 chars or less for summary
- **Skip CI**: Include `[skip ci]` or `[ci skip]` in commit message to skip CI

### CI/CD

- **GitHub Actions**: Automated builds and releases (`.github/workflows/release.yml`)
- **AppVeyor**: Legacy CI configuration (`appveyor.yml`)

To skip CI in a commit message, include `[skip ci]` or `[ci skip]`.

## Code Quality Requirements

- No compiler warnings
- No memory leaks (use RAII)
- Exception safety
- Unicode correctness
- Thread safety documentation
- Protocol specification compliance

## Troubleshooting

### Common Build Errors

**Missing vcvarsall.bat**:
Ensure Visual Studio 2022 is installed with "Desktop development with C++" workload.

**Ninja not found**:
Install Ninja via `winget install Ninja-build.ninja` or Chocolatey.

**Unity build errors**:
Disable unity builds: `-DOPT_USE_UNITY_BUILD=OFF`

### Runtime Issues

**Plugin fails to load**:
- Verify plugin DLL matches Far Manager architecture (x86/x64)
- Check Far Manager plugin directory permissions
- Review Windows Event Viewer for error details

**Connection failures**:
- Verify firewall settings allow outbound connections
- Check server accessibility with `ping` and `telnet`
- Review Far Manager plugin log for detailed error messages

## Special Considerations

- This project is based on WinSCP, PuTTY, and FileZilla codebases
- Maintain compatibility with original design patterns
- Support Windows 10+ (`_WIN32_WINNT=0x0A00`)
- Test on all target platforms (x86, x64, ARM64)
