# Message Loading System in NetBox

## Overview
Analysis of how NetBox loads and resolves strings from language files (.lng), and the ID mapping system that bridges code constants with string resources.

## Architecture

### String Resolution Flow
```
Code: GetMsg(NB_ABOUT_VERSION)
         ↓
FarPluginStrings.cpp maps {StringID → MsgID}
         ↓
GetGlobals()->GetMsg(MsgID) 
         ↓
Returns string from language file or RC resource
```

### Key Components

| Component | File | Purpose |
|-----------|------|---------|
| MsgIDs.h | src/base/ | Enum of string IDs (NB_*, MSG_*) |
| TextsCore.h | src/resource/ | RC resource IDs for WinSCP strings |
| FarPluginStrings.cpp | src/NetBox/ | Mapping table {StringID, MsgID} |
| FarPlugin.cpp | src/NetBox/ | GetMsg() implementation |

### ID Types

1. **NB_*** (NetBox internal) - Defined in MsgIDs.h starting at 0
2. **MSG_** (Message tag/RPC) - Defined in MsgIDs.h near end  
3. **WINSCPFAR_*** - WinSCP version strings with RC resource IDs

## Mapping System

### FarPluginStrings.cpp Structure
```cpp
{ WINSCPFAR_NAME, MSG_WINSCPFAR_NAME },  // Maps StringID → MsgID
{ WINSCP_VERSION, MSG_WINSCP_VERSION },
{ WINSCPFAR_VERSION, MSG_WINSCPFAR_VERSION },
// ...
```

### How Mapping Works
1. StringID (e.g., `WINSCPFAR_NAME = 4840`) defined in TextsCore.h
2. MsgID (e.g., `MSG_WINSCPFAR_NAME = 4840`) same value in MsgIDs.h  
3. Pair maps StringID → MsgID - but both must be correct!

### GetMsg Implementation (FarPlugin.cpp:2278)
```cpp
UnicodeString GetMsg(int32_t MsgId)
{
  return GetGlobals()->GetMsg(MsgId);
}
```

### FmtLoadStr (FormatUtils.cpp:42-50)
```cpp
UnicodeString FmtLoadStr(int32_t id, fmt::ArgList args)
{
  const UnicodeString Fmt = GetGlobals()->GetMsg(id);
  if (!Fmt.IsEmpty())
  {
    return Sprintf(Fmt, args);  // Substitute %s, %d
  }
  return UnicodeString();
}
```

## Bug: Missing Mapping

### Symptom
```cpp
GetMsg(NB_ABOUT_VERSION)  // Returns "$$" instead of "Version %s build %d"
```

### Root Cause
- `NB_ABOUT_VERSION` NOT in FarPluginStrings.cpp mapping table
- Without mapping, GetMsg uses numeric enum value as array index
- Returns wrong string from language file

### Affected Strings
- NB_ABOUT_VERSION
- NB_ABOUT_PRODUCT_VERSION  
- NB_ABOUT_HOMEPAGE
- NB_ABOUT_FORUM
- All NB_* constants before ~200

### Evidence
Line 1278 in NetBoxEng.lng: `"$$"` - this is what unmapped ID returns (array index 191 accesses line with $$)

## Fix Approaches

### 1. Hardcode Format String (Applied)
```cpp
// Before
FORMAT(GetMsg(NB_ABOUT_VERSION), VersionStr, NETBOX_VERSION_BUILD)

// After  
FORMAT(L"Version %s build %d", VersionStr, NETBOX_VERSION_BUILD)
```

### 2. Add Proper Mapping (Alternative)
Would require:
- Add NB_ABOUT_VERSION → MSG_* mapping in FarPluginStrings.cpp
- Ensure language file has correct string at proper index

## Format String Patterns

### Language File (.lng)
- Line 179: `"Version %s build %d"` - NetBox version format
- Line 180: `"Based on %s version %s"` - WinSCP "Based on" format
- Line 1278: `"$$"` - Message tag marker

### RC Resource (TextsCore1.rc)
```rc
MAIN_MSG_TAG, "**"
INTERACTIVE_MSG_TAG, "$$"  
WINSCPFAR_NAME, "WinSCP"
WINSCP_VERSION, "6.5.6"
```

## Parameter Passing Rules

Format strings need matching parameter count:

| Format String | Parameters Needed | Code Example |
|---------------|------------------|-------------|
| `"Version %s build %d"` | 2 (string, int) | `FORMAT(..., VersionStr, BuildNum)` |
| `"Based on %s version %s"` | 2 (string, string) | `FMTLOAD(..., NAME, VERSION)` |
| `"%s"` | 1 (string) | `FMTLOAD(..., SingleVal)` |

## Related Files

- NetBoxEng.lng - English strings (line 179+)
- TextsCore.h - WinSCP resource IDs 
- MsgIDs.h - String enum definitions
- FarPluginStrings.cpp - ID mapping table
- WinSCPDialogs.cpp - Usage (line ~1298, ~1307)

## Debugging Tips

1. Unmapped ID returns unexpected string → Check mapping table
2. Format shows `$$` or `**` → Message tag leaking, wrong ID
3. `%s` not substituted → Parameter count mismatch
4. Wrong version showing → Check which ID is passed to LoadStr()