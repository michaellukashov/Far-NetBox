# AGENTS.md — NetBox Development Guide

> You are an AI coding assistant working on the NetBox project — a **Far Manager plugin** (SFTP/FTP/SCP/WebDAV/S3 client) built in C++17 on top of WinSCP, PuTTY, and FileZilla codebases. Far Manager is a text-mode file manager for Windows; NetBox integrates as a plugin DLL loaded via F11.

## Quick Navigation

| Document | When to read |
|----------|-------------|
| [AGENTS-Overview.md](AGENTS-Overview.md) | Dependencies, troubleshooting |
| [AGENTS-Structure.md](AGENTS-Structure.md) | Adding libs, CMake, OpenSSL, language files |
| [AGENTS-Standards.md](AGENTS-Standards.md) | Writing code — naming, formatting, patterns |
| [AGENTS-Workflows.md](AGENTS-Workflows.md) | Build commands, git, shell rules |

## At a Glance

| Attribute | Value |
|-----------|-------|
| **Language** | C++17 (no extensions) |
| **Compiler** | MSVC (Visual Studio 2022) |
| **Build System** | CMake 3.15+ / Ninja |
| **Platforms** | x86, x64, ARM64 |
| **Base** | WinSCP, PuTTY, FileZilla codebases |

## Tool Selection Guide

| Task | Use this |
|------|----------|
| Read a known file | `read_file` (use `offset`/`limit` for large files) |
| Find files by pattern | `glob` (e.g., `src/**/*.cpp`) |
| Find code by keyword | `grep_search` (e.g., `GetSessionData`) |
| Open-ended / multi-step search | `task` (Explore for quick, general-purpose for deep) |
| Edit code | `edit` — surgical changes only, include 3+ lines context |
| Create new file | `write_file` |
| Run build/test | `bash` (one command per call) |
| Track multi-step work | `todowrite` |
| Ask user a question | `question` |
| Commit changes | skill: `aif-commit` |

## Task Type Decision Tree

```
What kind of task?
├── Fix a bug
│   1. grep_search for error/symbol
│   2. Read relevant code sections
│   3. Minimal fix → build → verify
│   4. Use /aif-fix if available
│
├── Add a feature
│   1. Read AGENTS-Standards.md
│   2. Locate related existing code
│   3. Follow existing patterns exactly
│   4. Use /aif-plan before coding
│
├── Refactor / clean up
│   1. Read AGENTS-Standards.md
│   2. Make minimal changes → build passes
│
├── Write or fix tests
│   1. Read AGENTS-Standards.md (code patterns)
│   2. Use /cpp-testing skill if available
│   3. Build → run tests → verify
│
├── Build / CI issue
│   1. Read AGENTS-Structure.md (CMake)
│   2. Read AGENTS-Workflows.md (build commands)
│   3. Check "Common Build Errors" below
│
└── Documentation
    1. Update relevant AGENTS-*.md file
    2. CRLF line endings, no trailing whitespace
```

## Essential Build Commands

Full commands in [AGENTS-Workflows.md](AGENTS-Workflows.md).

**Standard build (x64 RelWithDebugInfo):**

Create `build-all.bat` in project root:
```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

Run:
```cmd
cmd /c build-all.bat
```

> **IMPORTANT:** Configure and build MUST happen in the same cmd session with vcvarsall.bat environment.

## Common File Locations

| Area | Path | What's there |
|------|------|-------------|
| Plugin entry | `src/NetBox/` | Far Manager plugin interface |
| Protocols | `src/core/` | SSH, FTP, SCP, S3, WebDAV |
| UI / dialogs | `src/windows/` | Far Manager GUI code |
| Base classes | `src/base/` | UnicodeString, Classes |
| Utilities | `src/nbcore/` | Strings, memory, logging |
| Headers | `src/include/` | nbtypes.h, rtti.hpp |
| Resources | `src/resource/` | .rc, .lng, licenses |
| **DO NOT TOUCH** | `libs/` | Third-party: OpenSSL, PuTTY, FileZilla |
| Build config | `CMakeLists.txt` | Root + `cmake/` + `src/CMakeLists.txt` |

## Critical Agent Rules

- **NEVER modify `libs/`** — use patches instead
- **Build must pass with ZERO warnings** (MSVC W4)
- **All files must use CRLF line endings**
- **Don't combine shell commands** with `&&`, `||`, `;`
- **Don't use `2>/dev/null`** on Windows
- **Don't rewrite entire files** — make minimal edits
- **Don't commit secrets** or credentials

## Project-Specific Gotchas

### OpenSSL Patches

After updating OpenSSL from upstream (WinSCP), **re-apply the patch**:
```cmd
cd libs\openssl-3
git apply -p3 0001-openssl-NetBox-patches.patch
```
Without this: Win32 build breaks (`FARPROC` calling convention mismatch in `cryptlib.c`).

### Unity Build

Symbol redefinition errors? Unity build is the culprit. Disable:
```cmd
cmake -S . -B build-... -DOPT_USE_UNITY_BUILD=OFF ...
```

### Plugin Directory

Plugin DLLs go to `Far3_<platform>/Plugins/NetBox/` (not `build-*/src/`). Requires `OPT_CREATE_PLUGIN_DIR=ON`.

## Common Build Errors & Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `'call' is not recognized` | Running in pwsh without `cmd /c` | Use .bat files (see AGENTS-Workflows.md) |
| `vcvarsall.bat not found` | VS2022 not installed or wrong path | Check VS edition (Community/Professional/Enterprise) |
| `Cannot open include file: 'limits.h'` | Environment not set | **Use .bat files with vcvarsall.bat** - never separate configure/build |
| Ninja not found | Not installed | `winget install Ninja-build.ninja` |
| `CMAKE_C_COMPILER not set` (Win32) | x86 env not configured | Use `vcvarsall.bat x86` in .bat file |
| Symbol redefinition | Unity build conflict | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| OpenSSL Win32: `FARPROC` mismatch | Patch not applied | Re-apply patch (see above) |
| Link errors after adding file | Not in CMakeLists.txt | Add to `src/CMakeLists.txt` source list |

> **CRITICAL:** Always run `vcvarsall.bat` and CMake configure/build in the **same cmd session**. See [AGENTS-Workflows.md](AGENTS-Workflows.md) for correct build patterns.

## Far Manager Testing Cycle

1. Build with `OPT_CREATE_PLUGIN_DIR=ON`
2. Launch Far Manager from `Far3_<platform>/`
3. Press F11 → Plugins → NetBox
4. Test the affected functionality
5. Check plugin log for errors (tinylog, see `src/nbcore/logging.cpp`)

## Skills

Use these skills when applicable:
- **cpp-coding-standards** — writing/reviewing C++ code
- **cpp-expert** — complex C++ questions
- **memory-safety-patterns** — RAII, ownership, resource management
- **cpp-testing** — writing/fixing tests
- **aif-review** — code review before commit
- **aif-commit** — conventional commit messages
- **aif-fix** — fix specific bugs
- **aif-plan** — plan features before coding
- **aif-security-checklist** — security review before deploy

## Quality Gates

Check before declaring a task complete:

- [ ] Clean build with zero warnings
- [ ] No modifications to `libs/`
- [ ] CRLF line endings on all modified files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments
- [ ] Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`

**Verify CRLF**: See [AGENTS-Workflows.md](AGENTS-Workflows.md) → "Verify CRLF Line Endings" for pwsh commands.

## When You're Stuck

- **Don't guess** — say "insufficient information" and ask the user
- **Can't find the file?** — use `glob` with broader patterns
- **Can't find the code?** — use `grep_search` for the function/class name
- **Unsure about the approach?** — use `question` or `/aif-explore`
- **Build fails repeatedly?** — try clean reconfigure or read [AGENTS-Workflows.md](AGENTS-Workflows.md)
- **Shell commands failing?** — read [AGENTS-Workflows.md](AGENTS-Workflows.md) → "Shell Script Rules"
- **Something feels wrong?** — stop and tell the user