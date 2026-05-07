# Implementation Plan: Silent Mode UI and Refinements

Branch: feature/silent-mode-ui-and-refinements
Created: 2026-05-08

## Settings

- Testing: yes — update existing tests, add new test for error file output
- Logging: verbose — detailed DEBUG logs for all silent mode decisions and error file operations
- Docs: yes — mandatory docs checkpoint to update `docs/silent-mode.md`

## Roadmap Linkage

Milestone: "none"
Rationale: Silent mode is an internal feature not tracked on the roadmap

## Research Context

Source: `.ai-factory/RESEARCH.md` (Active Summary) + `.ai-factory/references/silent-mode-ui-and-refinements-exploration.md`

Goal: Complete the silent mode feature — add UI toggle, change overwrite behavior to boOlder, suppress all plugin-layer confirmation dialogs, write error reports to file instead of status line

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- WinXP compatibility (_WIN32_WINNT=0x0501)
- Incremental evolution — no architectural rewrites
- C++17 standard only (no std::filesystem, std::variant)

Decisions:
- A: Add SilentMode checkbox to Confirmations dialog, with other checkboxes disabled when silent is on
- B: Change EffectiveBatchOverwrite from boAll to boOlder (safer overwrite — only if source is newer)
- C: Add GetSilentMode() bypass to all 5 plugin-layer confirmation points in WinSCPFileSystem.cpp
- D: Write error reports to `.errors` file alongside session log; show summary via DoInformation instead of full report

Open questions: None — all four changes are fully specified from exploration

## Exploration Reference

[silent-mode-ui-and-refinements-exploration](../references/silent-mode-ui-and-refinements-exploration.md) — Complete audit of existing implementation, code locations, patterns, and edge case analysis

## Architecture Notes

Dependency flow for this plan:

```
Plugin Layer (WinSCPDialogs.cpp, WinSCPFileSystem.cpp)
    ↓ calls
Core Layer (Terminal.cpp, FileOperationProgress.cpp)
    ↓ uses
Base Layer (MsgIDs.h, Common.cpp for file I/O)
```

No third-party library modifications. No CMake changes. Plugin DLL output to `Far3_<platform>/Plugins/NetBox/`.

## Commit Plan

- **Commit 1** (after tasks 1-3): `feat(silent-mode): add SilentMode checkbox to Confirmations dialog`
- **Commit 2** (after task 4): `refactor(silent-mode): change overwrite mode from boAll to boOlder`
- **Commit 3** (after task 5): `feat(silent-mode): suppress all plugin-layer confirmation dialogs`
- **Commit 4** (after tasks 6-7): `feat(silent-mode): write error reports to .errors file`
- **Commit 5** (after task 8): `docs(silent-mode): update documentation for UI toggle and new behavior`

## Tasks

### Phase 1: UI Toggle

#### Task 1: Add NB_CONFIRMATIONS_SILENT_MODE message ID and localized strings [x]

**Files:**
- `src/base/MsgIDs.h` — add `NB_CONFIRMATIONS_SILENT_MODE` after `NB_COPY_PRESET_CUSTOM` (line 1446, before `};`)
- `src/NetBox/NetBoxEng.lng` — append `"Silent mode (suppress all confirmations)"` as last quoted string
- `src/NetBox/NetBoxRus.lng` — append `"Тихий режим (подавлять все подтверждения)"` as last quoted string
- `src/NetBox/NetBoxFr.lng` — append `"Mode silencieux (supprimer toutes les confirmations)"` as last quoted string
- `src/NetBox/NetBoxPol.lng` — append `"Tryb cichy (pomijaj wszystkie potwierdzenia)"` as last quoted string
- `src/NetBox/NetBoxSpa.lng` — append `"Modo silencioso (suprimir todas las confirmaciones)"` as last quoted string

**Implementation:**
- Add `NB_CONFIRMATIONS_SILENT_MODE,` after `NB_COPY_PRESET_CUSTOM` in the enum
- Append one quoted string line to each .lng file (same zero-based index across all files)
- Run `python scripts/verify_lng_alignment.py` to verify alignment

**LOGGING REQUIREMENTS:**
- No runtime logging needed — this is a static resource change

**Acceptance:**
- `verify_lng_alignment.py` exits with code 0
- All 5 .lng files have exactly 1309 quoted strings
- MsgIDs.h has exactly 1309 enum identifiers

---

#### Task 2: Add SilentMode checkbox to ConfirmationsConfigurationDialog [x]

**Files:**
- `src/NetBox/WinSCPDialogs.cpp` — `ConfirmationsConfigurationDialog()` (lines 937-990)

**Implementation:**
- Increase dialog height from `TPoint(67, 10)` to `TPoint(67, 12)` to accommodate the new checkbox
- Add `SilentModeCheck` checkbox after `ConfirmSynchronizedBrowsingCheck`:
  ```cpp
  TFarCheckBox * SilentModeCheck = MakeOwnedObject<TFarCheckBox>(Dialog);
  SilentModeCheck->SetCaption(GetMsg(NB_CONFIRMATIONS_SILENT_MODE));
  ```
- Load the current value:
  ```cpp
  SilentModeCheck->SetChecked(GetConfiguration()->GetSilentMode());
  ```

**LOGGING REQUIREMENTS:**
- No runtime logging needed — this is dialog construction

**Acceptance:**
- Dialog displays 5 checkboxes including the new SilentMode checkbox
- Checkbox reflects current `GetConfiguration()->GetSilentMode()` value

**Depends on:** Task 1

---

#### Task 3: Wire checkbox to configuration and disable other checkboxes when silent is on [x]

**Files:**
- `src/NetBox/WinSCPDialogs.cpp` — `ConfirmationsConfigurationDialog()` (lines 937-990)

**Implementation:**
- Set `EnabledDependencyNegative` on the 4 existing confirmation checkboxes so they are grayed out when SilentMode is checked:
  ```cpp
  ConfirmOverwritingCheck->SetEnabledDependencyNegative(SilentModeCheck);
  ConfirmCommandSessionCheck->SetEnabledDependencyNegative(SilentModeCheck);
  ConfirmResumeCheck->SetEnabledDependencyNegative(SilentModeCheck);
  ConfirmSynchronizedBrowsingCheck->SetEnabledDependencyNegative(SilentModeCheck);
  ```
- Save the value on OK (inside the `if (Result)` block, after `BeginUpdate()`):
  ```cpp
  GetConfiguration()->SetSilentMode(SilentModeCheck->GetChecked());
  ```
- Place this BEFORE the other checkbox saves so the disabled-state is consistent

**LOGGING REQUIREMENTS:**
- `DEBUG: ConfirmationsConfigurationDialog: SilentMode set to [true/false]` (level 2)

**Acceptance:**
- When SilentMode is checked, the other 4 checkboxes are grayed out
- When SilentMode is unchecked, the other 4 checkboxes are enabled normally
- Saving the dialog persists the SilentMode value to configuration
- Loading the dialog shows the saved SilentMode value

**Depends on:** Task 2

---

### Phase 2: Overwrite Mode Change

#### Task 4: Change EffectiveBatchOverwrite from boAll to boOlder for silent mode [x]

**Files:**
- `src/core/Terminal.cpp` — `EffectiveBatchOverwrite()` (line 3305)

**Implementation:**
- Change line 3305 from `return boAll;` to `return boOlder;`
- Update the comment from `// Silent mode: never prompt, always overwrite` to `// Silent mode: never prompt, overwrite only if source is newer`
- Update the log message (if any) accordingly

**Safety analysis (from exploration):**
- The early return at line 3305 bypasses the `!Special` downgrade at line 3336-3340, so `boOlder` will NOT be downgraded to `boNo`
- All protocols (SFTP, FTP, SCP, WebDAV, S3) provide `TRemoteFile::FModification` timestamps, so `boOlder` works correctly for all
- **Behavioral change note:** This changes existing behavior for any user who has manually enabled SilentMode via XML config. Their overwrite mode changes from "overwrite everything" to "overwrite only if newer." Commit message must note this.

**LOGGING REQUIREMENTS:**
- Update existing log: change `"Silent mode active: auto-overwrite enabled"` to `"Silent mode active: overwrite if newer (boOlder)"`
- `DEBUG: EffectiveBatchOverwrite: SilentMode=[true/false], returning boOlder` (level 2)

**Acceptance:**
- When SilentMode is true, `EffectiveBatchOverwrite()` returns `boOlder`
- When SilentMode is false, behavior is unchanged
- No downgrade of boOlder to boNo in the silent mode path

---

### Phase 3: Confirmation Suppression

#### Task 5: Add GetSilentMode() bypass to 7 confirmation points in WinSCPFileSystem.cpp [x]

**Files:**
- `src/NetBox/WinSCPFileSystem.cpp`

**Implementation:**

Add `GetConfiguration()->GetSilentMode()` as an additional OR condition at each confirmation point. Pattern: `GetConfiguration()->GetSilentMode() ||` inserted alongside the existing `(OpMode & OPM_SILENT)` check.

**Site 1 — Delete files confirmation (line ~2612):**
```cpp
// Before:
if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
    (MoreMessageDialog(Query, nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
// After:
if (GetConfiguration()->GetSilentMode() || (OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
    (MoreMessageDialog(Query, nullptr, qtConfirmation, qaOK | qaCancel) == qaOK))
```

**Site 2 — Delete sessions confirmation (line ~2621):**
```cpp
// Before:
if ((OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
    (MoreMessageDialog(GetMsg(NB_DELETE_SESSIONS_CONFIRM), ...)))
// After:
if (GetConfiguration()->GetSilentMode() || (OpMode & OPM_SILENT) || !GetFarConfiguration()->GetConfirmDeleting() ||
    (MoreMessageDialog(GetMsg(NB_DELETE_SESSIONS_CONFIRM), ...)))
```

**Site 3 — Synchronized browsing toggle (line ~2224):**
```cpp
// Before:
if (GetFarConfiguration()->GetConfirmSynchronizedBrowsing())
// After:
if (!GetConfiguration()->GetSilentMode() && GetFarConfiguration()->GetConfirmSynchronizedBrowsing())
```

**Site 4 — Create directory dialog (line ~2485):**
```cpp
// Before:
if ((OpMode & OPM_SILENT) ||
    CreateDirectoryDialog(AName, &Properties, SaveSettings))
// After:
if (GetConfiguration()->GetSilentMode() || (OpMode & OPM_SILENT) ||
    CreateDirectoryDialog(AName, &Properties, SaveSettings))
```

**Site 5 — Create session folder dialog (line ~2514):**
```cpp
// Before:
if (((OpMode & OPM_SILENT) ||
    GetWinSCPPlugin()->InputBox(...)))
// After:
if (GetConfiguration()->GetSilentMode() || (OpMode & OPM_SILENT) ||
    GetWinSCPPlugin()->InputBox(...))
```
**Site 6 — XML export overwrite confirmation (line ~2924):**
```cpp
// Before:
UnicodeString ConfirmMsg = FORMAT("File %s already exists. Overwrite?", XmlFileName);
if (MoreMessageDialog(ConfirmMsg, nullptr, qtConfirmation, qaYes | qaNo | qaCancel) != qaYes)
  return; // User cancelled
// After:
if (!GetConfiguration()->GetSilentMode() &&
    (MoreMessageDialog(ConfirmMsg, nullptr, qtConfirmation, qaYes | qaNo | qaCancel) != qaYes))
  return; // User cancelled
```

**Site 7 — Import sessions confirmation (line ~3180):**
```cpp
// Before:
if (MoreMessageDialog(ConfirmMsg, nullptr, qtConfirmation, qaYes | qaNo) != qaYes)
  return false;
// After:
if (!GetConfiguration()->GetSilentMode() &&
    (MoreMessageDialog(ConfirmMsg, nullptr, qtConfirmation, qaYes | qaNo) != qaYes))
  return false;
```

**LOGGING REQUIREMENTS:**
- At each site, add a log when SilentMode bypasses the dialog:
  - `DEBUG: Silent mode: bypassing delete files confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing delete sessions confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing synchronized browsing confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing create directory confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing create session folder confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing XML export overwrite confirmation` (level 2)
  - `DEBUG: Silent mode: bypassing import sessions confirmation` (level 2)
- Use `AppLogFmt` for consistency with existing plugin-layer logging

**Acceptance:**
- When SilentMode is true, all 7 confirmation dialogs are bypassed
- When SilentMode is false, all 7 confirmation dialogs behave as before
- `OPM_SILENT` (Far Manager's programmatic flag) still works independently

---

### Phase 4: Error Report File Output

#### Task 6: Implement error report file writing in Terminal.cpp [x]

**Files:**
- `src/core/Terminal.cpp` — error report sections at lines ~8020-8025 and ~8623-8628

**Implementation:**

Replace the current error report output at both sites:

```cpp
// Current:
if (FConfiguration->GetSilentMode() && OperationProgress.GetErrorLog().HasErrors())
{
  const UnicodeString Report = OperationProgress.GetErrorLog().GenerateReport();
  LogEvent(1, L"Silent mode error report:\n" + Report);
  DoInformation(Report, 0, L"");
}

// New:
if (FConfiguration->GetSilentMode() && OperationProgress.GetErrorLog().HasErrors())
{
  const UnicodeString Report = OperationProgress.GetErrorLog().GenerateReport();
  LogEvent(1, L"Silent mode error report:\n" + Report);

  // Write full report to .errors file
  UnicodeString LogFilePath = FConfiguration->GetLogFileName();
  if (LogFilePath.IsEmpty())
  {
    LogFilePath = FConfiguration->GetDefaultLogFileName();
  }
  UnicodeString ErrorFilePath = ChangeFileExt(LogFilePath, L".errors");

  FILE * ErrorFile = _wfsopen(ApiPath(ErrorFilePath).c_str(), L"w", SH_DENYWR);
  if (ErrorFile == nullptr)
  {
    // Retry with no sharing restrictions (same pattern as SessionInfo.cpp)
    ErrorFile = _wfsopen(ApiPath(ErrorFilePath).c_str(), L"w", SH_DENYNO);
  }
  if (ErrorFile != nullptr)
  {
    const UTF8String UtfReport = UTF8String(Report);
    fwrite(UtfReport.c_str(), 1, UtfReport.Length(), ErrorFile);
    fclose(ErrorFile);
    LogEvent(1, L"Silent mode error report written to: " + ErrorFilePath);
  }
  else
  {
    LogEvent(0, L"Silent mode: failed to write error report to: " + ErrorFilePath);
  }

  // Show summary in status line instead of full report
  const UnicodeString Summary = FORMAT(L"%d errors - see %s",
    static_cast<int32_t>(OperationProgress.GetErrorLog().GetErrorCount()),
    ErrorFilePath);
  DoInformation(Summary, 0, L"");
}
```

**Helper function:** `ChangeFileExt()` already exists in `Sysutils.hpp:377` — no new code needed. Same pattern used in `Common.cpp:4162` and `Common.cpp:4460`. Use `ChangeFileExt(LogFilePath, L".errors")`.

**File I/O pattern:** Follow `SessionInfo.cpp:828-834` — try `_wfsopen` with `SH_DENYWR` first, retry with `SH_DENYNO` on failure. Use `ApiPath()` wrapper on the file path.

**Path resolution:**
- `FConfiguration->GetLogFileName()` returns the expanded session log path (with placeholders resolved)
- If empty (no logging configured), use `FConfiguration->GetDefaultLogFileName()` which returns `%TEMP%\<session>.log`
- Replace extension: `.log` → `.errors`, `.xml` → `.errors`
- Write using `_wfsopen` with `ApiPath()` wrapper and `SH_DENYWR` / `SH_DENYNO` retry (same pattern as `SessionInfo.cpp:828-834`)
- Convert `UnicodeString` to `UTF8String` via `UTF8String(Report)` constructor for `fwrite` (UTF-8 encoding for cross-platform readability)

**LOGGING REQUIREMENTS:**
- `DEBUG: Silent mode: error report written to: <path>` (level 1)
- `WARN: Silent mode: failed to write error report to: <path>` (level 0, on file open failure)
- `DEBUG: Silent mode: <N> errors, summary shown in status` (level 2)

**Acceptance:**
- Error report is written to `.errors` file alongside session log
- Status line shows summary: `"<N> errors - see <path>"`
- If file open fails, warning is logged and operation continues
- Existing `LogEvent(1, ...)` with full report is preserved
- Both `CopyToLocal` and `CopyToRemote` error report sites are updated

---

#### Task 7: Add test for error report file output [x]

**Files:**
- `tests/integration/test_silent_mode.cpp` — add test section

**Implementation:**
- Add a test that creates a `TFileOperationErrorLog`, populates it with errors, calls `GenerateReport()`, and verifies the report content
- Add a test that simulates the file-writing logic: derive `.errors` path from a log path, write the report, read it back, verify content matches

**LOGGING REQUIREMENTS:**
- `DEBUG: Test: error report file written and verified` (level 2)

**Acceptance:**
- Test passes: report is written to correct path
- Test passes: file content matches `GenerateReport()` output
- Test passes: `ChangeFileExt` correctly transforms `.log` and `.xml` extensions

**Depends on:** Task 6

---

### Phase 5: Documentation

#### Task 8: Update docs/silent-mode.md with new behavior [x]

**Files:**
- `docs/silent-mode.md`

**Implementation:**
Update the documentation to reflect:
1. **UI toggle:** Silent mode can now be enabled via Preferences → Confirmations → "Silent mode" checkbox
2. **Overwrite mode:** Silent mode now uses "overwrite if newer" (boOlder) instead of "overwrite all" — only files with newer timestamps are overwritten
3. **Confirmation suppression:** Silent mode now suppresses ALL confirmation dialogs, including delete, create directory, synchronized browsing, and create session folder
4. **Error report file:** Error reports are written to a `.errors` file alongside the session log file, with a summary shown in the status line
5. **Behavioral change note:** If you previously enabled SilentMode via XML config, the overwrite mode has changed from "overwrite all" to "overwrite if newer"

**Acceptance:**
- Documentation accurately describes all four changes
- No mention of `boAll` or "always overwrites" — replaced with `boOlder` semantics
- `.errors` file path derivation is documented
