# Fix: Alt-F12 Folder History Does Not Restore Session (Panel Aborted)

**Branch:** feature/fix-folder-history-not-working
**Created:** 2026-05-01
**Type:** Bug Fix
**Related:** `folder-history-navigation-v2.md` (feature was implemented but has a runtime bug)

## Settings

- **Testing:** No
- **Logging:** Verbose -- add `AppLogFmt` to trace the restore path
- **Docs:** No

## Bug Description

Pressing **Alt-F12** to select a previously visited NetBox session from Far Manager's folder history does nothing -- the panel does not open. The history entry is visible in the list, selecting it produces no visible effect.

## Root Cause

In `WinSCPPlugin.cpp` `OpenPluginEx()`, the abort guard at **lines 420-425** fires unconditionally when `Entry.Valid && Directory.IsEmpty()` is false, even when there is **no existing panel** to reuse:

```cpp
// Lines 420-425 — THE BUG
else if (OpenFrom == OPEN_SHORTCUT || OpenFrom == OPEN_COMMANDLINE)
{
  // If we have a valid history entry but no matching connected panel,
  // don't attempt to create a new session — that causes reconnect loops.
  // Far Manager will handle the history navigation natively.
  if ((OpenFrom == OPEN_SHORTCUT && Entry.Valid) ||
      (OpenFrom == OPEN_COMMANDLINE && Entry.Valid && !Directory.IsEmpty()))
  {
    Abort();
    return nullptr;  // <-- No panel is created!
  }
  // ... ParseUrl path (never reached when Entry.Valid)
}
```

**Flow when Alt-F12 is pressed (no existing panel):**
1. Far Manager calls `OpenPluginEx(OpenFrom=OPEN_COMMANDLINE, CommandLine="netbox://MySession//home/user")`
2. Line 326: `DecodeSessionParam()` returns `{Valid=true, SessionName="MySession", RemoteDirectory="/home/user"}`
3. Line 333: `CommandLine` is rewritten to `"netbox:MySession"` (directory stripped, preserved in `Directory` variable)
4. Line 356: `GetPanelFileSystem()` returns `nullptr` (no panel open)
5. Falls through to line 399-402 (no existing panel found)
6. Line 406: `FileSystem` is created
7. **Line 420**: condition is TRUE (`OPEN_COMMANDLINE && Entry.Valid && !Directory.IsEmpty()`)
8. **Line 423-424**: `Abort()`, `return nullptr` -- the newly created `FileSystem` is leaked, no panel opens

The comment is misleading: "Far Manager will handle the history navigation natively" -- but Far Manager has **no panel to navigate** when no panel is open. The session creation path (ParseUrl) at lines 431-459 is unreachable when `Entry.Valid`.

## Fix

Remove the abort guard at lines 420-425. The existing reuse check at lines 351-403 already handles:
- Session matches existing panel → reuse (lines 366-376, 378-389)
- Session doesn't match → abort to prevent reconnect loop (lines 391-397)
- No existing panel → fall through to ParseUrl (correct behavior)

## Tasks

### Phase 1: Fix the abort guard

- [x] **TASK-1: Remove the overly aggressive abort in OpenPluginEx for valid history entries and add reentrancy guard to prevent reconnect loops**
  - **File:** `src/NetBox/WinSCPPlugin.cpp`, `src/NetBox/WinSCPPlugin.h`
  - **Deliverable:** 
    1. Add `FCreatingPanel` flag to `TWinSCPPlugin` class (WinSCPPlugin.h)
    2. Check `FCreatingPanel` at the start of `OpenPluginEx` — abort reentrant calls to prevent reconnect loops
    3. Wrap FileSystem creation in `try__finally` with `FCreatingPanel = true/false`
    4. Remove the abort guard at lines 420-425 that blocked legitimate session restoration
    5. Add `return nullptr` after `CommandsMenu(true)` to fix control path warning
  - **Why this approach:** The original abort at lines 420-425 prevented BOTH reconnect loops (good) AND legitimate session restoration (bad). The `FCreatingPanel` flag distinguishes these cases: it catches reentrant calls triggered by `UpdatePanelDirectoryParam()` during panel creation, while allowing the first call to proceed through `ParseUrl → Connect → SetDirectoryEx`.
  - **Code flow after fix:**
    ```
    Alt-F12 (no existing panel)
      → FCreatingPanel = false → proceed
      → No existing panel → reuse check skipped
      → FileSystem created
      → FCreatingPanel = true
      → ParseUrl → Connect() → UpdatePanelDirectoryParam()
        → FCTL_SETPANELDIRECTORY triggers reentrant OPEN_SHORTCUT
        → Reentrant call: FCreatingPanel = true → Abort()
      → SetDirectoryEx(Directory) → return panel
      → FCreatingPanel = false
    ```
  - **Logging:** Added `AppLogFmt` for reentrant call detection
  - **Dependency:** None

## Verification

After the fix:
1. Build with `cmd /c build-x64.bat` -- zero warnings
2. Open a NetBox session, navigate to a subdirectory
3. Close the panel
4. Press Alt-F12, select the session entry
5. Session should connect and navigate to the correct directory
6. Verify no `AppLogFmt` errors in the log

## Why the original guard was added

The guard was added as a safety measure to prevent reconnect loops when Far Manager's history mechanism fires `OPEN_SHORTCUT`/`OPEN_COMMANDLINE` back into `OpenPluginEx`. The reconnect loop issue was real but was solved by the reuse check (lines 351-403) which catches session name mismatches and aborts at that point. The extra guard at lines 420-425 is redundant and breaks the legitimate case of restoring a session when no panel is open.

---

### Changelog

#### 2026-05-01: Initial analysis
- Traced the full execution path from Alt-F12 to the abort
- Identified that the reuse check (lines 351-403) already handles all edge cases
- Single-line fix: remove lines 420-425
