# Master Password Infrastructure Research

> Source: WinSCP 6.5.6 @ `/run/media/user/SSD1TB/projects/WinSCP-work/winscp-master/source/`
> Target: NetBox MSVC build @ `src/windows/`, `src/core/`, `src/NetBox/`
> Date: 2026-05-05

## Research Goal

Compare WinSCP's master password backend and UX patterns against NetBox's MSVC implementation to identify:
- Missing functionality (the stub `RecryptPasswords`)
- Exception-safety regressions
- Hardcoded vs localized strings
- Thread-safety gaps
- Rate limiting feasibility

## WinSCP Reference Architecture

### Key Files (WinSCP)

| File | Responsibility |
|------|---------------|
| `windows/WinConfiguration.h/cpp` | `SetMasterPassword`, `ChangeMasterPassword`, `ClearMasterPassword`, `ValidateMasterPassword`, `GetMasterKey`, `AskForMasterPassword`, `BeginMasterPasswordSession`/`EndMasterPasswordSession`, `RecryptPasswords` |
| `windows/CustomWinConfiguration.h/cpp` | Base `RecryptPasswords` — delegates to `StoredSessions->RecryptPasswords()` then fires `OnMasterPasswordRecrypt` callback |
| `core/SessionData.h/cpp` | `TSessionData::RecryptPasswords()` — recrypts individual session passwords; `TStoredSessionList::RecryptPasswords()` — delegates to `DoSave(true, true, true, ...)` |
| `core/Terminal.h/cpp` | `TTerminal::RecryptPasswords()` — recrypts session + remembered passwords; `TTerminalList::RecryptPasswords()` — iterates terminals |
| `windows/TerminalManager.h/cpp` | Inherits `TTerminalList` — `RecryptPasswords()` available via inheritance |
| `windows/UserInterface.cpp` | `TMasterPasswordDialog` — proper dialog with `DoValidate()`, `DoChange()`, password strength check |
| `forms/Preferences.cpp` | `ChangeMasterPassword()`, `UseMasterPasswordCheckClick()` — collects recrypt errors with `TStringList`, displays with `MoreMessageDialog` |

### WinSCP RecryptPasswords Call Chain

```
TWinConfiguration::RecryptPasswords(Errors)
  → TCustomWinConfiguration::RecryptPasswords(Errors)
    → StoredSessions->RecryptPasswords(Errors)
      → TStoredSessionList::DoSave(true, true, true, Errors)
    → OnMasterPasswordRecrypt(nullptr)  // callback for extensions
  → TTerminalManager::Instance(false)->RecryptPasswords()
    → TTerminalList::RecryptPasswords()  // inherited
      → for each TTerminal: RecryptPasswords()
        → FSessionData->RecryptPasswords()
        → FRememberedPassword = EncryptPassword(...)
        → FRememberedTunnelPassword = EncryptPassword(...)
```

### WinSCP Exception Safety Pattern

```cpp
// ChangeMasterPassword — WinSCP (correct)
FPlainMasterPasswordEncrypt = value;
try {
  RecryptPasswords(RecryptPasswordErrors);
}
__finally {
  FPlainMasterPasswordDecrypt = value;  // always updated
}

// ClearMasterPassword — WinSCP (correct)
Shred(FPlainMasterPasswordEncrypt);
try {
  RecryptPasswords(RecryptPasswordErrors);
}
__finally {
  Shred(FPlainMasterPasswordDecrypt);  // always shredded
}
```

### WinSCP AskForMasterPassword Flow

```
AskForMasterPassword()
  → if FMasterPasswordSession > 0:
    → if FMasterPasswordSessionAsked: Abort()  // avoid double prompt
    → FMasterPasswordSessionAsked = true
  → if FOnMasterPasswordPrompt == NULL: throw
  → FOnMasterPasswordPrompt()  // callback → TTerminalManager::MasterPasswordPrompt()
    → if on main thread: DoMasterPasswordDialog() + SetMasterPassword()
    → else: PostMessage to main thread
```

## NetBox Current State — Gaps Found

### Gap 1: RecryptPasswords is a stub (P0 — Data Loss Risk)

**File:** `src/windows/WinConfiguration.cpp:4074-4078`

```cpp
// MSVC stub — does nothing
void TWinConfiguration::RecryptPasswords(TStrings * /*RecryptPasswordErrors*/)
{
  // TODO: Implement full RecryptPasswords for MSVC build
}
```

The BorlandC implementation at lines 982-1005 works correctly. Port to MSVC:
```cpp
void TWinConfiguration::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  TCustomWinConfiguration::RecryptPasswords(RecryptPasswordErrors);
  try {
    TTerminalManager * Manager = TTerminalManager::Instance(false);
    DebugAssert(Manager != nullptr);
    if (Manager != nullptr)
      Manager->RecryptPasswords();
  }
  catch (Exception & E) {
    UnicodeString Message;
    if (ExceptionMessage(&E, Message))
      RecryptPasswordErrors->Add(Message);
  }
}
```

`TTerminalManager` exists in MSVC builds and inherits `RecryptPasswords()` from `TTerminalList` (confirmed at `src/core/Terminal.cpp:9905`).

### Gap 2: ChangeMasterPassword/ClearMasterPassword Exception Safety (P0)

**File:** `src/windows/MasterPassword.cpp:19-27`, `36-41`

NetBox's `ChangeMasterPassword()` sets `FPlainMasterPasswordEncrypt` before
recrypting but only sets `FPlainMasterPasswordDecrypt` AFTER. If recryption throws,
the decrypt key is stale. Same issue in `ClearMasterPassword()` for the `Shred` call.

Fix: Use RAII scope guard (MSVC lacks `__finally`):
```cpp
auto Guard = finally([&]() { FPlainMasterPasswordDecrypt = value; });
MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
```

### Gap 3: Plaintext Passwords in UnicodeString (P1)

**File:** `src/windows/WinConfiguration.h:480-481`

```cpp
UnicodeString FPlainMasterPasswordEncrypt;
UnicodeString FPlainMasterPasswordDecrypt;
```

`UnicodeString` uses reference-counted copy-on-write with no secure wiping
guarantees. `Shred()` exists (`src/base/Common.cpp:746`) and is already used
in `ClearMasterPassword()`, but only provides best-effort zeroing — copies may
remain from COW semantics.

**Prior art:** `pass_ptrW`/`pass_ptrA` in `src/include/nbsystem_cpp.h:61-85`
already use `SecureZeroMemory`.

Fix: Create `TSecureString` in `src/base/` with `VirtualLock` (graceful fallback
if `SE_LOCK_MEMORY_NAME` unavailable) and `SecureZeroMemory`.

### Gap 4: Thread-Unsafe Session Counter (P2)

**File:** `src/windows/WinConfiguration.h:517-518`

```cpp
int32_t FMasterPasswordSession;
bool FMasterPasswordSessionAsked;
```

Both are accessed from worker threads during parallel transfers. `BeginMasterPasswordSession`
(line 2023) and `EndMasterPasswordSession` (line 2030) are non-atomic.

Fix: Replace with `std::atomic<int32_t>` and `std::atomic<bool>`.

### Gap 5: No Rate Limiting (P3)

**File:** `src/windows/WinConfiguration.cpp:1928-1933` (NetBox), `WinSCP WinConfiguration.cpp:1955-1961`

Neither WinSCP nor NetBox rate-limits `ValidateMasterPassword()`.
Add tracking with 5 failures → 30-second lockout.

**Constraint:** Use `GetTickCount()` (not `GetTickCount64()`) for WinXP compatibility.
49.7-day wrap-around is safe for 30-second lockout intervals (`uint32_t` subtraction
handles wrap correctly for intervals < 49.7 days).

**Dialog interaction:** `ValidateMasterPassword` is called from dialog inline
validation (lines 1137, 1207 in `WinSCPDialogs.cpp`). Add `bool CountAttempt = true`
parameter to skip counting during dialog typing.

### Gap 6: Hardcoded Dialog Strings (P4)

**File:** `src/NetBox/WinSCPDialogs.cpp`

Three hardcoded string literals (not using `LoadStr()`):

| Line | String | Needs MsgID |
|------|--------|-------------|
| 1134 | `"New password cannot be empty."` | `NB_MASTER_PASSWORD_EMPTY` |
| 1170 | `"Please enter a new master password."` | `NB_MASTER_PASSWORD_ENTER_NEW` |
| 1204 | `"Please enter current master password."` | `NB_MASTER_PASSWORD_ENTER_CURRENT` |

Must update: `src/base/MsgIDs.h`, 5 × `.lng` files, `src/NetBox/FarPluginStrings.cpp`.

### Gap 7: Recryption Errors Silently Discarded (P4)

**File:** `src/NetBox/WinSCPDialogs.cpp:1155`, `1214`

`ChangeMasterPassword(NewPwd, nullptr)` and `ClearMasterPassword(nullptr)` discard
errors. WinSCP collects errors in `TStringList` and shows `MoreMessageDialog`.

## NetBox Files Touched

| File | Tasks | Type |
|------|-------|------|
| `src/windows/WinConfiguration.cpp` | 1 (RecryptPasswords stub) | Replace stub |
| `src/windows/WinConfiguration.h` | 4 (atomic), 5 (rate limiting) | Modify members |
| `src/windows/MasterPassword.cpp` | 1.5 (exception safety) | Add RAII guards |
| `src/base/SecureString.h/cpp` | 2 (new class) | Create |
| `src/CMakeLists.txt` | 2.5 (register sources), 8.5 (test targets) | Modify |
| `src/NetBox/WinSCPDialogs.cpp` | 6 (i18n), 6.6 (error surfacing) | Modify |
| `src/base/MsgIDs.h` | 6 (new MsgIDs) | Modify |
| `src/NetBox/FarPluginStrings.cpp` | 6.5 (mapping table) | Modify |
| `src/NetBox/NetBox*.lng` (5 files) | 6 (i18n) | Modify |
| `tests/core/test_secure_string.cpp` | 7 (unit tests) | Create |
| `tests/integration/test_master_password.cpp` | 8 (integration) | Create |
| `docs/architecture.md` | 9 | Modify |
| `docs/security.md` | 9 | Create |

## WinSCP UX Patterns — For Reference

### Dialog Structure

WinSCP uses two separate dialogs:
- `DoMasterPasswordDialog()` — for clearing: prompts for current password only
- `DoChangeMasterPasswordDialog()` — for changing: prompts for current + new + confirm

Both delegate to `DoMasterPasswordDialog(bool Current, ...)` → `TMasterPasswordDialog`.

NetBox uses one combined dialog (`TWinSCPPlugin::MasterPasswordConfigurationDialog()`)
that handles enable/disable + change + set in a single Far dialog with a checkbox
toggle. Both approaches are valid for their respective UI frameworks.

### Validation Flow

WinSCP: `DoValidate()` throws `Exception(MainInstructions(LoadStr(...)))` on failure
→ dialog framework catches and displays inline error.

NetBox: Validates after `ShowModal()` returns → `MessageDialog()` on failure.
Far Manager dialogs don't support inline validation exceptions.

### Error Display

WinSCP:
```cpp
std::unique_ptr<TStrings> RecryptPasswordErrors(new TStringList());
WinConfiguration->ChangeMasterPassword(NewPassword, RecryptPasswordErrors.get());
MasterPasswordChanged(Message, RecryptPasswordErrors.get());
// MasterPasswordChanged → MoreMessageDialog with error list + help link
```

## Thread Safety Notes

- `FMasterPasswordSession` is incremented in `BeginMasterPasswordSession()` from
  the main thread and decremented in `EndMasterPasswordSession()` from worker threads
  during parallel transfers. This is a real data race.
- `FMasterPasswordSessionAsked` is read/written in `AskForMasterPassword()` which
  can be called from either thread.
- WinSCP has the same non-atomic `int` members — this is a latent bug in both codebases.
- `std::atomic<int32_t>::is_lock_free()` is `true` on all MSVC x86/x64 targets.

## Dependencies Between Tasks

```
Task 1 (RecryptPasswords stub)
  ↓
Task 1.5 (exception safety in MasterPassword.cpp)
  ↓
Task 2 (TSecureString class)  ←── Task 2.5 (CMake registration)
  ↓
Task 3 (replace UnicodeString) ←── depends on Task 1, Task 1.5
  ↓ (independent of 4/5)
Task 4 (atomic counter) ←── Task 4.5 (atomic header)
Task 5 (rate limiting)
  ↓
Task 6 (i18n strings) ←── Task 6.5 (FarPluginStrings), Task 6.6 (error surfacing)
  ↓
Task 7 (TSecureString unit tests)
Task 8 (integration tests) ←── Task 8.5 (CMake test targets)
  ↓
Task 9 (documentation)
```

## Commit Plan

1. `feat(win): implement RecryptPasswords for MSVC` — Tasks 1, 1.5
2. `feat(base): add TSecureString and use for master password` — Tasks 2, 2.5, 3
3. `feat(win): make master password session counter thread-safe` — Tasks 4, 4.5
4. `feat(win): add rate limiting to master password validation` — Task 5
5. `fix(i18n): replace hardcoded master password dialog strings` — Tasks 6, 6.5, 6.6
6. `test: add master password unit and integration tests` — Tasks 7, 8, 8.5
7. `docs: document master password architecture and security` — Task 9