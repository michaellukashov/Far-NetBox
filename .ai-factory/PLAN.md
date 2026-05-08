# Plan: Fix Borland-specific errors in TWinConfiguration (MSVC)

**Created:** 2026-05-08
**Branch:** lmv/dev (current)
**Description:** Fix 100+ MSVC compilation errors in `TWinConfiguration` methods (lines 549-1607) that use Borland-specific features. Phase 1-2 completed (implementations moved out of Borland guard, header compatibility verified). Phase 3 build failed with these errors.

## Settings

| Setting | Value |
|---------|-------|
| Testing | No |
| Logging | Verbose |
| Docs | No (warn-only) |
| Build type | RelWithDebugInfo |
| Platform | x64 |

## Roadmap Linkage

Milestone: "none"
Rationale: "Skipped by user"

## Context

### Current State

Phase 1-2 are complete:
- Shared implementations (TFileColorData, TEditorData, TEditorPreferences, TEditorList) moved out of `#if defined(__BORLANDC__)` guard
- MSVC compatibility stubs added to WinConfiguration.h (lines 14-49)
- RecryptPasswords guarded with `#if defined(__BORLANDC__)`

Build fails with 100+ errors in `TWinConfiguration` methods (lines 549-1607). The compiler stopped at 100 errors. These are all pre-existing Borland-specific features that were never guarded.

### Error Categories

| Category | Lines | Errors |
|----------|-------|--------|
| String concatenation (narrow + UnicodeString) | 549 | 1 |
| Undeclared identifiers (VCL constants) | 661-662, 1295, 1479 | ~10 |
| Function signature mismatches | 902, 1025, 1503, 1596 | 4 |
| Missing members (TStrings/TStringList/TCustomCommandType) | 1304, 1327, 1432, 1486-1505 | ~15 |
| Missing functions (ExtractFileName, StrToInt) | 1466, 1497, 1555, 1607 | 4 |
| Missing types (LastChar, ExeName) | 1563, 1485 | 2 |
| dynamic_cast conversion errors | 1538, 1573 | 2 |

### MSVC Compatibility Layer (Already Exists)

- `TColor` → `uint32_t` typedef
- `TFont` → empty struct
- `TList`, `TObject`, `TStringList` → defined in `src/base/Classes.hpp`
- `SaveDefaultPixelsPerInch()` → returns `"96"`
- `Application` / `Screen` → stub structs
- `TTerminalManager` → stub with `Instance()`
- `StrToIntDef`, `StrToInt64`, `TryStrToInt` → exist in `src/base/Sysutils.hpp`
- `ExtractFileName` → exists in `src/base/Common.cpp`

### Critical Gaps

- `StrToInt()` → missing (only `StrToIntDef` and `TryStrToInt` exist)
- `TStrings::Values[]`, `ValueFromIndex`, `Names[]` → missing
- `DefaultFixedWidthFontName`, `DefaultFixedWidthFontSize` → undeclared
- `CSIDL_APPDATA` → undeclared
- `TApplication::ExeName` → missing from struct
- `TEditorList::Modified`, `TCustomCommandList::Modified` → missing
- `TCustomCommandType::Id` → missing

## Tasks

### Phase 1: Add missing MSVC stubs to WinConfiguration.h

**Task 1.1:** Add missing VCL constants and struct members to WinConfiguration.h

- **Target:** `src/windows/WinConfiguration.h` (MSVC compatibility block, lines 14-49)
- **Changes:**
  - Add `DefaultFixedWidthFontName` (const UnicodeString, e.g. `L"Courier New"`)
  - Add `DefaultFixedWidthFontSize` (const int32_t, e.g. `10`)
  - Add `CSIDL_APPDATA` (constexpr, e.g. `0x001a`)
  - Add `ExeName` to `TApplication` struct (UnicodeString)
  - Add `FLocaleSafe`, `FLastMonitor`, `FHonorDrivePolicy`, `FTimeoutShellOperations`, `FFUpdates` as TWinConfiguration members or stubs
- **Logging:** Log at trace level when MSVC stubs are used
- **Verify:** Compile check — no more "undeclared identifier" errors for these symbols

### Phase 2: Add missing functions to Sysutils.hpp

**Task 2.1:** Add `StrToInt` function to Sysutils.hpp

- **Target:** `src/base/Sysutils.hpp` (around line 338)
- **Changes:**
  - Add `inline int32_t StrToInt(const UnicodeString & Value)` declaration
  - Add implementation in `src/base/Sysutils.cpp` (after TryStrToInt, ~line 187)
  - Implementation: call `TryStrToInt` and throw on failure
- **Logging:** Log on conversion failure
- **Verify:** Compile check — no more "identifier not found" for `StrToInt`

### Phase 3: Add missing TStrings members

**Task 3.1:** Add `Values[]`, `ValueFromIndex`, `Names[]` to TStrings in Classes.hpp

- **Target:** `src/base/Classes.hpp` (TStrings class)
- **Changes:**
  - Add `UnicodeString Values[const UnicodeString & Name]` property getter/setter
  - Add `UnicodeString ValueFromIndex[int32_t Index]` property getter
  - Add `UnicodeString Names[int32_t Index]` property getter
  - Implementation: delegate to existing `GetName`, `GetValue`, `GetValueFromIndex` methods
- **Logging:** Log at trace level for property access
- **Verify:** Compile check — no more "is not a member of TStrings" errors

### Phase 4: Add missing members to TCustomCommandList/TEditorList/TCustomCommandType

**Task 4.1:** Add `Modified` property to TCustomCommandList and TEditorList

- **Target:** `src/windows/WinConfiguration.h` (TCustomCommandList and TEditorList classes)
- **Changes:**
  - Add `bool Modified` property (or getter) to `TCustomCommandList`
  - Add `bool Modified` property (or getter) to `TEditorList`
  - Note: `TEditorList::Modified` already exists as a field, but `__property` might need adjustment
- **Logging:** Log at trace level
- **Verify:** Compile check — no more "is not a member" errors

**Task 4.2:** Add `Id` member to TCustomCommandType

- **Target:** `src/windows/WinConfiguration.h` (TCustomCommandType class)
- **Changes:**
  - Add `UnicodeString Id` member field
- **Logging:** Log at trace level
- **Verify:** Compile check — no more "is not a member" error

### Phase 5: Fix function signature mismatches

**Task 5.1:** Fix `IniFileStorageNameForReading` function call (line 902)

- **Target:** `src/windows/WinConfiguration.cpp` line 902
- **Changes:** Change `IniFileStorageNameForReading` to `IniFileStorageNameForReading()` (add `()` to call the function)
- **Verify:** Compile check

**Task 5.2:** Fix `GetUseMasterPassword` const mismatch (line 1025)

- **Target:** `src/windows/WinConfiguration.cpp` line 1025
- **Changes:** Add `const` to the definition: `bool TWinConfiguration::GetUseMasterPassword() const`
- **Verify:** Compile check

**Task 5.3:** Fix `IsPathToSameFile` function signature (line 1503)

- **Target:** `src/windows/WinConfiguration.cpp` line 1503
- **Changes:** Check declaration in `src/base/Common.h` and update call site to match
- **Verify:** Compile check

**Task 5.4:** Fix `DoLoadExtensionList` function signature (line 1596)

- **Target:** `src/windows/WinConfiguration.cpp` line 1596
- **Changes:** Check declaration and update call site to match (currently takes 2 args, declaration takes 1)
- **Verify:** Compile check

### Phase 6: Fix missing functions and types

**Task 6.1:** Fix `ExtractFileName` not found (lines 1466, 1497, 1555)

- **Target:** `src/windows/WinConfiguration.cpp`
- **Changes:** Check if `ExtractFileName` is declared in `Common.h`. If not, add declaration. If it's `ExtractFileBaseName` that's available, update calls.
- **Verify:** Compile check

**Task 6.2:** Fix `LastChar` not a member of UnicodeString (line 1563)

- **Target:** `src/windows/WinConfiguration.cpp` line 1563
- **Changes:** Replace `FName.LastChar()` with `FName[FName.Length()]` or similar MSVC-compatible alternative
- **Verify:** Compile check

**Task 6.3:** Add `ExeName` to `TApplication` struct (line 1485)

- **Target:** `src/windows/WinConfiguration.h` (TApplication struct in MSVC block)
- **Changes:** Add `UnicodeString ExeName;` to `TApplication` struct
- **Verify:** Compile check

### Phase 7: Fix string concatenation and dynamic_cast

**Task 7.1:** Fix string concatenation at line 549

- **Target:** `src/windows/WinConfiguration.cpp` line 549
- **Changes:** Change `"PixelsPerInch=" + SaveDefaultPixelsPerInch()` to `L"PixelsPerInch=" + SaveDefaultPixelsPerInch()`
- **Verify:** Compile check

**Task 7.2:** Fix dynamic_cast conversion errors (lines 1538, 1573)

- **Target:** `src/windows/WinConfiguration.cpp`
- **Changes:** Replace `dynamic_cast` with `reinterpret_cast` or `static_cast` for MSVC compatibility
- **Verify:** Compile check

### Phase 8: Build and verify

**Task 8.1:** Build with MSVC x64 RelWithDebugInfo

- **Target:** Run `cmd /c build-x64.bat`
- **Changes:** None — verification only
- **Verify:** Zero warnings, zero errors

**Task 8.2:** Verify plugin DLL location

- **Target:** Check `Far3_x64/Plugins/NetBox/NetBox.dll` exists
- **Verify:** DLL is in correct location

## Commit Plan

8 tasks → checkpoints every 3-5:

| Checkpoint | Tasks | Message |
|------------|-------|---------|
| 1 | Task 1.1 | `refactor(win): add VCL constants to MSVC compatibility layer` |
| 2 | Task 2.1, 3.1, 4.1, 4.2 | `refactor(win): add StrToInt, TStrings members, TCustomCommandList/TEditorList/TCustomCommandType stubs` |
| 3 | Task 5.1-5.4, 6.1-6.3 | `refactor(win): fix function signatures and missing types` |
| 4 | Task 7.1-7.2, 8.1-8.2 | `refactor(win): fix string concat, dynamic_cast, build verification` |

## Anti-Patterns

- **DO NOT** modify files in `libs/` (use patches)
- **DO NOT** combine shell commands with `&&` or `||`
- **DO NOT** use Unix-style redirections (`>/dev/null`, `2>/dev/null`)
- **DO NOT** rewrite entire files — make surgical edits
- **DO** verify CRLF line endings on all modified files
- **DO** verify UTF-8 without BOM
- **DO** verify zero trailing whitespace

## Next Steps

```
/aif-implement

CONTEXT FROM /aif-plan:
- Plan file: .ai-factory/PLAN.md
- Testing: no
- Logging: verbose
- Docs: no (warn-only)
```
