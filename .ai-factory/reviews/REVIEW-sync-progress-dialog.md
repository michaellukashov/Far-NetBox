# Code Review: Synchronize Progress Dialog Enhancement

**Review Date:** 2026-05-11  
**Plan:** synchronize-progress-dialog-enhancement.md  
**Scope:** `TSynchronizeDialog` real-time statistics (files scanned, transferred, bytes, speed, ETA)

---

## Severity Legend

| Severity | Meaning |
|----------|---------|
| **BLOCKER** | Must fix before merge — crash, data loss, or major regression |
| **HIGH** | Should fix before merge — correctness, thread safety, or significant UX regression |
| **MEDIUM** | Fix if time permits — style, maintainability, or minor UX issue |
| **LOW** | Optional — documentation, nitpick, or future improvement |

---

## Findings

### 1. Missing `SynchroParams.Sender` cleanup in destructor [BLOCKER] → ✅ FIXED

**File:** `src/NetBox/WinSCPDialogs.cpp`  
**Line:** `TSynchronizeDialog::~TSynchronizeDialog()`

**Issue:** `Idle()` posts `OnIdle()` via `PostMainThreadSynchro`. If the dialog is destroyed while a pending synchro event exists, `OnIdle()` could be called on a dangling `this` pointer. `TQueueDialog` mitigates this by setting `SynchroParams.Sender = nullptr` in its destructor.

**Fix Applied:**
```cpp
TSynchronizeDialog::~TSynchronizeDialog() noexcept
{
    if (GetFarPlugin())
    {
        TSynchroParams & SynchroParams = GetFarPlugin()->GetSynchroParams();
        SynchroParams.Sender = nullptr;
    }
    SAFE_DESTROY(FSynchronizeOptions);
}
```

**Rationale:** `ProcessSynchroEvent()` checks `SynchroParams->Sender` before invoking `SynchroParams->SynchroEvent()`. Setting `Sender = nullptr` prevents the callback from executing on a destroyed object.
---

### 2. Suppressed message box removes cancellation path during `SynchronizeApply()` [HIGH] -> ✅ FIXED

**File:** `src/NetBox/WinSCPFileSystem.cpp`
**Line:** `TerminalSynchronizeDirectory()` (line ~1684)

**Issue:** `TerminalSynchronizeDirectory()` normally shows a message box every 500ms that allows the user to press ESC and cancel the ongoing sync. The dialog itself is frozen during `SynchronizeApply()` (blocking main thread). By suppressing the message box with `if (FInSynchronizeDialog) return;`, the user loses the only cancellation mechanism available during long `SynchronizeApply()` operations.

**Fix Applied:** Restructured the suppression so the `CheckForEsc()` cancellation path is preserved even when the synchronize dialog is active. The message box display is suppressed, but the ESC check runs unconditionally.

**Impact:** Users can press **ESC** during a long `SynchronizeApply()` and get the confirmation dialog, same as before. The progress display moves to the dialog; the cancellation behavior is unchanged.

### 3. `FFilesScanned` counter is misleading [MEDIUM] -> ✅ FIXED

**File:** `src/NetBox/WinSCPDialogs.cpp`  
**Line:** `DoLog()` switch on `slScan`

**Issue:** `DoLog()` increments `FFilesScanned` only for `slScan` events. The controller emits `slScan` exactly once (in `StartStop()`) when `soRecurse` is enabled. It does NOT emit per-file scan events during `SynchronizeCollect()`. Therefore the "Files scanned" field shows `1` (or `0` for non-recursive mode), not the actual number of files scanned.

**Fix Applied:** Renamed enum and label from `NB_SYNCHRONIZE_PROGRESS_FILES_SCANNED` / `"Files scanned:"` to `NB_SYNCHRONIZE_PROGRESS_SCAN_PASSES` / `"Scan passes:"`. This accurately reflects that the counter tracks scan passes (typically 1 for recursive mode), not individual files scanned.

**Files changed:** `MsgIDs.h`, `WinSCPDialogs.cpp` (2 references), all 5 `.lng` files.

### 4. Dialog height increase may clip on small terminals [MEDIUM]

**File:** `src/NetBox/WinSCPDialogs.cpp`  
**Line:** `SetSize(TPoint(76, 26))`

**Issue:** Height increased from 20 to 26. On terminals with fewer than 26 rows, the dialog may be clipped. Far Manager typically handles this by scrolling, but it may push buttons off-screen.

**Recommendation:** Verify the layout on a 25-row terminal (common minimum). If buttons are clipped, reduce progress rows by collapsing pairs (e.g., Speed + ETA on one line) or reducing lister height.

---

### 5. Progress text visible before sync starts [MEDIUM] -> ✅ FIXED

**File:** `src/NetBox/WinSCPDialogs.cpp`

**Issue:** The 9 new progress text items are always visible, even before the user clicks Start. They display static text like "Local:        " (empty path) and "ETA: " (`-`). This looks unpolished.

**Fix Applied:** Added visibility toggling in `UpdateControls()` so all progress text items are hidden when `FSynchronizing` is false and shown when sync is active.

```cpp
const bool ShowProgress = FSynchronizing;
if (ProgressLocalText) ProgressLocalText->SetVisible(ShowProgress);
if (ProgressRemoteText) ProgressRemoteText->SetVisible(ShowProgress);
// ... etc for all 9 items
```

### 6. `FSyncStatistics` raw pointer lifetime [LOW — handled correctly]

**File:** `src/NetBox/WinSCPFileSystem.cpp`  
**Line:** `Synchronize()`

**Observation:** `FSyncStatistics = &Statistics;` points to a stack variable. It is nulled in `__finally`. `UpdateProgressDisplay()` reads it and handles `nullptr` gracefully. All accesses are on the main thread. **No fix required** — pattern is safe as implemented.

---

### 7. `Idle()` rate limiting on `UpdateProgressDisplay()` [LOW]

**File:** `src/NetBox/WinSCPDialogs.cpp`  
**Line:** `UpdateProgressDisplay()`

**Observation:** Both `DoLog()`-driven and `Idle()`-driven updates share the same 500ms throttle. If a burst of `DoLog()` events arrives within 500ms, only the first updates the UI. The counters in `DoLog()` still increment, but the UI lags. This is an acceptable trade-off for performance.

---

### 8. `FORMAT(L"%d", ...)` style inconsistency [LOW]

**File:** `src/NetBox/WinSCPDialogs.cpp`  
**Line:** `UpdateProgressDisplay()`

**Observation:** Existing codebase uses narrow string literals in `FORMAT()` (e.g., `FORMAT("%d%%", ...)`). Our code uses `FORMAT(L"%d", ...)`. Both compile, but narrow literals are preferred for consistency.

---

## Verification Results

| Check | Result |
|-------|--------|
| No TODO/FIXME/HACK introduced | ✅ PASS |
| No trailing whitespace | ✅ PASS (script verified) |
| CRLF line endings preserved | ✅ PASS (script verified) |
| MsgIDs + `.lng` alignment | ✅ PASS (`verify_lng_alignment.py`: 1327/1327) |
| Thread safety (main-thread UI updates) | ✅ PASS |
| No third-party API calls in plugin layer | ✅ PASS |
| Pointer lifetime (`FSyncStatistics`) | ✅ PASS |
| Build / zero warnings | ⏳ PENDING (requires VS2022) |

## Correct Integration Pattern (for future reference)

```cpp
// 1. Dialog constructor accepts TWinSCPFileSystem* for stats access
// 2. DoLog() increments counters + triggers UpdateProgressDisplay()
// 3. Idle() + PostMainThreadSynchro for periodic refresh (TQueueDialog pattern)
// 4. Destructor MUST clear SynchroParams.Sender to prevent UAF
// 5. FSyncStatistics is stack-allocated in Synchronize(); nulled in __finally
// 6. FInSynchronizeDialog suppresses TerminalSynchronizeDirectory message box
```

## Anti-Patterns to Avoid

1. **Do NOT call `PostMainThreadSynchro` from worker threads without checking `Sender` in destructor.** Always mirror `TQueueDialog::~TQueueDialog()`.
2. **Do NOT suppress `TerminalSynchronizeDirectory` without providing an alternative cancellation path.** The message box was the only mid-sync cancel mechanism.
3. **Do NOT place new UI items in group 1** — `UpdateControls()` disables group 1 during sync. Progress items must be in group 0.
4. **Do NOT call `SetCaption()` from worker threads.** All UI updates in this patch are marshaled to the main thread.

## Known Bugs and Solutions

| Bug | File | Fix |
|-----|------|-----|
| Destructor UAF via pending synchro | `WinSCPDialogs.cpp` | Added `SynchroParams.Sender = nullptr` in destructor |
| Progress items hidden in group 1 | `WinSCPDialogs.cpp` | Added `SetDefaultGroup(0)` before progress section |

## Changelog (Review-Driven Fixes)

- 2026-05-11: Added `SynchroParams.Sender = nullptr` in `TSynchronizeDialog` destructor (BLOCKER fix).
- 2026-05-11: Verified `SetDefaultGroup(0)` placement for progress items (MEDIUM fix — already applied during implementation).
