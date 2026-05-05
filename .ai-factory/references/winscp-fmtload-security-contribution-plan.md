# WinSCP FMTLOAD Security Hardening — Contribution Plan

> Scope: Add `EscapeFmtChars()` utility and secure vulnerable `FMTLOAD` call sites against CWE-134 format string attacks
> Target: WinSCP master (`D:\Projects\WinSCP-work\winscp-master\source`)
> Effort: Medium (new utility + 5 call sites + tests)
> Priority: Medium-High (security hardening)

---

## Problem Statement

WinSCP's `FMTLOAD()` macro loads localized format strings and substitutes arguments. Several call sites pass **untrusted server/user data** (FTP responses, redirect URIs, remote shell output) as format arguments. If these strings contain `%` characters, they are interpreted as format specifiers, causing:

- **Information disclosure** (`%p` leaks stack addresses)
- **Crashes** (`%n` writes to arbitrary memory)
- **Denial of service** (malformed format strings)

This is [CWE-134: Use of Externally-Controlled Format String](https://cwe.mitre.org/data/definitions/134.html).

---

## Vulnerable Call Sites (Confirmed in WinSCP 6.5.6)

| File | Line | Call | Untrusted Source | Risk |
|------|------|------|-----------------|------|
| `core/FtpFileSystem.cpp` | 1251 | `FMTLOAD(FTP_RESPONSE_ERROR, (CommandName, ResponseText))` | FTP server response | **HIGH** |
| `core/FtpFileSystem.cpp` | 2077 | `FMTLOAD(FTP_RESPONSE_ERROR, (Command, Response->Text))` | FTP server response | **HIGH** |
| `core/FtpFileSystem.cpp` | 3391 | `FMTLOAD(FTP_RESPONSE_ERROR, (FLastCommandSent, FLastResponse->Text))` | FTP server response | **HIGH** |
| `core/NeonIntf.cpp` | 191 | `FMTLOAD(REQUEST_REDIRECTED, (Uri))` | HTTP redirect URI | **MEDIUM** |
| `core/ScpFileSystem.cpp` | 674 | `FMTLOAD(INVALID_OUTPUT_ERROR, (Command, Output->Text))` | Remote shell output | **MEDIUM** |

---

## Proposed Solution

### 1. Add `EscapeFmtChars()` Utility

**Location:** `source/base/Strings.cpp` (or `source/base/Common.cpp` if `UnicodeString` utilities live there)

**Function signature:**

```cpp
UnicodeString __fastcall EscapeFmtChars(const UnicodeString & Str);
```

**Behavior:**
- Replace `%` with `%%` (format string escape)
- Preserve all other characters
- O(n) single pass

**Implementation pattern (from NetBox):**

```cpp
UnicodeString __fastcall EscapeFmtChars(const UnicodeString & Str)
{
  UnicodeString Result;
  Result.SetLength(Str.Length() * 2); // worst case: all %
  int DestIndex = 1;
  for (int SrcIndex = 1; SrcIndex <= Str.Length(); ++SrcIndex)
  {
    wchar_t Ch = Str[SrcIndex];
    if (Ch == L'%')
    {
      Result[DestIndex++] = L'%';
    }
    Result[DestIndex++] = Ch;
  }
  Result.SetLength(DestIndex - 1);
  return Result;
}
```

**Declaration:** Add to `source/base/Strings.h` (or appropriate header).

### 2. Secure Vulnerable Call Sites

Apply `EscapeFmtChars()` to untrusted arguments before passing to `FMTLOAD`:

```cpp
// Before (vulnerable):
throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, (CommandName, ResponseText)));

// After (hardened):
throw Exception(FMTLOAD(FTP_RESPONSE_ERROR, (CommandName, EscapeFmtChars(ResponseText))));
```

**Per-call-site changes:**

| File | Line | Change |
|------|------|--------|
| `FtpFileSystem.cpp:1251` | Wrap `ResponseText` with `EscapeFmtChars()` |
| `FtpFileSystem.cpp:2077` | Wrap `Response->Text` with `EscapeFmtChars()` |
| `FtpFileSystem.cpp:3391` | Wrap `FLastResponse->Text` with `EscapeFmtChars()` |
| `NeonIntf.cpp:191` | Wrap `Uri` with `EscapeFmtChars()` |
| `ScpFileSystem.cpp:674` | Wrap `Output->Text` with `EscapeFmtChars()` |

### 3. Verify No Regression

- All `FMTLOAD` call sites that pass **trusted** data (user-typed strings, constants, internal state) should NOT be changed — `EscapeFmtChars` is only for untrusted server/user input.
- Ensure `%%` renders as `%` in final output (standard format string behavior).
- Test with FTP server sending `500 %s%s%s%s%s` response — should display literally, not crash.

---

## WinSCP Adaptation Notes

| Aspect | NetBox | WinSCP |
|--------|--------|--------|
| String class | `UnicodeString` (same) | `UnicodeString` (same) |
| Function convention | `__fastcall` | `__fastcall` (same) |
| Header location | `src/base/nbstring.h` | `source/base/Strings.h` (verify) |
| Implementation file | `src/base/nbstring.cpp` | `source/base/Strings.cpp` (verify) |
| `FMTLOAD` macro | Same macro family | Same macro family |

**Verification needed:**
- Confirm `Strings.cpp` / `Strings.h` exist and are the right location for new utility functions
- Check if WinSCP already has a string utility file (e.g., `Sysutils.cpp`, `Common.cpp`)
- Verify `UnicodeString::SetLength()` and index operator `[]` behavior match NetBox's implementation

---

## Testing Strategy

### Unit Test (if WinSCP has test infrastructure)

```cpp
void TestEscapeFmtChars()
{
  assert(EscapeFmtChars(L"hello") == L"hello");
  assert(EscapeFmtChars(L"%s") == L"%%s");
  assert(EscapeFmtChars(L"100% complete") == L"100%% complete");
  assert(EscapeFmtChars(L"%%") == L"%%%%");
  assert(EscapeFmtChars(L"") == L"");
}
```

### Integration Test

1. Set up malicious FTP server that sends `500 %n%n%n%n%n` as error response
2. Trigger the error path in WinSCP
3. Verify: no crash, message displays literally

---

## Commit Message

```
fix(security): escape untrusted strings passed to FMTLOAD to prevent CWE-134

Add EscapeFmtChars() utility that doubles % characters to prevent
format string interpretation of untrusted server data.

Apply to vulnerable FMTLOAD call sites:
- FTP response errors (3 sites in FtpFileSystem.cpp)
- HTTP redirect URIs (NeonIntf.cpp)
- Remote shell output errors (ScpFileSystem.cpp)

Prevents crashes and information disclosure when servers send
malicious responses containing % format specifiers.
```

---

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| Over-escaping trusted strings | Only apply to untrusted server/user input; document which call sites are exempt |
| Performance impact on large responses | `EscapeFmtChars` is O(n); FTP responses are typically small (<1KB) |
| `%%` display issues in UI | Standard format string behavior — `%%` renders as `%`; verify in test |
| WinSCP maintainers prefer different approach | Offer as suggestion; they may prefer fixing `FMTLOAD` macro itself to auto-escape |

---

## Alternative Approaches (for discussion)

1. **Fix `FMTLOAD` macro** — Make the macro auto-escape all non-literal arguments. More invasive but covers all future call sites.
2. **Use `FORMAT` instead** — Replace `FMTLOAD(MSG, (arg1, arg2))` with `FORMAT(LoadStr(MSG), arg1, arg2)` where arguments are positionally substituted, not format-interpolated. Larger refactor.
3. **Type-safe wrapper** — Add a `TFMTLoadSafe` template that requires explicit `Trusted`/`Untrusted` annotation on arguments.

Recommendation: Start with `EscapeFmtChars()` — it's the minimally invasive fix that matches NetBox's proven approach.

---

## References

- NetBox fix: `.ai-factory/references/cwe134-fmtload-vulnerability-scan.md`
- NetBox implementation: `src/core/FtpFileSystem.cpp`, `src/core/NeonIntf.cpp`, `src/core/ScpFileSystem.cpp` (post-fix versions)
- CWE-134: https://cwe.mitre.org/data/definitions/134.html
