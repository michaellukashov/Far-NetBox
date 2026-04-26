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

# Memory Bank & RAG Maintenance

## Responsibilities

Your goal is to ensure the project's knowledge base remains structured, up-to-date, and free of clutter, while **never permanently deleting valuable context**. Information must flow from temporary working directories into permanent storage.

### The Information Lifecycle Rules:
1. **Core Context (`./ai-factory/memory-bank/*.md`)**: Frequently accessed files (`CONTEXT.md`, `TECHCONTEXT.md`, `progress.md`). Update these with high-level summaries when major milestones are reached.
2. **Component Specs (`./ai-factory/memory-bank/product-details/` & `specifications/`)**: Store detailed, up-to-date architectural decisions and system component documentation here.
3. **Future Work (`./ai-factory/memory-bank/to-research/`)**: Move unresolved issues, business logic ideas, and discovered technical debt here for later discussion.
4. **Archiving (`./ai-factory/storage/`)**: When a task is completed, summarize the temporary data from `./ai-factory/memory-bank/actions/` and move it into `./ai-factory/storage/archive/` or `./ai-factory/storage/incidents/`. This keeps the active context light but preserves the history for future Retrieval-Augmented Generation.
5. **Temporary Data (`./ai-factory/memory-bank/actions/`)**: Clear this directory **ONLY AFTER** its contents have been successfully summarized and archived into `./ai-factory/storage/` or integrated into core `memory-bank` files.

## When to Use

- **Task Completion** — "The feature is done. Archive the current session and update the progress."
- **Context Overload** — "Our active context is too big; refactor the documentation into the storage archive."
- **Backlog Grooming** — "Save these ideas about ROI calculation to the research folder."
- **Incident Resolution** — "The production bug is fixed. Write a post-mortem to `./ai-factory/storage/incidents/`."

## Usage Effectiveness & Best Practices

```markdown
# ❌ Bad Practice (Destructive & Messy)
Deleting plan files when the task is done without saving the history.
Dumping thousands of lines of raw logs into `CONTEXT.md`.

# ✅ Good Practice (Preservative & Structured)
1. Read the plan file and `temp-reports/`.
2. Write a summary to `./ai-factory/storage/archive/{YY-MM-DD}-{feature-name}.md`.
3. Extract architectural changes and update `./ai-factory/memory-bank/product-details/dataFlowArchitecture.md`.
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

## Testing

- **Manual testing:** Required for affected functionality
- **Build verification:** Clean build with no warnings
- **No automated tests:** Currently no test framework in place
