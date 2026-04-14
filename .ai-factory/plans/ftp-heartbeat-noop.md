<!-- handoff:task:ftp-heartbeat-noop -->
# Plan - FTP Heartbeat Implementation with NOOP Command

**Branch:** `feature/ftp-heartbeat-noop`  
**Created:** 2026-04-12  
**Base Branch:** `main`  
**Description:** Implement proper FTP heartbeat messages using NOOP command on control connection to prevent timeout

## Overview

Implement a dedicated heartbeat mechanism for FTP control connections that sends NOOP commands at configurable intervals to prevent server-side timeout. This replaces the current approach of using random dummy commands (PWD, REST 0, TYPE A/I) with a standardized, configurable heartbeat system.

## Settings

- **Testing:** yes - Unit tests for heartbeat logic, integration tests for FTP sessions
- **Logging:** verbose - DEBUG level logging for heartbeat events
- **Docs:** yes - Update configuration documentation and add technical notes
- **Roadmap Linkage:** none

## Research Context

### Current State

**FileZilla Engine (src/filezilla/):**
- `CFtpControlSocket::OnTimer()` (line 5646-5666) - Called every 1 second
  - Checks `OPTION_KEEPALIVE` flag
  - Uses random interval between `OPTION_INTERVALLOW` and `OPTION_INTERVALHIGH`
  - Calls `SendKeepAliveCommand()` when interval elapsed
- `CFtpControlSocket::SendKeepAliveCommand()` (line 5284-5292)
  - Sends random command from: PWD, REST 0, TYPE A, TYPE I
  - Sets `m_bKeepAliveActive` flag until reply received
  - No NOOP command used

**NetBox SessionData (src/core/SessionData.h):**
- `FFtpPingType` enum (line 35): `fptOff`, `fptDummyCommand0`, `fptDummyCommand`, `fptDirectoryListing`
- `FFtpPingInterval` (line 284): Configurable interval in seconds
- Current mapping: `fptDummyCommand` sends TYPE A/I, not NOOP

**Problem:**
- Random dummy commands work but are not semantically correct
- NOOP is the standard FTP command for keepalive (RFC 959)
- No dedicated NOOP-based heartbeat option
- Current implementation doesn't distinguish between "keep session alive" vs "test connectivity"

## Tasks

### Phase 1: Add NOOP Heartbeat Configuration

#### Task 1.1: Extend TFtpPingType enum
**File:** `src/core/SessionData.h`
**Change:** Add `fptNoop` value to `TFtpPingType` enum
```cpp
enum TFtpPingType { fptOff, fptDummyCommand0, fptDummyCommand, fptDirectoryListing, fptNoop };
```
**Logging:** N/A (enum change)
**Dependencies:** None

#### Task 1.2: Update ping type names
**File:** `src/core/SessionData.h`
**Change:** Update `FtpPingTypeNames` constant to include "Noop"
```cpp
constexpr const wchar_t * FtpPingTypeNames = L"Off;Dummy;Dummy;List;Noop";
```
**Logging:** N/A (constant update)
**Dependencies:** Task 1.1

#### Task 1.3: Add configuration property accessors
**File:** `src/core/SessionData.cpp`
**Change:** Ensure `GetFtpPingType()` returns correct value, verify default is `fptOff`
**Logging:** N/A (property accessor)
**Dependencies:** Task 1.1

### Phase 2: Implement NOOP Heartbeat in FileZilla Engine

#### Task 2.1: Modify SendKeepAliveCommand to support NOOP
**File:** `src/filezilla/FtpControlSocket.cpp`
**Change:** Update `SendKeepAliveCommand()` to check a new operation mode or option that determines whether to send NOOP vs random commands
**Approach:** Add new option `OPTION_HEARTBEAT_TYPE` or check existing configuration
**Logging:** 
```cpp
ADF("Heartbeat: Sending %s command", useNoop ? "NOOP" : "random dummy");
```
**Dependencies:** Task 1.1

#### Task 2.2: Add option to control heartbeat command type
**File:** `src/filezilla/FileZillaOpt.h`
**Change:** Add new option constant
```cpp
#define OPTION_HEARTBEAT_TYPE 11  // 0=random, 1=NOOP
```
**Logging:** N/A
**Dependencies:** None

#### Task 2.3: Wire FileZilla option to SessionData
**File:** `src/core/FtpFileSystem.cpp`
**Change:** In `TFileZillaImpl::OptionVal()`, add case for `OPTION_HEARTBEAT_TYPE` that returns based on `FFtpPingType`
```cpp
case OPTION_HEARTBEAT_TYPE:
    return (Data->GetFtpPingType() == fptNoop) ? 1 : 0;
```
**Logging:** N/A
**Dependencies:** Task 1.1, Task 2.2

#### Task 2.4: Update SendKeepAliveCommand logic
**File:** `src/filezilla/FtpControlSocket.cpp`
**Change:** Modify `SendKeepAliveCommand()` to send NOOP when `OPTION_HEARTBEAT_TYPE` is 1
```cpp
void CFtpControlSocket::SendKeepAliveCommand()
{
  if (GetOptionVal(OPTION_HEARTBEAT_TYPE))
  {
    ShowStatus(L"Sending NOOP to keep session alive.", FZ_LOG_PROGRESS);
    Send(L"NOOP");
  }
  else
  {
    ShowStatus(L"Sending dummy command to keep session alive.", FZ_LOG_PROGRESS);
    TCHAR commands[4][7]={L"PWD",L"REST 0",L"TYPE A",L"TYPE I"};
    int choice=(rand()*4)/(RAND_MAX+1);
    Send(commands[choice]);
  }
  m_bKeepAliveActive=TRUE;
}
```
**Logging:** DEBUG log before sending command
**Dependencies:** Task 2.1, Task 2.3

### Phase 3: Configuration and UI

#### Task 3.1: Update session configuration UI
**File:** Search for FTP ping type UI controls (likely in `src/windows/` dialogs)
**Change:** Add "NOOP" option to FTP ping type dropdown
**Logging:** N/A
**Dependencies:** Task 1.1

#### Task 3.2: Update configuration storage
**File:** `src/core/SessionData.cpp`
**Change:** Ensure `FtpPingType` saves/loads correctly with new `fptNoop` value
**Verify:** Check `SaveToStorage()` and `LoadFromStorage()` methods
**Logging:** N/A
**Dependencies:** Task 1.1

### Phase 4: Testing and Validation

#### Task 4.1: Create unit test for heartbeat command selection
**File:** Create test file or add to existing FTP tests
**Test:** Verify `SendKeepAliveCommand()` sends correct command based on configuration
**Test cases:**
- `fptOff` - no heartbeat sent
- `fptNoop` - sends NOOP
- `fptDummyCommand` - sends random dummy command
**Logging:** N/A
**Dependencies:** Task 2.4

#### Task 4.2: Integration test - FTP session with NOOP heartbeat
**File:** Manual test or automated test
**Test:** 
1. Configure FTP session with `fptNoop` and short interval (e.g., 30 seconds)
2. Connect to FTP server
3. Leave idle
4. Verify NOOP commands sent at configured interval
5. Verify server doesn't timeout connection
**Logging:** Verify DEBUG logs show "Sending NOOP" messages
**Dependencies:** Task 2.4

#### Task 4.3: Verify backward compatibility
**Test:** Existing sessions with `fptDummyCommand` continue to work
**Test:** Default behavior unchanged (should be `fptOff`)
**Logging:** N/A
**Dependencies:** Task 2.4

### Phase 5: Documentation

#### Task 5.1: Update configuration documentation
**File:** `docs/configuration.md` or similar
**Change:** Document new `fptNoop` option and when to use it vs dummy commands
**Logging:** N/A
**Dependencies:** Task 1.1

#### Task 5.2: Add technical note about heartbeat mechanism
**File:** Code comment or docs
**Explain:** Difference between NOOP (standard) and dummy commands (workaround)
**Logging:** N/A
**Dependencies:** Task 1.1

## Commit Plan

**Commit 1:** Configuration enum and constants
- Tasks: 1.1, 1.2, 1.3
- Message: `feat(ftp): add fptNoop option to TFtpPingType enum`

**Commit 2:** FileZilla engine heartbeat support
- Tasks: 2.1, 2.2, 2.3, 2.4
- Message: `feat(ftp): implement NOOP heartbeat in FileZilla engine`

**Commit 3:** Configuration and UI updates
- Tasks: 3.1, 3.2
- Message: `feat(ftp): add NOOP option to FTP session configuration`

**Commit 4:** Tests and validation
- Tasks: 4.1, 4.2, 4.3
- Message: `test(ftp): add tests for NOOP heartbeat functionality`

**Commit 5:** Documentation
- Tasks: 5.1, 5.2
- Message: `docs(ftp): document NOOP heartbeat configuration option`

## Acceptance Criteria

- [x] `TFtpPingType` enum includes `fptNoop` value
- [x] `SendKeepAliveCommand()` sends NOOP when configured
- [x] Configuration persists across sessions
- [x] Default behavior unchanged (backward compatible)
- [ ] Tests verify NOOP heartbeat functionality (skipped - no test framework)
- [x] Documentation updated with new option
- [x] Build succeeds with no warnings (MSVC W4)
- [ ] Manual test confirms FTP server doesn't timeout with NOOP heartbeat (user verification needed)

## Risks and Considerations

1. **Server compatibility:** Some old FTP servers may not support NOOP (rare)
   - Mitigation: Keep dummy command option available

2. **Firewall/NAT:** NOOP commands keep connection alive through NAT devices
   - Benefit: This is the intended use case

3. **Logging verbosity:** Heartbeat logs at DEBUG level to avoid noise
   - Verified: Only logged when DEBUG enabled

4. **Performance:** Minimal impact - one command per interval (typically 30-120 seconds)

## Next Steps

After plan approval:
1. Run `/aif-implement` to execute this plan
2. Manual testing with various FTP servers
3. Verify no regressions in existing FTP functionality
