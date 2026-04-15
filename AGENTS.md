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
- [ ] Build succeeds with no warnings (`cmake --build build-RelWithDebugInfo --clean-first -- -j4`)
- [ ] No trailing whitespaces introduced
- [ ] No spelling/grammar errors in comments

### Before Committing

- [ ] Clean build completes without warnings
- [ ] Changes follow naming conventions (see below)
- [ ] No modifications to third-party code in `libs/`
- [ ] Manual testing of affected functionality

## Tool Selection Guide

| Task | Use this |
|------|----------|
| Read a known file | `read` (use `offset`/`limit` for large files) |
| Find files by pattern | `glob` (e.g., `src/**/*.cpp`) |
| Find code by keyword | `grep_search` (e.g., `GetSessionData`) |
| Open-ended / multi-step search | `agent` (Explore for quick, general-purpose for deep) |
| Edit code | `edit` — surgical changes only, include 3+ lines context |
| Create new file | `write_file` |
| Run build/test | `bash` (one command per call) |
| Track multi-step work | `todo_write` |
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
git -C libs\openssl-3 apply -p3 0001-openssl-NetBox-patches.patch
```
See [AGENTS-Structure.md](AGENTS-Structure.md) → "OpenSSL Patch Application" for patch application instructions.

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

> **CRITICAL:** Always run `vcvarsall.bat` and CMake configure/build in the **same cmd session**. See [AGENTS-Workflows.md](AGENTS-Workflows.md) → "Common Build Errors" for correct build patterns.

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
- [ ] Plugin DLLs in `Far3_<platform>/Plugins/NetBox/` (not `build-*/src/`)
- [ ] CRLF line endings on all modified files
- [ ] No BOM (UTF-8 without BOM) in all text files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments
See [AGENTS-Standards.md](AGENTS-Standards.md) → "Quality Gates" for common typo checks.


## AI Context Files

| File | Purpose |
|------|---------|
| `AGENTS.md` | This file — project structure map and AI agent guide |
| `.ai-factory/DESCRIPTION.md` | Project specification and tech stack |
| `.ai-factory/ARCHITECTURE.md` | Architecture decisions and guidelines |
| `.ai-factory/config.yaml` | AI Factory configuration (paths, workflow, git) |
| `.ai-factory/rules/base.md` | Auto-detected project conventions |
| `.ai-factory/skill-context/aif-commit/SKILL.md` | Project-specific commit conventions |

| `.ai-factory/RULES.md` | Enforced coding rules and conventions |

## When You're Stuck

- **Don't guess** — say "insufficient information" and ask the user
- **Can't find the file?** — use `glob` with broader patterns
- **Can't find the code?** — use `grep_search` for the function/class name
- **Unsure about the approach?** — use `question` or `/aif-explore`
- **Build fails repeatedly?** — try clean reconfigure or read [AGENTS-Workflows.md](AGENTS-Workflows.md)
- **Shell commands failing?** — read [AGENTS-Workflows.md](AGENTS-Workflows.md) → "Shell Script Rules"
- **Something feels wrong?** — stop and tell the user

## Documentation Maintenance Checklist

Before submitting documentation changes, verify:

- [ ] All cross-references are valid
- [ ] No duplicate content exists
- [ ] All code blocks use correct syntax highlighting
- [ ] All paths are absolute and use forward slashes
- [ ] All references to tools use correct names (e.g., `read` not `read_file`)
- [ ] All formatting follows the style guide in `.ai-factory/DOCUMENTATION.md`
- [ ] Changes are reviewed by at least one other team member
- [ ] Documentation is updated when code changes
- [ ] Outdated or redundant information is removed
- [ ] New documentation is added when new features are introduced


## Documentation Review Process

All documentation changes must follow this review process:

1. **Submit for review** - Create a pull request with documentation changes
2. **Assign reviewer** - Assign at least one team member to review
3. **Verify compliance** - Reviewer checks against `.ai-factory/DOCUMENTATION.md`
4. **Check cross-references** - Verify all links are valid
5. **Confirm consistency** - Ensure no duplicate content exists
6. **Approve or request changes** - Reviewer either approves or requests changes
7. **Merge** - Only merge after approval

Documentation changes that don't follow this process will be rejected.
