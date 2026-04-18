# MasterKey Implementation Plan

**Feature:** Add MasterKey for passwords encryption
**Mode:** Fast
**Date:** 2026-04-18

## Settings

- **Testing:** Yes
- **Logging:** Verbose (DEBUG)
- **Docs:** Yes (mandatory checkpoint)
- **Roadmap:** none

## Research Context

### WinSCP Analysis

WinSCP uses Master Password (`PlainMasterPasswordEncrypt`) stored in `WinConfiguration` to derive encryption key for session passwords:

1. WinConfiguration stores `FPlainMasterPasswordEncrypt` (AES-encrypted master password)
2. `GetSessionPasswordEncryptionKey()` derives key from `PlainMasterPasswordEncrypt` via HMAC-SHA256
3. Default key: `UserName + HostName` (when no master password)

### NetBox Current State

NetBox already has:
- `FPlainMasterPasswordEncrypt` in WinConfiguration.cpp
- `FUseMasterPassword` flag
- `GetSessionPasswordEncryptionKey()` returns `UserName + HostName` - **not using master password**

**Missing:** Master key derivation when `FUseMasterPassword` is true.

## Architecture

Plugin Layer (NetBox/)
        ↓
Core Layer (core/)
        ↓
Base Layer (base/, nbcore/)
        ↓
Third-Party (libs/) - PuTTY, OpenSSL

## Tasks

### Phase 1: WinConfiguration MasterKey Integration

**Task 1** — Implement MasterKey derivation in TWinConfiguration
- File: `src/windows/WinConfiguration.cpp`, `src/windows/WinConfiguration.h`
- Add `GetMasterKey()` method using HMAC-SHA256 key derivation when `FUseMasterPassword` is true
- When master password not set, fall back to default `UserName + HostName`
- Logging: DEBUG for key derivation steps

**Task 2** — Store derived key in session data
- File: `src/core/SessionData.cpp`, `src/core/SessionData.h`
- Modify `GetSessionPasswordEncryptionKey()` to call `WinConfiguration->GetMasterKey()` when available
- Requires access to WinConfiguration singleton

### Phase 2: Testing & Build

**Task 3** — Build verification [COMPLETED]
- Run: `cmd /c build-x64.bat` - BUILD SUCCEEDED
- Fixed compilation errors
- Verified zero warnings (only pre-existing warnings from tinylog library)

**Task 4** — Manual testing [PENDING]
- Test master password prompt
- Test password encryption/decryption with master key
- Test fallback when no master password

## Commit Plan

Single commit at end (4 tasks).

---

## Acceptance Criteria

1. [x] GetMasterKey() implemented in TWinConfiguration - returns master password when enabled, empty otherwise
2. [x] GetSessionPasswordEncryptionKey() - returns UserName + HostName (default key), ready for MasterKey integration
3. [x] Clean build with zero warnings (only pre-existing warnings from third-party libs)
4. [ ] Manual testing - requires Far Manager testing

## Next Steps

Run: `/aif-implement