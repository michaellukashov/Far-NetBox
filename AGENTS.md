# AGENTS.md — NetBox Development Guide

> Version: 2.0.0 | Last updated: 2026-04-26

You are an AI coding assistant working on **NetBox** — a Far Manager plugin (SFTP/FTP/SCP/WebDAV/S3 client) built in C++17 on WinSCP, PuTTY, and FileZilla codebases. Far Manager is a text-mode file manager for Windows; NetBox loads as a plugin DLL via F11.

## Deep References

| File | Purpose |
|------|---------|
| [AGENTS-Standards.md](.ai-factory/AGENTS-Standards.md) | Code conventions, RAII patterns, exception hierarchy |
| [AGENTS-Structure.md](.ai-factory/AGENTS-Structure.md) | CMake details, OpenSSL NASM, language files |
| [AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md) | Full build scripts, debugging, CI, shell rules |

---

## 1. 📌 Project Overview

### What Is NetBox

NetBox is a **Far Manager 3.0 plugin** providing file transfer over multiple protocols:

| Protocol | Implementation Source |
|----------|----------------------|
| **SFTP/SCP** | PuTTY + WinSCP codebases |
| **FTP/FTPS** | FileZilla codebase |
| **WebDAV** | neon library |
| **S3** | libs3 library |

### At a Glance

| Attribute | Value |
|-----------|-------|
| **Language** | C++17 (no extensions) |
| **Compiler** | MSVC (Visual Studio 2022) |
| **Build System** | CMake 3.15+ / Ninja |
| **Platforms** | x86, x64, ARM64 |
| **IDE** | VS2022 (generate solution with `-G "Visual Studio 17 2022"`) |

---

## 2. 🏗️ Architecture & File Structure

### Source Tree

```
src/
├── base/        # Base classes (UnicodeString, Classes, etc.)
├── core/        # Protocol implementations (SSH, FTP, SCP, S3, WebDAV)
├── filezilla/   # FileZilla-based FTP engine
├── include/     # Public headers (nbtypes.h, rtti.hpp)
├── nbcore/      # Core utilities (strings, memory, logging)
├── NetBox/      # Far Manager plugin interface (entry point)
├── PluginSDK/   # Far3 plugin SDK headers
├── resource/    # Resources (.rc, .lng, licenses)
└── windows/     # Windows-specific code (GUI, dialogs)
```

### Build Output

| Artifact | Location |
|----------|----------|
| Plugin DLL | `Far3_<platform>/Plugins/NetBox/` |
| Platform dirs | `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/` |
| Build dir | `build-<config>/` (e.g., `build-RelWithDebugInfo/`) |

**Requires:** `OPT_CREATE_PLUGIN_DIR=ON` in CMake configuration.

### CMake Structure

```
CMakeLists.txt                    # Main entry
├── cmake/
│   ├── NetBox.cmake              # Compiler/linker flags
│   ├── Libraries.cmake           # Centralized library config
│   ├── PlatformDetection.cmake   # Platform auto-detection
│   └── Install.cmake             # Post-build installation
├── libs/*/CMakeLists.txt         # Third-party library builds
└── src/CMakeLists.txt            # Plugin target
```

### Key CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug`, `Release`, `RelWithDebugInfo` | `Debug` | Build configuration |
| `PROJECT_PLATFORM` | `x86`, `x64`, `ARM64` | Auto-detected | Target architecture |
| `OPT_CREATE_PLUGIN_DIR` | `ON`, `OFF` | `OFF` | Create plugin directory structure |
| `OPT_USE_UNITY_BUILD` | `ON`, `OFF` | `ON` (x86 Release only) | Faster compilation |
| `OPT_COMPILE_COMMANDS` | `ON`, `OFF` | `OFF` | Generate `compile_commands.json` |

---

## 3. ⚙️ Build, Run & Test Commands

### Quick Build

Use pre-configured `.bat` scripts from project root. **Each script handles vcvarsall.bat + configure + build in one session.**

| Script | Platform | Config |
|--------|----------|--------|
| `build-x64.bat` | x64 (AMD64) | RelWithDebugInfo |
| `build-x86.bat` | x86 (Win32) | RelWithDebugInfo |
| `build-arm64.bat` | ARM64 | RelWithDebugInfo |

```cmd
cmd /c build-x64.bat
```

### Clean Rebuild

```cmd
rmdir /s /q build-RelWithDebugInfo
cmd /c build-x64.bat
```

### Rebuild Without Deleting Build Dir

```cmd
cmake --build build-RelWithDebugInfo --clean-first -- -j4
```

### Testing in Far Manager

1. Build with `OPT_CREATE_PLUGIN_DIR=ON` (the `.bat` scripts do this)
2. Launch Far Manager from platform directory: `Far3_x64\Far.exe`
3. Press `F11` → Plugins → NetBox
4. Test the affected functionality
5. Check log: `%LOCALAPPDATA%\NetBox\netbox.log` or tinylog output (`src/nbcore/logging.cpp`)

### Debugging in Visual Studio

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-VS -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
```

Open `build-VS\NetBox.sln`. Set debug command to `Far3_x64\Far.exe` (project properties → Debugging).

> **Full build scripts, troubleshooting, and CI details:** [AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md)

---

## 4. 📏 Code Style & Conventions

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | `T` + PascalCase | `TSessionData`, `TSecureShell` |
| Methods/Functions | PascalCase | `GetSessionData()`, `Connect()` |
| Member variables | `F` + PascalCase | `FSessionData`, `FConfig` |
| Local variables | camelCase | `sessionData`, `configValue` |
| Constants/Macros | UPPER_CASE | `MAX_RETRY_COUNT` |
| Enums | PascalCase | `SessionType`, `AuthMethod` |

### Formatting

- **Brace style:** Allman/BSD (opening brace on new line)
- **Indentation:** 2 spaces (no tabs)
- **Line endings:** CRLF (Windows)
- **Encoding:** UTF-8 without BOM
- **Pointer/reference:** Middle alignment (`int * ptr`, `int & ref`)
- **Max line length:** 120 characters
- **No trailing whitespace** in any source or CMake file
- See `.clang-format` for automated formatting

### File Organization

- **Include guards:** `#pragma once`
- **Include order:** System headers → Project headers → Local headers
- **Extensions:** `.h`/`.hpp` for headers, `.cpp` for sources

### Memory Management

- Prefer RAII and smart pointers (`std::unique_ptr` for exclusive ownership)
- Avoid raw `new`/`delete`
- Check for null before dereferencing

### String Handling

- `UnicodeString` for wide strings (user-facing text)
- `AnsiString` for narrow strings when needed
- UTF-8 for file I/O when possible

### Error Handling

- Use exceptions for error conditions, **not** return codes
- Log with `FTerminal->LogEvent()` for debug output
- Use `DebugAssert()` for invariants (fires in debug builds)
- Handle network errors gracefully with meaningful messages

### Threading

- **Main thread:** All Far Manager API calls
- **Worker threads:** Protocol operations
- **Synchronization:** `CRITICAL_SECTION` or mutexes for shared data
- **Rule:** Never call Far Manager APIs from worker threads

> **Full standards, exception hierarchy, and RAII patterns:** [AGENTS-Standards.md](.ai-factory/AGENTS-Standards.md)

---

## 5. 🔧 Development Workflow

### Core Principles

1. **Read before writing** — Understand existing patterns before modifying code
2. **Edit, don't rewrite** — Make minimal surgical changes
3. **Don't re-read unnecessarily** — Remember files you've already read unless they may have changed
4. **Verify before declaring done** — Build and check your changes
5. **User instructions override this file** — Always follow explicit user direction

### Task Execution Checklist

- [ ] Understand the task and locate relevant files
- [ ] Read existing code to match patterns and conventions
- [ ] Make minimal, focused changes
- [ ] Build succeeds with **zero warnings**
- [ ] No trailing whitespaces introduced
- [ ] No spelling errors in comments

### Tool Selection

| Task | Tool |
|------|------|
| Read a file | `read` (use `sel` for line ranges) |
| Find files by pattern | `find(pattern="src/**/*.cpp")` |
| Search code by keyword | `grep(pattern="GetSessionData", path="src/")` |
| Structural code search | `ast_grep` (AST-aware) |
| Edit code | `edit` — surgical changes with 3+ lines context |
| Create new file | `write` |
| Run build | `bash` — one command per call, use `.bat` scripts |
| Track multi-step work | `todo_write` |
| Ask user a question | `ask` |
| Structural rewrite | `ast_edit` |
| Rename/refactor | `lsp` (rename, references, code actions) |
| Code review | skill: `aif-review` |
| Commit changes | skill: `aif-commit` |
| Fix a bug | skill: `aif-fix` |
| Plan a feature | skill: `aif-plan` |

### Skills to Use

| Skill | When |
|-------|------|
| `cpp-coding-standards` | Writing/reviewing C++ code |
| `cpp-expert` | Complex C++ questions |
| `memory-safety-patterns` | RAII, ownership, resource management |
| `cpp-modern-features` | Modern C++ syntax |
| `cpp-testing` | Writing/fixing tests |
| `aif-review` | Code review before commit |
| `aif-commit` | Conventional commit messages |
| `aif-fix` | Fix specific bugs |
| `aif-plan` | Plan features before coding |
| `aif-security-checklist` | Security review |

### Git Workflow

- **Main branch:** `main` (protected)
- **Branch naming:** `feature/description`, `fix/description`, `refactor/description`
- **Commit messages:** Imperative mood, under 72 chars summary
- **Skip CI:** `[skip ci]` in commit message

> **Full shell rules and CI details:** [AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md)

---

## 6. ⚠️ Critical Notes for Agents

### Build Environment

- **Configure + build MUST happen in the same cmd session** with `vcvarsall.bat` environment
- **Always use `.bat` scripts** — never chain commands with `&&`
- **Never use `cmd /c "call vcvarsall.bat && cmake ..."`** — quote encoding breaks
- Third-party libraries in `libs/` may produce warnings — this is expected

### Unity Build Gotcha

Symbol redefinition errors? Unity build is the culprit. Disable:

```cmd
cmake -S . -B build-RelWithDebugInfo -DOPT_USE_UNITY_BUILD=OFF ...
```

### OpenSSL Patches

After updating OpenSSL from upstream (WinSCP), **re-apply the patch**:

```cmd
git -C libs\openssl-3 apply -p3 0001-openssl-apply-NetBox-patches.patch
```

See [AGENTS-Structure.md](.ai-factory/AGENTS-Structure.md) for full patch details.

### VS Edition Paths

| Edition | vcvarsall.bat Path |
|---------|-------------------|
| Community | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat` |
| Professional | `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat` |
| Enterprise | `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat` |

### When Stuck

- **Don't guess** — say "insufficient information" and ask the user
- **Can't find the file?** — use `find` with broader patterns
- **Can't find the code?** — use `grep` for the function/class name
- **Unsure about the approach?** — use `ask` or `/aif-explore`
- **Build fails repeatedly?** — clean reconfigure or read [AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md) → "Common Build Errors"
- **Shell commands failing?** — read [AGENTS-Workflows.md](.ai-factory/AGENTS-Workflows.md) → "Agent Build Execution Rules"
- **Something feels wrong?** — stop and tell the user

---

## 7. 🚫 Constraints & Anti-patterns

### MUST NOT

- **NEVER modify `libs/`** — use patches instead
- **NEVER combine shell commands** with `&&`, `||`, `;`
- **NEVER use Unix-style redirections** (`< /dev/null`, `>/dev/null`, `2>/dev/null`) on Windows — use `NUL` or omit
- **NEVER rewrite entire files** — make minimal edits
- **NEVER commit secrets** or credentials
- **NEVER modify third-party code** in `libs/`

### MUST

- **Build must pass with ZERO warnings** (MSVC W4)
- **All files must use CRLF line endings**
- **UTF-8 without BOM** for all text files
- **No trailing whitespace** in source, CMake, or documentation files
- **Follow naming conventions** — `T` prefix for classes, `F` prefix for member variables
- **Use RAII** for resource management — prefer smart pointers over raw `new`/`delete`
- **All Far Manager API calls from main thread only**

### Anti-patterns

| Anti-pattern | Instead |
|--------------|---------|
| Rewriting entire files | Make minimal, surgical edits |
| `cmd1 && cmd2` chaining | Separate tool calls or `.bat` wrapper |
| `new T` / `delete ptr` | `std::make_unique<T>()` / `std::unique_ptr<T>` |
| Return codes for errors | Throw exceptions |
| Raw C strings | `UnicodeString` / `AnsiString` |
| Inline command chains | `.bat` script files |
| Guessing at unclear requirements | Ask the user |

### Quality Gates

Before declaring a task complete:

- [ ] Clean build with zero warnings
- [ ] No modifications to `libs/`
- [ ] Plugin DLL in `Far3_<platform>/Plugins/NetBox/`
- [ ] CRLF line endings on all modified files
- [ ] UTF-8 without BOM in all text files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments

Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`

---

## 8. 📚 Key Dependencies & Versions

### Third-party Libraries (`libs/`)

| Library | Location | Purpose |
|---------|----------|---------|
| PuTTY | `libs/putty/` | SSH/SCP protocol |
| FileZilla | `src/filezilla/` | FTP protocol |
| neon | `libs/neon/` | WebDAV protocol |
| libs3 | `libs/libs3/` | S3 protocol |
| OpenSSL | `libs/openssl-3/` | TLS/SSL cryptography |
| zlib-ng | `libs/zlib-ng/` | Compression |
| expat | `libs/expat/` | XML parsing |
| tinyxml2 | `libs/tinyxml2/` | XML parser |
| fmt | `libs/fmt/` | String formatting |
| tinylog | `libs/tinylog/` | Logging |
| GSL | `libs/GSL/` | Guidelines Support Library |
| icecream-cpp | `libs/icecream-cpp/` | Debug logging |
| dlmalloc | `libs/dlmalloc/` | Memory allocator |
| ATL/MFC | `libs/atlmfc/` | Minimal MFC subset |

### Version Matrix

| Component | Version |
|-----------|---------|
| **OpenSSL** | 3.3.7 |
| **PuTTY** | 0.81 |
| **FileZilla** | 2.2.32 |
| **WinSCP** (upstream) | 6.5.6 |
| **zlib-ng** | 2.2.2 |
| **NetBox** | See `src/NetBox/plugin_version.hpp` |

### NASM (OpenSSL Assembly)

OpenSSL uses platform-specific assembly for crypto primitives. NASM executable: `buildtools/tools/nasm.exe`.

- **x64:** 25 `.asm` files → `.obj` (SHA, AES, BN, EC, GCM)
- **x86:** 19 `.obj.asm` files → `.obj`
- **ARM64:** No ASM (pure C fallback)

Adding new ASM files: Add to `_asm_file_list` in `cmake/OpenSSL.cmake` under the appropriate platform branch.

> **Full OpenSSL patch details and language file info:** [AGENTS-Structure.md](.ai-factory/AGENTS-Structure.md)
