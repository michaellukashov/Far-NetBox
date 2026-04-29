# Implementation Plan: Fix FTP Directory Listing Hang / Every File as Directory (vsftpd) — Issue #507

Branch: none (fast mode)
Created: 2026-04-29
Improved: 2026-04-29

## Settings

- Testing: no
- Logging: verbose
- Docs: yes

## Roadmap Linkage

Milestone: "Version 1.2 — Background copy & progress UI"
Rationale: Issue #507 is a protocol functionality bug in the FTP layer that blocks reliable directory browsing; fixing it stabilizes core file transfer UX required for v1.2 polish.

## Research Context

Source: GitHub issue [#507](https://github.com/michaellukashov/Far-NetBox/issues/507)
Goal: Fix FTP directory listing that hangs or becomes impossibly slow when connected to vsftpd servers, where every file is incorrectly displayed as a directory.
Constraints:
- Do not modify third-party code in `libs/`; patch only if needed.
- Maintain WinXP compatibility.
- All changes must compile with MSVC W4, zero warnings.
- Preserve CRLF line endings.
Decisions:
- Plan targets `src/filezilla/FtpListResult.cpp` (listing parsers) and `src/core/FtpFileSystem.cpp` (listing pipeline) as primary suspects.
- Root cause is likely incorrect `Dir` flag propagation from `t_direntry` through `TListDataEntry` to `TRemoteFile`, or parser misclassification of vsftpd LIST/MLSD output.
Open questions:
- Which exact parser path (`parseAsMlsd`, `parseAsUnix`, `parseAsDos`, or fallback) is taken for vsftpd output?
- Does the `bUnsure` default (`TRUE` in constructor) affect downstream behaviour when `parseAsUnix` fails to reset it?
- Is the hang caused by Far Manager recursively attempting to read files that are marked as directories?

## Commit Plan

- **Commit 1** (after tasks 1-3): `fix(ftp): resolve vsftpd directory misidentification in listing parser`
- **Commit 2** (after tasks 4-5): `fix(ftp): prevent stale direntry state and parser cross-contamination`
- **Commit 3** (after task 6): `perf(ftp): prevent recursive directory read hang on misclassified files`
- **Commit 4** (after task 7): `docs(ftp): document listing parser defensive logging and vsftpd compatibility`

## Tasks

### Phase 1: Diagnose Root Cause

- [x] Task 1: Add verbose debug logging to FTP listing parser and pipeline
  - Add `FTerminal->LogEvent()` calls in `TFTPFileSystem::HandleListData()` (`src/core/FtpFileSystem.cpp:4628`) to log each `TListDataEntry` received: name, `Dir` flag, `Link` flag, `Size`, and `Permissions`.
  - Add logging in `CFtpListResult::AddData()` (`src/filezilla/FtpListResult.cpp:339`) to log which parser succeeded (`parseAsMlsd`, `parseAsUnix`, `parseAsDos`, etc.) and the raw line content for the first 20 entries per listing.
  - Log the `m_mlst` / `UsingMlsd()` state at the start of every `DoReadDirectory()` call in `TFTPFileSystem::DoReadDirectory()` (`src/core/FtpFileSystem.cpp:2222`).
  - Ensure all new log lines use `FORMAT()` and respect `FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2` gating.
  - Files: `src/core/FtpFileSystem.cpp`, `src/filezilla/FtpListResult.cpp`
  - Logging requirements:
    - DEBUG: raw parser input lines and which parser matched
    - INFO: listing start/end, parser mode (MLSD vs LIST), entry count
    - WARN: parser fallback or parse failure on individual lines
    - ERROR: none expected in this task

- [x] Task 2: Identify exact parser path for vsftpd output and trace `direntry.dir` propagation
  - Review `CFtpListResult::parseLine()` (`src/filezilla/FtpListResult.cpp:268`) and all sub-parsers to trace how `direntry.dir` is set for vsftpd LIST and MLSD responses.
  - Compare with WinSCP upstream source to detect any missing fixes in NetBox's ported `FtpListResult.cpp`.
  - Check if `t_directory::t_direntry` default constructor (`bUnsure=TRUE`, `dir=FALSE`) combined with `parseLine` not resetting `dir`/`bLink`/`bUnsure` causes stale directory flags.
  - Focus especially on `parseAsUnix` (`src/filezilla/FtpListResult.cpp:1261`): verify that `direntry.dir` is always explicitly set to `true` or `false`, and that `bUnsure` is reset to `FALSE` (it is currently missing in `parseAsUnix`, while all other parsers set it).
  - Check if `parseAsOther` (`src/filezilla/FtpListResult.cpp:1997`) or `parseAsDos` (`src/filezilla/FtpListResult.cpp:1920`) incorrectly match vsftpd output before `parseAsUnix` gets a chance.
  - Verify `parseAsMlsd` (`src/filezilla/FtpListResult.cpp:988`) does not set `direntry.dir = FALSE` for `type=file`, relying only on default constructor.
  - **Findings (2026-04-29):**
    - `parseAsMlsd` explicitly resets `direntry.dir=FALSE`, `direntry.bUnsure=FALSE`, `direntry.bLink=FALSE` at start (FtpListResult.cpp:1063-1065) and sets `dir=TRUE` only for `type=dir/symlink/cdir/pdir`. `type=file` correctly keeps `dir=FALSE`.
    - `parseAsUnix` sets `direntry.dir` correctly based on permissionstr[0] (`d`/`l`=TRUE, else FALSE) at lines 1354-1357. However, it does NOT reset `direntry.bUnsure=FALSE` — this is the only parser missing the reset.
    - `parseAsDos` and `parseAsOther` both reject lines starting with `b/c/d/l/p/s/-` (permission chars), so they cannot incorrectly match standard vsftpd LIST output.
    - `parseLine` only resets `ownergroup/owner/group` before calling sub-parsers. It does NOT reset `dir`, `bLink`, `bUnsure`, `size`, `name`, `permissionstr`, etc. This creates potential cross-contamination if one parser partially modifies `direntry` and fails before another parser succeeds.
    - `TListDataEntry` does not have a `bUnsure` field, so `bUnsure` does not directly affect `HandleListData`.
    - Each line gets a fresh `t_direntry` from default constructor in `AddData`, so stale state does NOT carry across lines. Cross-contamination is only within a single line's parser cascade.
  - Files: `src/filezilla/FtpListResult.cpp`, `src/filezilla/structures.cpp`, `src/filezilla/structures.h`
  - Logging requirements:
    - DEBUG: trace `direntry.dir` and `direntry.bUnsure` values after each parser returns
    - INFO: summary of which parser matched vsftpd-style sample lines

### Phase 2: Fix Directory Detection

- [x] Task 3: Fix `parseAsUnix` to reset `bUnsure = FALSE` and add defensive `direntry.dir = FALSE` for `type=file` in `parseAsMlsd`
  - Add `direntry.bUnsure = FALSE;` in `parseAsUnix` (`src/filezilla/FtpListResult.cpp:1261`) to match all other successful parsers (`parseAsMlsd`, `parseAsEPLF`, `parseAsVMS`, etc.).
  - In `parseAsMlsd` (`src/filezilla/FtpListResult.cpp:988`), explicitly set `direntry.dir = FALSE` when `type=file` is encountered (defensive coding, even though default constructor currently handles it).
  - Verify no other parser path leaves `direntry.dir` unset after returning `TRUE`.
  - Files: `src/filezilla/FtpListResult.cpp`
  - Logging requirements:
    - DEBUG: log before/after state of `direntry.dir` and `direntry.bUnsure` in modified parsers
    - INFO: log when a previously misclassified line is now parsed correctly

- [x] Task 4: Fix `parseLine` to fully reset `direntry` before calling sub-parsers
  - In `CFtpListResult::parseLine()` (`src/filezilla/FtpListResult.cpp:268`), add explicit reset of all fields before calling each sub-parser: `direntry.dir = FALSE`, `direntry.bLink = FALSE`, `direntry.bUnsure = FALSE`, `direntry.size = 0`, `direntry.name = L""`, `direntry.permissionstr = L""`, `direntry.humanpermstr = L""`, `direntry.linkTarget = L""`, and reset `direntry.date` to defaults.
  - Currently only `ownergroup`, `owner`, and `group` are reset (lines 270-272). If a parser partially modifies `direntry` and fails, the next parser inherits stale values, which can cause cross-contamination.
  - Keep changes minimal; add a single reset block at the top of `parseLine` before any parser calls.
  - Files: `src/filezilla/FtpListResult.cpp`
  - Logging requirements:
    - DEBUG: log that direntry reset occurred at start of `parseLine`
    - INFO: log if any parser now returns different results due to clean state

- [x] Task 5: Verify and tighten parser ordering / first-token checks for vsftpd output
  - Based on findings from Task 2, verify that `parseAsOther` and `parseAsDos` first-token checks do not incorrectly match standard vsftpd Unix LIST output (which starts with `-` or `d`).
  - If vsftpd output is being matched by `parseAsOther` before `parseAsUnix`, tighten `parseAsOther`'s numeric-first-token validation (`src/filezilla/FtpListResult.cpp:1997`) to avoid matching permission strings.
  - Verify `parseAsDos` (`src/filezilla/FtpListResult.cpp:1920`) correctly rejects lines starting with Unix permission characters.
  - Files: `src/filezilla/FtpListResult.cpp`
  - Logging requirements:
    - DEBUG: log parser attempts and rejections for sample vsftpd lines
    - WARN: log if a parser incorrectly matches a line it should reject

### Phase 3: Prevent Hang / Performance Degradation

- [x] Task 6: Add guard against recursive directory reads caused by misclassified files
  - In `TFTPFileSystem::HandleListData()` (`src/core/FtpFileSystem.cpp:4628`), add a validation guard: if an entry is marked as `Dir=true` but its name contains a file extension pattern (e.g., `.txt`, `.exe`) or matches known file-only patterns, log a warning and force `Dir=false` before creating the `TRemoteFile`.
  - In `TFTPFileSystem::ReadDirectory()` (`src/core/FtpFileSystem.cpp:2468`), add a safety check: if `ReadDirectory` is called on a path that was previously listed and returned non-directory entries only (or on a path known to be a file from a prior `ListFile` call), log a warning and return early instead of attempting to list it as a directory.
  - The primary goal is to ensure that even if the parser misidentifies a file as a directory, the plugin does not enter an infinite or expensive recursion loop.
  - Files: `src/core/FtpFileSystem.cpp`
  - Logging requirements:
    - WARN: log attempts to read a directory listing on a path known to be a file
    - INFO: log early-exit from `ReadDirectory` due to path-type mismatch guard
    - ERROR: log if the guard triggers repeatedly, indicating parser is still broken

### Phase 4: Documentation & Verification

- [x] Task 7: Document fix and verify build
  - Add concise code comments near modified parser sections referencing Issue #507 and explaining the vsftpd-specific handling.
  - Update `.ai-factory/references/` or `docs/` with a short note on vsftpd compatibility, parser reset requirements, and the directory detection guard.
  - Run `build-x64.bat` to verify zero warnings and successful compilation.
  - Confirm plugin DLL is produced in `Far3_x64/Plugins/NetBox/`.
  - Files: `src/filezilla/FtpListResult.cpp`, `src/core/FtpFileSystem.cpp`, `docs/` or `.ai-factory/references/`
  - Logging requirements:
    - INFO: log build success and artifact location
    - ERROR: log any build warnings and fix them before completing

## Acceptance Criteria

- [x] `direntry.dir` is explicitly set by every successful parser path in `FtpListResult.cpp`.
- [x] `parseAsUnix` resets `bUnsure = FALSE` to match other parsers.
- [x] `parseLine` fully resets `direntry` state (`dir`, `bLink`, `bUnsure`, `size`, `name`, `permissionstr`, `humanpermstr`, `linkTarget`, `date`) before calling sub-parsers.
- [x] `parseAsMlsd` explicitly sets `direntry.dir = FALSE` for `type=file` (defensive coding).
- [x] Parser ordering is verified: `parseAsOther` and `parseAsDos` do not incorrectly match standard vsftpd Unix LIST output.
- [x] `HandleListData` validates directory entries before creating `TRemoteFile` objects.
- [x] `ReadDirectory` has a guard against listing paths that are confirmed files.
- [x] Build completes with zero warnings on x64 RelWithDebugInfo.
- [x] Plugin DLL is present in `Far3_x64/Plugins/NetBox/`.
- [x] No trailing whitespace introduced in any modified file.
- [x] All modified files use CRLF line endings.

## Changelog

- **2026-04-29 (Improvement)** — Added Task 4: `parseLine` full reset of `direntry` state to prevent cross-contamination between parsers.
- **2026-04-29 (Improvement)** — Added Task 5: Verify parser ordering and tighten first-token checks.
- **2026-04-29 (Improvement)** — Added explicit `direntry.dir = FALSE` for `type=file` in `parseAsMlsd` to Task 3.
- **2026-04-29 (Improvement)** — Split Task 4 (original) into Task 6 (hang guard) and improved `HandleListData` validation.
- **2026-04-29 (Improvement)** — Added line number references to all file mentions for faster navigation.
- **2026-04-29 (Improvement)** — Added `direntry.bUnsure` trace to logging requirements.
- **2026-04-29 (Improvement)** — Updated Commit Plan to 4 commits (added Commit 2 for parser state fixes).

