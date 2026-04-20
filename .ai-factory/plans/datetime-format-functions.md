# Plan - DateTime Format Functions

**Feature:** Implement full FormatDateTime and ISO8601ToDate functions
**Branch:** feature/datetime-format-functions

**Created:** 2026-04-20

---

## Settings

| Setting | Value |
|---------|-------|
| Testing | No |
| Logging | Verbose |
| Docs | Yes |
| Roadmap Linkage | none |

---

## Research Context

Need to implement:
- `UnicodeString FormatDateTime(const UnicodeString & Fmt, const TDateTime & ADateTime)` - full format support
- `TDateTime ISO8601ToDate(const UnicodeString & S)` - ISO 8601 parser

Reference: FPC `dati.inc` FormatDateTime implementation

---

## Tasks

### Phase 1: Investigation

##### task-1: Изучить FormatDateTime в FPC (dati.inc)

- **Target:** `D:/Projects/FreePascal/FPCSource/rtl/objpas/sysutils/dati.inc`
- **Change:** Read FormatDateTime token parsing
- **Details:**
  - Token parsing loop
  - Format tokens: Y, M, D, H, N, S, Z, A, etc.
  - Date/Time separators

##### task-2: Изучить существующую реализацию в NetBox

- **Target:** `src/base/Sysutils.cpp`
- **Change:** Review current FormatDateTime
- **Details:** Only supports few formats

##### task-3: Определить формат токены

- **Target:** Analysis
- **Change:** Define supported tokens
- **Details:**
  - y, yy, yyy, yyyy - year
  - m, mm, mmm, mmmm - month
  - d, dd, ddd, dddd - day
  - h, hh, hhh, hhhh - hour
  - n, nn - minute
  - s, ss - second
  - z, zz, zzz - millisecond
  - AM/PM, AMPM

### Phase 2: Implementation

##### task-4: Implement FormatDateTime full functionality

- **Target:** `src/base/Sysutils.cpp`
- **Change:** Rewrite FormatDateTime with token parsing
- **Details:**
  - Parse format string token by token
  - Handle separators (: /)
  - Handle AM/PM

##### task-5: Implement ISO8601ToDate parser

- **Target:** `src/base/Sysutils.cpp`, `src/base/Sysutils.hpp`
- **Change:** Add ISO8601ToDate function
- **Details:**
  - Parse ISO 8601 format (YYYY-MM-DDTHH:MM:SS)
  - Handle timezone (Z, +HH:MM)
  - Throw on invalid

### Phase 3: Verification

##### task-6: Собрать проект

- **Target:** All modified files
- **Change:** Build
- **Details:** `cmd /c build-x64.bat`, zero warnings

---

## Commit Plan

6 tasks → checkpoints:

| Checkpoint | Tasks | Message |
|------------|-------|----------|
| 1 | task-1-3 | `feat(datetime): research format functions` |
| 2 | task-4-5 | `feat(datetime): implement format and parser` |
| 3 | task-6 | `feat(datetime): verify build` |

---

## Next Steps

```
/aif-implement
```

Plan: `.ai-factory/plans/datetime-format-functioms.md`