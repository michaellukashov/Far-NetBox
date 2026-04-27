# Investigation Plan: NetBox Issue #511 – Download speed limit ineffective (fast mode)

## Goal
Determine why the download speed limit set in NetBox’s transfer dialog does not restrict actual download speed for SSH/SFTP transfers.

## Non‑Goals
- Implement the fix (this will be a separate task).
- Modify unrelated UI components.

## Context
- Affected NetBox versions: `24.2.2.590` → `24.12.2.608`.
- Far Manager versions: `3.0.6278.3723` → `3.0.6614.4704`.
- OS: Windows 10 / Windows Server 2019.
- Issue reported for SSH connections (unclear if SFTP also affected).

## Investigation Steps (Fast Mode)
1. **Reproduce the problem**
   - Build the latest NetBox source.
   - Connect to an SSH server (e.g., local OpenSSH).
   - Open the download dialog (`F5`), set a speed limit (e.g., 100 KB/s), start the transfer.
   - Verify that the download speed is unlimited.
2. **Enable DEBUG logging**
   - Set `tinylog` level to `DEBUG`.
   - Perform the transfer again and capture `%LOCALAPPDATA%\NetBox\netbox.log`.
   - Search the log for entries containing `SpeedLimit`, `TransferThread`, `TSecureShell`, `TSFTPFileSystem`.
3. **Locate UI handling code**
   - Search for the dialog class that reads the speed‑limit value (likely under `src/NetBox/` or `src/windows/`).
   - Verify how the value is stored (e.g., in `TSessionData`).
4. **Trace the transfer path**
   - **SSH path**: `TSecureShell::GetFile` → PuTTY SCP implementation.
   - **SFTP path**: `TSFTPFileSystem`.
   - Look for any throttling logic (`SetSpeedLimit`, `Throttle`, `RateLimiter`). Confirm whether the limit is applied.
5. **Compare with older releases**
   - If a tag where the limit worked exists, checkout that version.
   - Diff the relevant source files to identify regressions.
6. **Check platform‑specific guards**
   - Search for `#if`/`#ifdef` that may disable the limit on newer Windows builds.
   - Ensure the limit value is passed to the PuTTY library.
7. **Document findings**
   - Indicate whether the limit is never set, stripped, overridden, or ignored.
   - Pinpoint the exact source file and function responsible.
   - Propose a minimal fix (e.g., propagate the limit to PuTTY or add client‑side throttling).

## Deliverables
- A concise Markdown report with log excerpts and code locations.
- A list of files/functions that need modification.
- (Optional) A prototype patch that enforces the speed limit.

## Acceptance Criteria
- Reproduction steps are verified and logged.
- Root cause is pinpointed to a specific code path.
- No new build warnings or errors are introduced during investigation.
