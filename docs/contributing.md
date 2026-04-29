[← Architecture](architecture.md) · [Back to README](../README.md) · [Testing →](testing.md)

# Contributing

Development guide for Far-NetBox. Target audience: AI assistants and human contributors working on the C++17/MSVC/CMake codebase.

## Code Conventions

### Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | `T` + PascalCase | `TSessionData`, `TSecureShell` |
| Methods | PascalCase | `GetSessionData()`, `Connect()` |
| Member variables | `F` + PascalCase | `FSessionData`, `FConfig` |
| Local variables | camelCase | `sessionData`, `configValue` |
| Constants | UPPER_CASE | `MAX_RETRY_COUNT` |

### Formatting

- **Brace style:** Allman (opening brace on new line)
- **Indentation:** 2 spaces (no tabs)
- **Line endings:** CRLF (Windows)
- **Encoding:** UTF-8 without BOM
- **Pointer/reference:** Middle alignment (`int * ptr`)
- **Max line length:** 120 characters
- **No trailing whitespace**

### File Organization

- Include guards: `#pragma once`
- Include order: System headers → Project headers → Local headers
- Extensions: `.h`/`.hpp` for headers, `.cpp` for sources

## Memory Management

- Prefer RAII and smart pointers (`std::unique_ptr`)
- Avoid raw `new`/`delete`
- Check for null before dereferencing

## Error Handling

- Use exceptions for error conditions, not return codes
- Log with `FTerminal->LogEvent()` for debug output
- Use `DebugAssert()` for invariants (fires in debug builds)

## Thread Safety

All Far Manager API calls must execute on the main thread. Worker threads use event-driven waits (`WaitForSingleObject`) instead of busy-waiting (`Sleep` loops).

See `.ai-factory/rules/threading.md` for the full threading convention rules.

## Build Workflow

### Quick Build

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

### CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug, Release, RelWithDebugInfo | Debug | Build configuration |
| `PROJECT_PLATFORM` | x86, x64, ARM64 | Auto-detected | Target architecture |
| `OPT_CREATE_PLUGIN_DIR` | ON, OFF | OFF | Create plugin directory structure |
| `OPT_USE_UNITY_BUILD` | ON, OFF | ON (x86 Release) | Faster compilation |

### Unity Build Gotcha

Symbol redefinition errors indicate unity build is the culprit. Disable:

```cmd
cmake -S . -B build-RelWithDebugInfo -DOPT_USE_UNITY_BUILD=OFF ...
```

### OpenSSL Patches

After updating OpenSSL from upstream (WinSCP), re-apply the patch:

```cmd
git -C libs\openssl-3 apply -p3 0001-openssl-apply-NetBox-patches.patch
```

## Git Workflow

- **Main branch:** `main` (protected)
- **Branch naming:** `feature/description`, `fix/description`, `refactor/description`
- **Commit messages:** Imperative mood, under 72 chars summary
- **Skip CI:** `[skip ci]` in commit message

## Testing in Far Manager

1. Build with `OPT_CREATE_PLUGIN_DIR=ON`
2. Launch Far Manager from platform directory: `Far3_x64\Far.exe`
3. Press `F11` → Plugins → NetBox
4. Test the affected functionality
5. Check log: `%LOCALAPPDATA%\NetBox\netbox.log`

## Anti-Patterns

- Skip protocol inheritance — all protocols must inherit from `TCustomFileSystem`
- Direct third-party changes — modify `libs/` only after confirmation
- Skip build verification — changes must compile without warnings
- No debug output — use `FTerminal->LogEvent()` for tracing
- Skip exception handling — network errors must be caught
- Mix layers — plugin code should not call third-party directly

## See Also

- [Testing](testing.md) — Manual regression testing guide
- [Architecture](architecture.md) — Project structure and patterns
- `.ai-factory/AGENTS.md` — Full AI development guide with deep references
