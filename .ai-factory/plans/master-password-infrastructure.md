# Plan: Master Password Infrastructure Hardening

**Branch:** feature/master-password-hardening
**Created:** 2026-05-05
**Status:** Ready for implementation

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
  try
  {
    // Recrypt all stored session passwords
    const TStoredSessionList * Sessions = GetStoredSessions();
    for (int32_t Index = 0; Index < Sessions->Count; ++Index)
    {
      TSessionData * SessionData = Sessions->Sessions[Index];
      if (SessionData->HasAnyPassword())
      {
        try
        {
          SessionData->RecryptPasswords();
        }
        catch (Exception & E)
        {
          if (RecryptPasswordErrors != nullptr)
          {
            RecryptPasswordErrors->Add(
              FORMAT(L"%s: %s", SessionData->SessionName, E.Message));
          }
        }
      }
    }
    
    // Recrypt any other encrypted data (bookmarks, etc.)
    // TODO: Extend as needed for other encrypted fields
  }
  catch (Exception & E)
  {
    if (RecryptPasswordErrors != nullptr)
    {
      RecryptPasswordErrors->Add(E.Message);
    }
    throw;
  }
}
```

**Acceptance:**
- `RecryptPasswords` implemented for MSVC build
- Handles errors gracefully, collecting them in `RecryptPasswordErrors`
- Iterates all stored sessions
- Does not lose passwords on recryption failure

---

## Phase 2: Secure Password Storage (P1)

### Task 2: Create TSecureString class

**Files:**
- `src/base/SecureString.h` (create)
- `src/base/SecureString.cpp` (create)

**Description:**
Create a secure string class that stores data in a locked heap buffer (VirtualLock on Windows) and securely wipes on destruction.

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

---

### Task 3: Replace UnicodeString with TSecureString in TWinConfiguration

**Files:**
- `src/windows/WinConfiguration.h`
- `src/windows/WinConfiguration.cpp`
- `src/windows/MasterPassword.cpp`

**Description:**
Replace `FPlainMasterPasswordEncrypt` and `FPlainMasterPasswordDecrypt` with `TSecureString`.

**Implementation:**
```cpp
// In WinConfiguration.h
TSecureString FPlainMasterPasswordEncrypt;
TSecureString FPlainMasterPasswordDecrypt;
```

**Changes needed:**
- `SetMasterPassword()`: Store in TSecureString
- `ChangeMasterPassword()`: Use TSecureString for new password
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
std::atomic<int32_t> FMasterPasswordSession{0};
```

**Changes needed:**
- `BeginMasterPasswordSession()`: Use `fetch_add(1)`
- `EndMasterPasswordSession()`: Use `fetch_sub(1)`
- `AskForMasterPassword()`: Compare `load() > 0`

**Acceptance:**
- No data races on session counter
- Parallel transfers work correctly
- Atomic operations are lock-free on x86/x64

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
  std::atomic<uint64_t> LastAttemptTime{0};
  static constexpr uint32_t MaxAttempts = 5;
  static constexpr uint32_t LockoutSeconds = 30;
};

TValidationAttemptTracker FValidationTracker;
```

**Changes in ValidateMasterPassword():**
```cpp
bool TWinConfiguration::ValidateMasterPassword(UnicodeString value)
{
  DebugAssert(GetUseMasterPassword());
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());
  
  // Rate limiting check
  const uint32_t Failures = FValidationTracker.ConsecutiveFailures.load();
  if (Failures >= TValidationAttemptTracker::MaxAttempts)
  {
    const uint64_t Now = GetTickCount64();
    const uint64_t Last = FValidationTracker.LastAttemptTime.load();
    const uint64_t Elapsed = (Now - Last) / 1000;
    if (Elapsed < TValidationAttemptTracker::LockoutSeconds)
    {
      throw Exception(FORMAT(
        L"Too many failed attempts. Please wait %d seconds.",
        TValidationAttemptTracker::LockoutSeconds - Elapsed));
    }
    // Reset after lockout period
    FValidationTracker.ConsecutiveFailures.store(0);
  }
  
  bool Result = AES256Verify(value, HexToBytes(FMasterPasswordVerifier));
  
  if (Result)
  {
    FValidationTracker.ConsecutiveFailures.store(0);
  }
  else
  {
    FValidationTracker.ConsecutiveFailures.fetch_add(1);
    FValidationTracker.LastAttemptTime.store(GetTickCount64());
  }
  
  return Result;
}
```

**Acceptance:**
- 5 consecutive failures trigger 30-second lockout
- Successful validation resets counter
- Lockout uses existing `GetTickCount64()` (Windows API)
- Thread-safe with std::atomic

---

## Phase 5: i18n Cleanup (P4)

### Task 6: Replace hardcoded dialog messages with LoadStr

**Files:**
- `src/NetBox/WinSCPDialogs.cpp`
- `src/windows/UserInterface.cpp` (TMasterPasswordDialog)

**Description:**
Replace hardcoded literal strings with `LoadStr()` lookups.

**Current hardcoded strings found:**
```cpp
// WinSCPDialogs.cpp:1170
MessageDialog(L"Please enter a new master password.", qtError, qaOK);

// WinSCPDialogs.cpp:1204
MessageDialog(L"Please enter current master password.", qtError, qaOK);
```

**Implementation:**
Add new MsgIDs:
```cpp
NB_MASTER_PASSWORD_ENTER_NEW,
NB_MASTER_PASSWORD_ENTER_CURRENT,
```

Add to all .lng files:
```
"Please enter a new master password."
"Please enter current master password."
```

Replace literals with:
```cpp
MessageDialog(LoadStr(NB_MASTER_PASSWORD_ENTER_NEW), qtError, qaOK);
MessageDialog(LoadStr(NB_MASTER_PASSWORD_ENTER_CURRENT), qtError, qaOK);
```

**Acceptance:**
- All hardcoded master password dialog messages use LoadStr()
- New strings added to all 5 .lng files
- MsgIDs.h updated
- No missing translations

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
- **Rate limiting** uses `GetTickCount64()` which requires Windows Vista+ (NetBox already requires this)
- **Thread safety** for session counter is critical for parallel file transfers with encrypted passwords
- All changes must pass build with zero warnings (MSVC /W4)

---

## Changelog

| Date | Change |
|------|--------|
| 2026-05-05 | Initial plan created based on code exploration |
