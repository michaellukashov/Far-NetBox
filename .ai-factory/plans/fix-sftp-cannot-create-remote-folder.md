# Implementation Plan: Fix SFTP Cannot Create Remote Folder

Branch: current
Created: 2026-04-25

## Settings
- Testing: no
- Logging: verbose
- Docs: no

## Summary

Creating a remote directory via SFTP fails with `"Cannot get real path for '/share/...'"`.
Root cause: `TSFTPFileSystem::Canonify()` (line 3136 in `src/core/SftpFileSystem.cpp`)
calls `GetRealPath()` with the not-yet-existent target path. The server returns
`SSH_FXP_STATUS` with an error code, causing `GetRealPath()` to throw `ExtException`
wrapped with the user-visible message. `Canonify()` has `catch(...)` fallback blocks
at lines 3147-3156 and 3175-3184, but they check `FTerminal->GetActive()` before
falling back. When the connection was closed during exception handling, `GetActive()`
returns false and the catch blocks re-throw instead of falling back silently.

Fix: Remove the `GetActive()` guards — always execute the fallback unconditionally.

Reference: https://github.com/michaellukashov/Far-NetBox/issues/485

---

## Tasks

### Phase 1: Read and Understand

- [ ] **Task 1: Read affected functions**
  - Read `src/core/SftpFileSystem.cpp:3018-3095` (GetRealPath)
  - Read `src/core/SftpFileSystem.cpp:3136-3192` (Canonify)
  - Read `src/core/SftpFileSystem.cpp:4232-4239` (CreateDirectory)
  - Read `src/core/Terminal.cpp:5390-5430` (CreateDirectory / DoCreateDirectory)
  - Read `.ai-factory/AGENTS-Standards.md` (naming, formatting, code patterns)
  - Confirm: Canonify() is called from CreateDirectory, sends SSH_FXP_REALPATH,
    server rejects non-existent path, exception thrown, catch(...) should handle it
  - Note: Canonify() has 5 callers — CreateDirectory, RenameFile (x2), CreateLink (x2)
    — all benefit from robust fallback when paths do not yet exist

  **Files:** `src/core/SftpFileSystem.cpp` (read-only in this task)

### Phase 2: Apply Fix

- [ ] **Task 2: Remove GetActive() guards from Canonify catch blocks** (depends on Task 1)

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

- [ ] **Task 3: Build and verify zero warnings** (depends on Task 2)
  - Run: `cmd /c build-x64.bat`
  - Confirm: zero MSVC W4 warnings
  - If build fails: diagnose against current code, retry max 1 time
  - Stop and ask if error persists beyond 1 retry

  **Files:** none (build verification only)

---

## Commit Plan

Single commit at completion:

```
fix(sftp): prevent "Cannot get real path" error when creating remote directory

Remove FTerminal::GetActive() guards from Canonify() catch blocks in
TSFTPFileSystem. When GetRealPath() fails for a non-existent path during
directory creation, Canonify now always falls back to parent-path
canonicalization instead of re-throwing the exception.

Fixes #485
```

---

## Post-Implementation Manual Verification

1. Deploy plugin DLLs to `Far3_x64/Plugins/NetBox/` (build with `OPT_CREATE_PLUGIN_DIR=ON`)
2. Launch Far Manager, press F11, select Plugins, choose NetBox
3. Connect to SFTP server (QNAP or similar NAS)
4. Create a new subdirectory (e.g. `/share/CACHEDEV1_DATA/Download/Kodi/newdir`)
5. Confirm: directory created successfully, no error dialog appears
6. Check plugin log for `"Canonify: GetRealPath failed"` messages (expected — not errors)
