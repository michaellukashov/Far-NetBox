# AGENTS-Workflows.md — Build Commands, Git Workflow, and Shell Rules

> Part of the AGENTS documentation series. See also: [AGENTS.md](AGENTS.md) (entry), [AGENTS-Overview.md](AGENTS-Overview.md), [AGENTS-Structure.md](AGENTS-Structure.md), [AGENTS-Standards.md](AGENTS-Standards.md).
>
> For core principles, task checklist, and agent rules — see [AGENTS.md](AGENTS.md).

## Build Commands

> **Agent execution**: Run build commands via Bash tool with individual calls — do NOT chain with `&&` or use cmd-specific constructs inside pwsh.

### Standard Build (x64 RelWithDebugInfo)

**Step 1: Configure** (adjust VS edition path if needed):
```cmd
cmd /c "call &quot;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat&quot; x86_amd64 && cmake -S . -B build-RelWithDebugInfo -G &quot;Ninja&quot; -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON"
```

**Step 2: Build:**
```cmd
cmd /c "cmake --build build-RelWithDebugInfo -j"
```

### Debug Build x64

**Step 1: Configure:**
```cmd
cmd /c "call &quot;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat&quot; x86_amd64 && cmake -S . -B build-Debug-x64 -G &quot;Ninja&quot; -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON"
```

**Step 2: Build:**
```cmd
cmd /c "cmake --build build-Debug-x64 -j"
```

### Debug Build Win32

Win32 (x86) requires the x86 MSVC compiler. If the environment is not already set up, CMake will fail with "CMAKE_C_COMPILER not set".

**Step 1: Configure:**
```cmd
cmd /c "call &quot;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat&quot; x86 && cmake -S . -B build-Debug-Win32 -G &quot;Ninja&quot; -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON"
```

**Step 2: Build:**
```cmd
cmd /c "cmake --build build-Debug-Win32 -j"
```

**Verify architecture:**
```cmd
dumpbin /headers build-Debug-Win32\src\NetBox.dll | findstr /i "machine"
:: Expected output: "14C machine (x86)" and "32 bit word machine"
```

### Release Build (x86, Unity)

**Step 1: Configure:**
```cmd
cmd /c "call &quot;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat&quot; x86 && cmake -S . -B build-Release-Win32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DOPT_USE_UNITY_BUILD=ON -DOPT_CREATE_PLUGIN_DIR=ON"
```

**Step 2: Build:**
```cmd
cmd /c "cmake --build build-Release-Win32 -j"
```

### Clean Build

**x64 (RelWithDebugInfo):**
```cmd
cmd /c "cmake --build build-RelWithDebugInfo --clean-first -- -j4"
```

**Win32 (x86 Debug):**
```cmd
cmd /c "cmake --build build-Debug-Win32 --clean-first -- -j4"
```

**Full clean reconfigure (nuke and reconfigure):**

Step 1 - Remove build directory:
```cmd
rmdir /s /q build-RelWithDebugInfo
```

Step 2 - Configure:
```cmd
cmd /c "call &quot;C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat&quot; x86_amd64 && cmake -S . -B build-RelWithDebugInfo -G &quot;Ninja&quot; -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON"
```

Step 3 - Build:
```cmd
cmd /c "cmake --build build-RelWithDebugInfo -j"
```

### Verify No Warnings

```cmd
cmd /c "cmake --build build-RelWithDebugInfo --clean-first -- -j4"
```

## Common Build Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `'call' is not recognized` | Running in pwsh without `cmd /c` | Prefix commands with `cmd /c "..."` |
| `vcvarsall.bat not found` | VS2022 not installed or wrong edition | Check VS installation path (Community, Professional, Enterprise) |
| `CMAKE_C_COMPILER not set` (Win32) | x86 env not configured | Run `vcvarsall.bat x86` (not x86_amd64) |
| `Ninja not found` | Not in PATH | Add to PATH: `winget install Ninja-build.ninja` |
| `limits.h: No such file` | VS2022 C++ headers not installed | Reinstall VS2022 with "Desktop development with C++" workload |
| `Symbol redefinition` | Unity build conflict | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Link errors after adding file | Not in CMakeLists.txt | Add to `src/CMakeLists.txt` source list |

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

## Agent Build Execution Rules

**Why these rules matter:**
- The opencode tool runs in a pwsh environment
- Direct `call` commands fail in pwsh
- Chained commands (`cmd1 && cmd2`) fail when run via Bash tool

**Correct patterns for agent:**

```cmd
cmd /c "call &quot;%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat&quot; x86_amd64 && cmake -S . -B build-RelWithDebugInfo -G &quot;Ninja&quot; -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON"
```

```cmd
cmd /c "cmake --build build-RelWithDebugInfo -j"
```

**Always separate configure and build into individual tool calls.**

### Verify CRLF Line Endings

All source, markdown, and CMake files must use CRLF line endings. Verify before committing.

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

### Verify Build with `act` (GitHub Actions locally) — PARTIALLY TESTED

> **Status:** Self-hosted runner mode (`-P windows-2022=-self-hosted`) works.
> Docker mode and full workflow execution have not been thoroughly tested.

Use `act` to run GitHub Actions workflows locally on a self-hosted runner.

**Prerequisites:**
- `act` installed: `winget install nektos.act`
- Self-hosted runner configured and available

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

## Git Workflow

- **Main branch**: `main` (protected)
- **Branch naming**: `feature/description`, `fix/description`, `refactor/description`
- **Commit messages**: Imperative mood, under 72 chars for summary
- **Multi-line commit**: Use `git commit -m "title" -m "body"` instead of heredoc (causes parsing errors)
- **Skip CI**: `[skip ci]` or `[ci skip]` in commit message
- **CI/CD**: GitHub Actions (`.github/workflows/release.yml`), AppVeyor (legacy)

## Agent Rules

See [AGENTS.md](AGENTS.md) → "Critical Agent Rules" for the full list.

Key rules repeated here:
- Never combine shell commands with `&&`, `||`, or `;`
- Never use `2>/dev/null` on Windows
- Never modify third-party code in `libs/`
- Build must succeed with ZERO warnings

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
