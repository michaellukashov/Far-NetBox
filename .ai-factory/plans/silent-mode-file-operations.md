# Plan: Silent Mode for File Operations

**Branch:** feature/silent-mode-file-operations
**Created:** 2026-04-22
**Status:** Ready for implementation

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

#### Task 1: Add SilentMode configuration flag
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

#### Task 2: Add SilentMode to GUI configuration
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

#### Task 3: Add SilentMode to Far configuration
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

#### Task 4: Create TFileOperationError structure
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
  UnicodeString ProtocolName;
  TFileOperationErrorCategory Category;
  
  TFileOperationError() = default;
  TFileOperationError(
    const UnicodeString & AFileName,
    const UnicodeString & AErrorMessage,
    TOperationSide ASide,
    const UnicodeString & AProtocolName,
    TFileOperationErrorCategory ACategory = TFileOperationErrorCategory::Other);
};
```

**Logging:**
- `DEBUG: FileOperationError created: file=[FileName], side=[Side], protocol=[ProtocolName], category=[Category]`

**Acceptance:**
- `TFileOperationError` structure defined
- `TFileOperationErrorCategory` enum defined
- Constructor initializes all fields including current timestamp
- Supports copy/move semantics

---

#### Task 5: Create TFileOperationErrorLog class
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
    const UnicodeString & ProtocolName,
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

#### Task 6: Integrate error log into TFileOperationProgressType
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
    const UnicodeString & ProtocolName,
    TFileOperationErrorCategory Category = TFileOperationErrorCategory::Other)
  {
    FErrorLog.AddError(FileName, ErrorMessage, Side, ProtocolName, Category);
  }
  ```
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
me|#### Task 7: Modify EffectiveBatchOverwrite for silent mode
ft|**Files:**
tp|- `src/core/Terminal.cpp` (line ~3285: `EffectiveBatchOverwrite()`)

cx|**Description:**
lw|Update `TTerminal::EffectiveBatchOverwrite()` to return `boAll` (overwrite all) when silent mode is active, bypassing all user prompts. This is the first line of defense -- it prevents confirmation dialogs before they are reached.

qt|**Implementation:**
lt|- Add check at the beginning of `EffectiveBatchOverwrite()`:
nn|  ```cpp
ta|  TBatchOverwrite TTerminal::EffectiveBatchOverwrite(
sy|    const UnicodeString & ASourceFullFileName,
rm|    const TCopyParamType * CopyParam,
ed|    int32_t Params,
ie|    TFileOperationProgressType * AOperationProgress,
vl|    bool Special) const
rz|  {
dn|    // Silent mode: never prompt, always overwrite
ea|    if (FConfiguration->GetSilentMode())
cb|    {
ue|      LogEvent(1, L"Silent mode active: auto-overwrite enabled");
dj|      return boAll;
du|    }

uw|    // ... existing logic
ct|  }
xa|  ```

**Logging:**
- `DEBUG: Silent mode active: auto-overwrite enabled` (level 2 = DEBUG)
- `DEBUG: EffectiveBatchOverwrite: SilentMode=[true/false], returning [BatchOverwriteMode]`

**Note:** Silent mode always returns `boAll` (overwrite all). If user wants conditional overwrite (e.g., only newer files), they must use interactive mode with batch overwrite settings.

**Acceptance:**
- When `SilentMode` is `true`, `EffectiveBatchOverwrite()` returns `boAll`
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
- `TTerminal::CopyToLocal()` (around line 6900)
- `TTerminal::CopyToRemote()` (around line 7900)
- `TTerminal::SynchronizeCollect()` (around line 8300)

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
- `TTerminal::Synchronize()` (around line 8200)
- `TTerminal::SynchronizeCollect()` (around line 8300)

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

zs|#### Task 10: Modify FileOperationLoopQuery for silent mode continue-on-error
ft|**Files:**
hp|- `src/core/Terminal.cpp` (line ~2657: `FileOperationLoopQuery()`)

cx|**Description:**
wy|Modify `FileOperationLoopQuery()` -- the single chokepoint where all file operation exceptions trigger interactive prompts -- to support silent mode. When silent mode is active, instead of calling `QueryUserException()` (which blocks for user input), collect the error and skip the file.

qt|**Implementation:**
tm|Add silent mode check at the top of `FileOperationLoopQuery()`, before the prompt logic:
pb|```cpp
rh|bool TTerminal::FileOperationLoopQuery(
lu|  Exception & E, TFileOperationProgressType * AOperationProgress,
ct|  const UnicodeString & Message, uint32_t AFlags,
ec|  const UnicodeString & /*SpecialRetry*/, const UnicodeString & /*HelpKeyword*/)
ec|{
ym|  bool Result{false};
jd|  Log->AddException(&E);
yd|
jw|  // Silent mode: collect error, skip file, continue
kv|  if (FConfiguration->GetSilentMode())
ur|  {
gl|    const UnicodeString ErrorMsg = TranslateExceptionMessage(&E);
ak|    OperationProgress->AddOperationError(
fe|      /*FileName extracted from Message or context*/,
hl|      ErrorMsg, OperationProgress->GetSide(), L"");
td|    LogEvent(0, L"Silent mode: Skipping file after error: " + ErrorMsg);
aa|    throw ESkipFile(&E, Message);
kg|  }
ed|
wm|  // ... existing interactive prompt logic
ib|}
rq|```

gs|**Key Design Points:**
zb|- Single point of modification -- all 20+ `FILE_OPERATION_LOOP_END` macro call sites benefit
al|- Uses existing `ESkipFile` exception to continue to next file (standard NetBox/WinSCP pattern)
hj|- Error collected via `OperationProgress->AddOperationError()` (Task 6 integration)
sr|- No changes to `FILE_OPERATION_LOOP_END` macro itself -- preserves existing interactive behavior

gs|**Logging:**
zb|- `WARN: Silent mode: Skipping file after error: [ErrorMessage]` (level 0 = WARN)
al|- `DEBUG: Silent mode: Error collected for [FileName]` (level 2 = DEBUG)
hj|- `INFO: Silent mode: [N] errors collected so far` (level 1 = INFO)

fk|**Acceptance:**
kh|- When `SilentMode` is `true`, file operation errors do NOT prompt user
yv|- Errors are collected and operation continues to next file
pm|- Interactive mode behavior unchanged when `SilentMode` is `false`
df|- `ESkipFile` propagates correctly to skip the current file
ng|
fw|**Blocked by:** Task 6, Task 8
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
    Report += FORMAT(L"[%s] %s (%s, %s)\n  %s\n\n",
      FormatDateTime(L"yyyy-mm-dd hh:nn:ss", Error.Timestamp),
      Error.FileName,
      Error.Side == osLocal ? L"Local" : L"Remote",
      Error.ProtocolName,
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
- Report includes timestamp, filename, side, protocol, error message
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

#### Task 13: Display error report after operations
**Files:**
- `src/core/Terminal.cpp` (end of file operation methods)

**Description:**
After file operations complete in silent mode, display error report if any errors occurred.

**Implementation:**
At the end of `CopyToLocal()`, `CopyToRemote()`, etc.:
```cpp
if (FConfiguration->GetSilentMode() && OperationProgress->GetErrorLog().HasErrors())
{
  const UnicodeString Report = OperationProgress->GetErrorLog().GenerateReport();
  LogEvent(1, L"Silent mode error report:\n" + Report);
  
  // Show to user via callback if available
  if (FOnInformation)
  {
    FOnInformation(this, Report, 0, L"");
  }
  else
  {
    // Fallback: write to error log file
    const UnicodeString ErrorLogPath = FConfiguration->GetLogFileName() + L".errors";
    // Write Report to ErrorLogPath
  }
}
```
```

**Logging:**
- `INFO: Silent mode error report:` (level 1 = INFO)
- `[Full error report content]`
- `DEBUG: Error report written to [ErrorLogPath]` (if fallback used)

**Acceptance:**
- Error report logged after operations complete
- Report shown to user if callback available
- Only displayed when errors occurred

**Blocked by:** Task 12, Task 10

---

### Phase 6: Testing

#### Task 14: Add unit tests for configuration
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

#### Task 15: Add integration tests for silent mode
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

#### Task 16: Update user documentation
**Files:**
- `docs/silent-mode.md` (create)
- `docs/configuration.md` (update)

**Description:**
Document silent mode feature, configuration, and error reporting for end users.

**Content:**
- What is silent mode
- When to use it (automation, batch operations, deadlock prevention)
- How to enable/disable (configuration setting)
- Error reporting behavior (dialog if available, fallback to .errors file)
- Limitations and caveats:
  - Silent mode always overwrites files (no conditional overwrite)
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

pq|**Checkpoint 4** (after Task 10):
sx|```
hm|feat(core): add silent mode continue-on-error to FileOperationLoopQuery

nt|- Modify FileOperationLoopQuery() to skip file and collect error when SilentMode active
op|- Single-point modification benefits all 20+ FILE_OPERATION_LOOP_END call sites
vr|- Uses existing ESkipFile pattern for continue-to-next-file behavior
hn|- Preserves existing interactive prompt behavior when SilentMode is false
zt|```

**Checkpoint 5** (after Task 13):
```
feat(core): add error reporting

- Implement error report generation
- Display error report after operations complete
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
