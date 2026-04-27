## Summary
Analysis of version substitution bug in About dialog where placeholder issues.

## Investigation Steps

### Root Cause Discovered
1. **Missing ID mappings**: NB_ABOUT_VERSION is used in code but NOT mapped in FarPluginStrings.cpp mapping table.
2. **LoadStr returning wrong strings**: Even WINSCPFAR_NAME returns wrong value ($$ instead of WinSCP) - likely array index mismatch.
3. When GetMsg(NB_ABOUT_VERSION) is called with unmapped ID, it uses numeric value as array index, which returns wrong string "$$".

### Fix Applied
1. Line ~1303-1321: Changed ALL WinSCP version strings to hardcoded literals:
   - "Based on WinSCPFar: SFTP/FTP/SCP client for Far 1.75"
   - "Based on WinSCP version 6.5.6" 
   - "Copyright (c) 2000-2025 Martin Prikryl"

## Files Modified
- `src/NetBox/WinSCPDialogs.cpp`:
  - Lines ~1298-1321: Hardcoded all version strings

## Build Status
- Compilation: ✓
- DLL: build-RelWithDebugInfo/src/NetBox.dll (12MB)