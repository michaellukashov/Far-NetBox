# Phase 01 Code Review

**Scope:** Files changed in most recent commit (git diff HEAD~1)
**Depth:** standard
**Review Date:** 2026-04-30

## Files Reviewed

| File | Lines Changed | Type |
|---|---|---|
| src/core/S3FileSystem.cpp | 2 changed | C++ implementation |
| src/NetBox/FarPluginStrings.cpp | 4 added | Mapping table |
| src/base/MsgIDs.h | 2 added | Enum declaration |
| src/resource/TextsCore.h | 2 added | String ID constant |
| src/resource/TextsCore1.rc | 1 added | String resource |

---

## Findings

### MEDIUM: Duplicate mapping entry in FarPluginStrings.cpp

**Location:** `src/NetBox/FarPluginStrings.cpp`, lines 246 and 304
**Category:** Code Quality / Maintenance

`S3_UPLOAD_NEED_FILENAME` is registered twice in the `FarPluginStrings[]` array:

- Line 246: Correctly placed in the error-strings section (before `CORE_CONFIRMATION_STRINGS`).
- Line 304: Redundant entry immediately before `CORE_INFORMATION_STRINGS`.

The second instance is unnecessary and likely resulted from copy-pasting alongside `MISSING_TARGET_BUCKET`, which is also duplicated in the same two locations (lines 245 and 303). Duplicate entries bloat the lookup table, confuse future maintainers, and suggest the string belongs to two sections when it does not.

**Recommendation:** Remove the duplicate at line 304. While touching this file, also consider removing the pre-existing duplicate of `MISSING_TARGET_BUCKET` at line 303.

---

### LOW: Indentation regression in S3FileSystem.cpp

**Location:** `src/core/S3FileSystem.cpp`, lines 1920 and 2700
**Category:** Style / Consistency

Both changed `throw` lines were indented with 6 spaces, up from 4 spaces in the original code. The surrounding `if` blocks use 2-space indentation for the brace and 6 spaces for the body (the `FTerminal->LogEvent(...)` lines immediately above the throws already had this over-indentation). The project standard is 2 spaces per nesting level, meaning the body should be at 4 spaces.

**Recommendation:** Reduce indentation of the `throw` lines to 4 spaces, or ideally fix the entire `if` blocks to match the project standard.

---

### INFO: Pre-existing duplicate of MISSING_TARGET_BUCKET

**Location:** `src/NetBox/FarPluginStrings.cpp`, lines 245 and 303
**Category:** Technical Debt

`MISSING_TARGET_BUCKET` is already duplicated in the same two sections. The new change replicated this pattern for `S3_UPLOAD_NEED_FILENAME`. This is not a new bug introduced by the current change, but it is the root cause of the duplicate noted above.

**Recommendation:** Audit `FarPluginStrings.cpp` for other duplicate entries and remove them in a dedicated cleanup pass.

---

## Security Assessment

No security vulnerabilities identified in the changed code. The new error message uses `FMTLOAD` with a single `UnicodeString` argument (`DestBucketName` / `BucketName`). No format-string injection risk is present because the format string is a controlled resource string, and `UnicodeString` formatting in this codebase does not suffer from classic `printf` buffer-overflow issues.

---

## Summary

| Severity | Count | Description |
|---|---|---|
| MEDIUM | 1 | Duplicate `S3_UPLOAD_NEED_FILENAME` entry in mapping table |
| LOW | 1 | Indentation regression (adopted pre-existing over-indentation) |
| INFO | 1 | Pre-existing `MISSING_TARGET_BUCKET` duplicate (technical debt) |

The functional change is sound: replacing a generic "Specify target bucket" error with a specific message that includes the bucket name and guidance on required S3 path format. The two C++ call sites (`CopyFile` and `Source`) correctly use the new string via `FMTLOAD`.

**Actionable before merge:**
1. Remove the duplicate `{ S3_UPLOAD_NEED_FILENAME, MSG_S3_UPLOAD_NEED_FILENAME }` at line 304 of `FarPluginStrings.cpp`.
2. Correct indentation on lines 1920 and 2700 of `S3FileSystem.cpp` to 4 spaces (or standardize the entire block).
