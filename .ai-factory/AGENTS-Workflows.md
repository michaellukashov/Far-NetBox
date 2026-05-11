# AGENTS-Workflows.md — Build, Process, and Agent Workflow Documentation

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

This file contains all process and workflow documentation, including step-by-step build instructions, development workflow principles, tool selection, git conventions, shell rules, CI procedures, critical agent notes, and compact session instructions. It is the reference for how to execute tasks correctly in the NetBox project.

---

## Build Commands

> **CRITICAL:** Configure and build **MUST** happen in the **same cmd session** with `vcvarsall.bat` environment. Always use `.bat` files — never chain commands with `&&`.

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

### Script Template

All build scripts follow this pattern:

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

**Platform-specific vcvarsall.bat arguments:**

| Platform | Argument |
|----------|----------|
| x64 | `x86_amd64` |
| x86 (Win32) | `x86` |
| ARM64 | `x86_arm64` |

### Clean Build

```cmd
rmdir /s /q build-RelWithDebugInfo
cmd /c build-x64.bat
```

### Rebuild Without Deleting

```cmd
cmake --build build-RelWithDebugInfo --clean-first -- -j4
```

### Debug Builds

Create a debug script (e.g., `build-debug-x64.bat`):

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-Debug-x64 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Debug-x64 -j
```

### Release Build (x86 with Unity)

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-Release-Win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Release-Win32 -j
```

### Verify Architecture

```cmd
dumpbin /headers build-Debug-Win32\src\NetBox.dll | findstr /i "machine"
:: Expected: "14C machine (x86)" and "32 bit word machine"
```

---

## Testing the Plugin

1. Build with `OPT_CREATE_PLUGIN_DIR=ON` (done by `.bat` scripts)
2. Launch Far Manager: `Far3_x64\Far.exe` (or `Far3_x86`, `Far3_ARM64`)
3. Press `F11` → Plugins → NetBox
4. Test the affected functionality
5. Check log: `%LOCALAPPDATA%\NetBox\netbox.log` or tinylog (`src/nbcore/logging.cpp`)

---

## Debugging in Visual Studio

```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-VS -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
```

Open `build-VS\NetBox.sln`. Set debug command (project properties → Debugging):

| Setting | Value |
|---------|-------|
| Command | `Far3_x64\Far.exe` |
| Working Directory | `Far3_x64\` |

---

## Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `'call' is not recognized` | Running in pwsh without `cmd /c` | Use `.bat` files with `cmd /c` |
| `vcvarsall.bat not found` | VS2022 not installed or wrong edition | Check edition path (Community/Professional/Enterprise) |
| `CMAKE_C_COMPILER not set` (Win32) | Wrong vcvarsall argument | Use `vcvarsall.bat x86` (not `x86_amd64`) |
| `Ninja not found` | Not in PATH | `winget install Ninja-build.ninja` |
| `Cannot open include file: 'cstddef'` | MSVC environment not active | **Use `.bat` files with vcvarsall.bat** |
| `Symbol redefinition` | Unity build conflict | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Link errors after adding file | Not in CMakeLists.txt | Add to `src/CMakeLists.txt` source list |
| Plugin fails to load | Architecture mismatch | Check x86 vs x64 plugin vs Far Manager |

### Standard Headers Not Found

```
fatal error C1083: Cannot open include file: 'cstddef': No such file or directory
fatal error C1083: Cannot open include file: 'cstdlib': No such file or directory
```

**Root cause:** MSVC compiler environment (INCLUDE paths from `vcvarsall.bat`) is not active during build.

**Solution:** ALWAYS run `vcvarsall.bat` and CMake configure/build in the **same cmd session**. Use `.bat` files.

### VS Edition Paths

| Edition | Path |
|---------|------|
| Community | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat` |
| Professional | `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat` |
| Enterprise | `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat` |

---

## Development Workflow

### Core Principles

1. **Read before writing** — Understand existing patterns before modifying code
2. **Edit, don't rewrite** — Make minimal surgical changes
3. **Don't re-read unnecessarily** — Remember files you've already read unless they may have changed
4. **Verify before declaring done** — Build and check your changes
5. **User instructions override this file** — Always follow explicit user direction

### Task Execution Checklist

- [ ] Understand the task and locate relevant files
- [ ] Read existing code to match patterns and conventions
- [ ] Make minimal, focused changes (see [AGENTS-Standards.md](AGENTS-Standards.md) — Surgical Changes)
- [ ] Build succeeds with **zero warnings**
- [ ] No trailing whitespaces introduced
- [ ] No spelling errors in comments
- [ ] Stated assumptions and surfaced tradeoffs (see [AGENTS-Standards.md](AGENTS-Standards.md) — Think Before Coding)
- [ ] Minimal solution — no speculative abstractions (see [AGENTS-Standards.md](AGENTS-Standards.md) — Simplicity First)
- [ ] Defined verifiable success criteria (see [AGENTS-Standards.md](AGENTS-Standards.md) — Goal-Driven Execution)

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

---

## Git Workflow

- **Main branch:** `main` (protected)
- **Branch naming:** `feature/description`, `fix/description`, `refactor/description`
- **Commit messages:** Imperative mood, under 72 chars summary
- **Multi-line commit:** Use `git commit -m "title\nline1\nline2"` instead of heredoc
- **Skip CI:** `[skip ci]` or `[ci skip]` in commit message

---

## Critical Notes for Agents

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

See [AGENTS-Structure.md](AGENTS-Structure.md) for full patch details.

### When Stuck

- **Don't guess** — say "insufficient information" and ask the user
- **Can't find the file?** — use `find` with broader patterns
- **Can't find the code?** — use `grep` for the function/class name
- **Unsure about the approach?** — use `ask` or `/aif-explore`
- **Build fails repeatedly?** — clean reconfigure or read this file → "Common Build Errors"
- **Shell commands failing?** — read this file → "Agent Build Execution Rules"
- **Something feels wrong?** — stop and tell the user

---

## Agent Build Execution Rules

The agent tool runs in a PowerShell environment:

- Direct `call` commands fail in PowerShell
- Chained commands (`cmd1 && cmd2`) fail when run via bash tool
- **HTML entities like `"` break command parsing** — use `.bat` files instead
- **Environment isolation:** Each `cmd /c` call creates a new shell session — `vcvarsall.bat` environment is lost

**Correct patterns:**

✅ **Use `.bat` files:**
```cmd
cmd /c build-x64.bat
```

✅ **Or wrap in cmd /c:**
```cmd
cmd /c "call my-build-script.bat"
```

❌ **DO NOT use inline `&&`:**
```cmd
cmd /c "call vcvarsall.bat x86_amd64 && cmake ..."  # BREAKS due to quote encoding
```

❌ **DO NOT separate configure and build:**
```cmd
cmd /c "call vcvarsall.bat && cmake -S . -B build..."  # Session 1
cmd /c "cmake --build build..."                        # Session 2 — FAILS! Environment lost
```

**Always put vcvarsall.bat + configure + build in ONE `.bat` file.**

> **Never use PowerShell for build operations** — always use `.bat` files with `cmd /c`.

---

## Shell Script Rules

### General

- **Platform:** Windows — use native commands where possible
- **No `&&`, `||`, `;` chaining** — execute each command as a separate tool call
- **No Unix redirections** (`< /dev/null`, `>/dev/null`, `2>/dev/null`) — use `NUL` or omit

### PowerShell

- Use `powershell -Command "..."` for inline commands
- **String quoting:** Double-quotes around `-Command`, single-quotes inside
  - ✅ `powershell -Command "if (Test-Path 'path\to\file') { 'exists' }"`
  - ❌ `powershell -Command 'if (Test-Path "path\to\file") { ... }'`
- Avoid complex pipelines — split into individual calls
- Use `Test-Path` for file checks
- Use `Get-ChildItem` with `-Include` or `-Filter` for listings

### Bash (Git Bash / WSL)

- Only use when POSIX tools are required (`find`, `grep -P`, `sed`, `awk`)
- **Never use** `/dev/null` redirection — Windows uses `NUL`
- **Path separator:** Convert Windows `\` to `/`: `cygpath -u "D:\path"`
- **Line endings:** Output may mix CRLF/LF — be aware when parsing

### Common Pitfalls

| Pitfall | Wrong | Right |
|---------|-------|-------|
| Redirect stderr | `cmd 2>/dev/null` | `cmd 2>nul` (cmd) or omit |
| Chain commands | `cmd1 && cmd2` | Two separate tool calls |
| Complex pwsh pipeline | `ps \| ForEach-Object { ... }` | Individual `powershell -Command` calls |
| Path in pwsh string | `"$env:PATH\file"` | Use explicit paths or `Join-Path` |
| `findstr` with regex | `findstr ".*pattern"` | `findstr /r ".*pattern"` |

---

## Line Endings & BOM

### Verify CRLF

All source, markdown, and CMake files must use CRLF line endings.

### Check BOM

```powershell
$bytes = [System.IO.File]::ReadAllBytes("file.txt")
if ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
    Write-Host "File has BOM"
}
```

### Remove BOM

```powershell
$bytes = [System.IO.File]::ReadAllBytes("file.txt")
$content = [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
[System.IO.File]::WriteAllText("file.txt", $content, [System.Text.Encoding]::UTF8)
```

### Fix LF → CRLF

```powershell
$c = [System.IO.File]::ReadAllText("file.txt")
$c = $c -replace "`r`n", "`n"
$c = $c -replace "`n", "`r`n"
[System.IO.File]::WriteAllText("file.txt", $c, [System.Text.Encoding]::UTF8)
```

---

## Local CI Testing with `act`

Use `act` to run GitHub Actions workflows locally on a self-hosted runner.

```cmd
cmd /c "act -W .github/workflows/release.yml -j create-release -P windows-2022=-self-hosted --use-new-action-cache"
```

| Flag | Purpose |
|------|---------|
| `-W .github/workflows/release.yml` | Workflow file |
| `-j create-release` | Run only this job |
| `-P windows-2022=-self-hosted` | Map runner label to self-hosted |
| `--use-new-action-cache` | Cache actions for faster re-runs |

**Expected:** All builds (x86, x64, ARM64) show `✅ Success`. Upload-artifacts may fail on self-hosted runners without GitHub token — this is expected.

---

## Compact Instructions

> Save key context between sessions to reduce ramp-up time and preserve continuity.

### What to Save

| Item | Description | Suggested Location |
|------|-------------|-------------------|
| **Architectural solutions** | Design decisions, trade-offs, and rationale for non-trivial changes | `.ai-factory/decisions/<ticket-or-topic>.md` |
| **Changed files** | List of files modified in the current session | `.ai-factory/session/changed-files.md` |
| **Current plan** | Active phase plan, milestone, or roadmap item in progress | `.ai-factory/session/current-plan.md` |
| **Tasks** | Pending, in-progress, and completed tasks with acceptance criteria | `.ai-factory/session/tasks.md` |
| **File structure** | Notable directory layout or structural changes discovered | `.ai-factory/session/file-structure.md` |

### How to Maintain

1. **At session end** -- update `changed-files.md` and `tasks.md` with the latest state.
2. **After significant decisions** -- append the architectural rationale to the relevant decision file.
3. **When resuming** -- read `.ai-factory/session/` first to restore context.
4. **Keep concise** -- entries should be bullet-style; avoid duplicating full documentation.

> These compact artifacts are **session aids**, not replacements for full docs in `docs/` or `.ai-factory/`. Prefer linking over copying.
