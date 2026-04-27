Reference: https://github.com/michaellukashov/Far-NetBox/issues/511
# Investigation Plan: NetBox Issue #511 – Download speed limit ineffective

## Goal
Determine why the download speed limit set in NetBox’s transfer dialog does not restrict actual download speed for SSH/SFTP transfers.

## Non‑Goals
- Implement the fix (will be a separate task).
- Modify unrelated UI components.

## Context
- Affected NetBox versions: 24.2.2.590 → 24.12.2.608.
- Far Manager versions: 3.0.6278.3723 → 3.0.6614.4704.
- OS: Windows 10 / Windows Server 2019.
- Issue reported for SSH connections (unclear if SFTP also affected).

## Investigation Steps
1. **Reproduce**: Build latest NetBox, connect via SSH, open download dialog (F5), set speed limit, start transfer, verify unlimited speed.
2. **Enable DEBUG logging** in tinylog, repeat transfer, capture `%LOCALAPPDATA%\NetBox\netbox.log`, search for entries related to speed limiting, transfer threads, `TSecureShell`, `TSFTPFileSystem`.
3. **Locate UI code** that reads the speed‑limit value (likely in dialog classes under `src/NetBox/` or `src/windows/`) and verify how it is stored (e.g., in `TSessionData`).
4. **Trace transfer path**:
   - SSH: `TSecureShell::GetFile` → PuTTY SCP implementation.
   - SFTP: `TSFTPFileSystem`.
   Look for throttling logic (`SetSpeedLimit`, `Throttle`, `RateLimiter`). Confirm if limit is applied.
5. **Compare with older tag** where the limit behaved correctly (if available); diff relevant source files to identify regressions.
6. **Check platform‑specific code** for `#if` guards that might disable the limit on newer Windows builds and ensure the limit is passed to PuTTY.
7. **Summarize findings**: state whether the limit is never set, stripped, overridden, or ignored; pinpoint source file/function; propose minimal fix (e.g., pass limit to PuTTY loop or add client‑side throttler).

## Deliverables
- Concise Markdown report with log excerpts and code locations.
- List of files/functions requiring modification.
- (Optional) Prototype patch demonstrating speed‑limit enforcement.

## Acceptance Criteria
- Reproduction steps are verified and logged.
- Root cause is pinpointed to a specific code path.
- Investigation does not introduce new build warnings or errors.
