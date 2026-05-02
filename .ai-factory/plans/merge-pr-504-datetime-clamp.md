# Merge PR #504: DateTimeToTimeStamp Milliseconds Clamp (#390)

**Source:** [PR #504](https://github.com/michaellukashov/Far-NetBox/pull/504)  
**Issue:** [GitHub #390](https://github.com/michaellukashov/Far-NetBox/issues/390)  
**Created:** 2026-05-02  
**Mode:** Fast  

## Settings

| Setting | Value |
|---------|-------|
| **Testing** | No |
| **Logging** | Verbose |
| **Docs** | Yes |
| **Branch** | `lmv/dev` (current) |

## Problem

Certificate timestamps (and other TDateTime values) get internally converted to float representation and later converted back to integral milliseconds with rounding. When the original timestamp is 23:59:59, the rounding can produce an illegal number of milliseconds (86400000 = MSecsPerDay), which exceeds the valid range (0..86399999).

## Solution

Clamp `Result.Time` to `MSecsPerDay - 1` (86399999) in `DateTimeToTimeStamp()` after the rounding calculation.

## Changes

### File: `src/base/Sysutils.cpp`

**Location:** `DateTimeToTimeStamp()` function (line ~496)

**Add after line 501** (`Result.Time = nb::ToInt32(fractpart * MSecsPerDay + 0.5);`):

```cpp
  // Due to conversions from int to float and back to int (with rounding) we might end up with a value
  // that exceeds the maximum possible number of milliseconds in a day (MSecsPerDay - 1).
  // Clamp the value to the maximum allowed.
  if (Result.Time >= MSecsPerDay)
  {
    Result.Time = MSecsPerDay - 1;
  }
```

## Tasks

### Phase 1: Apply Fix
- [x] **Task 1:** Apply clamping logic and comment to `DateTimeToTimeStamp()` in `src/base/Sysutils.cpp`
  - Verify exact line numbers match current file state
  - Preserve existing code style (Allman braces, 2-space indent)

### Phase 2: Build Verification
- [x] **Task 2:** Run `build-x64.bat` to verify zero warnings
  - MSVC W4 must pass clean
  - Confirm `Far3_x64/Plugins/NetBox/NetBox.dll` is produced

### Phase 3: Documentation
- [x] **Task 3:** Update `ChangeLog` with fix entry
  - 2-3 line description referencing issue #390 and PR #504
  - Link to GitHub issue

- [x] **Task 4:** Update `.ai-factory/Github-Issues.md`
  - Mark #390 as FIXED (if present in tracker)

### Phase 4: Commit
- [x] **Task 5:** Commit with conventional message
  - Type: `fix`
  - Scope: `base`
  - Reference PR #504 and issue #390

## Commit Message

```
fix(base): clamp DateTimeToTimeStamp milliseconds to prevent overflow

Due to float-to-int conversion with rounding, timestamps like 23:59:59
can produce 86400000 ms (MSecsPerDay), exceeding the valid range.
Clamp Result.Time to MSecsPerDay - 1 (86399999).

Fixes #390
Refs: PR #504
```

## Notes

- **Single-commit PR** — commit `5c4930d2` by @ArkBrjazovski
- **SonarQube Quality Gate:** Passed (0 new issues)
- **No test changes** in original PR
- **Impact:** All callers of `DateTimeToTimeStamp()` benefit (certificate parsing, time formatting, etc.)
