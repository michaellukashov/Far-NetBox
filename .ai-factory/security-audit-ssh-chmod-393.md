---
type: security-audit
scope: fix-ssh-chmod-444-crash-393 (Issue #393)
audited: 2026-05-01T22:45:00Z
files:
  - src/core/Terminal.cpp
  - src/core/SftpFileSystem.cpp
  - src/NetBox/WinSCPFileSystem.cpp
severity: none
findings: 0
---

# Security Audit: SSH chmod 444 Crash Fix (#393)

**Audited:** 2026-05-01
**Scope:** 3 source files changed for null-dereference hardening
**Base commits:** `e66be50a0` (before fix) .. `a797cddd2` (after review fixes)

## Executive Summary

No security vulnerabilities found. All changes are defensive null-guard hardening that **reduce** attack surface (null dereference → controlled exception). Logging additions use hardcoded format strings with type-safe arguments. No secrets, credentials, or sensitive data exposed.

---

## Per-File Analysis

### `src/core/SftpFileSystem.cpp`

| Change | Security Review |
|--------|----------------|
| `LogEvent(FORMAT(L"ChangeFileProperties: ReadFile returned null for %s (unexpected)", RealFileName))` | ✅ **Safe.** Format string is a compile-time literal. `RealFileName` is `UnicodeString&`. `FORMAT` → `nb::Sprintf` uses `fmt` library which is type-safe. No format-string injection risk (CWE-134). |
| `throw EFatal(nullptr, L"Internal error: ReadFile returned null without throwing")` | ✅ **Safe.** Generic internal error message. No user-controlled data, no server response text, no file contents leaked (CWE-209). `nullptr` inner exception means no exception chain exposure. |
| `LogEvent(FORMAT(L"ChangeFileProperties: rights set=%04x unset=%04x path=%s", ...))` | ✅ **Safe.** Rights values (`%04x`) are bitwise flags, not sensitive. Path logging is at the same level as existing NetBox logging. Log file is local to user machine (`%LOCALAPPDATA%`). |

### `src/core/Terminal.cpp`

| Change | Security Review |
|--------|----------------|
| `LogEvent(FORMAT(L"ReadDirectory catch: FFiles=%p, using directory=%s", static_cast<const void*>(FFiles.get()), Directory))` | ✅ **Safe.** `%p` receives a `void*` cast from `std::unique_ptr::get()`. `Directory` is `UnicodeString&`. No format-string injection. Pointer value is not sensitive. |
| `FFiles->GetDirectory()` with `GetCurrentDirectory()` fallback | ✅ **Safe.** Correctly prevents null dereference. The fallback is a local getter, not user-controlled. |
| `FFiles.reset()` when `AFiles == nullptr` | ✅ **Safe.** Synchronous state cleanup. No TOCTOU race. Prevents stale panel data display, not a security boundary. |
| `try { LogEvent(...) } catch (...) { /* ignore */ }` | ✅ **Safe.** Prevents logging failures from superseding the original exception. Reduces risk of secondary failures masking the real error. |

### `src/NetBox/WinSCPFileSystem.cpp`

| Change | Security Review |
|--------|----------------|
| `if (GetTerminal()->Files != nullptr)` guard | ✅ **Safe.** Graceful degradation to empty panel. No security state bypass. |
| `LogEvent(L"GetFindDataEx: GetTerminal()->Files is null, panel will be empty")` | ✅ **Safe.** Static message, no user data. |

---

## CWE Coverage

| CWE | Relevance | Verdict |
|-----|-----------|---------|
| CWE-134: Externally-Controlled Format String | All `FORMAT` calls use hardcoded literals with typed arguments. `fmt` library is type-safe. | ✅ **Not applicable** |
| CWE-209: Information Exposure Through Error Messages | Error messages are generic ("Internal error"). Log paths are local-only and consistent with existing NetBox logging policy. | ✅ **No exposure** |
| CWE-476: NULL Pointer Dereference | **This is the bug being fixed.** All three crash paths now have null guards. | ✅ **Fixed** |
| CWE-770: Uncontrolled Resource Consumption | No new allocations or unbounded loops introduced. | ✅ **Not applicable** |
| CWE-754: Improper Check for Unusual Conditions | Added defensive checks for unusual (null return from `ReadFile`) conditions. | ✅ **Improved** |

---

## Secrets & Credential Audit

```
Search: password|secret|api_key|token|credential|private_key|passphrase
Files: src/core/Terminal.cpp, src/core/SftpFileSystem.cpp, src/NetBox/WinSCPFileSystem.cpp
Result: 0 matches
```

No secrets, credentials, or hardcoded keys in the changed code.

---

## Overall Verdict

🔒 **PASSED** — No security issues. Changes are purely defensive hardening that reduce crash surface and improve error handling. Safe to merge.

_Reviewed: 2026-05-01_
_Auditor: Claude (aif-security-checklist)_
