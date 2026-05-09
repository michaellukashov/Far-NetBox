# Far3 Plugin Startup Hang Investigation

> **Bug:** Far Manager 3 hangs when a key is pressed during NetBox plugin startup.
> **Status:** Resolved — root cause identified as Far3 startup race condition, fixed in Far3 6682.
> **Investigated:** 2026-05-12
> **Component:** Plugin initialization (`NetBox.cpp`, `FarPlugin.cpp`, `WinSCPPlugin.cpp`)

---

## Symptom

When Far Manager 3 loads the NetBox plugin during startup, pressing any key before the file panels appear causes the entire Far3 process to hang indefinitely. The process must be killed from Task Manager.

Key observations:
- Hang **only** occurs if a key is pressed **during** Far3 startup (before panels appear)
- Hang **never** occurs after Far3 has fully loaded and panels are visible
- Hang **does not** reproduce under a debugger (timing-sensitive Heisenbug)
- Hang **does not** reproduce with Far3 version 3.0.6682.0 x64 or later

---

## Root Cause

The hang is a **timing-sensitive race condition in Far3's startup sequence**, not a NetBox plugin deadlock. Investigation showed:

1. **NetBox initialization completes successfully** — `SetStartupInfoW` and `GetPluginInfoW` both enter and leave normally
2. **No NetBox export is invoked** when the key is pressed — no `ProcessPanelInputW`, `ProcessPanelEventW`, or `ProcessSynchroEventW` traces appear in DebugView
3. **The hang is upstream of the plugin** — Far3 itself becomes unresponsive before dispatching the key event to any plugin
4. **Console mode is correct** — `ENABLE_LINE_INPUT` is not set, ruling out a cooked-mode console read
5. **Far3 6682 fixes the issue** — the hang disappears in newer Far3 builds

### Why NetBox Triggered It

NetBox opens `CONIN$` (console input) during `CreatePlugin()` and starts a background idle thread (`TPluginIdleThread`). The combination of:
- `CONIN$` handle ownership
- Background thread posting `FE_IDLE` synchro events
- Far3 reading console input during its own startup (startup macros, initial key state)

...creates a narrow timing window where a key press during startup triggers a race in Far3's console input handling or plugin load reentrancy.

---

## NetBox Defensive Changes

Although the root cause was in Far3, the investigation produced several defensive improvements that remain in the codebase:

### 1. `TUnguard` Lock Release in `HandleException`

Applied `TUnguard` (temporary lock release) in both `TCustomFarPlugin::HandleException` and `TWinSCPPlugin::HandleException` before showing modal dialogs. This prevents a **genuine deadlock** if an exception occurs during initialization while the global plugin lock is held:

```cpp
void TCustomFarPlugin::HandleException(Exception * E, OPERATION_MODES /*OpMode*/)
{
  DebugAssert(E);
  // Release the global plugin lock before showing a modal dialog.
  // Far may dispatch keyboard events to plugin exports while the dialog
  // message loop runs; holding the lock would cause a deadlock.
  TUnguard Unguard(GetCriticalSection());
  Message(FMSG_WARNING | FMSG_MB_OK, L"", E ? E->Message : L"");
}
```

### 2. `TUnguard` Const-Correctness Fix

Changed `TUnguard` constructor to accept `const TCriticalSection &` (matching `TGuard`), since `TCriticalSection::Enter()` and `Leave()` are `const` methods using `mutable` internal state.

### 3. Diagnostic Instrumentation (`DEBUG_PRINTFA`)

Added `DEBUG_PRINTFA`-based trace instrumentation to key initialization and export paths for future debugging:
- `TExportTracer` RAII struct in `NetBox.cpp` — logs ENTER/LEAVE for all Far exports
- `LogConsoleMode()` helper — traces console input mode flags at export boundaries
- `CheckForEsc()` traces — logs console input event counts and ESC detection
- `CoreLoad`, `SetStartupInfo`, `GetPluginInfo` traces
- Idle thread `FE_IDLE` synchro posting trace

---

## Prevention Guidelines

**Rule:** Never hold a cross-export lock across any API that pumps messages or runs a modal event loop.

**Applies to:**
- `DialogRun()` / `DialogInit()` (Far dialog API)
- `Message()` / `Menu()` (Far message/menu API)
- `MessageBox()` / `ShowExtendedException()` (plugin dialog wrappers)
- Any Windows API that pumps messages (`GetMessage`, `PeekMessage` with `PM_REMOVE`, `WaitMessage`)

**Pattern:**
```cpp
// WRONG — deadlock risk
void SomeExport()
{
  TFarPluginGuard Guard;  // acquires global lock
  ShowSomeDialog();       // pumps messages, may dispatch to another export
}                         // lock released here

// CORRECT — release lock before modal operation
void SomeExport()
{
  TFarPluginGuard Guard;  // acquires global lock
  DoNonBlockingWork();
  {
    TUnguard Unguard(GetCriticalSection());  // temporarily release
    ShowSomeDialog();                        // safe: lock is free
  }                                          // lock re-acquired
}                         // lock released here
```

---

## Debugging Startup Hangs

If you suspect a similar startup-only hang:

1. **Start DebugView** as Administrator — capture plugin trace output
2. **Start Far3** — do not press any key until panels appear
3. **Verify initialization** — check for ENTER/LEAVE pairs for `SetStartupInfoW` and `GetPluginInfoW`
4. **Press the problematic key** — observe if any plugin export traces appear
5. **If no export traces appear** — the hang is upstream in Far3, not in the plugin
6. **Attach WinDbg** to the hung `Far.exe` process and run `~0 kb` to capture the main thread stack
7. **Check Far3 version** — try updating to the latest Far3 build; the issue may already be fixed

---

## Testing Checklist

- [ ] Start Far3 with NetBox plugin — no hang
- [ ] Press keys rapidly during plugin load — no hang (on Far3 >= 6682)
- [ ] Open a session (SFTP/FTP) — connects normally
- [ ] Open plugin menu and settings dialog — works
- [ ] Trigger a config error (e.g., corrupt `NetBox.xml`), restart Far — error dialog appears without hang
- [ ] Press keys while the error dialog is open — no deadlock
- [ ] Regression: normal plugin open/close/session connect still works