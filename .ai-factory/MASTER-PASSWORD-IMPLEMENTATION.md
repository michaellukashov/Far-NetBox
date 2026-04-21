# Master Password Implementation Status

> Last updated: 2026-04-21

## Current State

**Build compiles but linking fails** with unresolved externals:
- `TWinConfiguration::RecryptPasswords(TStrings*)` 
- `WinConfiguration` (global instance)

## Work Completed

### 1. MasterPassword.h (created)
- Forward declaration for `TWinConfiguration`

### 2. MasterPassword.cpp (created)
- Implements master password methods for `TWinConfiguration`:
  - `GetUseMasterPassword()`
  - `ChangeMasterPassword()`
  - `ValidateMasterPassword()`
  - `ClearMasterPassword()`

### 3. WinConfiguration.h (modified)
- Removed stub `class TCustomWinConfiguration {};` that was hiding real class
- Made `RecryptPasswords()` and `GetUseMasterPassword()` public in `TWinConfiguration`

### 4. CustomWinConfiguration.h (modified)
- Made `RecryptPasswords()` public in `TCustomWinConfiguration`

### 5. Build infrastructure
- Created `build-x64.bat` script
- Added NASM to `buildtools/tools/`

## Remaining Issues

### 1. Undefined RecryptPasswords implementation
`TWinConfiguration::RecryptPasswords()` is declared but not implemented. Need to add implementation in `WinConfiguration.cpp` or call base class.

### 2. Undefined global WinConfiguration
The global instance `WinConfiguration` is referenced in `WinSCPDialogs.cpp` but not defined.

## Solution Path

1. **For RecryptPasswords**: Either implement in `WinConfiguration.cpp` or remove the `TWinConfiguration` override (use base class implementation)

2. **For WinConfiguration instance**: Define global `WinConfiguration` instance - likely in `WinConfiguration.cpp`

## Files Modified

| File | Change |
|------|--------|
| `src/windows/MasterPassword.h` | Created |
| `src/windows/MasterPassword.cpp` | Created |
| `src/windows/WinConfiguration.h` | Removed stub, made methods public |
| `src/windows/CustomWinConfiguration.h` | Made RecryptPasswords public |
| `build-x64.bat` | Created |
| `buildtools/tools/nasm.exe` | Added |

## WinSCP Reference

- Source: `D:\Projects\WinSCP-work\winscp-master\source\windows\`
- Key files: `WinConfiguration.h`, `WinConfiguration.cpp`, `MasterPassword.cpp`