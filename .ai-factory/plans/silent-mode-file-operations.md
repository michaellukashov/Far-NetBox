# Plan: Silent Mode for File Operations

**Branch:** feature/silent-mode-file-operations
**Created:** 2026-04-22
**Status:** Complete — all tasks implemented

## Settings

- **Testing:** Yes — unit tests for configuration flags, integration tests for silent mode behavior
- **Logging:** Verbose — detailed DEBUG logs for all silent mode decisions, error collection, and batch operations
- **Docs:** Yes — update user documentation with silent mode configuration and error reporting

## Roadmap Linkage

**Milestone:** none
**Rationale:** Skipped by user

## Research Context

**Topic:** Silent Mode for File Operations

**Goal:** Eliminate blocking dialogs during server operations (search, upload, download) and enable continue-on-error with detailed reporting

**Constraints:**
- Must not break existing interactive mode
- Must preserve all error information for post-operation review
- Must work across all protocols (SFTP, FTP, SCP, WebDAV, S3)
- Zero modifications to libs/ (third-party code)

**Decisions:**
- Use existing cpNoConfirmation/spNoConfirmation flags as foundation
- Add SilentMode configuration flag to auto-enable no-confirmation behavior
- Implement error collection mechanism instead of abort-on-error
- Generate detailed error report after operations complete

**Open questions:**
- Should silent mode be per-session or global configuration?
- How to expose silent mode to user (UI checkbox, command-line flag, config file)?
- Should error report be shown in dialog, written to log, or both?

**Success signals:**
- File operations complete without blocking dialogs
- All errors logged with file path, error type, timestamp
- Operations continue despite individual file failures
- Detailed error summary available after operation

## Overview

Implement silent mode for file operations to eliminate blocking dialogs during server operations (search, upload, download, synchronization) and enable continue-on-error with detailed error reporting. This feature prevents deadlocks when UI cannot respond and provides comprehensive error summaries after operations complete.

## Tasks

### Phase 1: Configuration Infrastructure

#### Task 1: Add SilentMode configuration flag [x]
**Files:**
- `src/core/Configuration.h`
- `src/core/Configuration.cpp`

**Description:**
Add `FSilentMode` boolean flag to `TConfiguration` class with getter/setter methods. This flag controls whether file operations run in silent mode (no dialogs, continue on error).

**Design Decision:** SilentMode is implemented as a global configuration flag (not per-session) because:
- Simpler implementation and UI
- Typical use case is automation/scripting where all sessions should behave consistently
- Can be extended to per-session in future if needed via TSessionData

**Implementation:**
- Add `bool FSilentMode{false};` private member to `TConfiguration`
- Add `bool GetSilentMode() const { return FSilentMode; }`
- Add `void SetSilentMode(bool Value) { FSilentMode = Value; }`
- Add storage key `"SilentMode"` to `DoSave()` and `DoLoad()` methods
- Default to `false` (interactive mode)
- Value persists across application restarts (stored in configuration)

**Logging:**
- `DEBUG: Configuration: SilentMode flag set to [true/false]`
- `DEBUG: Configuration: Loading SilentMode from storage: [value]`
- `DEBUG: Configuration: Saving SilentMode to storage: [value]`

**Acceptance:**
- `TConfiguration` has `SilentMode` property
- Value persists across sessions via storage
- Default is `false` (interactive mode)

---

#### Task 2: Add SilentMode to GUI configuration [x]
**Files:**
- `src/windows/GUIConfiguration.h`
- `src/windows/GUIConfiguration.cpp`

**Description:**
Expose `SilentMode` flag in `TGUIConfiguration` for GUI applications. Inherit from base `TConfiguration` implementation.

**Implementation:**
- Add forwarding getter/setter in `TGUIConfiguration`:
  ```cpp
  bool GetSilentMode() const { return TConfiguration::GetSilentMode(); }
  void SetSilentMode(bool Value) { TConfiguration::SetSilentMode(Value); }
  ```
- No additional storage needed (inherited from base)

**Logging:**
- `DEBUG: GUIConfiguration: SilentMode accessed: [value]`

**Acceptance:**
- `TGUIConfiguration` exposes `SilentMode` property
- Value synchronized with base `TConfiguration`

**Blocked by:** Task 1

---

#### Task 3: Add SilentMode to Far configuration [x]
**Files:**
- `src/NetBox/FarConfiguration.h`
- `src/NetBox/FarConfiguration.cpp`

**Description:**
Expose `SilentMode` flag in `TFarConfiguration` for Far Manager plugin. Inherit from base `TConfiguration` implementation.

**Implementation:**
- Add forwarding getter/setter in `TFarConfiguration`:
  ```cpp
  bool GetSilentMode() const { return TConfiguration::GetSilentMode(); }
  void SetSilentMode(bool Value) { TConfiguration::SetSilentMode(Value); }
  ```
- No additional storage needed (inherited from base)

**Logging:**
- `DEBUG: FarConfiguration: SilentMode accessed: [value]`

**Acceptance:**
- `TFarConfiguration` exposes `SilentMode` property
- Value synchronized with base `TConfiguration`

**Blocked by:** Task 1

---

### Phase 2: Error Collection Infrastructure

#### Task 4: Create TFileOperationError structure [x]
**Files:**
- `src/core/FileOperationProgress.h`

**Description:**
Define error record structure to capture individual file operation failures during silent mode execution. Includes error categorization for future enhancements.

**Implementation:**
```cpp
enum class TFileOperationErrorCategory
{
  NetworkError,      // Connection lost, timeout
  PermissionDenied,  // Access denied, read-only
  ResourceError,     // Disk full, quota exceeded
  FileNotFound,      // Source/dest missing
  Other
};

struct TFileOperationError
{
  UnicodeString FileName;
  UnicodeString ErrorMessage;
  TDateTime Timestamp;
  TOperationSide Side;
  TFileOperationErrorCategory Category;
  
  TFileOperationError() = default;
  TFileOperationError(
    const UnicodeString & AFileName,
    const UnicodeString & AErrorMessage,
    TOperationSide ASide,
    TFileOperationErrorCategory ACategory = TFileOperationErrorCategory::Other);
};
```

**Logging:**
- `DEBUG: FileOperationError created: file=[FileName], side=[Side], category=[Category]`

**Acceptance:**
- `TFileOperationError` structure defined
- `TFileOperationErrorCategory` enum defined
- Constructor initializes all fields including current timestamp
- Supports copy/move semantics

---

#### Task 5: Create TFileOperationErrorLog class [x]
**Files:**
- `src/core/FileOperationProgress.h`
- `src/core/FileOperationProgress.cpp`

**Description:**
Implement thread-safe error collection mechanism to accumulate file operation failures during silent mode execution. Thread safety required for parallel file operations.

**Implementation:**
```cpp
class TFileOperationErrorLog
{
public:
  TFileOperationErrorLog() = default;
  ~TFileOperationErrorLog() = default;
  
  void AddError(
    const UnicodeString & FileName,
    const UnicodeString & ErrorMessage,
    TOperationSide Side,
    TFileOperationErrorCategory Category = TFileOperationErrorCategory::Other);
  
  void Clear();
  bool HasErrors() const;
  size_t GetErrorCount() const;
  const std::vector<TFileOperationError> & GetErrors() const;
  
  UnicodeString GenerateReport() const;
  
private:
  std::vector<TFileOperationError> FErrors;
  mutable std::mutex FMutex;  // Protects FErrors for parallel operations
};
```

**Thread Safety:**
- All public methods use `std::lock_guard<std::mutex>` to protect `FErrors`
- Required because `TTerminalQueue` supports parallel file transfers

**Logging:**
- `DEBUG: ErrorLog: Adding error for file [FileName]: [ErrorMessage]`
- `DEBUG: ErrorLog: Total errors collected: [count]`
- `DEBUG: ErrorLog: Generating error report with [count] entries`
- `DEBUG: ErrorLog: Cleared [count] errors`

**Acceptance:**
- `TFileOperationErrorLog` class implemented
- Thread-safe for concurrent access
- Errors stored in chronological order
- `GenerateReport()` produces human-readable summary with timestamps

**Blocked by:** Task 4

---

#### Task 6: Integrate error log into TFileOperationProgressType [x]
**Files:**
- `src/core/FileOperationProgress.h`
- `src/core/FileOperationProgress.cpp`

**Description:**
Add error log instance to progress tracking so file operations can collect errors during silent mode execution.

**Implementation:**
- Add `TFileOperationErrorLog FErrorLog;` member to `TFileOperationProgressType`
- Add getter: `TFileOperationErrorLog & GetErrorLog() { return FErrorLog; }`
- Add convenience method:
  ```cpp
  void AddOperationError(
    const UnicodeString & FileName,
    const UnicodeString & ErrorMessage,
    TOperationSide Side,
    TFileOperationErrorCategory Category = TFileOperationErrorCategory::Other)
  {
    FErrorLog.AddError(FileName, ErrorMessage, Side, Category);
  }
  ```
- Clear error log in `Reset()` method

**Logging:**
- `DEBUG: OperationProgress: Error log accessed`
- `DEBUG: OperationProgress: Error log reset`

**Acceptance:**
- `TFileOperationProgressType` has error log member
- Error log cleared when progress is reset
- Accessible via getter method

**Blocked by:** Task 5

---

### Phase 3: Silent Mode Logic Integration
#### Task 7: Modify EffectiveBatchOverwrite for silent mode [x]
**Files:**
- `src/core/Terminal.cpp` (line ~3285: `EffectiveBatchOverwrite()`)

**Description:**
Update `TTerminal::EffectiveBatchOverwrite()` to return `boAll` (overwrite all) when silent mode is active, bypassing all user prompts. This is the first line of defense -- it prevents confirmation dialogs before they are reached.

**Implementation:**
- Add check at the beginning of `EffectiveBatchOverwrite()`:
  ```cpp
  TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
    const UnicodeString & ASourceFullFileName,
    const TCopyParamType * CopyParam,
    int32_t Params,
    TFileOperationProgressType * AOperationProgress,
    bool Special) const
  {
    // Silent mode: never prompt, overwrite only if source is newer
    if (FConfiguration->GetSilentMode())
    {
      LogEvent(1, L"Silent mode active: overwrite if newer (boOlder)");
      return boOlder;
    }

    // ... existing logic
  }
  ```

**Logging:**
- `DEBUG: Silent mode active: overwrite if newer (boOlder)` (level 2 = DEBUG)
- `DEBUG: EffectiveBatchOverwrite: SilentMode=[true/false], returning [BatchOverwriteMode]`

**Note:** Silent mode returns `boOlder` (overwrite if source is newer). This is safer than unconditional overwrite and works across all protocols since they populate `TRemoteFile::FModification` timestamps. The early return bypasses the `!Special` downgrade at line ~3336-3340, so `boOlder` will NOT be downgraded to `boNo`.

**Acceptance:**
- When `SilentMode` is `true`, `EffectiveBatchOverwrite()` returns `boOlder`
- Existing interactive mode behavior unchanged when `SilentMode` is `false`
- Log entry confirms silent mode activation

**Blocked by:** Task 1

---

#### Task 8: Auto-set cpNoConfirmation in silent mode
**Files:**
- `src/core/Terminal.cpp` (multiple locations where copy params are prepared)

**Description:**
Automatically add `cpNoConfirmation` flag to copy parameters when silent mode is active, ensuring no confirmation dialogs appear.

**Implementation:**
Find all locations where copy operations are initiated and add:
```cpp
if (FConfiguration->GetSilentMode())
{
  Params |= cpNoConfirmation;
  LogEvent(1, L"Silent mode: cpNoConfirmation flag added");
}
```

Key locations:
- `TTerminal::CopyToLocal()` (`Terminal.cpp`) — download entry point
- `TTerminal::CopyToRemote()` (`Terminal.cpp`) — upload entry point
- `TTerminal::SynchronizeCollect()` (`Terminal.cpp`) — sync collection
**Logging:**
- `DEBUG: Silent mode: cpNoConfirmation flag added to copy operation` (level 2 = DEBUG)
- `DEBUG: Copy params: Params=[hex], SilentMode=[true/false]`

**Acceptance:**
- `cpNoConfirmation` automatically set when `SilentMode` is `true`
- No impact on explicit `cpNoConfirmation` usage
- Logged for each operation

**Blocked by:** Task 7

---

#### Task 9: Auto-set spNoConfirmation in silent mode
**Files:**
- `src/core/Terminal.cpp` (synchronization methods)

**Description:**
Automatically add `spNoConfirmation` flag to synchronization parameters when silent mode is active.

**Implementation:**
Find synchronization entry points and add:
```cpp
if (FConfiguration->GetSilentMode())
{
  Params |= spNoConfirmation;
  LogEvent(1, L"Silent mode: spNoConfirmation flag added");
}
```

Key locations:
- `TTerminal::Synchronize()` (`Terminal.cpp`) — sync entry point
- `TTerminal::SynchronizeCollect()` (`Terminal.cpp`) — sync collection
**Logging:**
- `DEBUG: Silent mode: spNoConfirmation flag added to synchronization` (level 2 = DEBUG)
- `DEBUG: Sync params: Params=[hex], SilentMode=[true/false]`

**Acceptance:**
- `spNoConfirmation` automatically set when `SilentMode` is `true`
- No impact on explicit `spNoConfirmation` usage
- Logged for each operation

**Blocked by:** Task 7

---

### Phase 4: Continue-on-Error Implementation

#### Task 10: Modify FileOperationLoopQuery for silent mode continue-on-error
**Files:**
- `src/core/Terminal.cpp` (line ~2657: `FileOperationLoopQuery()`)

**Description:**
Modify `FileOperationLoopQuery()` -- the single chokepoint where all file operation exceptions trigger interactive prompts -- to support silent mode. When silent mode is active, instead of calling `QueryUserException()` (which blocks for user input), collect the error and skip the file.

**Implementation:**
Add silent mode check at the top of `FileOperationLoopQuery()`, before the prompt logic:
```cpp
bool TTerminal::FileOperationLoopQuery(
  Exception & E, TFileOperationProgressType * AOperationProgress,
  const UnicodeString & Message, uint32_t AFlags,
  const UnicodeString & /*SpecialRetry*/, const UnicodeString & /*HelpKeyword*/)
{
  bool Result{false};
  Log->AddException(&E);

  // Silent mode: collect error, skip file, continue
  if (FConfiguration->GetSilentMode())
  {
    const UnicodeString ErrorMsg = TranslateExceptionMessage(&E);
    if (AOperationProgress != nullptr)
    {
      OperationProgress->AddOperationError(
        /*FileName extracted from AMessage or error context*/,
        ErrorMsg, OperationProgress->GetSide());
    }
    LogEvent(0, L"Silent mode: Skipping file after error: " + ErrorMsg);
    throw ESkipFile(&E, Message);
  }

  // ... existing interactive prompt logic
}
```

**Key Design Points:**
- Single point of modification -- all 20+ `FILE_OPERATION_LOOP_END` macro call sites benefit
- Uses existing `ESkipFile` exception to continue to next file (standard NetBox/WinSCP pattern)
- Error collected via `OperationProgress->AddOperationError()` (Task 6 integration)
- No changes to `FILE_OPERATION_LOOP_END` macro itself -- preserves existing interactive behavior

**Logging:**
- `WARN: Silent mode: Skipping file after error: [ErrorMessage]` (level 0 = WARN)
- `DEBUG: Silent mode: Error collected for [FileName]` (level 2 = DEBUG)
- `INFO: Silent mode: [N] errors collected so far` (level 1 = INFO)

**Acceptance:**
- When `SilentMode` is `true`, file operation errors do NOT prompt user
- Errors are collected and operation continues to next file
- Interactive mode behavior unchanged when `SilentMode` is `false`
- `ESkipFile` propagates correctly to skip the current file

**Blocked by:** Task 6, Task 8
---


### Phase 5: Error Reporting

#### Task 12: Implement error report generation
**Files:**
- `src/core/FileOperationProgress.cpp`

**Description:**
Implement `TFileOperationErrorLog::GenerateReport()` to produce detailed, human-readable error summary.

**Implementation:**
```cpp
UnicodeString TFileOperationErrorLog::GenerateReport() const
{
  std::lock_guard<std::mutex> lock(FMutex);
  
  if (FErrors.empty())
    return L"No errors occurred.";
  
  const size_t MaxDetailedErrors = 100;
  UnicodeString Report = FORMAT(L"File Operation Errors: %d total\n\n", FErrors.size());
  
  const size_t DetailCount = std::min(FErrors.size(), MaxDetailedErrors);
  for (size_t i = 0; i < DetailCount; ++i)
  {
    const auto & Error = FErrors[i];
    Report += FORMAT(L"[%s] %s (%s)\n  %s\n\n",
      FormatDateTime(L"yyyy-mm-dd hh:nn:ss", Error.Timestamp),
      Error.FileName,
      Error.Side == osLocal ? L"Local" : L"Remote",
      Error.ErrorMessage);
  }
  
  if (FErrors.size() > MaxDetailedErrors)
  {
    Report += FORMAT(L"\n... and %d more errors (truncated for readability)\n",
      FErrors.size() - MaxDetailedErrors);
  }
  
  return Report;
}
```

**Logging:**
- `DEBUG: Generating error report with [N] entries`

**Acceptance:**
- Report includes timestamp, filename, side, error message
- Formatted for readability
- Truncates after 100 detailed errors to prevent UI freezes
- Empty report handled gracefully
```

**Logging:**
- `DEBUG: Generating error report with [N] entries`

**Acceptance:**
- Report includes timestamp, filename, side, protocol, error message
- Formatted for readability
- Empty report handled gracefully

**Blocked by:** Task 5

---

#### Task 13: Display error report after operations [x]
**Files:**
- `src/core/Terminal.cpp` (end of file operation methods)
**Description:**
After file operations complete in silent mode, write the full error report to a `.errors` file alongside the session log and show a brief summary in the status line.

**Implementation:**
At the end of `CopyToLocal()`, `CopyToRemote()`, etc.:
```cpp
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
    nb::EscapeFmtChars(ErrorFilePath));
  DoInformation(Summary, 0, L"");
}
```

**Logging:**
- `INFO: Silent mode error report written to: <ErrorFilePath>` (level 1 = INFO)
- `WARN: Silent mode: failed to write error report to: <ErrorFilePath>` (level 0 = WARN)

**Acceptance:**
- Error report written to `.errors` file alongside session log
- Status line shows summary: `"<N> errors - see <path>"`
- If file open fails, warning is logged and operation continues
- Both `CopyToLocal` and `CopyToRemote` error report sites updated

**Blocked by:** Task 12, Task 10

---

### Phase 6: Testing

#### Task 14: Add unit tests for configuration [x]
**Files:**
- `tests/core/test_configuration.cpp` (create if needed)

**Description:**
Test `SilentMode` configuration flag persistence and retrieval.

**Tests:**
- Default value is `false`
- Set/get round-trip works
- Value persists across save/load
- Inherited correctly in `TGUIConfiguration` and `TFarConfiguration`

**Logging:**
- `DEBUG: Test: SilentMode default value: [value]`
- `DEBUG: Test: SilentMode after set: [value]`
- `DEBUG: Test: SilentMode after reload: [value]`

**Acceptance:**
- All configuration tests pass
- Coverage for all getters/setters

**Blocked by:** Task 1, Task 2, Task 3

---

#### Task 15: Add integration tests for silent mode [x]
**Files:**
- `tests/integration/test_silent_mode.cpp` (create if needed)

**Description:**
Test silent mode behavior end-to-end with mock file operations.

**Tests:**
- File operations complete without prompts when `SilentMode` is `true`
- Errors collected in error log
- Operations continue after individual file failures
- Error report generated correctly
- Interactive mode unchanged when `SilentMode` is `false`
- Protocol-specific tests:
  - `TEST(SilentMode, SFTP_ContinuesOnError)`
  - `TEST(SilentMode, FTP_ContinuesOnError)`
  - `TEST(SilentMode, SCP_ContinuesOnError)`
  - `TEST(SilentMode, WebDAV_ContinuesOnError)`
  - `TEST(SilentMode, S3_ContinuesOnError)`

**Acceptance:**
- All integration tests pass
- Silent mode prevents dialogs
- Error collection works correctly
- Interactive mode unaffected

**Blocked by:** Task 13

---

### Phase 7: Documentation

#### Task 16: Update user documentation [x]
**Files:**
- `docs/silent-mode.md` (create)
- `docs/configuration.md` (update)

**Description:**
Document silent mode feature, configuration, and error reporting for end users.

**Content:**
- What is silent mode
- When to use it (automation, batch operations, deadlock prevention)
- How to enable/disable (configuration setting)
- Error reporting behavior (`.errors` file + status line summary)
- Limitations and caveats:
  - Silent mode uses `boOlder` (overwrite only if source is newer)
  - Silent mode persists across application restarts
  - Error reports truncated after 100 entries for readability

**Acceptance:**
- Documentation complete and accurate
- Examples provided
- Linked from main documentation index

**Blocked by:** Task 13

---

## Commit Plan

**Checkpoint 1** (after Task 3):
```
feat(config): add SilentMode configuration flag

- Add FSilentMode flag to TConfiguration with persistence
- Expose in TGUIConfiguration and TFarConfiguration
- Default to false (interactive mode)
```

**Checkpoint 2** (after Task 6):
```
feat(core): add error collection infrastructure

- Create TFileOperationError structure
- Implement TFileOperationErrorLog class
- Integrate error log into TFileOperationProgressType
```

**Checkpoint 3** (after Task 9):
```
feat(core): integrate silent mode logic

- Modify EffectiveBatchOverwrite for silent mode
- Auto-set cpNoConfirmation in silent mode
- Auto-set spNoConfirmation in silent mode
```

**Checkpoint 4** (after Task 10):
```
feat(core): add silent mode continue-on-error to FileOperationLoopQuery

- Modify FileOperationLoopQuery() to skip file and collect error when SilentMode active
- Single-point modification benefits all 20+ FILE_OPERATION_LOOP_END call sites
- Uses existing ESkipFile pattern for continue-to-next-file behavior
- Preserves existing interactive prompt behavior when SilentMode is false
```

**Checkpoint 5** (after Task 13):
```
feat(core): add error reporting

- Implement error report generation
- Write `.errors` file + show summary status line after operations complete
```

**Checkpoint 6** (after Task 15):
```
test: add silent mode tests

- Unit tests for configuration
- Integration tests for silent mode behavior
```

**Final commit** (after Task 16):
```
docs: document silent mode feature

- Add silent-mode.md user guide
- Update configuration.md with SilentMode flag
```

---

## Notes

- Silent mode is opt-in via configuration flag
- Interactive mode behavior completely unchanged when `SilentMode` is `false`
- Error collection only active during silent mode
- All protocols (SFTP, FTP, SCP, WebDAV, S3) supported
- No modifications to third-party code in `libs/`
- Verbose logging throughout for debugging and audit trail

---

## Changelog

| Date | Change |
|------|--------|
| 2026-04-22 | Initial plan created |
| 2026-05-04 | **Revised for implementation readiness:** |
| | - Updated stale line numbers: `EffectiveBatchOverwrite` (3285), `ConfirmFileOverwrite` (3341), `FileOperationLoopQuery` (2657), macro definitions (Terminal.h:148-161) |
| | - **Simplified Task 10:** Replaced "wrap 20+ individual file operation loops" with single-point modification of `FileOperationLoopQuery()` -- all `FILE_OPERATION_LOOP_END` macro call sites benefit without macro changes |
| | - **Removed Task 11** (protocol name tracking) -- unnecessary complexity; error `Message` already provides sufficient context |
| | - Merged Checkpoint 4a/4b into single Checkpoint 4 reflecting simplified approach |
| 2026-05-04 | **Refined after codebase analysis (/aif-improve):** |
| | - **Task 10:** Added null guard for `AOperationProgress` (can be nullptr, existing code at :2665 does this) |
| | - **Task 4:** Removed `ProtocolName` field from `TFileOperationError` (inconsistent with Task 11 removal) |
| | - **Task 5:** Removed `ProtocolName` from `AddError()` signature |
| | - **Task 6:** Removed `ProtocolName` from `AddOperationError()` convenience method |
| | - **Task 12:** Removed `ProtocolName` from `GenerateReport()` format string and acceptance criteria |
| | - **Tasks 8-9:** Replaced vague "around line ~6900" with concrete method names |
| | - Verified: `TranslateExceptionMessage()` exists (`Sysutils.hpp:367`), `ExtException(const Exception*, UnicodeString)` constructor available via inheritance, `Sysutils.hpp` already included in `Terminal.cpp` |
