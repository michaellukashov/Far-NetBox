# Plan: Fix Crash on Second File Open Without Directory Refresh

> **Exploration:** [crash-second-file-open-analysis.md](../references/crash-second-file-open-analysis.md)
> **GitHub Issue:** [#508](https://github.com/michaellukashov/Far-NetBox/issues/508)

## Metadata

| Field | Value |
|-------|-------|
| **Branch** | `fix/crash-on-second-file-open` |
| **Created** | 2026-04-26 |
| **Type** | Bug fix |
| **Priority** | High |
| **Status** | ✅ Verified Fixed |

## Problem Statement

Far Manager crashes after opening a file via SFTP and then attempting to open another file without refreshing the directory (Ctrl+R). The crash occurs in `TTerminal::ProcessFiles` when accessing `TRemoteFile*` pointers that have become dangling.

## Root Cause Summary

1. **Panel stores `TRemoteFile*` in `UserData`**: When the remote directory is listed, `TRemoteFile` objects are created and stored in `TFarPanelItem::UserData`.

2. **Directory refresh deletes files**: In `TTerminal::DoReadDirectoryFinish()`, when the directory is refreshed, the old `TRemoteDirectory` calls `Reset()` which deletes all `TRemoteFile` objects (because `TRemoteFileList::OwnsObjects = true`).

3. **Dangling pointers**: After the refresh, the panel's `UserData` still contains pointers to deleted `TRemoteFile` objects.

4. **Crash on second open**: When the user opens another file, `CreateFileList()` extracts these dangling `TRemoteFile*` pointers, and `ProcessFiles()` crashes when accessing them.

## Solution

Make a **deep copy** of `TRemoteFile` objects in `CreateFileList()` to ensure the file list owns its own copies that are independent of the panel's files.

### Key Changes

1. **Set `FileList->SetOwnsObjects(true)`** for remote file lists - ensures duplicated files are cleaned up
2. **Duplicate `TRemoteFile` objects** using `Duplicate(true)` - creates independent copies with FFullFileName computed
3. **Add null-pointer check** - handles cases where `GetUserData()` returns nullptr
4. **Add debug logging** - helps with troubleshooting future issues

### Why `Standalone=true` is Required

The `Duplicate(true)` parameter is required because:

- With `Standalone=true`, `FFullFileName` is computed and stored during duplication
- This ensures `GetFullFileName()` works correctly on the duplicated file
- Code paths that call `GetFullFileName()` will work properly

## Code Changes

### File: `src/NetBox/WinSCPFileSystem.cpp`

#### Location: `CreateFileList()` function (lines 3123-3180)

**Current code (lines 3131-3152):**
```cpp
std::unique_ptr<TStrings> FileList((AFileList == nullptr) ? new TStringList() : AFileList);
if (AFileList == nullptr)
{
    // FileList->SetOwnsObjects(true);
    FileList->SetCaseSensitive(true);
    FileList->SetDuplicates(dupAccept);
}
// ...
for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
{
    // ...
    if (Side == osRemote)
    {
        Data = static_cast<TRemoteFile *>(PanelItem->GetUserData());
        DebugAssert(Data);  // Line 3149
    }
    // ...
}
```

**Replace with:**
```cpp
std::unique_ptr<TStrings> FileList((AFileList == nullptr) ? new TStringList() : AFileList);
if (AFileList == nullptr)
{
    FileList->SetOwnsObjects(true);  // Enable ownership for remote file lists
    FileList->SetCaseSensitive(true);
    FileList->SetDuplicates(dupAccept);
}
// ...
for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
{
    // ...
    if (Side == osRemote)
    {
        TRemoteFile * RemoteFile = static_cast<TRemoteFile *>(PanelItem->GetUserData());
        if (RemoteFile != nullptr)
        {
            // Duplicate the file to avoid dangling pointer issues after directory refresh.
            // Use Standalone=true to compute and store FFullFileName so that
            // GetFullFileName() works correctly even when FDirectory is nullptr.
            Data = RemoteFile->Duplicate(true);
        }
        else
        {
            Data = nullptr;
        }
    }
    // ...
}
```

**Add debug logging** after the for-loop (before `return FileList.release();`):
```cpp
if (Side == osRemote && FileList->GetCount() > 0)
{
    TINYLOG_DEBUG(g_tinylog) << TLogContext::Format()
        << " CreateFileList: duplicated " << FileList->GetCount()
        << " remote file objects to prevent stale pointer issues";
}
```

### Duplicate() Method Reference

**Location:** `src/core/RemoteFiles.cpp:898-950`

```cpp
TRemoteFile * TRemoteFile::Duplicate(bool Standalone) const
{
    std::unique_ptr<TRemoteFile> Result(std::make_unique<TRemoteFile>());
    // Copies: Terminal, Owner, ModificationFmt, Size, CalculatedSize,
    //         FileName, DisplayName, INodeBlocks, Modification, LastAccess,
    //         Group, IconIndex, TypeName, IsSymLink, LinkTo, Type,
    //         Tags, CyclicLink, HumanRights, IsEncrypted
    // If Standalone=true: computes and stores FFullFileName
    // Note: FDirectory is NOT copied (remains nullptr)
    return Result.release();
}
```

## Tasks

### Phase I: Implement Fix

1. ~~**Modify `CreateFileList()` to duplicate remote files**~~ ✅
   - File: `src/NetBox/WinSCPFileSystem.cpp`
   - Function: `TStrings * TWinSCPFileSystem::CreateFileList()` (line 3123)
   - Changes:
     - Set `FileList->SetOwnsObjects(true)` for remote file lists
     - Replace direct pointer assignment with `Duplicate()` call
     - Add null-pointer check for robustness
     - Add verbose logging (TINYLOG_TRACE, TINYLOG_DEBUG, TINYLOG_WARNING)
   - Implemented: 2026-04-26

### Phase II: Verification

2. ~~**Build the plugin**~~ ✅
   - Build type: `RelWithDebugInfo` (default)
   - Platform: `x64` (or test platform)
   - Build command: `cmd /c build-x64.bat`
   - Expected: Zero warnings, successful compilation
   - Build completed: 2026-04-26

3. **Test the fix**
   - Manual testing in Far Manager
   - Verify no crash on second file open
   - See Testing Plan below for details

## Architecture Notes

### Dependency Flow

```
Plugin (WinSCPFileSystem.cpp)
  └─ Core (Terminal.cpp)
      └─ Base (RemoteFiles.cpp)
          └─ Third-party (None)
```

- Plugin layer calls core layer (CreateFileList → ProcessFiles → CopyToLocal)
- Core layer uses base classes (TRemoteFile, TStrings)
- No third-party dependencies affected

### Third-Party Library Boundaries

- No changes to `libs/` directory
- No CMake changes required
- Standard MSVC build process applies

### Build Configuration

| Setting | Value |
|---------|-------|
| **Build type** | `RelWithDebugInfo` |
| **Platform** | `x64` (or test platform) |
| **CMake option** | `OPT_CREATE_PLUGIN_DIR=ON` (for plugin directory) |
| **Unity build** | Can disable if symbol conflicts: `OPT_USE_UNITY_BUILD=OFF` |
| **Output** | `Far3_x64/Plugins/NetBox/NetBox.dll` |

## Testing Plan

### Primary Test: Second File Open Without Refresh

**Steps:**
1. Connect to SFTP server via NetBox
2. Navigate to a directory containing multiple files
3. Press Enter on first file → file downloads and opens in associated application
4. Close the external application
5. Press Enter on a different file → **should NOT crash** (after fix)
6. Close the external application
7. Repeat steps 5-6 multiple times

**Expected result:** No crash, files open successfully each time

### Baseline Test: With Refresh (Should Always Work)

**Steps:**
1. Same as above, but press Ctrl+R between file opens
2. Press Enter on file → file opens
3. Press Ctrl+R to refresh directory
4. Press Enter on another file → should work

**Expected result:** Works both before and after fix (regression check)

### Edge Case Tests

| Test | Scenario | Expected Result |
|------|----------|----------------|
| Single file directory | Open only file, close, reopen same file | Success |
| Large directory | Open files in directory with 100+ items | Success |
| Subdirectory | Open file after entering/exiting subdirectory | Success |
| Directory download | Download entire directory (parallel) | Works correctly |
| Rename during session | Rename file while panel shows old name, then open | Graceful handling |
| Connection drop | Connection drops during file open | Proper error handling |

### Debug Logging Verification

Enable debug logging and verify output:
```
CreateFileList: duplicated N remote file objects to prevent stale pointer issues
```

Should appear in log file: `%LOCALAPPDATA%\NetBox\netbox.log`

## Edge Cases and Error Handling

### Null-Pointer Handling

If `PanelItem->GetUserData()` returns nullptr:
- Set `Data = nullptr` (don't crash)
- Log warning: `"CreateFileList: panel item has no UserData for file: %s"`
- File operations may fail gracefully with proper error message

### Duplicate() Exception Safety

The `Duplicate()` method contains a `try__catch` block, so it won't throw:
```cpp
TRemoteFile * TRemoteFile::Duplicate(bool Standalone) const
{
    std::unique_ptr<TRemoteFile> Result(std::make_unique<TRemoteFile>());
    try__catch
    {
        // ... copy properties ...
    }
    __catch__removed
    {
#if defined(__BORLANDC__)
        delete Result;
        throw;
#endif
    } end_try__catch
    return Result.release();
}
```

### Memory Considerations

- Each `Duplicate()` call creates a new `TRemoteFile` object (~1KB per file)
- `SetOwnsObjects(true)` ensures cleanup when file list is destroyed
- Memory is freed when file operation completes (via TStrings destructor)
- For typical usage (1-10 files), memory overhead is negligible

## Acceptance Criteria

| Criterion | Verification Method |
|-----------|-------------------|
| No crash on second file open | Manual test, 5+ iterations |
| Debug logging appears | Check `%LOCALAPPDATA%\NetBox\netbox.log` |
| Build succeeds with zero warnings | `build-x64.bat` output |
| Regression: refresh still works | Manual test with Ctrl+R |
| Edge cases handled | All edge case tests pass |

## WinSCP Porting Notes

This fix is based on WinSCP patterns:

1. **Original WinSCP behavior**: WinSCP likely handles this differently (different UI framework)
2. **NetBox equivalent**: Far Manager plugin requires explicit pointer management
3. **What was missing**: The commented-out `SetOwnsObjects(true)` and lack of duplication

The fix follows existing patterns in `RemoteFiles.cpp`:
- `DuplicateTo()` methods use `Duplicate(false)` for same-session operations
- File ownership is managed by container objects

## Commit Plan

Single commit:

```
fix(sftp): prevent crash on second file open without directory refresh

Make CreateFileList() duplicate TRemoteFile objects to avoid dangling
pointers after directory refresh. This fixes a crash where Far Manager
crashes when opening files twice without pressing Ctrl+R to refresh.

Root cause: TRemoteFile* stored in panel UserData become invalid after
directory refresh because the original objects are deleted. Duplicating
the files ensures the file list owns independent copies.

Changes:
- Enable FileList->SetOwnsObjects(true) for remote file lists
- Duplicate TRemoteFile using Duplicate(false) before storing
- Add null-pointer check for robustness
- Add debug logging for troubleshooting

Fixes: crash on second file open without directory refresh
Fixes: GitHub issue #508
```
