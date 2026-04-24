# Kitty Keyboard Protocol Implementation Plan

**Created**: 2026-04-24  
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
  - `libs/` — Third-party code (DO NOT MODIFY)

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

### Files to Modify (estimate 3-5 files)

Identify and modify these specific files:
- Terminal emulation layer (likely in `src/core/` or integration with PuTTY in `libs/putty/`)
- Keyboard input handling for SSH sessions
- SCP/SSH session management code

### Files to Create

- New KKP protocol handler module (if needed)
- Unit tests for KKP escape sequence generation

### DO NOT Touch

- Any files in `libs/` directory
- Build configuration files unless absolutely necessary
- Unrelated protocol implementations (FTP, WebDAV, S3)

## Implementation Steps

### 1. Locate existing terminal emulation code

- Find where PuTTY terminal emulation is integrated
- Identify keyboard input → escape sequence conversion
- Map current VT/ANSI sequence generation

### 2. Design KKP integration layer

- Create escape sequence generator for KKP format
- Design capability negotiation (terminal must advertise KKP support)
- Plan modifier key encoding (Shift, Ctrl, Alt, Super combinations)

### 3. Implement KKP escape sequences

- Extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
- Modifier encoding: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
- Event types: press=1, repeat=2, release=3
- Handle function keys F1-F12 with all modifier combinations

### 4. Integrate with session management

- Add KKP mode flag to SSH session state
- Implement capability query/response during connection setup
- Ensure KKP events don't interfere with file transfer operations

### 5. Test with Midnight Commander

- Verify F1 (help), F10 (quit), Ctrl+O (shell toggle) work correctly
- Test complex combinations: Ctrl+Shift+F1, Alt+F7, etc.
- Validate pseudographics rendering remains intact

## Constraints

- **MUST** use C++17 standard (no extensions)
- **MUST** build with zero warnings (MSVC /W4)
- **MUST** use CRLF line endings
- **MUST** follow existing naming conventions (TPascalCase for types, FPrefix for fields)
- **MUST NOT** modify any code in `libs/` directory
- **MUST** maintain thread safety for parallel file/shell operations
- **NEVER** break existing SSH/SCP functionality

## Edge Cases

- Terminal doesn't support KKP → graceful fallback to standard VT sequences
- Mid-session protocol switch (user starts mc after connection established)
- Rapid key repeat events → buffer management
- Unicode characters in key events (non-ASCII input)
- Session disconnect during KKP negotiation

## Acceptance Criteria

Done when:

1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander launches over SSH and responds to:
   - F1 (help menu appears)
   - F10 (exits mc)
   - Ctrl+O (toggles shell)
   - Ctrl+Shift+F1 (if mc supports it)
3. File transfer operations (SCP copy/paste) continue working during interactive shell use
4. No regression in existing SSH/terminal functionality
5. Code follows project conventions (see AGENTS.md, AGENTS-Standards.md)

## Stop Conditions

Stop and ask before:

- Modifying any file in `libs/` directory
- Changing CMake build configuration
- Adding external dependencies
- Modifying Far Manager plugin interface (`src/NetBox/`)
- Making changes that affect FTP/WebDAV/S3 protocols

## Checkpoints

After each major step, output: ✅ [what was completed]

At the end, provide:
- List of all files modified
- Summary of KKP implementation approach
- Test instructions for Midnight Commander

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
  - `libs/` — Third-party code (DO NOT MODIFY)

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
## Files to Modify (estimate 3-5 files)
Identify and modify these specific files:
- Terminal emulation layer (likely in `src/core/` or integration with PuTTY in `libs/putty/`)
- Keyboard input handling for SSH sessions
- SCP/SSH session management code

## Files to Create
- New KKP protocol handler module (if needed)
- Unit tests for KKP escape sequence generation

## DO NOT Touch
- Any files in `libs/` directory
- Build configuration files unless absolutely necessary
- Unrelated protocol implementations (FTP, WebDAV, S3)
</scope>

<implementation_steps>
1. **Locate existing terminal emulation code**
   - Find where PuTTY terminal emulation is integrated
   - Identify keyboard input → escape sequence conversion
   - Map current VT/ANSI sequence generation

2. **Design KKP integration layer**
   - Create escape sequence generator for KKP format
   - Design capability negotiation (terminal must advertise KKP support)
   - Plan modifier key encoding (Shift, Ctrl, Alt, Super combinations)

3. **Implement KKP escape sequences**
   - Extended key event format: `CSI unicode-key-code:shifted-key ; modifiers:event-type u`
   - Modifier encoding: Shift=1, Alt=2, Ctrl=4, Super=8 (bitwise OR)
   - Event types: press=1, repeat=2, release=3
   - Handle function keys F1-F12 with all modifier combinations

4. **Integrate with session management**
   - Add KKP mode flag to SSH session state
   - Implement capability query/response during connection setup
   - Ensure KKP events don't interfere with file transfer operations

5. **Test with Midnight Commander**
   - Verify F1 (help), F10 (quit), Ctrl+O (shell toggle) work correctly
   - Test complex combinations: Ctrl+Shift+F1, Alt+F7, etc.
   - Validate pseudographics rendering remains intact
</implementation_steps>

<constraints>
- **MUST** use C++17 standard (no extensions)
- **MUST** build with zero warnings (MSVC /W4)
- **MUST** use CRLF line endings
- **MUST** follow existing naming conventions (TPascalCase for types, FPrefix for fields)
- **MUST NOT** modify any code in `libs/` directory
- **MUST** maintain thread safety for parallel file/shell operations
- **NEVER** break existing SSH/SCP functionality
</constraints>

<edge_cases>
- Terminal doesn't support KKP → graceful fallback to standard VT sequences
- Mid-session protocol switch (user starts mc after connection established)
- Rapid key repeat events → buffer management
- Unicode characters in key events (non-ASCII input)
- Session disconnect during KKP negotiation
</edge_cases>

<acceptance_criteria>
Done when:
1. Build completes with zero warnings: `cmake --build build-RelWithDebugInfo --clean-first -- -j4`
2. Midnight Commander launches over SSH and responds to:
   - F1 (help menu appears)
   - F10 (exits mc)
   - Ctrl+O (toggles shell)
   - Ctrl+Shift+F1 (if mc supports it)
3. File transfer operations (SCP copy/paste) continue working during interactive shell use
4. No regression in existing SSH/terminal functionality
5. Code follows project conventions (see AGENTS.md, AGENTS-Standards.md)
</acceptance_criteria>

<stop_conditions>
Stop and ask before:
- Modifying any file in `libs/` directory
- Changing CMake build configuration
- Adding external dependencies
- Modifying Far Manager plugin interface (`src/NetBox/`)
- Making changes that affect FTP/WebDAV/S3 protocols
</stop_conditions>

<checkpoints>
After each major step, output: ✅ [what was completed]

At the end, provide:
- List of all files modified
- Summary of KKP implementation approach
- Test instructions for Midnight Commander
</checkpoints>
```
