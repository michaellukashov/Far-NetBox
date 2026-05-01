# Fix: SSH chmod 444 Crash on Remote Directories (Issue #393)

## Problem

Far Manager crashes when the user changes a remote directory's permissions to `444`
(`r--r--r--`) via the Attributes dialog (`Ctrl + A`) in NetBox over SFTP/SSH.

### Crash Scenario

1. Connect to an SSH/SFTP server
2. Navigate to a directory
3. Select the directory, press `Ctrl+A`
4. Change permissions to `444` (remove all execute bits)
5. Apply → **Far Manager crashes**

## Root Cause

`TSFTPFileSystem::ChangeFileProperties()` calls `ReadFile()` before sending the
chmod request to retrieve the file's current metadata (owner, group, rights).
If the server refuses this read (e.g., permission denied on a restricted
directory), `ReadFile()` sets the `File` pointer to `nullptr`.

The code then dereferences `File` at multiple sites without any null check:

- `File->GetIsDirectory()`
- `File->GetFileOwner()`
- `File->GetFileGroup()`
- `File->GetRights()` ← crash point

Secondary crash paths (same pattern — missing null guards):

- `TTerminal::ReadDirectory()` catch block: `FFiles->GetDirectory()` when `FFiles` is null
- `TWinSCPFileSystem::GetFindDataEx()`: `GetTerminal()->Files->Count` when `Files` is null

## Fix

Three null guards added with logging instrumentation:

### 1. SftpFileSystem.cpp — ChangeFileProperties()

After `ReadFile()`, check if `File == nullptr` and throw a meaningful exception
instead of crashing:

```cpp
ReadFile(RealFileName, File);
if (File == nullptr)
{
    FTerminal->LogEvent(FORMAT(L"ChangeFileProperties: ReadFile failed for %s", RealFileName));
    throw ExtException(nullptr, L"Cannot read file properties before changing them");
}
```

### 2. Terminal.cpp — ReadDirectory() catch block

Use fallback directory string when `FFiles` is null:

```cpp
catch (Exception & E)
{
    const UnicodeString Directory = (FFiles != nullptr) ? FFiles->GetDirectory() : GetCurrentDirectory();
    CommandError(&E, FMTLOAD(LIST_DIR_ERROR, Directory));
}
```

### 3. WinSCPFileSystem.cpp — GetFindDataEx()

Skip file iteration when `Files` is null:

```cpp
if (GetTerminal()->Files != nullptr)
{
    for (int32_t Index = 0; Index < GetTerminal()->Files->Count; ++Index)
    { ... }
}
else
{
    FTerminal->LogEvent(L"GetFindDataEx: GetTerminal()->Files is null");
}
```

## Verification

Build: `cmd /c build-x64.bat` — zero new warnings.

Manual test:

1. Launch Far Manager with patched NetBox plugin
2. Connect to SSH/SFTP server
3. Navigate to a writable directory
4. `Ctrl+A` → set permissions to `444`
5. Verify no crash; panel updates gracefully (may show empty listing)

## References

- Issue: [michaellukashov/Far-NetBox#393](https://github.com/michaellukashov/Far-NetBox/issues/393)
- Upstream: [FarGroup/Far-NetBox#37](https://github.com/FarGroup/Far-NetBox/issues/37)
