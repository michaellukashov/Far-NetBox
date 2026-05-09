# Deep Analysis: Far3 Plugin Startup Key-Press Hang

> Generated: 2026-05-12
> Scope: NetBox Far Manager 3 plugin initialization deadlock investigation

---

## 1. Far3 Plugin Lifecycle & Initialization Sequence

### 1.1 Export Entry Points (src/NetBox/NetBox.cpp)

Every exported WINAPI function is wrapped by `TFarPluginGuard`, which acquires the plugin-level `TCriticalSection` for the entire duration of the call:

```
DllMain(DLL_PROCESS_ATTACH)          → minimal, no Far API use
  ↓
GetGlobalInfoW()                     → returns plugin metadata (GUID, version, etc.)
  ↓
SetStartupInfoW(const PluginStartupInfo * Info)
  → TFarPluginGuard lock acquired
  → TCustomFarPlugin::SetStartupInfo()
     → Stores Far API function pointers
     → Creates and starts TPluginIdleThread (400 ms interval)
     → TFarPluginGuard lock released
  ↓
GetPluginInfoW()
  → TFarPluginGuard lock acquired
  → TWinSCPPlugin::GetPluginInfo()
     → Calls CoreInitializeOnce() (lazy-init on first use)
        → CryptographyInitialize()
        → PuttyInitialize()  ← InitializeCriticalSection(&putty_section)
        → TFileZillaIntf::Initialize()
        → NeonInitialize()
        → CoreLoad()         ← may throw; catch block calls ShowExtendedException()
  → TFarPluginGuard lock released
  ↓
OpenW(int OpenFrom, intptr_t Item)  → called when user presses F11 / activates plugin
  → Creates TWinSCPFileSystem, may auto-connect and show dialogs
```

**Key insight:** `SetStartupInfoW` and `GetPluginInfoW` both hold the **same global plugin critical section** for their entire execution. If either blocks or enters a modal loop while holding the lock, any concurrent plugin export (including keyboard input via `ProcessPanelInputW`) will deadlock trying to acquire the same lock.

### 1.2 The Idle Thread (src/NetBox/FarPlugin.cpp)

`TPluginIdleThread` is created inside `SetStartupInfoW`. It sleeps on a `pthread_cond_t` and wakes every 400 ms to call `PostMainThreadSynchro()`:

```cpp
void TCustomFarPlugin::PostMainThreadSynchro(TThreadMethod Event)
{
  FSynchroParams.SynchroEvent = Event;   // sets callback
  FSynchroParams.Sender = this;
  FarAdvControl(ACTL_SYNCHRO, 0, &FSynchroParams);  // schedules callback
}
```

The idle thread **never** calls Far API directly — it only schedules a synchronous callback via `ACTL_SYNCHRO`. This is the correct pattern and is not itself a deadlock source.

### 1.3 Critical-Section Hierarchy

| Lock | Scope | Held During Init? | Held During Dialog? |
|------|-------|-------------------|---------------------|
| `TCustomFarPlugin::FCriticalSection` | Global plugin lock | **Yes** (all exports) | **Yes** (TFarPluginGuard spans entire export) |
| `TWinSCPFileSystem::FCriticalSection` | Per-filesystem lock | No | Yes (ProcessKeyEx, etc.) |
| `putty_section` | PuTTY global lock | Yes (inside GetPluginInfoW → PuttyInitialize) | Yes (runtime SSH ops) |
| Dialog-level semaphores | Per-dialog | No | Yes (TFarDialog::Synchronize) |

---

## 2. Root-Cause Scenarios

### Scenario A: Exception During Init → Modal Dialog While Holding Global Lock

**Path:**
1. `GetPluginInfoW` acquires global lock.
2. `CoreInitializeOnce` → `CoreLoad` throws (corrupt session registry, missing config, etc.).
3. Catch handler calls `ShowExtendedException` → creates a **modal Far dialog** (`TFarDialog::ShowModal` → `DialogInit` + `DialogRun`).
4. `DialogRun` enters a **message loop** inside `GetPluginInfoW`, which still holds the global lock.
5. User presses any key. Far dispatches `ProcessPanelInputW` to the plugin.
6. `ProcessPanelInputW` tries to acquire the same global lock.
7. **Deadlock:** `ProcessPanelInputW` waits for the lock; main thread is stuck in the dialog loop holding the lock.

**Why keys specifically?** The dialog message loop (`DialogRun`) explicitly pumps keyboard and mouse messages. A mouse click inside the dialog is consumed by the dialog itself. A key press, however, may be dispatched by Far to the plugin's panel input handler (`ProcessPanelInputW`) if the dialog does not consume it, or if the key is a global shortcut (e.g., F11 for plugins menu, F1 for help). Far's dialog engine sometimes routes unhandled keys back to the underlying panel, which then calls `ProcessPanelInputW`.

**Evidence:** `src/core/CoreMain.cpp:195,238,257,293` — catch blocks inside `CoreLoad` / `CoreFinalize` call `ShowExtendedException`, which opens a modal dialog.

### Scenario B: TPluginIdleThread Synchro Event Fires During Init

**Path:**
1. `SetStartupInfoW` starts `TPluginIdleThread`.
2. Thread wakes (400 ms) and calls `ACTL_SYNCHRO`.
3. Far schedules `ProcessSynchroEventW` with `FE_IDLE`.
4. `ProcessSynchroEventW` tries to acquire the global lock.
5. If main thread is still inside `SetStartupInfoW` or `GetPluginInfoW` (e.g., slow crypto init), it holds the lock.
6. `ProcessSynchroEventW` blocks waiting for the lock.
7. If Far dispatches this synchro event on the **same thread** as the init call (which it does — Far is single-threaded), we have a re-entrant deadlock: the thread tries to acquire a lock it already holds.

**Note:** Windows critical sections are **recursive** by default (same thread can re-enter). So this specific path is unlikely to deadlock unless the synchro event is dispatched on a different thread or the lock is implemented as a non-recursive mutex. NetBox uses `InitializeCriticalSection`, which **is** recursive. So this path is low probability.

### Scenario C: Background Thread Started During Init Calls Back Into Plugin

**Path:**
1. During init, a background thread is started (e.g., by `CoreInitializeOnce` or by `TPluginIdleThread`).
2. The background thread calls a function that acquires `putty_section` or another lock.
3. The main thread also needs `putty_section`.
4. Classic AB-BA deadlock if ordering is wrong.

**Evidence:** `PuttyInitialize` creates `putty_section`. If a background thread calls a PuTTY function during init while the main thread holds the global plugin lock and then tries to enter `putty_section`, and the background thread tries to enter the global plugin lock, deadlock ensues.

### Scenario D: OpenW Called During Init (Auto-Open or Far Startup Panel)

**Path:**
1. Far restores a panel that was left open from a previous session.
2. Far calls `OpenW` immediately after `SetStartupInfoW`.
3. `OpenW` creates `TWinSCPFileSystem` and auto-connects.
4. Connection attempt shows a login/progress dialog (`TFarDialog::ShowModal`).
5. Dialog runs message loop. User presses a key.
6. Key is dispatched to `ProcessPanelInputW` (or dialog idle thread tries to synchro).
7. Both need the global plugin lock. Deadlock.

---

## 3. Why Keys Specifically Trigger the Hang

| Event Type | Why It May / May Not Deadlock |
|------------|------------------------------|
| **Keyboard** | Far's input dispatcher calls `ProcessPanelInputW` **synchronously** on the main thread. This export acquires the global plugin lock. If the main thread is already inside a modal dialog (which is also on the main thread) that was opened while holding the same lock, the re-entrant lock acquisition succeeds (recursive CS). However, if the dialog's message loop dispatches the key to a **different** plugin export (e.g., `ProcessPanelEventW`), or if Far internally tries to acquire a non-recursive lock, deadlock occurs. |
| **Mouse** | Usually consumed directly by the dialog window. If the click is outside the dialog, Far may route it to the panel, but this is less common during startup. |
| **Timer / Idle** | `TPluginIdleThread` uses `ACTL_SYNCHRO`, which Far queues. If Far processes the synchro event while the main thread is in a dialog loop, it depends on Far's internal re-entrancy model. |
| **Window resize** | Usually handled by Far's frame window, not routed to plugins. |

The key insight: **Keyboard events are the most aggressive input source** because:
1. They are generated rapidly and buffered by Windows.
2. Far's console input reader (`ReadConsoleInput`) produces `INPUT_RECORD`s that Far must dispatch.
3. Unlike mouse clicks (which have a clear target window), keyboard events are often routed through the panel → plugin chain even when a dialog is technically active, especially for global shortcuts.
4. If the dialog is a plugin dialog (`TFarDialog`), it runs inside Far's `DialogRun`, which may not fully suppress panel-level key processing for certain keys.

---

## 4. Reproduction Strategy

### 4.1 Minimal Plugin Skeleton

Create a minimal Far3 plugin that replicates the NetBox init pattern:

```cpp
// MinimalHangPlugin.cpp
#include <windows.h>
#include <plugin.hpp>

static CRITICAL_SECTION g_cs;

static void ShowModalDuringInit()
{
  // Simulate a dialog or long blocking operation
  // In real scenario: MessageBox, DialogRun, or WaitForSingleObject
  for (int i = 0; i < 100; ++i) {
    Sleep(100);  // simulate slow init
    // pump messages manually to let keys through
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

extern "C" void WINAPI SetStartupInfoW(const PluginStartupInfo * Info)
{
  InitializeCriticalSection(&g_cs);
  EnterCriticalSection(&g_cs);   // simulate TFarPluginGuard
  ShowModalDuringInit();         // simulate init that pumps messages
  LeaveCriticalSection(&g_cs);
}

extern "C" int WINAPI GetPluginInfoW(PluginInfo * Info)
{
  EnterCriticalSection(&g_cs);
  // ...
  LeaveCriticalSection(&g_cs);
  return 0;
}

extern "C" int WINAPI ProcessPanelInputW(const ProcessPanelInputInfo * Info)
{
  EnterCriticalSection(&g_cs);   // will deadlock if main thread holds g_cs
  // ...
  LeaveCriticalSection(&g_cs);
  return 0;
}
```

Compile as a Far3 plugin, load it, and press any key during the 10-second init window. The process should hang.

### 4.2 Reproducing in NetBox Specifically

1. **Trigger an init error:** Corrupt `NetBox.xml` session file, or force OpenSSL init to fail by setting `OPENSSL_CONF` to a bad path.
2. **Add artificial delay:** Insert `Sleep(5000)` inside `CoreInitializeOnce`.
3. **Press keys** during the delay.
4. **Observe hang.**

---

## 5. Debugging Approach

### 5.1 Attaching WinDbg / x64dbg

1. Start Far3 with NetBox plugin.
2. Before pressing the problematic key, attach WinDbg to the `Far.exe` process.
3. Press the key that causes the hang.
4. Break into the debugger (`Ctrl+Break` or Debug → Break).

### 5.2 Inspecting All Thread Stacks

```
~* kb
```

Look for:
- **Main thread (thread 0):** Should be inside `DialogRun`, `WaitForSingleObject`, `Sleep`, or `EnterCriticalSection`.
- **Other threads:** Look for any thread also inside `EnterCriticalSection` or `RtlEnterCriticalSection`.

### 5.3 Identifying Lock Owner

For a critical section deadlock:

```
!cs -l          // list all locked critical sections with owner thread
!locks          // comprehensive lock analysis
```

Or manually:
```
!cs <address_of_g_cs>
```

This shows:
- `OwningThread` — the thread ID that holds the lock.
- `LockCount` — how many times it's been entered.
- `RecursionCount` — re-entrancy count.

### 5.4 If the Hang Is Not a CS Deadlock

If `!cs` shows no contention, the hang may be:
- **Message-loop starvation:** The dialog loop is not pumping messages, or a `WaitForSingleObject(INFINITE)` is blocking the main thread.
- **Far internal deadlock:** Far's own critical section (not the plugin's) is held.
- **x64dbg approach:** Use the "Threads" tab → right-click each thread → "Go to Disassembly" to see where they are stuck.

### 5.5 Adding Debug Logging

Instrument the critical-section enter/exit points in `NetBox.cpp`:

```cpp
// Around TFarPluginGuard usage:
#define DEBUG_CS_TRACE 1
#ifdef DEBUG_CS_TRACE
  #define TRACE_CS_ENTER() OutputDebugStringA("[NetBox] CS ENTER " __FUNCTION__ "\n")
  #define TRACE_CS_LEAVE() OutputDebugStringA("[NetBox] CS LEAVE " __FUNCTION__ "\n")
#else
  #define TRACE_CS_ENTER()
  #define TRACE_CS_LEAVE()
#endif
```

Use DebugView (Sysinternals) to watch the sequence. If you see `ENTER GetPluginInfoW` followed by `ENTER ProcessPanelInputW` with no `LEAVE GetPluginInfoW`, that's the deadlock.

---

## 6. Concrete Fixes

### Fix A: Release Global Lock Before Modal Dialogs

**Problem:** `TFarPluginGuard` wraps the entire export function.

**Solution:** Restructure exports so that the lock is released before any potentially blocking or modal operation:

```cpp
// Before (current):
void WINAPI SetStartupInfoW(const PluginStartupInfo * Info)
{
  TFarPluginGuard Guard(CreateFarPlugin());  // lock held for entire function
  FarPlugin->SetStartupInfo(Info);
}

// After (fixed):
void WINAPI SetStartupInfoW(const PluginStartupInfo * Info)
{
  {
    TFarPluginGuard Guard(CreateFarPlugin());
    FarPlugin->SetStartupInfo(Info);  // stores info, starts idle thread
    // lock released here before anything that might block
  }
  // any modal or blocking work happens outside the lock
}
```

However, this is tricky because `SetStartupInfo` itself should not block. The real problem is inside `GetPluginInfoW` when `CoreInitializeOnce` opens error dialogs.

### Fix B: Deferred Initialization

Defer heavy or error-prone initialization until `OpenW` (when the user actively opens the plugin), rather than doing it in `GetPluginInfoW`:

```cpp
// In TWinSCPPlugin::GetPluginInfoW
void TWinSCPPlugin::GetPluginInfoW(PluginInfo * Info)
{
  // Do NOT call CoreInitializeOnce here.
  // Just return static menu items.
  Info->MenuItems = ...;
}

// In TWinSCPPlugin::OpenW
void * TWinSCPPlugin::OpenW(int OpenFrom, intptr_t Item)
{
  CoreInitializeOnce();  // now it's safe — user opened plugin intentionally
  // ... create file system, connect, etc.
}
```

**Trade-off:** The plugin menu may show items that require init, but the init can be done lazily on first open.

### Fix C: Guard Dialog Creation with Lock Release

Inside `CoreInitializeOnce` or any init function that might show a dialog, explicitly release the plugin lock:

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
      // Release global lock before showing error dialog
      // (requires passing lock state or using a re-entrant safe dialog wrapper)
      {
        TCriticalSection * PluginCS = GetCriticalSection();
        TGuard Unguard(PluginCS, false);  // temporarily release
        ShowExtendedException(&E);          // modal dialog — may pump messages
        // lock re-acquired automatically when Unguard goes out of scope
      }
    }
    FCoreInitialized = true;
  }
}
```

**Note:** This requires `TGuard` to support explicit `Leave`/`Enter` or a scoped unguard RAII pattern. NetBox's `TGuard` is simple RAII (enters in ctor, leaves in dtor). A `TUnguard` class that leaves in ctor and re-enters in dtor would work:

```cpp
class TUnguard
{
  TCriticalSection * FSection;
public:
  explicit TUnguard(TCriticalSection * Section) : FSection(Section)
  {
    FSection->Leave();
  }
  ~TUnguard()
  {
    FSection->Enter();
  }
};
```

### Fix D: PostMessage-Style Deferred Execution for Dialogs

Instead of showing a dialog synchronously during init, post a deferred task:

```cpp
void TWinSCPPlugin::GetPluginInfoW(PluginInfo * Info)
{
  // Return info immediately
  Info->MenuItems = ...;

  // If init failed earlier, schedule error display for later
  if (FInitErrorPending)
  {
    PostMainThreadSynchro(nb::bind(&TWinSCPPlugin::ShowPendingInitError, this));
    FInitErrorPending = false;
  }
}
```

This ensures no dialog runs inside `GetPluginInfoW`, so the global lock is never held during a message loop.

### Fix E: Use a Separate Init-Phase Lock

Instead of one global plugin lock, use a lighter lock for init-only state:

```cpp
class TCustomFarPlugin
{
  TCriticalSection FInitLock;     // protects init state only
  TCriticalSection FRuntimeLock;  // protects runtime state (file systems, dialogs)
};

// Exports that only read init state acquire FInitLock (short duration).
// Exports that interact with runtime state acquire FRuntimeLock.
// Modal dialogs never hold FRuntimeLock.
```

This is a larger architectural change but eliminates the root cause entirely.

---

## 7. Priority Ranking of Fixes

| Fix | Effort | Risk | Effectiveness |
|-----|--------|------|---------------|
| **C** — Unguard around dialog in catch blocks | Low | Low | High (fixes the specific exception→dialog path) |
| **B** — Defer CoreInitializeOnce to OpenW | Low | Medium | High (eliminates init-time dialogs entirely) |
| **D** — PostMessage-style deferred error display | Medium | Low | High (cleanest, but requires synchro plumbing) |
| **A** — Restructure TFarPluginGuard scope | Medium | High | Medium (may break assumptions in other exports) |
| **E** — Separate init/runtime locks | High | High | Very High (architectural, long-term correct) |

---

## 8. Additional Observations

- `TPluginIdleThread` uses `pthread_cond_t` via an emulation layer. If this layer has a bug (e.g., lost wakeups), the idle thread could be in a bad state. However, the hang is described as triggered by a key press, not spontaneous, so this is unlikely to be the root cause.
- `TFarDialog::Synchronize` uses a semaphore + event pair. If a dialog is shown during init and its `Synchronize` is called from a background thread while the main thread is in `DialogRun`, the background thread blocks on `WaitForMultipleObjects(INFINITE)` until the main thread calls `TFarDialog::SynchronizeEvent`. If the main thread never reaches that point (e.g., stuck in a nested message loop), the background thread waits forever. But this is a secondary symptom, not the primary cause.
- The `putty_section` critical section is created in `PuttyInitialize` and used throughout PuTTY code. If PuTTY is initialized on a background thread (it shouldn't be), `putty_section` could be uninitialized when accessed. However, `CoreInitializeOnce` is called on the main thread inside `GetPluginInfoW`.
