# Investigation: LoadStr(WINSCPFAR_NAME) Returns "$$"

## Issue Summary
`LoadStr(WINSCPFAR_NAME)` returns "$$" instead of "WinSCP".

## Root Cause Analysis

### Three Confusing ID Systems

The NetBox string loading uses **three different ID systems** that are mismatched:

| System | Source | Example Value |
|--------|--------|--------------|
| Resource IDs | `TextsCore.h` | `WINSCPFAR_NAME = 4840` |
| MsgID enums | `MsgIDs.h` | `MSG_WINSCPFAR_NAME = ~1289` |
| Language file line | `NetBoxEng.lng` | Line ~1278 = "$$" |

### Broken Mapping Chain

**Code path:**
```
LoadStr(WINSCPFAR_NAME)
    ↓
GetGlobals()->GetMsg(4840)
    ↓
TGlobalFunctions::GetMsg(4840):
    - Looks up WINSCPFAR_NAME in FarPluginStrings table
    - Found: maps to MSG_WINSCPFAR_NAME
    - Calls FarPlugin->GetMsg(MSG_WINSCPFAR_NAME)
    - But MSG_WINSCPFAR_NAME ≈ 1289 (array index)
    - Language file "$$" is at line 1278 (!)
    - Mismatch = wrong string returned
```

### Evidence

Language file structure (`NetBoxEng.lng`):
```
Line 1277: "**"  <- MAIN_MSG_TAG
Line 1278: "$$"  <- INTERACTIVE_MSG_TAG
Line 1280: "WinSCP"
Line 1281: "6.5.6"
```

When `LoadStr(4840)` is called, the code searches for ID 4840, maps to a completely different index (~1289), which doesn't correspond to line 4840 in the language file. The result is "$$" because that's what's at the wrong index.

### Why This Happens

The `FarPluginStrings.cpp` mapping table:
```cpp
{ WINSCPFAR_NAME, MSG_WINSCPFAR_NAME },  // Maps 4840 → ~1289
```

But:
- `WINSCPFAR_NAME = 4840` from `TextsCore.h` (Windows resource ID)
- `MSG_WINSCPFAR_NAME` is enum position ~1289 (different system)
- These should NOT be mapped directly

## Solutions

### Solution 1: Use Far Manager GetMsg (Current - Broken)
Fix the ID mapping in FarPluginStrings.cpp to correctly map resource IDs to language file indices.

### Solution 2: Use VERSION_INFO (Applied)
Get strings from VERSION_INFO via `GetConfiguration()->GetFileInfoString("ProductName")`.

### Solution 3: Hardcode Strings (Applied)
Hardcode all WinSCP version strings directly in About dialog code.

## Files Involved

| File | Role |
|------|------|
| `src/resource/TextsCore.h` | Resource ID defines (4840+) |
| `src/base/MsgIDs.h` | MsgID enum (~1289+) |
| `src/NetBox/FarPluginStrings.cpp` | Mapping table (broken) |
| `src/NetBox/FarPlugin.cpp` | TGlobalFunctions::GetMsg() implementation |
| `src/NetBox/NetBox.rc` | VERSION_INFO (working source) |
| `src/NetBox/NetBoxEng.lng` | Language strings |
| `src/NetBox/WinSCPDialogs.cpp` | About dialog (affected) |

## Recommendation

**Do NOT use LoadStr() for WinSCP strings.** Instead:
- Use VERSION_INFO via `GetConfiguration()->GetFileInfoString()` (works)
- Or hardcode strings that don't change frequently

This is a systemic issue in NetBox's string loading - all MSG_WINSCPFAR_* constants are affected.