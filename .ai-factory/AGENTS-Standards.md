# AGENTS-Standards.md — Coding Standards and Patterns

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | `T` + PascalCase | `TSessionData`, `TSecureShell` |
| Methods/Functions | PascalCase | `GetSessionData()`, `Connect()` |
| Member variables | `F` + PascalCase | `FSessionData`, `FConfig` |
| Local variables | camelCase | `sessionData`, `configValue` |
| Constants/Macros | UPPER_CASE | `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT` |
| Enums | PascalCase | `SessionType`, `AuthMethod` |

## Formatting

- **Brace style:** Allman/BSD (opening brace on new line)
- **Indentation:** 2 spaces (no tabs)
- **Line endings:** CRLF (Windows)
- **Encoding:** UTF-8 without BOM
- **Pointer/reference:** Middle alignment (`int * ptr`, `int & ref`)
- **Max line length:** 120 characters
- **No trailing whitespace** in any source or CMake file
- See `.clang-format` for automated formatting

## File Organization

- **Include guards:** `#pragma once`
- **Include order:** System headers → Project headers → Local headers
- **Extensions:** `.h`/`.hpp` for headers, `.cpp` for sources, `.rc`/`.lng` for resources

## Comments

- `//` for single-line comments
- `/* */` for multi-line comments
- `// TODO: description` for future work
- `// FIXME: description` for known issues
- Comments must be clear, grammatically correct English

## Exception Hierarchy

```
Exception (base)
└── ExtException
    ├── ESkipFile
    ├── EOSExtException
    │   ├── ECRTExtException
    │   └── EStreamError
    ├── EFatal
    ├── ETerminate
    ├── ECallbackGuardAbort
    ├── EFCreateError
    └── EFOpenError
```

**Usage:** Throw exceptions for error conditions, not return codes.

## RAII Patterns

**Resource acquisition:** Initialize resources in constructor.
**Release:** Release in destructor.

```cpp
class TFileHandle
{
public:
    TFileHandle(const UnicodeString & FileName)
    {
        FHandle = CreateFileW(FileName.c_str(), ...);
        if (FHandle == INVALID_HANDLE_VALUE)
        {
            throw EFCreateError(FileName, GetLastError());
        }
    }

    ~TFileHandle()
    {
        if (FHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(FHandle);
        }
    }
private:
    HANDLE FHandle;
};
```

**Smart pointers:** Use `std::unique_ptr` for exclusive ownership.

```cpp
#include <memory>

auto handle = std::make_unique<TFileHandle>(fileName);
```

## Threading

- **Main thread:** All Far Manager API calls must be from the main thread
- **Worker threads:** Protocol operations run in background threads
- **Synchronization:** Use critical sections (`CRITICAL_SECTION`) or mutexes for shared data
- **Inter-thread communication:** Use custom message queues or events

**Rule:** Never call Far Manager APIs from worker threads.

## Memory Management

- Prefer RAII and smart pointers
- Avoid raw `new`/`delete`
- Use `nbstr_*` functions for string memory management
- Check for null before dereferencing

## String Handling

- `UnicodeString` for wide strings (user-facing text)
- `AnsiString` for narrow strings when needed
- UTF-8 for file I/O when possible
- Handle encoding conversions explicitly

## Debugging

- `FTerminal->LogEvent()` for debug output (enabled in debug builds)
- `DebugAssert()` for invariants (fires in debug builds)
- Logging via `tinylog` (see `src/nbcore/logging.cpp`)
- **VS debugging:** Generate solution with `-G "Visual Studio 17 2022"`, set Far.exe as debug command

**Log output:** `%LOCALAPPDATA%\NetBox\netbox.log` (production) or `tinylog` output.

## Error Handling

- Use exceptions for error conditions, not return codes
- Log with `FTerminal->LogEvent()` for debug output
- Use `DebugAssert()` for invariants
- Handle network errors gracefully with meaningful messages

**Pattern:**

```cpp
if (result == ERROR)
{
    FTerminal->LogEvent("Operation failed: %s", description.c_str());
    throw EOperationError(description, errorCode);
}
```

## Code Quality Checklist

- [ ] No compiler warnings (MSVC W4)
- [ ] No memory leaks (use RAII)
- [ ] Exception safety
- [ ] Unicode correctness
- [ ] No spelling/grammar errors in comments
- [ ] Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`
