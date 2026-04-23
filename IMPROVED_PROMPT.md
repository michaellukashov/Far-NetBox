# Task: Implement FormatFloat, TFormatSettings, and TTimeSpan from Pascal to C++17

## Context
You are working on NetBox, a Far Manager plugin (SFTP/FTP/SCP/WebDAV/S3 client) written in C++17. The codebase is a port from Pascal (WinSCP/PuTTY/FileZilla). You need to implement three incomplete components by referencing the FreePascal RTL implementations.

## File Scope

**ALLOWED FILES (read/write):**
- `src/base/Sysutils.cpp`
- `src/base/Sysutils.hpp`
- `src/base/Classes.cpp`
- `src/base/Classes.hpp`

**READ-ONLY FILES (reference only):**
- `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\fmtflt.inc`
- `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\sysinth.inc`
- `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\dati.inc`
- `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\datih.inc`
- `AGENTS-Standards.md`

**FORBIDDEN:** All other files including `libs/` directory are off-limits. Any attempt to modify files outside ALLOWED FILES must trigger STOP_REQUIRED.

## Implementation Order

Execute in this sequence - each component must be complete and verified before proceeding to next:

### STEP 1: Read Pascal References (30 min)

Read these files to understand the Pascal implementations you will port:

1. **FormatFloat reference:**
   - File: `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\fmtflt.inc`
   - Lines: 1-409
   - Function: `IntFloatToTextFmt`
   - Focus: Format string parsing, section handling, digit placement, scientific notation

2. **TFormatSettings reference:**
   - File: `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\sysinth.inc`
   - Lines: 36-89
   - Record: `TFormatSettings`
   - Focus: Field definitions, default values

3. **TTimeSpan reference:**
   - File: `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\datih.inc`
   - Lines: 112-221
   - Focus: Method declarations, constants
   - File: `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\dati.inc`
   - Lines: 1-150
   - Focus: Date/time utilities, tick calculations

**CHECKPOINT:** After reading all references, output:
```
REFERENCES_LOADED: Confirmed understanding of:
- IntFloatToTextFmt format parsing logic
- TFormatSettings field structure
- TTimeSpan tick-based arithmetic
```

### STEP 2: Implement TFormatSettings Constructor

**File:** `src/base/Sysutils.cpp` (currently stub at line 1898)  
**Header:** `src/base/Sysutils.hpp` lines 175-202 (struct already defined)

**Current stub:**
```cpp
void GetLocaleFormatSettings(int32_t LCID, const TFormatSettings & FormatSettings)
{
  (void)LCID;
  (void)FormatSettings;
  ThrowNotImplemented(1204);
}
```

**Requirements:**

1. **Implement constructor:** `TFormatSettings::TFormatSettings(LCID id) noexcept`
2. **Populate all fields using Windows API:**
   - Use `GetLocaleInfoW(id, LOCALE_*, buffer, size)` for each field
   - Fields to populate (20+ total):
     - `CurrencyFormat`, `NegCurrFormat`, `CurrencyDecimals` (numeric)
     - `ThousandSeparator`, `DecimalSeparator`, `DateSeparator`, `TimeSeparator`, `ListSeparator` (wchar_t)
     - `CurrencyString`, `ShortDateFormat`, `LongDateFormat`, `TimeAMString`, `TimePMString`, `ShortTimeFormat`, `LongTimeFormat` (UnicodeString)
     - `ShortMonthNames[12]`, `LongMonthNames[12]`, `ShortDayNames[7]`, `LongDayNames[7]` (UnicodeString arrays)
     - `TwoDigitYearCenturyWindow` (uint16_t)

3. **Error handling policy:**
   - If `GetLocaleInfoW` fails for ANY field, initialize ALL fields to LOCALE_INVARIANT (LCID 0x007F) defaults:
     - `DecimalSeparator = L'.'`
     - `ThousandSeparator = L','`
     - `DateSeparator = L'\\'`
     - `TimeSeparator = L':'`
     - `CurrencyString = L"$"`
     - `ShortDateFormat = L"MM/dd/yyyy"`
     - `LongDateFormat = L"dddd, MMMM dd, yyyy"`
     - `TimeAMString = L"AM"`
     - `TimePMString = L"PM"`
     - `ShortTimeFormat = L"HH:mm"`
     - `LongTimeFormat = L"HH:mm:ss"`
     - Month/day names: use English defaults from header
     - `TwoDigitYearCenturyWindow = 50`
   - Constructor MUST NOT throw exceptions - always return valid object

4. **Thread safety:** TFormatSettings instances are immutable after construction (thread-safe by design)

**Verification test:**
```cpp
TFormatSettings fs = TFormatSettings::Create(LOCALE_USER_DEFAULT);
// Must not crash, all fields must be populated
```

### STEP 3: Implement FormatFloat Function

**File:** `src/base/Sysutils.cpp` line 483-489  
**Header:** `src/base/Sysutils.hpp` line 301 (signature already declared)

**Current stub:**
```cpp
UnicodeString FormatFloat(const UnicodeString & /*Format*/, double Value)
{
  UnicodeString Result(20, L'\0');
  swprintf_s(&Result[1], Result.Length(), L"%.2f", Value);
  PackStr(Result);
  return Result;
}
```

**Requirements:**

1. **Format string parsing - detect these elements:**
   - `#` = optional digit (omit leading/trailing zeros)
   - `0` = required digit (show zero if no digit available)
   - `.` = decimal separator position
   - `,` = thousand separator flag (if present before decimal point)
   - `E` or `e` = scientific notation (followed by `+` or `-` and exponent digits)
   - `;` = section separator (positive;negative;zero)

2. **Section logic:**
   - 1 section: use for all values (positive, negative, zero)
   - 2 sections: positive;negative (zero uses positive section)
   - 3 sections: positive;negative;zero

3. **TFormatSettings integration:**
   - Check existing FormatFloat call sites to determine if function should:
     - Use global `DefaultFormatSettings` instance, OR
     - Accept `TFormatSettings` as optional parameter
   - Use `TFormatSettings.DecimalSeparator` for decimal point
   - Use `TFormatSettings.ThousandSeparator` for thousand grouping

4. **Edge case handling:**
   - `NaN` → return `L"NaN"`
   - `+Infinity` → return `L"Infinity"`
   - `-Infinity` → return `L"-Infinity"`
   - Empty or invalid format string → return `Value` formatted as `%.15g`
   - Negative zero → treat as positive zero

5. **CRITICAL - Pascal to C++ porting rules:**
   - Pascal uses 1-indexed strings: `Section[1]` is first character
   - C++ uses 0-indexed strings: `Section[0]` is first character
   - Pascal uses manual pointer arithmetic: `Format[I]`, `Inc(I)`
   - C++ MUST use UnicodeString methods: `Format[i]`, `i++`, `Format.Length()`, `Format.SetLength()`
   - Do NOT port Pascal pointer arithmetic directly - causes buffer overflows
   - Use `UnicodeString::SetLength()`, `operator[]`, and bounds checking

6. **Thread safety:** Do NOT use static buffers - allocate on stack or return new UnicodeString

**Verification test:**
```cpp
UnicodeString result = FormatFloat(L"#,##0.00", 1234.5);
// Must return exactly: L"1,234.50"
```

### STEP 4: Implement TTimeSpan Methods

**File:** `src/base/Classes.cpp` line 1975+ (partial implementation exists)  
**Header:** `src/base/Classes.hpp` lines 637-720 (class already declared)

**Current partial implementation:**
```cpp
TTimeSpan::TTimeSpan(int64_t ATicks) : FTicks(ATicks)
{
}

TTimeSpan TTimeSpan::FromSeconds(double Value)
{
  TTimeSpan Result(nb::ToInt64(round(Value * TicksPerSecond)));
  return Result;
}
```

**Requirements - implement exactly these 15 method groups:**

**Group 1: Constructors (3 methods)**
1. `TTimeSpan(int32_t Hours, int32_t Minutes, int32_t Seconds)`
   - Calculate: `FTicks = (Hours * TicksPerHour) + (Minutes * TicksPerMinute) + (Seconds * TicksPerSecond)`
2. `TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds)`
   - Calculate: `FTicks = (Days * TicksPerDay) + (Hours * TicksPerHour) + (Minutes * TicksPerMinute) + (Seconds * TicksPerSecond)`
3. `TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds)`
   - Calculate: `FTicks = (Days * TicksPerDay) + (Hours * TicksPerHour) + (Minutes * TicksPerMinute) + (Seconds * TicksPerSecond) + (Milliseconds * TicksPerMillisecond)`

**Group 2: Static factory methods (5 methods)**
4. `FromDays(double Value)` - return `TTimeSpan(nb::ToInt64(round(Value * TicksPerDay)))`
5. `FromHours(double Value)` - return `TTimeSpan(nb::ToInt64(round(Value * TicksPerHour)))`
6. `FromMinutes(double Value)` - return `TTimeSpan(nb::ToInt64(round(Value * TicksPerMinute)))`
7. `FromMilliseconds(double Value)` - return `TTimeSpan(nb::ToInt64(round(Value * TicksPerMillisecond)))`
8. `FromTicks(int64_t Value)` - return `TTimeSpan(Value)`

**Group 3: Instance arithmetic methods (4 methods)**
9. `Add(const TTimeSpan & TS)` - return `TTimeSpan(FTicks + TS.FTicks)` with overflow clamping
10. `Duration()` - return `TTimeSpan(abs(FTicks))`
11. `Negate()` - return `TTimeSpan(-FTicks)` with overflow clamping
12. `Subtract(const TTimeSpan & TS)` - return `TTimeSpan(FTicks - TS.FTicks)` with overflow clamping

**Group 4: ToString method (1 method)**
13. `ToString()` - format as `"HH:MM:SS"` with zero-padding
    - Positive example: `FromSeconds(90).ToString()` → `"00:01:30"`
    - Negative example: `FromSeconds(-90).ToString()` → `"-00:01:30"`
    - Format: `sprintf` or `UnicodeString::Format` with `%02d:%02d:%02d`

**Group 5: Static arithmetic methods (already declared in header, implement in .cpp)**
14. Static overloads of `Add`:
    - `Add(const TTimeSpan & Left, const TTimeSpan & Right)` → `TTimeSpan`
    - `Add(const TTimeSpan & Left, const TDateTime & Right)` → `TDateTime`
    - `Add(const TDateTime & Left, const TTimeSpan & Right)` → `TDateTime`
15. Static overloads of `Subtract`:
    - `Subtract(const TTimeSpan & Left, const TTimeSpan & Right)` → `TTimeSpan`
    - `Subtract(const TDateTime & Left, const TTimeSpan & Right)` → `TDateTime`
    - `Subtract(const TDateTime & D1, const TDateTime & D2)` → `TTimeSpan`

**Group 6: Static helper methods (already declared in header, implement in .cpp)**
16. `Negative(const TTimeSpan & Value)` - return `Value.Negate()`
17. `Positive(const TTimeSpan & Value)` - return `Value`
18. `Implicit(const TTimeSpan & Value)` - return `Value.ToString()`
19. `Explicit(const TTimeSpan & Value)` - return `Value.ToString()`

**Group 7: Static getters (already declared in header, implement in .cpp)**
20. `GetZero()` - return `TTimeSpan(FZero)`
21. `GetMinValue()` - return `TTimeSpan(FMinValue)`
22. `GetMaxValue()` - return `TTimeSpan(FMaxValue)`

**Overflow handling policy:**
- All arithmetic methods (Add, Subtract, Negate) MUST clamp result to `[FMinValue, FMaxValue]`
- Never throw exceptions on overflow
- Clamping logic:
  ```cpp
  if (result > FMaxValue) result = FMaxValue;
  if (result < FMinValue) result = FMinValue;
  ```

**Verification test:**
```cpp
UnicodeString result = TTimeSpan::FromSeconds(90).ToString();
// Must return exactly: L"00:01:30"
```

### STEP 5: Runtime Verification

Before declaring the task complete, verify existing code behavior is preserved:

1. **Search for existing call sites:**
   ```bash
   grep -rn "FormatFloat" src/ --include="*.cpp" --include="*.hpp"
   grep -rn "TTimeSpan" src/ --include="*.cpp" --include="*.hpp"
   grep -rn "TFormatSettings" src/ --include="*.cpp" --include="*.hpp"
   ```

2. **Select 3 representative call sites** (one for each component if possible)

3. **Verify output is identical** before and after your changes:
   - Add temporary logging to capture output
   - Run the code path (or write minimal test harness)
   - Compare output

4. **If any verification fails:**
   - Output: `VERIFICATION_FAILED: [test name] | EXPECTED: [value] | ACTUAL: [value]`
   - Fix the implementation
   - Rebuild and re-verify
   - Do NOT proceed to Step 6 until all tests pass

### STEP 6: Final Build

1. **Build command:**
   ```cmd
   cmd /c build-x64.bat
   ```
   - This script loads `vcvarsall.bat` automatically
   - If build fails with `'cl.exe' is not recognized`, run `vcvarsall.bat x64` first

2. **Build must complete with ZERO warnings**
   - MSVC warning level: W4
   - Any warning is a failure

3. **If build fails:**
   - Read the error message carefully
   - Fix the issue
   - Rebuild
   - Do NOT proceed until build succeeds with zero warnings

## Coding Standards

Follow NetBox C++17 conventions (see `AGENTS-Standards.md` for full details):

- **Types:** PascalCase (`TTimeSpan`, `TFormatSettings`)
- **Methods:** PascalCase (`FromSeconds`, `ToString`)
- **Parameters:** camelCase (`hours`, `minutes`, `formatString`)
- **Local variables:** PascalCase (`Result`, `Value`)
- **Line endings:** CRLF (Windows)
- **Indentation:** 2 spaces (no tabs)
- **Encoding:** UTF-8 without BOM
- **No trailing whitespace**

**Type usage:**
- Use `int64_t`, `int32_t`, `uint8_t`, `uint16_t` (not `long`, `int`, `unsigned`)
- Use `UnicodeString` (not `std::wstring`, `wchar_t*`)
- Use `nb::ToInt64()`, `nb::ToInt32()`, `nb::ToDouble()` for conversions

**Function additions:**
- You MUST NOT add any new functions to header files
- If helper functions are required, implement them as `static` functions in the `.cpp` file only

## Stop Conditions

You MUST output this format and wait for human approval before:

```
STOP_REQUIRED: [reason]
PROPOSED_ACTION: [what you want to do]
AWAITING_APPROVAL
```

**Stop triggers:**
1. Adding method declarations to `Classes.hpp` or `Sysutils.hpp` UNLESS:
   - Method is already declared in the header but not implemented, OR
   - Method is called from existing code but declaration is missing
2. Modifying any file outside the ALLOWED FILES list
3. Changing any existing public API signatures
4. Any action that would break existing code compilation

## Done When

Task is complete when ALL of these conditions are met:

1. ✅ All Pascal reference files have been read (Step 1 checkpoint confirmed)
2. ✅ TFormatSettings constructor implemented and populates all 20+ fields
3. ✅ TFormatSettings verification test passes (no crash, all fields populated)
4. ✅ FormatFloat function implemented with full format string parsing
5. ✅ FormatFloat verification test passes: `FormatFloat(L"#,##0.00", 1234.5) == L"1,234.50"`
6. ✅ All 22 TTimeSpan methods implemented (6 groups listed in Step 4)
7. ✅ TTimeSpan verification test passes: `FromSeconds(90).ToString() == L"00:01:30"`
8. ✅ Runtime verification complete (Step 5) - at least 3 existing call sites verified
9. ✅ Build succeeds with zero warnings (Step 6)
10. ✅ No modifications outside the 4 ALLOWED FILES

**Output this checklist when done:**
```
IMPLEMENTATION_COMPLETE:
[✅/❌] Pascal references read
[✅/❌] TFormatSettings constructor implemented
[✅/❌] TFormatSettings verification passed
[✅/❌] FormatFloat implemented
[✅/❌] FormatFloat verification passed
[✅/❌] TTimeSpan 22 methods implemented
[✅/❌] TTimeSpan verification passed
[✅/❌] Runtime verification passed (3+ call sites)
[✅/❌] Build passed with zero warnings
[✅/❌] No files modified outside allowed scope
```
