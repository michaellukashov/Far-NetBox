# NetBox Logging Integration Test Plan

**Created:** 2026-04-25  
**Plan:** `.ai-factory/plans/logging-thread-safety.md`  
**Type:** Manual integration test

## Prerequisites

- Far Manager 3.0+ (x64 build)
- NetBox plugin built with `OPT_CREATE_PLUGIN_DIR=ON`
- SFTP server access (test server)
- Debug build or RelWithDebugInfo for structured log output

## Environment Setup

1. Build NetBox with plugin directory:
   ```cmd
   cmd /c build-x64.bat
   ```

2. Verify plugin DLL location:
   ```
   Far3_x64\Plugins\NetBox\NetBox.dll
   ```

3. Clear previous logs:
   ```cmd
   del %TEMP%\netbox-dbglog.txt
   del %TEMP%\*.log
   del %TEMP%\*.xml
   ```

## Test Scenarios

### Scenario 1: Basic File Copy (Download)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 1.1 | Launch Far Manager from `Far3_x64\Far.exe` | Far starts normally |
| 1.2 | Press F11 → NetBox | Plugin loads, session list appears |
| 1.3 | Connect to SFTP server | Connection established |
| 1.4 | Navigate to a file, press F5 (Copy) | Copy dialog appears |
| 1.5 | Select local destination, confirm | File downloads successfully |
| 1.6 | Check `%TEMP%\netbox-dbglog.txt` | Contains `[op=download]` structured context |
| 1.7 | Check `%TEMP%\<sessionname>.log` | Session protocol log exists and non-empty |
| 1.8 | Check `%TEMP%\<sessionname>.xml` | Action log exists and non-empty |

### Scenario 2: File Move (Upload + Delete)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 2.1 | Launch Far, connect to SFTP server | Connected |
| 2.2 | Navigate to local file, press F6 (Move) | Move dialog appears |
| 2.3 | Select remote destination, confirm | File uploaded, remote file appears |
| 2.4 | Check debug log | Contains `[op=upload]` structured context |

### Scenario 3: File Delete

| Step | Action | Expected Result |
|------|--------|-----------------|
| 3.1 | Launch Far, connect to SFTP server | Connected |
| 3.2 | Select a remote file, press F8 (Delete) | Delete confirmation appears |
| 3.3 | Confirm deletion | File deleted, no crash |
| 3.4 | Check debug log | Contains structured context for delete operation |

### Scenario 4: Regression — Issue #508 (Edit/View Sequential Opens)

**IMPORTANT:** This scenario validates the fix for the null dereference bug in Task 2.7.

| Step | Action | Expected Result |
|------|--------|-----------------|
| 4.1 | Launch Far, connect to SFTP server | Connected |
| 4.2 | Navigate to a text file, press Enter (Edit mode) | File opens in configured editor |
| 4.3 | Close the editor (don't modify the file) | Return to Far panel |
| 4.4 | WITHOUT refreshing (no Ctrl+R), press Enter on the SAME file again | File opens in editor again — NO CRASH |
| 4.5 | Close the editor | Return to Far panel |
| 4.6 | WITHOUT refreshing, press Enter on a DIFFERENT file | File opens in editor — NO CRASH |
| 4.7 | Check debug log | Contains `[op=edit_view]` and `CopyToLocal completed` entries |
| 4.8 | Verify NO `DebugAssert(FOpenedPlugins->GetCount() == 0)` assertion failure | No crash dialog appears |

### Scenario 5: Regression — Issue #508 Stress (5+ Opens)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 5.1 | Launch Far, connect to SFTP server | Connected |
| 5.2 | Press Enter on first text file, close editor | No crash |
| 5.3 | WITHOUT Ctrl+R, press Enter on second text file, close editor | No crash |
| 5.4 | WITHOUT Ctrl+R, press Enter on third text file, close editor | No crash |
| 5.5 | WITHOUT Ctrl+R, press Enter on fourth text file, close editor | No crash |
| 5.6 | WITHOUT Ctrl+R, press Enter on fifth text file, close editor | No crash |
| 5.7 | Check debug log | All 5 opens logged with `[src=...]` context |

### Scenario 6: Structured Logging Context Verification

| Step | Action | Expected Result |
|------|--------|-----------------|
| 6.1 | After any file operation, open `%TEMP%\netbox-dbglog.txt` | File exists |
| 6.2 | Search for `[op=` | Entries have operation type context |
| 6.3 | Search for `[src=` | Entries have source path context |
| 6.4 | Search for `GetFilesRemote start` | Entry shows count and move flag |
| 6.5 | Search for `CopyToLocal completed` | Entry appears after download |
| 6.6 | Search for `CopyDialog result=` | Entry shows confirm/cancel |
| 6.7 | Verify queue entries (if queued operations used) | `[op=...]` context present |

### Scenario 7: Lock Contention Check (Debug Build Only)

| Step | Action | Expected Result |
|------|--------|-----------------|
| 7.1 | Use debug build (`_DEBUG` defined) | Log includes lock contention warnings if any |
| 7.2 | Perform rapid sequential operations | Check for "Lock contention: waited" in log |
| 7.3 | If warnings appear | Contention time < 5% of total operation time |

## Log File Paths

| Log | Path | Content |
|-----|------|---------|
| Debug log | `%TEMP%\netbox-dbglog.txt` | Global debug output, structured context |
| Session log | `%TEMP%\<sessionname>.log` | Session-specific protocol log |
| Action log | `%TEMP%\<sessionname>.xml` | XML action log for operations |

## Pass Criteria

- [ ] All 7 scenarios complete without crash
- [ ] Scenario 4 and 5 pass (Issue #508 regression)
- [ ] No `DebugAssert` failures
- [ ] Log files exist at all 3 expected paths
- [ ] Debug log contains `[op=...][src=...]` structured context
- [ ] Debug log contains `GetFilesRemote start`, `CopyToLocal completed`, `CopyDialog result`
- [ ] Lock contention < 5% (debug build only)
- [ ] Plugin loads and unloads normally after all operations

## Results

| Scenario | Result | Notes |
|----------|--------|-------|
| 1: Basic Copy | ⏳ Pending | |
| 2: File Move | ⏳ Pending | |
| 3: File Delete | ⏳ Pending | |
| 4: #508 Regression | ⏳ Pending | |
| 5: #508 Stress | ⏳ Pending | |
| 6: Structured Context | ⏳ Pending | |
| 7: Lock Contention | ⏳ Pending | |

**Tester:** ___________  
**Date:** ___________  
**Build:** x64 RelWithDebugInfo  
**NetBox version:** ___________
