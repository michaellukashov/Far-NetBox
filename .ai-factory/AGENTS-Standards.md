# AGENTS-Standards.md — Coding Standards and Behavioral Principles

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

This file contains all formatting and code standards, naming conventions, threading rules, memory management patterns, error handling patterns, anti-patterns, quality gates, and AI behavioral principles that agents must follow when generating or modifying code and documentation.

---

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | `T` + PascalCase | `TSessionData`, `TSecureShell` |
| Methods/Functions | PascalCase | `GetSessionData()`, `Connect()` |
| Member variables | `F` + PascalCase | `FSessionData`, `FConfig` |
| Local variables | camelCase | `sessionData`, `configValue` |
| Loop counters | Uppercase single letter | `I`, `J`, `K` |
| Constants/Macros | UPPER_CASE | `MAX_RETRY_COUNT`, `DEFAULT_TIMEOUT` |
| Enums | PascalCase | `SessionType`, `AuthMethod` |

---

## Formatting

- **Brace style:** Allman/BSD (opening brace on new line)
- **Indentation:** 2 spaces (no tabs)
- **Line endings:** CRLF (Windows)
- **Encoding:** UTF-8 without BOM
- **Pointer/reference:** Middle alignment (`int * ptr`, `int & ref`)
- **Max line length:** 120 characters
- **No trailing whitespace** in any source or CMake file
- See `.clang-format` for automated formatting

---

## File Organization

- **Include guards:** `#pragma once`
- **Include order:** System headers → Project headers → Local headers
- **Extensions:** `.h`/`.hpp` for headers, `.cpp` for sources, `.rc`/`.lng` for resources

---

## Comments

- `//` for single-line comments
- `/* */` for multi-line comments
- `// TODO: description` for future work
- `// FIXME: description` for known issues
- Comments must be clear, grammatically correct English

---

## Memory Management

- Prefer RAII and smart pointers (`std::unique_ptr` for exclusive ownership)
- Avoid raw `new`/`delete`
- Check for null before dereferencing
- Use `nbstr_*` functions for string memory management

---

## String Handling

- `UnicodeString` for wide strings (user-facing text)
- `AnsiString` for narrow strings when needed
- UTF-8 for file I/O when possible
- Handle encoding conversions explicitly

---

## Error Handling

- Use exceptions for error conditions, **not** return codes
- Log with `FTerminal->LogEvent()` for debug output
- Use `DebugAssert()` for invariants (fires in debug builds)
- Handle network errors gracefully with meaningful messages

**Pattern:**

```cpp
if (result == ERROR)
{
    FTerminal->LogEvent("Operation failed: %s", description.c_str());
    throw EOperationError(description, errorCode);
}
```

---

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

---

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

---

## Threading

- **Main thread:** All Far Manager API calls must be from the main thread
- **Worker threads:** Protocol operations run in background threads
- **Synchronization:** Use critical sections (`CRITICAL_SECTION`) or mutexes for shared data
- **Inter-thread communication:** Use custom message queues or events

**Rule:** Never call Far Manager APIs from worker threads.

---

## Debugging

- `FTerminal->LogEvent()` for debug output (enabled in debug builds)
- `DebugAssert()` for invariants (fires in debug builds)
- Logging via `tinylog` (see `src/nbcore/logging.cpp`)
- **VS debugging:** Generate solution with `-G "Visual Studio 17 2022"`, set Far.exe as debug command

**Log output:** `%LOCALAPPDATA%\NetBox\netbox.log` (production) or `tinylog` output.

---

## Constraints

### MUST NOT

- **NEVER modify `libs/`** — use patches instead
- **NEVER combine shell commands** with `&&`, `||`, `;`
- **NEVER use Unix-style redirections** (`< /dev/null`, `>/dev/null`, `2>/dev/null`) on Windows — use `NUL` or omit
- **NEVER rewrite entire files** — make minimal edits
- **NEVER commit secrets** or credentials
- **NEVER modify third-party code** in `libs/`

### MUST

- **Build must pass with ZERO warnings** (MSVC W4)
- **All files must use CRLF line endings**
- **UTF-8 without BOM** for all text files
- **No trailing whitespace** in source, CMake, or documentation files
- **Follow naming conventions** — `T` prefix for classes, `F` prefix for member variables
- **Use RAII** for resource management — prefer smart pointers over raw `new`/`delete`
- **All Far Manager API calls from main thread only**

---

## Anti-patterns

> See [AI Behavioral Principles](#ai-behavioral-principles) for the behavioral philosophy behind these rules.

| Anti-pattern | Instead |
|--------------|---------|
| Rewriting entire files | Make minimal, surgical edits |
| `cmd1 && cmd2` chaining | Separate tool calls or `.bat` wrapper |
| `new T` / `delete ptr` | `std::make_unique<T>()` / `std::unique_ptr<T>` |
| Return codes for errors | Throw exceptions |
| Raw C strings | `UnicodeString` / `AnsiString` |
| Inline command chains | `.bat` script files |
| Guessing at unclear requirements | Ask the user |

---

## Quality Gates

Before declaring a task complete:

- [ ] Clean build with zero warnings
- [ ] No modifications to `libs/`
- [ ] Plugin DLL in `Far3_<platform>/Plugins/NetBox/`
- [ ] CRLF line endings on all modified files
- [ ] UTF-8 without BOM in all text files
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase)
- [ ] No spelling errors in comments


---

## Code Quality Checklist

- [ ] No compiler warnings (MSVC W4)
- [ ] No memory leaks (use RAII)
- [ ] Exception safety
- [ ] Unicode correctness
- [ ] No spelling/grammar errors in comments
- [ ] Common typo check: `loose`→`lose`, `connexion`→`connection`, `authentification`→`authentication`, `occured`→`occurred`, `recieve`→`receive`, `seperate`→`separate`

---

## AI Behavioral Principles

> Behavioral guidelines to reduce common LLM coding mistakes. Merge with project-specific instructions as needed.
>
> **Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" -> "Write tests for invalid inputs, then make them pass"
- "Fix the bug" -> "Write a test that reproduces it, then make it pass"
- "Refactor X" -> "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] -> verify: [check]
2. [Step] -> verify: [check]
3. [Step] -> verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.
