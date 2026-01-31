# AGENTS.md - NetBox Development Guide

This document provides guidelines for AI agents working on the NetBox project (Far-NetBox SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager).

## Getting Started

### First-Time Setup

1. **Clone the repository**:
   ```cmd
   git clone https://github.com/michaellukashov/Far-NetBox.git
   cd Far-NetBox
   ```

2. **Install required tools**:
   - Visual Studio 2022 (Desktop development with C++ workload)
   - CMake 3.15 or later
   - Ninja build system (optional but recommended)
   - Git

3. **Configure build environment**:
   ```cmd
   call "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
   ```

4. **Configure and build**:
   ```cmd
   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build
   ```

5. **Install to Far Manager**:
   - Copy `Far3_x64/Plugins/NetBox/` to your Far Manager plugins directory
   - Or set `OPT_CREATE_PLUGIN_DIR=ON` during configuration

### Quick Verification

After building, verify the plugin works:
1. Start Far Manager
2. Press `Alt+F1` to open the disk menu
3. Select NetBox from the plugins list
4. Configure a connection and connect to test

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

## Linting and Quality Checks

### Compiler Warnings

The project uses compiler warnings as the primary linting mechanism. MSVC is configured with:
- Warning level 4 (W4) for comprehensive error detection
- Disabled specific deprecation warnings for legacy code compatibility

**Enable strict warnings during build**:
```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build 2>&1 | findstr /C:"warning"
```

### Quality Verification Commands

Before committing changes, verify code quality:

```cmd
# Clean rebuild to catch all warnings
cmake --build build --clean-first

# Check for specific warning patterns (PowerShell)
cmake --build build | Select-String "warning" | Select-String -NotMatch "third-party|libs/"
```

### Static Analysis (Optional)

For additional code analysis, you can integrate:
- **Visual Studio's built-in Code Analysis** (Ctrl+Shift+Alt+M in VS)
- **Clang-Tidy** (requires Clang installation)

Enable Clang-Tidy in CMake:
```cmd
cmake -S . -B build -G "Ninja" -DCMAKE_CXX_CLANG_TIDY="clang-tidy"
```

### Build Quality Checklist

Before submitting changes:
- [ ] Build completes with no errors
- [ ] No warnings in project source files (third-party libs excluded)
- [ ] Debug and Release builds both succeed
- [ ] Plugin loads in Far Manager
- [ ] Manual test of affected functionality

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

## Debugging

### Visual Studio Debugging

**To debug the plugin in Visual Studio**:

1. **Generate VS2022 solution**:
   ```cmd
   cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
   ```

2. **Open the solution**:
   ```cmd
   start build\NetBox.sln
   ```

3. **Configure debugger**:
   - Right-click the NetBox project → Properties
   - Go to Configuration Properties → Debugging
   - Set Command to your Far Manager executable path
     - Example: `C:\Program Files\Far Manager\Far.exe`
   - Set Working Directory to Far Manager's directory
   - Set Command Arguments to plugins path if needed

4. **Set breakpoints and start debugging** (F5)

### Remote Debugging

For debugging on another machine:
1. Install Remote Tools for Visual Studio on target machine
2. Copy the plugin DLL and PDB to the target
3. Attach to `Far.exe` process on the target machine

### Debug Logging

Use the project's logging facilities:
- `ADF()` macro for debug output
- `DebugAssert()` for invariants
- Built-in logging via `tinylog` library (see `src/nbcore/logging.cpp`)

### Common Debugging Scenarios

**Plugin fails to load**:
- Check architecture compatibility (x86 vs x64)
- Verify dependencies are available (OpenSSL DLLs, etc.)
- Check Windows Event Viewer for detailed error messages

**Connection issues**:
- Enable debug logging in plugin settings
- Review Far Manager plugin log
- Test connectivity with external tools (ping, telnet, etc.)

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

```text
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
| ---- | -------- |
| `cmake/NetBox.cmake` | Centralized compiler flags and defines |
| `cmake/Install.cmake` | Plugin installation and post-build commands |
| `cmake/[Library].cmake` | Library-specific configurations |

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

## Contribution Guidelines

### Before Contributing

1. **Search existing issues**: Check if your feature request or bug report already exists
2. **Discuss major changes**: For significant modifications, open an issue first to discuss the approach
3. **Follow coding standards**: Adhere to the code style guidelines in this document
4. **Test thoroughly**: Ensure manual testing covers the changed functionality
5. **Check build quality**: Verify builds complete without warnings

### Pull Request Process

1. **Create a feature branch**:
   ```cmd
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/your-bug-fix
   ```

2. **Make your changes** following the code style guidelines

3. **Build and test**:
   ```cmd
   cmake --build build --clean-first
   # Test in Far Manager
   ```

4. **Commit your changes** with clear, descriptive messages (50 chars or less for summary)

5. **Push to your fork**:
   ```cmd
   git push origin feature/your-feature-name
   ```

6. **Open a pull request**:
   - Describe the changes clearly
   - Reference related issues
   - Include screenshots for UI changes
   - List testing performed

### Code Review Process

- All contributions require review before merging
- Address review comments promptly
- Keep PRs focused on a single concern
- Squash commits if necessary before merging

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Clear, descriptive, 50 chars or less for summary
- **Skip CI**: Include `[skip ci]` or `[ci skip]` in commit message to skip CI

### CI/CD

- **GitHub Actions**: Automated builds and releases (`.github/workflows/release.yml`)
- **AppVeyor**: Legacy CI configuration (`appveyor.yml`)

## Code Quality Requirements

- No compiler warnings
- No memory leaks (use RAII)
- Exception safety
- Unicode correctness
- Thread safety documentation
- Protocol specification compliance
- **No spelling/grammar typos in comments and documentation**

### Code Quality Verification

Regular quality checks should be performed:

1. **Build Verification**: Ensure clean compilation without warnings
2. **Typo Checking**: Check for common spelling errors in source files:
   - `loose` vs `lose` (common error)
   - `connexion` vs `connection`
   - `authentification` vs `authentication`
   - `occured` vs `occurred`
   - `recieve` vs `receive`
   - `seperate` vs `separate`
   - And other common typos

3. **Comment Quality**: Ensure comments are clear, grammatically correct, and use proper English terminology

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
