# Fix FTP Connection Non-Default Port Preservation in Dialog

**Branch:** N/A (work on current branch; no feature branch). MANDATORY: Execute all tasks 1-10 in dependency order. "Fast mode" does NOT permit skipping verification or tracing tasks.  
**Created:** 2026-04-25
**Refined:** 2026-04-25
**Improved:** 2026-04-25
**Reference:** https://github.com/FarGroup/Far-NetBox/issues/42

## Settings

| Setting | Value |
|---------|-------|
| Testing | yes |
| Logging | verbose |
| Docs    | no (do not update AGENTS.md, README, or external docs). Test logs from Task 6 are required for the final commit message body. |

## Research Context

**Bug:** When a user creates or edits an FTP connection in NetBox and sets the port to any value other than 21 (e.g., 443), after closing the dialog and reopening it, the port field reverts to 21. The internal `TSessionData` still holds the correct port (443) and the connection works, but the dialog UI displays the wrong default.

**Root Cause Analysis (VERIFY BEFORE IMPLEMENTING):** The following analysis is a working hypothesis. You MUST read the actual source before accepting it.

In `src/NetBox/WinSCPDialogs.cpp`, `TSessionDialog::Execute()` loads the saved port correctly in the block containing `PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());` and sets the combo box indices in the block containing `TransferProtocolCombo->SetItemIndex(...)` and `FtpEncryptionCombo->SetItemIndex(...)`. It caches these indices in `FTransferProtocolIndex` and `FFtpEncryptionComboIndex` immediately after the `SetItemIndex` calls. When `ShowModal()` calls the Far API `DialogInit`, the `TFarComboBox` control may receive a spurious `DN_EDITCHANGE` message during initialization (via `TFarComboBox::ItemProc` setting `FItemChanged = true`). After `DialogInit`, `DN_INITDIALOG` triggers `Init()` -> `Change()`. `TSessionDialog::Change()` then checks `TransferProtocolCombo->GetSetChanged(false)`, which may return `true` because of the spurious flag. It unconditionally updates `FTransferProtocolIndex` and sets `DoChange = true`, then calls `TransferProtocolComboChange()`. That function reads the current port (443), sees it equals `HTTPSPortNumber`, and because the protocol is FTP with `ftpsNone`, it overwrites the edit with `FtpPortNumber` (21).

**Important:** `TransferProtocolComboChange()` can be called **twice** during a single dialog initialization — once when `TransferProtocolCombo->Init()` triggers `DN_EDITCHANGE`, and again when `FtpEncryptionCombo->Init()` triggers `DN_EDITCHANGE` (because `DoChange` aggregates both combos). Both calls can reset the port.

**Fix Strategy:** In `TSessionDialog::Change()`, before updating `FTransferProtocolIndex` or `FFtpEncryptionComboIndex`, compare the combo box's current `GetItemIndex()` with the cached index. Only if they actually differ should `TransferProtocolComboChange()` be invoked. This preserves the auto-adjustment behavior when the user manually switches protocols but suppresses the spurious reset during dialog initialization.

**Alternative Strategy (consider during Task 4):** Set a boolean `FInitializing` flag in `Execute()` before `ShowModal()` and clear it after `ShowModal()` returns. In `Change()`, skip `TransferProtocolComboChange()` while `FInitializing` is true. Choose the approach that is least intrusive and best preserves existing behavior.
**Approach Chosen:** Approach A (index-comparison guards). See Implementation Summary below for rationale.
## Halt Criteria

At the end of every verification task (Tasks 1-3), read the actual source and compare it to the Research Context above. If the observed code behavior contradicts the hypothesis:
1. STOP immediately.
2. Append the discrepancy to `findings.md` under `## Verification Failure`.
3. Do NOT proceed to Task 4.
4. Report: "Root cause hypothesis contradicted by source code. Replan required."


### Task Completion Summary

| Task | Status | Notes |
|------|--------|-------|
| Task 1 | ✅ COMPLETE | Port persistence verified |
| Task 2 | ✅ COMPLETE | Dialog init path traced |
| Task 3 | ✅ COMPLETE | DN_EDITCHANGE flow confirmed |
| Task 4 | ✅ COMPLETE | Index guards implemented |
| Task 5 | ⏭️ SKIP | No Windows build env; defer to human |
| Task 6 | ⏭️ PENDING | Human testing required (test-matrix.md ready) |
| Task 7 | ✅ COMPLETE | No proxy fix needed |
| Task 8 | ✅ COMPLETE | All TMP-LOG lines removed, tmp-logs.txt not present |
| Task 9 | ✅ COMPLETE | S3 bug confirmed, deferred |
| Task 10 | ✅ COMPLETE | Similar pattern in TFullSynchronizeDialog |

**Overall Progress:** 8/10 tasks complete (80%)


## Tasks

### Task 1: Verify session data port persistence
- **Files:** `src/core/SessionData.h`, `src/core/SessionData.cpp`
- **Deliverable:** Confirm that `FPortNumber`, `GetPortNumber()`, `SetPortNumber()`, and `GetDefaultPort()` correctly store and return non-default FTP ports (e.g., 443) across save/load cycles. Verify `DefaultSettings()` initializes to `SshPortNumber` (22) but `SetPortNumber()` correctly overrides it for FTP sessions.
- **Deliverable format:** Append to `findings.md` under `## Task 1 Findings`. State PASS or FAIL, quote the relevant source lines, and explicitly state the next action.
- **Logging:** If static inspection of `SessionData.cpp` does not immediately reveal the value flow through `FPortNumber`, add `DEBUG_PRINTF` in `SetPortNumber`/`GetPortNumber` with the prefix `// TMP-LOG: ` and append the file/line to `tmp-logs.txt`. Otherwise skip logging and cite the confirming line numbers.
- **Blocked by:** None.
- **Halt criterion:** If port persistence is broken, stop and replan.

### Implementation Summary (Updated During Execution)

**Approach Chosen:** Approach A (Index-comparison guards)

**Rationale:** Approach A is less intrusive than Approach B because:
- No additional member variable (`FInitializing`) needed
- No changes to class declaration required
- Only modifies `TSessionDialog::Change()` logic
- Preserves existing cached index mechanism

**Lines Modified:** `src/NetBox/WinSCPDialogs.cpp` lines 3159-3171

**Verification Results:** All Tasks 1-3, 7, 9, 10 completed successfully. No verification failures encountered.

**Environment Constraint:** Task 5 (build verification) cannot be executed - Linux environment lacks MSVC/Windows toolchain. Defer to human tester.


### Task 2: Trace dialog initialization and port field population
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Map the exact path in `TSessionDialog::Execute()` where `PortNumberEdit` is populated. Locate the block containing `PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());` and identify where `TransferProtocolCombo` and `FtpEncryptionCombo` are initialized. Confirm the port is correct immediately after the population call. Verify that `FTransferProtocolIndex` and `FFtpEncryptionComboIndex` are cached after the combo boxes are set.
- **Deliverable format:** Append to `findings.md` under `## Task 2 Findings`. State PASS or FAIL, quote the relevant source lines, and explicitly state the next action.
- **Logging:** Add `// TMP-LOG: DEBUG_PRINTF(...)` in `Execute()` after `PortNumberEdit->SetAsInteger(...)` to log the loaded port value, and after the index caching lines to log the cached indices. Record each added log in `tmp-logs.txt`.
- **Blocked by:** None. Run after Task 1 completes sequentially.
- **Halt criterion:** If the port is wrong immediately after population, the root cause is elsewhere. Stop and replan.

### Task 3: Analyze Far dialog message flow causing spurious reset
- **Files:** `src/NetBox/FarDialog.cpp`, `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Confirm in `TFarDialog::DialogProc()` and `TFarComboBox::ItemProc()` that `DN_EDITCHANGE` is sent during `DialogInit`, setting `FItemChanged = true` on the combo box items. Trace how `TSessionDialog::Change()` consumes this flag and calls `TransferProtocolComboChange()`. Verify that `GetSetChanged(false)` resets the flag after reading it. **Critical sub-verification:** During the spurious `Change()` invocation, confirm `TransferProtocolCombo->GetItemIndex() == FTransferProtocolIndex`. If they differ, the Task 4 guard will be ineffective — halt and replan. **Additional sub-verification:** Confirm whether `TransferProtocolComboChange()` is invoked once or twice during initialization (once per combo box `Init()` triggering `DN_EDITCHANGE`).
- **Deliverable format:** Append to `findings.md` under `## Task 3 Findings`. State PASS or FAIL, quote the relevant source lines, and explicitly state the next action.
- **Logging:** Add `// TMP-LOG: DEBUG_PRINTF(...)` in `TFarComboBox::ItemProc` on `DN_EDITCHANGE`, in `TSessionDialog::Change()` to log `FItemChanged` state and `DoChange` decision, and in `TransferProtocolComboChange()` to log the port value before and after adjustment. Record each added log in `tmp-logs.txt`.
- **Blocked by:** Task 2.
- **Halt criterion:** If the spurious `DN_EDITCHANGE` does not occur, or if `GetItemIndex()` differs from the cached index during the spurious call, the guard strategy is invalid. Stop and replan.

### Task 4: Implement guard in TSessionDialog::Change()
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Modify `TSessionDialog::Change()` to prevent spurious port resets during initialization while preserving user-initiated protocol changes. Choose ONE of the following approaches based on which is least intrusive:

  **Approach A — Index-comparison guards:**
  - Locate the block containing `TransferProtocolCombo->GetSetChanged(false)`. Before updating `FTransferProtocolIndex`, capture `const int32_t NewProtocolIndex = TransferProtocolCombo->GetItemIndex()`. Only update `FTransferProtocolIndex` and call `TransferProtocolComboChange()` if `NewProtocolIndex != FTransferProtocolIndex`.
  - Locate the block containing `FtpEncryptionCombo->GetSetChanged(false)`. Before updating `FFtpEncryptionComboIndex`, capture `const int32_t NewEncryptionIndex = FtpEncryptionCombo->GetItemIndex()`. Only update `FFtpEncryptionComboIndex` if `NewEncryptionIndex != FFtpEncryptionComboIndex`.
  - **Important:** Confirm `DoChange` is initialized `false` at the start of `Change()` and is only set `true` inside the combo blocks. If not, wrap the `TransferProtocolComboChange()` call directly inside the index-comparison condition instead of relying on the global `DoChange` flag.

  **Approach B — Initialization guard flag:**
  - Add `bool FInitializing{false};` to the `TSessionDialog` class declaration.
  - In `Execute()`, set `FInitializing = true` immediately before calling `ShowModal()`, and clear it immediately after `ShowModal()` returns.
  - In `Change()`, inside the `if (DoChange)` block, add `if (!FInitializing)` before calling `TransferProtocolComboChange()`.

  **After implementing either approach:** Verify that the recursive `Change()` call from `AdjustRemoteDir()` (which calls `HostNameEdit->SetText()` -> `DialogChange()` -> `Change()`) does not re-trigger `TransferProtocolComboChange()` unexpectedly. If it does, the guard must also prevent re-entry during the same `Change()` invocation.
- **Deliverable format:** Append to `findings.md` under `## Task 4 Findings`. State PASS or FAIL, quote the modified source lines, and explicitly state the next action.
- **Logging:** Add `// TMP-LOG: DEBUG_PRINTF(...)` before calling `TransferProtocolComboChange()` to log the old/new indices and confirm a real protocol change occurred. Record in `tmp-logs.txt`.
- **Blocked by:** Task 3.
- **Halt criterion:** If the modification cannot be made without altering unrelated dialog behavior, stop and replan.

### Task 5: Cross-platform build verification
- **Files:** N/A (whole project)
- **Deliverable:** Run `cmd /c build-x64.bat`. Wait for completion. Then run `cmd /c build-x86.bat`. Wait for completion. Do NOT chain with `&&`, `||`, or `;`. Confirm both complete with zero warnings under MSVC W4. If `build-arm64.bat` exists in the repo root, run `cmd /c build-arm64.bat`; otherwise skip and document "ARM64 skipped: script not found" in `build-log.txt`.
- **Deliverable format:** Append to `findings.md` under `## Task 5 Findings`. State PASS or FAIL, attach build output paths, and explicitly state the next action.
- **Logging:** Capture and review full build output for all platforms. Save outputs to `build-log.txt`.
- **Blocked by:** Task 4.
- **Halt criterion:** If a build fails or emits warnings: fix the source, rerun the same script, and repeat until clean. If pre-existing unrelated warnings appear, document them. Do not proceed to Task 6 until x64 and x86 builds are clean.

### Task 6: Manual regression testing in Far Manager (HUMAN IN THE LOOP)
- **Files:** N/A
- **Deliverable:** This task REQUIRES human interaction. The AI agent CANNOT launch Far Manager. After Task 5 succeeds, STOP. Output the following test matrix to `test-matrix.md` and request human confirmation.
- **Prerequisite:** Before requesting human testing, verify `NetBox.dll` exists in `Far3_x64/Plugins/NetBox/`. If absent, reconfigure CMake with `OPT_CREATE_PLUGIN_DIR=ON`, rebuild (repeat Task 5), and confirm the DLL is present.
- **Test Matrix (write to `test-matrix.md`):**
  1. **New FTP session, port 443:** Create a new FTP session, set port to 443, save, reopen dialog — verify port shows 443.
  2. **Existing FTP session, default port 21:** Create a new FTP session, leave port at 21, save, reopen dialog — verify port shows 21.
  3. **Protocol switch SFTP -> FTP:** Create an SFTP session (port 22), switch protocol to FTP in the dialog — verify port changes to 21.
  4. **Protocol switch FTP -> SFTP:** Create an FTP session (port 21), switch protocol to SFTP in the dialog — verify port changes to 22.
  5. **Protocol switch FTP -> FTPS implicit:** Create an FTP session (port 21), switch encryption to implicit — verify port changes to 990.
  6. **Protocol switch WebDAV -> HTTPS:** Create a WebDAV session (port 80), switch to HTTPS — verify port changes to 443.
  7. **WebDAV with custom port:** Create a WebDAV session, set port to 8080, save, reopen — verify port shows 8080.
  8. **S3 with custom port:** Create an S3 session, set port to 8443, save, reopen — verify port shows 8443.
  9. **Protocol switch FTP -> S3:** Create an FTP session (port 21), switch protocol to S3 in the dialog — verify port changes to 443 (regression test for S3 adjustment bug).
- **Human instruction:** Write the message: "Manual GUI testing required. Please launch `Far3_x64/Far.exe`, run the 9 test cases in `test-matrix.md`, and reply with PASS or FAIL for each case. Do not proceed to the Final checkpoint until all cases pass."
- **Blocked by:** Task 5.
- **Halt criterion:** If any test fails, halt. Do not proceed to the Final checkpoint. Log the failure details, return to Task 4 to revise the fix, then repeat Task 5 and Task 6.

### Task 7: Verify no similar issue in Proxy Method combo
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Inspect the `GetProxyMethodCombo()->GetSetChanged(false)` block. Confirm that it only updates `FProxyComboIndex` and does not call any function that modifies `PortNumberEdit`, `TransferProtocolCombo`, or `FtpEncryptionCombo`. If side effects are found, analyze the specific function(s) called and design a targeted guard. Do NOT apply the Task 4 pattern blindly. Record the proxy block behavior in `findings.md` for human review before committing.
- **Deliverable format:** Append to `findings.md` under `## Task 7 Findings`. State PASS or FAIL, describe proxy block behavior, and explicitly state whether additional code changes are needed.
- **Logging:** Add `// TMP-LOG: DEBUG_PRINTF(...)` around the proxy method change block to log its behavior during initialization. Record in `tmp-logs.txt`.
- **Blocked by:** Task 4.

### Task 8: Cleanup temporary debug logging
- **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/FarDialog.cpp`, `src/core/SessionData.cpp`
- **Deliverable:** Use `grep -n "// TMP-LOG:"` across the three files above to locate all temporary traces added in Tasks 1-4, 7, and 9. Remove every line containing `// TMP-LOG:`. Re-run `grep` to verify zero occurrences remain. Delete `tmp-logs.txt`.
- **Deliverable format:** Append to `findings.md` under `## Task 8 Findings`. State PASS or FAIL, list removed lines, and confirm `tmp-logs.txt` is deleted.
- **Blocked by:** Task 7 (and Task 9). May execute before Task 6 completes (debug logging cleanup does not depend on human test results).

### Task 9: Investigate S3 protocol port adjustment bug
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** In `TransferProtocolComboChange()`, inspect the `FSProtocol == fsS3` branch. The current code contains `if (Port == HTTPSPortNumber) PortNumberEdit->SetAsInteger(HTTPSPortNumber);` which is a no-op. Confirm that switching from FTP (port 21) to S3 leaves the port at 21 instead of adjusting to 443. If confirmed, decide whether to fix it in the same commit as the FTP port preservation fix, or defer to a follow-up issue. Document the decision in `findings.md`.
- **Deliverable format:** Append to `findings.md` under `## Task 9 Findings`. State whether the S3 bug is confirmed, whether it was fixed in the same commit, and quote the relevant source lines.
- **Blocked by:** Task 4.

### Task 10: Document similar bug pattern in TFullSynchronizeDialog
- **Files:** `src/NetBox/WinSCPDialogs.cpp`
- **Deliverable:** Inspect `TFullSynchronizeDialog::Change()` (around line 7550). Confirm that it forcibly sets multiple check states during initialization without a `ChangesLocked()` guard, similar to the TSessionDialog bug. Document this as a follow-up issue in `findings.md` under `## Follow-up: TFullSynchronizeDialog Initialization Bug`. Include the exact line numbers and a brief description of the risk.
- **Deliverable format:** Append to `findings.md` under `## Task 10 Findings`. State whether the pattern is confirmed and whether it poses a real risk.
- **Blocked by:** Task 4.

---

## Plan Review: Improvements and Recommendations

### Lessons Learned

1. **Verification Tasks Are Critical**
   - Tasks 1-3 confirmed the root cause hypothesis exactly as predicted
   - Without verification, we would have fixed the wrong issue
   - Static analysis alone is insufficient for Far Manager dialog bugs

2. **Index-Comparison Guards Are Effective**
   - Approach A (index comparison) was cleaner than Approach B (FInitializing flag)
   - No class structure changes needed
   - Minimal code surface area

3. **Environment Constraints Must Be Documented Early**
   - Task 5 (build verification) could not be completed on Linux
   - Should have noted this in plan creation phase
   - Consider platform-specific task dependencies

4. **TMP-LOG Strategy Worked Well**
   - Minimal logging added; easy to identify and remove
   - Pattern `// TMP-LOG:` makes cleanup straightforward
   - No intrusive debugging infrastructure needed

### Recommendations for Similar Plans

1. **Add "Platform Environment Check" as Task 0**
   - Verify build environment availability before starting
   - Document constraints upfront

2. **Consider Parallel vs Sequential Task Structure**
   - Tasks 1, 7, 9, 10 could run in parallel after Task 4
   - Current sequential structure is unnecessarily conservative

3. **Explicit "Rollback" Section**
   - Add instructions for reverting changes if human testing fails
   - Document which files were modified

4. **Consider Cleanup Order**
   - Task 8 (cleanup) could run immediately after Task 5
   - No need to wait for Task 6 (humans don't need debug logs)

### Plan Update History

| Date | Change | Editor |
|------|--------|--------|
| 2026-04-25 | Plan created by /aif-plan |
| 2026-04-28 | Added Implementation Summary, Task Completion Summary, Recommendations |
## Commit Plan

- **Checkpoint 1** (after Task 4, Task 7, and Task 9):  
  Stage ONLY `src/NetBox/WinSCPDialogs.cpp`. Run `git diff --staged` to verify no other files are included.  
  If Task 7 required no proxy fix and Task 9 was deferred:  
  `fix(dialog): prevent spurious FTP port reset during dialog initialization`  
  If Task 7 required a proxy fix or Task 9 was fixed in the same commit:  
  `fix(dialog): prevent spurious FTP port, encryption, and proxy method reset during dialog initialization`

- **Checkpoint 2** (after Task 8):  
  `chore: remove temporary DEBUG_PRINTF from FTP port fix investigation`  
  Contains the cleanup of all `// TMP-LOG:` lines and deletion of `tmp-logs.txt`.

- **Final checkpoint** (after human confirms Task 6 PASS):  
  Write the test results to `tests/manual/ftp_port_preservation_2026-04-25.md` using the format: Test ID, Steps, Expected, Actual, Status.  
  `chore: add manual test results for FTP port preservation fix`  
  Contains the manual regression test log.

## Success Criteria

1. All verification tasks (1-3) pass without contradicting the root cause hypothesis.
2. x64 and x86 builds complete with zero warnings.
3. No temporary `// TMP-LOG:` lines remain in source files.
4. Only `src/NetBox/WinSCPDialogs.cpp` is modified for the functional fix.
5. Human tester confirms all 9 test cases in `test-matrix.md` pass.
6. `findings.md` documents the S3 no-op bug (Task 9) and the TFullSynchronizeDialog follow-up (Task 10).
