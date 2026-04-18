# AGENTS-Workflows.md — Build Commands, Git Workflow, and Shell Rules

> Part of the AGENTS documentation series. See also: [AGENTS.md](../AGENTS.md) (entry), [AGENTS-Overview.md](AGENTS-Overview.md), [AGENTS-Structure.md](AGENTS-Structure.md), [AGENTS-Standards.md](AGENTS-Standards.md).
>
> For core principles, task checklist, and agent rules — see [AGENTS.md](../AGENTS.md).
> Version: 1.3.0 | Last updated: 2026-04-16

## Build Commands

> **Agent execution**: Use `.bat` files for all build operations. Each `cmd /c` creates a new shell session, losing `vcvarsall.bat` environment. Never chain with `&&` or use cmd-specific constructs inside pwsh.

### Standard Build (x64 RelWithDebugInfo)

> **CRITICAL:** Configure and build **MUST** happen in the **same cmd session** with `vcvarsall.bat` environment. 
> If you run configure in one session and build in another, you'll get "Cannot open include file" errors.

**Recommended approach - use `build-all.bat` script:**

Create `build-all.bat` in project root:
```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

**Run build:**
```cmd
cmd /c build-all.bat
```

> **NOTE:** Do NOT use `cmd /c "call vcvarsall.bat && cmake ... && cmake --build ..."` — the HTML encoding of quotes will break. Use .bat files instead.

### Debug Build x64

Create `build-debug-x64.bat`:
```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-Debug-x64 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Debug-x64 -j
```

**Run:**
```cmd
cmd /c build-debug-x64.bat
```

### Debug Build Win32

Win32 (x86) requires the x86 MSVC compiler.

Create `build-debug-win32.bat`:
```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-Debug-Win32 -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Debug-Win32 -j
```

**Run:**
```cmd
cmd /c build-debug-win32.bat
```

**Verify architecture:**
```cmd
dumpbin /headers build-Debug-Win32\src\NetBox.dll | findstr /i "machine"
:: Expected output: "14C machine (x86)" and "32 bit word machine"
```

### Release Build (x86, Unity)

Create `build-release-win32.bat`:
```bat
@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake -S . -B build-Release-Win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-Release-Win32 -j
```

**Run:**
```cmd
cmd /c build-release-win32.bat
```

### Clean Build

**Full clean reconfigure:**

Create `build-clean.bat`:
```bat
@echo off
rmdir /s /q build-RelWithDebugInfo
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build build-RelWithDebugInfo -j
```

**Run:**
```cmd
cmd /c build-clean.bat
```

**Clean build with --clean-first** (rebuilds without deleting build dir):
```cmd
cmd /c "cmake --build build-RelWithDebugInfo --clean-first -- -j4"
```

> **NOTE:** Third-party libraries in `libs/` may produce warnings - these are expected and acceptable since we cannot modify them directly (use patches instead).

### Platform-Specific Build Scripts

Pre-configured build scripts are available in the project root for all supported platforms:

| Script | Platform | Configuration |
|--------|----------|---------------|
| `build-x64.bat` | x64 (AMD64) | RelWithDebugInfo |
| `build-x86.bat` | x86 (Win32) | RelWithDebugInfo |
| `build-arm64.bat` | ARM64 | RelWithDebugInfo |

**Usage:**
```cmd
cmd /c build-x64.bat
cmd /c build-x86.bat
cmd /c build-arm64.bat
```

These scripts configure and build in a single session, setting `OPT_CREATE_PLUGIN_DIR=ON` to output DLLs to `Far3_<platform>/Plugins/NetBox/`.

### Testing the Plugin in Far Manager

After successful build:

1. **Launch Far Manager** from the platform directory:
   - x64: `Far3_x64/Far.exe`
   - x86: `Far3_x86/Far.exe`
   - ARM64: `Far3_ARM64/Far.exe`

2. **Open NetBox**: Press `F11` → Plugins → NetBox

3. **Test functionality**: Create a connection or open a session

4. **Check logs** for errors:
   - Production: `%LOCALAPPDATA%\NetBox\netbox.log`
   - Debug: Use tinylog output (see `src/nbcore/logging.cpp`)

### Debugging in Visual Studio

To debug with breakpoints:

1. **Generate VS solution** (instead of Ninja):
   ```bat
   @echo off
   call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
   cmake -S . -B build-VS -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
   ```

2. **Open solution** in Visual Studio:
   ```
   build-VS\NetBox.sln
   ```

3. **Set debug command**: Right-click NetBox project → Properties → Debugging
   - Command: `D:\Projects\NetBox\NetBox-dev\Far3_x64\Far.exe`
   - Working Directory: `D:\Projects\NetBox\NetBox-dev\Far3_x64`

4. **Set breakpoints** in source code and press F5 to debug

## Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `'call' is not recognized` | Running in pwsh without `cmd /c` | Use .bat files or prefix with `cmd /c` |
| `vcvarsall.bat not found` | VS2022 not installed or wrong edition | Check VS installation path (Community, Professional, Enterprise) |
| `CMAKE_C_COMPILER not set` (Win32) | x86 env not configured | Use `vcvarsall.bat x86` (not x86_amd64) |
| `Ninja not found` | Not in PATH | Add to PATH: `winget install Ninja-build.ninja` |
| `Cannot open include file: 'cstddef'` | Environment not set properly | **Use .bat files with vcvarsall.bat** - see build examples above |
| `Symbol redefinition` | Unity build conflict | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Link errors after adding file | Not in CMakeLists.txt | Add to `src/CMakeLists.txt` source list |

### Critical: Standard Headers Not Found Errors

If you see errors like:
```
fatal error C1083: Cannot open include file: 'cstddef': No such file or directory
fatal error C1083: Cannot open include file: 'cstdlib': No such file or directory
fatal error C1083: Cannot open include file: 'stdio.h': No such file or directory
fatal error C1083: Cannot open include file: 'limits.h': No such file or directory
```

**Root cause:** The MSVC compiler environment (INCLUDE paths from `vcvarsall.bat`) is not active during build.

**Solution:** ALWAYS run `vcvarsall.bat` and CMake configure/build in the **same cmd session**. Use the `.bat` file approach shown above.

**DO NOT:**
- Run configure in one cmd session and build in another
- Use inline commands with `&&` chaining (HTML encoding breaks quotes)
- Try to set INCLUDE manually - use `vcvarsall.bat` instead

### Visual Studio Edition Detection

Different VS editions install to different paths. Use the one installed:

| Edition | Path |
|--------|------|
| Community | `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat` |
| Professional | `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat` |
| Enterprise | `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat` |

**Auto-detect in build command:**
```cmd
for /f "delims=" %a in ('dir /b "C:\Program Files\Microsoft Visual Studio\2022\*\VC\Auxiliary\Build\vcvarsall.bat" 2^>nul') do @(
    call "C:\Program Files\Microsoft Visual Studio\2022\%a\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64 && cmake ...
)
```

Or set `VS170COMNTOOLS` environment variable to point to the correct edition.

This code automatically detects the installed VS edition by searching for vcvarsall.bat in all possible paths. It's recommended to use this approach in build scripts to ensure compatibility across different VS installations.



⚠️ **Never use PowerShell for build operations** — always use .bat files with `cmd /c` to ensure proper environment setup.

## Agent Build Execution Rules

**Why these rules matter:**
- The opencode tool runs in a pwsh environment
- Direct `call` commands fail in pwsh
- Chained commands (`cmd1 && cmd2`) fail when run via Bash tool
- **HTML entities like `&quot;` break command parsing** — use .bat files instead
- **Environment isolation:** Each `cmd /c` call creates a new shell session — `vcvarsall.bat` environment is lost

**Correct patterns for agent:**

✅ **USE .bat files** (recommended):
```cmd
cmd /c build-all.bat
```

✅ **Or use cmd /c with .bat wrapper:**
```cmd
cmd /c "call my-build-script.bat"
```

❌ **DO NOT use inline commands with &&:**
```cmd
cmd /c "call vcvarsall.bat x86_amd64 && cmake ..."  // BREAKS due to quote encoding
```

❌ **DO NOT separate configure and build into different sessions:**
```cmd
cmd /c "call vcvarsall.bat && cmake -S . -B build..."  // Session 1 - configure
cmd /c "cmake --build build..."  // Session 2 - build - FAILS! Environment lost
```

**Always put vcvarsall.bat + configure + build in ONE .bat file.**

### Verify CRLF Line Endings

All source, markdown, and CMake files must use CRLF line endings. Verify before committing.

**Check for BOM** (script `check_bom.ps1`):
```powershell
$ErrorActionPreference = "SilentlyContinue"
Get-ChildItem -Path "D:\Projects\NetBox\NetBox-dev" -Filter "*.md" -Recurse | Where-Object { $_.FullName -notlike "*\libs\*" } | ForEach-Object {
    $bytes = [System.IO.File]::ReadAllBytes($_.FullName)
    $bom = "{0:X2} {1:X2} {2:X2}" -f $bytes[0], $bytes[1], $bytes[2]
    if ($bom -eq "EF BB BF") {
        Write-Output "BOM: $($_.FullName)"
    }
}
```

**Check for BOM:**
```cmd
powershell -ExecutionPolicy Bypass -File "D:\Projects\NetBox\NetBox-dev\check_bom.ps1"
```

**Remove BOM** (for files that have it):
```powershell
$files = Get-ChildItem -Path "D:\Projects\NetBox\NetBox-dev" -Filter "*.md" -Recurse
foreach ($f in $files) {
    $bytes = [System.IO.File]::ReadAllBytes($f.FullName)
    if ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        $content = [System.Text.Encoding]::UTF8.GetString($bytes, 3, $bytes.Length - 3)
        [System.IO.File]::WriteAllText($f.FullName, $content, [System.Text.Encoding]::UTF8)
        Write-Host "Removed BOM: $($f.Name)"
    }
}
```

**Create a check script** (`check_crlf.ps1`):
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$files = @("AGENTS.md","AGENTS-Workflows.md","AGENTS-Standards.md","AGENTS-Overview.md")
foreach ($f in $files) {
    $path = Join-Path "D:\Projects\NetBox\NetBox-dev" $f
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $lf = 0
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        if ($bytes[$i] -eq 10 -and ($i -eq 0 -or $bytes[$i-1] -ne 13)) { $lf++ }
    }
    if ($lf -gt 0) { Write-Host "$f : LF=$lf (BROKEN)" -ForegroundColor Red } else { Write-Host "$f : OK" }
}
```

**Run check:**
```cmd
powershell -ExecutionPolicy Bypass -File "D:\Projects\NetBox\NetBox-dev\check_crlf.ps1"
```

**Fix LF → CRLF** (`fix_crlf.ps1`):
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$files = @("AGENTS.md","AGENTS-Workflows.md","AGENTS-Standards.md","AGENTS-Overview.md")
foreach ($path in $files) {
    $fullPath = Join-Path "D:\Projects\NetBox\NetBox-dev" $path
    $c = [System.IO.File]::ReadAllText($fullPath)
    $c = $c -replace "`r`n", "`n"
    $c = $c -replace "`n", "`r`n"
    [System.IO.File]::WriteAllText($fullPath, $c, [System.Text.Encoding]::UTF8)
    Write-Host "Fixed: $path"
}
```

**Run fix:**
```cmd
powershell -ExecutionPolicy Bypass -File "D:\Projects\NetBox\NetBox-dev\fix_crlf.ps1"
```

### Verify Build with `act` (GitHub Actions locally) — TESTED

> **Status:** Self-hosted runner mode (`-P windows-2022=-self-hosted`) works. Build and pack succeed; upload-artifacts fails (expected, requires GitHub token).

Use `act` to run GitHub Actions workflows locally on a self-hosted runner.

**Prerequisites:**
- `act` installed: `winget install nektos.act` (or run via `cmd /c "where act"`)
- Self-hosted runner configured and available

**Test release workflow (create-release job):**

```cmd
cmd /c "act -W .github/workflows/release.yml -j create-release -P windows-2022=-self-hosted --use-new-action-cache"
```

**Flags explained:**
| Flag | Purpose |
|------|---------|
| `-W .github/workflows/release.yml` | Workflow file to run |
| `-j create-release` | Run only the `create-release` job |
| `-P windows-2022=-self-hosted` | Map `windows-2022` runner label to self-hosted |
| `--use-new-action-cache` | Use action caching for faster re-runs |

**Expected output:** All builds (x86, x64, ARM64) should show `✅ Success - Main Build` and `✅ Success - Main Pack artifacts`. The final "Upload artifacts" step may fail with `ACTIONS_RUNTIME_TOKEN` error — this is expected on self-hosted runners without a GitHub token and does not indicate build failure.

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Imperative mood, under 72 chars for summary
- **Multi-line commit**: Use `git commit -m "title" -m "body"` instead of heredoc (causes parsing errors)
- **Skip CI**: `[skip ci]` or `[ci skip]` in commit message
- **CI/CD**: GitHub Actions (`.github/workflows/release.yml`), AppVeyor (legacy)

## Agent Rules

See [AGENTS.md](../AGENTS.md) → "Critical Agent Rules" for the full list.

Key rules repeated here:
- Never combine shell commands with `&&`, `||`, or `;`
- Never use Unix-style redirections (`< /dev/null`, `>/dev/null`, `2>/dev/null`) on Windows
- Never modify third-party code in `libs/`
- Build must succeed with ZERO warnings

## Shell Script Rules (Bash / PowerShell)

### General

- **Platform:** Windows (`cmd.exe`) — use native commands where possible
- **Prefer:** Built-in commands (`dir`, `findstr`, `robocopy`, `powershell -Command`)
- **Avoid:** Unix-style redirections (`< /dev/null`, `>/dev/null`, `2>/dev/null`) — not supported on Windows
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
- **Never use:** any `/dev/null` redirection — Windows uses `NUL` (use `2>nul`, `>NUL`, or `< NUL` in cmd; or omit)
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
