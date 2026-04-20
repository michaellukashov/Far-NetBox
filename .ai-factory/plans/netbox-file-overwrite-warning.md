# Plan - NetBox File Overwrite Warning

**Feature:** Add warning when copying .netbox file if target already exists
**Branch:** feature/netbox-overwrite-warning

**Created:** 2026-04-20

---

## Settings

| Setting | Value |
|---------|-------|
| Testing | No |
| Logging | Verbose |
| Docs | Yes (mandatory) |
| Roadmap Linkage | Milestone: none |

---

## Research Context

When copying .netbox files:
- **Export (plugin → local):** Warning if local .netbox file already exists
- **Import (local → plugin):** Warning if connection with same name already exists

---

## Tasks

### Phase 1: Investigation

##### task-1: Изучить ImportSessions логику

- **Target:** `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Read ImportSessions() method
- **Details:** How sessions are imported from .netbox file

##### task-2: Найти PutFilesW обработку

- **Target:** `src/NetBox/NetBox.cpp`, `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Find where PutFilesW handles .netbox files
- **Details:** Understand export flow

### Phase 2: Implementation

##### task-3: Добавить проверку существования файла при экспорте

- **Target:** `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Before export, check if local .netbox file exists
- **Details:**
  - Use FileExists() API
  - Show warning dialog if exists

##### task-4: Добавить проверку существования подключения при импорте

- **Target:** `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Before import, check if connection name already exists
- **Details:**
  - Query stored sessions
  - Compare with imported session name

##### task-5: Добавить диалог подтверждения

- **Target:** `src/windows/`
- **Change:** Reuse or create confirmation dialog
- **Details:**
  - Show: "File/connection exists. Overwrite?"
  - Yes/No/Cancel buttons

### Phase 3: Verification

##### task-6: Собрать проект

- **Target:** Modified files
- **Change:** Build
- **Details:** `cmd /c build-x64.bat`, zero warnings

##### task-7: Протестировать в Far Manager

- **Target:** Plugin
- **Change:** Manual test
- **Details:** Copy .netbox file both directions

---

## Commit Plan

7 tasks → checkpoints every 3-5:

| Checkpoint | Tasks | Message |
|------------|-------|----------|
| 1 | task-1-2 | `feat(warn): research netbox copy flow` |
| 2 | task-3-5 | `feat(warn): add overwrite warnings` |
| 3 | task-6-7 | `feat(warn): verify` |

---

## Next Steps

```
/aif-implement
```

Plan: `.ai-factory/plans/netbox-file-overwrite-warning.md`