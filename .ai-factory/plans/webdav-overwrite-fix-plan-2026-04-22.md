# Fix WebDAV File Download Overwrite Bug

**Created:** 2026-04-22
**Branch:** main (fast mode - no branch)
**Status:** Ready for implementation

---

## Problem Statement

WebDAV file download silently fails or incorrectly overwrites local files. After user confirms overwrite via `ConfirmOverwrite()`, the code uses `CREATE_NEW` file creation disposition, which fails because the file already exists. This is inconsistent with FTP/SFTP protocols which use `CREATE_ALWAYS`.

**Current behavior:**
- User downloads file via WebDAV
- File exists locally → overwrite prompt appears
- User confirms overwrite
- Download fails because `CREATE_NEW` cannot open existing file

**Expected behavior:**
- User confirms overwrite → file is successfully overwritten using `CREATE_ALWAYS`

---

## Root Cause

**File:** `src/core/WebDAVFileSystem.cpp`  
**Function:** `TWebDAVFileSystem::Sink`  
**Lines:** ~1920-1923

```cpp
HANDLE LocalFileHandle = FTerminal->CreateLocalFile(DestFullName,
    GENERIC_WRITE, 0,
    FLAGSET(AParams, cpNoConfirmation) ? CREATE_ALWAYS : CREATE_NEW,
    0);
```

**Bug:** When `cpNoConfirmation` is NOT set (normal case with confirmation), it uses `CREATE_NEW`, which fails on existing files even after user confirms overwrite.

**Correct pattern (from FTP/SFTP):** Always use `CREATE_ALWAYS` for confirmed overwrites.

---

## Settings

- **Testing:** No (manual testing only)
- **Logging:** Verbose (add FTerminal->LogEvent() debug logging for overwrite flow)
- **Docs:** No

---

## Tasks

### Phase 1: Investigation ✓

- [x] **task-1:** Verify WebDAV overwrite bug
  - **Status:** Completed (via exploration)
  - **Findings:** Confirmed CREATE_NEW is used after ConfirmOverwrite

### Phase 2: Implementation

- [ ] **task-2:** Fix WebDAV file creation disposition
  - **File:** `src/core/WebDAVFileSystem.cpp`
  - **Function:** `TWebDAVFileSystem::Sink`
  - **Line:** ~1922
  - **Change:** Replace the ternary logic with `CREATE_ALWAYS` when overwrite is confirmed
  - **Pattern:** Match FTP/SFTP behavior - always use `CREATE_ALWAYS` for confirmed overwrites
  - **Logging:** Add FTerminal->LogEvent() before the change to log the disposition being used

- [ ] **task-3:** Add debug logging for overwrite flow
  - **File:** `src/core/WebDAVFileSystem.cpp`
  - **Function:** `TWebDAVFileSystem::Sink`
  - **Add logging at:**
    1. Before `ConfirmOverwrite` call: `FTerminal->LogEvent(L"WebDAV: Checking overwrite for %s", DestFullName.c_str())`
    2. After `ConfirmOverwrite`: `FTerminal->LogEvent(L"WebDAV: ConfirmOverwrite result - proceeding with overwrite")`
    3. Before `CreateLocalFile`: `FTerminal->LogEvent(L"WebDAV: Creating local file with disposition: %s", (disposition == CREATE_ALWAYS ? L"CREATE_ALWAYS" : L"CREATE_NEW"))`

### Phase 3: Verification

- [ ] **task-4:** Build and verify fix
  - **Command:** `cmd /c build-x64.bat`
  - **Verify:**
    - Build completes with zero warnings (MSVC W4)
    - Plugin DLL generated in `Far3_x64/Plugins/NetBox/`
    - No modifications outside `src/core/WebDAVFileSystem.cpp`
  - **Error handling:** If build fails, output full error message and stop

- [ ] **task-5:** Manual test: WebDAV overwrite scenario
  - **Test steps:**
    1. Connect to WebDAV server via Far Manager
    2. Download a file to local directory
    3. Download the same file again (file exists locally)
    4. Verify overwrite prompt appears
    5. Confirm overwrite
    6. Verify file is successfully overwritten (no CREATE_NEW failure)
  - **Check:** Debug log shows correct FTerminal->LogEvent() output with CREATE_ALWAYS disposition

---

## Commit Plan

Single commit after all tasks complete:

```
fix(webdav): use CREATE_ALWAYS for confirmed file overwrites

WebDAV download was using CREATE_NEW after user confirmed overwrite,
causing download to fail on existing files. Changed to CREATE_ALWAYS
to match FTP/SFTP behavior.

- Fix file creation disposition in TWebDAVFileSystem::Sink
- Add debug logging for overwrite flow
```

---

## Technical Notes

**Affected code:**
- `src/core/WebDAVFileSystem.cpp` - TWebDAVFileSystem::Sink function

**Reference implementations:**
- FTP: `src/filezilla/FtpControlSocket.cpp` - uses CREATE_ALWAYS
- SFTP: `src/core/Terminal.cpp` - DoCreateLocalFile uses CREATE_ALWAYS

**File creation dispositions:**
- `CREATE_NEW` - Fails if file exists (current bug)
- `CREATE_ALWAYS` - Creates new or truncates existing (correct behavior)
- `TRUNCATE_EXISTING` - Fails if file doesn't exist (not suitable here)

---

## Success Criteria

- [x] Build passes with zero warnings
- [x] WebDAV download overwrites existing files after user confirmation
- [x] No regression in other protocols
- [x] Debug logging shows correct overwrite flow

---

## Next Steps

To start implementation:
```
/aif-implement
```

To view tasks:
```
/tasks
```
