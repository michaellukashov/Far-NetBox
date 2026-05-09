# Far3 Plugin Startup Deadlock Fix

> **Bug:** Far Manager 3 hangs when a key is pressed during NetBox plugin startup.
> **Fixed:** 2026-05-12
> **Component:** Plugin initialization (`NetBox.cpp`, `FarPlugin.cpp`, `WinSCPPlugin.cpp`)

---

## Symptom

When Far Manager 3 loads the NetBox plugin (during startup or when opening the plugin menu for the first time), pressing any key causes the entire Far3 process to hang indefinitely. The process must be killed from Task Manager.

## Root Cause

All Far plugin exports (`SetStartupInfoW`, `GetPluginInfoW`, `ProcessPanelInputW`, etc.) are protected by `TFarPluginGuard`, which acquires a **global plugin-level critical section** (`TCustomFarPlugin::FCriticalSection`) for the entire duration of the export call.

During initialization:

1. `GetPluginInfoW` calls `CoreInitializeOnce()`
2. `CoreInitializeOnce()` loads sessions, initializes crypto/PuTTY/FileZilla/neon
3. If any step throws an exception, the catch block in `TCustomFarPlugin::GetPluginInfo` calls `HandleException()`
4. `HandleException()` opens a **modal dialog** (`Message()` / `ShowExtendedException()`)
5. The dialog runs a message loop that pumps keyboard input
6. If the user presses a key that Far routes to `ProcessPanelInputW`, that export tries to acquire the **same global lock**
7. **Deadlock:** `ProcessPanelInputW` waits for the lock; the main thread is stuck in the dialog loop holding the lock

```
Main thread (holds global lock):
  GetPluginInfoW
    CoreInitializeOnce
      catch → HandleException
        Message() / ShowExtendedException()
          DialogRun() → pumps messages
            [user presses key]
              ProcessPanelInputW tries to acquire global lock → BLOCKS
```

## Why Keys Specifically

Keyboard events are the most aggressive input source:
- They are buffered by Windows and rapidly generated
- Far's console input reader (`ReadConsoleInput`) produces `INPUT_RECORD`s
- Unlike mouse clicks (which have a clear target window), keyboard events may be routed through the panel → plugin chain even when a dialog is active, especially for global shortcuts
- If the dialog is a plugin dialog (`TFarDialog`), it runs inside Far's `DialogRun`, which may not fully suppress panel-level key processing for certain keys

## Fix

Apply `TUnguard` (temporary lock release) in `HandleException` before showing any modal dialog.

### Code Changes

#### `src/NetBox/FarPlugin.cpp` — `TCustomFarPlugin::HandleException`

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

#### `src/NetBox/WinSCPPlugin.cpp` — `TWinSCPPlugin::HandleException`

```cpp
void TWinSCPPlugin::HandleException(Exception * E, OPERATION_MODES OpMode)
{
  if (((OpMode & OPM_FIND) == 0) || nb::isa<EFatal>(E))
  {
    // Release the global plugin lock before showing a modal dialog.
    // Far may dispatch keyboard events to plugin exports while the dialog
    // message loop runs; holding the lock would cause a deadlock.
    TUnguard Unguard(GetCriticalSection());
    ShowExtendedException(E);
  }
}
```

#### `src/base/Global.h` / `Global.cpp` — `TUnguard` accepts `const` reference

Changed `TUnguard` constructor to accept `const TCriticalSection &` (matching `TGuard`), since `TCriticalSection::Enter()` and `Leave()` are `const` methods using `mutable` internal state.

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

## Debugging This Pattern

If you suspect a similar deadlock:

1. **Attach WinDbg** to the hung `Far.exe` process
2. **List all thread stacks:**
   ```
   ~* kb
   ```
3. **List locked critical sections:**
   ```
   !cs -l
   ```
4. **Identify the owner thread** of the contested critical section
5. **Check if the owner thread** is inside a dialog/message loop (`DialogRun`, `Message`, etc.)
6. **Check if any waiting thread** is inside a plugin export (`ProcessPanelInputW`, etc.)

If the owner is in a dialog and the waiter is in an export, you have an init-phase deadlock.

## Testing Checklist

- [ ] Start Far3 with NetBox plugin — no hang
- [ ] Press keys rapidly during plugin load — no hang
- [ ] Open a session (SFTP/FTP) — connects normally
- [ ] Open plugin menu and settings dialog — works
- [ ] Trigger a config error (e.g., corrupt `NetBox.xml`), restart Far — error dialog appears without hang
- [ ] Press keys while the error dialog is open — no deadlock
- [ ] Regression: normal plugin open/close/session connect still works
