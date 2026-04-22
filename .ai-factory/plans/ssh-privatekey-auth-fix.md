# Implementation Plan: SSH Private Key Authentication Bug Fix

Branch: none (bug fix)
Created: 2026-04-22

## Settings
- Testing: no (manual testing required with actual SSH server)
- Logging: verbose (debug auth flow)
- Docs: no (not needed for bug fix)

## Goal
Fix NetBox SSH connection failure when using private key certificates (.ppk files converted by PuTTY). The plugin shows "Authorizing" briefly then falls back to session menu. Also fixes incorrect password prompt when key file is explicitly specified.

## Bug Description
- **Error:** "No supported authentication methods available (server sent: publickey)"
- **Symptoms:**
  1. "Authorizing" dialog blinks briefly, then falls back to session menu
  2. NetBox prompts for password even when key file is explicitly passed
- **Works in:** WinSCP, standard Windows sftp — fails only in NetBox

## Constraints
- Only modify code in `src/core/` and `src/base/`
- Do NOT modify `libs/` — if patches needed, document separately
- Must pass MSVC W4 warnings on build
- CRLF line endings on all changes

## Research Hypothesis
The SSH authentication code has one of these issues:
1. Not properly loading/parsing the PPK private key file
2. Not sending correct key format to the SSH agent
3. Falling back to password auth when publickey should succeed
4. Authentication sequence error (tries password before publickey, or gives up too early)

## Tasks

### Phase 1: Locate Authentication Code
- [ ] Task 1: grep "publickey" in src/core/ — find authentication sequence files (priority: first)
- [ ] Task 2: grep "Authenticating" in src/core/ — find UI/auth flow files
- [ ] Task 3: STOP if no fix found after 15 files — report "insufficient information"

### Phase 2: Identify Root Cause
- [ ] Task 4: grep "PPK" or "LoadKey" in src/base/ — find key loading code
- [ ] Task 5: grep "fallback" near "auth" in src/core/ — find auth fallback logic
- [ ] Task 6: Identify root cause and document in code comment

### Phase 3: Apply Fix
- [ ] Task 7: Apply minimal surgical fix to authentication code
- [ ] Task 8: Verify fix is in PUBLICKEY auth path (grep — not dead code)
- [ ] Task 9: Build with `cmd /c build-x64.bat` — must pass with zero warnings
- [ ] Task 10: Verify no trailing whitespace introduced

### Phase 4: Report
- [ ] Task 11: Include modified function/class name in completion report

## Stop Conditions
- Stop and ask BEFORE:
  - Deleting any file
  - Adding any NEW source file to the project
  - Touching any code in `libs/`

Modifying existing files in src/core/ or src/base/ is OK without asking.

## Done Criteria
- [ ] Root cause identified and documented
- [ ] Minimal fix applied to authentication code
- [ ] Build passes with zero warnings
- [ ] Modified code verified reachable in publickey auth path