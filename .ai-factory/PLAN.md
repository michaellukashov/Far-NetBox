# Plan: Threading Fix & Cleanup

> Mode: fast
> Created: 2026-05-12
> Branch: current (uncommitted changes in working tree)

## Settings

| Setting | Value |
|---------|-------|
| Build verification | MSVC W4 x64 RelWithDebugInfo |
| Logging | verbose |
| Docs | skip (cleanup only) |
| Tests | skip (fix/cleanup scope) |

## Architecture Notes

All tasks operate within `src/windows/` (Windows layer) and `src/core/` (Core Layer).
Dependency flow:

```
|Plugin Layer (src/NetBox/)         — not touched
    ↓
Core Layer (src/core/)             — Task 2: FileOperationProgress.cpp
    ↓
Windows Layer (src/windows/)       — Task 1: GUITools.cpp (calls Core TTerminal)
    ↓
Base Layer (src/base/)             — not touched
    ↓
Third-Party (libs/, src/filezilla/) — not touched — libs/ boundary respected
```

- **No modifications to `libs/`** — all changes in `src/` source tree
- **No CMake changes** — existing files only, no new compilation units; Unity build unaffected
- **Build platform:** x64 RelWithDebugInfo, MSVC W4

---


Two threading fixes discovered during threading safety exploration:
|1. **Fix TPuttyCleanupThread Finalize() double-wait** — uncommitted event-based replacement has a bug
2. **Fix FileOperationProgress::Assign() address-based locking** — dormant dual-acquisition deadlock risk

---

## Task 1: Fix TPuttyCleanupThread Finalize() double-wait on auto-reset event

**File(s):** `src/windows/GUITools.cpp`

**Problem:** The working tree already has event-based replacement for `TPuttyCleanupThread` (Sleep→WaitForSingleObject), but `Finalize()` waits twice on auto-reset `FDoneEvent`:

```cpp
// FDoneEvent is auto-reset (CreateEvent with FALSE for bManualReset)
::WaitForSingleObject(FDoneEvent, INFINITE);   // (1) unconditional — consumes signal
if (NeedWait)
{
  ::WaitForSingleObject(FDoneEvent, INFINITE); // (2) BLOCKS FOREVER
}
```

The `__finally` block in `Execute()` signals `FDoneEvent` exactly once. The first wait consumes the signal; the second blocks forever.

**Fix — Option A (single conditional wait):**

```cpp
void TPuttyCleanupThread::Finalize()
{
  if (FDoneEvent != nullptr)
  {
    bool NeedWait = false;
    {
      TGuard Guard(*FSection.get());
      NeedWait = (FInstance != nullptr);
    }
    if (NeedWait)
    {
      ::WaitForSingleObject(FDoneEvent, INFINITE);
    }
  }
}
```

Also in `Execute()`, the `__finally` block already signals `FDoneEvent` (correct), and the loop's `WaitForSingleObject(FTimerEvent, 400)` with `Sleep(400)` fallback is correct. Verify no other changes needed.

**Edge case:** `Finalize()` called with no thread ever started → `NeedWait` is false → no wait → correct (event never signaled, wait would hang).

**Verification:** Build with MSVC W4, zero warnings.

---

## Task 2: Fix FileOperationProgress::Assign() with address-based ordering

**File(s):** `src/core/FileOperationProgress.cpp`, `src/core/FileOperationProgress.h`

**Problem:** `Assign()` acquires `this->FSection` then `Other.FSection` in fixed order. Under concurrent bidirectional assignment (Thread A: `progressA.Assign(progressB)`, Thread B: `progressB.Assign(progressA)`), this is a classic deadly embrace.

Currently dead code (no external callers), but making it safe removes a dormant risk.

**Fix — address-based locking:**

```cpp
void TFileOperationProgressType::Assign(const TFileOperationProgressType & Other)
{
  if (&Other == this) return;  // self-assignment guard

  // Lock in address order to prevent deadlock under concurrent assignment
  TCriticalSection * first = &FSection;
  TCriticalSection * second = &Other.FSection;
  if (first > second) { std::swap(first, second); }

  const TGuard Guard1(*first);
  const TGuard Guard2(*second);

  *this = Other;
}
```

Do NOT remove the outer `TGuard Guard(FSection)` from `AssignButKeepSuspendState()` — it exists for a reason (protects `FSuspendTime` restoration). The recursive `TCriticalSection` makes the re-entrant acquisition safe.

**Verification:** Build with MSVC W4, zero warnings.

**Also update:** When applying the fix, also update the `Known Limitations` section in `.ai-factory/references/multithreading-review-fix-results.md` (line 106) which says `"Assign() dual-acquires FSection and Other.FSection without address-based ordering"` — remove or update this line since the address-based fix resolves it.

## Verification

- [x] Task 1: Build with MSVC W4 (x64) — zero warnings
- [x] Task 2: Build with MSVC W4 (x64) — zero warnings
- [x] Run `/aif-verify` to validate the implementation against the plan
