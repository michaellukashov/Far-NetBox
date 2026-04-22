# Plan - Remote-to-Remote Copy

**Feature:** Add remote-to-remote copy capability (without local download) for SFTP, WebDAV, S3 protocols
**Branch:** feature/remote-to-remote-copy

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

NetBox already supports SCP remote-to-remote copy via `cp -r` command. The goal is to add similar functionality to other protocols:
- **SFTP** - SFTP protocol CopyFile
- **WebDAV** - HTTP COPY method
- **S3** - AWS S3 CopyObject API

---

## Tasks

### Phase 1: Investigation

##### task-1: Изучить существующий SCP remote-to-remote copy
- [x] Completed

- **Target:** `src/core/ScpFileSystem.cpp`, `src/core/ScpFileSystem.h`
- **Change:** Read CopyFile implementation in SCP
- **Details:**
  - How fsCopyFile is implemented in TSCPFileSystem
  - Command pattern: `cp -r %s -d -t "%s"`
  - How remote paths are delimited

##### task-2: Найти SFTP CopyFile реализацию
- [x] Completed

- **Target:** `src/core/SftpFileSystem.cpp`, `src/core/SftpFileSystem.h`
- **Change:** Review SFTP copy methods
- **Details:**
  - Check if CopyFile exists
  - Find SFTP protocol extensions for copy

##### task-3: Изучить WebDAV COPY метод
- [x] Completed

- **Target:** `src/core/WebDAVFileSystem.cpp`, `src/core/WebDAVFileSystem.h`
- **Change:** Review WebDAV CopyFile implementation
- **Details:**
  - Find CopyFileInternal method
  - Check HTTP COPY method

##### task-4: Изучить S3 CopyObject API
- [x] Completed

- **Target:** `src/core/S3FileSystem.cpp`, `src/core/S3FileSystem.h`
- **Change:** Review S3 copy methods
- **Details:**
  - Find CopyObject implementation
  - Check AWS S3 CopyObject API

### Phase 2: Implementation

##### task-5: Реализовать SFTP remote-to-remote copy
- [x] Completed

- **Target:** `src/core/SftpFileSystem.cpp`, `src/core/SftpFileSystem.h`
- **Change:** Implement CopyFile for SFTP if missing
- **Details:**
  - Use SFTP extension or rename+upload via temp
  - Add verbose logging

##### task-6: Реализовать WebDAV remote-to-remote copy
- [x] Completed

- **Target:** `src/core/WebDAVFileSystem.cpp`, `src/core/WebDAVFileSystem.h`
- **Change:** Verify/fix CopyFile for WebDAV
- **Details:**
  - Use HTTP COPY with Destination header

##### task-7: Реализовать S3 remote-to-remote copy
- [x] Completed

- **Target:** `src/core/S3FileSystem.cpp`, `src/core/S3FileSystem.h`
- **Change:** Verify/fix CopyObject for S3
- **Details:**
  - Use S3 CopyObject API

##### task-8: Интегрировать с Terminal интерфейсом
- [x] Completed

- **Target:** `src/core/Terminal.cpp`, `src/core/Terminal.h`
- **Change:** Ensure Terminal::CopyFiles routes correctly
- **Details:**
  - Check fsCopyFile case handling
  - Add protocol support detection

### Phase 3: Verification

##### task-9: Собрать проект
- [x] Completed

- **Target:** All modified files
- **Change:** Run build
- **Details:**
  - Use `cmd /c build-x64.bat`
  - Verify zero warnings (MSVC W4)

##### task-10: Протестировать в Far Manager
- [x] Completed

- **Target:** Plugin in Far3_x64/Plugins/NetBox/
- **Change:** Manual test
- **Details:**
  - Connect to SFTP server
  - Copy files between remote directories
  - Repeat for WebDAV, S3

---

## Commit Plan

10 tasks, commit checkpoints every 3-5:

| Checkpoint | Tasks | Commit Message |
|-----------|-------|--------------|
| 1 | task-1-4 | `feat(copy): research existing remote copy implementations` |
| 2 | task-5-8 | `feat(copy): implement remote-to-remote for SFTP/WebDAV/S3` |
| 3 | task-9-10 | `feat(copy): verify build and test` |

---

## Next Steps

To start implementation, run:
```
/aif-implement
```

Plan file: `.ai-factory/plans/remote-to-remote-copy.md`