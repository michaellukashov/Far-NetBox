# Implementation Plan: Fix SFTP Cannot Create Remote Folder

Branch: current
Created: 2026-04-25

## Settings
- Testing: no
- Logging: verbose
- Docs: no

## Summary

Creating a remote directory via SFTP fails with `"Cannot get real path for '/share/...'"`.

**Primary root cause:** `TTerminal::DoCreateDirectory()` calls `AbsolutePath(ADirName, false)`
for the `TMkdirSessionAction` log (line 5417 in `src/core/Terminal.cpp`). The second
parameter `false` means "not local" — it calls `GetRealPath()` which sends `SSH_FXP_REALPATH`
to the server. Since the target directory does not exist yet, the server returns an error
and `GetRealPath()` throws. The `TMkdirSessionAction` constructor propagates this exception
upward as the user-visible message.

**Fix 1 (primary):** Change `AbsolutePath(ADirName, false)` to `AbsolutePath(ADirName, true)`.
When `Local=true`, `AbsolutePath()` calls `LocalCanonify()` (path normalization only,
no server round-trip). Applied at `src/core/Terminal.cpp:5417`.

**Secondary root cause:** `TSFTPFileSystem::Canonify()` (line 3136 in `src/core/SftpFileSystem.cpp`)
calls `GetRealPath()` internally. `Canonify()` has `catch(...)` fallback blocks at lines
3147-3156 and 3175-3184, but they check `FTerminal->GetActive()` before falling back.
When the connection was closed during exception handling, `GetActive()` returns false
and the catch blocks re-throw instead of falling back silently.

**Fix 2 (defensive):** Remove the `GetActive()` guards — always execute the fallback
unconditionally. Benefits all 5 Canonify callers: CreateDirectory, RenameFile (x2),
CreateLink (x2).

Reference: https://github.com/michaellukashov/Far-NetBox/issues/485

---

## Tasks

### Phase 1: Read and Understand

- [x] **Task 1: Read affected functions**
  - Read `src/core/SftpFileSystem.cpp:3018-3095` (GetRealPath)
  - Read `src/core/SftpFileSystem.cpp:3136-3192` (Canonify)
  - Read `src/core/SftpFileSystem.cpp:4232-4239` (CreateDirectory)
  - Read `src/core/Terminal.cpp:5390-5430` (CreateDirectory / DoCreateDirectory)
  - Read `.ai-factory/AGENTS-Standards.md` (naming, formatting, code patterns)
  - Confirm: Canonify() is called from CreateDirectory, sends SSH_FXP_REALPATH,
    server rejects non-existent path, exception thrown, catch(...) should handle it
  - Note: Canonify() has 5 callers — CreateDirectory, RenameFile (x2), CreateLink (x2)
    — all benefit from robust fallback when paths do not yet exist

  **Files:** `src/core/SftpFileSystem.cpp`, `src/core/Terminal.cpp` (read-only in this task)

- [x] **Task 1b: Read AbsolutePath call in DoCreateDirectory**
  - Read `src/core/Terminal.cpp:5412-5430` (DoCreateDirectory)
  - Confirm: `AbsolutePath(ADirName, true)` uses `LocalCanonify()` (no server query)
  - Note: `AbsolutePath(APath, false)` calls `GetRealPath()` which sends `SSH_FXP_REALPATH`
    to server — this is the primary root cause of the error
  - Note: `TMkdirSessionAction` constructor at line 5417 takes the AbsolutePath result;
    when Local=false, the server query fails for non-existent paths

**Files:** `src/core/Terminal.cpp` (read-only in this task)

### Phase 2: Apply Fix

- [x] **Task 2: Remove GetActive() guards from Canonify catch blocks** (depends on Task 1)

  **File:** `src/core/SftpFileSystem.cpp`

  This fix improves all 5 Canonify() callers — CreateDirectory (line 4235),
  RenameFile source/target (lines 4173, 4175), CreateLink file/target (lines 4317, 4321).
  When any of these canonicalize a path that does not yet exist, the fallback now
  always returns the original path instead of re-throwing.

  **Change 1** — First catch block (lines 3147-3156):

  BEFORE:
  ```
    catch(...)
    {
      if (FTerminal->GetActive())
      {
        TryParent = true;
      }
      else
      {
        throw;
      }
    }
  ```

  AFTER:
  ```
    catch(...)
    {
      FTerminal->LogEvent(FORMAT("Canonify: GetRealPath failed for \"%s\", falling back to parent", APath));
      TryParent = true;
    }
  ```

  **Change 2** — Second catch block (lines 3175-3184):

  BEFORE:
  ```
    catch(...)
    {
      if (FTerminal->GetActive())
      {
        Result = Path;
      }
      else
      {
        throw;
      }
    }
  ```

  AFTER:
  ```
    catch(...)
    {
      FTerminal->LogEvent(FORMAT("Canonify: GetRealPath failed for parent \"%s\", using original path", Path3));
      Result = Path;
    }
  ```

  **Logging requirements:**
  - Log both fallback paths at `LogEvent(0, ...)` level (DEBUG equivalent)
  - Use `FORMAT(...)` macro to match existing `Canonify()` logging pattern (lines 3140, 3189)
  - Existing `Canonify()` entry/exit logs already cover the success path

  **Conventions:**
  - Follow AGENTS-Standards.md: CRLF line endings, no trailing whitespace, no BOM
  - Keep changes minimal — no refactoring of surrounding code

  **Files:** `src/core/SftpFileSystem.cpp`

### Phase 3: Build

- [x] **Task 3: Build and verify zero warnings** (depends on Task 2)
  - Run: `cmd /c build-x64.bat`
  - Confirm: zero MSVC W4 warnings
  - If build fails: diagnose against current code, retry max 1 time
  - Stop and ask if error persists beyond 1 retry

  **Files:** none (build verification only)


### Phase 4: Verification and Investigation

- [ ] **Task 4: Verify fix with original bug path `/share/...`** (depends on Task 3)
  - The verification error showed path `CACHEDEV1_DATA/Download/Kodi/newdir` (relative, no `/share/`).
  - The original bug report (#485) used absolute path `/share/CACHEDEV1_DATA/Download/Kodi/newdir`.
  - Re-test with the **exact absolute path** from the bug report.
  - The Canonify fallback correctly preserves absolute paths — the `/share/` prefix
    should be present if the SFTP session's current directory includes it.
  - Confirm: directory created successfully when path has `/share/` prefix.

  **Files:** none (manual Far Manager testing)

- [ ] **Task 5: Investigate upstream `/share/` prefix loss** (depends on Task 4)
  - If Task 4 fails with the same error, the `/share/` prefix is lost upstream of Canonify.
  - Trace: `FarPlugin::MakeDirectory` (line 2151) receives `Info->Name` from Far Manager.
    This is the user-typed directory name. If the user types a relative path
    (e.g., `CACHEDEV1_DATA/Download/Kodi/newdir` instead of just `newdir`),
    `LocalCanonify` will attempt to combine it with `FCurrentDirectory`.
  - If `FCurrentDirectory` = `/share/CACHEDEV1_DATA/Download/Kodi/`, `AbsolutePath` produces:
    `/share/CACHEDEV1_DATA/Download/Kodi/CACHEDEV1_DATA/Download/Kodi/newdir` (doubled path).
  - This doubled path causes `GetRealPath` to fail → fallback returns relative path → SFTP rejects.
  - Determine: Is this a user input issue (typing full relative path instead of directory name)
    or a Far Manager plugin bug (constructing wrong `Info->Name`)?
  - Check: Does `FCurrentDirectory` always include `/share/`? Verify via plugin log.

  **Files:** `src/NetBox/FarPlugin.cpp`, `src/NetBox/WinSCPFileSystem.cpp`, `src/core/SftpFileSystem.cpp`
---

## Commit Plan

Single commit at completion:

```
fix(sftp): prevent "Cannot get real path" error when creating remote directory

Two changes fix this issue:

1. DoCreateDirectory: AbsolutePath(ADirName, false) -> true
   TMkdirSessionAction was constructed with AbsolutePath(Local=false), which
   calls GetRealPath() and sends SSH_FXP_REALPATH to the server. Since the
   target directory does not exist yet, the server returns an error and
   GetRealPath() throws. Using Local=true calls LocalCanonify() instead,
   avoiding the server round-trip entirely for non-existent paths.

2. Canonify: Remove FTerminal::GetActive() guards from catch(...) blocks
   When GetRealPath() fails for any reason (not just directory creation),
   Canonify now always falls back to parent-path canonicalization instead
   of re-throwing when GetActive() returns false. This benefits all 5
   Canonify callers: CreateDirectory, RenameFile (x2), CreateLink (x2).

Fixes #485
```

---

## Post-Implementation Manual Verification

1. Deploy plugin DLLs to `Far3_x64/Plugins/NetBox/` (build with `OPT_CREATE_PLUGIN_DIR=ON`)
2. Launch Far Manager, press F11, select Plugins, choose NetBox
3. Connect to SFTP server (QNAP or similar NAS)
4. Create a new subdirectory (e.g. `/share/CACHEDEV1_DATA/Download/Kodi/newdir`).
   **IMPORTANT:** Use the full absolute path with `/share/` prefix.
   If the verification error `CACHEDEV1_DATA/Download/Kodi/newdir` (no `/share/`) recurs,
   the issue is upstream of Canonify — see Task 5.
5. Confirm: directory created successfully, no error dialog appears
6. Check plugin log for `"Canonify: GetRealPath failed"` messages (expected — not errors)

**Expected logs after fix:**
- `TMkdirSessionAction` log now uses `LocalCanonify()` — no `SSH_FXP_REALPATH`
  server query for the action log entry, avoiding the exception trigger entirely.
- `Canonify` fallback logs may still appear for other operations (RenameFile,
  CreateLink) if their target paths do not exist — these are expected, not errors.
