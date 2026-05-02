# Issue #391 — Daylight Saving Time Bug in SCP Upload Timestamps

**GitHub Issue:** [michaellukashov/Far-NetBox#391](https://github.com/michaellukashov/Far-NetBox/issues/391)  
**Upstream Reference:** [FarGroup/Far-NetBox#35](https://github.com/FarGroup/Far-NetBox/issues/35) by @viccpp  
**Exploration Date:** 2026-05-02

---

## Problem Statement

File saved from the Far Manager editor via SCP protocol with "Upload after every save" enabled receives a remote timestamp that is **exactly 1 hour behind** the actual local time during Daylight Saving Time (DST) periods. Both client and server are in the same timezone. Workaround: switch DST mode from default `dstmWin` to `dstmUnix`.

---

## Root Cause

`ConvertTimestampToUnix()` in `src/base/Common.cpp` (line ~2918) applies an **incorrect DST bias correction** when converting a Windows `FILETIME` to a Unix timestamp on modern Windows (Windows 7 and newer, where `UsesDaylightHack()` returns `false`).

### Key Facts

- Windows `FILETIME` is **always UTC** by definition.
- Unix timestamps are **always UTC** by definition.
- The conversion should be pure arithmetic: `Unix = (FILETIME - epoch) / 10000000`.
- On Win7+ with default `dstmWin` mode, `ConvertTimestampToUnix` **subtracts** `DaylightDifferenceSec` (typically 3600) from the UTC result when the file's local date falls within DST.
- This subtraction is a legacy compensation for the pre-Windows 7 DST display bug, which **no longer exists** on Win7+.
- The bug is most visible in **SCP** because SCP uploads use `LocalFileHandle.MTime` (already corrupted) directly in the SCP `T` command. SFTP performs a double conversion (`DateTimeToFileTime` → `ConvertTimestampToUnix`) which partially masks the error.

---

## Affected Code Path (Editor Save → SCP Upload)

1. `ProcessEditorEvent()` detects `EE_SAVE` → sets `FEditorPendingSave = true`
2. `UploadFromEditor()` → `OpenLocalFile(fileName, GENERIC_READ, LocalFileHandle)`
3. `OpenLocalFile()` calls `GetLocalFileTime()` → `ConvertTimestampToUnix(MTime, GetSessionData()->GetDSTMode())`
4. **BUG:** With `dstmWin` on Win7+, `ConvertTimestampToUnix` subtracts 3600 seconds if DST is active
5. `SCPSource()` sends corrupted `LocalFileHandle.MTime` via SCP `T` command:
   ```cpp
   Buf = FORMAT("T%lu 0 %lu 0", nb::ToUInt32(LocalFileHandle.MTime), nb::ToUInt32(LocalFileHandle.ATime));
   ```

---

## Related Functions and Files

| Function | File | Role |
|---|---|---|
| `ConvertTimestampToUnix()` | `src/base/Common.cpp:2918` | **Root cause** — subtracts DST for `dstmWin` on Win7+ |
| `UnixToDateTime()` | `src/base/Common.cpp:2638` | Reverse conversion; subtracts `BaseDifference` only for `dstmWin` on Win7+ |
| `DateTimeToFileTime()` | `src/base/Common.cpp:2856` | Applies `(fileDST - currentDST)` adjustment on Win7+ regardless of DST mode |
| `GetLocalFileTime()` | `src/core/Terminal.cpp:6028` | Reads FILETIME and converts via `ConvertTimestampToUnix` |
| `OpenLocalFile()` | `src/core/Terminal.cpp:5982` | Populates `LocalFileHandle.MTime` using `ConvertTimestampToUnix` |
| `SCPSource()` | `src/core/ScpFileSystem.cpp:2065` | Sends `LocalFileHandle.MTime` via SCP `T` command |
| `ProcessEditorEvent()` | `src/NetBox/WinSCPFileSystem.cpp:4479` | Detects editor save event |
| `UploadFromEditor()` | `src/NetBox/WinSCPFileSystem.cpp:4216` | Triggers upload after editor save |
| `TDSTMode` enum | `src/base/Common.h:286` | `dstmWin = 0`, `dstmUnix = 1`, `dstmKeep = 2` |
| `UsesDaylightHack()` | `src/base/Common.cpp:2633` | Returns `!IsWin7()` — true on pre-Win7, false on Win7+ |

---

## DST Mode Behavior Analysis

### `ConvertTimestampToUnix(FILETIME, DSTMode)` on Win7+ (`!UsesDaylightHack()`)

| DSTMode | Current Behavior | Expected Behavior |
|---|---|---|
| `dstmWin` | **Subtracts** `DaylightDifferenceSec` if file date in DST | **No adjustment** — FILETIME is already pure UTC |
| `dstmUnix` | No adjustment | No adjustment (correct) |
| `dstmKeep` | No adjustment | No adjustment (correct) |

### `UnixToDateTime(timestamp, DSTMode)` on Win7+ (`!DaylightHack`)

| DSTMode | Current Behavior | Correct? |
|---|---|---|
| `dstmWin` | Subtracts `BaseDifference` only (timezone offset) | **Yes** — consistent with pure UTC conversion |
| `dstmUnix` | Subtracts `BaseDifference` + `DSTDifferenceForTime` | **Legacy** — compensates for server-side DST bug |
| `dstmKeep` | Subtracts `BaseDifference` + `DSTDifferenceForTime` | **Legacy** — compensates for server-side DST bug |

### Why the Workaround Works

Setting DST mode to `dstmUnix` on Win7+ makes `ConvertTimestampToUnix` skip the DST subtraction (the `else` branch only triggers for `dstmWin`), producing a correct UTC timestamp. However, `dstmUnix` is documented for legacy Windows servers, and users expect the default `dstmWin` to "just work" for modern systems.

---

## WinSCP Upstream Comparison

The upstream WinSCP codebase (Pascal/Object Pascal) has the same `ConvertTimestampToUnix` logic structure. The bug pattern is inherited from WinSCP's original design where `dstmWin` mode was intended to compensate for pre-Win7 FILETIME behavior. On Win7+, the compensation is no longer needed but was not removed.

WinSCP Bug 1974 (2021) addressed a related issue: "DST start or end causes edited/opened files to be uploaded" — this was fixed by using `TTimeZone::Local->ToUniversalTime()` for editor timestamp comparisons instead of `FileAge()`, which is a different but related DST surface area.

---

## Proposed Fix

Remove the erroneous DST subtraction in `ConvertTimestampToUnix()` for the `!UsesDaylightHack()` (Win7+) branch when `DSTMode == dstmWin`.

The `dstmWin` mode on Win7+ should perform **pure UTC arithmetic conversion** with no DST adjustment, matching the pre-Win7 `dstmWin` behavior (which also did no adjustment because Windows itself had the bug).

### Minimal Fix Scope

- **Modify:** `src/base/Common.cpp` — `ConvertTimestampToUnix()` only
- **Verify:** `UnixToDateTime()` symmetry (already correct, no change needed)
- **Document:** `DateTimeToFileTime()` asymmetry (intentionally unchanged for download path compatibility)
- **Verify:** SCP upload path automatically corrects (uses `ConvertTimestampToUnix`)
- **Verify:** SFTP/FTP/WebDAV/S3 paths (double-conversion or other mechanisms; fix improves consistency)

### Regression Risks

| Risk | Level | Mitigation |
|---|---|---|
| Pre-Win7 DST handling | Low | Fix only modifies `!UsesDaylightHack()` branch |
| `dstmUnix`/`dstmKeep` handling | Low | Fix only affects `dstmWin` path |
| Download-side asymmetry | Medium | `DateTimeToFileTime` intentionally unchanged |
| Cross-protocol consistency | Low | Fix improves consistency for all protocols using direct FILETIME→Unix conversion |

---

## Related Plan

See [implementation plan](../plans/issue-391-daylight-saving-time-dst-bug.md) for detailed task breakdown.

---

## External Resources

- [WinSCP FAQ: Why is the timestamp of uploaded/downloaded file different?](https://winscp.net/eng/docs/faq_timestamp)
- [WinSCP Docs: Timestamps](https://winscp.net/eng/docs/timestamp)
- [WinSCP Docs: Environment Page — DST](https://winscp.net/eng/docs/ui_login_environment#dst)
- [WinSCP Commit be1997b: Bug 1974 — DST start or end causes edited/opened files to be uploaded](https://github.com/winscp/winscp/commit/be1997b579a80c7e12125c4d9d83f3682b629b6b)
