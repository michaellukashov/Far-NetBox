# Investigation: MsgIDs.h vs NetBoxEng.lng Index Mismatch

## Issue Summary
The enum indices in `MsgIDs.h` do NOT match the string indices in `NetBoxEng.lng`. This causes `LoadStr()` to return wrong strings.

## Current State

### Language File (`NetBoxEng.lng`)
- Total lines: 1332 (indices 0-1331)
- "WinSCP" is at index **1279** (line 1280, 1-indexed)
- "6.5.6" is at index **1280**
- "$$" (INTERACTIVE_MSG_TAG) is at index **1277**

### MsgIDs.h Enums
- NB_* count: 743 enums (indices 0-742)
- MSG_* count: ~460 enums after NB_*
- MSG_WINSCPFAR_NAME at enum index **~1288** (line 1291 in file)

### The Mismatch
| String | Expected Enum Index | Actual Enum Index | Difference |
|--------|-----------------|---------------|---------------|
| "WinSCP" | 1279 | ~1288 | +9 |
| "6.5.6" | 1280 | ~1289 | +9 |
| "$$" | 1277 | ~1286 | +9 |
| "**" | 1276 | ~1285 | +9 |

The indices are offset by approximately **9** because:
1. Language file header (`.Language=English,English`) takes ~1 line before actual strings
2. Enum file may have extra entries not in language file

## Root Cause
The enum in MsgIDs.h:
```cpp
MSG_WINSCPFAR_NAME,  // Line 1291 in MsgIDs.h file
```

Should map to index **1279** (where "WinSCP" is in language file), but the computed enum value is ~1288.

## Investigation Required

### Step 1: Create Index Mapping Table
Generate a mapping between:
- MsgIDs.h enum name
- Expected language file index
- Actual string value

### Step 2: Fix MsgIDs.h
Options:
1. **Reorder** enum values to match language file indices
2. **Add offset** constants to align
3. **Use separate** string loading for WinSCP strings

### Step 3: Alternative - Keep WinSCP Strings Separate
Since WinSCP strings are loaded from different sources:
- Use `GetFileInfoString()` from VERSION_INFO
- Use `.rc` resources (TextsCore1.rc)
- Do NOT use the language file mapping

## Proposed Fix Approach

### Preferred: Hybrid Approach
Keep NB_* strings using current mapping (works for first ~1279 strings).
For WinSCP strings: use direct sources instead of LoadStr():
1. VERSION_INFO strings via `GetFileInfoString()`
2. WinSCP .rc resources via `LoadStrFrom()`

This avoids the systemic index mismatch issue.

## Files to Modify
- `src/base/MsgIDs.h` - if fixing enum indices
- `src/NetBox/WinSCPDialogs.cpp` - use alternative string sources
- `src/resource/TextsWin1.rc` - verify WinSCP string resources exist

## Note
This is a long-standing design issue - the NB_* section works because it starts at index 0 and matches the beginning of the language file. The WinSCP section was added later with a mismatch.