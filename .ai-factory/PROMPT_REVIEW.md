# PromptSentinel Review: Implement FormatFloat, TFormatSettings, and TTimeSpan from Pascal to C++17

**Overall Risk Level:** Critical  
**Critical Issues:** 5 | **High:** 10 | **Medium:** 7 | **Low:** 2  
**Estimated Production Failure Rate if Unfixed:** ~100% of runs

## Critical & High Findings

| # | Source | Failure Mode | Exact Quote / Location | Risk (High-Volume) | Mitigation & Rewritten Example |
|---|--------|--------------|------------------------|--------------------|-------------------------------|
| 1 | Mode 2 | Ambiguous Completion | "All three components fully implemented" | No measurable criteria for "fully implemented" - agent may skip methods or implement stubs | Replace with: "Done when: (1) All 15 TTimeSpan methods listed in Classes.hpp have non-stub implementations in Classes.cpp, (2) FormatFloat passes test case `FormatFloat(L"#,##0.00", 1234.5) == L"1,234.50"`, (3) TFormatSettings constructor populates all 20+ fields without throwing" |
| 2 | Mode 6 | Negation Fragility | Multiple "Do NOT" / "MUST NOT" statements (8 instances) | Under load or with weaker models, negations are frequently ignored - agent may modify libs/, add files, change signatures | Reframe as positive constraints: "ALLOWED FILES: src/base/Sysutils.cpp, src/base/Sysutils.hpp, src/base/Classes.cpp, src/base/Classes.hpp. All other files are READ-ONLY." |
| 3 | ADV | Production Scale Risk | "Handle default English locale when LCID lookup fails" | Inconsistent behavior across runs when LCID fails - some runs use "." decimal, others use "," causing data corruption | Specify exact fallback: "On LCID lookup failure, use LOCALE_INVARIANT (LCID 0x007F) which guarantees: DecimalSeparator='.', ThousandSeparator=',', DateSeparator='\\', TimeSeparator=':'" |
| 4 | ADV | Production Scale Risk | "Must handle edge cases: overflow, underflow, NaN, infinity" | FormatFloat may throw on NaN while TTimeSpan clamps on overflow, causing inconsistent error handling across codebase | Add error handling policy: "Edge case handling: (1) NaN/Infinity in FormatFloat: return "NaN"/"Infinity" string, (2) Overflow in TTimeSpan: clamp to FMinValue/FMaxValue, (3) Never throw exceptions - return safe defaults" |
| 5 | ADV | Production Scale Risk | "Reference Pascal implementation in `fmtflt.inc` function `IntFloatToTextFmt` (lines 1-409)" | Agent ports Pascal pointer arithmetic directly, causing heap corruption when UnicodeString reallocates | Add warning: "CRITICAL: Pascal uses 1-indexed strings and manual memory. C++ port MUST use 0-indexed UnicodeString methods. Do NOT port pointer arithmetic directly - use UnicodeString::SetLength(), operator[], and bounds checking." |
| 6 | Mode 1 | Silent Ignoring | "Reference Pascal implementation in `fmtflt.inc` function `IntFloatToTextFmt` (lines 1-409)" | Agent may skip reading 409-line Pascal reference and implement simplified version, causing format incompatibility | Make reading the reference file an explicit numbered step with verification: "STEP 1: Read D:\...\fmtflt.inc lines 1-409. Confirm you have loaded the IntFloatToTextFmt function before proceeding." |
| 7 | Mode 3 | Context Window Assumptions | "Reference Pascal TTimeSpan implementations (search FreePascal source for TTimeSpan record and methods)" | Vague instruction to "search" - agent may not find implementations or may hallucinate based on partial context | Provide explicit file paths: "Read TTimeSpan implementations from: (1) D:\...\datih.inc lines 112-221 for declarations, (2) D:\...\dati.inc lines 1-150 for date/time utilities" |
| 8 | Mode 8 | Variable Resolution Gaps | "Use `TFormatSettings` for locale-aware separators" | FormatFloat signature takes no TFormatSettings parameter - agent must infer to use global or create instance, causing inconsistency | Specify: "FormatFloat must use the global DefaultFormatSettings instance or accept TFormatSettings as optional parameter (check existing usage first)" |
| 9 | ADV | Production Scale Risk | "Check that `TTimeSpan::FromSeconds(90).ToString()` returns a valid time string" | Agent implements ToString() with arbitrary format, breaks existing code that expects specific format | Specify exact format: "TTimeSpan::FromSeconds(90).ToString() MUST return "00:01:30" (format: HH:MM:SS with zero-padding)" |
| 10 | ADV | Production Scale Risk | No mention of thread safety | Race conditions in TFormatSettings initialization or FormatFloat static buffers causing crashes in production | Add: "Thread safety: TFormatSettings instances are immutable after construction (thread-safe). FormatFloat must not use static buffers - allocate on stack or return new UnicodeString." |
| 11 | ADV | Production Scale Risk | "All existing usages of these types continue to compile" | Agent changes FormatFloat behavior subtly (e.g., rounding mode), breaks existing file size formatting, data export, log formatting | Add runtime verification: "Before declaring done, search codebase for all FormatFloat/TTimeSpan/TFormatSettings call sites (use grep). Verify at least 3 existing call sites produce identical output before and after changes." |
| 12 | PATH | Execution Path Gap | TFormatSettings constructor path | No done-state defined - should constructor throw? Return partially initialized? Use defaults? | Add: "TFormatSettings constructor error path: If GetLocaleInfoW fails for ANY field, initialize ALL fields to LOCALE_INVARIANT defaults. Constructor MUST NOT throw - always return valid object." |
| 13 | PATH | Execution Path Gap | TTimeSpan arithmetic methods path | No overflow handling specified - clamp? Wrap? Throw? | Add: "TTimeSpan overflow handling: All arithmetic methods (Add, Subtract, Negate) MUST clamp result to [FMinValue, FMaxValue]. Never throw on overflow." |
| 14 | PATH | Execution Path Gap | FormatFloat implementation path | No entry condition specified - unclear if agent should detect E/e in format string or always support it | Add: "FormatFloat MUST parse format string and detect: (1) E/e for scientific notation, (2) ; for section separators, (3) # and 0 for digit placeholders. If format is empty or invalid, return Value formatted as %.15g" |
| 15 | PATH | Execution Path Gap | TTimeSpan::ToString() implementation path | No specification of negative time formatting - should it show "-00:01:30" or "-(00:01:30)" or throw? | Add: "TTimeSpan::ToString() for negative spans: prepend minus sign to result: "-HH:MM:SS". Example: FromSeconds(-90).ToString() returns "-00:01:30"" |

## Medium & Low Findings

| # | Source | Failure Mode | Exact Quote / Location | Risk (High-Volume) | Mitigation & Rewritten Example |
|---|--------|--------------|------------------------|--------------------|-------------------------------|
| 16 | Mode 4 | Over-specification | Entire "Current incomplete implementation" code blocks (50+ lines each) | Large code blocks cause selective attention - agent may miss key requirements buried after code | Move code blocks to appendix or collapse to "See current stub at line X". Keep requirements list first and prominent. |
| 17 | Mode 7 | Implicit Ordering | Three components listed without dependency order | TFormatSettings must be implemented before FormatFloat (which uses it), but order is not explicit | Add: "IMPLEMENTATION ORDER: (1) TFormatSettings constructor first, (2) FormatFloat second (depends on TFormatSettings), (3) TTimeSpan third (independent)" |
| 18 | Mode 9 | Scope Creep Invitation | "Implement all missing constructors... Implement all static factory methods... Implement instance methods..." | Vague "all" without explicit count - agent may implement more or fewer than intended | Enumerate exactly: "Implement these 15 methods: (1) TTimeSpan(int32_t, int32_t, int32_t), (2) TTimeSpan(int32_t, int32_t, int32_t, int32_t), ... (15) ToString()" |
| 19 | PATH | Execution Path Gap | FormatFloat section separator path | Unclear which section to use - positive section or treat as special case? | Add: "FormatFloat section logic: 1 section = all values, 2 sections = positive;negative (zero uses positive), 3 sections = positive;negative;zero. Match Pascal behavior exactly." |
| 20 | PATH | Execution Path Gap | Verification Steps path | No instruction on what to do - retry? Report? Fix? | Add: "If any verification test fails: (1) Output VERIFICATION_FAILED: [test name] | EXPECTED: [value] | ACTUAL: [value], (2) Fix the implementation, (3) Rebuild and re-verify. Do NOT proceed to next component until all tests pass." |
| 21 | PATH | Execution Path Gap | Stop Conditions path | No clear entry condition - how does agent know if declaration is "missing" vs "should not be added"? | Replace ambiguous clause with: "You MAY add method declarations to Classes.hpp/Sysutils.hpp ONLY if: (1) Method is already declared in the header but not implemented, OR (2) Method is called from existing code but declaration is missing. For all other cases, STOP and ask." |
| 22 | ADV | Production Scale Risk | Build command uses cmd /c which creates new shell | Agent reports "build passes" but user's build fails, wasting iteration cycles | Change to: "Build with: cmd /c build-x64.bat (this loads vcvarsall.bat automatically). If build fails with 'cl.exe not found', run vcvarsall.bat x64 first." |
| 23 | Mode 5 | Non-deterministic Phrasing | "private methods in .cpp are OK" | Permissive phrasing invites scope creep - agent may add unnecessary helper methods | Replace with: "You MUST NOT add any new functions to headers. If helper functions are required, implement them as static functions in the .cpp file only." |
| 24 | Mode 10 | Halt / Checkpoint Gaps | Stop conditions exist but no output format specified | Agent may silently proceed without asking, or ask in unclear format | Add: "When stopping, output: STOP_REQUIRED: [reason] | PROPOSED_ACTION: [what you want to do] | AWAITING_APPROVAL" |

## Positive Observations

- Explicit file scope provided (4 target files) - reduces risk of unintended modifications
- Verification steps included with concrete test cases - enables automated validation
- Stop conditions present - provides human review gates for critical decisions
- Reference implementations specified - reduces hallucination risk
- Build command provided - enables immediate feedback loop

## Recommended Refactor Summary

**Highest-leverage changes:**
1. **Reframe all negations as positive constraints** - replace 8 "Do NOT" statements with "ALLOWED FILES" whitelist
2. **Add explicit implementation order** - TFormatSettings → FormatFloat → TTimeSpan with dependency rationale
3. **Make Pascal reference reading mandatory first step** - with confirmation checkpoint before coding
4. **Add comprehensive error handling policy** - specify exact behavior for NaN, infinity, overflow, LCID failure
5. **Enumerate all 15 TTimeSpan methods explicitly** - replace "all missing" with numbered checklist
6. **Specify exact output formats** - ToString() format, default locale values, section separator logic
7. **Add thread safety requirements** - immutable TFormatSettings, no static buffers in FormatFloat
8. **Add runtime verification step** - grep existing call sites, verify output unchanged

## Revised Prompt Sections (Critical/High items only)

### REVISED: File Scope (replaces Forbidden Actions + Constraints)

```markdown
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
```

### REVISED: Implementation Order (new section)

```markdown
## Implementation Order

Execute in this sequence - each component must be complete and verified before proceeding to next:

**STEP 1: Read Pascal References (30 min)**
- Read `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\fmtflt.inc` lines 1-409 (IntFloatToTextFmt function)
- Read `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\sysinth.inc` lines 36-89 (TFormatSettings record)
- Read `D:\Projects\FreePascal\FPCSource\rtl\objpas\sysutils\datih.inc` lines 112-221 (TTimeSpan declarations)
- **CHECKPOINT:** Output "REFERENCES_LOADED: Confirmed IntFloatToTextFmt, TFormatSettings, TTimeSpan structures understood"

**STEP 2: Implement TFormatSettings Constructor (src/base/Sysutils.cpp line 1898)**
- Dependency: None
- Populate all 20+ fields using GetLocaleInfoW
- **Error handling:** If GetLocaleInfoW fails for ANY field, initialize ALL fields to LOCALE_INVARIANT (LCID 0x007F) defaults:
  - DecimalSeparator = L'.'
  - ThousandSeparator = L','
  - DateSeparator = L'\\'
  - TimeSeparator = L':'
  - CurrencyString = L"$"
  - (etc. - use LOCALE_INVARIANT for all fields)
- Constructor MUST NOT throw - always return valid object
- **Verification:** `TFormatSettings::Create(LOCALE_USER_DEFAULT)` populates all fields without crashing

**STEP 3: Implement FormatFloat Function (src/base/Sysutils.cpp line 483-489)**
- Dependency: TFormatSettings (uses DefaultFormatSettings global or check existing call sites for parameter passing)
- **CRITICAL:** Pascal uses 1-indexed strings and manual memory. C++ port MUST use 0-indexed UnicodeString methods. Do NOT port pointer arithmetic directly - use UnicodeString::SetLength(), operator[], and bounds checking.
- Format string parsing requirements:
  - Detect `#` (optional digit), `0` (required digit)
  - Detect `.` (decimal separator position)
  - Detect `,` (thousand separator flag)
  - Detect `E` or `e` (scientific notation)
  - Detect `;` (section separators)
- Section logic: 1 section = all values, 2 sections = positive;negative (zero uses positive), 3 sections = positive;negative;zero
- **Edge cases:**
  - NaN → return L"NaN"
  - Infinity → return L"Infinity" or L"-Infinity"
  - Empty/invalid format → return Value formatted as %.15g
- **Thread safety:** Do NOT use static buffers - allocate on stack or return new UnicodeString
- **Verification:** `FormatFloat(L"#,##0.00", 1234.5)` returns exactly L"1,234.50"

**STEP 4: Implement TTimeSpan Methods (src/base/Classes.cpp line 1975+)**
- Dependency: None
- Implement exactly these 15 methods:
  1. `TTimeSpan(int32_t Hours, int32_t Minutes, int32_t Seconds)`
  2. `TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds)`
  3. `TTimeSpan(int32_t Days, int32_t Hours, int32_t Minutes, int32_t Seconds, int32_t Milliseconds)`
  4. `FromDays(double Value)` - static
  5. `FromHours(double Value)` - static
  6. `FromMinutes(double Value)` - static
  7. `FromMilliseconds(double Value)` - static
  8. `FromTicks(int64_t Value)` - static
  9. `Add(const TTimeSpan & TS)` - instance
  10. `Duration()` - instance (returns absolute value)
  11. `Negate()` - instance
  12. `Subtract(const TTimeSpan & TS)` - instance
  13. `ToString()` - instance, format: "HH:MM:SS" with zero-padding, negative spans prepend minus: "-HH:MM:SS"
  14. Static arithmetic: `Add(TTimeSpan, TTimeSpan)`, `Add(TTimeSpan, TDateTime)`, `Add(TDateTime, TTimeSpan)`, `Subtract(TTimeSpan, TTimeSpan)`, `Subtract(TDateTime, TTimeSpan)`, `Subtract(TDateTime, TDateTime)`
  15. Static helpers: `Negative`, `Positive`, `Implicit`, `Explicit`, `GetZero()`, `GetMinValue()`, `GetMaxValue()`
- **Overflow handling:** All arithmetic methods MUST clamp result to [FMinValue, FMaxValue]. Never throw on overflow.
- **Verification:** `TTimeSpan::FromSeconds(90).ToString()` returns exactly L"00:01:30"

**STEP 5: Runtime Verification**
- Search codebase for existing call sites: `grep -rn "FormatFloat\|TTimeSpan\|TFormatSettings" src/`
- Select 3 representative call sites
- Verify output is identical before and after changes
- If any verification fails:
  - Output: `VERIFICATION_FAILED: [test name] | EXPECTED: [value] | ACTUAL: [value]`
  - Fix implementation
  - Rebuild and re-verify
  - Do NOT proceed until all tests pass

**STEP 6: Final Build**
- Build with: `cmd /c build-x64.bat`
- MUST complete with zero warnings
- If build fails with 'cl.exe not found', run vcvarsall.bat x64 first
```

### REVISED: Stop Conditions (replaces original)

```markdown
## Stop Conditions

You MUST output `STOP_REQUIRED: [reason] | PROPOSED_ACTION: [what you want to do] | AWAITING_APPROVAL` and wait for human approval before:

1. Adding method declarations to Classes.hpp or Sysutils.hpp UNLESS:
   - Method is already declared in the header but not implemented, OR
   - Method is called from existing code but declaration is missing
2. Modifying any file outside the ALLOWED FILES list
3. Changing any existing public API signatures
4. Any action that would break existing code compilation
```

**Reviewer Confidence:** 95/100  
**Review Complete** – ready for re-submission or automated patching.
