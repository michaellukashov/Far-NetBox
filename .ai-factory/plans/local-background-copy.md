# Plan - Local Background Copy

**Feature:** Add local-to-local background copy capability via NetBox plugin
**Branch:** feature/local-background-copy

**Created:** 2026-04-20

---

## Settings

| Setting | Value |
|---------|-------|
| Testing | No |
| Logging | Verbose |
| Docs | Yes (mandatory) |
| Roadmap Linkage | Milestone: none (skipped by user) |

---

## Research Context

NetBox already implements remote file operations (GetFiles, PutFiles) via plugin interface. The user wants to add local-to-local background copy so users can copy files without blocking Far Manager UI.

---

## Tasks

### Phase 1: Investigation

##### task-1: Исследовать существующую реализацию PutFiles/GetFiles

- **Target:** `src/NetBox/NetBox.cpp`, `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Read PutFilesW, GetFilesW, FarPlugin->PutFiles() implementations
- **Details:**
  - Understand how plugin interface works
  - Find where GetFiles/PutFiles delegates to TTerminal
  - Identify file transfer flow

##### task-2: Найти паттерны фоновых операций в Terminal

- **Target:** `src/core/Terminal.cpp`, `src/core/Terminal.h`, `src/core/Queue.cpp`
- **Change:** Search for async queue patterns
- **Details:**
  - Look for TFileOperationProgress::Progress in copy operations
  - Find how other transfers handle progress updates
  - Understand queue system for remote operations

##### task-3: Определить интерфейс LocalFileSystem

- **Target:** `src/core/FileSystems.h`
- **Change:** Review TCustomFileSystem interface for local operations
- **Details:**
  - Does NetBox have local filesystem abstraction?
  - Use Windows API (CopyFileEx) for local copies

### Phase 2: Implementation

##### task-4: Создать LocalCopyOperation класс

- **Target:** `src/core/LocalFileOperation.cpp`, `src/core/LocalFileOperation.h`
- **Change:** Create new class for local file copy operations
- **Details:**
  - Implement copy using Windows CopyFileEx API
  - Support progress callback for UI updates
  - Use worker thread for background execution

##### task-5: Реализовать постановку в очередь копирования

- **Target:** `src/core/Queue.cpp`
- **Change:** Add queue entry for local operations
- **Details:**
  - Integrate with existing queue system if possible
  - Or create separate queue for local operations

##### task-6: Интегрировать с PutFilesW

- **Target:** `src/NetBox/WinSCPFileSystem.cpp`
- **Change:** Modify PutFiles handler
- **Details:**
  - Detect local source and destination
  - Route to LocalCopyOperation instead of remote protocol

##### task-7: Добавить UI индикатор прогресса

- **Target:** `src/windows/CopyProgressDlg.cpp` or new
- **Change:** Reuse or create progress dialog
- **Details:**
  - Show copy progress in background
  - Allow cancellation

### Phase 3: Verification

##### task-8: Собрать проект

- **Target:** All modified files
- **Change:** Run build
- **Details:**
  - Use `cmd /c build-x64.bat`
  - Verify zero warnings

##### task-9: Протестировать в Far Manager

- **Target:** Plugin in Far3_x64/Plugins/NetBox/
- **Change:** Manual test
- **Details:**
  - Open Far Manager
  - Select local files
  - Copy to another local path
  - Verify background operation

---

## Commit Plan

Given 9 tasks, commit checkpoints every 3-5 tasks:

| Checkpoint | Tasks | Commit Message |
|-----------|-------|-----------------|
| 1 | task-1-3 | `feat(copy): research existing transfer patterns` |
| 2 | task-4-7 | `feat(copy): implement local background copy` |
| 3 | task-8-9 | `feat(copy): verify build and test`

---

## Next Steps

To start implementation, run:
```
/aif-implement
```

Plan file: `.ai-factory/plans/local-background-copy.md`