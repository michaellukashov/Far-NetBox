# Plan: Fix Stack Overflow on SFTP Directory Size Calculation — Issue #497

**GitHub Issue:** [#497](https://github.com/michaellukashov/Far-NetBox/issues/497)
**Bug:** `0xC00000FD — STATUS_STACK_OVERFLOW` when pressing F3 on SFTP directory with ~98,648 files
**Root cause:** Recursive directory traversal for size calculation has no symlink cycle detection

---

## Problem Analysis

### What happens
1. User presses F3 on an SFTP directory → `foCalculateSize` operation
2. `CalculateFilesSize` → `ProcessFiles` → `DoCalculateFileSize` → `ProcessDirectory`
3. `ProcessDirectory` lists files, calls `CalculateFileSize` callback for each
4. For directory entries, `CalculateFileSize` calls `DoCalculateDirectorySize` → `ProcessDirectory` (recursive)
5. For symlinked directories (when `ResolveSymlinks` is enabled), the symlink may point to a parent directory (e.g., `self -> .` or `self -> ..`)
6. The recursion continues infinitely until the C++ call stack overflows

### Why cycle detection doesn't work here
- `TLoopDetector` class exists and works correctly for `FilesFind` (search operations) — lines 7317-7324
- **But** `ProcessDirectory` used by the size calculation path has NO `LoopDetector` — lines 4061-4117
- The `CanRecurseToDirectory` check (line 9165-9168) only checks `FollowDirectorySymlinks` session setting, not cycle detection
- The `FindLinkedFile` cyclic link detection (RemoteFiles.cpp:1563-1576) only checks if the same `LinkTo` string appears in the current chain — it does NOT detect cycles where symlink points to the current directory or any ancestor via relative paths

### Confirmed by user
Disabling "Resolve symbolic links" (`NB_LOGIN_RESOLVE_SYMLINKS`) avoids the crash — proving the bug is in symlink resolution during recursive traversal.

---

## Fix Strategy

### I. Add `TLoopDetector` to `TCalculateSizeParams`

**File:** `src/core/Terminal.cpp` (or `Terminal.h`)

- The `TFilesFindParams` struct (line 105-108) already has a `TLoopDetector LoopDetector` member
- The `TCalculateSizeParams` struct (around line 117) does NOT
- **Add** `TLoopDetector` member (or a `std::unique_ptr<TStringList> FVisitedDirectories`) to `TCalculateSizeParams`
- Alternatively, since `TCalculateSizeParams` is a simple struct with a constructor, add a `std::unique_ptr<TStringList> FVisitedDirs` field

### II. Record visited directories during size calculation

**File:** `src/core/Terminal.cpp` — `DoCalculateDirectorySize` (line 4945)

- When entering a directory for size calculation, record its **real path** in the visited set
- For symlinks, resolve to the real/absolute path (similar to line 7314: `base::AbsolutePath(AParams->RealDirectory, AFile->GetLinkTo())`)
- For NON-symlink directories: use `TUnixPath::Join(AParams->RealDirectory, AFile->GetFileName())`

### III. Check before recursing into directories

**File:** `src/core/Terminal.cpp` — `CalculateFileSize` (line 4893-4922)

**Critical:** This is where the fix must be applied. Before calling `DoCalculateDirectorySize`:

1. **For symlinked directories** (when `AFile->GetIsSymLink()` is true and `AFile->GetLinkTo()` is non-empty):
   - Compute `RealDirectory = base::AbsolutePath(currentPath, AFile->GetLinkTo())`
   - Check if `RealDirectory` is already in visited set

2. **For regular (non-symlink) directories**:
   - Compute `RealDirectory = TUnixPath::Join(currentPath, AFile->GetFileName())`
   - Also check against visited set (handles mount point cycles)

3. **If already visited:** Log the cycle detection, skip recursion, increment symlink counter
4. **If not visited:** Record it, proceed with `DoCalculateDirectorySize`

### IV. Add logging for cycle detection

Log when a cycle is detected (similar to the existing message at line 7319):
```
LogEvent(FORMAT("Already counted \"%s\" directory (real path \"%s\"), link loop detected", FullFileName, RealDirectory));
```

---

## Detailed Implementation Plan

### Phase I. Foundation

1. **Add `VisitedDirs` to `TCalculateSizeParams`** — The struct is at line 990 in `src/core/Terminal.h`. Add `TStringList * VisitedDirs{nullptr};` as the last member before the closing `};`. This is safe because `TStringList` is already available through existing includes (forward-declared in `base/Classes.hpp`).

2. **Initialize and clean up in call sites** — There are 3 places where `TCalculateSizeParams` is created:
   - `Terminal.cpp:6149` (`CalculateLocalFileSize`)
   - `Terminal.cpp:6626` (`DirectoryIsEmpty`)
   - `Terminal.cpp:7161` (`SynchronizeByChecksum`)
   - Additionally, public API `CalculateFilesSize` at line 4987 also takes this struct

   All of these need no changes because the new field defaults to `nullptr` (zero-initialized). The key is that we MUST initialize `VisitedDirs` before recursion happens in the function that initiates size calculation.

3. **Create helper functions in Terminal.cpp** (not modifying TLoopDetector):
   - `CreateVisitedDirsSet()` — creates `CreateSortedStringList()` for visited paths
   - `IsDirectoryVisitedAndRecord()` — checks-and-records in one operation (pattern from TLoopDetector)
   - These are private to Terminal.cpp, using the new `Params->VisitedDirs` field

### Phase II. Integration

3. **Modify `CalculateFilesSize`** (line 4987-5003) — This is the public API that receives a file list for size calculation:
   - At the START of this function (before any processing), initialize `VisitedDirs` if it's nullptr: `Params.VisitedDirs = CreateSortedStringList();`
   - Record the starting directory: `Params.VisitedDirs->Add(startingDirectory);`
   - At the END (before return), optionally clean up: `delete Params.VisitedDirs; Params.VisitedDirs = nullptr;` (or let caller manage)

4. **Modify `CalculateFileSize` callback** (line 4893-4922) — This is the critical path:
   - After line 4895 (`if (CanRecurseToDirectory(AFile))`), BEFORE the recursive call at line 4913:

   ```cpp
   // NEW: Check for cycles before recursing
   UnicodeString RealDirectory;
   if (AFile->GetIsSymLink() && !AFile->GetLinkTo().IsEmpty())
   {
     // Symlink: resolve to absolute path
     RealDirectory = base::AbsolutePath(currentPath, AFile->GetLinkTo());
   }
   else
   {
     // Regular directory: join path
     RealDirectory = TUnixPath::Join(currentPath, AFile->GetFileName());
   }
   // Normalize for comparison
   RealDirectory = ::ExcludeTrailingBackslash(RealDirectory);

   // Check and record
   if (Params->VisitedDirs && Params->VisitedDirs->IndexOf(RealDirectory) >= 0)
   {
     LogEvent(FORMAT("Already counted \"%s\" directory (real path \"%s\"), link loop detected", FileName, RealDirectory));
     // Skip recursion, directory already counted
     // Fall through without incrementing directory counter
   }
   else
   {
     if (Params->VisitedDirs) Params->VisitedDirs->Add(RealDirectory);
     // Continue with DoCalculateDirectorySize call
   }
   ```

5. **Modify `DoCalculateDirectorySize`** (line 4945) — Record the initial directory:
   - At beginning of function: `if (Params->VisitedDirs == nullptr) Params->VisitedDirs = CreateSortedStringList();`
   - Optionally record `FileName` at entry point

### Phase III. Edge Cases & Additional Considerations

6. **Handle relative symlink targets** — `base::AbsolutePath()` handles `.` and `..` correctly
7. **Handle absolute symlink targets** — use the path as-is
8. **Handle empty LinkTo** — treat as non-symlink directory (current behavior at line 4895 via `CanRecurseToDirectory`)
9. **Non-symlink directories** — need cycle detection for hard links or mount points that may cause cycles (use joined path as real path)

10. **Thread safety** — The `VisitedDirs` TStringList is NOT thread-safe by default. However, the size calculation path runs on the main thread (Far Manager API requirement), so no thread safety issue is expected.

11. **Memory management** — Since `TCalculateSizeParams` is created on the stack in some call sites (lines 6149, 6626, 7161), the new `VisitedDirs` pointer will default to nullptr and be cleaned up when the stack frame is popped. For heap-allocated usage, ensure the caller frees `VisitedDirs` after use.

12. **Lazy initialization** — Initialize `VisitedDirs` only when first needed (inside `DoCalculateDirectorySize`), not upfront, to minimize overhead.

### Phase IV. Verification

13. **Build with zero warnings** (MSVC W4)
14. **Test scenario:** SFTP directory containing cyclic symlinks (`self -> .`, `link -> ..`)
15. **Test scenario:** SFTP directory with deep symlink chains (no cycles) — should still work
16. **Test scenario:** Regular directory without symlinks — should work exactly as before

---

## Questions for Confirmation

Before proceeding with implementation, please clarify:

1. **Scope:** Should we also protect other code paths that use `ProcessDirectory` for recursion (e.g., file search with `IsRecurse` enabled)? The same issue could theoretically affect search operations.

2. **Memory cleanup:** Who should own and free the `VisitedDirs` TStringList? Options:
   - Caller allocates, caller frees (current approach in plan)
   - Structestructor frees (would require non-trivial changes)
   - Lazy-free in `CalculateFilesSize` after completion

3. **Additional protection:** Should we also add a maximum recursion depth limit as a safety net? Current stack depth at crash is ~98K files. A configurable limit (e.g., 10,000 directories) could provide defense-in-depth.

4. **Alternative approach:** Instead of modifying `TCalculateSizeParams`, we could pass `TLoopDetector&` as a parameter to `DoCalculateDirectorySize`. This keeps changes more localized. Should we consider this approach instead?

---

## Out of Scope - Not Fixed in This Plan

The following code paths use `ProcessDirectory` recursively with similar vulnerability but are NOT fixed here:
- `Terminal.cpp:4050` - DeleteFile operation (triggers on F8 key)
- `Terminal.cpp:8640` - SinkFile operation (triggers on F5 copy key)

**Why out of scope**: Bug #497 specifically concerns F3 (get file info → size calculation). These other operations involve different Far Manager keys and are separate code paths.

---

## Files to Modify

|File|Change|
|---|---|
|`src/core/Terminal.h`|Add `TStringList * VisitedDirs{nullptr};` member to `TCalculateSizeParams` struct (line 1011)|
|`src/core/Terminal.cpp`|Add helper functions for visited-set management; add cycle check in `CalculateFileSize` callback (line 4893-4922); ensure `VisitedDirs` is initialized before recursion|

## Risk Assessment

- **Low risk**: The change is additive — adds a visited-directory check before existing recursion
- **No behavior change** for directories without cycles — they will work exactly as before
- **No modification to `libs/`** — all changes are in `src/core/`
- **Memory impact**: `TStringList` of visited paths per size calculation — minimal (each path entry is small)
