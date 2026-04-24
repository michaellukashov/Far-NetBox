# Implementation Plan: Kitty Keyboard Protocol Support

Branch: lmv/dev
Created: 2026-04-24

## Settings
- Testing: yes
- Logging: verbose
- Docs: yes

## Overview

Implement Kitty Keyboard Protocol (KKP) support in NetBox to enable full keyboard functionality in interactive SSH/SCP sessions, particularly for terminal applications like Midnight Commander.

**Problem**: When users connect via SCP/SSH and run interactive terminal programs (like Midnight Commander), advanced keyboard input fails. Function keys (F1-F12) with modifiers don't work correctly, and complex key combinations are not transmitted.

**Solution**: Add KKP escape sequence generation integrated with existing PuTTY-based terminal emulation, with graceful fallback to standard VT sequences.

## Reference

Kitty Keyboard Protocol specification: https://sw.kovidgoyal.net/kitty/keyboard-protocol/

## Constraints

- **MUST** use C++17 standard (no extensions)
- **MUST** build with zero warnings (MSVC /W4)
- **MUST** use CRLF line endings
- **MUST** follow naming conventions (TPascalCase for types, FPrefix for fields)
- **MUST** maintain thread safety for parallel file/shell operations
- **NEVER** modify files in `libs/` directory
- **NEVER** break existing SSH/SCP functionality
- **ONLY** modify files in: `src/core/`, `src/windows/`, `src/base/`

## Commit Plan

- **Commit 1** (after tasks 1-3): "feat(ssh): add KKP escape sequence generation"
- **Commit 2** (after tasks 4-5): "feat(ssh): integrate KKP with session management"
- **Commit 3** (after tasks 6-7): "test(ssh): add KKP tests and validation"

## Tasks

### Phase 1: Discovery & Analysis

- [ ] **Task 1: Read project conventions and locate terminal emulation code**
  
  Read and summarize key constraints from:
  - AGENTS.md sections: "Naming Conventions", "Build Commands", "Quality Gates"
  - AGENTS-Standards.md sections: "Code Style", "Error Handling"
  
  Search for and identify these files:
  - Terminal emulation layer (search: `src/core/**/*.cpp` for "terminal" OR "putty")
  - Keyboard input handling (search: `src/core/**/*.cpp` for "keyboard" OR "keypress")
  - SSH session management (search: `src/core/**/*.cpp` for "SSH" AND "session")
  
  **DONE when**: You have listed 3-5 exact file paths and confirmed each exists by reading its first 50 lines.
  
  **LOGGING REQUIREMENTS**:
  - Log discovered file paths with line counts
  - Log key structures/classes found in each file
  - Use format: [KKP.Discovery] message {data}
  - Use DEBUG level for file scanning, INFO for summary
  
  Files: (to be discovered)

- [ ] **Task 2: Analyze existing keyboard-to-escape-sequence conversion**
  
  Read the identified keyboard handling files and document:
  - Current VT/ANSI sequence generation logic
  - Where keyboard events enter the system
  - How modifiers (Ctrl, Alt, Shift) are currently encoded
  - Integration points with PuTTY terminal emulation
  
  Create a 10-line comment block describing:
  1. When to send KKP capability query
  2. Expected response format
  3. Timeout handling strategy
  
  **DONE when**: Comment block is written in the target file and you can read it back.
  
  **LOGGING REQUIREMENTS**:
  - Log current escape sequence patterns found
  - Log modifier encoding schemes discovered
  - Log integration points identified
  - Use format: [KKP.Analysis] message {data}
  - Use DEBUG level for detailed findings, INFO for summary
  
  Files: (files from Task 1)

### Phase 2: Core Implementation

- [ ] **Task 3: Implement KKP escape sequence generator**
  
  Create function `GenerateKKPSequence(uint32_t keycode, uint32_t modifiers, uint32_t eventType)` that:
  - Generates extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
  - Encodes modifiers: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
  - Handles event types: press=1, repeat=2, release=3
  - Supports function keys F1-F12 with all modifier combinations
  
  **Implementation constraint**: Add a single function that wraps existing sequence generation. Do NOT refactor PuTTY integration.
  
  **LOGGING REQUIREMENTS**:
  - Log function entry with keycode, modifiers, eventType parameters
  - Log generated escape sequence (hex dump for debugging)
  - Log any encoding errors or unsupported key combinations
  - Use format: [KKP.Generator] message {keycode=X, mods=Y, type=Z}
  - Use DEBUG level for all sequence generation
  
  Files: (keyboard handling file from Task 1, likely `src/core/Terminal.cpp` or similar)

- [ ] **Task 4: Add KKP mode flag to SSH session state**
  
  Add boolean field `FKittyKeyboardEnabled` to SSH session state structure (likely `TSessionData` or `TTerminal`):
  - Initialize to `false` in constructor
  - Add getter/setter methods following project conventions
  - Document field purpose in header comment
  
  **LOGGING REQUIREMENTS**:
  - Log flag initialization in constructor
  - Log flag state changes (false→true after negotiation, false on timeout)
  - Use format: [KKP.SessionState] message {enabled=true/false}
  - Use INFO level for state changes
  
  Files: (SSH session state file from Task 1, likely `src/core/SessionData.h` and `.cpp`)
  
  Depends on: Task 1

- [ ] **Task 5: Implement KKP capability negotiation**
  
  Add capability negotiation logic:
  - Send query `CSI ? u` on connection establishment
  - Parse response `CSI ? <flags> u` within 2 seconds
  - On timeout: set `FKittyKeyboardEnabled = false`, log warning, continue with standard VT
  - On success: parse flags, set `FKittyKeyboardEnabled = true`
  - Handle mid-session protocol switch: send capability query on first keyboard input if flag is false
  - Cache result to avoid repeated queries
  
  **LOGGING REQUIREMENTS**:
  - Log capability query sent with timestamp
  - Log response received (or timeout) with elapsed time
  - Log parsed flags on success
  - Log fallback to standard VT on timeout
  - Use format: [KKP.Negotiation] message {elapsed_ms=X, flags=Y}
  - Use INFO level for negotiation start/end, WARN for timeout, DEBUG for response parsing
  
  Files: (SSH connection/session management file from Task 1)
  
  Depends on: Task 3, Task 4

### Phase 3: Integration & Testing

- [ ] **Task 6: Integrate KKP generator with keyboard input pipeline**
  
  Modify keyboard input handling to:
  - Check `FKittyKeyboardEnabled` flag before generating escape sequences
  - Call `GenerateKKPSequence()` when flag is true
  - Fall back to existing VT sequence generation when flag is false
  - Ensure no interference with file transfer operations (SCP copy/paste)
  
  **Verification**: Start a 10MB file copy, launch mc, press F1 20 times rapidly, confirm copy completes without errors.
  
  **LOGGING REQUIREMENTS**:
  - Log keyboard event routing decision (KKP vs VT)
  - Log any sequence generation failures
  - Log file transfer interference checks (if detected)
  - Use format: [KKP.Integration] message {mode=kkp/vt, key=X}
  - Use DEBUG level for routing decisions, WARN for interference
  
  Files: (keyboard input handler from Task 1)
  
  Depends on: Task 3, Task 5

- [ ] **Task 7: Create Midnight Commander test matrix**
  
  Create test documentation with 12 key combinations:
  
  | # | Key Combination | Expected Behavior | Pass/Fail |
  |---|-----------------|-------------------|-----------|
  | 1 | F1 | Help menu appears | |
  | 2 | F10 | Exits mc | |
  | 3 | Ctrl+O | Toggles shell | |
  | 4 | Ctrl+F1 | Context help | |
  | 5 | Alt+F1 | Left panel menu | |
  | 6 | Shift+F1 | (mc-specific) | |
  | 7 | Ctrl+Shift+F1 | (if supported) | |
  | 8 | Alt+F7 | Find file | |
  | 9 | Ctrl+Alt+F5 | (if supported) | |
  | 10 | F5 | Copy | |
  | 11 | Shift+F5 | Copy with rename | |
  | 12 | Ctrl+F5 | Copy to same panel | |
  
  **Pass threshold**: ≥10/12 combinations work correctly.
  
  Additional verification:
  - Validate pseudographics rendering remains intact
  - Confirm no visual artifacts or corruption
  
  **LOGGING REQUIREMENTS**:
  - Log each test case execution with result
  - Log overall pass rate
  - Log any rendering issues detected
  - Use format: [KKP.Test] message {test=N, key=X, result=pass/fail}
  - Use INFO level for test results
  
  Files: Create `docs/testing/kkp-test-matrix.md` or add to existing test documentation
  
  Depends on: Task 6

### Phase 4: Build Verification

- [ ] **Task 8: Build and verify zero warnings**
  
  Run full build and verify:
  - Build completes: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
  - Zero warnings (MSVC /W4)
  - No modifications to `libs/` directory
  - CRLF line endings on all modified files
  - No trailing whitespace
  
  **LOGGING REQUIREMENTS**:
  - Log build command execution
  - Log warning count (must be 0)
  - Log any CRLF/whitespace violations
  - Use format: [KKP.Build] message {warnings=N, errors=N}
  - Use INFO level for build summary, ERROR for failures
  
  Files: (all modified files)
  
  Depends on: Task 7

## Edge Cases

- **Terminal doesn't support KKP**: Graceful fallback to standard VT sequences (handled by timeout in Task 5)
- **Mid-session protocol switch**: Send capability query on first keyboard input (handled in Task 5)
- **Rapid key repeat events**: Buffer management (verify in Task 6 non-interference test)
- **Unicode characters in key events**: Handle non-ASCII input in GenerateKKPSequence (Task 3)
- **Session disconnect during KKP negotiation**: Timeout mechanism prevents hang (2 second limit in Task 5)

## Acceptance Criteria

Done when:

1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander test matrix shows ≥10/12 pass rate (Task 7)
3. File transfer operations (SCP copy/paste) continue working during interactive shell use (verified in Task 6)
4. No regression in existing SSH/terminal functionality
5. Code follows project conventions (verified in Task 1)
6. All logging requirements met (verbose DEBUG logs for development)

## Implementation Notes

**Architecture Decision**: Extend existing terminal emulation code rather than creating a new module, unless existing code requires >100 lines of changes.

**Backward Compatibility**: KKP is opt-in via capability negotiation. Terminals that don't support KKP will timeout and fall back to standard VT sequences automatically.

**Thread Safety**: KKP flag is session-scoped. Each SSH session has independent state, preventing interference between parallel operations.

**Testing Strategy**: Manual testing with Midnight Commander is primary validation. Automated unit tests for escape sequence generation are secondary (optional enhancement).
