# Implementation Plan: Kitty Keyboard Protocol Support (Improved)

Branch: lmv/dev
Created: 2026-04-24
Improved: 2026-04-24

## Settings
- Testing: yes
- Logging: verbose
- Docs: yes

## Overview

Complete the Kitty Keyboard Protocol (KKP) implementation in NetBox to enable full keyboard functionality in interactive SSH/SCP sessions, particularly for terminal applications like Midnight Commander.

**Problem**: When users connect via SCP/SSH and run interactive terminal programs (like Midnight Commander), advanced keyboard input fails. Function keys (F1-F12) with modifiers don't work correctly, and complex key combinations are not transmitted.

**Current State**: KKP infrastructure is **partially implemented**:
- ✅ `TKittyKeyboard` class exists (state management, flags, push/pop)
- ✅ Protocol negotiation parsing exists (CSI sequence parsing in `SecureShell.cpp`)
- ✅ Session configuration flag exists (`FKittyKeyboardProtocol`)
- ✅ Win32Input mode exists (separate feature)
- ❌ **MISSING**: Keyboard event → KKP escape sequence generation
- ❌ **MISSING**: Integration with keyboard input pipeline
- ❌ **MISSING**: Actual transmission of KKP sequences to remote terminal

**Solution**: Implement the missing keyboard event encoding and pipeline integration to complete the KKP feature.

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
- **MUST** preserve existing `TKittyKeyboard` class and protocol negotiation code

## Commit Plan

- **Commit 1** (after tasks 1-2): "feat(ssh): add KKP escape sequence encoder"
- **Commit 2** (after tasks 3-4): "feat(ssh): integrate KKP with keyboard input pipeline"
- **Commit 3** (after tasks 5-6): "test(ssh): add KKP validation and test matrix"

## Tasks

### Phase 1: Discovery & Design

- [ ] **Task 1: Locate keyboard input pipeline and analyze existing code**
  
  **Discovery objectives**:
  - Find where keyboard events enter the SSH/SCP session (likely in `SecureShell.cpp` or related files)
  - Identify current VT/ANSI escape sequence generation logic
  - Understand how `FKittyKeyboard` is currently used (only for state management, not encoding)
  - Map the flow: Far Manager key event → NetBox → SSH channel → remote terminal
  - Identify where Win32Input encoding happens (as a reference pattern)
  
  **Files to examine**:
  - `src/core/SecureShell.cpp` (lines 475-481, 1527-1601 for existing KKP code)
  - `src/core/SecureShell.h` (line 97 for `FKittyKeyboard` field)
  - `src/core/Win32Input.cpp` (reference for encoding pattern)
  - `src/core/Terminal.cpp` (keyboard event handling)
  - Search for: keyboard event handlers, `SendData`, `Send`, input methods
  
  **DONE when**: You have documented:
  1. Exact entry point for keyboard events (function name, file, line range)
  2. Current escape sequence generation logic (VT/ANSI)
  3. How `FKittyKeyboard.IsActive()` should gate KKP encoding
  4. Where encoded sequences are sent to the SSH channel
  
  **LOGGING REQUIREMENTS**:
  - Log discovered entry points with function signatures
  - Log current escape sequence patterns found
  - Log integration points for KKP encoder
  - Use format: [KKP.Discovery] message {data}
  - Use DEBUG level for file scanning, INFO for summary
  
  Files: `src/core/SecureShell.cpp`, `src/core/SecureShell.h`, `src/core/Win32Input.cpp`, `src/core/Terminal.cpp`

- [ ] **Task 2: Design KKP encoder API and integration strategy**
  
  Design the KKP encoder as a static utility class (similar to `TWin32Input`):
  
  **API Design**:
  ```cpp
  class TKittyKeyboardEncoder
  {
  public:
    // Encode keyboard event to KKP escape sequence
    // Format: CSI unicode-key-code:shifted-key ; modifiers:event-type u
    static UnicodeString Encode(uint32_t keycode, uint32_t modifiers, uint32_t eventType);
    
    // Helper: Convert Windows VK code to Unicode key code
    static uint32_t VKToUnicode(uint32_t vkCode);
    
    // Helper: Encode modifier flags (Shift=1, Alt=2, Ctrl=4, Super=8)
    static uint32_t EncodeModifiers(uint32_t controlKeyState);
  };
  ```
  
  **Integration strategy**:
  1. Check `FKittyKeyboard.IsActive()` before encoding
  2. If active: call `TKittyKeyboardEncoder::Encode()`
  3. If inactive: fall back to existing VT/ANSI sequence generation
  4. Send encoded sequence via existing SSH channel send method
  
  **DONE when**: You have written a design comment block (15-20 lines) in `src/core/KittyKeyboard.h` describing:
  1. Encoder class structure
  2. Integration points with keyboard input pipeline
  3. Fallback strategy for non-KKP mode
  
  **LOGGING REQUIREMENTS**:
  - Log design decisions and rationale
  - Log API surface area
  - Use format: [KKP.Design] message {data}
  - Use INFO level
  
  Files: `src/core/KittyKeyboard.h` (add design comment)
  
  Depends on: Task 1

### Phase 2: Core Implementation

- [ ] **Task 3: Implement KKP escape sequence encoder**
  
  Create `TKittyKeyboardEncoder` class in `src/core/KittyKeyboard.h` and `KittyKeyboard.cpp`:
  
  **Implementation requirements**:
  - Extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
  - Modifier encoding: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
  - Event types: press=1, repeat=2, release=3
  - Support function keys F1-F12 with all modifier combinations
  - Handle special keys: Enter, Tab, Backspace, Delete, Arrow keys, Home, End, PageUp, PageDown
  - Handle printable characters (A-Z, 0-9, punctuation)
  
  **Key mapping table** (partial, for reference):
  ```cpp
  // Function keys: F1=0xFFBE, F2=0xFFBF, ..., F12=0xFFC9
  // Arrow keys: Up=0xFF52, Down=0xFF54, Left=0xFF51, Right=0xFF53
  // Special: Enter=0x0D, Tab=0x09, Backspace=0x08, Delete=0xFFFF
  ```
  
  **LOGGING REQUIREMENTS**:
  - Log function entry with keycode, modifiers, eventType parameters
  - Log generated escape sequence (hex dump for debugging)
  - Log any encoding errors or unsupported key combinations
  - Use format: [KKP.Encoder] message {keycode=0xX, mods=0xY, type=Z, seq="..."}
  - Use DEBUG level for all sequence generation
  
  Files: `src/core/KittyKeyboard.h`, `src/core/KittyKeyboard.cpp`
  
  Depends on: Task 2

- [ ] **Task 4: Integrate KKP encoder with keyboard input pipeline**
  
  Modify the keyboard input handler identified in Task 1 to:
  
  **Integration logic**:
  ```cpp
  if (FKittyKeyboard.IsActive())
  {
    // Use KKP encoding
    UnicodeString Sequence = TKittyKeyboardEncoder::Encode(keycode, modifiers, eventType);
    SendData(Sequence);
    LogEvent(FORMAT("[KKP.Input] Sent KKP sequence: %s", Sequence));
  }
  else
  {
    // Fall back to existing VT/ANSI sequence generation
    UnicodeString Sequence = GenerateVTSequence(keycode, modifiers);
    SendData(Sequence);
  }
  ```
  
  **Verification**: Ensure no interference with file transfer operations (SCP copy/paste).
  
  **LOGGING REQUIREMENTS**:
  - Log keyboard event routing decision (KKP vs VT)
  - Log key event details (VK code, modifiers, event type)
  - Log any sequence generation failures
  - Log file transfer interference checks (if detected)
  - Use format: [KKP.Integration] message {mode=kkp/vt, vk=0xX, mods=0xY}
  - Use DEBUG level for routing decisions, WARN for interference
  
  Files: (keyboard input handler from Task 1, likely `src/core/SecureShell.cpp`)
  
  Depends on: Task 3

### Phase 3: Testing & Validation

- [ ] **Task 5: Create Midnight Commander test matrix and validation procedure**
  
  Create test documentation with 12 key combinations:
  
  | # | Key Combination | Expected Behavior | VK Code | KKP Sequence (expected) | Pass/Fail |
  |---|-----------------|-------------------|---------|-------------------------|-----------|
  | 1 | F1 | Help menu appears | 0x70 | `\x1b[0xFFBE;1u` | |
  | 2 | F10 | Exits mc | 0x79 | `\x1b[0xFFC7;1u` | |
  | 3 | Ctrl+O | Toggles shell | 0x4F | `\x1b[0x4F;5u` | |
  | 4 | Ctrl+F1 | Context help | 0x70 | `\x1b[0xFFBE;5u` | |
  | 5 | Alt+F1 | Left panel menu | 0x70 | `\x1b[0xFFBE;3u` | |
  | 6 | Shift+F1 | (mc-specific) | 0x70 | `\x1b[0xFFBE;2u` | |
  | 7 | Ctrl+Shift+F1 | (if supported) | 0x70 | `\x1b[0xFFBE;6u` | |
  | 8 | Alt+F7 | Find file | 0x76 | `\x1b[0xFFC4;3u` | |
  | 9 | Ctrl+Alt+F5 | (if supported) | 0x74 | `\x1b[0xFFC2;7u` | |
  | 10 | F5 | Copy | 0x74 | `\x1b[0xFFC2;1u` | |
  | 11 | Shift+F5 | Copy with rename | 0x74 | `\x1b[0xFFC2;2u` | |
  | 12 | Ctrl+F5 | Copy to same panel | 0x74 | `\x1b[0xFFC2;5u` | |
  
  **Pass threshold**: ≥10/12 combinations work correctly.
  
  **Additional verification**:
  - Validate pseudographics rendering remains intact
  - Confirm no visual artifacts or corruption
  - Test with KKP disabled (fallback to VT sequences)
  
  **Test procedure**:
  1. Enable KKP in session settings (`FKittyKeyboardProtocol = true`)
  2. Connect to SSH server with KKP-capable terminal (e.g., kitty, WezTerm)
  3. Launch Midnight Commander (`mc`)
  4. Execute each test case and record result
  5. Repeat with KKP disabled to verify fallback
  
  **LOGGING REQUIREMENTS**:
  - Log each test case execution with result
  - Log overall pass rate
  - Log any rendering issues detected
  - Use format: [KKP.Test] message {test=N, key=X, result=pass/fail}
  - Use INFO level for test results
  
  Files: Create `docs/testing/kkp-test-matrix.md` or add to existing test documentation
  
  Depends on: Task 4

- [ ] **Task 6: Add unit tests for KKP encoder**
  
  Create unit tests for `TKittyKeyboardEncoder`:
  
  **Test cases**:
  1. Function keys F1-F12 without modifiers
  2. Function keys with Ctrl, Alt, Shift modifiers
  3. Function keys with combined modifiers (Ctrl+Shift, Ctrl+Alt, etc.)
  4. Printable characters (A-Z, 0-9)
  5. Special keys (Enter, Tab, Backspace, Delete, Arrow keys)
  6. Edge cases: unsupported keys, invalid modifiers
  
  **Test framework**: Use existing NetBox test infrastructure (if available) or create standalone test file.
  
  **LOGGING REQUIREMENTS**:
  - Log test execution start/end
  - Log each test case result (pass/fail)
  - Log any assertion failures with details
  - Use format: [KKP.UnitTest] message {test=X, result=pass/fail}
  - Use INFO level for test results
  
  Files: Create `src/core/KittyKeyboard.test.cpp` or add to existing test file
  
  Depends on: Task 3

### Phase 4: Build Verification & Documentation

- [ ] **Task 7: Build and verify zero warnings**
  
  Run full build and verify:
  - Build completes: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
  - Zero warnings (MSVC /W4)
  - No modifications to `libs/` directory
  - CRLF line endings on all modified files
  - No trailing whitespace
  - All new code follows naming conventions (TPascalCase, FPrefix)
  
  **LOGGING REQUIREMENTS**:
  - Log build command execution
  - Log warning count (must be 0)
  - Log any CRLF/whitespace violations
  - Use format: [KKP.Build] message {warnings=N, errors=N}
  - Use INFO level for build summary, ERROR for failures
  
  Files: (all modified files)
  
  Depends on: Task 6

- [ ] **Task 8: Update documentation**
  
  Update project documentation to reflect KKP support:
  
  **Documentation updates**:
  1. Add KKP feature description to `AGENTS.md` or relevant user-facing docs
  2. Document session configuration flag (`KittyKeyboardProtocol`)
  3. Add troubleshooting section for KKP issues
  4. Document compatibility (which terminals support KKP)
  5. Add developer notes about encoder implementation
  
  **Content to include**:
  - What is KKP and why it matters
  - How to enable KKP in NetBox session settings
  - Supported terminals (kitty, WezTerm, foot, etc.)
  - Fallback behavior when KKP is not supported
  - Known limitations
  
  **LOGGING REQUIREMENTS**:
  - Log documentation files updated
  - Use format: [KKP.Docs] message {file=X}
  - Use INFO level
  
  Files: `AGENTS.md`, `docs/features/kitty-keyboard-protocol.md` (new), `docs/testing/kkp-test-matrix.md`
  
  Depends on: Task 7

## Edge Cases

- **Terminal doesn't support KKP**: Graceful fallback to standard VT sequences (handled by `FKittyKeyboard.IsActive()` check)
- **Mid-session protocol switch**: Already handled by existing protocol negotiation parsing in `SecureShell.cpp` (lines 1527-1601)
- **Rapid key repeat events**: Buffer management (verify in Task 4 integration)
- **Unicode characters in key events**: Handle non-ASCII input in encoder (Task 3)
- **Session disconnect during KKP negotiation**: Already handled by existing timeout mechanism
- **KKP disabled in session settings**: Encoder is never called, VT fallback always used
- **Unsupported key combinations**: Encoder should log warning and fall back to VT sequence

## Acceptance Criteria

Done when:

1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander test matrix shows ≥10/12 pass rate (Task 5)
3. Unit tests pass for KKP encoder (Task 6)
4. File transfer operations (SCP copy/paste) continue working during interactive shell use (verified in Task 4)
5. No regression in existing SSH/terminal functionality
6. Code follows project conventions (verified in Task 7)
7. All logging requirements met (verbose DEBUG logs for development)
8. Documentation updated (Task 8)
9. Existing `TKittyKeyboard` class and protocol negotiation code remain unchanged

## Implementation Notes

### Architecture Decision

**Extend existing KKP infrastructure** rather than creating a new module:
- Reuse `TKittyKeyboard` class for state management (already implemented)
- Reuse protocol negotiation parsing (already implemented in `SecureShell.cpp`)
- Add encoder class to `KittyKeyboard.h/cpp` (follows existing pattern)
- Integrate with keyboard input pipeline (minimal changes)

### Backward Compatibility

KKP is opt-in via session configuration flag (`FKittyKeyboardProtocol`):
- Disabled by default (no behavior change for existing users)
- When enabled, protocol negotiation happens automatically
- Terminals that don't support KKP will not set `FKittyKeyboard` flags, so encoder is never called
- Fallback to standard VT sequences is automatic and transparent

### Thread Safety

KKP flag is session-scoped (`FKittyKeyboard` is a member of `TSecureShell`):
- Each SSH session has independent state
- No shared mutable state between sessions
- Prevents interference between parallel operations

### Testing Strategy

1. **Manual testing** with Midnight Commander is primary validation (Task 5)
2. **Unit tests** for encoder logic (Task 6)
3. **Integration testing** with real SSH servers and KKP-capable terminals
4. **Regression testing** to ensure no breakage of existing functionality

### Key Differences from Original Plan

**What changed**:
1. **Removed redundant tasks**: Protocol negotiation and state management already exist
2. **Focused scope**: Only implement encoder and pipeline integration
3. **Preserved existing code**: No modifications to `TKittyKeyboard` class or negotiation parsing
4. **Added unit tests**: Explicit test task for encoder logic
5. **Clarified integration**: Specific integration strategy based on existing code patterns
6. **Updated file paths**: Accurate file locations based on codebase analysis

**Why these changes improve the plan**:
- Avoids duplicate work (protocol negotiation already done)
- Reduces risk of breaking existing functionality
- Focuses effort on the missing pieces (encoder + integration)
- Provides clearer guidance based on actual codebase structure
- Adds test coverage that was missing in original plan
