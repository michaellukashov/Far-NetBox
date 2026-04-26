# SFTP Directory Size Stack Overflow Exploration — Issue #497

**Created:** 2026-04-26
**Related Issue:** GitHub #497 - SFTP NetBox.dll Exception 0xC00000FD STATUS_STACK_OVERFLOW

---

## Summary

Exploration of stack overflow bug when pressing F3 on SFTP directory with ~98,648 files. Root cause: recursive directory traversal for size calculation lacks symlink cycle detection.

---

## Key Findings

### Call Chain (Verified)
```
F3 key → foCalculateSize → CalculateFilesSize → ProcessFiles
  → DoCalculateFileSize (at 4833)
    → ProcessDirectory (at 4061)
      → CalculateFileSize callback (for each file)
        → if directory: DoCalculateDirectorySize (at 4945)
          → ProcessDirectory (recursive) → ... infinite recursion
```

### Critical Location
- **File:** `src/core/Terminal.cpp`
- **Function:** `CalculateFileSize` callback at line 4893
- **Issue:** No cycle check before recursive call at line 4913

### Existing Pattern (Working)
- `FilesFind` at lines 7305-7324 uses `TLoopDetector.IsUnvisitedDirectory()`
- Uses `base::AbsolutePath()` for symlink resolution
- Correctly logs and skips already-visited directories

### Root Cause Analysis
- `TCalculateSizeParams` lacks visited-directory tracking
- `CanRecurseToDirectory()` only checks session settings, not cycles
- No stack overflow protection like `FilesFind` has

### Symlink Target Types
| Type | Example | How Detected |
|------|---------|--------------|
| Self | `link -> .` | Resolves to parent dir |
| Parent | `link -> ..` | Resolves to grandparent |
| Relative | `link -> subdir` | Resolves to subdirectory |
| Absolute | `link -> /path` | Uses absolute path |

---

## Code Locations

| Location | Function | Line | Purpose |
|----------|----------|------|---------|
| Terminal.cpp | ProcessDirectory | 4061 | Lists files, calls callback |
| Terminal.cpp | CalculateFileSize | 4862 | Size callback |
| Terminal.cpp | CalculateFileSize | 4893 | Directory processing |
| Terminal.cpp | DoCalculateDirectorySize | 4945 | Recursive entry |
| Terminal.cpp | CalculateFilesSize | 4987 | Public API |
| Terminal.cpp | FileFind | 7280 | Search callback |
| Terminal.cpp | FilesFind | 7355 | Search with loop detection |
| Terminal.cpp | CanRecurseToDirectory | 9165 | Symlink check |

---

## Implementation Approach

The fix should mirror `FilesFind` pattern:

1. Add `TStringList * VisitedDirs` to `TCalculateSizeParams` in Terminal.h
2. Before calling `DoCalculateDirectorySize` at line 4913:
   - Compute `RealDirectory` (join path for regular dirs, absolute path for symlinks)
   - Check `VisitedDirs->IndexOf(RealDirectory)`
   - If found: log cycle, skip recursion
   - If not found: add to `VisitedDirs`, proceed
3. Initialize `VisitedDirs` in `CalculateFilesSize` (line 4987)

---

## References

- Original GitHub Issue: https://github.com/michaellukashov/Far-NetBox/issues/497
- Existing LoopDetector: Terminal.cpp lines 47-72
- Working pattern: Terminal.cpp lines 7307-7324 (FilesFind)
- Symlink handling: RemoteFiles.cpp lines 1550-1616 (FindLinkedFile)