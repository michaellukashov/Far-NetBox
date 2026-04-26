# AGENTS-Workflows.md — Build Commands, Debugging, and Shell Rules

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

## Build Commands

> **CRITICAL:** Configure and build **MUST** happen in the **same cmd session** with `vcvarsall.bat` environment. Always use `.bat` files — never chain commands with `&&`.

### Platform Scripts

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

## Testing the Plugin

1. Build with `OPT_CREATE_PLUGIN_DIR=ON` (done by `.bat` scripts)
2. Launch Far Manager: `Far3_x64\Far.exe` (or `Far3_x86`, `Far3_ARM64`)
3. Press `F11` → Plugins → NetBox
4. Test the affected functionality
5. Check log: `%LOCALAPPDATA%\NetBox\netbox.log` or tinylog (`src/nbcore/logging.cpp`)

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

## Agent Build Execution Rules

The agent tool runs in a PowerShell environment:

- Direct `call` commands fail in PowerShell
- Chained commands (`cmd1 && cmd2`) fail when run via bash tool
- **HTML entities like `&quot;` break command parsing** — use `.bat` files instead
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

## Git Workflow

- **Main branch:** `main` (protected)
- **Branch naming:** `feature/description`, `fix/description`, `refactor/description`
- **Commit messages:** Imperative mood, under 72 chars summary
- **Multi-line commit:** Use `git commit -m "title\nline1\nline2"` instead of heredoc
- **Skip CI:** `[skip ci]` or `[ci skip]` in commit message

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
