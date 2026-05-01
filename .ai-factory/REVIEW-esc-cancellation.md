---
status: issues_found
files_reviewed: 8
depth: standard
critical: 0
warning: 4
info: 2
total: 6
review_date: 2026-05-02
reviewed_by: claude
---

# Code Review: Issue #511 — SCP Esc Cancellation Fix

## Scope

8 source files modified across 5 fix layers (current committed state on `lmv/dev`):

| File | Review Focus |
|------|-------------|
| `src/NetBox/FarPlugin.cpp` | `CheckForEsc()` / `FlushEscBuffer()` console buffer rewrite |
| `src/NetBox/WinSCPFileSystem.cpp` | `CancelConfiguration` qaCancel fix, reentrancy guards, `EAbort` handling |
| `src/NetBox/WinSCPFileSystem.h` | `FInShowOperationProgress`, `FInCancelDialog` members |
| `src/core/ScpFileSystem.cpp` | csCancel parity, `ReadCommandOutput` guard, Layer 5 reconnect |
| `src/core/ScpFileSystem.h` | `FNeedsSessionReset`, `FLastDirectory` members |
| `src/core/Terminal.cpp` | `EAbort("")` + cancel-state logic in catch blocks |
| `src/core/SecureShell.h` | `ClearPending()`, `DrainSocket()` declarations |
| `src/core/SecureShell.cpp` | `ClearPending()`, `DrainSocket()`; `ReceiveLine` hex logging |

---

## Findings

### WR-001 — Misleading comment in __finally describes old architecture

- **File:** `src/core/ScpFileSystem.cpp`
- **Line:** 2561
- **Severity:** Warning

The comment says "Flag for SendCommand to Close()+Open() before next cd/ls." but the primary handler is now in `ChangeDirectory()` (Layer 5j). `SendCommand` still has the fallback reconnect block, but the comment implies it's the only consumer.

```cpp
// Current (misleading):
// Flag for SendCommand to Close()+Open() before next cd/ls.

// Should be:
// Flag for ChangeDirectory (primary) and SendCommand (fallback) to reconnect.
```

**Recommendation:** Update the comment to reflect the dual-consumer architecture.

---

### WR-002 — DrainSocket() is dead code but exported in header

- **File:** `src/core/SecureShell.h` (line ~180), `src/core/SecureShell.cpp` (line ~1364)
- **Severity:** Warning

`DrainSocket()` was added during Layer 5b experimentation but never worked due to edge-triggered `EventSelectLoop` limitations. It remains in the public API of `TSecureShell` but is never called from production code.

```cpp
void TSecureShell::DrainSocket()
{
    // 32 lines of dead code
    for (int32_t Round = 0; Round < 200; ++Round)
    {
        ...
    }
}
```

**Recommendation:** Either remove `DrainSocket()` entirely or mark it with a comment explaining it's retained for potential future use with `ioctlsocket(FIONREAD)`.

---

### WR-003 — Duplicate reconnect logic in ChangeDirectory and SendCommand

- **Files:** `src/core/ScpFileSystem.cpp`
- **Lines:** 1058-1065 (ChangeDirectory) and 575-588 (SendCommand)
- **Severity:** Warning

Both `ChangeDirectory` and `SendCommand` handle `FNeedsSessionReset`. The `ChangeDirectory` handler is the primary fix (5j); the `SendCommand` handler is a fallback for code paths that call `SendCommand` without going through `ChangeDirectory`. The duplicated logic is error-prone if one path is updated but not the other.

```cpp
// In ChangeDirectory (1060-1065):
FNeedsSessionReset = false;
try { FSecureShell->Close(); } catch (...) {}
FSecureShell->Open();
if (!FLastDirectory.IsEmpty())
    FCachedDirectoryChange = FLastDirectory;

// In SendCommand (577-588): IDENTICAL LOGIC
FNeedsSessionReset = false;
const UnicodeString SavedDir = FLastDirectory;
try { FSecureShell->Close(); } catch (...) {}
FSecureShell->Open();
if (!SavedDir.IsEmpty())
    FCachedDirectoryChange = SavedDir;
```

**Recommendation:** Extract the reconnect logic into a private `TSCPFileSystem::ReconnectSession()` helper method to eliminate duplication and reduce future maintenance risk.

---

### WR-004 — FLastDirectory not guarded against empty-string overwrite

- **File:** `src/core/ScpFileSystem.cpp`
- **Lines:** 1098, 1042
- **Severity:** Warning

`FLastDirectory` is set unconditionally in `CachedChangeDirectory()` and `ReadCurrentDirectory()`. If `FCachedDirectoryChange` or `FCurrentDirectory` is ever empty, `FLastDirectory` is silently set to empty, losing the previous known directory.

```cpp
// CachedChangeDirectory:
FCachedDirectoryChange = base::UnixExcludeTrailingBackslash(Directory);
FLastDirectory = FCachedDirectoryChange;  // Could be empty if Directory is empty

// ReadCurrentDirectory:
FCurrentDirectory = base::UnixExcludeTrailingBackslash(FOutput->GetString(0));
FLastDirectory = FCurrentDirectory;  // Could be empty if pwd returns nothing
```

**Recommendation:** Guard with `if (!FCachedDirectoryChange.IsEmpty())` or `if (!FCurrentDirectory.IsEmpty())` before assigning to `FLastDirectory`.

---

### IN-001 — Unused DrainSocket() method in public API

- **File:** `src/core/SecureShell.h`
- **Line:** ~180
- **Severity:** Info

Same as WR-002 but classified as Info since dead code in a public header is more of a documentation/clarity issue than a correctness issue. The method inflates the `TSecureShell` API surface without providing value.

---

### IN-002 — ChangeDirectory's FNeedsSessionReset guard not needed for non-SCP paths

- **File:** `src/core/ScpFileSystem.cpp`
- **Line:** 1058
- **Severity:** Info

The `FNeedsSessionReset` guard in `ChangeDirectory` only applies to SCP sessions (`TSCPFileSystem` is SCP-only). However, the guard itself has no comment explaining this, and someone reading the code might wonder if it applies to other file system implementations. This is a documentation concern only.

---

## Clean Files

| File | Status |
|------|--------|
| `src/core/Terminal.cpp` | ✓ Catch blocks correctly handle csCancel/csCancelTransfer vs csRemoteAbort |
| `src/NetBox/WinSCPFileSystem.h` | ✓ Member declarations are correct |

---

## Summary

| Severity | Count |
|----------|-------|
| Critical | 0 |
| Warning | 4 |
| Info | 2 |
| **Total** | **6** |

No critical (crash/security/data-loss) bugs found. Four warnings identified:

1. **WR-001:** Misleading comment in `__finally` describes old architecture
2. **WR-002:** `DrainSocket()` is dead code exported in header
3. **WR-003:** Duplicate reconnect logic in `ChangeDirectory` and `SendCommand`
4. **WR-004:** `FLastDirectory` not guarded against empty-string overwrite

Two info-level findings for API surface bloat and documentation gaps.

The Layer 5 fix (5j) is architecturally correct — handling reconnect in `ChangeDirectory` before `EnsureLocation()` resolves the dead-session bypass issue. The remaining warnings are maintainability concerns, not correctness issues.