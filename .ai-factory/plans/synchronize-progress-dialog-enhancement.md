# Implementation Plan: Synchronize Progress Dialog Enhancement

**Branch:** feature/sync-progress-enhancement  
**Created:** 2026-05-11  
**Plan Type:** task (focused enhancement)  
**Scope:** Enhance the synchronization progress dialog to display real-time statistics (files scanned, files to transfer, bytes transferred, ETA) during sync operations.

---

## Settings

- **Testing:** yes — test plans included per milestone
- **Logging:** verbose — detailed DEBUG logs for all new features
- **Docs:** yes — mandatory docs checkpoint at completion

---

## Research Context

The current `TSynchronizeDialog` (in `src/NetBox/WinSCPDialogs.cpp`) only shows:
  - A 3-line `CopyParamLister` log viewer (bounded to 100 lines)

However, the codebase ALREADY has a separate progress reporting mechanism in `TWinSCPFileSystem::TerminalSynchronizeDirectory` (lines 1665-1711) that shows a message box with:
  - Local and remote directory names
  - Start time and elapsed time
  - Updates every 500ms using existing MsgIDs (`NB_SYNCHRONIZE_PROGRESS_TITLE`, `NB_SYNCHRONIZE_PROGRESS_LOCAL`, `NB_SYNCHRONIZE_PROGRESS_START_TIME`, `NB_SYNCHRONIZE_PROGRESS_ELAPSED`)

The dialog and the message box are separate UI elements. The enhancement should integrate this progress information into the dialog itself, plus add:
  - Files scanned count
  - Files transferred count
  - Bytes transferred
  - Estimated time of arrival (ETA)
  - Transfer speed


Enhancing this dialog will provide users with informative feedback during long synchronization operations, improving usability.

## Genuine Gap Identified

This corresponds to **Phase 4 Task 4.2** from the WinSCP feature alignment roadmap:  
*"Enhance existing TSynchronizeDialog with real-time statistics. Show: files scanned, files to transfer, bytes transferred, ETA."*

## Phased Implementation Plan

### Task 4.2: Synchronize progress dialog enhancement

**Goal:** Modify `TSynchronizeDialog` to display real-time synchronization statistics.

**Affected files:**
  - `src/NetBox/WinSCPDialogs.cpp` (TSynchronizeDialog class)
  - `src/NetBox/WinSCPFileSystem.cpp` (TerminalSynchronizeDirectory progress integration)
  - `src/core/Terminal.cpp` (SynchronizeApply, TFileOperationStatistics)
  - `src/core/FileOperationProgress.h` (TFileOperationStatistics)
  - `src/windows/SynchronizeController.cpp` (TSynchronizeController)

**Implementation steps:**
1. Add progress counter fields to `TSynchronizeDialog` (`FFilesScanned`, `FFilesTransferred`, `FBytesTransferred`, `FSyncStartTime`) and update them in `DoLog()` when `slScan`/`slUpload`/`slDelete` events are received. The existing `DoLog` callback already receives all synchronization events from `TSynchronizeController::LogOperation` — no controller changes needed for basic event counting.

2. Modify `TWinSCPFileSystem::Synchronize()` to create a `TFileOperationStatistics` instance and pass it to `FTerminal->SynchronizeApply()` (currently passes `nullptr`). Add a new callback or extend the existing progress mechanism to expose `TFileOperationStatistics` data (files uploaded/downloaded/deleted, total bytes) to the dialog for accurate byte/file counts.

3. Integrate existing `TerminalSynchronizeDirectory` progress into the dialog. The existing message box (lines 1665-1711 in `WinSCPFileSystem.cpp`) shows local/remote directories, start time, and elapsed time. Move this display into the dialog by passing the directory/time info through the existing `DoLog` or a new lightweight callback, then remove or conditionally disable the separate message box when the dialog is active.

4. Update `TSynchronizeDialog` to include new UI elements for statistics display. Reuse existing MsgIDs where possible (`NB_SYNCHRONIZE_PROGRESS_TITLE`, `NB_SYNCHRONIZE_PROGRESS_LOCAL`, `NB_SYNCHRONIZE_PROGRESS_START_TIME`, `NB_SYNCHRONIZE_PROGRESS_ELAPSED`). Add new text labels for: files scanned, files transferred, bytes transferred, ETA, transfer speed. Consider expanding dialog height from current 20 lines to accommodate new elements.

5. Add `Idle()` override to `TSynchronizeDialog` following the `TQueueDialog` pattern (`Idle()` + `PostMainThreadSynchro`) for periodic UI refresh at 500ms intervals. Add `UpdateProgressDisplay()` method to format statistics using `base::FormatBytes` (following `TQueueDialog::FillQueueItemLine` pattern) and update the UI text elements. Call `UpdateProgressDisplay()` from both `DoLog()` (when events occur) and `Idle()` (for periodic refresh).

6. Ensure the dialog remains responsive and does not block the synchronization thread. All UI updates are marshaled to the main thread via existing mechanisms (`DoLog` is already called from the main thread via `Synchronize()` callback chain; `Idle()` + `PostMainThreadSynchro` ensures main-thread execution).

7. Add any missing message IDs to `MsgIDs.h` and update all `.lng` files. Reuse existing `NB_SYNCHRONIZE_PROGRESS_*` MsgIDs where possible. Only add new IDs for statistics labels that don't have existing equivalents (e.g., files transferred count, bytes transferred, ETA, speed).

8. Verify that the enhancement works for all synchronization modes (local, remote, both) and for both one-way and two-way sync. Ensure `TerminalSynchronizeDirectory` message box still appears when dialog is not used (e.g., silent mode or background sync without UI).

**Logging:**
Add log traces such as:
`FTerminal->LogEvent(L"Sync progress: %d/%d files, %s bytes, ETA: %s", Scanned, Total, base::FormatBytes(Transferred), EtaString);`

**Note:** Use `base::FormatBytes` for byte formatting (following `TQueueDialog::FillQueueItemLine` pattern) and `FORMAT` macro for percentage/ETA formatting.

**Dependencies:**
  - Depends on Phase 4.1 (Directory comparison mode) being complete (it is).
  - Does not depend on Phase 3 (workspace) as it is UI-only.
  - Follows `TQueueDialog` pattern for `Idle()` + `PostMainThreadSynchro` periodic refresh.

**Commit checkpoint:**
```
feat(ui): enhance synchronize dialog with real-time progress statistics

- Added files scanned, files transferred, bytes transferred, ETA to TSynchronizeDialog
- Integrated existing TerminalSynchronizeDirectory progress into dialog
- Wired TFileOperationStatistics from SynchronizeApply to dialog
- Added Idle() + PostMainThreadSynchro periodic refresh (following TQueueDialog pattern)
- Reused existing NB_SYNCHRONIZE_PROGRESS_* MsgIDs where possible
- Updated .lng files with new message strings
```

---

## Dependencies

- Phase 4.1 (Directory comparison mode) must be complete (it is).
- No other phases are required for this UI enhancement.

## Success Criteria

  - Build zero warnings.
  - Manual test: Perform a synchronization (e.g., SFTP sync) and observe the dialog updates with accurate, real-time statistics.
  - Statistics should update at least every 500ms during active sync (matching existing TerminalSynchronizeDirectory refresh interval).
  - Dialog remains usable (can click Stop, Transfer Settings, etc.) while sync is in progress.
  - No regression in existing synchronization functionality.
  - All `.lng` files updated and aligned (run `python scripts/verify_lng_alignment.py`).

## Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| UI updates cause flickering or performance degradation | Medium | Medium | Update statistics at a fixed interval (e.g., 500ms) and only when values change; use efficient string formatting. |
| Thread safety issues when updating UI from worker thread | Low | High | Ensure all UI updates are marshaled to the main thread (already done via existing mechanisms like `DoLog`). |
| Localization: missing string IDs in `.lng` files | Low | Medium | Run the string alignment verification script before committing. |
| Inaccurate ETA calculation | Low | Low | Use simple linear extrapolation based on elapsed time and bytes transferred; label as estimate. |

## Testing Strategy

### Per-Task Manual Test Protocol
1. **Build verification:** `build-x64.bat` → zero warnings → DLL in `Far3_x64/Plugins/NetBox/`.
2. **Protocol smoke test:** Connect SFTP → FTP → WebDAV → S3 (one file each).
3. **Feature-specific test:** 
   - Perform a synchronization with a noticeable number of files (e.g., sync a directory with 50+ files).
   - Verify that the dialog shows:
     - Files scanned increasing.
     - Files transferred increasing (uploaded/downloaded/deleted).
     - Bytes transferred increasing.
     - ETA displayed and updating.
     - Transfer speed displayed.
   - Test with both local and remote synchronization directions.
   - Test with "Keep local directory up to date" mode to ensure background sync also updates the dialog when invoked manually.
4. **Regression test:** Synchronize, queue, custom command, editor, bookmarks.
5. **48-hour stress test:** No crashes during normal usage.

### Automated Testing (where feasible)
- Unit tests for progress calculation logic (if extracted).
- Validation that UI elements are updated correctly via mock controllers.

## Documentation Checkpoints

After completion:
- [ ] Update `docs/user-guide.md` with description of the enhanced synchronization progress dialog.
- [ ] Update `.hlf` help files (EN, RU, PL, FR, ES) with new dialog controls and their descriptions.
- [ ] Update `NetBoxEng.lng` / `NetBoxRus.lng` with new message strings (sync `MsgIDs.h`).
- [ ] Add CHANGELOG entry.

## Changelog

  - 2026-05-11: Initial plan created for synchronize progress dialog enhancement (Phase 4 Task 4.2).
  - 2026-05-11: Plan refined after deep codebase analysis:
    - Discovered existing `TerminalSynchronizeDirectory` progress message box (lines 1665-1711) to integrate
    - Identified `TFileOperationStatistics` (FileOperationProgress.h:83-103) as existing data structure for bytes/files
    - Found `TQueueDialog` pattern for `Idle()` + `PostMainThreadSynchro` periodic refresh
    - Updated implementation steps to leverage existing infrastructure instead of building from scratch
    - Added specific file paths and API references to all steps
    - Refined commit checkpoint with accurate change descriptions

## Post-Implementation Review Findings

### Fixed During Review

| Issue | Severity | Fix |
|-------|----------|-----|
| Destructor UAF via pending `PostMainThreadSynchro` | **BLOCKER** | Added `SynchroParams.Sender = nullptr` in `~TSynchronizeDialog()` (mirrors `TQueueDialog`) |
| Progress items in group 1 (hidden during sync) | **MEDIUM** | Added `SetDefaultGroup(0)` before progress section |

### Open Questions Requiring Approval

1. **Cancellation during `SynchronizeApply()` [HIGH]:**
   `TerminalSynchronizeDirectory()` normally shows a message box every 500ms that allows ESC to cancel the ongoing sync. The dialog is frozen during `SynchronizeApply()` (main thread blocks). By suppressing the message box, users lose mid-sync cancellation until the current file completes.
   - *Option A:* Keep as-is (accept limitation; Stop button works between files and during idle poller).
   - *Option B:* Do not suppress message box during `SynchronizeApply()`; only suppress during idle poller phase.
   - *Option C:* Add periodic `CheckForEsc()` inside `UpdateProgressDisplay()` when called from `DoLog()` during active sync.

2. **"Files scanned" label semantics [MEDIUM]:**
   `DoLog()` only receives `slScan` once (when recursive mode starts). The counter shows scan passes, not individual files scanned.
   - *Option A:* Keep label as "Files scanned:" (users may expect actual file count).
   - *Option B:* Rename label to "Scan passes:" or "Scans:" to match semantics.

### Known Limitations

- **ETA displays `-`** because `TFileOperationStatistics` does not expose total bytes-to-transfer. Linear extrapolation requires total work, which is only available if `SynchronizeCollect()` computes sizes. This can be enhanced in a future iteration.
- **Dialog height increased to 26 rows** — may clip on terminals with fewer rows. Far Manager typically scrolls dialogs, but verify on 25-row terminals.
- **Progress text visible before Start is clicked** — shows static labels with empty paths and `ETA: -`. Consider toggling visibility in `UpdateControls()`.

### Verification Results

| Check | Result |
|-------|--------|
| No TODO/FIXME/HACK | PASS |
| No trailing whitespace | PASS (script verified) |
| CRLF line endings | PASS (script verified) |
| MsgIDs + `.lng` alignment | PASS (`verify_lng_alignment.py`: 1327/1327) |
| Thread safety | PASS |
| No third-party APIs in plugin layer | PASS |
| Build / zero warnings | PENDING (requires VS2022 workstation) |