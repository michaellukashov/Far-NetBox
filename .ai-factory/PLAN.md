# Plan - Add French Translation File NetBoxFr.lng

## Settings

- **Testing:** No
- **Logging:** Verbose
- **Docs:** No
- **Roadmap:** None

## Tasks

### I. Implementation

1. ✅ **Create French language file from English template**
   - File: `src/NetBox/NetBoxFr.lng`
   - Copied `NetBoxEng.lng` as base
   - Header: `.Language=French,Français`

2. ✅ **Add Debug 3 to both log sections**
   - File already has "Debug 1", "Debug 2", "Debug 3" in both sections

3. ✅ **Ensure file format compliance**
   - UTF-8 BOM (EF BB BF) at start ✅
   - CRLF line endings ✅
   - Trailing newline ✅
   - Line count: 1331 (matches other files) ✅

4. ✅ **Add to CMakeLists.txt**
   - Added `NetBox/NetBoxFr.lng` to `NetBoxResources` target

### II. Build & Verify

5. ✅ **Build Plugin**
   - Build: Zero warnings

## Commit Plan

Single commit:

```
feat(i18n): add French translation file NetBoxFr.lng
```