# Project Base Rules

> Auto-detected conventions from codebase analysis. Edit as needed.
## File Validation

 - Always re-read file before trying to modify it

## Core Principles

 - **Read before writing** - Understand existing patterns before modifying code
 - **Edit, don't rewrite** - Make minimal surgical changes to existing files
 - **Don't re-read unnecessarily** - Remember files you've already read unless they may have changed
 - **Verify before declaring done** - Build and check your changes
 - **Be concise** - No fluff, no summaries, just the work
 - **User instructions override this file** - Always follow explicit user direction

## Shell Command Conventions

- **Platform:** Windows — use native commands (`cmd.exe` built-ins) where possible
- **Never use Unix-style redirections** (`< /dev/null`, `>/dev/null`, `2>/dev/null`) — Windows uses `NUL` (`2>nul`, `>NUL`, `< NUL`) or omit
- **Never chain commands** with `&&`, `||`, `;` — each command must be a separate tool call
- **Prefer PowerShell** for complex operations, but ensure proper quoting
- **Path handling:** Use backslashes for Windows paths; in Bash, convert with `cygpath -u` if needed

## Markdown rules

Follow Markdown Linting rules
MD009: No trailing spaces
MD022: Headings surrounded by blank lines
MD031: Blank lines around fenced code blocks
MD032: Blank lines around lists
MD047: Files end with single blank line
MD055: Bordered tables
MD056: Tables have ≥2 columns
MD060: Consistent table column style
End of file: Exactly one blank line
Headings: Separated from content by one blank line

## Usage Effectiveness & Best Practices

```markdown
# ❌ Bad Practice (Destructive & Messy)
Deleting plan files when the task is done without saving the history.
Dumping thousands of lines of raw logs into `CONTEXT.md`.

# ✅ Good Practice (Preservative & Structured)
1. Read the plan file and `temp-reports/`.
2. Write a summary to `./ai-factory/storage/archive/{YY-MM-DD}-{feature-name}.md`.
4. Clear the `actions/` folder for the next task.
```

## Skills

- use skill `caveman` if available

## Naming Conventions

- **Files:** PascalCase with `.h`/`.hpp` for headers, `.cpp` for sources (e.g., `SessionData.h`, `SessionData.cpp`)
- **Classes:** `T` prefix + PascalCase (e.g., `TSessionData`, `TSecureShell`, `TCustomFileSystem`)
- **Methods/Functions:** PascalCase (e.g., `GetSessionData()`, `Connect()`, `Initialize()`)
- **Member variables:** `F` prefix + PascalCase (e.g., `FSessionData`, `FConfig`, `FTerminal`)
- **Local variables:** camelCase (e.g., `sessionData`, `configValue`, `filePath`)
- **Loop counters:** Uppercase single letter (e.g., `I`, `J`, `K` — `for (int32_t I = 0; ...)`)
- **Constants/Macros:** UPPER_CASE (e.g., `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT`)
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
- **Debug logging:** Use `FTerminal->LogEvent()` for debug output
- **Assertions:** Use `DebugAssert()` for invariants
- **Network errors:** Handle gracefully with meaningful messages

## Logging

- **Debug output:** `FTerminal->LogEvent()` (e.g., `FTerminal->LogEvent("Connection established")`)
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
8. **Use `nb::vector_t<T>` instead of `std::vector<T>`** — NetBox provides a custom-allocator alias (`nb::vector_t`) in `<nbtypes.h>` that is used throughout the codebase. Prefer it over `std::vector` for consistency and to ensure proper allocator integration. Remove `#include <vector>` when it is no longer needed.
9. **Use `UnicodeString` and `AnsiString` instead of `std::wstring` / `std::string`** — NetBox provides its own string types that are used consistently across the codebase. Prefer them over standard C++ string types for API compatibility and to avoid type mismatches with Far Manager and protocol layers.
10. **Modernize legacy C types to fixed-width C++ types** — Prefer `uint8_t` over `unsigned char`, `uint32_t` over `unsigned int`, and `int32_t` over `int` for platform-independent, self-documenting code. Apply this when touching existing code that uses legacy C-style integer types, especially in cryptography, protocol, and binary handling modules.
11. **UnicodeString/AnsiString index and iterator positions start at 1** — NetBox string types use 1-based indexing (`operator[]`, `SubString`, iterators). Accessing index `0` or calling `SubString(1, N)` on an empty string throws `Exception("Index is out of range")`. Always guard with `!Str.IsEmpty()` and `Str.Length() > 0` before 1-based operations. Loop counters over characters must start at `1`, not `0`.

- **Manual testing:** Required for affected functionality
- **Build verification:** Clean build with no warnings
- **No automated tests:** Currently no test framework in place
