---
name: powershell-best-practices
description: >-
  PowerShell (powershell/pwsh) and cmd.exe best practices for Windows development.
  Use when running shell commands on Windows to avoid common errors.
argument-hint: "[mode]"
disable-model-invocation: false
user-invocable: true
allowed-tools: Bash
context: fork
---

# PowerShell Best Practices

Follow these rules when executing shell commands on Windows.

## powershell vs pwsh

| Executable | Version | Path |
|------------|---------|-----|
| `powershell` | Windows PowerShell 5.1 | System32\WindowsPowerShell\v1.0 |
| `pwsh` | PowerShell Core 7+ | User-installed |

Use `powershell` for maximum compatibility. Use `pwsh` when PowerShell 7+ features needed.

## When to Use What

| Task | Use | Why |
|------|-----|-----|
| Build operations | `.bat` files + `cmd /c` | vcvarsall.bat environment preserved |
| VSDeveloper command prompt | Direct cmd commands | Environment already set |
| File checks, simple queries | `pwsh -Command` | Quick inline commands |
| Complex scripts | `.ps1` file + `pwsh -ExecutionPolicy Bypass -File` | Better escaping support |

## Never Use Directly

- ❌ `call` — only works in cmd.exe, fails in pwsh
- ❌ `cmd && cmake` — environment lost after first cmd
- ❌ `cmake ... && cmake --build` — chained commands break in pwsh

## Use cmd /c for Builds

```bash
cmd /c build-all.bat
cmd /c build-x64.bat
cmd /c "cmake --build build-RelWithDebugInfo --clean-first -- -j4"
```

**Why:** Each `cmd /c` spawns a fresh cmd.exe with vcvarsall.bat environment intact.

## PowerShell Inline Commands

### String Quoting Rules

```bash
# Double-quotes around -Command, single-quotes inside
✅ pwsh -Command "if (Test-Path 'path\to\file') { 'exists' }"

# Never use single-quotes around -Command
❌ pwsh -Command 'if (Test-Path "path\to\file") { ... }'
```

### Simple File Checks

```bash
# Test file existence
pwsh -Command "Test-Path 'D:\Projects\NetBox\NetBox-dev\README.md'"

# Get file content (limit lines)
pwsh -Command "Get-Content 'D:\Projects\NetBox\NetBox-dev\AGENTS.md' | Select-Object -First 10"
```

### Run Script Files

For complex scripts, use `.ps1` files:

```bash
# Create script
@'
$ErrorActionPreference = "SilentlyContinue"
Get-ChildItem "D:\Projects\NetBox\NetBox-dev\*.md" -Name | Select-Object -First 3
'@ | Out-File test.ps1 -Encoding utf8

# Run
pwsh -ExecutionPolicy Bypass -File test.ps1

# Clean up
del test.ps1
```

### Avoid Pipeline Variables ($_)

```bash
# BAD — $_ gets stripped in -Command
pwsh -Command "Get-Process | Where-Object { $_.CPU -gt 0 }"

# OK — no pipeline variable
pwsh -Command "Get-Date"

# For complex pipelines, use .ps1 file
```

## Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `'call' is not recognized` | Running in pwsh, not cmd | Use `cmd /c` prefix |
| `C1083: Cannot open include file: 'limits.h'` | vcvarsall.bat environment lost | Run configure + build in SAME .bat file |
| `C1083: Cannot open include file: 'stdio.h'` | vcvarsall.bat environment lost | Run configure + build in SAME .bat file |
| `PredictionViewStyle` error | PSReadLine profile incompatibility (both pwsh/powershell) | Profile uses newer parameter, ignore or fix |
| `ParameterBindingException` | Profile error | Commands still work, ignore profile error |
| Mixed charset output | Output encoding not UTF-8 | Use `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8` |
| `$_` gets stripped | Pipeline variable fails in inline -Command | Avoid `Where-Object` with `$_`, use `ForEach-Object` file instead |
| ObjectNotFound: `.CPU` | `$_` garbled when passed via cmd | Do NOT use `$_` in inline commands |
| Environment lost | `cmd && cmd2` in pwsh | Two separate `cmd /c` calls |
| Quote breaking | Mixed quote types | Use `"..."` outside, `'...'` inside |
| Path not found | Backslashes escaped | Use forward slashes or `'..\\..'` |

## Critical: vcvarsall.bat Must Stay in Session

**Problem:** vcvarsall.bat sets environment variables (INCLUDE, LIB, PATH). These exist only in the cmd.exe session that runs it.

**WRONG:**
```bash
cmd /c build-x64.bat          # Session 1: configure
cmd /c "cmake --build ..."    # Session 2: BUILD FAILS! Environment lost
```

**RIGHT — configure and build in same session:**
```bash
cmd /c build-x64.bat          # Runs both configure + build
```

The .bat file must contain BOTH cmake commands:

```bat
@echo off
REM NOTE: Use find-vs.bat or vswhere.exe for dynamic discovery instead of hardcoded paths
call "%~dp0find-vs.bat"
if %errorlevel% neq 0 (
    echo ERROR: Failed to locate Visual Studio
    exit /b 1
)
call "%VCVARSALL%" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo
cmake --build build-RelWithDebugInfo -j
```

Run with `cmd /c build-x64.bat` — not `build-x64.bat` directly.

## Path Handling

```bash
# Use forward slashes in PowerShell
✅ pwsh -Command "Test-Path 'D:/Projects/NetBox/README.md'"

# Or double escaped backslashes
✅ pwsh -Command "Test-Path 'D:\\Projects\\NetBox\\README.md'"
```

## VS Edition Detection

Different VS editions (Community, Professional, Enterprise, BuildTools) install to different paths.
**Always prefer `vswhere.exe` (ships with every VS install) for dynamic discovery.**

### Primary — vswhere.exe (Recommended)

```bat
@echo off
for /f "delims=" %%a in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul') do (
    set "VSINSTALLPATH=%%a"
)
if not defined VSINSTALLPATH (
    echo ERROR: Visual Studio with C++ tools not found. Install VS 2022 with "Desktop development with C++" workload.
    exit /b 1
)
call "%VSINSTALLPATH%\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
```

### Fallback — Edition scan (no vswhere)

If `vswhere.exe` is unavailable, scan common edition paths:

```bat
@echo off
for /f "delims=" %%a in ('dir /b "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Auxiliary\Build\vcvarsall.bat" 2^>nul') do (
    call "C:\Program Files\Microsoft Visual Studio\2022\%%a\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
    goto :eof
)
echo ERROR: VS2022 not found
```

### Common install paths by edition

| Edition | Typical Path |
|---------|-------------|
| Community | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| Professional | `C:\Program Files\Microsoft Visual Studio\2022\Professional` |
| Enterprise | `C:\Program Files\Microsoft Visual Studio\2022\Enterprise` |
| BuildTools | `C:\Program Files\Microsoft Visual Studio\2022\BuildTools` |

## Platform Targets

| Target | vcvarsall.bat arg |
|--------|-----------------|
| x64 | `x86_amd64` |
| x86 (Win32) | `x86` |
| ARM64 | `x86_arm64` |

## Quick Reference

```bash
# Build x64 (requires cmd)
cmd /c build-x64.bat

# Build x86 (Win32)
cmd /c build-x86.bat

# Run PowerShell script
pwsh -ExecutionPolicy Bypass -File "D:\Projects\NetBox\check_bom.ps1"

# Quick inline check
pwsh -Command "Test-Path 'D:\Projects\NetBox\README.md'"

# List files
pwsh -Command "Get-ChildItem 'D:\Projects\NetBox' -Name"
```