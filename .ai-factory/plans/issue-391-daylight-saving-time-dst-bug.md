# Fix Daylight Saving Time Bug in SCP Upload Timestamps (Issue #391)

**GitHub Reference:** [michaellukashov/Far-NetBox#391](https://github.com/michaellukashov/Far-NetBox/issues/391)

**Original upstream reference:** [FarGroup/Far-NetBox#35](https://github.com/FarGroup/Far-NetBox/issues/35) by @viccpp
**Exploration Reference:** [issue-391-dst-timestamp-exploration](../../references/issue-391-dst-timestamp-exploration.md)

**Plan Date:** 2026-05-02
**Mode:** Fast
**Plan ID:** issue-391-daylight-saving-time-dst-bug

---

## Settings

- **Testing:** no
- **Logging:** verbose
- **Docs:** yes

---

## Research Context

> **Exploration:** See [issue-391-dst-timestamp-exploration](../../references/issue-391-dst-timestamp-exploration.md) for detailed root-cause analysis, affected code paths, and WinSCP upstream comparison.

### Problem Summary
When a file is saved from the Far Manager editor with "Upload after every save" enabled over SCP protocol, the remote file receives a timestamp that is exactly 1 hour behind the actual local time during Daylight Saving Time (DST) periods. Both client and server are in the same timezone.

The issue is reproducible with default session settings (`dstmWin` DST mode). Workaround exists: switching DST mode to "Preserve remote timestamp (Unix)" (`dstmUnix`). The same root cause also affects overwrite confirmation dialogs and timestamp comparisons in SFTP, S3, and WebDAV uploads.

### Root Cause Analysis

`ConvertTimestampToUnix()` in `src/base/Common.cpp` (line ~2918) applies an incorrect DST bias correction when converting a Windows `FILETIME` to a Unix timestamp on modern Windows (Windows 7 and newer, where `UsesDaylightHack()` returns `false`).

**Cascade effect:** `OpenLocalFile` (Terminal.cpp:6102) derives `AHandle.Modification` (TDateTime) from the corrupted `MTime` via `UnixToDateTime(AHandle.MTime, DSTMode)`. Since `MTime` is wrong, `Modification` is equally corrupted. This affects overwrite confirmation dialogs and timestamp comparisons across all protocols. Fixing the root cause in `ConvertTimestampToUnix` automatically corrects both `MTime` (int64_t) and `Modification` (TDateTime).

**Key facts:**
- Windows `FILETIME` is **always UTC** by definition.
- Unix timestamps are **always UTC** by definition.
- The conversion should be pure arithmetic: `Unix = (FILETIME - epoch) / 10000000`.
- On Win7+ with default `dstmWin` mode, `ConvertTimestampToUnix` **subtracts** `DaylightDifferenceSec` (typically 3600) from the UTC result when the file's local date falls within DST.
- This subtraction is a legacy compensation for the pre-Windows 7 DST display bug, which **no longer exists** on Win7+.
- The bug is most visible in **SCP** because SCP uploads use `LocalFileHandle.MTime` (already corrupted) directly in the SCP `T` command. SFTP performs a double conversion (`DateTimeToFileTime` → `ConvertTimestampToUnix`) which partially masks the error.

### Affected Code Path (Editor Save → SCP Upload)

1. `ProcessEditorEvent()` detects `EE_SAVE` → sets `FEditorPendingSave = true`
2. `UploadFromEditor()` → `OpenLocalFile(fileName, GENERIC_READ, LocalFileHandle)`
3. `OpenLocalFile()` calls `GetLocalFileTime()` → `ConvertTimestampToUnix(MTime, GetSessionData()->GetDSTMode())`
4. **BUG:** With `dstmWin` on Win7+, `ConvertTimestampToUnix` subtracts 3600 seconds if DST is active
5. `SCPSource()` sends corrupted `LocalFileHandle.MTime` via SCP `T` command:
   ```cpp
   Buf = FORMAT("T%lu 0 %lu 0", nb::ToUInt32(LocalFileHandle.MTime), nb::ToUInt32(LocalFileHandle.ATime));
   ```

### Related Functions

| Function | File | Role |
|---|---|---|
| `ConvertTimestampToUnix()` | `src/base/Common.cpp:2918` | **Root cause** — subtracts DST for `dstmWin` on Win7+ |
| `UnixToDateTime()` | `src/base/Common.cpp:2638` | Reverse conversion; subtracts `BaseDifference` only for `dstmWin` on Win7+ |
| `DateTimeToFileTime()` | `src/base/Common.cpp:2856` | Applies `(fileDST - currentDST)` adjustment on Win7+ regardless of DST mode |
| `GetLocalFileTime()` | `src/core/Terminal.cpp:6028` | Reads FILETIME and converts via `ConvertTimestampToUnix` |
| `OpenLocalFile()` | `src/core/Terminal.cpp:5982` | Populates `LocalFileHandle.MTime` using `ConvertTimestampToUnix` |
| `SCPSource()` | `src/core/ScpFileSystem.cpp:2065` | Sends `LocalFileHandle.MTime` via SCP `T` command |

### Why the Workaround Works

Setting DST mode to `dstmUnix` on Win7+ makes `ConvertTimestampToUnix` skip the DST subtraction (the `else` branch only triggers for `dstmWin`), producing a correct UTC timestamp. However, `dstmUnix` is documented for legacy Windows servers, and users expect the default `dstmWin` to "just work" for modern systems.

---

## Tasks

### Phase 1: Fix Core Timestamp Conversion

- [x] Task 1.1: Fix `ConvertTimestampToUnix` for `dstmWin` on Win7+
**File:** `src/base/Common.cpp`
**Lines:** ~2943-2956 (the `else` block for `!UsesDaylightHack()`)

**Change:** Remove the entire `else` branch body for `dstmWin` in `ConvertTimestampToUnix()`. On Win7+, `FILETIME` is pure UTC and no DST compensation is needed for the `dstmWin` mode.

**Current code:**
```cpp
else  // Win7+ — Windows DST bug is fixed, FILETIME is pure UTC
{
    if (DSTMode == dstmWin)
    {
        FILETIME LocalFileTime;
        SYSTEMTIME SystemTime;
        FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
        FileTimeToSystemTime(&LocalFileTime, &SystemTime);
        const TDateTime DateTime = SystemTimeToDateTimeVerbose(SystemTime);
        const TDateTimeParams * Params = GetDateTimeParams(DecodeYear(DateTime));
        Result -= (IsDateInDST(DateTime) ?
            Params->DaylightDifferenceSec : Params->StandardDifferenceSec);
    }
}
```

**Expected fix:** Remove the entire `if (DSTMode == dstmWin)` block inside the `else` branch (lines 2945-2956). The `else` branch should do nothing for `dstmWin`, just as the pre-Win7 `UsesDaylightHack()` branch does nothing for `dstmWin`. All of `FileTimeToLocalFileTime`, `FileTimeToSystemTime`, `GetDateTimeParams`, `IsDateInDST`, and the subtraction are dead code for this path.

**Rationale:** On Win7+, `FILETIME` is already pure UTC. There is no legacy DST bug to compensate for. The subtraction corrupts the Unix timestamp by the DST offset (typically 3600 seconds).

**Logging:** Add an inline comment explaining that `dstmWin` on Win7+ performs pure UTC arithmetic conversion with no DST adjustment. A `DebugAssert` may be added to document the invariant that `DaylightDifferenceSec == 0` for this path.
- [x] Task 1.2: Verify symmetry with `UnixToDateTime`
**File:** `src/base/Common.cpp`  
**Lines:** ~2638-2668

**Check:** Confirm that `UnixToDateTime` for `dstmWin` on Win7+ does **not** re-add the DST offset that Task 1.1 removed.

Current `UnixToDateTime` for `dstmWin` on Win7+:
```cpp
else  // !DaylightHack
{
    Result -= Params->BaseDifference;  // timezone offset only — correct
}
// No DST adjustment for dstmWin on Win7+ here — correct
```

This is already consistent with the intended fix (no DST adjustment for `dstmWin` on Win7+). **No change needed** in `UnixToDateTime`.

- [x] Task 1.2b: Check `Sysutils.cpp` `FileAge` hardcoded `dstmUnix` usage
**File:** `src/base/Sysutils.cpp`
**Lines:** ~619-624

**Check:** `FileAge` uses `ConvertTimestampToUnixSafe(FindData.ftLastWriteTime, dstmUnix)` with `dstmUnix` hardcoded, followed by `UnixToDateTime(..., dstmUnix)`. This bypasses the session DST mode entirely.

**Verification:** This path is not affected by the `dstmWin` bug (it uses `dstmUnix`). Confirm that this internal utility is intentionally using `dstmUnix` for consistency with legacy behavior. **No change needed**, but document the finding for completeness.

#### Task 1.3: Audit and document `DateTimeToFileTime` behavior on Win7+
- [x] Task 1.3: Audit and document `DateTimeToFileTime` behavior on Win7+
**File:** `src/base/Common.cpp`  
**Lines:** ~2856-2885

**Check:** `DateTimeToFileTime` applies DST adjustment on Win7+ regardless of `DSTMode` (parameter is currently `/*DSTMode*/` and unused). Determine if this causes download-side asymmetry.

Current `DateTimeToFileTime` net effect on Win7+:
```cpp
UnixTimeStamp += (IsDateInDST(DateTime) ? DaylightDiff : StandardDiff) - CurrentDaylightDiff;
```

This adjusts based on whether the file's date is in DST vs. current DST state. This is the same legacy compensation.

**Decision:** For this minimal fix, **do not change** `DateTimeToFileTime`. The primary user-reported issue is upload (local → remote). Changing `DateTimeToFileTime` affects all protocols' download paths and carries higher regression risk. Document the asymmetry in code comments for future maintenance.

**Logging:** Add a verbose comment in `DateTimeToFileTime` explaining the DST adjustment logic and why it is intentionally retained for backward compatibility with download paths.

- [x] Task 1.4: Verify `SynchronizeRemoteTimestamp` after fix
**File:** `src/core/Terminal.cpp`
**Lines:** ~7375-7389

**Check:** `SynchronizeRemoteTimestamp` calls `ConvertTimestampToUnix(ChecklistItem->FLocalLastWriteTime, GetDSTMode())` directly on a `FILETIME` value. After Task 1.1, this path will also produce correct UTC timestamps for `dstmWin` on Win7+.

**Verification:** Confirm that `FLocalLastWriteTime` is a `FILETIME` and that the conversion is the same pattern as `OpenLocalFile`. After the fix, synchronize-to-remote operations will correctly preserve local timestamps. **No code change expected** in `Terminal.cpp`.

- [x] Task 1.5: Document upload/download round-trip asymmetry as known limitation
**Scope:** `src/base/Common.cpp` (comments), plan documentation

**Analysis:** After fixing `ConvertTimestampToUnix` (upload) but intentionally not fixing `DateTimeToFileTime` (download), the round-trip is asymmetric:
  - Upload: `FILETIME` (UTC) → correct Unix timestamp (after fix)
  - Download: Unix timestamp → `DateTimeToFileTime` → still applies `(fileDST ? DaylightDiff : StandardDiff) - CurrentDaylightDiff`

**Impact:** A file uploaded during DST will download correctly only if the current DST state matches the file's creation DST state. If the states differ, the downloaded local timestamp may be off by 1 hour. This affects synchronize-compare operations where files are compared after download.

**Decision:** Document this as a known limitation in the inline comment added in Task 1.3. Fixing `DateTimeToFileTime` is out of scope for this bugfix because it affects all protocol download paths and requires broader regression testing. A follow-up issue may be opened if users report download-side DST problems.

### Phase 2: Verify Protocol-Specific Upload Paths

- [x] Task 2.1: Verify SCP upload uses correct timestamp
**File:** `src/core/ScpFileSystem.cpp`  
**Lines:** ~2175-2182

**Check:** After Task 1.1, `LocalFileHandle.MTime` will contain the correct UTC Unix timestamp. Confirm that the SCP `T` command format `T%lu 0 %lu 0` sends `MTime` and `ATime` directly.

**No code change expected** in `ScpFileSystem.cpp`. The fix is purely in the core conversion function. The SCP `T` command at line 2179 sends `LocalFileHandle.MTime` which now contains the correct UTC Unix timestamp.

- [x] Task 2.2: Verify SFTP upload timestamp path
**File:** `src/core/SftpFileSystem.cpp`  
**Lines:** ~4200-4210

**Check:** SFTP upload uses:
```cpp
const int64_t MTime = ConvertTimestampToUnix(DateTimeToFileTime(AFile->Modification, DSTMode), DSTMode);
```

With Task 1.1 fix, `ConvertTimestampToUnix` will no longer subtract DST for `dstmWin`. Verify that the double-conversion path (`DateTimeToFileTime` → `ConvertTimestampToUnix`) still produces reasonable results. Note that `DateTimeToFileTime` still has its own DST adjustment, so the round-trip is not perfectly symmetric, but the primary upload-from-local path (via `OpenLocalFile` → `ConvertTimestampToUnix`) is what the user reported.

**No code change expected** in `SftpFileSystem.cpp`.

- [x] Task 2.3: Check FTP / WebDAV / S3 upload paths
**Files:** `src/core/FtpFileSystem.cpp`, `src/core/WebDAVFileSystem.cpp`, `src/core/S3FileSystem.cpp`

**Check:** Search for uses of `ConvertTimestampToUnix`, `LocalFileHandle.MTime`, or `Modification` in upload paths. Determine if any of these protocols directly use `ConvertTimestampToUnix` on a raw `FILETIME` for upload timestamp setting.

**Expected:** FTP typically uses `MFMT` or site-specific commands. WebDAV and S3 use HTTP headers with `TDateTime` conversions. Most likely no direct `FILETIME`→Unix conversion in these protocols for upload. Confirm with search.

**Action:** If any direct `ConvertTimestampToUnix(FILETIME, dstmWin)` usage is found on Win7+ upload paths, the same fix (Task 1.1) automatically corrects it.

### Phase 3: Edge Cases and Validation
### Phase 3: Edge Cases and Validation

- [x] Task 3.1: Handle files created outside current DST period

**Rationale:** On pre-Win7 (`UsesDaylightHack() == true`), `ConvertTimestampToUnix` does **not** subtract DST for `dstmWin`. It only adds DST for `dstmUnix`/`dstmKeep`. Task 1.1 only affects the Win7+ path. Pre-Win7 behavior is unchanged.

**Verification:** Confirm that the `if (UsesDaylightHack())` branch is untouched by the fix.

- [x] Task 3.3: `ConvertTimestampToUnixSafe` behavior
**File:** `src/base/Common.cpp`  
**Lines:** ~2995-3009

**Check:** `ConvertTimestampToUnixSafe` delegates to `ConvertTimestampToUnix` for non-zero FILETIME. After Task 1.1, it will also produce correct UTC timestamps for `dstmWin` on Win7+. **No change needed.**

### Phase 4: Documentation

- [x] Task 4.1: Add inline documentation to `ConvertTimestampToUnix`
**File:** `src/base/Common.cpp`

Add a comment block explaining the DST mode behavior:
- `dstmWin` on pre-Win7: no adjustment (Windows already has DST bug in FILETIME)
- `dstmWin` on Win7+: no adjustment (FILETIME is pure UTC, bug is fixed)
- `dstmUnix`/`dstmKeep`: adjustments for legacy servers

- [x] Task 4.2: Update NetBox changelog / fix tracking
**File:** `.ai-factory/Github-Issues.md` (or equivalent project tracking file)

Add an entry for issue #391:
- Description: DST bug in SCP upload timestamps
- Root cause: `ConvertTimestampToUnix` subtracted DST offset for `dstmWin` on Win7+
- Fix: removed incorrect DST subtraction
- Verification: editor save + SCP upload produces correct remote timestamp during DST

---

## Commit Plan

Since this is a focused single-file core fix with verification and documentation tasks, a single commit is sufficient.

**Suggested commit message:**
```
fix(scp): correct DST handling in upload timestamps on Win7+

ConvertTimestampToUnix() incorrectly subtracted DaylightDifferenceSec
from the UTC Unix timestamp for dstmWin mode on Windows 7 and newer,
where FILETIME is already pure UTC and the legacy Windows DST bug is
fixed. This caused SCP uploads (especially editor "Upload after every
save") to set remote timestamps 1 hour behind during DST periods.

Remove the erroneous DST subtraction in the !UsesDaylightHack()
branch when DSTMode == dstmWin, aligning Win7+ behavior with the
pre-Win7 dstmWin path (which correctly did no adjustment).

Fixes #391
```

---

## Risk Assessment

| Risk | Level | Mitigation |
|---|---|---|
| Regression in pre-Win7 DST handling | Low | Task 1.1 only modifies `!UsesDaylightHack()` branch |
| Regression in `dstmUnix`/`dstmKeep` handling | Low | Task 1.1 only affects `dstmWin` path |
 Download-side timestamp asymmetry | Medium | Documented in Tasks 1.3 and 1.5; `DateTimeToFileTime` unchanged to limit scope |
| SFTP/FTP/WebDAV/S3 timestamp changes | Low | Task 2.2/2.3 verifies these use `DateTimeToFileTime` double-conversion or other mechanisms; `ConvertTimestampToUnix` fix improves consistency |
---

## Next Steps

1. Implement Task 1.1 (core fix in `ConvertTimestampToUnix`)
2. Run build verification (`build-x64.bat`)
3. Implement Tasks 1.2–1.5 (symmetry verification, `FileAge` audit, `DateTimeToFileTime` documentation, `SynchronizeRemoteTimestamp` check, asymmetry documentation)
4. Implement Tasks 2.1–2.3 (protocol path verification)
5. Implement Tasks 3.1–3.3 (edge case validation)
6. Implement Tasks 4.1–4.2 (documentation)
7. Build and verify zero warnings
8. Commit with suggested message
