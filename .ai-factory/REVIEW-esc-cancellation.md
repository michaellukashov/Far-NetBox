---
status: issues_found
files_reviewed: 5
depth: standard
critical: 0
warning: 7
info: 3
total: 10
review_date: 2026-05-02
reviewed_by: gsd-code-reviewer
---
# Code Review: SCP Esc Cancellation Fix (Issue #511 Follow-up)

## Scope

5 files reviewed across 4 review areas:

| File | Review Focus |
|------|-------------|
| `src/NetBox/FarPlugin.cpp` | CheckForEsc()/FlushEscBuffer() console buffer rewrite |
| `src/core/ScpFileSystem.cpp` | Cancel-state checks in SCP upload/download loops, ReadCommandOutput guard |
| `src/core/Terminal.cpp` | Exception conversion (EAbort) and diagnostic logging in CopyToRemote/CopyToLocal |
| `src/NetBox/WinSCPFileSystem.cpp` | Reentrancy guards, CancelConfiguration logic (prior fix) |
| `src/NetBox/WinSCPFileSystem.h` | Member declarations for FInShowOperationProgress, FInCancelDialog (prior fix) |

---

## Findings

### WR-001 — CancelConfiguration guard inconsistent with ShowOperationProgress

- **File:** `src/NetBox/WinSCPFileSystem.cpp`
- **Line:** 4128–4131
- **Severity:** Warning

CancelConfiguration's early-return guard (`GetCancel() > csContinue`) is more restrictive than
ShowOperationProgress's entry condition (`GetCancel() < csCancel`). During the `csCancelFile` state,
CheckForEsc() consumes Esc from the input buffer but CancelConfiguration returns early without showing
a dialog or any feedback — the Esc press is silently discarded. Practical impact is low because
csCancelFile is transient, but the inconsistency could become a real bug if csCancelFile usage expands.

**Recommendation:** Align CancelConfiguration guard to `if (GetCancel() >= csCancel) return;` to match
the ShowOperationProgress condition, or move CheckForEsc() behind a `GetCancel() < csCancel` check so
Esc is not consumed when the dialog cannot be shown.

---

### WR-002 — OnceDoneOperation unreachable after throw in CopyToRemote catch

- **File:** `src/core/Terminal.cpp`
- **Line:** 7930–7932 (CopyToRemote catch block)
- **Severity:** Warning

The `else` branch at line 7927 unconditionally throws `EAbort("")`, making the subsequent
`OnceDoneOperation = odoIdle` at line 7930 unreachable on the cancellation path. The original code
always set `odoIdle` before the prior fix. Depending on what catches the `EAbort`, the stale
`OnceDoneOperation` value could cause unintended side effects (e.g., triggering `CloseOnCompletion`).

```cpp
// Current (problematic):
else
{
    throw EAbort("");           // line 7927-7928
}
OnceDoneOperation = odoIdle;    // line 7930 — unreachable when cancelled
```

**Recommendation:** Move `OnceDoneOperation = odoIdle` before the throw, or set it to `odoIdle` in
BOTH branches before any throw/CommandError path:

```cpp
if (OperationProgress.GetCancel() == csContinue)
{
    CommandError(&E, MainInstructions(LoadStr(TOREMOTE_COPY_ERROR)));
}
else
{
    OnceDoneOperation = odoIdle;
    throw EAbort("");
}
```

---

### WR-003 — CopyToLocal catch block indentation is broken

- **File:** `src/core/Terminal.cpp`
- **Line:** 8510–8528 (CopyToLocal catch block)
- **Severity:** Warning

The CopyToLocal catch block has inconsistent indentation that makes control flow difficult to read:
- `catch` at 2-space indent pairs with `try` at 6-space indent (line 8468)
- `if`/`else` body braces at 8-space vs `catch`-level statements at 4-space
- Closing brace at 6-space mismatches opening at 2-space

The CopyToRemote catch block (line 7920) is correctly indented.

**Recommendation:** Re-indent the entire CopyToLocal catch block to match the `try` level (6-space)
and apply consistent 2-space incremental indentation inside, same pattern as CopyToRemote.

---

### WR-004 — Cancel condition narrowing silently discards csRemoteAbort diagnostics

- **File:** `src/core/Terminal.cpp`
- **Line:** 7924–7929 (CopyToRemote) and 8516+ (CopyToLocal)
- **Severity:** Warning

The condition was changed from `GetCancel() != csCancel` to `GetCancel() == csContinue`, expanding
the `throw EAbort` path to include `csCancelFile`, `csCancelTransfer`, and `csRemoteAbort`. While
`csCancelFile` is unlikely at this level and `csCancelTransfer` should be handled correctly,
`csRemoteAbort` is a legitimate state indicating the remote side closed the connection mid-transfer.
The original code would call `CommandError` (possibly showing a dialog) for `csRemoteAbort`; the new
code silently aborts.

If a genuine `EFatal` (connection loss) coincides with a non-Continue cancel state, the fatal error
would be silently swallowed as `EAbort("")` — the user gets no diagnostic.

**Recommendation:** Use a more nuanced condition: throw `EAbort` only for `csCancel` and
`csCancelTransfer` (user-initiated), but call `CommandError` for `csRemoteAbort` and `csCancelFile`
(which may need user-visible diagnostics).

---

### WR-005 — OnceDoneOperation unreachable after throw in CopyToLocal catch

- **File:** `src/core/Terminal.cpp`
- **Line:** 8521–8523
- **Severity:** Warning

Same issue as WR-002, but in the CopyToLocal catch block. `OnceDoneOperation` is unreachable after
`throw EAbort("")`.

**Recommendation:** Same fix as WR-002 — move `OnceDoneOperation = odoIdle` before the throw.

---

### WR-006 — WriteConsoleInput partial-write not checked

- **File:** `src/NetBox/FarPlugin.cpp`
- **Line:** 1785–1787 (CheckForEsc) and 1825–1827 (FlushEscBuffer)
- **Severity:** Warning

Both functions call `WriteConsoleInput` to restore non-Esc events but ignore the `Written` count.
If the console input buffer is full or the call partially fails, user keystrokes consumed by
`ReadConsoleInput` are silently discarded. The codebase has precedent for checking this
(`ConsoleRunner.cpp` uses `DebugCheck` + `DebugAssert`). The guarantee of preserving non-Esc events
becomes best-effort rather than robust.

**Recommendation:** After `WriteConsoleInput`, check `Written < NonEscEvents.size()` and handle
the shortfall — at minimum log a warning:

```cpp
if (Written < NonEscEvents.size())
{
    LogEvent(FORMAT(L"WriteConsoleInput: wrote %u of %u events",
        Written, static_cast<DWORD>(NonEscEvents.size())));
}
```

---

### IN-001 — Non-atomic static LastTicks in CheckForEsc()

- **File:** `src/NetBox/FarPlugin.cpp`
- **Line:** 1745–1746
- **Severity:** Info

`static uint32_t LastTicks` is read and written without synchronization. It's currently
single-threaded so safe, but the static storage makes this a latent data-race hazard.

**Recommendation:** Replace with `static std::atomic<uint32_t> LastTicks{0};` using relaxed ordering.

---

### IN-002 — Potential MSVC C4702 unreachable code warning

- **File:** `src/core/Terminal.cpp`
- **Line:** 7930–7932 (CopyToRemote), 8521–8523 (CopyToLocal)
- **Severity:** Info

The `else` branch unconditionally throws, making subsequent code unreachable. The project compiles
with `/W4` which may generate C4702 ("unreachable code"). Not observed in current build, but could
appear depending on compiler version or optimization flags.

**Recommendation:** Restructure to avoid unreachable code (same fix as WR-002/WR-005).

---

### IN-003 — Unnamed EAbort message string literal

- **File:** `src/core/Terminal.cpp`
- **Line:** 7930 (CopyToRemote), 8521 (CopyToLocal)
- **Severity:** Info

`throw EAbort("")` uses an empty string literal instead of a named constant. The codebase uses
`EXCEPTION_MSG_REPLACED` (`L"[replaced]"`) for fatal aborts. While functionally correct, a named
constant would make intent explicit and match the codebase pattern.

**Recommendation:** Define a named constant (e.g., `EXCEPTION_MSG_USER_CANCEL`) and use it instead
of the literal empty string.

---

## Clean Files

| File | Status |
|------|--------|
| `src/core/ScpFileSystem.cpp` | ✓ No issues found. Cancel checks, fatal guards, and ReadCommandOutput skip are internally consistent |
| `src/NetBox/WinSCPFileSystem.h` | ✓ No issues found. Member declarations are correct |

---

## Summary

| Severity | Count |
|----------|-------|
| Critical | 0 |
| Warning | 7 |
| Info | 3 |
| **Total** | **10** |

No critical (crash/security/data-loss) bugs found. Seven warnings identified:

1. **WR-001:** CancelConfiguration guard mismatch during csCancelFile (latent UX bug)
2. **WR-002/WR-005:** Unreachable `OnceDoneOperation = odoIdle` after throw in both catch blocks
3. **WR-003:** Broken indentation in CopyToLocal catch block (maintainability)
4. **WR-004:** Cancel condition narrowing silently discards csRemoteAbort diagnostics
5. **WR-006:** WriteConsoleInput partial-write not checked (robustness gap)

Three info-level findings for atomicity, unreachable code, and naming patterns.
