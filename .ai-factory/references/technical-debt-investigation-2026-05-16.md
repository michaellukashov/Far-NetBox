# Technical Debt Investigation Report

> Date: 2026-05-16
> Scope: `src/` directory (NetBox-specific code)
> Based on: TODO-INVENTORY.md audit + targeted codebase analysis

---

## Summary

This report documents technical debt patterns discovered during investigation of the TODO inventory and targeted source analysis. Findings are organized by severity and category. Where a finding already has a plan or TODO in the inventory, it is cross-referenced.

---

## 1. Code Style & Consistency Debt

### 1.1 TODO Marker Inconsistency (Medium)

**Problem:** The codebase uses two incompatible TODO marking styles:
- **`TODO("literal")`** — Expands to `__pragma(message(...))` via `Sysutils.hpp`, emitting a compiler warning
- **`// TODO: description`** — Plain comment, invisible to the compiler

**Impact:** TODOs that should be compilation-visible (e.g., `TODO("implement")`) get lost alongside plain comments. The pragma-style TODO macro is disabled when `HIDE_TODO` is defined, but many plain-comment TODOs never emit warnings at all.

**Evidence:**
- `TODO("hide cursor")` in `FarDialog.cpp:2725` (pragma)
- `// TODO: throw error` in `FarDialog.cpp:876` (plain comment)
- `TODO("implement")` in `SessionData.cpp:1458` (pragma)
- `// TODO: report error` in `FarPlugin.cpp:1762` (plain comment)

**Recommendation:** Standardize on pragma-style `TODO()` for all actionable items, reserve `// TODO:` for notes only. This should be codified in `AGENTS-Standards.md`.

---

### 1.2 Duplicate Error Handling Code (Low)

**Problem:** Identical error-handling blocks copied without extraction to a helper.

**Evidence:** `src/NetBox/WinSCPDialogs.cpp` lines 1249–1253 and 1288–1292:
```cpp
AppLogFmt(L"MasterPassword: new passwords do not match");
MessageDialog(GetMsg(NB_MASTER_PASSWORD_DIFFERENT), qtError, qaOK);
return false;
```

**Recommendation:** Extract to `ValidatePasswordsMatch()` helper method.

---

### 1.3 Aggressive Warning Suppression (Low)

**Problem:** `src/include/disabled_warnings.hpp` disables ~35 compiler warnings globally, including legitimate ones:
- C4100 (unreferenced parameter) — masks dead code
- C4242/4244 (narrowing conversions) — data-loss risk
- C4365 (signed/unsigned mismatch) — common bug source
- C4640 (non-thread-safe local static) — threading risk

**Impact:** Real bugs may be silently compiled. The project builds at `/W4` but many useful warnings are globally off.

**Recommendation:** Audit warning suppressions. Remove C4244, C4365, and C4640 from global suppression; handle locally with `#pragma warning(push/pop)` at specific call sites.

---

## 2. Memory Management Debt

### 2.1 Raw `new` Outside BORLANDC Guards (Medium)

**Problem:** Standards document says "Prefer RAII and smart pointers. Avoid raw new/delete." Yet several active (non-legacy) code paths use raw `new`.

**Evidence:**
| File | Line | Allocation | Ownership |
|------|------|------------|-----------|
| `Terminal.cpp` | 419 | `FFatalError = new ExtException(...)` | `SAFE_DESTROY_EX` later |
| `Terminal.cpp` | 1190 | `CustomCopyParam = new TCopyParamType(...)` | **Leak risk** — no obvious delete |
| `Terminal.cpp` | 4959 | `AParams->Files = new TCollectedFileList()` | Added to parent list |
| `Terminal.cpp` | 6346 | `Params.Files = new TCollectedFileList()` | Added to parent list |
| `Terminal.cpp` | 6647 | `new TSynchronizeFileData` | Added to string list |
| `Terminal.cpp` | 9955 | `new TTerminal()` | Returned to caller |
| `Queue.cpp` | 1207 | `new TTerminalItem(...)` | Added to `FTerminals` list |
| `Queue.cpp` | 2521 | `new TParallelTransferQueueItem(...)` | Returned to caller |
| `RemoteFiles.cpp` | 875 | `FRights = new TRights()` | Member, lifetime unclear |
| `WinSCPDialogs.cpp` | 2895 | `new TBookmark()` | **Leak risk** — added to list? |
| `WinSCPDialogs.cpp` | 7414 | `new TRightsContainer(...)` | Local pointer, no delete |
| `WinSCPDialogs.cpp` | 8473 | `new TCopyParamsContainer(...)` | Local pointer, no delete |
| `WinSCPDialogs.cpp` | 10880 | `new TSynchronizeOptions` | Member replacement |

**Note:** Many `new` calls in `Terminal.cpp` and `Queue.cpp` are inside `#if defined(__BORLANDC__)` blocks — those are legacy BCC32 paths and acceptable. The table above lists only active MSVC paths.

**Recommendation:**
1. `CustomCopyParam` in `Terminal.cpp:1190` — wrap in `std::unique_ptr<TCopyParamType>`
2. Dialog-scoped allocations (`TRightsContainer`, `TCopyParamsContainer`) — verify they are attached to parent dialog that destroys them
3. `TSynchronizeOptions` in `WinSCPDialogs.cpp:10880` — already uses `SAFE_DESTROY` on previous value; fine but could be `std::unique_ptr`

---

### 2.2 `reinterpret_cast` Abuse (Low)

**Problem:** Heavy use of `reinterpret_cast` for type punning. Most are justified (Windows API, PuTTY structs, Far API), but some are unnecessary.

**Evidence:**
- `FarDialog.cpp:1569` — `int32_t * D = reinterpret_cast<int32_t *>(&R);` (TRect to int32_t array)
- `Terminal.cpp:7417` — `reinterpret_cast<const TChecklistItem *>(AFile)` where `AFile` is `TRemoteFile *`

**Recommendation:** Document invariants. The `TRect` cast is a known Delphi/VCL pattern; add a `static_assert(sizeof(TRect) == 4*sizeof(int32_t))` to make it safe.

---

## 3. Error Handling Debt

### 3.1 Bare `catch(...)` Without Rethrow (Medium)

**Problem:** `catch (...)` without rethrow swallows all exceptions, making debugging impossible.

**Evidence:** `src/core/Terminal.cpp:3884`:
```cpp
catch (...)
{
  LogEvent(FORMAT(L"ReadDirectory catch: FFiles=%p, using directory=%s", ...));
}
```

**Impact:** If an unexpected exception occurs during directory reading, it is silently swallowed. The log may not even be emitted if the format string itself throws.

**Recommendation:** At minimum, log the exception type (or rethrow). Consider:
```cpp
catch (const std::exception & E)
{
  LogEvent(FORMAT(L"ReadDirectory catch: %s, FFiles=%p...", E.what(), ...));
}
```

---

### 3.2 `ThrowNotImplemented()` with Dummy Stubs (Low)

**Problem:** Several functions call `ThrowNotImplemented(N)` with a TODO, but the `#if defined(__BORLANDC__)` block below contains the actual implementation. On MSVC these are permanently dead.

**Evidence:**
- `SessionData.cpp:1458` — `TODO("implement"); ThrowNotImplemented(3041);`
- `SessionData.cpp:4112` — `TODO("implement");` (settings export)
- `WebDAVFileSystem.cpp:2378` — `TODO("implement");`
- `Tools.cpp:1101` — `TODO("implement");` (file save dialog)

**Impact:** Feature gaps on MSVC that may never be noticed because the code "compiles fine."

**Recommendation:** For each `ThrowNotImplemented`, either:
1. Port the BORLANDC implementation to MSVC, or
2. Replace with a compile-time `#error` or runtime `DebugAssert(false)` so the gap is visible

---

## 4. Configuration & Magic Values

### 4.1 Hardcoded Timeouts (Already Known, Medium)

These already have TODOs in the inventory but bear repeating:

| Location | Value | Context | TODO ID |
|----------|-------|---------|---------|
| `SecureShell.cpp:2172` | 500ms | EOF wait timeout | 3.13 |
| `SecureShell.cpp:2175` | 100ms | Event loop step | 3.14 |
| `SecureShell.cpp:1481` | 500ms | Host communication timeout | — |
| `Queue.cpp:2492` | 5 * 1000ms | Parallel operation threshold | 3.53 |
| `Terminal.cpp:2414` | 1000ms | Progress log interval | — |
| `SecureShell.cpp:2605` | 1000ms | Send buffer update interval | — |

**Recommendation:** Define constants in a central config header (e.g., `constexpr int32_t DefaultEofTimeoutMs = 500;`). Several of these should be user-configurable.

---

## 5. Protocol-Specific Debt

### 5.1 SCP ASCII Mode 32-Bit Limit (High)

**TODO ID:** 3.35
**File:** `src/core/ScpFileSystem.cpp:2200`

**Problem:** When transferring large files in ASCII mode, the entire file is buffered into `AsciiBuf` and sent with a 32-bit size field. Files >4GB will overflow.

**Code:**
```cpp
/* TODO : We can't send file above 32bit size in ASCII mode! */
```

**Root cause:** The SCP protocol line is `C%s %lld %s` where `%lld` is 64-bit, but `AsciiBuf.GetSize()` returns `size_t` and the loop uses `OperationProgress->TransferBlockSize()` which may be capped.

**Recommendation:** Investigate whether the limit is in the buffer size variable or the protocol itself. If the buffer type is 32-bit, upgrade to 64-bit buffer indexing.

---

### 5.2 Path Normalization for `..` (Medium)

**TODO IDs:** 3.26, 3.40
**Files:** `FtpFileSystem.cpp:1010`, `SftpFileSystem.cpp:3121`

**Problem:** `AbsolutePath()` does not handle relative path segments like `..` or `.`. This can lead to path traversal issues or incorrect remote path resolution.

**Evidence:**
```cpp
TODO("improve (handle .. etc.)");
if (base::UnixIsAbsolutePath(APath))
{
  return APath;
}
```

**Recommendation:** Implement `base::UnixNormalizePath()` in `Common.cpp` that resolves `.` and `..` segments, then use it in both `TFTPFileSystem::AbsolutePath()` and `TSFTPFileSystem::AbsolutePath()`.

---

### 5.3 `std::unique_ptr` for `FileStream` (Low)

**TODO ID:** 3.42
**File:** `SftpFileSystem.cpp:5726`

**Problem:**
```cpp
TStream * FileStream = nullptr; // TODO: use std::unique_ptr<>
```

The `FileStream` is conditionally assigned and must outlive a `try__finally` block. A `std::unique_ptr<TStream>` would simplify cleanup.

**Recommendation:** Replace with `std::unique_ptr<TStream> FileStream;` and use `FileStream.reset(new THandleStream(...))`. Verify no downstream code stores the raw pointer.

---

## 6. Threading Debt (Already Addressed by Existing Plans)

The following areas already have active plans in `.ai-factory/plans/` and are **not** new debt:

| Plan | File | Status |
|------|------|--------|
| `multithreading-review-fix.md` | Terminal/thread marshaling | In progress |
| `threading-safety-audit-and-fixes.md` | Critical section coverage | In progress |
| `expand-threading-rules-lock-inventory.md` | Lock ordering | In progress |
| `terminal-thread-progress-marshaling.md` | Progress UI thread safety | In progress |
| `far3-startup-key-hang.md` | Deadlock during startup | Fixed (2026-05-12) |

**No additional threading debt identified beyond existing plan coverage.**

---

## 7. Recommendations Priority Matrix

| Priority | Item | Effort | Impact |
|----------|------|--------|--------|
| **H** | Fix SCP ASCII >32-bit transfer (3.35) | Medium | Data integrity for large files |
| **H** | Fix bare `catch(...)` in `Terminal.cpp:3884` | Low | Silent exception swallowing |
| **M** | Standardize TODO markers (pragma vs comment) | Low | Visibility of technical debt |
| **M** | Path normalization for `..` (3.26, 3.40) | Medium | Security/correctness |
| **M** | Audit raw `new` in active code | Medium | Memory leak prevention |
| **M** | Port `ThrowNotImplemented` stubs | High | Feature completeness |
| **L** | Extract duplicate password-match code | Low | DRY |
| **L** | Replace `SftpFileSystem` `FileStream` with `unique_ptr` (3.42) | Low | Cleanup |
| **L** | Centralize timeout constants | Low | Maintainability |
| **L** | Audit global warning suppressions | Medium | Bug prevention |

---

## 8. New TODOs to Add to Inventory

The following items were discovered during this investigation and should be added to the next inventory regeneration:

| File | Line | Type | Summary | Priority |
|------|------|------|---------|----------|
| `Terminal.cpp` | 1190 | TODO | `CustomCopyParam` raw `new` should use `unique_ptr` | M |
| `Terminal.cpp` | 3884 | FIXME | Bare `catch(...)` swallows exceptions silently | M |
| `WinSCPDialogs.cpp` | 1249/1288 | TODO | Duplicate password-match error block | L |
| `SecureShell.cpp` | 1481 | TODO | Hardcoded 500ms timer for host communication | L |
| `Terminal.cpp` | 2414 | TODO | Hardcoded 1000ms progress log interval | L |
| `disabled_warnings.hpp` | 30–80 | FIXME | Aggressive global warning suppression masks bugs | L |

---

*Report generated during technical debt investigation phase. Items should be triaged against existing roadmap priorities.*
