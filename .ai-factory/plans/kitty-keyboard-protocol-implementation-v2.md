# Kitty Keyboard Protocol Implementation Plan v2

**Created**: 2026-04-24  
**Version**: 2.0 (PromptSentinel Reviewed)  
**Status**: Ready for Implementation  
**Target**: NetBox Far Manager Plugin

## Overview

Implement Kitty Keyboard Protocol (KKP) support in NetBox to enable full keyboard functionality in interactive SSH/SCP sessions, particularly for terminal applications like Midnight Commander.

## Context

NetBox is a Far Manager plugin (SFTP/FTP/SCP/WebDAV/S3 client) built in C++17 on Windows. The codebase integrates WinSCP, PuTTY, and FileZilla components. Far Manager is a text-mode file manager; NetBox integrates as a plugin DLL.

### Current Architecture
- **Language**: C++17, MSVC (Visual Studio 2022)
- **Build**: CMake 3.15+ / Ninja
- **Platforms**: x86, x64, ARM64
- **Key directories**:
  - `src/core/` — SSH/SCP/FTP protocol implementations
  - `src/windows/` — Far Manager GUI integration
  - `src/base/` — Base classes (UnicodeString, etc.)
  - `libs/` — Third-party code (READ-ONLY)

### Problem Statement

When users connect via SCP/SSH and run interactive terminal programs (like Midnight Commander), advanced keyboard input fails. Specifically:
- Function keys (F1-F12) with modifiers don't work correctly
- Complex key combinations are not transmitted
- Terminal emulation lacks support for modern keyboard protocols

### Reference

Kitty Keyboard Protocol specification: https://sw.kovidgoyal.net/kitty/keyboard-protocol/

## Objective

Implement Kitty Keyboard Protocol (KKP) support in NetBox to enable full keyboard functionality in interactive SSH/SCP sessions.

The implementation must:
1. Add KKP escape sequence generation for extended keyboard events
2. Integrate with existing PuTTY-based terminal emulation
3. Support ANSI/VT control sequences and pseudographics
4. Handle parallel operations: file transfers (SCP), interactive shell commands, and KKP events
5. Maintain backward compatibility with existing SSH/terminal code

## Scope

### Files to Modify

**Step 1: Discovery Phase**

Before starting implementation, read and summarize key constraints from:
- AGENTS.md sections: "Naming Conventions", "Build Commands", "Quality Gates"
- AGENTS-Standards.md sections: "Code Style", "Error Handling"

Store summary in working memory.

**Step 2: File Discovery**

Search for and identify these files:
- Terminal emulation layer (search: `src/core/**/*.cpp` for "terminal" OR "putty")
- Keyboard input handling (search: `src/core/**/*.cpp` for "keyboard" OR "keypress")
- SSH session management (search: `src/core/**/*.cpp` for "SSH" AND "session")

**DONE when:** You have listed 3-5 exact file paths and confirmed each exists by reading its first 50 lines.

### Files to Create

Create `src/core/KittyKeyboard.cpp` and `src/core/KittyKeyboard.h` ONLY if:
- Existing terminal emulation code requires >100 lines of changes, OR
- No single existing file handles keyboard-to-escape-sequence conversion

Otherwise, extend existing files.

### Files You MAY Modify

ONLY these directories:
- `src/core/`
- `src/windows/`
- `src/base/`

All other directories are read-only. Attempting to modify them will fail.

## Implementation Steps

**Execution Mode:** Strict sequential. Do NOT proceed to step N+1 until step N checkpoint is confirmed.

### 1. Locate existing terminal emulation code

- Find where PuTTY terminal emulation is integrated
- Identify keyboard input → escape sequence conversion
- Map current VT/ANSI sequence generation

**Verification:** Read the identified files and confirm they contain keyboard handling code.

**CHECKPOINT:** Output JSON with discovered file paths and their roles.

### 2. Design KKP integration layer

- Write a 10-line comment block in the target file describing:
  1. When to send capability query
  2. Expected response format
  3. Timeout handling
  
- Create escape sequence generator function signature:
  ```cpp
  UnicodeString GenerateKKPSequence(uint32_t keycode, uint32_t modifiers, uint32_t eventType);
  ```
  
- Plan modifier key encoding (Shift=1, Alt=2, Ctrl=4, Super=8, bitwise OR)

**Verification:** Read back the comment block and function signature.

**CHECKPOINT:** Output JSON with comment location and function signature.

### 3. Implement KKP escape sequences

- Extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
- Modifier encoding: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
- Event types: press=1, repeat=2, release=3
- Handle function keys F1-F12 with all modifier combinations

**Implementation constraint:** Add a single function `GenerateKKPSequence(keycode, modifiers, eventType)` that wraps existing sequence generation. Do NOT refactor PuTTY integration.

**Verification:** Compile the code and verify zero warnings.

**CHECKPOINT:** Output JSON with function location and compilation result.

### 4. Integrate with session management

- **Add KKP mode flag:**
  * Field name: `FKittyKeyboardEnabled` (boolean)
  * Location: SSH session state structure (likely `TSessionData` or `TTerminal`)
  * Initialize to `false` in constructor
  * Set to `true` after successful capability negotiation
  
- **Implement capability negotiation:**
  * Send query: `CSI ? u` on connection establishment
  * Expected response: `CSI ? <flags> u` within 2 seconds
  * On timeout: set `FKittyKeyboardEnabled = false`, log warning, continue with standard VT
  * On success: parse flags, set `FKittyKeyboardEnabled = true`
  
- **Handle mid-session protocol switch:**
  * On first keyboard input, if `FKittyKeyboardEnabled == false`, send capability query
  * Cache result to avoid repeated queries
  
- **Verify non-interference:**
  * Start a 10MB file copy operation
  * Launch mc and press F1 20 times rapidly
  * Confirm copy completes without errors or slowdown

**CHECKPOINT:** Output JSON with flag location, negotiation code location, and verification result.

### 5. Test with Midnight Commander

Create a test matrix of 12 key combinations:

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

**Pass threshold:** ≥10/12 combinations work correctly.

**Additional verification:**
- Validate pseudographics rendering remains intact
- Confirm no visual artifacts or corruption

**CHECKPOINT:** Output JSON with test matrix results (pass/fail for each) and overall pass rate.

## Constraints

- **MUST** use C++17 standard (no extensions)
- **MUST** build with zero warnings (MSVC /W4)
- **MUST** use CRLF line endings
- **MUST** follow existing naming conventions (TPascalCase for types, FPrefix for fields)
- **MUST** maintain thread safety for parallel file/shell operations
- **NEVER** break existing SSH/SCP functionality

## Edge Cases

- **Terminal doesn't support KKP** → graceful fallback to standard VT sequences (handled by timeout in step 4)
- **Mid-session protocol switch** → send capability query on first keyboard input (handled in step 4)
- **Rapid key repeat events** → buffer management (verify in step 4 non-interference test)
- **Unicode characters in key events** → handle non-ASCII input in GenerateKKPSequence
- **Session disconnect during KKP negotiation** → timeout mechanism prevents hang (2 second limit)

## Acceptance Criteria

Done when:

1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander test matrix shows ≥10/12 pass rate
3. File transfer operations (SCP copy/paste) continue working during interactive shell use (verified in step 4)
4. No regression in existing SSH/terminal functionality
5. Code follows project conventions (verified by reading AGENTS.md, AGENTS-Standards.md in discovery phase)

## Stop Conditions

Stop and ask before:

- Modifying any file outside `src/core/`, `src/windows/`, `src/base/`
- Changing CMake build configuration
- Adding external dependencies
- Modifying Far Manager plugin interface (`src/NetBox/`)
- Making changes that affect FTP/WebDAV/S3 protocols

## Checkpoints

**Execution Mode:** Strict sequential. Do NOT proceed to step N+1 until step N checkpoint is confirmed.

After each major step, output this JSON:

```json
{
  "step": <number>,
  "completed": true,
  "files_modified": ["path1", "path2"],
  "files_created": ["path3"],
  "verification": "description of what was verified",
  "next_action": "what step N+1 will do"
}
```

At the end, provide:
- List of all files modified (with line count changes)
- Summary of KKP implementation approach (architecture diagram if >3 files changed)
- Test instructions for Midnight Commander (12-item test matrix with pass/fail for each)

## Claude Code Prompt

```markdown
<context>
You are working on NetBox — a Far Manager plugin (SFTP/FTP/SCP/WebDAV/S3 client) built in C++17 on Windows. The codebase integrates WinSCP, PuTTY, and FileZilla components. Far Manager is a text-mode file manager; NetBox integrates as a plugin DLL.

## Current Architecture
- **Language**: C++17, MSVC (Visual Studio 2022)
- **Build**: CMake 3.15+ / Ninja
- **Platforms**: x86, x64, ARM64
- **Key directories**:
  - `src/core/` — SSH/SCP/FTP protocol implementations
  - `src/windows/` — Far Manager GUI integration
  - `src/base/` — Base classes (UnicodeString, etc.)
  - `libs/` — Third-party code (READ-ONLY)

## Problem Statement
When users connect via SCP/SSH and run interactive terminal programs (like Midnight Commander), advanced keyboard input fails. Specifically:
- Function keys (F1-F12) with modifiers don't work correctly
- Complex key combinations are not transmitted
- Terminal emulation lacks support for modern keyboard protocols

## Reference
Kitty Keyboard Protocol specification: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
</context>

<objective>
Implement Kitty Keyboard Protocol (KKP) support in NetBox to enable full keyboard functionality in interactive SSH/SCP sessions, particularly for terminal applications like Midnight Commander.

The implementation must:
1. Add KKP escape sequence generation for extended keyboard events
2. Integrate with existing PuTTY-based terminal emulation
3. Support ANSI/VT control sequences and pseudographics
4. Handle parallel operations: file transfers (SCP), interactive shell commands, and KKP events
5. Maintain backward compatibility with existing SSH/terminal code
</objective>

<scope>
## Discovery Phase

Before starting implementation, read and summarize key constraints from:
- AGENTS.md sections: "Naming Conventions", "Build Commands", "Quality Gates"
- AGENTS-Standards.md sections: "Code Style", "Error Handling"

Store summary in working memory.

## File Discovery

Search for and identify these files:
- Terminal emulation layer (search: `src/core/**/*.cpp` for "terminal" OR "putty")
- Keyboard input handling (search: `src/core/**/*.cpp` for "keyboard" OR "keypress")
- SSH session management (search: `src/core/**/*.cpp` for "SSH" AND "session")

**DONE when:** You have listed 3-5 exact file paths and confirmed each exists by reading its first 50 lines.

## Files to Create

Create `src/core/KittyKeyboard.cpp` and `src/core/KittyKeyboard.h` ONLY if:
- Existing terminal emulation code requires >100 lines of changes, OR
- No single existing file handles keyboard-to-escape-sequence conversion

Otherwise, extend existing files.

## Files You MAY Modify

ONLY these directories:
- `src/core/`
- `src/windows/`
- `src/base/`

All other directories are read-only. Attempting to modify them will fail.
</scope>

<implementation_steps>
**Execution Mode:** Strict sequential. Do NOT proceed to step N+1 until step N checkpoint is confirmed.

1. **Locate existing terminal emulation code**
   - Find where PuTTY terminal emulation is integrated
   - Identify keyboard input → escape sequence conversion
   - Map current VT/ANSI sequence generation
   
   **Verification:** Read the identified files and confirm they contain keyboard handling code.
   
   **CHECKPOINT:** Output JSON with discovered file paths and their roles.

2. **Design KKP integration layer**
   - Write a 10-line comment block in the target file describing:
     1. When to send capability query
     2. Expected response format
     3. Timeout handling
   
   - Create escape sequence generator function signature:
     ```cpp
     UnicodeString GenerateKKPSequence(uint32_t keycode, uint32_t modifiers, uint32_t eventType);
     ```
   
   - Plan modifier key encoding (Shift=1, Alt=2, Ctrl=4, Super=8, bitwise OR)
   
   **Verification:** Read back the comment block and function signature.
   
   **CHECKPOINT:** Output JSON with comment location and function signature.

3. **Implement KKP escape sequences**
   - Extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
   - Modifier encoding: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
   - Event types: press=1, repeat=2, release=3
   - Handle function keys F1-F12 with all modifier combinations
   
   **Implementation constraint:** Add a single function `GenerateKKPSequence(keycode, modifiers, eventType)` that wraps existing sequence generation. Do NOT refactor PuTTY integration.
   
   **Verification:** Compile the code and verify zero warnings.
   
   **CHECKPOINT:** Output JSON with function location and compilation result.

4. **Integrate with session management**
   - **Add KKP mode flag:**
     * Field name: `FKittyKeyboardEnabled` (boolean)
     * Location: SSH session state structure (likely `TSessionData` or `TTerminal`)
     * Initialize to `false` in constructor
     * Set to `true` after successful capability negotiation
   
   - **Implement capability negotiation:**
     * Send query: `CSI ? u` on connection establishment
     * Expected response: `CSI ? <flags> u` within 2 seconds
     * On timeout: set `FKittyKeyboardEnabled = false`, log warning, continue with standard VT
     * On success: parse flags, set `FKittyKeyboardEnabled = true`
   
   - **Handle mid-session protocol switch:**
     * On first keyboard input, if `FKittyKeyboardEnabled == false`, send capability query
     * Cache result to avoid repeated queries
   
   - **Verify non-interference:**
     * Start a 10MB file copy operation
     * Launch mc and press F1 20 times rapidly
     * Confirm copy completes without errors or slowdown
   
   **CHECKPOINT:** Output JSON with flag location, negotiation code location, and verification result.

5. **Test with Midnight Commander**
   Create a test matrix of 12 key combinations:
   
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
   
   **Pass threshold:** ≥10/12 combinations work correctly.
   
   **Additional verification:**
   - Validate pseudographics rendering remains intact
   - Confirm no visual artifacts or corruption
   
   **CHECKPOINT:** Output JSON with test matrix results (pass/fail for each) and overall pass rate.
</implementation_steps>

<constraints>
- **MUST** use C++17 standard (no extensions)
- **MUST** build with zero warnings (MSVC /W4)
- **MUST** use CRLF line endings
- **MUST** follow existing naming conventions (TPascalCase for types, FPrefix for fields)
- **MUST** maintain thread safety for parallel file/shell operations
- **NEVER** break existing SSH/SCP functionality
</constraints>

<edge_cases>
- **Terminal doesn't support KKP** → graceful fallback to standard VT sequences (handled by timeout in step 4)
- **Mid-session protocol switch** → send capability query on first keyboard input (handled in step 4)
- **Rapid key repeat events** → buffer management (verify in step 4 non-interference test)
- **Unicode characters in key events** → handle non-ASCII input in GenerateKKPSequence
- **Session disconnect during KKP negotiation** → timeout mechanism prevents hang (2 second limit)
</edge_cases>

<acceptance_criteria>
Done when:
1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander test matrix shows ≥10/12 pass rate
3. File transfer operations (SCP copy/paste) continue working during interactive shell use (verified in step 4)
4. No regression in existing SSH/terminal functionality
5. Code follows project conventions (verified by reading AGENTS.md, AGENTS-Standards.md in discovery phase)
</acceptance_criteria>

<stop_conditions>
Stop and ask before:
- Modifying any file outside `src/core/`, `src/windows/`, `src/base/`
- Changing CMake build configuration
- Adding external dependencies
- Modifying Far Manager plugin interface (`src/NetBox/`)
- Making changes that affect FTP/WebDAV/S3 protocols
</stop_conditions>

<checkpoints>
**Execution Mode:** Strict sequential. Do NOT proceed to step N+1 until step N checkpoint is confirmed.

After each major step, output this JSON:

```json
{
  "step": <number>,
  "completed": true,
  "files_modified": ["path1", "path2"],
  "files_created": ["path3"],
  "verification": "description of what was verified",
  "next_action": "what step N+1 will do"
}
```

At the end, provide:
- List of all files modified (with line count changes)
- Summary of KKP implementation approach (architecture diagram if >3 files changed)
- Test instructions for Midnight Commander (12-item test matrix with pass/fail for each)
</checkpoints>
```

## PromptSentinel Review Summary

**Review Date:** 2026-04-24  
**Overall Risk Level:** Medium → Low (after fixes)  
**Issues Fixed:** 13 (4 High, 6 Medium, 3 Low)

### Key Improvements Applied

1. **Explicit file discovery verification** with done-state criteria
2. **Defined KKP mode flag structure** (field name, type, location, initialization)
3. **Replaced "if needed" with decision criteria** for file creation
4. **Converted "design" steps to concrete outputs** (comment blocks, function signatures)
5. **Added AGENTS.md reading instruction** to discovery phase
6. **Strict sequential execution requirement** with checkpoint gates
7. **Reframed negations as positive constraints** (ONLY modify vs DO NOT modify)
8. **Added structured JSON checkpoint format** for progress tracking
9. **Quantified test success criteria** (≥10/12 pass rate)
10. **Added explicit error handling** for capability negotiation timeout
11. **Bounded scope creep** with "single function" constraint
12. **Added mid-session protocol switch handling** to implementation steps
13. **Added non-interference verification** with concrete test procedure

**Estimated Production Failure Rate:** ~15-20% → ~3-5% (after fixes)
