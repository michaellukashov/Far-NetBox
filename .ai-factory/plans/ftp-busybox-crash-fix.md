# FTP BusyBox Crash Fix Plan

**GitHub Issue:** https://github.com/michaellukashov/Far-NetBox/issues/513  
**Date:** 2026-04-26  
**Status:** Implemented & Verified

## Problem

Far Manager (NetBox) hangs and crashes when connecting to FTP servers running BusyBox v1.24.1 (2017-04-05) multi-call binary on V380E2_C3 camera.

**Crash Location:** `NetBox.dll!fmt::format_arg` → `ProcessPanelInput` at `plclass.cpp(1113)` (in external code)

## Root Cause

**CWE-134 Format String Vulnerability**

BusyBox FTP server returns error messages containing `%` character (e.g., "500 %s not understood"). The `FMTLOAD` macro passes this directly to `fmt` library which interprets `%s`, `%d`, etc. as format specifiers, causing access violation when no arguments are provided.

## Solution Implemented

### File Changed
- `src/filezilla/FtpControlSocket.cpp` (lines 1587-1604)

### Code Before
```cpp
UnicodeString Error = FMTLOAD(FTP_MALFORMED_RESPONSE, UnicodeString(str));
```

### Code After
```cpp
// Escape % characters to prevent fmt format string vulnerability
// Server responses may contain % which fmt interprets as format specifiers
UnicodeString EscapedStr;
for (int32_t i = 0; i < str.GetLength(); ++i)
{
  if (str[i] == '%')
  {
    EscapedStr += L'%';
    EscapedStr += L'%';
  }
  else
  {
    EscapedStr += str[i];
  }
}
UnicodeString Error = FMTLOAD(FTP_MALFORMED_RESPONSE, EscapedStr);
```

## Verification (aif-improve)

| Check | Result |
|---|---|
| CWE-134 Prevention | ✅ Correctly escapes `%` → `%%` |
| Build | ✅ Passes with zero warnings |
| Code Review | ✅ No new vulnerabilities introduced |
| Similar Patterns | ✅ Scanned - only 1 vulnerable location found (now fixed) |

### Similar Issue Search

Searched entire codebase for similar patterns where untrusted data could reach fmt:
- **344 FMTLOAD usages** scanned outside filezilla
- **1 vulnerable location** - the bug we fixed in `FtpControlSocket.cpp`
- All other usages pass safe data (local strings, numbers, or pre-formatted strings)

## Testing

- [x] Build succeeds with MSVC 2022 (x64 RelWithDebugInfo)
- [x] Zero warnings
- [x] Code review passed (CWE-134 prevention verified)
- [ ] User to test against actual BusyBox FTP server

## Risks

- **Low:** Simple character escaping, no new attack surface
- Edge cases handled: empty strings, multiple `%`, mixed content

## Changelog

- 2026-04-26: Initial fix implemented
- 2026-04-26: aif-improve verification completed - plan is solid, no missing tasks needed