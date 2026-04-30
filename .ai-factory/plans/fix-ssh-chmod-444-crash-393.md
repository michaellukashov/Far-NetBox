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
- **Known functional quirk:** `TRights::Combine()` clears bits via `Result &= static_cast<uint16_t>(~Other.NumberUnset)` in addition to setting bits via `Result |= Other.NumberSet`. If `NumberUnset` is not populated when the dialog returns `444` (e.g., because `AllowUndef` is false), execute bits may not be cleared. This may contribute to unexpected server responses or NetBox state.
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
1a. **Critical:** `ReadFile(RealFileName, File)` at line 4373 may set `File` to nullptr on failure (e.g., permission denied). Line 4378 has `DebugAssert(FilePtr.get())` which fires in debug but is a no-op in release. If `ReadFile` fails, line 4412 dereferences null `File`. Add step to verify this path and plan a null-guard.
2. Examine `TRights::Combine()` bit-clearing behavior via `NumberUnset`. Verify that `TRightsContainer::SetRights()` and `TRemoteProperties::ChangedProperties()` populate `NumberUnset` for `444` so that execute bits are actually cleared, not just OR'd in.
3. Check `AddProperties()` (`SftpFileSystem.cpp` ~line 502) for unexpected rights values. Verify `File->GetRights()` is non-null (defensive) and that `Properties->Rights` has `NumberUnset` set for execute bits so `Combine()` produces `0x124` (444), not a value with leftover execute bits.
4. Add verbose `LogEvent()` instrumentation around rights calculation and packet construction.

**Deliverable:** Written findings on whether the crash originates in the pre-chmod `ReadFile`, the `AddProperties` packet build, the `NumberUnset` propagation, or the post-chmod response handling.

---

#### Task 2: Analyze panel refresh, `DoReadDirectoryFinish`, and error-handling path after non-traversable directory chmod

**Target files:**
- `src/core/Terminal.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`
- `src/NetBox/FarPlugin.cpp`

**Steps:**
1. Trace `TTerminal::ChangeFileProperties()` → `FileModified()` → `ReactOnCommand(fsChangeProperties)` → `ReadDirectory(true)`.
2. Examine `ReadDirectory()` exception handling (`catch(Exception & E) { CommandError(...) }`). Verify `FFiles` is non-null before `FFiles->GetDirectory()` in the catch block. Check that `FMTLOAD(LIST_DIR_ERROR, ...)` receives a valid directory string and does not crash `fmt::format`.
3. Examine `DoReadDirectoryFinish()`: verify `AFiles` is non-null and valid before `FFiles.reset(AFiles)`. Check if `DoReadDirectory(ReloadOnly)` (which calls `FOnReadDirectory`) dereferences `FFiles` when it is null or empty.
4. Examine `DoReadDirectory()` callback (`FOnReadDirectory`) in the plugin layer: verify it handles an empty or permission-denied directory gracefully without dereferencing null `TRemoteFile` objects.
5. Examine `TWinSCPFileSystem::FileProperties()` → `UpdatePanel()` / `RedrawPanel()` for invalid panel-item or `UserData` access after a failed refresh. Check `CreateSelectedFileList` for stale `TRemoteFile` pointers from a previous directory listing.
6. Add verbose `LogEvent()` instrumentation in `ReadDirectory`, `DoReadDirectoryFinish`, `DoReadDirectory`, and `FileModified`.

**Deliverable:** Written findings on whether the crash is a null dereference, use-after-free, unhandled exception during panel refresh, or invalid `FFiles` state after the directory becomes permission-denied.

---

#### Task 1.5: Verify `TRights::Combine()` `NumberUnset` propagation from dialog to packet

**Target files:**
- `src/core/RemoteFiles.cpp`
- `src/NetBox/WinSCPDialogs.cpp`

**Steps:**
1. Trace `TRightsContainer::SetRights()` → `TRights::SetRight()` → `TRights::SetNumberSet/Unset()` when the dialog returns `444`. Verify `NumberUnset` is set for execute bits (owner, group, other).
2. Trace `TRemoteProperties::ChangedProperties()` to confirm `NumberUnset` survives the `OriginalProperties → NewProperties` diff.
3. Verify `TRights::Combine()` receives a `Properties->Rights` with `NumberUnset` execute bits set, so `Result &= ~Other.NumberUnset` actually clears them.
4. If `NumberUnset` is zero for `444`, document this as a pre-existing functional bug (chmod sends wrong value) and note whether it contributes to the crash.

**Deliverable:** Confirmation that `Combine()` produces the expected `0x124` (444) or identification of a `NumberUnset` propagation gap.

---

#### Task 2.5: Add defensive null-check for `GetTerminal()->Files` in `GetFindDataEx`

**Target files:**
- `src/NetBox/WinSCPFileSystem.cpp`

**Steps:**
1. Examine `GetFindDataEx()` (`WinSCPFileSystem.cpp` ~line 497): `for (int32_t Index = 0; Index < GetTerminal()->Files->Count; ++Index)`.
2. Add a defensive `if (GetTerminal()->Files != nullptr)` guard before the loop. If null, log a DEBUG event and return empty panel items.
3. Follow naming conventions and add `FTerminal->LogEvent()` at INFO level for the defensive path.

**Deliverable:** Hardened `GetFindDataEx()` that does not crash if `FFiles` is unexpectedly null during panel refresh.

**Depends on:** Task 2 (must understand panel refresh flow first).

#### Task 2.6: Harden `DoReadDirectoryFinish` against exceptions in `FOnReadDirectory` callback

**Target files:**
- `src/core/Terminal.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`

**Steps:**
1. Examine `DoReadDirectoryFinish()`: inside `try__finally`, `DoReadDirectory(ReloadOnly)` calls `FOnReadDirectory` which triggers `TerminalReadDirectory` and downstream panel code (`GetFindDataEx`).
2. Verify that if `GetFindDataEx` or panel code dereferences stale `TRemoteFile` objects (from `CreateSelectedFileList` duplication), the exception is caught or the state is protected.
3. Consider wrapping the `FOnReadDirectory` callback invocation with additional exception logging so that crashes in the plugin layer do not corrupt `OldFiles->Reset()`.
4. Ensure `FFiles` is in a consistent state before `DoReadDirectory` is called (already set via `FFiles.reset(AFiles)`).

**Deliverable:** Findings on whether `DoReadDirectoryFinish` needs a `try/catch` around `DoReadDirectory` or whether the existing `__finally` cleanup is sufficient.

**Depends on:** Task 2 (must understand panel refresh flow first).

---

#### Task 2.7: Guard `ReadDirectory` catch block against null `FFiles`

**Target files:**
- `src/core/Terminal.cpp`

**Steps:**
1. Examine `ReadDirectory()` catch block (`Terminal.cpp` line 3814-3817):
```cpp
catch (Exception & E)
{
  CommandError(&E, FMTLOAD(LIST_DIR_ERROR, FFiles->GetDirectory()));
}
```
2. If `FFiles` is null when this executes (possible if `CustomReadDirectory` throws before `DoReadDirectoryFinish` sets `FFiles`), dereferencing `FFiles->GetDirectory()` crashes.
3. Add null check: if `FFiles` is null, use a fallback directory string (e.g., `GetCurrentDirectory()`) for the error message.
4. Add verbose `LogEvent()` instrumentation in the catch block.

**Deliverable:** `ReadDirectory` catch block that does not crash when `FFiles` is null.

**Depends on:** Task 2 (same investigation path).

### Phase II. Fix

#### Task 3: Implement defensive null-checks and error handling in critical paths

**Target files:** Primary: `src/core/Terminal.cpp`, `src/NetBox/WinSCPFileSystem.cpp`. Secondary investigation: `src/core/RemoteFiles.cpp`, `src/NetBox/WinSCPDialogs.cpp`, `src/core/SftpFileSystem.cpp`.

**Depends on:** Tasks 1, 1.5, 2, 2.5, 2.6, 2.7 (all investigation tasks must complete before fix is applied).

**Steps:**
1. **Pre-chmod null guard** (from Task 1 findings): In `TSFTPFileSystem::ChangeFileProperties()` (`SftpFileSystem.cpp` ~line 4373-4412), add defensive null check after `ReadFile()` returns. If `File` is null, log the failure and throw a meaningful exception instead of dereferencing null at line 4412 (`File->GetRights()`).
2. **ReadDirectory catch block guard** (from Task 2.7 findings): In `TTerminal::ReadDirectory()` (`Terminal.cpp` line 3816), guard against null `FFiles` in the catch block. Use `GetCurrentDirectory()` as fallback for the error message if `FFiles` is null.
3. **GetFindDataEx null guard** (from Task 2.5 findings): In `TWinSCPFileSystem::GetFindDataEx()` (`WinSCPFileSystem.cpp` line 497), add `if (GetTerminal()->Files != nullptr)` check before the loop. Return empty panel items if null.
4. **DoReadDirectoryFinish hardening** (from Task 2.6 findings): If investigation shows `FOnReadDirectory` callback can throw and corrupt state, add `try/catch` around the callback invocation. Ensure `FFiles` is set before `DoReadDirectory` is called.
5. **NumberUnset propagation fix** (from Task 1.5 findings): If `TRights::Combine()` or `TRightsContainer::SetRights()` does not properly populate `NumberUnset` for execute bits when setting `444`, fix the propagation so `Result &= ~Other.NumberUnset` actually clears execute bits.
6. Ensure all new code paths log via `FTerminal->LogEvent()` at appropriate levels (INFO for success paths, DEBUG for internal state).
7. Follow naming conventions (`T` prefix for classes, `F` prefix for members, PascalCase methods, 2-space indent, CRLF, UTF-8 no BOM).

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
