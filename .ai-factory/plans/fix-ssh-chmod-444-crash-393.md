# Fix SSH chmod 444 crash on remote directories (Issue #393)

> **Reference:** [michaellukashov/Far-NetBox#393](https://github.com/michaellukashov/Far-NetBox/issues/393)  
> **Upstream:** [FarGroup/Far-NetBox#37](https://github.com/FarGroup/Far-NetBox/issues/37)  
> **Created:** 2026-04-30  
> **Mode:** Fast plan (no branch)

---

Far Manager crashes when the user changes a remote directory's permissions to `444` (`r--r--r--`) via the Attributes dialog (`Ctrl + A`) in NetBox over SFTP/SSH. The crash is synchronous during the `Ctrl+A` key-press handling (stack top: `ProcessPanelInputW`), which means it occurs either when opening the dialog, when `ChangeFilesProperties` triggers a post-chmod panel refresh, or in the `UpdatePanel`/`RedrawPanel` cleanup after the operation. The `fmt` and `tinylog` frames in the crash dump (with large offsets, indicating symbol-resolution artifacts) point to the formatting/logging neighbourhood of the binary, suggesting the crash may involve a `FORMAT()` call or `LogEvent()` on an invalid object.
---

## Settings

| Setting   | Value    | Notes |
|-----------|----------|-------|
| Testing   | **no**   | Skip test tasks per user instruction |
| Logging   | **verbose** | Add `FTerminal->LogEvent()` calls at DEBUG/INFO level in all changed areas |
| Docs      | **yes**  | Mandatory docs checkpoint at completion |

---

- **Issue:** Crash when setting remote directory permissions to `444` (`r--r--r--`) via `Ctrl + A`.
- **Protocol:** SFTP over SSH (PuTTY-based `TSFTPFileSystem`).
- **Environment:** Far Manager 3.0 x64 with NetBox plugin.
- **Crash stack:** `ProcessPanelInputW` → `FileProperties` → `ChangeFilesProperties` → `ReactOnCommand(fsChangeProperties)` → `ReadDirectory(true)`. The crash is synchronous in the key-press handler.
- **Key observation:** A directory with `444` has no execute bits, so it cannot be listed. After a successful chmod, `ReadDirectory(true)` fails with permission denied. The crash is likely in the error-handling or cleanup path after this failure, or in `DoReadDirectoryFinish` / `DoReadDirectory` callback (`FOnReadDirectory`) when `FFiles` is in an unexpected state.
- **Staleness risk:** `CreateSelectedFileList` duplicates `TRemoteFile` objects from panel `UserData`. If the panel items or `FFiles` are updated mid-operation, duplicated objects may reference stale state.
- **Known functional quirk:** `TRights::Combine()` uses bitwise OR (`Result |= Other.NumberSet`), which means it cannot *clear* permission bits. Setting `444` on a directory that previously had execute bits may not actually remove them. This may contribute to unexpected server responses or NetBox state.
---

## Architecture & Boundaries

Changes are scoped to the **Core Layer** (`src/core/`) and **Plugin Layer** (`src/NetBox/`).

```
Plugin Layer (src/NetBox/)
  └── WinSCPFileSystem::FileProperties() → Ctrl+A dialog
Core Layer (src/core/)
  └── TTerminal::ChangeFilesProperties()
      └── TSFTPFileSystem::ChangeFileProperties()
      └── TTerminal::ReactOnCommand(fsChangeProperties)
          └── ReadDirectory(true) → CustomReadDirectory → SFTP OPENDIR/READDIR
Base Layer (src/base/)
  └── TRights, TRemoteFile, TRemoteDirectory
```

**Constraints:**
- No modifications to `libs/` — use patches if third-party code needs changing.
- CMake build must pass with zero warnings (MSVC W4).
- Plugin DLL must land in `Far3_<platform>/Plugins/NetBox/`.

---

## Tasks

### Phase I. Investigation

#### Task 1: Analyze crash path in SFTP `ChangeFileProperties`, rights handling, and dialog initialization

**Target files:**
- `src/core/SftpFileSystem.cpp`
- `src/core/RemoteFiles.cpp`
- `src/NetBox/WinSCPDialogs.cpp`

**Steps:**
1. Trace `TSFTPFileSystem::ChangeFileProperties()` from `ReadFile()` through `Packet.AddProperties()` and `SendPacketAndReceiveResponse()`. Verify `File->GetRights()` is non-null before `Packet.AddProperties(&Properties, *File->GetRights(), ...)` (~line 4412).
2. Examine `TRights::Combine()` behavior when the new rights are `444` and the base rights include execute bits. Verify whether the resulting packet sends the intended value or a corrupted/unexpected value.
3. Check `AddProperties()` (`SftpFileSystem.cpp` ~line 502) for any null-dereference or invalid `TRights` dereference (`*File->GetRights()`).
4. Examine `TRightsContainer::SetRights()` in `WinSCPDialogs.cpp` for crashes when `AllowUndef` is true and rights bits are partially undefined. Verify the octal-edit mask (`9999`) and `GetModeStr()` / `GetText()` do not produce invalid strings for `444`.
5. Add verbose `LogEvent()` instrumentation around rights calculation and packet construction.

**Deliverable:** Written findings on whether the crash originates in the pre-chmod `ReadFile`, the `AddProperties` packet build, the dialog rights display, or the post-chmod response handling.

---

#### Task 2: Analyze panel refresh, `DoReadDirectoryFinish`, and error-handling path after non-traversable directory chmod

**Target files:**
- `src/core/Terminal.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`
- `src/NetBox/FarPlugin.cpp`

**Steps:**
1. Trace `TTerminal::ChangeFileProperties()` → `FileModified()` → `ReactOnCommand(fsChangeProperties)` → `ReadDirectory(true)`.
2. Examine `ReadDirectory()` exception handling (`catch(Exception & E) { CommandError(...) }`). Verify `AFileList` is valid and `CommandError` does not call `FORMAT` with a null/invalid argument.
3. Examine `DoReadDirectoryFinish()`: verify `AFiles` is non-null and valid before `FFiles.reset(AFiles)`. Check if `DoReadDirectory(ReloadOnly)` (which calls `FOnReadDirectory`) dereferences `FFiles` when it is null or empty.
4. Examine `DoReadDirectory()` callback (`FOnReadDirectory`) in the plugin layer: verify it handles an empty or permission-denied directory gracefully without dereferencing null `TRemoteFile` objects.
5. Examine `TWinSCPFileSystem::FileProperties()` → `UpdatePanel()` / `RedrawPanel()` for invalid panel-item or `UserData` access after a failed refresh. Check `CreateSelectedFileList` for stale `TRemoteFile` pointers from a previous directory listing.
6. Add verbose `LogEvent()` instrumentation in `ReadDirectory`, `DoReadDirectoryFinish`, `DoReadDirectory`, and `FileModified`.

**Deliverable:** Written findings on whether the crash is a null dereference, use-after-free, unhandled exception during panel refresh, or invalid `FFiles` state after the directory becomes permission-denied.

---

### Phase II. Fix

#### Task 3: Implement the crash fix

**Target files:** TBD by Tasks 1 and 2 (likely one or more of):
- `src/core/SftpFileSystem.cpp`
- `src/core/Terminal.cpp`
- `src/core/RemoteFiles.cpp`
- `src/NetBox/WinSCPDialogs.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`

**Steps:**
1. Apply the fix identified in Phase I. Likely candidates:
   - Add defensive null checks before dereferencing `File->GetRights()` or `AFile->GetDirectory()`.
   - Harden `ReadDirectory` error handling so a permission-denied refresh does not pass invalid state to `DoReadDirectoryFinish` or the Far panel.
   - Fix `DoReadDirectoryFinish` to handle `AFiles == nullptr` or `AFiles->GetCount() == 0` safely before calling `DoReadDirectory`.
   - Fix `DoReadDirectory` callback (`FOnReadDirectory`) to verify `FFiles` is non-null before accessing it.
   - Fix `TRights::Combine` or `AddProperties` if they produce incorrect/corrupted permission values for `444`.
   - Fix `CreateSelectedFileList` or `TRemoteFile::Duplicate` if they produce objects with invalid `FRights` state.
2. Ensure all new code paths log via `FTerminal->LogEvent()` at appropriate levels (INFO for success paths, DEBUG for internal state).
3. Follow naming conventions (`T` prefix for classes, `F` prefix for members, PascalCase methods, 2-space indent, CRLF, UTF-8 no BOM).

**Deliverable:** Crash-free chmod operation on remote directories with `444` permissions, with zero compiler warnings.

---

### Phase III. Verification & Documentation

#### Task 4: Build verification and documentation update

**Steps:**
1. Run `cmd /c build-x64.bat` (or equivalent platform script). Build must pass with **zero warnings**.
2. Confirm the plugin DLL exists at `Far3_x64/Plugins/NetBox/NetBox.dll`.
3. Update this plan file's `## Fix Summary` section (below) with the actual root cause and fix description.
4. If the fix reveals a pattern worth recording, add a short note to `docs/` (e.g., `docs/fix-ssh-chmod-444-crash-393.md`) describing the crash scenario, root cause, and fix for future maintainers.

**Deliverable:** Clean build, verified artifact location, and updated documentation.

---

## Fix Summary

> *To be filled in after Task 3 is complete. Record the actual root cause and the exact files/lines changed.*

---

## Roadmap Linkage

Milestone: "none"  
Rationale: Skipped — this is an independent bug fix not tied to a current roadmap milestone.
