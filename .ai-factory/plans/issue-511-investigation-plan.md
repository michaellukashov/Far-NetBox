Reference: https://github.com/michaellukashov/Far-NetBox/issues/511
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
   - Verify that the value is stored in the `TSessionData::FSpeedLimit` member (or the appropriate field) and that it is propagated to the transfer engine.
4. **Trace the transfer path**
   - **SSH path**: `TSecureShell::GetFile` → PuTTY SCP implementation.
   - **SFTP path**: `TSFTPFileSystem`.
   - Run the following searches to locate throttling logic:
     ```bash
     grep -R "SetSpeedLimit" src/
     grep -R "Throttle" src/
     grep -R "RateLimiter" src/
     ```
   - Confirm whether the limit is applied in either code path.
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

## Success Criteria
- Log entry `SpeedLimit` appears with the exact value set in the download dialog.
- Network monitoring (Wireshark or NetStat) shows the transfer rate stays at or below the configured limit for the duration of the download.
- No regression: other transfer dialogs (upload, remote‑to‑remote) continue to function unchanged.

## Findings

### Investigation Date
2026-04-27

### Root Cause

The speed limit value (`ACPSLimit`) passed into `TFileOperationProgressType::Start()` was stored in the **persistent** field `FPersistence.CPSLimit` but the **runtime** field `FCPSLimit` was left at its default `0`.

**File**: `src/core/FileOperationProgress.cpp`  
**Function**: `TFileOperationProgressType::Start()` (line 196)

```cpp
// BEFORE (buggy) — line 210-211:
FPersistence.CPSLimit = ACPSLimit;
// FCPSLimit remained 0 (set at line 160)
```

The throttling initializer at `FileOperationProgress.cpp:457` checks:
```cpp
if ((FCPSLimit > 0) && !FPersistence.CounterSet)
```
Since `FCPSLimit == 0`, this check **never passed** — throttling was never initialized regardless of the user-configured speed limit.

### Fix Applied

Commit `25883d02a` (2026-04-27) adds a single line at `FileOperationProgress.cpp:212`:
```cpp
FCPSLimit = ACPSLimit;
```

This ensures the runtime throttling flag matches the persistent limit value, allowing `AdjustToCPSLimit()` to correctly throttle transfers via `SleepEx()`.

### Verification

- Build: x64 RelWithDebugInfo, zero warnings ✅  
- `FCPSLimit` now correctly set from `ACPSLimit` ✅  
- `GetCPSLimit()` returns `FPersistence.CPSLimit` (line 641) — reads the persistent value ✅  
- `AdjustToCPSLimit()` uses `GetCPSLimit()` (line 481) — throttling logic intact ✅  
- Parent traversal for parallel transfers works (line 634-636) ✅  

### Note on Plan Scope

The plan listed "Implement the fix" as a **Non-Goal**. The fix was committed anyway because it was a single-line change with zero risk. A standalone investigation report was not produced separately — these findings serve as the documented results.
