# Project Base Rules

> Auto-detected conventions from codebase analysis. Edit as needed.

## Naming Conventions

- **Files:** PascalCase with `.h`/`.hpp` for headers, `.cpp` for sources (e.g., `SessionData.h`, `SessionData.cpp`)
- **Classes:** `T` prefix + PascalCase (e.g., `TSessionData`, `TSecureShell`, `TCustomFileSystem`)
- **Methods/Functions:** PascalCase (e.g., `GetSessionData()`, `Connect()`, `Initialize()`)
- **Member variables:** `F` prefix + PascalCase (e.g., `FSessionData`, `FConfig`, `FTerminal`)
- **Local variables:** camelCase (e.g., `sessionData`, `configValue`, `filePath`)
- **Constants/Macros:** UPPER_CASE (e.g., `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`, `ADF()`)
- **Enums:** PascalCase (e.g., `SessionType`, `AuthMethod`, `ProtocolType`)

## Module Structure

```
src/
├── base/        # Foundation classes (UnicodeString, Classes, Exceptions)
├── core/        # Protocol implementations (SSH, FTP, SCP, S3, WebDAV)
├── filezilla/   # FileZilla-based FTP engine
├── include/     # Public headers (nbtypes.h, rtti.hpp)
├── nbcore/      # Core utilities (strings, memory, logging)
├── NetBox/      # Far Manager plugin interface
├── PluginSDK/   # Far3 plugin SDK headers
├── resource/    # Resources (.rc, .lng, licenses)
└── windows/     # Windows-specific code (GUI, dialogs)
```

**Module boundaries:**
- Plugin Layer (`src/NetBox/`) → Far Manager API integration
- Core Layer (`src/core/`) → Protocol implementations
- Base Layer (`src/base/`) → Foundation classes
- Third-Party (`libs/`) → **NEVER modify directly**, use patches

## Error Handling

- **Exception-based:** Use exceptions for error conditions
- **Hierarchy:** Inherit from `Exception` base class (`EAbort`, `EInOutError`, `EConvertError`, etc.)
- **Debug logging:** Use `ADF()` macro for debug output
- **Assertions:** Use `DebugAssert()` for invariants
- **Network errors:** Handle gracefully with meaningful messages

## Logging

- **Debug output:** `ADF()` macro (e.g., `ADF("Connection established")`)
- **Assertions:** `DebugAssert(condition, "message")`
- **Integration:** tinylog library (`src/nbcore/logging.cpp`)
- **Log levels:** Configurable via tinylog

## Code Style

- **C++ Standard:** C++17 (strict, no extensions)
- **Brace Style:** Allman/BSD (opening brace on new line)
- **Indentation:** 2 spaces (no tabs)
- **Line endings:** CRLF (Windows)
- **Pointer/reference:** Middle alignment (`int * ptr`, `int & ref`)
- **Max line length:** 120 characters
- **No trailing whitespaces** in source or CMake files
- **Include guards:** `#pragma once`

## Build Patterns

- **Build types:** Debug, Release, RelWithDebugInfo
- **Unity builds:** Enabled for x86 Release only
- **Platform detection:** Auto-detected via `PlatformDetection.cmake`
- **Output:** `Far3_<platform>/Plugins/NetBox/`

## Critical Rules

1. **NEVER modify third-party code** in `libs/` directly — use patches
2. **Always read existing code** before writing to match patterns
3. **Make minimal, surgical changes** — don't rewrite working code
4. **Build must succeed with NO warnings** (MSVC W4)
5. **No spelling/grammar errors** in comments (common typos: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`)
6. **Preserve WinXP compatibility** — avoid modern Windows APIs
7. **Maintain backward compatibility** with existing configurations

## Testing

- **Manual testing:** Required for affected functionality
- **Build verification:** Clean build with no warnings
- **No automated tests:** Currently no test framework in place
