# Implementation Plan: SSH Private Key Authentication Bug Fix

Branch: none (bug fix)
Created: 2026-04-22

Superseded by: `.ai-factory/plans/fix-ssh-private-key-cert-392.md` (15 tasks, fully implemented on lmv/dev)
Root cause: `DoPromptUser()` passphrase/password misidentification + `PromptUser()` inverted Result check.
See commits: 33f26efe7, f193293ca, 9e608fcab, 49e86a5a8
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
- [x] Task 1: grep "publickey" — SUPERSEDED by comprehensive fix plan
- [x] Task 2: grep "Authenticating" — SUPERSEDED by comprehensive fix plan
- [x] Task 3: STOP if no fix found — SUPERSEDED (fix found and applied)

### Phase 2: Identify Root Cause
- [x] Task 4: grep "PPK" or "LoadKey" — SUPERSEDED by comprehensive fix plan
- [x] Task 5: grep "fallback" near "auth" — SUPERSEDED by comprehensive fix plan
- [x] Task 6: Identify root cause — SUPERSEDED: two bugs found

### Phase 3: Apply Fix
- [x] Task 7: Apply surgical fix — SUPERSEDED: committed in f193293ca
- [x] Task 8: Verify fix in publickey path — SUPERSEDED: comprehensive plan verified
- [x] Task 9: Build zero warnings — SUPERSEDED: x64 clean build
- [x] Task 10: No trailing whitespace — SUPERSEDED

### Phase 4: Report
- [x] Task 11: Report modified function/class — SUPERSEDED: Terminal.cpp DoPromptUser(), SecureShell.cpp PromptUser()

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