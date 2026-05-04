# Plan - Implement Unfinished TODOs

> **Cross-reference:** This plan covers 4 TODOs in `src/base/SysUtils.cpp`.
> See the full inventory at `.ai-factory/TODO-INVENTORY.md` (~100 items across all of `src/`).


**Feature:** Implement unfinished TODO functions in core/base
**Branch:** feature/implement-todos

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

Found TODOs in core/base directories and SysUtils.cpp:
1. `Exception::Exception()` — use HelpContext parameter
2. `FileGetAttr()` — implement FollowLink parameter
3. `FileWrite()` — throw exception on error instead of return false
4. `IsDriveRooted()` — implement in Sysutils.hpp

---

## Tasks

### Phase 1: Implementation

##### task-1: Exception: use HelpContext parameter

- **Target:** `src/base/SysUtils.cpp`
- **Change:** Implement HelpContext in Exception constructors
- **Details:**
  - Store AHelpContext in exception
  - Pass to base Exception class
  - Add logging

##### task-2: FileGetAttr: implement FollowLink parameter

- **Target:** `src/base/SysUtils.cpp`
- **Change:** Implement FollowLink logic
- **Details:**
  - If FollowLink=true, resolve symbolic link
  - Use GetFinalPathNameByName or equivalent
  - Add logging

##### task-3: FileWrite: throw exception on error

- **Target:** `src/base/SysUtils.cpp`
- **Change:** Throw exception instead of returning false
- **Details:**
  - Create EInOutError or custom exception
  - Include error code and message
  - Add logging

##### task-4: IsDriveRooted: implement in Sysutils.hpp

- **Target:** `src/base/Sysutils.hpp`
- **Change:** Implement IsDriveRooted method
- **Details:**
  - Check if path starts with drive letter (C:\, etc.)
  - Handle UNC paths
  - Add logging

### Phase 2: Verification

##### task-5: Собрать проект

- **Target:** Modified files
- **Change:** Build
- **Details:** `cmd /c build-x64.bat`, zero warnings

---

## Commit Plan

5 tasks → checkpoints:

| Checkpoint | Tasks | Message |
|-------------|-------|----------|
| 1 | task-1-4 | `fix(todo): implement unfinished TODOs` |
| 2 | task-5 | `fix(todo): verify build` |

---

## Next Steps

```
/aif-implement
```

Plan: `.ai-factory/plans/implement-todos.md`

