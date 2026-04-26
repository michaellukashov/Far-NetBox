# Crash Analysis: Second File Open Without Directory Refresh

## Summary

Far Manager crashes after opening a file via SFTP and then attempting to open another file without refreshing the directory (Ctrl+R). The crash occurs in `TTerminal::ProcessFiles` when accessing `TRemoteFile*` pointers that have become dangling.

## Call Stack

```
TTerminal::ProcessFiles → TTerminal::CalculateFilesSize → TTerminal::CopyToLocal
→ TWinSCPFileSystem::GetFilesRemote → TCustomFarFileSystem::GetFiles
```

## Root Cause Analysis

### 1. Panel Stores TRemoteFile* Pointers

When the remote directory is listed, `TRemoteFile` objects are created and stored in `TFarPanelItem::UserData`.

**Location:** `src/NetBox/WinSCPFileSystem.cpp`

```cpp
// TRemoteFilePanelItem::GetData() - stores pointer
void TRemoteFilePanelItem::GetData(...) {
    // ...
    UserData = FRemoteFile;  // Line 113
}
```

### 2. Directory Refresh Deletes Files

In `TTerminal::DoReadDirectoryFinish()`, when the directory is refreshed, the old `TRemoteDirectory` calls `Reset()` which deletes all `TRemoteFile` objects.

**Location:** `src/core/Terminal.cpp`

```cpp
void TTerminal::DoReadDirectoryFinish(TRemoteDirectory * AFiles, bool ReloadOnly) {
    std::unique_ptr<TRemoteDirectory> OldFiles(FFiles.release());
    FFiles.reset(AFiles);
    // ...
    OldFiles->Reset();  // Deletes TRemoteFile objects!
}
```

**Why files are deleted:**
- `TRemoteFileList` has `OwnsObjects = true` by default
- `Reset()` calls `Clear()` which calls `TList::Clear()`
- `TObjectList::Notify()` deletes owned objects

### 3. Dangling Pointers Remain in Panel

After the refresh, the panel's `UserData` still contains pointers to deleted `TRemoteFile` objects. The panel is not updated during the refresh cycle because Far Manager calls `GetFindData()` only after the refresh completes.

### 4. Crash on Second File Open

When the user opens another file:
1. `GetFiles()` is called
2. `CreatePanelItemList()` creates new `TFarPanelItem` wrappers
3. `GetFindDataEx()` fills these with `TFarPanelItem` (not `TRemoteFilePanelItem`)
4. `TFarPanelItem::GetUserData()` returns the old (dangling) pointer
5. `CreateFileList()` extracts these dangling `TRemoteFile*` pointers
6. `ProcessFiles()` calls `CalculateFilesSize()` → crash when accessing the dangling pointer

## Key Code Locations

### GetFiles Flow

| File | Function | Line | Purpose |
|------|----------|------|---------|
| `FarPlugin.cpp` | `GetFiles()` | 776 | Entry point from Far Manager |
| `WinSCPFileSystem.cpp` | `GetFilesEx()` | 2576 | Main GetFiles implementation |
| `WinSCPFileSystem.cpp` | `GetFilesRemote()` | 2693 | Remote file download logic |
| `WinSCPFileSystem.cpp` | `CreateFileList()` | 3123 | **Creates TStrings with TRemoteFile pointers** |
| `Terminal.cpp` | `CopyToLocal()` | 8279 | Copies files from remote to local |
| `Terminal.cpp` | `CalculateFilesSize()` | 4987 | Calculates size before copy |
| `Terminal.cpp` | `ProcessFiles()` | 4256 | Iterates files and calls callbacks |

### File List Creation

**Location:** `src/NetBox/WinSCPFileSystem.cpp:3123-3180`

```cpp
TStrings * TWinSCPFileSystem::CreateFileList(TObjectList * PanelItems,
    TOperationSide Side, bool SelectedOnly, const UnicodeString & ADirectory,
    bool FileNameOnly, TStrings * AFileList) {
    // ...
    if (Side == osRemote) {
        Data = static_cast<TRemoteFile *>(PanelItem->GetUserData());
        DebugAssert(Data);  // CRASH: Data is dangling pointer!
    }
    FileList->AddObject(FileName, Data);
    // ...
}
```

**Note:** The comment `// FileList->SetOwnsObjects(true);` at line 3131 suggests this was intentionally disabled, likely because the code expected objects to remain valid.

### File List Destruction (Where Files Get Deleted)

**Location:** `src/core/Terminal.cpp:3698-3715`

```cpp
void TTerminal::DoReadDirectoryFinish(TRemoteDirectory * AFiles, bool ReloadOnly) {
    std::unique_ptr<TRemoteDirectory> OldFiles(FFiles.release());
    FFiles.reset(AFiles);
    try__finally {
        DoReadDirectory(ReloadOnly);
    }
    __finally {
        // delete only after loading new files to dir view,
        // not to destroy the file objects that the view holds
        OldFiles->Reset();  // Deletes owned TRemoteFile objects!
    } end_try__finally
}
```

### Remote File Storage in Panel

**Location:** `src/NetBox/WinSCPFileSystem.cpp:94-113`

```cpp
void TRemoteFilePanelItem::GetData(...) {
    AFileName = FRemoteFile->GetFileName();
    // ...
    UserData = FRemoteFile;  // Stores raw pointer!
}
```

### FarPanelItem Creation for GetFindData

**Location:** `src/NetBox/FarPlugin.cpp:2760-2762`

```cpp
// Inside GetOpenPanelInfo()
FItems->Add(new TFarPanelItem(ppi, /*OwnsItem=*/true));
```

The `TFarPanelItem` wraps Far Manager's `PluginPanelItem` structure, preserving the `UserData` from that structure.

## Object Ownership Chain

```
TRemoteFile* (owned by TRemoteDirectory)
    ↓ stored in
TFarPanelItem::FPanelItem->UserData.Data
    ↓ wrapped by
TFarPanelItem
    ↓ added to
TObjectList (PanelItems)
    ↓ extracted by
CreateFileList()
    ↓ added to
TStrings (FileList)
    ↓ passed to
TTerminal::ProcessFiles()
```

**Problem:** When `TRemoteDirectory` is destroyed, all `TRemoteFile*` are deleted, but `TFarPanelItem` still holds pointers to them.

## Why Refresh Works

Pressing Ctrl+R explicitly refreshes the panel, which:
1. Triggers `GetFindData()` call
2. `GetFindDataEx()` creates new `TRemoteFilePanelItem` objects
3. These are created from the current `FFiles`, so they have valid pointers
4. Far Manager updates the panel's internal structures with new data

## Solution Options

### Option 1: Duplicate Remote Files (Recommended)

Modify `CreateFileList()` to duplicate `TRemoteFile` objects:

```cpp
Data = static_cast<TRemoteFile *>(PanelItem->GetUserData())->Duplicate(false);
```

**Pros:**
- Self-contained fix in one location
- File list owns its copies
- Works for all operations using file lists

**Cons:**
- Slight memory overhead for duplicates

### Option 2: Null-Check Before Use

Add null-check in `CreateFileList()`:

```cpp
Data = static_cast<TRemoteFile *>(PanelItem->GetUserData());
if (Data == nullptr) {
    // Handle missing data
}
```

**Pros:**
- Quick fix

**Cons:**
- Only masks the problem
- May break functionality for files with no cached metadata

### Option 3: Refresh Panel After Directory Change

Force panel refresh after directory operations.

**Pros:**
- Fixes root cause

**Cons:**
- May cause performance issues
- Affects all operations

## Selected Solution

**Option 1: Duplicate Remote Files** is recommended because:
1. It ensures the file list is self-contained
2. It doesn't rely on external state
3. It's a localized fix with clear boundaries

### Implementation Notes

- Use `Duplicate(false)` - creates a non-standalone copy (shares some data with original)
- Set `FileList->SetOwnsObjects(true)` - ensures duplicates are cleaned up
- Add debug logging for troubleshooting

## Related Code Patterns

### File Duplication Method

**Location:** `src/core/RemoteFiles.cpp:898-937`

```cpp
TRemoteFile * TRemoteFile::Duplicate(bool Standalone) const {
    std::unique_ptr<TRemoteFile> Result(std::make_unique<TRemoteFile>());
    // ... copies all properties ...
    if (Standalone && (!FFullFileName.IsEmpty() || (GetDirectory() != nullptr))) {
        Result->FFullFileName = GetFullFileName();
    }
    return Result.release();
}
```

### ObjectList Ownership

**Location:** `src/base/Classes.hpp:349`

```cpp
RWProperty2<bool> OwnsObjects{&FOwnsObjects};
```

When `OwnsObjects = true`, objects added to the list are deleted when the list is destroyed.

## Test Scenario

1. Connect to SFTP server
2. Navigate to a directory with files
3. Press Enter on a file → file downloads and opens in external app
4. Close external app
5. Press Enter on another file → **CRASH** (before fix)
6. Press Enter on another file → **SUCCESS** (after fix)

### Alternative Test (Workaround)
1-5. Same as above
6. Press Ctrl+R to refresh directory
7. Press Enter on another file → **SUCCESS** (always worked)

## References

- Stack overflow question: N/A (internal bug report)
- Related WinSCP issue: N/A (NetBox-specific)
- Similar patterns: None found in codebase

## Metadata

| Field | Value |
|-------|-------|
| **Analyzed** | 2026-04-26 |
| **Analyzer** | AI Assistant |
| **Severity** | High |
| **Status** | Root cause identified |
