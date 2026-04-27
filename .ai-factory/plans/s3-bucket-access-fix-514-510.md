# S3 Bucket Access Fix — Implementation Plan

**Created:** 2026-04-27T20:23:20Z
**Issues:** [#514](https://github.com/michaellukashov/Far-NetBox/issues/514), [#510](https://github.com/michaellukashov/Far-NetBox/issues/510)
**Status:** Ready for Implementation

## Settings

- **Testing:** No (skip tests)
- **Logging:** Verbose (detailed DEBUG logs for development)
- **Documentation:** Yes (mandatory docs checkpoint at completion)

---

## Problem Summary

NetBox S3 implementation has critical issues preventing bucket access:

### Issue #514 (Russian user report)
- **Symptom:** Can list buckets successfully, but cannot enter any bucket
- **Error:** Connection fails when trying to access bucket contents
- **Impact:** Complete inability to browse bucket contents after initial connection

### Issue #510 (English user report)
- **Symptoms:**
  1. Connects successfully but shows empty directory
  2. Can create directories from Far Manager, but they are not visible
  3. Cannot upload files — error: "Specify target bucket. Copying files to remote side failed."
  4. Latest version fails to connect with error: "Invalid argument to time encode [24:00:00.0000]. Connection failed."

### Root Cause Analysis

Based on code review of `src/core/S3FileSystem.cpp`:

1. **Path Parsing Issue** (`ParsePath` at line 1093):
   - Correctly splits path into bucket name and key
   - However, bucket context retrieval may fail for newly accessed buckets

2. **Bucket Context Retrieval** (`GetBucketContext` at line 1158):
   - Performs region auto-detection by attempting to list bucket with 1 item
   - If region is unknown, retries with detected region from error response
   - **Potential issue:** Retry logic may not handle all error cases properly
   - **Potential issue:** Endpoint redirection may fail for some S3-compatible services

3. **Directory Listing** (`ReadDirectoryInternal` at line 1616):
   - Calls `ParsePath` to extract bucket name and prefix
   - Calls `GetBucketContext` to get bucket context
   - Calls `DoListBucket` to list objects
   - **Potential issue:** Empty directory may indicate prefix handling problem

4. **Time Encoding Error** (Issue #510):
   - Error message: "Invalid argument to time encode [24:00:00.0000]"
   - Occurs in date/time parsing code (line 1527-1547)
   - **Root cause:** Invalid time value 24:00:00 (should be 00:00:00 or 23:59:59)
   - Likely caused by S3 API returning malformed timestamp

---

## Plan Refinement Report

**Analysis Date:** 2026-04-27
**Codebase Review:** Deep analysis of `src/core/S3FileSystem.cpp` (2988 lines)

### Key Findings from Codebase Analysis

1. **Existing Logging Infrastructure:**
   - `FTerminal->LogEvent()` is already used extensively (20+ calls)
   - Logging levels: `LogEvent(0, msg)` for info, `LogEvent(1, msg)` for verbose
   - Error logging happens in `LibS3ResponseCompleteCallback` (line 809)

2. **Error Handling Pattern:**
   - `CheckLibS3Error()` (line 876) centralizes error handling
   - Handles 10+ S3 status codes with specific error messages
   - Already logs error messages and details automatically

3. **Region Detection Flow:**
   - `GetBucketContext()` (line 1158) has retry logic for region detection
   - `HandleNonBucketStatus()` (line 1600) handles auth region updates
   - Region caching in `FRegions` map prevents repeated detection

4. **Critical Discovery - Potential Root Cause:**
   - Line 1683-1685: Infinite loop protection exists for `IsTruncated` with no keys
   - Line 1699-1700: Comment mentions empty bucket detection issue
   - **This suggests the "empty directory" bug may be related to bucket existence checks**

---

## I. Investigation & Diagnosis

### Task 1.1: Add Targeted Logging for Bucket Access
**File:** `src/core/S3FileSystem.cpp`
**Rationale:** Existing logging infrastructure is good, but needs specific additions for bucket access debugging.
**Note:** Line numbers are approximate and may shift after edits - reference function names as primary anchors.

Add logging at these critical points:

1. **In `ParsePath` (around line 1093):**
 ```cpp
 FTerminal->LogEvent(FORMAT("ParsePath: input='%s', bucket='%s', key='%s'", APath, BucketName, AKey));
 ``

2. **In `GetBucketContext` (line 1158) - at retry decision points:**
 ```cpp
 FTerminal->LogEvent(FORMAT("GetBucketContext: bucket='%s', region='%s', retry=%d (count=%d)", ABucketName, Region, Retry, RetryCount));
 ```

3. **In `ReadDirectoryInternal` (around line 1616) - before and after bucket listing:**
 ```cpp
 FTerminal->LogEvent(FORMAT("ReadDirectoryInternal: path='%s', bucket='%s', prefix='%s', maxKeys=%d", Path, BucketName, Prefix, AMaxKeys));
 // After DoListBucket:
 FTerminal->LogEvent(FORMAT("ListBucket result: keyCount=%d, truncated=%d, any=%d", Data.KeyCount, Data.IsTruncated, Data.Any));
 ```

4. **In `LibS3ListBucketCallback` (around line 1503) - log what's being added:**
 ```cpp
 FTerminal->LogEvent(1, FORMAT("ListBucket callback: contents=%d, prefixes=%d", ContentsCount, CommonPrefixesCount));
 ```
**Acceptance:** 
- Build succeeds with zero warnings
- Logs show bucket access flow without overwhelming output
- Uses existing `FTerminal->LogEvent()` pattern consistently

### Task 1.2: Reproduce and Capture Logs
**Prerequisites:** Task 1.1 complete

**Objective:** Reproduce both issues and capture detailed logs to confirm root cause hypothesis.
**Alternative:** If S3 test environment not available, rely on code review findings to proceed.

**Steps:**

1. **Set up test environment:**
 - AWS S3 account with test bucket
 - Or S3-compatible service (Minio, Wasabi)
 - Configure NetBox with test credentials

2. **Reproduce Issue #514 (Cannot enter bucket):**
 - Connect to S3 service
 - List buckets (should succeed)
 - Attempt to enter a bucket
 - Capture error message and full log
 - Note: Check if `Data.Any` is false in logs

3. **Reproduce Issue #510 (Empty directory + upload fails):**
 - Connect to S3 service
 - Enter a bucket
 - Verify directory appears empty
 - Attempt to create directory
 - Attempt to upload file
 - Capture all error messages and logs

4. **Analyze logs:**
 - Identify exact failure point from Task 1.1 logging
 - Confirm whether empty bucket detection (line 1704) is the issue
 - Check if `BucketName.IsEmpty()` in upload path (line ~2622)

5. **If reproduction blocked:**
 - Use code review findings as primary evidence
 - Proceed with fixes based on analysis in Task 2.x
 - Add TODO to validate with real S3 testing later
### Task 1.3: Analyze Time Encoding Error
**File:** `src/core/S3FileSystem.cpp` (lines 1527-1547)
**Prerequisites:** None (can run in parallel with Task 1.2)

**Objective:** Understand the time encoding error and verify the fix approach.

**Investigation:**

1. **Verify ISO 8601 standard:**
   - Confirm that 24:00:00 is valid in ISO 8601 (represents midnight of next day)
   - Check if S3 API documentation mentions this format

2. **Test current parsing code:**
   - Create test cases with various timestamps:
     - Normal: `2024-01-15T12:30:45`
     - Midnight (24:00): `2024-01-15T24:00:00`
     - Midnight (00:00): `2024-01-16T00:00:00`
     - Edge case: `2024-12-31T24:00:00` (year rollover)
   - Verify which ones cause `EncodeTimeVerbose()` to fail

3. **Review VCL TDateTime behavior:**
   - Check if `EncodeDateVerbose()` + 1 correctly handles month/year rollover
   - Verify `DecodeDate()` works correctly after date increment
4. **Validate fix approach:**
 - Confirm that normalizing 24:00:00 to next day 00:00:00 is correct
 - Check for any other invalid hour values (negative, >24)
 - **NOTE:** TDateTime handles month/year rollover, but test edge cases: month-end, year-end, leap year

**Acceptance:**
- Time encoding error root cause confirmed
- Fix approach validated
- Test cases documented
- Edge cases identified (month/year boundaries, leap years)
## II. Core Fixes

### Task 2.1: Fix Time Encoding Error
**File:** `src/core/S3FileSystem.cpp` (lines 1527-1547)  
**Prerequisites:** Task 1.3 complete

**Issue:** S3 API can return `24:00:00` for midnight (ISO 8601 allows this), but `EncodeTimeVerbose()` expects 0-23 hours.

**Fix:** Add validation and normalization:

```cpp
const int32_t Filled =
  sscanf(Content->lastModifiedStr, ISO8601_FORMAT, &Year, &Month, &Day, &Hour, &Min, &Sec);
if (Filled == 6)
{
  // Normalize 24:00:00 to next day 00:00:00 (ISO 8601 allows 24:00:00 for midnight)
  if (Hour == 24)
  {
    Hour = 0;
    // Increment day (TDateTime handles month/year rollover automatically)
    TDateTime Date = EncodeDateVerbose(static_cast<uint16_t>(Year), static_cast<uint16_t>(Month), static_cast<uint16_t>(Day));
    Date = Date + 1; // Add one day
    uint16_t NewYear, NewMonth, NewDay;
    DecodeDate(Date, NewYear, NewMonth, NewDay);
    Year = NewYear;
    Month = NewMonth;
    Day = NewDay;
  }
  
  // Validate hour range after normalization
  if (Hour >= 0 && Hour < 24)
  {
    TDateTime Modification =
      EncodeDateVerbose(static_cast<uint16_t>(Year), static_cast<uint16_t>(Month), static_cast<uint16_t>(Day)) +
      EncodeTimeVerbose(static_cast<uint16_t>(Hour), static_cast<uint16_t>(Min), static_cast<uint16_t>(Sec), 0);
    File->Modification = ConvertTimestampFromUTC(Modification);
    File->ModificationFmt = mfFull;
  }
  else
  {
    // Invalid hour value - log and skip timestamp
    FTerminal->LogEvent(FORMAT("Invalid hour value %d in timestamp for '%s'", Hour, FileName));
    File->ModificationFmt = mfNone;
  }
}
else
{
  File->ModificationFmt = mfNone;
}
```

**Acceptance:** 
- Build succeeds with zero warnings
- Time encoding error no longer occurs
- Timestamps with 24:00:00 are correctly normalized
- Invalid timestamps are logged and handled gracefully

### Task 2.2: Fix Bucket Context Retry Logic
**File:** `src/core/S3FileSystem.cpp` (`GetBucketContext` at line 1158)
**Prerequisites:** Task 1.2 complete (or use code analysis if reproduction blocked)

**Current State:** Retry logic exists but has NO retry limit - potential infinite loop risk.

**Critical Fix - Add retry counter:**

Add before line 1162 (`bool First = true;`):
```cpp
int32_t RetryCount = 0;
const int32_t MaxRetries = 3;
```

Add inside the retry decision logic (after line 1247 where `Retry = true;`):
```cpp
if (Retry)
{
    RetryCount++;
    if (RetryCount >= MaxRetries)
    {
        FTerminal->LogEvent(FORMAT("Max retries (%d) reached for bucket '%s' (region='%s', endpoint='%s'), aborting", 
            MaxRetries, ABucketName, Region, HostName));
        Retry = false;
    }
}
```

**Additional improvements:**

1. **Log all retry attempts:**
 - Already partially done, ensure all retry paths log RetryCount

2. **Improve error handling for non-AWS S3 services:**
 - Some S3-compatible services (Minio, Wasabi) may not return proper region headers
 - Add fallback to use `FAuthRegion` if region detection fails

**Acceptance:**
- Build succeeds with zero warnings
- Retry limit prevents infinite loops
- Bucket context retrieval succeeds for AWS and S3-compatible services
- All retry attempts are logged with retry count for debugging
### Task 2.3: Fix Empty Directory Issue
**File:** `src/core/S3FileSystem.cpp` (`ReadDirectoryInternal` at line 1616)  
**Prerequisites:** Task 2.2 complete (can run in parallel with Task 1.2)
**Critical Discovery:** Lines 1704-1715 show the actual empty bucket logic:
```cpp
if (Prefix.IsEmpty() || Data.Any)
{
  FileList->AddFile(new TRemoteParentDirectory(FTerminal));
}
else
{
  // When called from DoReadFile (FileName is set), leaving error handling to the caller.
  if (AFileName.IsEmpty())
  {
    throw Exception(FMTLOAD(FILE_NOT_EXISTS, APath));
  }
}
```

**Root Cause Hypothesis:** The issue is NOT with empty buckets, but with **GetFolderKey behavior**.

**Actual Logic:**
- `Prefix.IsEmpty()` → Listing bucket root → Always succeeds (adds parent dir)
- `Prefix.IsEmpty() == false && Data.Any == false` → Prefix doesn't exist → Throws error
- `Prefix.IsEmpty() == false && Data.Any == true` → Prefix exists → Succeeds

**GetFolderKey Implementation (line 1088):**
```cpp
UnicodeString TS3FileSystem::GetFolderKey(const UnicodeString & AKey) const
{
  return AKey + "/";
}
```

**Real Problem:**
When entering a bucket for the first time at `/bucket-name/`, the code:
1. Parses path: BucketName="bucket-name", Prefix=""
2. Line 1662: `if (!Prefix.IsEmpty())` → false, so GetFolderKey is NOT called
3. Line 1666: `Prefix += AFileName` → Prefix remains ""
4. Line 1704: `if (Prefix.IsEmpty() || Data.Any)` → true (Prefix is empty)
5. Should succeed and add parent directory

**Alternative Hypothesis:**
The bug may be in `ParsePath` (line 1093) or path normalization before calling `ReadDirectoryInternal`.

**Investigation needed:**

1. **Check ParsePath behavior with trailing slashes:**
   - Test: `/bucket-name` → BucketName=?, Prefix=?
   - Test: `/bucket-name/` → BucketName=?, Prefix=?
   - Test: `/bucket-name/dir` → BucketName=?, Prefix=?
   - Test: `/bucket-name/dir/` → BucketName=?, Prefix=?
   - **Critical:** Verify Prefix is truly empty for bucket root

2. **Check AbsolutePath normalization:**
   - Line 1619: `AbsolutePath(APath, false)` is called
   - Check if this adds/removes trailing slashes
   - Verify `UnixExcludeTrailingBackslash` behavior

3. **Check GetBucketContext with empty prefix:**
   - Line 1667: `GetBucketContext(BucketName, Prefix)` with Prefix=""
   - Verify this doesn't fail or return invalid context
   - Check region detection with empty prefix

4. **Check DoListBucket with empty prefix:**
   - Line 1676: `DoListBucket(Prefix, FileList, AMaxKeys, BucketContext, Data)`
   - Verify empty prefix is handled correctly by libs3
   - Check if `Data.Any` is set correctly for empty buckets

5. **Check file filtering:**
   - `Terminal->IsValidFile` may filter out files (lines 1555, 1575)
   - Empty filenames are skipped (lines 1520, 1568) - correct behavior

**Acceptance:**
- Build succeeds with zero warnings
- Empty buckets display correctly (no error)
- Bucket root can be entered successfully
- Directory listing shows all objects in bucket
- Subdirectories are correctly displayed
- Pagination works for large buckets (>1000 objects)

### Task 2.4: Fix File Upload Issue
**File:** `src/core/S3FileSystem.cpp` (`Source` method at line 2577)
**Prerequisites:** Task 2.3 complete

**Error:** "Specify target bucket. Copying files to remote side failed."
**Error Source:** `MISSING_TARGET_BUCKET` string ID (TextsCore1.rc line 241, MsgIDs.h line 1087)

**Root Cause:** The error message "Specify target bucket" is **misleading**. The check at line 2622-2625 validates `Key.IsEmpty()`, not `BucketName.IsEmpty()`. The issue is that:
- User IS in a bucket (bucket name is present)
- But `Key` is empty because user didn't specify a filename
- S3 requires an object key (filename) for uploads
**Fix - Improve error message and add logging:**

```cpp
if (Key.IsEmpty())
{
    // Log the actual issue before throwing
    FTerminal->LogEvent(FORMAT("Upload rejected: Key is empty. Bucket='%s', path='%s'", BucketName, DestFullName));
    // Use formatted message to explain S3 requires filename
    throw Exception(FMTLOAD(S3_UPLOAD_NEED_FILENAME, BucketName));
    // Alternative if new string not available:
    // throw Exception(FORMAT("Cannot upload to bucket root '%s': S3 requires an object name (filename).\nUse format: /bucket-name/file.txt", BucketName));
}
FTerminal->LogEvent(FORMAT("Upload target validated: bucket='%s', key='%s'", BucketName, Key));
```

**Note on string resources:** If adding `S3_UPLOAD_NEED_FILENAME` to TextsCore1.rc is out of scope,
use the FORMAT alternative with clear inline message.

**Acceptance:**
- Build succeeds with zero warnings
- File upload succeeds to bucket root with filename (`/bucket-name/file.txt`)
- File upload succeeds to subdirectories (`/bucket-name/dir/file.txt`)
- Clear error message if target path is invalid (no filename)
- Upload progress is displayed correctly
---

## III. Testing & Validation

### Task 3.1: Manual Testing
**Prerequisites:** All Task 2.x complete

Test scenarios:

1. **Basic Connection:**
   - Connect to S3 service
   - List buckets
   - Enter a bucket
   - Verify directory contents are shown

2. **Directory Navigation:**
   - Navigate into subdirectories
   - Navigate back to parent
   - Navigate to bucket root

3. **File Operations:**
   - Upload file to bucket root
   - Upload file to subdirectory
   - Download file from bucket
   - Delete file from bucket

4. **Directory Operations:**
   - Create directory in bucket
   - Verify directory is visible
   - Delete directory from bucket

5. **Edge Cases:**
   - Empty bucket
   - Bucket with many objects (pagination)
   - Bucket with special characters in names
   - Objects with timestamps at midnight

**Acceptance:** All test scenarios pass

### Task 3.2: Regression Testing
**Prerequisites:** Task 3.1 complete

Verify existing functionality still works:

1. **Other Protocols:**
   - SFTP connection and operations
   - FTP connection and operations
   - WebDAV connection and operations

2. **S3 Features:**
   - Region auto-detection
   - Endpoint redirection
   - Google Cloud compatibility
   - Requester pays buckets

**Acceptance:** No regressions in existing functionality

---

## IV. Documentation

### Task 4.1: Update Logs and Comments
**Prerequisites:** All fixes complete

1. Add comments explaining the fixes
2. Document known limitations
3. Update any relevant documentation files

**Acceptance:** Code is well-documented

---

## Dependencies

```
Task 1.1 (Logging)
  ↓
Task 1.2 (Reproduce) ← Task 1.3 (Time Analysis)
  ↓
Task 2.1 (Time Fix) ← Task 1.3
  ↓
Task 2.2 (Bucket Context) ← Task 1.2
  ↓
Task 2.3 (Empty Directory Fix) ← Task 2.2  [CRITICAL - likely root cause]
  ↓
Task 2.4 (File Upload) ← Task 2.3
  ↓
Task 3.1 (Manual Testing)
  ↓
Task 3.2 (Regression Testing)
  ↓
Task 4.1 (Documentation)
```

**Note:** Task 2.3 is now identified as the most critical fix - the empty directory issue is likely the root cause of both #514 and #510 symptoms.
---

## Risk Assessment

### High Risk
- **Time encoding fix:** May affect all S3 operations if not done correctly
- **Bucket context retry logic:** May cause infinite loops or connection failures

### Medium Risk
- **Directory listing:** May break pagination or filtering
- **File upload:** May affect other file operations

### Low Risk
- **Logging additions:** Should not affect functionality
- **Documentation updates:** No code impact

---

## Success Criteria

1. ✅ Users can list buckets successfully
2. ✅ Users can enter buckets and see contents
3. ✅ Users can navigate subdirectories
4. ✅ Users can upload files to buckets
5. ✅ Users can download files from buckets
6. ✅ Users can create and delete directories
7. ✅ No time encoding errors occur
8. ✅ All existing S3 features continue to work
9. ✅ Build succeeds with zero warnings
10. ✅ Code follows NetBox conventions

---

## Notes

- This plan assumes access to an S3 test environment (AWS or compatible)
- Logging uses existing `FTerminal->LogEvent()` infrastructure
- Each fix should be tested independently before moving to the next
- Consider adding unit tests for time parsing and path parsing functions

## Improvements Applied

**Based on deep codebase analysis:**

1. ✅ **Leveraged existing logging infrastructure** - No need to reinvent logging, use `FTerminal->LogEvent()`
2. ✅ **Identified likely root cause** - Empty bucket handling (lines 1699-1700) is suspicious
3. ✅ **Added retry limit** - Prevent infinite loops in `GetBucketContext`
4. ✅ **Improved time encoding fix** - Added validation and better error handling
5. ✅ **Better error messages** - Added specific guidance for upload path format
6. ✅ **Focused investigation** - Task 2.3 now targets the specific code area with known issues

**Risk reduction:**
- Logging additions use existing patterns (low risk)
- Time encoding fix includes validation (reduced risk)
- Retry limit prevents infinite loops (reduced risk)
- Empty bucket handling is now the primary focus (addresses root cause)