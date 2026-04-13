- **Multi-line commit**: Use `git commit -m "title" -m "body"` instead of heredoc (causes parsing errors)
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

1. **Read before writing** — Understand existing patterns before modifying code
2. **Edit, don't rewrite** — Make minimal surgical changes to existing files
3. **Don't re-read unnecessarily** — Remember files you've already read unless they may have changed
4. **Verify before declaring done** — Build and check your changes
5. **Be concise** — No fluff, no summaries, just the work
6. **User instructions override this file** — Always follow explicit user direction

Use skills if available:

- cpp-coding-standards
- cpp-expert
- memory-safety-patterns
- cpp-modern-features
- git-commit

When compacting, keep
- Current editable files
- Test error messages
- Architectural solutions from this session

### Task Execution Checklist

- [ ] Understand the task and locate relevant files
- [ ] Read existing code to match patterns and conventions
- [ ] Make minimal, focused changes
- [ ] Build succeeds with no warnings (`cmake --build ../build-RelWithDebugInfo --clean-first -- -j4`)
- [ ] No trailing whitespaces introduced
- [ ] No spelling/grammar errors in comments

### Before Committing

- [ ] Clean build completes without warnings
- [ ] Changes follow naming conventions (see below)
- [ ] No modifications to third-party code in `libs/`
- [ ] Manual testing of affected functionality

## Quick Reference

### Setting Up MSVC Build Environment

All build commands require the MSVC compiler environment. Use `vcvarsall.bat` from your Visual Studio installation.

**Find vcvarsall.bat** — common locations:
- VS2022 Community: `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat`
- VS2022 Professional: `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat`
- VS2022 Enterprise: `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat`

**Or use the environment variable** (if set by installer):
```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" <arch>
```

**Architecture options:** `x86_amd64` (x64), `x86` (Win32), `amd64_arm64` (ARM64)

### Standard Build (x64 RelWithDebugInfo)

```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B ../build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build ../build-RelWithDebugInfo -j
```

### Debug Build

```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-Debug-x64 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Debug-x64 -j
```

### Debug Build Win32

Win32 (x86) requires the x86 MSVC compiler. If the environment is not already set up, CMake will fail with "CMAKE_C_COMPILER not set".

```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-Debug-Win32 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Debug-Win32 -j
```

**Verify architecture:**
```cmd
dumpbin /headers build-Debug-Win32\src\NetBox.dll | findstr /i "machine"
:: Expected output: "14C machine (x86)" and "32 bit word machine"
```

### Release Build (x86, Unity)

```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-Release-Win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Release-Win32 -j
```

### Clean build

**x64 (RelWithDebugInfo):**
```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake --build build-RelWithDebugInfo --clean-first -- -j4
```

**Win32 (x86 Debug):**
```cmd
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake --build build-Debug-Win32 --clean-first -- -j4
```

**Full clean reconfigure (nuke and reconfigure):**
```cmd
rmdir /s /q build-RelWithDebugInfo
call "%VS170COMNTOOLS%\..\..\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

### Verify No Warnings

```cmd
cmake --build ../build-RelWithDebugInfo --clean-first -- -j4
```

### Verify Build with `act` (GitHub Actions locally)

Use `act` to run GitHub Actions workflows locally on a self-hosted runner.

**Prerequisites:**
- `act` installed: `winget install nektos.act`
- Self-hosted runner configured and available
- Docker or runner environment ready

**Test release workflow (create-release job):**

```cmd
act -W .github/workflows/release.yml -j create-release -P windows-2022=-self-hosted --use-new-action-cache
```

**Flags explained:**
| Flag | Purpose |
|------|---------|
| `-W .github/workflows/release.yml` | Workflow file to run |
| `-j create-release` | Run only the `create-release` job |
| `-P windows-2022=-self-hosted` | Map `windows-2022` runner label to self-hosted |
| `--use-new-action-cache` | Use action caching for faster re-runs |

**Verify output:** Look for build success in the workflow logs. If the job completes without error, the build is verified.

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

### OpenSSL NASM Assembly

OpenSSL uses platform-specific assembly files for optimized crypto primitives (SHA, AES, BN, etc.).
The build system compiles them with NASM via `cmake/OpenSSL.cmake` → `openssl_setup_asm_files()`.

**NASM executable:** `buildtools/tools/nasm.exe`

**x64 platform** (25 `.asm` files → `.obj`):

```cmake
nasm.exe -f win64 -o <build-dir>/<filename>.asm.obj libs/openssl-3/<path>/<filename>.asm
```

Key x64 ASM files: `crypto/sha/sha*-x86_64.asm`, `crypto/aes/aesni-x86_64.asm`,
`crypto/bn/x86_64-mont*.asm`, `crypto/ec/ecp_nistz256-x86_64.asm`,
`crypto/modes/ghash-x86_64.asm`, `crypto/modes/aesni-gcm-x86_64.asm`.

**x86 platform** (19 `.obj.asm` files → `.obj`):

```cmake
nasm.exe -f win32 -o <build-dir>/<filename>.obj.asm.obj libs/openssl-3/<path>/<filename>.obj.asm
```

**ARM64:** No ASM files (pure C fallback).

**Adding new ASM files:** Add to `_asm_file_list` in `cmake/OpenSSL.cmake` under the appropriate platform branch. The `add_custom_command()` in `openssl_setup_asm_files()` handles compilation automatically. ASM objects are linked into `libeay32` via `CRYPTO_SOURCES` + `ASM_OBJECTS`.

### OpenSSL Patch Application

NetBox maintains a local patch file `libs/openssl-3/0001-openssl-NetBox-patches.patch` with MSVC/Win32 fixes. After copying updated OpenSSL sources from upstream (e.g., WinSCP), **the patch is overwritten and must be re-applied**.

**Patch contents:** TSAN type casts (`volatile LONG*`), `FARPROC` fix in `cryptlib.c`, platform detection (`_WIN32`/`_WIN64`/`_M_ARM64`), `OPENSSL_NO_*` defines, directory path fixes, and rcu.h guard.

**How to apply:**

From the **project root** (`D:\Projects\NetBox\NetBox-dev`):

```cmd
cd libs\openssl-3
git apply -p3 0001-openssl-NetBox-patches.patch
```

**Important:** The `-p3` flag strips 3 path components from the patch (`libs/openssl-3/openssl-3/crypto/...` → `crypto/...`). Always run from inside `libs/openssl-3/` — running from the project root will fail with "No such file or directory".

**Verify the patch applied:**

```cmd
git apply -p3 --check 0001-openssl-NetBox-patches.patch
```

Silent output = OK. Any "patch does not apply" message means upstream changed — manually inspect the hunk and re-create it.

**Common failure after update:** If `git apply` skips patches, check context lines have changed. Use `--reject` to see what failed:

```cmd
git apply -p3 --reject 0001-openssl-NetBox-patches.patch
```

Then manually fix rejected hunks by comparing with the patch diff.

**Critical fixes that break Win32 build if not applied:**

1. `crypto/cryptlib.c` line 45: `int (*f)(void)` → `FARPROC f` (calling convention mismatch on x86)
2. `include/crypto/bn_conf.h`: platform detection for `_WIN32` vs `_WIN64` vs `_M_ARM64`
3. `include/openssl/configuration.h`: `OPENSSL_SYS_WIN32`/`OPENSSL_SYS_WIN64A` defines

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
- **Multi-line commit**: Use `git commit -m "title" -m "body"` instead of heredoc
- **Skip CI**: `[skip ci]` or `[ci skip]` in commit message
- **CI/CD**: GitHub Actions (`.github/workflows/release.yml`), AppVeyor (legacy)

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `vcvarsall.bat` not found | Install VS2022 with "Desktop development with C++". Check: `C:\Program Files\Microsoft Visual Studio\2022\{Community,Professional,Enterprise}\VC\Auxiliary\Build\vcvarsall.bat` |
| `CMAKE_C_COMPILER not found` | Run `vcvarsall.bat` first: `call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64` |
| Ninja not found | `winget install Ninja-build.ninja` |
| Unity build errors | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Plugin fails to load | Check architecture match (x86/x64), verify dependencies |
| Connection failures | Check firewall, test with `ping`/`telnet`, review plugin log |
| WinXP build failures | Use v141_xp toolset: `-T v141_xp` in CMake or set in VS IDE |

## Shell Script Rules (Bash / PowerShell)

### General

- **Platform:** Windows (`cmd.exe`) — use native commands where possible
- **Prefer:** Built-in commands (`dir`, `findstr`, `robocopy`, `powershell -Command`)
- **Avoid:** Unix-style redirections (`2>/dev/null`, `>/dev/null`) — not supported on Windows
- **No:** `&&`, `||`, `;` chaining — execute each command as a separate tool call

### PowerShell (`pwsh` / `powershell`)

- Use `powershell -Command "..."` for inline commands
- **String quoting:** Use double-quotes around the entire `-Command` argument, single-quotes inside
  - ✅ `powershell -Command "if (Test-Path 'path\to\file') { 'exists' } else { 'not exists' }"`
  - ❌ `powershell -Command 'if (Test-Path "path\to\file") { ... }'`
- **Avoid:** pipe + `ForEach-Object` with complex expressions (parsing issues with shell escaping)
- **Path comparisons:** Use `Test-Path` instead of `Test-Path` in loops with `ForEach-Object` — split into individual calls
- **File listings:** Use `Get-ChildItem` with `-Include` or `-Filter`, avoid complex pipeline expressions
- **Encoding:** PowerShell may output Unicode — use `| Out-String -Width 200` if lines get truncated

### Bash (Git Bash / WSL)

- Only use when POSIX tools are required (`find`, `grep -P`, `sed`, `awk`)
- **Never use:** `2>/dev/null` — Windows has no `/dev/null` (use `2>nul` in cmd, or omit)
- **Path separator:** Windows paths use `\` — convert to `/` for bash: `cygpath -u "D:\path"` or `sed 's/\\/\//g'`
- **Line endings:** Output from bash on Windows may mix CRLF/LF — be aware when parsing

### Common Pitfalls

| Pitfall | Wrong | Right |
|---------|-------|-------|
| Redirect stderr | `cmd 2>/dev/null` | `cmd 2>nul` (cmd) or omit |
| Chain commands | `cmd1 && cmd2` | Two separate tool calls |
| Complex pwsh pipeline | `ps | ForEach-Object { "$_ \| WinSCP: $(Test-Path ...)" }` | Loop with individual `powershell -Command` calls |
| Path in pwsh string | `"$env:PATH\file"` (expands incorrectly) | Use explicit paths or `Join-Path` |
| `findstr` with regex | `findstr ".*pattern"` | `findstr /r ".*pattern"` |
| Empty output check | `if [ -z "$var" ]` | `powershell -Command "if ($null -eq $var) { ... }"` |

## AI Context Files

| File | Purpose |
|------|---------|
| AGENTS.md | This file — project structure map and AI agent guide |
| .ai-factory/DESCRIPTION.md | Project specification and tech stack |
| .ai-factory/ARCHITECTURE.md | Architecture decisions and guidelines |
| .ai-factory/config.yaml | AI Factory configuration (paths, workflow, git) |
| .ai-factory/rules/base.md | Auto-detected project conventions |
| PROJECT.md | Project requirements and scope |
| CODEBASE.md | Codebase documentation index |
| DEPENDENCIES.md | Third-party library documentation |

**Last Updated**: 2026-04-13

## Agent Rules

- Never combine shell commands with `&&`, `||`, or `;` — execute each command as a separate Bash tool call. This applies even when a skill, plan, or instruction provides a combined command — always decompose it into individual calls.
  - ❌ Wrong: `git checkout <configured-base-branch> && git pull`
  - ✅ Right: Two separate Bash tool calls — first `git checkout <configured-base-branch>`, then `git pull origin <configured-base-branch>`
- **CRITICAL:** Always read `.ai-factory/rules/base.md` before making changes — it contains auto-detected project conventions
- **CRITICAL:** Never modify third-party code in `libs/` — use patches instead
- **CRITICAL:** Build must succeed with NO warnings (MSVC W4) before declaring task complete

## Code Quality Checklist

- [ ] No compiler warnings (MSVC W4)
- [ ] No memory leaks (use RAII)
- [ ] Exception safety
- [ ] Unicode correctness
- [ ] No spelling/grammar errors in comments
- [ ] Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`
