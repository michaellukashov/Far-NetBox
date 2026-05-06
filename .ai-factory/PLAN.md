# Implementation Plan: Master Password Infrastructure Hardening

> Source: `.ai-factory/plans/master-password-infrastructure.md`
> Branch: `feature/master-password-hardening`
> Based on: WinSCP 6.5.6 reference implementation
> Created: 2026-05-05

---

## Verified State

| Finding | Location | Status |
|---|---|---|
| MSVC `RecryptPasswords()` stub | `WinConfiguration.cpp:4074-4078` | **STUB — data loss risk** |
| BorlandC `RecryptPasswords()` full impl | `WinConfiguration.cpp:982-1005` | **Working — port target** |
| `ChangeMasterPassword()` exception unsafe | `MasterPassword.cpp:18-27` | **Bug — decrypt key stale on throw** |
| `ClearMasterPassword()` exception unsafe | `MasterPassword.cpp:36-42` | **Bug — Shred skipped on throw** |
| `FMasterPasswordSession` non-atomic | `WinConfiguration.h:517` | **Race condition under parallel transfers** |
| Plaintext in `UnicodeString` | `WinConfiguration.h:480-481` | **COW copies may persist in memory** |
| Errors discarded as `nullptr` | `WinSCPDialogs.cpp:1217,1250,1278` | **Silent failures** |
| Scope guard macros exist | `Sysutils.hpp:510-560` | **Available — `try__finally` / `SCOPE_EXIT`** |
| `pass_ptrW`/`pass_ptrA` prior art | `nbsystem_cpp.h:61-85` | **Pattern for secure memory** |
| `Shred()` utility | `Common.cpp:746` | **Available — best-effort zeroing** |

**Note:** Hardcoded dialog strings (originally reported at WinSCPDialogs.cpp:1134,1170,1204) were **not found** in current code — already fixed or line numbers shifted. Plan adjusted accordingly.

---

## Phase 1: Implement RecryptPasswords for MSVC (P0 — Data Loss Fix)

### Task 1.1: Port BorlandC RecryptPasswords to MSVC stub

**File:** `src/windows/WinConfiguration.cpp` (lines 4074-4078)

**Change:** Replace stub with full implementation matching BorlandC lines 982-1005:

```cpp
void TWinConfiguration::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  TCustomWinConfiguration::RecryptPasswords(RecryptPasswordErrors);

  try
  {
    TTerminalManager * Manager = TTerminalManager::Instance(false);
    DebugAssert(Manager != nullptr);
    if (Manager != nullptr)
    {
      Manager->RecryptPasswords();
    }
  }
  catch (Exception & E)
  {
    UnicodeString Message;
    if (ExceptionMessage(&E, Message))
    {
      RecryptPasswordErrors->Add(Message);
    }
  }
}
```

**Verification:**
- Build passes zero warnings (`build-x64.bat`)
- BorlandC implementation at lines 982-1005 is **untouched** (guarded by `#ifdef __BORLANDC__`)

---

### Task 1.2: Fix exception safety in ChangeMasterPassword

**File:** `src/windows/MasterPassword.cpp` (lines 18-27)

**Problem:** `FPlainMasterPasswordEncrypt = value` is set BEFORE `RecryptPasswords()`, but `FPlainMasterPasswordDecrypt = value` is set AFTER. If recryption throws, decrypt key remains old → subsequent password loads decrypt with wrong key → garbage.

**Fix:** Use existing `SCOPE_EXIT` macro from `Sysutils.hpp`:

```cpp
void TWinConfiguration::ChangeMasterPassword(
    UnicodeString value, TStrings * RecryptPasswordErrors)
{
  RawByteString Verifier;
  AES256CreateVerifier(value, Verifier);
  FMasterPasswordVerifier = BytesToHex(Verifier);
  FPlainMasterPasswordEncrypt = value;
  FUseMasterPassword = true;
  SCOPE_EXIT
  {
    FPlainMasterPasswordDecrypt = value;
  };
  MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
}
```

**Alternative:** If `SCOPE_EXIT` conflicts with existing `try/catch` in callers, use `std::unique_ptr` with custom deleter or a local `finally`-style lambda. But `SCOPE_EXIT` is already used elsewhere in the codebase.

---

### Task 1.3: Fix exception safety in ClearMasterPassword

**File:** `src/windows/MasterPassword.cpp` (lines 36-42)

**Problem:** `Shred(FPlainMasterPasswordEncrypt)` runs before `RecryptPasswords()`, but `Shred(FPlainMasterPasswordDecrypt)` runs after. If recryption throws, old decrypt password remains in memory.

**Fix:**

```cpp
void TWinConfiguration::ClearMasterPassword(TStrings * RecryptPasswordErrors)
{
  FMasterPasswordVerifier = L"";
  FUseMasterPassword = false;
  Shred(FPlainMasterPasswordEncrypt);
  SCOPE_EXIT
  {
    Shred(FPlainMasterPasswordDecrypt);
  };
  MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
}
```

---

### Task 1.4: Surface recryption errors in master password dialog

**File:** `src/NetBox/WinSCPDialogs.cpp` (lines 1217, 1250, 1278)

**Problem:** `ChangeMasterPassword(NewPwd, nullptr)` and `ClearMasterPassword(nullptr)` silently discard errors.

**Fix:** Pass a `TStringList` to collect errors, then display them:

```cpp
// Around line 1217 (change mode)
std::unique_ptr<TStringList> Errors(std::make_unique<TStringList>());
WinConfiguration->ChangeMasterPassword(NewPwd, Errors.get());
if (Errors->GetCount() > 0)
{
  // Show first error or aggregated message
  MessageDialog(Errors->GetString(0), qtWarning, qaOK);
}
```

Repeat for set mode (line ~1250) and clear mode (line ~1278).

**Acceptance:**
- [ ] Build zero warnings
- [ ] RecryptPasswords calls TTerminalManager::RecryptPasswords()
- [ ] ChangeMasterPassword always sets decrypt key even if recrypt throws
- [ ] ClearMasterPassword always shreds decrypt key even if recrypt throws
- [ ] Dialog displays errors instead of discarding them

---

## Phase 2: Secure Password Storage (P1)

### Task 2.1: Create TSecureString class

**Files:** Create `src/base/SecureString.h`, `src/base/SecureString.cpp`

**Design:**
- Move-only (non-copyable)
- `VirtualLock` on Windows with graceful fallback
- `SecureZeroMemory` on destruction/clear
- API: `SetValue()`, `GetValue()`, `Clear()`, `IsEmpty()`

**Prior art reference:** `pass_ptrW` in `nbsystem_cpp.h` — same `SecureZeroMemory` pattern.

**Implementation sketch:**

```cpp
#pragma once
#include <Common.h>
#include <vector>

class NB_CORE_EXPORT TSecureString
{
public:
  TSecureString() noexcept = default;
  explicit TSecureString(const UnicodeString & Value);
  ~TSecureString() noexcept;

  TSecureString(const TSecureString &) = delete;
  TSecureString & operator=(const TSecureString &) = delete;
  TSecureString(TSecureString && Other) noexcept;
  TSecureString & operator=(TSecureString && Other) noexcept;

  void SetValue(const UnicodeString & Value);
  UnicodeString GetValue() const;
  void Clear();
  bool IsEmpty() const { return FData.empty(); }

private:
  std::vector<wchar_t> FData;
  void SecureZero();
  static void * SecureAllocate(size_t Size);
  static void SecureFree(void * Ptr, size_t Size);
};
```

**Note:** `VirtualLock` requires `SE_LOCK_MEMORY_NAME` privilege. If `VirtualLock` fails, fall back to normal heap allocation but still `SecureZeroMemory` on free.

---

### Task 2.2: Register TSecureString in CMake build

**File:** `src/CMakeLists.txt`

Add `src/base/SecureString.cpp` to the NetBox plugin target sources.

---

### Task 2.3: Replace UnicodeString with TSecureString in WinConfiguration

**Files:**
- `src/windows/WinConfiguration.h` (lines 480-481)
- `src/windows/WinConfiguration.cpp` (all FPlainMasterPassword* references)
- `src/windows/MasterPassword.cpp`

**Changes:**
1. Replace `UnicodeString FPlainMasterPasswordEncrypt` → `TSecureString FPlainMasterPasswordEncrypt`
2. Replace `UnicodeString FPlainMasterPasswordDecrypt` → `TSecureString FPlainMasterPasswordDecrypt`
3. Update all call sites:
   - `GetMasterKey()` (line ~1931): `FPlainMasterPasswordDecrypt.GetValue()`
   - `ChangeMasterPassword()`: `FPlainMasterPasswordEncrypt.SetValue(value)`
   - `ClearMasterPassword()`: `FPlainMasterPasswordEncrypt.Clear()` (auto-wipes)
   - Any direct assignments

**Acceptance:**
- [ ] No `UnicodeString` members hold master password plaintext
- [ ] `TSecureString::Clear()` / destructor wipes memory
- [ ] All encrypt/decrypt call sites compile
- [ ] Build zero warnings

---

## Phase 3: Thread-Safe Session Counter (P2)

### Task 3.1: Make FMasterPasswordSession atomic

**Files:**
- `src/windows/WinConfiguration.h` (line 517)
- `src/windows/WinConfiguration.cpp` (lines 2020-2033)

**Change:**

```cpp
// In WinConfiguration.h
#include <atomic>
std::atomic<int32_t> FMasterPasswordSession{0};
std::atomic<bool> FMasterPasswordSessionAsked{false};
```

```cpp
// In WinConfiguration.cpp
void TWinConfiguration::BeginMasterPasswordSession()
{
  DebugAssert(FMasterPasswordSession.load() == 0);
  DebugAssert(!FMasterPasswordSessionAsked.load() || (FMasterPasswordSession.load() > 0));
  FMasterPasswordSession.fetch_add(1, std::memory_order_relaxed);
}

void TWinConfiguration::EndMasterPasswordSession()
{
  if (DebugAlwaysTrue(FMasterPasswordSession.load() > 0))
  {
    FMasterPasswordSession.fetch_sub(1, std::memory_order_relaxed);
  }
  FMasterPasswordSessionAsked.store(false, std::memory_order_relaxed);
}
```

**Acceptance:**
- [ ] `std::atomic<int32_t>::is_lock_free()` verified true on x86/x64
- [ ] No data races on session counter
- [ ] Build zero warnings

---

## Phase 4: Rate Limiting (P3)

### Task 4.1: Add validation attempt tracking

**Files:**
- `src/windows/WinConfiguration.h`
- `src/windows/WinConfiguration.cpp` (ValidateMasterPassword, line ~1961)

**Design:**
- Track consecutive failures with `std::atomic<uint32_t>`
- 5 failures → 30-second lockout
- Use `GetTickCount()` for WinXP compatibility (uint32_t wrap safe for 30s intervals)
- Add `bool CountAttempt = true` parameter to skip counting during dialog inline validation

**Implementation sketch:**

```cpp
struct TValidationAttemptTracker
{
  std::atomic<uint32_t> ConsecutiveFailures{0};
  std::atomic<uint32_t> LastAttemptTime{0};
  static constexpr uint32_t MaxAttempts = 5;
  static constexpr uint32_t LockoutSeconds = 30;
};

bool TWinConfiguration::ValidateMasterPassword(UnicodeString value, bool CountAttempt = true)
{
  DebugAssert(GetUseMasterPassword());
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());

  if (CountAttempt)
  {
    const uint32_t Failures = FValidationTracker.ConsecutiveFailures.load();
    if (Failures >= TValidationAttemptTracker::MaxAttempts)
    {
      const uint32_t Now = GetTickCount();
      const uint32_t Last = FValidationTracker.LastAttemptTime.load();
      const uint32_t Elapsed = (Now - Last) / 1000;  // uint32_t wrap safe for < 49.7 days
      if (Elapsed < TValidationAttemptTracker::LockoutSeconds)
      {
        throw Exception(FORMAT(
          L"Too many failed attempts. Please wait %d seconds.",
          TValidationAttemptTracker::LockoutSeconds - Elapsed));
      }
      FValidationTracker.ConsecutiveFailures.store(0);
    }
  }

  bool Result = AES256Verify(value, HexToBytes(FMasterPasswordVerifier));

  if (CountAttempt)
  {
    if (Result)
      FValidationTracker.ConsecutiveFailures.store(0);
    else
    {
      FValidationTracker.ConsecutiveFailures.fetch_add(1);
      FValidationTracker.LastAttemptTime.store(GetTickCount());
    }
  }

  return Result;
}
```

**Acceptance:**
- [ ] 5 consecutive failures trigger 30-second lockout
- [ ] Successful validation resets counter
- [ ] Thread-safe with `std::atomic`
- [ ] Dialog inline validation passes `CountAttempt=false`
- [ ] Build zero warnings

---

## Phase 5: Build Verification

### Task 5.1: Verify zero-warning build

**Command:** `cmd /c build-x64.bat`

**Acceptance:**
- [ ] `Far3_x64/Plugins/NetBox/NetBox.dll` exists
- [ ] Zero MSVC warnings (`/W4`)
- [ ] CRLF line endings preserved
- [ ] No trailing whitespace

---

### Task 5.2: Regression test existing master password flow

**Manual test in Far Manager:**
1. Open NetBox plugin (F11)
2. Navigate to Configuration → Security
3. Set master password
4. Save configuration
5. Restart Far Manager
6. Verify master password prompt appears
7. Enter correct password → sessions load
8. Change master password → verify no errors
9. Clear master password → verify no errors

**Acceptance:**
- [ ] Existing functionality preserved
- [ ] No regressions in password-encrypted sessions

---

## Commit Plan

| Checkpoint | Tasks | Commit Message |
|---|---|---|
| 1 | Task 1.1–1.4 | `feat(win): implement RecryptPasswords for MSVC and fix exception safety` |
| 2 | Task 2.1–2.3 | `feat(base): add TSecureString and secure master password storage` |
| 3 | Task 3.1 | `feat(win): make master password session counter thread-safe` |
| 4 | Task 4.1 | `feat(win): add rate limiting to master password validation` |
| 5 | Task 5.1–5.2 | `chore: build verification and regression testing` |

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| `TSecureString` VirtualLock fails (no privilege) | Medium | Low | Graceful fallback to normal heap |
| `std::atomic` breaks BorlandC build | Low | High | Guard with `#ifdef _MSC_VER` |
| Existing encrypted sessions break after password change | Low | **Critical** | RecryptPasswords must work correctly; test with real sessions |
| `SCOPE_EXIT` macro conflicts with existing exception handlers | Low | Medium | Test all call paths manually |

---

## Dependencies

```
Task 1.2, 1.3 → Task 1.1 (RecryptPasswords must be functional)
Task 2.2 → Task 2.1 (class must exist)
Task 2.3 → Task 2.2 (must be in CMake)
Task 3.1 → None (independent)
Task 4.1 → None (independent)
Task 5.1 → All above (final verification)
Task 5.2 → Task 5.1 (build first, then test)
```

---

## Notes

- **RecryptPasswords** is the highest priority — current stub risks password loss on every master password change/clear
- **TSecureString** should live in `src/base/` for potential reuse by other components
- **Rate limiting** uses `GetTickCount()` to preserve Windows XP compatibility
- **Thread safety** for session counter is critical for parallel file transfers with encrypted passwords
- All changes must pass build with zero warnings (MSVC `/W4`)
- No modifications to `libs/` — use existing patterns only
