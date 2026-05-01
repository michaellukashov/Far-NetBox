---
type: code-review
scope: fix-ssh-chmod-444-crash-393 (commits 143bcdbcf..7167433b0)
reviewed: 2026-05-01T22:45:00Z
files: 4
commits: 4
---

# Code Review: SSH chmod 444 Crash Fix (#393)

**Reviewed:** 2026-05-01
**Commits:** `143bcdbcf` → `e8963516b` → `4163d6bef` → `7167433b0`
**Files Changed:** `src/core/Terminal.cpp`, `src/core/SftpFileSystem.cpp`, `src/NetBox/WinSCPFileSystem.cpp`, `ChangeLog`, `.ai-factory/plans/fix-ssh-chmod-444-crash-393.md`
**Risk Level:** 🟢 Low

---

## Commit-Level Review

### Commit 1: `143bcdbcf` — fix(sftp): prevent crash when chmod 444 fails on remote directories

| Aspect | Verdict | Notes |
|--------|---------|-------|
| Commit message | ✅ Excellent | Conventional commit, references issue #393, explains the three crash paths |
| Atomicity | ✅ Good | Three related null guards in one commit — appropriate grouping |
| Null guards | ✅ Correct | All three crash paths properly guarded |
| `LogEvent(FORMAT(..., FFiles, Directory))` | ❌ **Bug** | `FFiles` (unique_ptr) passed to `%p` — undefined behavior. Fixed in follow-up commit. |
| `throw ExtException(nullptr, L"Cannot read file properties before changing them")` | ⚠️ Acceptable | Generic error message, but `ExtException` is not the best choice for a "should never happen" path. Fixed in follow-up commit. |
| `DoReadDirectoryFinish` early return | ⚠️ Acceptable | Does not clear `FFiles`, leaving stale directory listing. Fixed in follow-up commit. |
| Logging in catch block | ⚠️ Acceptable | No protection against `LogEvent` throwing. Fixed in follow-up commit. |

### Commit 2: `e8963516b` — fix(review): address code-review findings for SSH chmod 444 crash fix

| Aspect | Verdict | Notes |
|--------|---------|-------|
| Commit message | ✅ Excellent | Lists each review finding by ID (CR-01, IN-02, WR-02, WR-01, IN-01) |
| Atomicity | ✅ Good | All review fixes in one commit — appropriate for follow-up |
| `static_cast<const void*>(FFiles.get())` | ✅ Correct | Fixes the `%p` format specifier issue |
| `try { LogEvent(...) } catch (...) { /* ignore */ }` | ✅ Correct | Prevents logging failures from superseding original exception |
| `FFiles.reset()` in `DoReadDirectoryFinish` | ✅ Correct | Clears stale directory listing |
| `EFatal` with defensive comment | ✅ Correct | Better exception type for "should never happen" path |

### Commit 3: `4163d6bef` — docs(changelog): add SSH chmod 444 crash fix (#393)

| Aspect | Verdict | Notes |
|--------|---------|-------|
| Commit message | ✅ Good | Conventional commit, follows existing ChangeLog style |
| Entry format | ✅ Correct | Consistent with existing `[Unreleased]` entries |
| Issue link | ✅ Correct | Includes full GitHub issue URL |

### Commit 4: `7167433b0` — docs(plan): mark Task 5 manual verification complete

| Aspect | Verdict | Notes |
|--------|---------|-------|
| Commit message | ✅ Good | Conventional commit |
| Also creates security audit file | ⚠️ Minor | Commit message doesn't mention `.ai-factory/security-audit-ssh-chmod-393.md` creation — slightly incomplete |

---

## Per-File Code Review

### `src/core/Terminal.cpp`

#### `DoReadDirectoryFinish()` (line 3730)

```cpp
if (AFiles == nullptr)
{
  LogEvent("DoReadDirectoryFinish: AFiles is null, clearing stale directory listing");
  FFiles.reset();
  return;
}
```

- ✅ **Null guard is correct.** Returns early before `OldFiles.release()` / `FFiles.reset(AFiles)`, preventing null dereference.
- ✅ **`FFiles.reset()` clears stale listing.** Before the fix, the old `FFiles` remained, causing the panel to show outdated files.
- ⚠️ **Nit:** `LogEvent` takes `UnicodeString&`; the narrow string literal `"..."` implicitly converts. This is consistent with surrounding code but mixing narrow/wide string literals across the codebase is a minor style inconsistency. **Non-blocking.**

#### `ReadDirectory()` catch block (line 3819)

```cpp
catch (Exception & E)
{
  const UnicodeString Directory = (FFiles != nullptr) ? FFiles->GetDirectory() : GetCurrentDirectory();
  try
  {
    LogEvent(FORMAT(L"ReadDirectory catch: FFiles=%p, using directory=%s",
      static_cast<const void*>(FFiles.get()), Directory));
  }
  catch (...)
  {
    // Ignore logging failures — the original error is more important
  }
  CommandError(&E, FMTLOAD(LIST_DIR_ERROR, Directory));
}
```

- ✅ **Null guard correct.** `GetCurrentDirectory()` fallback prevents crash when `FFiles == nullptr`.
- ✅ **`static_cast<const void*>(FFiles.get())` fixes CWE-134.** Correctly passes raw pointer to `%p`.
- ✅ **Logging wrapped in try/catch.** Prevents secondary failures from masking the original error.
- ⚠️ **Suggestion:** The `catch (...)` with empty body could use `FTerminal->LogEvent(L"Logging failed in ReadDirectory catch block")` as a last-resort fallback, but this is overkill for a catch block that already calls `CommandError`. **Non-blocking.**

### `src/core/SftpFileSystem.cpp`

#### `ChangeFileProperties()` (line 4375)

```cpp
if (File == nullptr)
{
  FTerminal->LogEvent(FORMAT(L"ChangeFileProperties: ReadFile returned null for %s (unexpected)",
    RealFileName));
  // Defensive: ReadFile should throw on failure, not return null
  throw EFatal(nullptr, L"Internal error: ReadFile returned null without throwing");
}
```

- ✅ **Null guard correct.** Prevents dereferencing `File->GetIsDirectory()`, `File->GetFileOwner()`, `File->GetFileGroup()`, `File->GetRights()`.
- ✅ **`EFatal` is the right exception type.** `EFatal` signals a terminal-level fatal error, which is appropriate for a "should never happen" internal inconsistency. `ExtException` (used in the first commit) would be misleading.
- ✅ **Comment explains the defensive nature.** Helps future maintainers understand this guard catches an internal protocol inconsistency, not a normal error path.
- ✅ **`FORMAT` uses hardcoded literal with typed argument.** `RealFileName` is `UnicodeString&`. No format-string injection risk.

### `src/NetBox/WinSCPFileSystem.cpp`

#### `GetFindDataEx()` (line 500)

```cpp
if (GetTerminal()->Files != nullptr)
{
  for (int32_t Index = 0; Index < GetTerminal()->Files->Count; ++Index)
  {
    TRemoteFile * File = GetTerminal()->Files->GetFile(Index);
    DebugAssert(File);
    PanelItems->Add(new TRemoteFilePanelItem(File));
  }
}
else
{
  FTerminal->LogEvent(L"GetFindDataEx: GetTerminal()->Files is null, panel will be empty");
}
```

- ✅ **Null guard correct.** Prevents crash when `GetTerminal()->Files->Count` is accessed while `Files` is null.
- ✅ **Graceful degradation.** Returns empty panel items instead of crashing.
- ⚠️ **Nit:** The log message says "panel will be empty" but the function continues to `Result = true` (line 518). The message is accurate — the panel is indeed empty. **Non-blocking.**

---

## Context Gates

### Architecture Gate

- **Pass.** Changes are scoped to Core Layer (`src/core/`) and Plugin Layer (`src/NetBox/`), matching the plan's architecture boundaries. No Base Layer or `libs/` modifications.

### Rules Gate

- **Pass.** All changes follow NetBox naming conventions (`T` prefix classes, `F` prefix members). No trailing whitespace introduced. `LogEvent()` calls use `L"..."` wide strings consistently within each file.

### Roadmap Gate

- **Pass.** This is an independent bug fix. The ROADMAP.md does not list Issue #393, and the plan correctly marks milestone as "none" with rationale "independent bug fix."

---

## Summary

| Category | Count |
|----------|-------|
| Critical Issues | 0 (all fixed in e8963516b) |
| Warnings | 0 |
| Nits (non-blocking) | 3 (narrow string literal, catch body, log phrasing) |
| Positive Notes | 5 (null guards, EFatal choice, FFiles.reset(), try/catch around logging, cast to void*) |

### Final Verdict

**🟢 APPROVED with minor nits.**

The initial commit (`143bcdbcf`) introduced a format-string bug (`FFiles` passed to `%p`) and suboptimal exception type (`ExtException` vs `EFatal`), but these were **all corrected in the follow-up review commit (`e8963516b`)**. The final code is correct, defensive, and well-instrumented.

**No further action required.** The implementation is ready for merge.

_Reviewed: 2026-05-01_
_Reviewer: Claude (aif-review)_
