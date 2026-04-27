# About Dialog Version Fix

> **Exploration:** [.ai-factory/references/about-dialog-version-fix.md](../.ai-factory/references/about-dialog-version-fix.md)
> **Message System:** [.ai-factory/references/message-loading-system.md](../.ai-factory/references/message-loading-system.md) — GetMsg/FmtLoadStr resolution, ID mapping tables, missing ID bug

## Issue
About dialog showed:
- "Based on WinSCPFar: SFTP/FTP/SCP client for Far 1.75" ✓
- "Based on $$ version WinSCP" - $$ not substituted
- "Version %s" - %s not filled

## Root Cause
Line 1307 in WinSCPDialogs.cpp:
```cpp
FMTLOAD(WINSCPFAR_BASED_VERSION, LoadStr(WINSCPFAR_VERSION))
```
Format string needs TWO arguments: "Based on %s version %s"
But only one argument passed.

## Fix

### All WinSCP Strings (~lines 1303-1321)
```cpp
// Before (broken - LoadStr returns $$ or wrong values):
LoadStr(WINSCPFAR_BASED_ON)
FMTLOAD(WINSCPFAR_BASED_VERSION, ...)
LoadStr(WINSCPFAR_BASED_COPYRIGHT)

// After (hardcoded literals):
L"Based on WinSCPFar: SFTP/FTP/SCP client for Far 1.75"
L"Based on WinSCP version 6.5.6"
L"Copyright (c) 2000-2025 Martin Prikryl"
```

## Status
- Compiled ✓
- **Need close Far Manager to copy DLL**

Manual copy if Far locked:
```
copy build-RelWithDebugInfo\src\NetBox.dll Far3_x64\Plugins\NetBox\NetBox.dll
```