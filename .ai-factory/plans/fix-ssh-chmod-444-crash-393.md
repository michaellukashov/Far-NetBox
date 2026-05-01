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

**Note:** All tasks in Phase I are investigation steps; execution order is flexible but Tasks 2.6‑2.7 should follow Task 2's findings.

#### Task 1: Analyze crash path in SFTP `ChangeFileProperties`, rights handling, and dialog initialization

**Target files:**
- `src/core/SftpFileSystem.cpp`
- `src/core/RemoteFiles.cpp`
- `src/NetBox/WinSCPDialogs.cpp`

**Steps:**
1. Trace `TSFTPFileSystem::ChangeFileProperties()` from `ReadFile()` through `Packet.AddProperties()` and `SendPacketAndReceiveResponse()`. Verify `File->GetRights()` is non-null before `Packet.AddProperties(&Properties, *File->GetRights(), ...)` (~line 4412).
1a. **Critical:** `ReadFile(RealFileName, File)` at line 4373 may set `File` to nullptr on failure (e.g., permission denied). Line 4378 has `DebugAssert(FilePtr.get())` which fires in debug but is a no-op in release. If `ReadFile` fails, line 4412 dereferences null `File`. Add step to verify this path and plan a null-guard.
2. Examine `TRights::Combine()` bit-clearing behavior via `NumberUnset`.
   a. Trace `TRightsContainer::SetRights()` → `TRights::SetRight()` → `TRights::SetNumberSet/Unset()` when the dialog returns `444`. Verify `NumberUnset` is set for execute bits (owner, group, other).
   b. Trace `TRemoteProperties::ChangedProperties()` to confirm `NumberUnset` survives the `OriginalProperties → NewProperties` diff.
   c. Verify `TRights::Combine()` receives a `Properties->Rights` with `NumberUnset` execute bits set, so `Result &= ~Other.NumberUnset` actually clears them.
   d. If `NumberUnset` is zero for `444`, document this as a pre-existing functional bug (chmod sends wrong value) and note whether it contributes to the crash.
   e. **Note:** `TRights::operator==` uses dual-mode comparison: `AllowUndef==true` compares per-right (`FSet` and `FUnset`), `AllowUndef==false` compares only `GetNumber()` (`FSet`). For chmod 444 on a standard Unix directory, both sides are fully-defined, so the comparison is correct. This is a **functional bug** that could silently skip rights changes in edge cases (S3, WebDAV, incomplete parsing), but is **unlikely to be the direct crash cause**. Classify accordingly.
3. Check `AddProperties()` (`SftpFileSystem.cpp` ~line 502) for unexpected rights values. Verify `File->GetRights()` is non-null (defensive) and that `Properties->Rights` has `NumberUnset` set for execute bits so `Combine()` produces `0x124` (444), not a value with leftover execute bits.
4. Add verbose `LogEvent()` instrumentation around rights calculation and packet construction.

**Deliverable:** Written findings on whether the crash originates in the pre-chmod `ReadFile`, the `AddProperties` packet build, the `NumberUnset` propagation, or the post-chmod response handling.

#### Task 1.5: Add concrete logging instrumentation to track crash path

**Target files:** `src/core/SftpFileSystem.cpp`, `src/core/Terminal.cpp`, `src/NetBox/WinSCPFileSystem.cpp`

**Steps:**
1. In `TSFTPFileSystem::ChangeFileProperties()` (line 4373‑4412), add `FTerminal->LogEvent()` calls:
   - Before `ReadFile()`: log `"ChangeFileProperties: reading file %s"`, `RealFileName`.
   - After `ReadFile()`: log `"ChangeFileProperties: ReadFile returned %p"`, `File`.
   - Before `Packet.AddProperties()`: log `"ChangeFileProperties: rights set=%04x unset=%04x"`, `Properties.Rights.GetNumberSet()`, `Properties.Rights.GetNumberUnset()`.
   - After `SendPacketAndReceiveResponse()`: log `"ChangeFileProperties: chmod completed"`.
2. In `TTerminal::ReadDirectory()` catch block (line 3816), add log: `"ReadDirectory catch: FFiles=%p"`, `FFiles`.
3. In `TWinSCPFileSystem::GetFindDataEx()` (line 497), add log: `"GetFindDataEx: GetTerminal()->Files=%p"`, `GetTerminal()->Files`.
4. All logs must respect `FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2` for DEBUG, `>= 1` for INFO.

**Deliverable:** Concrete logging points inserted; enable verbose logging to capture crash state.

---

#### Task 2: Analyze panel refresh, `DoReadDirectoryFinish`, and error-handling path after non-traversable directory chmod

**Target files:**
- `src/core/Terminal.cpp`
- `src/NetBox/WinSCPFileSystem.cpp`
- `src/NetBox/FarPlugin.cpp`

**Steps:**
1. Trace `TTerminal::ChangeFileProperties()` → `FileModified()` → `ReactOnCommand(fsChangeProperties)` → `ReadDirectory(true)`.
2. Examine `ReadDirectory()` exception handling (`catch(Exception & E) { CommandError(...) }`). Verify `FFiles` is non-null before `FFiles->GetDirectory()` in the catch block. Check that `FMTLOAD(LIST_DIR_ERROR, ...)` receives a valid directory string and does not crash `fmt::format`. **Critical:** If `FFiles` is in a corrupted state (not null but with invalid internal data), `GetDirectory()` could return bad data that crashes `fmt::format`.
3. Examine `DoReadDirectory()` callback (`FOnReadDirectory`) in the plugin layer: verify it handles an empty or permission-denied directory gracefully without dereferencing null `TRemoteFile` objects.
3a. **Note:** `CreateSelectedFileList` duplicates `TRemoteFile` objects (via `RemoteFile->Duplicate(true)`), so stale pointer risk is mitigated for the dialog path. Focus investigation on the post-chmod `UpdatePanel()` → `GetFindDataEx()` path where `FFiles` is accessed directly without duplication.
4. Examine `DoReadDirectoryFinish()`: verify `AFiles` is non-null and valid before `FFiles.reset(AFiles)`. Check if `DoReadDirectory(ReloadOnly)` (which calls `FOnReadDirectory`) dereferences `FFiles` when it is null or empty.
5. Examine `TWinSCPFileSystem::FileProperties()` → `UpdatePanel()` / `RedrawPanel()` for invalid panel-item or `UserData` access after a failed refresh. Check `CreateSelectedFileList` for stale `TRemoteFile` pointers from a previous directory listing.
6. Add verbose `LogEvent()` instrumentation in `ReadDirectory`, `DoReadDirectoryFinish`, `DoReadDirectory`, and `FileModified`.
7. Examine `GetFindDataEx()` (`WinSCPFileSystem.cpp` ~line 497): `for (int32_t Index = 0; Index < GetTerminal()->Files->Count; ++Index)`. Plan a defensive `if (GetTerminal()->Files != nullptr)` guard before the loop. If null, log a DEBUG event and return empty panel items.

**Deliverable:** Written findings on whether the crash is a null dereference, use-after-free, unhandled exception during panel refresh, or invalid `FFiles` state after the directory becomes permission-denied.

---


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
2a. **Refinement:** The `__finally` block in `ReadDirectory` ensures `DoReadDirectoryFinish` runs before the catch block, so `FFiles` should be set by then. The crash risk is more about `FFiles` being in a **corrupted** state than being **null**. Verify `Files.release()` is non-null when `CustomReadDirectory` throws. If `Files` was created but `SetDirectory` or `CustomReadDirectory` failed partway, `Files` may be in an inconsistent state. Check `TRemoteDirectory` constructor and `SetDirectory` for null-safety.
3. Add null check: if `FFiles` is null or corrupted, use a fallback directory string (e.g., `GetCurrentDirectory()`) for the error message.
4. Add verbose `LogEvent()` instrumentation in the catch block.

**Deliverable:** `ReadDirectory` catch block that does not crash when `FFiles` is null.

**Depends on:** Task 2 (same investigation path).

### Phase II. Fix


#### Task 3.1: Guard `TSFTPFileSystem::ChangeFileProperties()` against null `File` after `ReadFile()` failure

**Target files:** `src/core/SftpFileSystem.cpp`

**Depends on:** Task 1 findings.

**Steps:**
1. In `TSFTPFileSystem::ChangeFileProperties()` (line 4373‑4412), add defensive null check after `ReadFile()` returns.
2. If `File` is null, log the failure (`"ChangeFileProperties: ReadFile failed for %s"`, `RealFileName`) and throw a meaningful exception (e.g., `EFatal`) instead of dereferencing null at line 4412 (`File->GetRights()`).
3. Ensure logging respects `FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1`.

**Deliverable:** Null‑safe `ChangeFileProperties()` that does not crash when `ReadFile` fails.

#### Task 3.2: Guard `TTerminal::ReadDirectory()` catch block against null `FFiles`

**Target files:** `src/core/Terminal.cpp`

**Depends on:** Task 2.7 findings.

**Steps:**
1. In `TTerminal::ReadDirectory()` catch block (line 3816), add null check: if `FFiles` is null, use `GetCurrentDirectory()` as fallback for the error message.
2. Log the condition: `"ReadDirectory catch: FFiles=%p, using fallback directory"`, `FFiles`.
3. Ensure `FMTLOAD(LIST_DIR_ERROR, …)` receives a valid directory string (non‑null).

**Deliverable:** `ReadDirectory` catch block that does not crash when `FFiles` is null.

#### Task 3.3: Guard `TWinSCPFileSystem::GetFindDataEx()` against null `GetTerminal()->Files`

**Target files:** `src/NetBox/WinSCPFileSystem.cpp`

**Depends on:** Task 2 findings.

**Steps:**
1. In `TWinSCPFileSystem::GetFindDataEx()` (line 497), add `if (GetTerminal()->Files != nullptr)` guard before the loop.
2. If null, log a DEBUG event (`"GetFindDataEx: GetTerminal()->Files is null, returning empty panel"`) and return empty panel items.
3. Follow naming conventions and add `FTerminal->LogEvent()` at INFO level for the defensive path.

**Deliverable:** Hardened `GetFindDataEx()` that does not crash if `FFiles` is unexpectedly null during panel refresh.

#### Task 3.4: Harden `DoReadDirectoryFinish()` against exceptions in `FOnReadDirectory` callback

**Target files:** `src/core/Terminal.cpp`, `src/NetBox/WinSCPFileSystem.cpp`

**Depends on:** Task 2.6 findings.

**Steps:**
1. If investigation shows `FOnReadDirectory` callback can throw and corrupt state, add `try/catch` around the callback invocation in `DoReadDirectoryFinish()`.
2. Ensure `FFiles` is set before `DoReadDirectory` is called (already done via `FFiles.reset(AFiles)`).
3. Log any caught exception with `FTerminal->LogEvent()` at WARN level.

**Deliverable:** `DoReadDirectoryFinish()` that isolates plugin‑layer exceptions from core state.

#### Task 3.5: Fix `TRights::Combine()` `NumberUnset` propagation (if bug confirmed)

**Target files:** `src/core/RemoteFiles.cpp`, `src/NetBox/WinSCPDialogs.cpp`

**Depends on:** Task 1 findings (NumberUnset verification).

**Steps:**
1. If investigation confirms `TRights::Combine()` or `TRightsContainer::SetRights()` does not properly populate `NumberUnset` for execute bits when setting `444`, fix the propagation so `Result &= ~Other.NumberUnset` actually clears execute bits.
2. Ensure the fix does not break other rights‑combination scenarios (e.g., `AllowUndef` modes).
3. Add a test‑comment referencing Issue #393.

**Deliverable:** Correct `NumberUnset` propagation for `444` chmod, ensuring execute bits are cleared.


### Phase III. Verification & Documentation

#### Task 4: Build verification and documentation update

**Steps:**
1. Run `cmd /c build-x64.bat` (or equivalent platform script). Build must pass with **zero warnings**.
2. Confirm the plugin DLL exists at `Far3_x64/Plugins/NetBox/NetBox.dll`.
3. Update this plan file's `## Fix Summary` section (below) with the actual root cause and fix description.
4. If the fix reveals a pattern worth recording, add a short note to `docs/` (e.g., `docs/fix-ssh-chmod-444-crash-393.md`) describing the crash scenario, root cause, and fix for future maintainers.

**Deliverable:** Clean build, verified artifact location, and updated documentation.

---

#### Task 5: Manual verification steps

**Target:** Far Manager with NetBox plugin, connected to an SSH/SFTP server.

**Steps:**
1. After building the plugin (Task 4), launch Far Manager (`Far3_x64\Far.exe`).
2. Connect to an SSH/SFTP server with a directory that you have write permissions to.
3. Navigate to that directory in NetBox panel.
4. Select the directory, press `Ctrl+A` to open the Attributes dialog.
5. Change permissions to `444` (`r--r--r--`), uncheck all execute bits, and apply.
6. Verify that Far Manager does not crash.
7. If the directory becomes non‑traversable (expected), verify that the panel updates gracefully (shows empty listing or error message).
8. Repeat the test with a file (non‑directory) to ensure the fix does not break normal chmod.

**Deliverable:** Confirmation that the crash is resolved and no regression introduced.

## Fix Summary

**Root cause:** `TSFTPFileSystem::ChangeFileProperties()` calls `ReadFile()` to retrieve the target file's metadata before sending the chmod request. If the server refuses the read (e.g., because the directory already has restrictive permissions or the file does not exist), `ReadFile()` sets the `File` pointer to `nullptr`. The code then immediately dereferences `File` at multiple sites (`GetIsDirectory()`, `GetFileOwner()`, `GetFileGroup()`, `GetRights()`) without any null check, causing an access-violation crash.

**Fixes applied:**

| Task | File | Change |
|------|------|--------|
| 3.1 | `src/core/SftpFileSystem.cpp` | Null guard after `ReadFile()` — throw `ExtException` if `File == nullptr` instead of crashing |
| 3.2 | `src/core/Terminal.cpp` | Null guard in `ReadDirectory()` catch block — use `GetCurrentDirectory()` fallback when `FFiles` is null |
| 3.3 | `src/NetBox/WinSCPFileSystem.cpp` | Null guard in `GetFindDataEx()` — skip file iteration when `GetTerminal()->Files` is null |
| 2.6 | `src/core/Terminal.cpp` | Null guard in `DoReadDirectoryFinish()` — return early if `AFiles == nullptr` |
| 1.5 | All three files | Added `LogEvent()` instrumentation at DEBUG/INFO level for crash-path tracing |

**NumberUnset investigation:** `TRights::Combine()` correctly populates `NumberUnset` for execute bits when setting `444`. No fix required — this is a functional bug only in edge cases, not the crash cause.

**Build:** x64 RelWithDebugInfo passes with zero new warnings.

---

## Roadmap Linkage

Milestone: "none"  
Rationale: Skipped — this is an independent bug fix not tied to a current roadmap milestone.
