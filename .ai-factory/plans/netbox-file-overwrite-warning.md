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

##### task-3: Добавить проверку существования файла при экспорте → ALREADY IMPLEMENTED

- **Status:** ✅ Complete - Already implemented in ExportSession() at lines ~2815-2822
- **Code:**
  ```cpp
  if (FileExists(XmlFileName))
  {
    UnicodeString ConfirmMsg = FORMAT("File %s already exists. Overwrite?", XmlFileName);
    if (MoreMessageDialog(ConfirmMsg, nullptr, qtConfirmation, qaYes | qaNo | qaCancel) != qaYes)
    {
      return;
    }
  }
  ```

##### task-4: Добавить проверку существования подключения при импорте → ALREADY IMPLEMENTED

- **Status:** ✅ Complete - Already implemented in ImportSessions() at lines ~3012-3022
- **Code:** Shows warning when importing sessions with existing names:
  ```cpp
  UnicodeString ConfirmMsg = FORMAT("%s will import sessions: %s. Continue?", FileName, SessionNames);
  ```

##### task-5: Добавить диалог подтверждения → REUSE (not needed)


- **Status:** ✅ Uses existing MoreMessageDialog() API
- No separate dialog needed

---

- [x] task-3: Export file exists check (already implemented)
- [x] task-4: Import session name warning (already implemented)
- [x] task-5: Reuse confirmation dialog

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