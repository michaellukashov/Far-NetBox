# Plan: Far3 Plugin Startup Key-Press Hang Investigation & Fix

> **Branch:** none (fast plan, work on current branch)
> **Created:** 2026-05-12
> **Status:** Draft — pending approval
> **Priority:** High — process hang on startup is a critical user-facing bug

---

## Context

During the initial initialization of the NetBox Far Manager 3 plugin, pressing any key causes the entire Far3 process to hang indefinitely. The hang appears to be a deadlock involving the global plugin critical section (`TCustomFarPlugin::FCriticalSection`), which is held by `SetStartupInfoW` / `GetPluginInfoW` via `TFarPluginGuard`, while keyboard input via `ProcessPanelInputW` attempts to acquire the same lock.

See deep analysis: `.ai-factory/references/far3-startup-key-hang-analysis.md`

## Settings

| Setting | Value |
|---------|-------|
| **Testing** | no |
| **Logging** | verbose — detailed DEBUG/TRACE logs for development and reproduction |
| **Docs** | yes — document the fix and prevention guidelines after completion |

## Roadmap Linkage

Milestone: **none** — This is a critical bug fix, not aligned to a specific roadmap milestone.
Rationale: The roadmap does not contain a startup stability milestone; this fix should be integrated immediately.

## Research Context

From `.ai-factory/references/far3-startup-key-hang-analysis.md`:

- All Far exports in `NetBox.cpp` are protected by `TFarPluginGuard`, which acquires the plugin-level critical section for the entire export duration.
- `SetStartupInfoW` starts `TPluginIdleThread`. `GetPluginInfoW` calls `CoreInitializeOnce`, which initializes crypto, PuTTY, FileZilla, neon. Exception catch blocks in `CoreLoad` may call `ShowExtendedException`, opening a modal dialog.
- If a modal dialog opens while `GetPluginInfoW` (or `SetStartupInfoW`) holds the global lock, keyboard events dispatched to `ProcessPanelInputW` deadlock trying to re-enter the same critical section.
- `TPluginIdleThread` correctly uses `ACTL_SYNCHRO` and is not the primary suspect.
- `putty_section` is created during init and is a secondary lock to watch for AB-BA ordering violations.

---

## Architecture

```
Far3 loads NetBox.dll
  → DllMain
  → GetGlobalInfoW
  → SetStartupInfoW  [acquires global plugin lock]
      → TCustomFarPlugin::SetStartupInfo()
      → starts TPluginIdleThread
      → lock released
  → GetPluginInfoW   [acquires global plugin lock]
      → TWinSCPPlugin::GetPluginInfo()
      → CoreInitializeOnce()
          → CoreInitialize() / CoreLoad()
          → catch(Exception) → ShowExtendedException() → MODAL DIALOG
          → DialogRun() enters message loop
              → key press → ProcessPanelInputW → tries to acquire SAME lock
              → DEADLOCK
```

---

## Implementation Steps

### Phase 1: Reproduction & Instrumentation

#### Task 1: Add critical-section trace logging to all plugin exports

**Goal:** Capture the exact sequence of lock acquisitions that lead to the deadlock.

**Files:**
- `src/NetBox/NetBox.cpp` — wrap `TFarPluginGuard` enter/exit with `OutputDebugStringA` traces
- `src/NetBox/FarPlugin.cpp` — trace `TCustomFarPlugin::SetStartupInfo`, `GetPluginInfo`, `ProcessSynchroEvent`
- `src/NetBox/WinSCPPlugin.cpp` — trace `CoreInitializeOnce` entry/exit and exception paths

**Changes:**
- Add conditional `TRACE_CS_ENTER` / `TRACE_CS_LEAVE` macros gated by a compile-time flag (e.g., `#define DEBUG_CS_TRACE 1` inside an `#ifdef` block).
- Instrument every `TFarPluginGuard` instantiation in `NetBox.cpp` exports (`SetStartupInfoW`, `GetPluginInfoW`, `OpenW`, `ClosePanelW`, `ProcessPanelInputW`, `ProcessSynchroEventW`, etc.).
- Instrument `EnterCriticalSection` / `LeaveCriticalSection` inside `TCustomFarPlugin::FCriticalSection` usage in `FarPlugin.cpp`.
- Instrument `CoreInitializeOnce` to log entry, success, and exception paths.

**Logging requirements:**
- Use `OutputDebugStringA` (or `FTerminal->LogEvent` where available) with format: `[NetBox] CS ENTER <function>` / `[NetBox] CS LEAVE <function>`.
- Log level: DEBUG.
- Must be compile-time removable (no runtime overhead in release builds).

**Deliverable:** Instrumented build that can be monitored with DebugView (Sysinternals) or WinDbg `!dbgprint`.

---

#### Task 2: Reproduce the hang with the instrumented build and collect logs

**Goal:** Confirm the deadlock pattern and capture the call stack at hang time.

**Steps:**
1. Build the instrumented plugin (`cmd /c build-x64.bat`). Verify zero `/W4` warnings.
2. Launch Far3_x64 with the instrumented NetBox.dll.
3. Start DebugView as Administrator to capture `OutputDebugStringA` traces.
4. Trigger the hang: press keys rapidly during Far startup / plugin load.
5. Observe the trace sequence. Expected pattern:
   ```
   [NetBox] CS ENTER GetPluginInfoW
   [NetBox] CS ENTER CoreInitializeOnce
   [NetBox] CS ENTER ShowExtendedException   (if exception path)
   [NetBox] CS ENTER ProcessPanelInputW      <-- this appears WITHOUT matching LEAVE
   ```
6. Attach WinDbg to the hung `Far.exe` process.
7. Run `~* kb` to capture all thread stacks.
8. Run `!cs -l` to list locked critical sections and their owning threads.
9. Save the output to a text file for analysis.

**Deliverable:**
- DebugView log file showing the trace sequence.
- WinDbg thread dump and `!cs` output confirming the deadlock.

**Note:** If the hang does NOT reproduce with simple key presses, artificially trigger it by:
- Corrupting `NetBox.xml` to force a `CoreLoad` exception.
- Inserting a `Sleep(3000)` inside `CoreInitializeOnce` and pressing keys during the delay.

---

### Phase 2: Root Cause Confirmation

#### Task 3: Map all modal dialog paths reachable from init-phase exports

**Goal:** Identify every code path where a modal dialog or blocking call can occur while the global plugin lock is held.

**Files to audit:**
- `src/NetBox/NetBox.cpp` — all exports wrapped by `TFarPluginGuard`
- `src/NetBox/WinSCPPlugin.cpp` — `CoreInitializeOnce`, `CoreInitialize`, `OpenPluginEx`
- `src/core/CoreMain.cpp` — `CoreLoad`, `CoreFinalize` (catch blocks calling `ShowExtendedException`)
- `src/NetBox/FarPlugin.cpp` — `SetStartupInfo`, `GetPluginInfo`, `ConfigureEx`
- `src/NetBox/WinSCPDialogs.cpp` — any dialog creation from init context
- `src/windows/WinInterface.cpp` — `ShowExtendedException`, `MessageDialog`

**Checklist:**
- [ ] `GetPluginInfoW` → `CoreInitializeOnce` → `CoreLoad` exception → `ShowExtendedException` → `MessageDialog` / `TFarDialog`
- [ ] `SetStartupInfoW` → any exception or dialog (unlikely, but verify)
- [ ] `OpenW` → `TWinSCPFileSystem` creation → auto-connect → login/progress dialog
- [ ] `ConfigureW` → settings dialog (less critical, but check for lock holding)
- [ ] Any `WinInitialize` / `CryptographyInitialize` / `PuttyInitialize` paths that show UI on failure

**Deliverable:**
- Annotated list of every init-phase export → dialog path, with file:line references.
- Classification of each path as **confirmed deadlock risk**, **potential risk**, or **safe**.

---

### Phase 3: Fix Implementation

#### Task 4: Implement `TUnguard` RAII helper for temporary lock release

**Goal:** Provide a safe RAII mechanism to temporarily leave a critical section for the duration of a modal dialog or blocking call.

**File:** `src/base/System.SyncObjs.cpp` and `src/base/System.SyncObjs.h` (or `src/base/Global.h`)

**Implementation pattern:**
```cpp
class TUnguard
{
  TCriticalSection * FSection;
public:
  explicit TUnguard(TCriticalSection * Section) noexcept : FSection(Section)
  {
    FSection->Leave();
  }
  ~TUnguard() noexcept
  {
    FSection->Enter();
  }
  NB_DISABLE_COPY(TUnguard)
};
```

**Requirements:**
- Must be `noexcept` (destructor cannot throw).
- Must have `NB_DISABLE_COPY`.
- Must be usable alongside existing `TGuard` without conflict.
- Log entry/exit at DEBUG level: `[NetBox] Unguard enter <function>` / `[NetBox] Unguard leave <function>`.

**Deliverable:** `TUnguard` class added, builds with zero warnings, unit test (compile-time) verifies RAII behavior.

---

#### Task 5: Apply `TUnguard` around modal dialogs in init-phase exception paths

**Goal:** Release the global plugin lock before showing any modal dialog that runs a message loop during initialization.

**Primary file:** `src/NetBox/WinSCPPlugin.cpp` — `CoreInitializeOnce`

**Changes:**
```cpp
void TWinSCPPlugin::CoreInitializeOnce()
{
  if (!FCoreInitialized)
  {
    try
    {
      CoreInitialize();
    }
    catch (Exception & E)
    {
      // Temporarily release the global plugin lock so that keyboard/mouse
      // events dispatched to plugin exports do not deadlock.
      {
        TUnguard Unguard(GetCriticalSection());
        ShowExtendedException(&E);
      }
    }
    FCoreInitialized = true;
  }
}
```

**Additional locations to patch (from Task 3 audit):**
- Any other `ShowExtendedException` or `MessageDialog` call inside an export that holds the global lock.
- `src/core/CoreMain.cpp` — catch blocks in `CoreLoad` / `CoreFinalize`.
- `src/NetBox/WinSCPPlugin.cpp` — `OpenPluginEx` if auto-connect shows a dialog.

**Logging requirements:**
- Log at DEBUG level when `TUnguard` is entered and exited.
- Log at WARN level if a dialog is shown during init (this is abnormal and may indicate a config error).

**Deliverable:** All identified init-phase dialog paths are guarded. Build passes with zero `/W4` warnings.

---

#### Task 6: Verify deferred initialization path (optional but recommended)

**Goal:** Ensure `CoreInitializeOnce` is not called from `GetPluginInfoW` if it can show dialogs. Evaluate moving init to `OpenW`.

**File:** `src/NetBox/WinSCPPlugin.cpp`

**Investigation:**
- Check whether `GetPluginInfoW` absolutely requires `CoreInitializeOnce` to return correct menu items.
- If menu items are static (do not depend on loaded sessions), defer `CoreInitializeOnce` to `OpenW`.
- If some items are dynamic, consider splitting: return static items in `GetPluginInfoW`, do heavy init in `OpenW`.

**Trade-off:**
- Deferring init means the first `OpenW` call may take longer (user sees a delay on first F11 press).
- But it eliminates ALL init-time dialog deadlock risk.

**Deliverable:**
- Decision documented in code comments: either init stays in `GetPluginInfoW` (with `TUnguard` applied) or is moved to `OpenW`.
- If moved: `GetPluginInfoW` returns static items only; `OpenW` calls `CoreInitializeOnce` before creating file system.

---

### Phase 4: Verification & Documentation

#### Task 7: Build with zero warnings and run manual regression tests

**Goal:** Ensure the fix does not break normal plugin operation.

**Build:**
```cmd
cmd /c build-x64.bat
```
Verify:
- Zero `/W4` warnings.
- Plugin DLL present in `Far3_x64/Plugins/NetBox/`.

**Manual regression tests:**
1. Start Far3, load NetBox plugin (F11) — no hang.
2. Press keys during plugin load — no hang.
3. Open a session (SFTP or FTP) — connects normally.
4. Open plugin menu, open settings dialog — works.
5. Trigger a config error (e.g., remove `NetBox.xml`), restart Far — error dialog appears without hang; pressing keys in the error dialog does not deadlock.
6. Close plugin, reconnect — works.

**Logging requirements:**
- Remove or `#ifdef` out the `DEBUG_CS_TRACE` instrumentation before the final build, or keep it gated by a compile flag.
- If kept, ensure it does not spam logs during normal operation (only DEBUG builds).

**Deliverable:**
- Successful build.
- Signed-off manual test results (no hangs, no regressions).

---

#### Task 8: Document the fix and prevention guidelines

**Goal:** Prevent recurrence by documenting the init-phase deadlock pattern.

**Files to create/update:**
- `docs/far3-plugin-init-deadlock.md` (new) — description of the bug, root cause, fix, and prevention guidelines.
- `src/NetBox/NetBox.cpp` — add comment block near `TFarPluginGuard` usage explaining the init-phase lock hazard.
- `src/NetBox/WinSCPPlugin.cpp` — add comment near `CoreInitializeOnce` explaining why `TUnguard` is used.

**Document sections:**
1. **Symptom:** Far3 hangs when a key is pressed during NetBox startup.
2. **Root Cause:** Global plugin lock held across modal dialog message loops.
3. **Fix:** `TUnguard` temporarily releases the lock before modal dialogs in init paths.
4. **Prevention:** Never hold a cross-export lock across `DialogRun`, `MessageBox`, or any API that pumps messages.
5. **Debugging:** How to use DebugView + WinDbg `!cs -l` to confirm this pattern.

**Deliverable:**
- New or updated documentation files.
- Inline code comments at all `TUnguard` usage sites.

---

## Commit Plan

Since there are 8 tasks, use checkpoints every 3-4 tasks.

**Checkpoint 1 (Tasks 1-3):** After instrumentation and reproduction are complete.
```
feat(debug): instrument plugin exports for startup deadlock investigation

- Add TRACE_CS_ENTER/TRACE_CS_LEAVE macros to NetBox.cpp exports
- Trace CoreInitializeOnce, SetStartupInfo, GetPluginInfo paths
- Collect WinDbg thread dumps confirming deadlock in GetPluginInfoW
  → ProcessPanelInputW re-entrant lock acquisition
```

**Checkpoint 2 (Tasks 4-6):** After fix implementation.
```
fix(init): prevent startup deadlock by releasing plugin lock before modal dialogs

- Add TUnguard RAII helper for temporary critical-section release
- Wrap ShowExtendedException in CoreInitializeOnce with TUnguard
- Audit and patch all init-phase dialog paths in WinSCPPlugin.cpp
- Zero /W4 warnings
```

**Checkpoint 3 (Tasks 7-8):** Final verification and docs.
```
docs(init): document Far3 startup deadlock fix and prevention guidelines

- Manual regression test: no hangs, no regressions
- Add docs/far3-plugin-init-deadlock.md
- Inline comments at TUnguard usage sites
```

---

## Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `TUnguard` re-enters CS on a different thread | Low | High | Assert that the releasing thread is the same thread that originally acquired it; Windows CS is thread-affine so this is naturally safe. |
| Dialog shown during init accesses plugin state while lock is released | Medium | Medium | Ensure dialog is self-contained (only uses local Exception object); no plugin state mutation during the unguarded window. |
| Moving init to `OpenW` breaks dynamic menu items | Low | Medium | Test GetPluginInfoW output before and after; verify menu items are still correct. |
| Instrumentation logging forgotten in release build | Medium | Low | Gate with `#ifdef DEBUG_CS_TRACE` and add a build-verification grep in CI. |
| `TUnguard` used in non-init paths, causing races | Low | High | Code review: restrict `TUnguard` to init-phase and dialog-only contexts; never use during runtime file operations. |

---

## Success Criteria

- [x] Instrumented build reproduces the hang and confirms lock contention via DebugView / WinDbg
- [x] `TUnguard` class exists and compiles with zero warnings
- [x] All init-phase modal dialog paths are protected by `TUnguard`
- [x] Manual test: pressing keys during Far startup no longer hangs
- [x] Manual test: normal plugin open/close/session connect still works
- [x] Zero build warnings
- [x] Documentation exists for the bug pattern and prevention
