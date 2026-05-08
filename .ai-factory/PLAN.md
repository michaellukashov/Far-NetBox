# Plan: Build TWinConfiguration under MSVC with Borland Guards

**Created:** 2026-05-08  
**Branch:** lmv/dev (current)  
**Description:** Make `TWinConfiguration` compile under MSVC by restructuring Borland-specific code with `#if defined(__BORLANDC__)` / `#endif // defined(__BORLANDC__)` guards and providing MSVC alternatives.

## Settings

- **Testing:** No (skip tests)
- **Logging:** Verbose (default)
- **Docs:** No (warn-only)
- **Build type:** RelWithDebugInfo
- **Platform:** x64

## Context

### Current Problem

`WinConfiguration.cpp` has a large `#if defined(__BORLANDC__)` block (lines 22-477) containing implementations for:
- `TFileColorData` (constructor, Load, Save, LoadList, SaveList)
- `TEditorData` (constructors, operator==, ExternalEditorOptionsAutodetect)
- `TEditorPreferences` (constructors, operator==, Matches, GetDefaultExternalEditor, etc.)
- `TEditorList` (constructor, destructor, Init, Modify, Saved, operator=, operator==, Clear, Add, Insert, Change, Move, Delete, Find, Load, Save, GetCount, GetEditor, IsDefaultList)
- Static variables: `NotepadName`, `ToolbarsLayoutKey`, `ToolbarsLayoutOldKey`, `DefaultUpdatesPeriod`, `ScpExplorerDirViewParamsDefault`, etc.

Under MSVC, these implementations are excluded, but `TWinConfiguration` methods (lines 479-4099) depend on them.

### Compatibility Layer (Already Exists)

The codebase already provides MSVC compatibility:
- `__property` → no-op comment macro (`src/include/nbglobals.h` line 219)
- `__finally` → lambda syntax macro (`src/base/Sysutils.hpp` line 562)
- `TList`, `TObject`, `TStringList` → defined in `src/base/Classes.hpp`
- `TColor` → `uint32_t` typedef (`WinConfiguration.h` line 20)
- `TNortonLikeMode`, `TCompareCriterias`, `TFormatBytesStyle`, `TIncrementalSearch`, `TAssemblyLanguage` → enums/typedefs in `WinConfiguration.h` lines 16-24

### Borland-Specific Features in the Guard

| Feature | Location | MSVC Alternative |
|---------|----------|------------------|
| `SaveDefaultPixelsPerInch()` | Lines 30, 33 | Return `"96"` (or hardcode default) |
| `RestoreColor()` / `StoreColor()` | Lines 47, 53 | Provide MSVC stubs (parse/return color values) |
| `CutToChar()` | Line 47 | Available in Common.h |
| `CommaTextToStringList()` | Line 59 | Available in Common.h |
| `StripHotkey()` / `LoadStr()` | Lines 217, 221 | Available in WinInterface/Common |
| `mbSingleByte` | Line 236 | Define as enum constant |
| `ByteType()`, `SubString()` | Lines 236, 245-246 | UnicodeString methods (available) |
| `Application->Name` / `Application->ExeName` | Lines 499, 1469 | `ModuleFileName()` alternative |

## Tasks

### Phase 1: Restructure WinConfiguration.cpp

**Goal:** Move shared implementations out of the Borland guard; keep only Borland-specific code inside `#if defined(__BORLANDC__)` / `#endif // defined(__BORLANDC__)`.

**Task 1.1:** Move static variables out of the Borland guard
- Move `NotepadName`, `ToolbarsLayoutKey`, `ToolbarsLayoutOldKey`, `DefaultUpdatesPeriod` outside the guard
- Move `QueueViewLayoutDefault`, `ScpCommanderWindowParamsDefault`, `ScpExplorerWindowParamsDefault` outside the guard
- **Verify:** These variables are needed by both compilers

**Task 1.2:** Move `ScpExplorerDirViewParamsDefault` and `ScpCommanderLocalPanelDirViewParamsDefault` to `#if !defined(__BORLANDC__)` with hardcoded defaults
- Under Borland: use `SaveDefaultPixelsPerInch()` (VCL function)
- Under MSVC: use hardcoded `"96"` (standard DPI)
- **Verify:** Default values are correct for MSVC

**Task 1.3:** Move `TFileColorData` implementation out of the Borland guard
- The implementation uses `TColor()` (already `uint32_t` in MSVC), `RestoreColor()`, `StoreColor()`, `TList`, `TStringList`
- `TList` and `TStringList` are available in both compilers
- `RestoreColor()` and `StoreColor()` need MSVC stubs if they're VCL-specific
- **Verify:** Check if `RestoreColor()` and `StoreColor()` are available in MSVC

**Task 1.4:** Move `TEditorData` implementation out of the Borland guard
- Uses `NotepadName` (static), `ReformatFileNameCommand()`, `ExtractProgramName()`, `SameText()`, `IsWin10Build()`
- All should be available in MSVC
- **Verify:** All functions are available

**Task 1.5:** Move `TEditorPreferences` implementation out of the Borland guard
- Uses `StripHotkey()`, `LoadStr()`, `mbSingleByte`, `ByteType()`, `SubString()`, `UpperCase()`, `LowerCase()`
- `mbSingleByte` needs definition for MSVC
- `ByteType()`, `SubString()`, `UpperCase()`, `LowerCase()` are UnicodeString methods (available)
- **Verify:** Check `mbSingleByte` and `StripHotkey()` availability

**Task 1.6:** Move `TEditorList` implementation out of the Borland guard
- Uses `TList`, `TObject`, `reinterpret_cast<TObject *>`, `__finally`
- `TList` and `TObject` are available in both compilers
- `__finally` is a macro that converts to lambda syntax
- `reinterpret_cast<TObject *>` may need adjustment
- **Verify:** Check `reinterpret_cast` usage and `__finally` macro

**Task 1.7:** Handle `RecryptPasswords` MSVC implementation
- Currently at line 4100 in `#else` block
- Need to ensure it's properly guarded
- **Verify:** MSVC implementation is correct

### Phase 2: Update WinConfiguration.h

**Goal:** Ensure the header compiles under MSVC.

**Task 2.1:** Verify `__property` declarations are handled
- The `__property` keyword is defined as a no-op comment in `nbglobals.h`
- All `__property` declarations should compile under MSVC
- **Verify:** No compilation errors from `__property`

**Task 2.2:** Verify `__closure` vs function pointer handling
- Already handled at lines 387-391 of WinConfiguration.h
- **Verify:** No compilation errors

### Phase 3: Build Verification

**Goal:** Ensure the code compiles with zero warnings.

**Task 3.1:** Build with MSVC x64 RelWithDebugInfo
- Use `cmd /c build-x64.bat`
- Verify zero warnings
- **Verify:** `build-x64.bat` completes with zero warnings

**Task 3.2:** Verify plugin DLL location
- Check `Far3_x64/Plugins/NetBox/NetBox.dll` exists
- **Verify:** DLL is in correct location

## Commit Plan

Single commit with conventional format:

```
Build TWinConfiguration under MSVC with Borland guards

- Move shared implementations (TFileColorData, TEditorData, TEditorPreferences, TEditorList) out of #if defined(__BORLANDC__) guard
- Add #if !defined(__BORLANDC__) sections for MSVC-specific defaults
- Keep Borland-specific code (SaveDefaultPixelsPerInch, etc.) in #if defined(__BORLANDC__) guard
- Ensure zero MSVC warnings (W4)
```

## Anti-Patterns

- **DO NOT** modify files in `libs/` (use patches)
- **DO NOT** combine shell commands with `&&` or `||`
- **DO NOT** use Unix-style redirections (`>/dev/null`, `2>/dev/null`)
- **DO NOT** rewrite entire files — make surgical edits
- **DO** verify CRLF line endings on all modified files
- **DO** verify UTF-8 without BOM
- **DO** verify zero trailing whitespace
