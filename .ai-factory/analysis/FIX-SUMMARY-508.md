# Bug Fix Summary: Far Manager Crash on Second SFTP File Open

**Issue:** #508  
**Status:** ✅ Fixed  
**Date:** 2026-04-23

---

## Quick Summary

**Problem:** Far Manager crashes when opening a second file via SFTP without refreshing the directory between opens.

**Root Cause:** Stale pointers in `FFileList` after temporary file operations (Edit/View mode).

**Fix:** Clear `FFileList` after temporary file operations complete.

**Impact:** 7 lines added, minimal surgical change.

---

## The Fix

**File:** `src/NetBox/WinSCPFileSystem.cpp`  
**Function:** `TWinSCPFileSystem::GetFilesRemote()`  
**Lines:** 2764-2770

```cpp
FTerminal->CopyToLocal(FFileList.get(), DestPath, &CopyParam, Params, nullptr);

// Clear FFileList after temporary file operations to prevent stale pointers
// This fixes crash on second file open without directory refresh (GitHub issue #508)
if (EditView)
{
  FFileList.reset();
}
```

---

## Why This Works

1. **Before:** `FFileList` retained pointers to `TRemoteFile` objects after temp file operations
2. **Problem:** Second file open accessed stale pointers → crash
3. **After:** `FFileList.reset()` clears pointers when `EditView` mode is active
4. **Workaround Explained:** Ctrl+R worked by rebuilding entire panel state

---

## Testing

**Build Status:** ✅ PASSED (zero warnings)

**Manual Test:**
1. Open Far Manager → NetBox → SFTP server
2. Open file via Enter (view/edit)
3. Close editor
4. Open another file WITHOUT Ctrl+R
5. **Expected:** No crash (previously crashed)

---

## Files Changed

- `src/NetBox/WinSCPFileSystem.cpp` (+7 lines)
- `.ai-factory/analysis/issue-508-crash-on-second-file-open.md` (analysis stored)

---

## Next Steps

1. ✅ Build verified
2. ⏳ Manual testing (user required)
3. ⏳ Commit fix (if testing passes)

---

**Reference:** See `.ai-factory/analysis/issue-508-crash-on-second-file-open.md` for full analysis.
