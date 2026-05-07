# Plan: Master Password Infrastructure Hardening

**Branch:** feature/master-password-hardening
**Created:** 2026-05-05
**Status:** Complete — core infrastructure implemented, tests added (2026-05-10)

---

## Research Context

**Topic:** Hardening and completing the master password infrastructure

**Goal:** Fix critical security gaps, complete MSVC stub implementation, and harden password storage and validation

**Constraints:**
- Must not break existing password-encrypted sessions
- Must preserve backward compatibility with stored passwords
- Must work on MSVC build (primary target)
- Zero modifications to libs/ (third-party code)

**Decisions:**
- Implement RecryptPasswords for MSVC as priority
- Use RAII secure buffer instead of raw UnicodeString for plaintext passwords
- Add thread-safe session counter with std::atomic
- Add basic rate limiting without external dependencies

**Success signals:**
- Password change/clear operations recrypt all stored passwords correctly
- No plaintext password remains in memory after clear/change
- Thread-safe concurrent access to session counter
- Rate limiting prevents rapid validation attempts

---

**Reference:** [master-password-infrastructure-research](../references/master-password-infrastructure-research.md) — Full WinSCP-to-NetBox comparison, dependency graph, and file map

## Overview


The master password infrastructure in NetBox has critical gaps:
1. **Data Loss Risk**: `RecryptPasswords()` is a stub on MSVC — changing/clearing master password may silently fail to recrypt stored passwords
2. **Security Risk**: Plaintext passwords stored in `UnicodeString` members (`FPlainMasterPasswordEncrypt/Decrypt`) which use reference-counted copy-on-write with no secure wiping guarantees
3. **Thread Safety**: `FMasterPasswordSession` counter is not thread-safe despite being accessed from worker threads during parallel transfers
4. **Brute Force**: No rate limiting on `ValidateMasterPassword()` — rapid attempts could brute-force weak passwords
5. **i18n**: Some dialog messages use hardcoded literals instead of `LoadStr()`

---

## Phase 1: Implement RecryptPasswords for MSVC (P0)

### Task 1: Implement RecryptPasswords in WinConfiguration.cpp

**Files:**
- `src/windows/WinConfiguration.cpp` (stub at line ~4075)

**Description:**
Implement the `RecryptPasswords()` method that iterates all stored sessions and recrypts their passwords when master password is changed or cleared.

**Implementation:**
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

**Acceptance:**
- `RecryptPasswords` implemented for MSVC build
- Handles errors gracefully, collecting them in `RecryptPasswordErrors`
- Iterates all stored sessions
- Does not lose passwords on recryption failure
  - Mirrors existing BorlandC implementation; no manual session iteration needed

---

### Task 1.5: Fix ChangeMasterPassword/ClearMasterPassword exception safety

**Files:**
- `src/windows/MasterPassword.cpp`

**Description:**
`ChangeMasterPassword()` sets `FPlainMasterPasswordEncrypt = value` before calling `RecryptPasswords()`, but only sets `FPlainMasterPasswordDecrypt = value` AFTER. If recryption throws, decrypt key remains the old password. Same pattern in `ClearMasterPassword()` — `Shred(FPlainMasterPasswordDecrypt)` skipped if recrypt throws. WinSCP uses `try/__finally` for both; MSVC needs a scope-guard RAII helper.

**Implementation:**
```cpp
void TWinConfiguration::ChangeMasterPassword(
    UnicodeString value, TStrings * RecryptPasswordErrors) {
  RawByteString Verifier;
  AES256CreateVerifier(value, Verifier);
  FMasterPasswordVerifier = BytesToHex(Verifier);
  FPlainMasterPasswordEncrypt = value;
  FUseMasterPassword = true;
  {
    auto Guard = finally([&]() { FPlainMasterPasswordDecrypt = value; });
    MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  }
}

void TWinConfiguration::ClearMasterPassword(TStrings * RecryptPasswordErrors) {
  FMasterPasswordVerifier = L"";
  FUseMasterPassword = false;
  Shred(FPlainMasterPasswordEncrypt);
  {
    auto Guard = finally([&]() { Shred(FPlainMasterPasswordDecrypt); });
    MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  }
}
```

**Acceptance:**
- If `RecryptPasswords()` throws, `FPlainMasterPasswordDecrypt` is still updated/shredded
- No regression in password change/clear functionality

---

## Phase 2: Secure Password Storage (P1)

### Task 2: Create TSecureString class

**Files:**
- `src/base/SecureString.h` (create)
- `src/base/SecureString.cpp` (create)
- `src/CMakeLists.txt` (update: add to plugin sources)

**Description:**
Create a secure string class that stores data in a locked heap buffer (VirtualLock on Windows) and securely wipes on destruction.
  **Prior art:** `pass_ptrW`/`pass_ptrA` in `src/include/nbsystem_cpp.h` already use `SecureZeroMemory`. `VirtualLock` requires `SE_LOCK_MEMORY_NAME` privilege; implementation must fallback gracefully if unavailable.

**Implementation:**
```cpp
#pragma once

#include <Common.h>
#include <vector>
#include <memory>

class NB_CORE_EXPORT TSecureString
{
public:
  TSecureString() noexcept = default;
  explicit TSecureString(const UnicodeString & Value);
  ~TSecureString() noexcept;
  
  TSecureString(const TSecureString &) = delete;
  TSecureString & operator =(const TSecureString &) = delete;
  TSecureString(TSecureString && Other) noexcept;
  TSecureString & operator =(TSecureString && Other) noexcept;
  
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

**Acceptance:**
- Uses VirtualLock/VirtualUnlock on Windows for memory locking
- SecureZeroMemory on buffer before free
- Non-copyable (move-only)
- RAII destruction guarantees wipe
  - Source files registered in `src/CMakeLists.txt`


---

### Task 2.5: Register TSecureString in CMake build

**Files:**
- `src/CMakeLists.txt`

**Description:**
Add `src/base/SecureString.cpp` to the NetBox plugin target sources so it compiles and links.

**Acceptance:**
- `SecureString.cpp` appears in the plugin source list
- Build passes with zero warnings
---

### Task 3: Replace UnicodeString with TSecureString in TWinConfiguration

**Files:**
- `src/windows/WinConfiguration.h`
- `src/windows/WinConfiguration.cpp`
- `src/windows/MasterPassword.cpp`

**Description:**
Replace `FPlainMasterPasswordEncrypt` and `FPlainMasterPasswordDecrypt` with `TSecureString`.
  **Depends on:** Task 1 (RecryptPasswords must be functional before integration can be tested end-to-end)

**Implementation:**
```cpp
// In WinConfiguration.h
TSecureString FPlainMasterPasswordEncrypt;
TSecureString FPlainMasterPasswordDecrypt;
```

**Changes needed:**
- `SetMasterPassword()`: Store in TSecureString
  - `ChangeMasterPassword()`: Use TSecureString for new password; add RAII guard so FPlainMasterPasswordDecrypt is always set even if RecryptPasswords() throws
  - `ClearMasterPassword()`: Add RAII guard so Shred(FPlainMasterPasswordDecrypt) runs even if RecryptPasswords() throws
- `GetMasterKey()`: Return UnicodeString from TSecureString (transient, caller responsible)
- `ClearMasterPassword()`: TSecureString auto-wipes on clear
- All AES encrypt/decrypt call sites: Convert to/from TSecureString

**Acceptance:**
- No plaintext UnicodeString members for master password
- All existing tests pass
- Password correctly wiped from memory after clear

---

## Phase 3: Thread-Safe Session Counter (P2)

### Task 4: Make FMasterPasswordSession thread-safe

**Files:**
- `src/windows/WinConfiguration.h`
- `src/windows/WinConfiguration.cpp`

**Description:**
Replace `int32_t FMasterPasswordSession` with `std::atomic<int32_t>`.

**Implementation:**
```cpp
// In WinConfiguration.h
#include <atomic>
std::atomic<int32_t> FMasterPasswordSession{0};

**Changes needed:**
- `BeginMasterPasswordSession()`: Use `fetch_add(1)`
- `EndMasterPasswordSession()`: Use `fetch_sub(1)`
- `AskForMasterPassword()`: Compare `load() > 0`
  - Also audit `FMasterPasswordSessionAsked` for concurrent access; protect with `std::atomic<bool>` if needed

**Acceptance:**
- No data races on session counter
- Parallel transfers work correctly
- Atomic operations are lock-free on x86/x64


---

### Task 4.5: Add atomic header and verify lock-free guarantee

**Files:**
- `src/windows/WinConfiguration.h`

**Description:**
Add `#include <atomic>` to `WinConfiguration.h`. Verify `std::atomic<int32_t>::is_lock_free()` is true on MSVC x86/x64 at compile time or runtime assertion.

**Acceptance:**
- `<atomic>` header included where `std::atomic` members are declared
- Runtime assertion confirms lock-free operations on target platforms
---

## Phase 4: Rate Limiting (P3)

### Task 5: Add validation attempt tracking

**Files:**
- `src/windows/WinConfiguration.h`
- `src/windows/WinConfiguration.cpp`

**Description:**
Add rate limiting to `ValidateMasterPassword()` to prevent brute force.

**Implementation:**
```cpp
// In WinConfiguration.h
struct TValidationAttemptTracker
{
  std::atomic<uint32_t> ConsecutiveFailures{0};
  std::atomic<uint32_t> LastAttemptTime{0};
  static constexpr uint32_t MaxAttempts = 5;
  static constexpr uint32_t LockoutSeconds = 30;
};

TValidationAttemptTracker FValidationTracker;
```

**Changes in ValidateMasterPassword():**
```cpp
bool TWinConfiguration::ValidateMasterPassword(UnicodeString value,
    bool CountAttempt = true)
{
  DebugAssert(GetUseMasterPassword());
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());
  
  // Rate limiting check (skipped for inline dialog validation)
  if (CountAttempt)
  {
    const uint32_t Failures = FValidationTracker.ConsecutiveFailures.load();
    if (Failures >= TValidationAttemptTracker::MaxAttempts)
    {
      const uint32_t Now = GetTickCount();
      const uint32_t Last = FValidationTracker.LastAttemptTime.load();
      const uint32_t Elapsed = (Now - Last) / 1000;
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
- 5 consecutive failures trigger 30-second lockout
- Successful validation resets counter
- Lockout uses `GetTickCount()` for Windows XP compatibility
- Thread-safe with std::atomic
- Dialog inline validation passes `CountAttempt=false` to avoid lockout during typing
---

## Phase 5: i18n Cleanup (P4)

### Task 6: Replace hardcoded dialog messages with LoadStr

**Files:**
- `src/NetBox/WinSCPDialogs.cpp`
- `src/windows/UserInterface.cpp` (TMasterPasswordDialog)
- `src/NetBox/FarPluginStrings.cpp` (add mapping entries)

**Description:**
Replace hardcoded literal strings with `LoadStr()` lookups.

**Current hardcoded strings found:**
```cpp
// WinSCPDialogs.cpp:1134
MessageDialog(L"New password cannot be empty.", qtError, qaOK);

// WinSCPDialogs.cpp:1170
MessageDialog(L"Please enter a new master password.", qtError, qaOK);

// WinSCPDialogs.cpp:1204
MessageDialog(L"Please enter current master password.", qtError, qaOK);

**Implementation:**
Add new MsgIDs:
```cpp
NB_MASTER_PASSWORD_EMPTY,
NB_MASTER_PASSWORD_ENTER_NEW,
NB_MASTER_PASSWORD_ENTER_CURRENT,
```

Add to all .lng files:
```
"New password cannot be empty."
"Please enter a new master password."
"Please enter current master password."

Replace literals with:
```cpp
MessageDialog(LoadStr(NB_MASTER_PASSWORD_EMPTY), qtError, qaOK);
MessageDialog(LoadStr(NB_MASTER_PASSWORD_ENTER_NEW), qtError, qaOK);
MessageDialog(LoadStr(NB_MASTER_PASSWORD_ENTER_CURRENT), qtError, qaOK);

**Acceptance:**
- All hardcoded master password dialog messages use LoadStr()
- New strings added to all 5 .lng files
- MsgIDs.h updated
- No missing translations
  - Mapping entries added to `FarPluginStrings.cpp`

---

### Task 6.6: Surface recryption errors in master password dialog

**Files:**
- `src/NetBox/WinSCPDialogs.cpp`

**Description:**
`MasterPasswordConfigurationDialog()` calls `ChangeMasterPassword(NewPwd, nullptr)` and `ClearMasterPassword(nullptr)`, discarding recryption errors. WinSCP collects errors in a `TStringList` and displays them with `MoreMessageDialog`. NetBox must do the same so users know when session passwords failed to recrypt.

**Acceptance:**
- `ChangeMasterPassword`/`ClearMasterPassword` called with a `TStrings *` for error collection
- Errors displayed to user via `MoreMessageDialog` or equivalent


---

### Task 6.5: Add FarPluginStrings mappings for new MsgIDs

**Files:**
- `src/NetBox/FarPluginStrings.cpp`
- `src/base/MsgIDs.h`

**Description:**
Add mapping entries in `FarPluginStrings.cpp` so `GetMsg()` can resolve `NB_MASTER_PASSWORD_ENTER_NEW` and `NB_MASTER_PASSWORD_ENTER_CURRENT`.

**Acceptance:**
- Mapping entries present in the translation table
- Dialogs display correct text at runtime
---

## Phase 6: Testing

### Task 7: Add unit tests for TSecureString

**Files:**
- `tests/core/test_secure_string.cpp` (create)

**Tests:**
- Construction from UnicodeString
- Secure wipe on destruction
- Move semantics (source wiped after move)
- Clear() wipes memory
- IsEmpty() after clear

---

### Task 8: Add integration tests for master password flow

**Files:**
- `tests/integration/test_master_password.cpp` (create)

**Tests:**
- Change master password recrypts all sessions
- Clear master password recrypts all sessions
- Validate with correct password succeeds
- Validate with wrong password fails
- Rate limiting after 5 failures
- Thread-safe session counter with concurrent access


---

### Task 8.5: Register test targets in CMake

**Files:**
- `src/CMakeLists.txt`

**Description:**
Add `add_executable` and `add_test` definitions for `test_secure_string` and `test_master_password` in the `OPT_CREATE_TESTS` block.

**Acceptance:**
- Tests build when `OPT_CREATE_TESTS=ON`
- `ctest` discovers the new tests
---

## Phase 7: Documentation

### Task 9: Update architecture documentation

**Files:**
- `docs/architecture.md` (update with master password section)
- `docs/security.md` (create)

**Content:**
- Master password data flow diagram
- Security considerations
- Thread safety notes
- Rate limiting behavior

---

## Commit Plan

**Checkpoint 1** (after Task 1):
```
feat(win): implement RecryptPasswords for MSVC

- Complete stub implementation in WinConfiguration.cpp
- Iterate all stored sessions and recrypt passwords
- Collect errors without losing data
```

**Checkpoint 2** (after Task 3):
```
feat(base): add TSecureString and use for master password storage

- Create TSecureString with VirtualLock and secure wiping
- Replace FPlainMasterPasswordEncrypt/Decrypt in TWinConfiguration
- Update all encrypt/decrypt call sites
- Register SecureString.cpp in src/CMakeLists.txt
```

**Checkpoint 3** (after Task 4):
```
feat(win): make master password session counter thread-safe

- Replace int32_t with std::atomic<int32_t>
- Use fetch_add/fetch_sub for Begin/End session
```

**Checkpoint 4** (after Task 5):
```
feat(win): add rate limiting to master password validation

- Track consecutive failures with std::atomic
- 5 failures trigger 30-second lockout
- Reset on successful validation
```

**Checkpoint 5** (after Task 6):
```
fix(i18n): replace hardcoded master password dialog strings

- Add NB_MASTER_PASSWORD_ENTER_NEW/ENTER_CURRENT
- Update all .lng files
- Use LoadStr() in all dialog messages
```

**Checkpoint 6** (after Task 8):
```
test: add master password unit and integration tests

- TSecureString tests
- Master password flow integration tests
```

**Final commit** (after Task 9):
```
docs: document master password architecture and security

- Add security section to architecture docs
- Document rate limiting and thread safety
```

---

## Notes

- **RecryptPasswords** is the highest priority — current stub risks password loss
- **TSecureString** should be in `src/base/` for potential reuse by other components
  - **Rate limiting** uses `GetTickCount()` to preserve Windows XP compatibility; 49.7-day wrap-around is safe for 30-second lockout intervals
- **Thread safety** for session counter is critical for parallel file transfers with encrypted passwords
- All changes must pass build with zero warnings (MSVC /W4)
  - **Exception safety** for ChangeMasterPassword/ClearMasterPassword matches WinSCP try/__finally semantics via RAII guards

---

## Changelog

| Date | Change |
|------|--------|
| 2026-05-05 | Initial plan created based on code exploration |
| 2026-05-05 | Refined: ported RecryptPasswords to match BorlandC pattern, added WinXP compatibility, CMake registration, FarPluginStrings mappings, atomic header requirements |
| 2026-05-05 | Second pass (WinSCP UX/backend): added exception-safety fix for Change/ClearMasterPassword, third hardcoded string, dialog error surfacing, rate limiting bypass for inline validation |
