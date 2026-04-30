# Review Findings: Issue #511 — Speed Limit & Esc Hang

> **Review Date:** 2026-05-01  
> **Scope:** Changes for issue-511-speed-limit-esc-hang (commit comparison: staged changes)  
> **Files:** 10 files modified (Queue.cpp, Queue.h, WinSCPFileSystem.cpp/.h, FarPlugin.cpp/.h, FileOperationProgress.cpp, S3FileSystem.cpp, FarPluginStrings.cpp, plan document)

---

## Critical Issue: `FlushEscBuffer()` Consumes All Console Input

**Location:** `src/NetBox/FarPlugin.cpp:1765`  
**Severity:** HIGH  
**Category:** Correctness / Data Loss

### Problem

The `FlushEscBuffer()` method uses a `while` loop that calls `ReadConsoleInput` for **every** pending `INPUT_RECORD` — including mouse events, window resizes, focus events, and regular keystrokes. Only `VK_ESCAPE` key-down events are meant to be discarded, but the loop removes **all** events from the console input buffer unconditionally. Non-Escape events are never restored, causing user keystrokes and other input to be permanently lost.

### Current Implementation (Buggy)

```cpp
void TCustomFarPlugin::FlushEscBuffer() const
{
  INPUT_RECORD Rec;
  DWORD ReadCount;
  while (::PeekConsoleInput(FConsoleInput, &Rec, 1, &ReadCount) && ReadCount)
  {
    ::ReadConsoleInput(FConsoleInput, &Rec, 1, &ReadCount);  // removes ALL events
    if (Rec.EventType == KEY_EVENT &&
        Rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
        Rec.Event.KeyEvent.bKeyDown)
    {
      // Discard pending Esc events
    }
  }
}
```

### Recommended Fix

Buffer non-Escape events and write them back to the input buffer:

```cpp
void TCustomFarPlugin::FlushEscBuffer() const
{
  std::vector<INPUT_RECORD> Preserved;
  INPUT_RECORD Rec;
  DWORD ReadCount;

  while (::PeekConsoleInput(FConsoleInput, &Rec, 1, &ReadCount) && ReadCount)
  {
    if (!::ReadConsoleInput(FConsoleInput, &Rec, 1, &ReadCount))
      break;

    if (!(Rec.EventType == KEY_EVENT &&
          Rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
          Rec.Event.KeyEvent.bKeyDown))
    {
      Preserved.push_back(Rec);
    }
  }

  if (!Preserved.empty())
  {
    DWORD Written = 0;
    ::WriteConsoleInput(FConsoleInput, Preserved.data(),
        static_cast<DWORD>(Preserved.size()), &Written);
  }
}
```

---

## Medium Issue: Missing ROADMAP / Changelog Update

**Location:** Plan Task 6 (no file changes in diff)  
**Severity:** MEDIUM  
**Category:** Documentation Gap

The plan explicitly requires updating `ROADMAP.md` with issue #511 fixes (CPS limit propagation, reentrancy guards, `qaCancel` handling). No `ROADMAP.md` changes were present in the reviewed diff. Either:

- Add the ROADMAP entry documenting the fixes, or
- Mark Task 6 as not applicable and document why in the plan.

---

## Low Issue: `AnsiString` Narrowing in Log Context

**Location:** `src/core/Queue.cpp:1158`  
**Severity:** LOW  
**Category:** Internationalization / Data Loss

```cpp
TLogContext ctx_item("item", AnsiString(Item1->GetInfo()->Source).c_str());
```

`Source` is a `UnicodeString` (wide-char path). Converting to `AnsiString` drops non-ANSI characters in localized path names. If `TLogContext` cannot accept `UnicodeString` or `const wchar_t*` directly, the narrowing is a pre-existing limitation inherited by the new call site.

**Mitigation:** Verify whether `TLogContext` can be updated to accept wide strings, or document the limitation.

---

## Informational: Comment Precision in `AdjustToCPSLimit`

**Location:** `src/core/FileOperationProgress.cpp:478`  
**Severity:** INFO  
**Category:** Documentation Accuracy

The comment claims the code reads the "live value" of the CPS limit. In reality, `CPSLimit` is captured once into a `const` local at function entry. The improvement over direct `FCPSLimit` access is that `GetCPSLimit()` traverses the **parent chain** to resolve the effective limit for parallel transfers — not that it dynamically updates mid-loop. The behavior is correct; the comment could be more precise:

> "Use `GetCPSLimit()` to read the effective limit (including parent chain for parallel transfers)."

---

## Positive Findings

| Finding | Description |
|---|---|
| **Correct `qaCancel` handling** | Explicit `case qaCancel: ACancel = csCancel; break;` in `CancelConfiguration` fixes a real user-facing bug where pressing Esc inside the cancel dialog continued the transfer instead of cancelling it. |
| **Defense-in-depth guards** | `FInShowOperationProgress` (SCOPE_EXIT-managed), `!ProgressData.GetSuspended()`, `ProgressData.GetCancel() < csCancel`, and `!FInCancelDialog` create robust protection against reentrancy and exception-unwinding hazards. |
| **CPS limit propagation** | Passing `GetCPSLimit()` from parent into child `OperationProgress.Start()` correctly fixes the parallel-transfer speed-limit bug for SFTP/SCP/WebDAV/S3. |
| **Assert-before-dereference** | `DebugAssert(FParallelOperation->GetMainOperationProgress() != nullptr)` precedes the log line and `Start()` call, with a runtime `if` guard for release builds. |
| **Minimal accessor addition** | Adding `GetInfo()` to `TQueueItem` is a targeted fix for the pre-existing build error without expanding the public surface area. |
| **String table cleanup** | Duplicate `MISSING_TARGET_BUCKET` and `S3_UPLOAD_NEED_FILENAME` entries removed as recommended by prior review (`01-REVIEW.md`). |

---

## Summary

| Severity | Count | Description |
|---|---|---|
| HIGH | 1 | `FlushEscBuffer` swallows all console input (not just Escape) |
| MEDIUM | 1 | Missing ROADMAP/changelog update for documented fixes |
| LOW | 1 | `AnsiString` narrowing drops non-ANSI path characters in logs |
| INFO | 1 | Comment overstates "live value" behavior in `AdjustToCPSLimit` |

The functional fixes for issue #511 are well-designed and address the documented root causes. The changes **should not be merged as-is** until the `FlushEscBuffer` input-loss bug is corrected and the ROADMAP entry is added (or Task 6 is explicitly waived).
