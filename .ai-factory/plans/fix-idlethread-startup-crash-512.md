# Fix: IdleThread Startup Crash (Issue #512)

> **GitHub Issue:** [#512](https://github.com/michaellukashov/Far-NetBox/issues/512)  
> **Created:** 2026-05-02  
> **Mode:** fast  
> **Settings:** Testing=no, Logging=verbose, Docs=yes

## Problem Statement

NetBox `TPluginIdleThread` starts too early during Far Manager plugin loading and can crash with `EXCEPTION_ACCESS_VIOLATION` + `EXCEPTION_EXECUTE_FAULT`.

### Crash Scenario

1. Far Manager calls `GetGlobalInfoW` to query plugin information.
2. `GetGlobalInfoW` calls `CreatePlugin()` → `CreateFarPlugin()` → `Initialize()`.
3. `Initialize()` starts the background `TPluginIdleThread`.
4. Far Manager may unload the plugin immediately after `GetGlobalInfoW` returns (e.g., version mismatch, duplicate GUID already loaded).
5. If the thread managed to start before unload, its code is suddenly yanked — the thread executes garbage memory and crashes.

Reference: [Far Manager bug #4097](https://bugs.farmanager.com/view.php?id=4097)

## Root Cause

- `TCustomFarPlugin::Initialize()` was called from `CreateFarPlugin()`, which is invoked inside `GetGlobalInfoW`.
- `Initialize()` immediately created and started `TPluginIdleThread`.
- Far Manager does not guarantee the plugin will survive past `GetGlobalInfoW`; `SetStartupInfoW` is the first call that confirms the plugin is truly loaded.

## Solution

Move `TPluginIdleThread` startup from `Initialize()` to `SetStartupInfo()`.

- `Initialize()` — keep empty (or minimal setup only, no threads).
- `SetStartupInfo()` — start the idle thread after Far Manager has committed to loading the plugin.

## Code Changes

### File: `src/NetBox/FarPlugin.cpp`

#### `TCustomFarPlugin::Initialize()` (line ~1999)

```cpp
void TCustomFarPlugin::Initialize()
{
//  ::SetGlobals(new TGlobalFunctions());
  // Idle thread initialization moved to SetStartupInfo to avoid early start
}
```

#### `TCustomFarPlugin::SetStartupInfo()` (line ~220)

Add at the end of the function, after the `try/catch` block:

```cpp
  // Start idle thread now that plugin is fully loaded
  if (!FTIdleThread)
  {
    FTIdleThread = std::make_unique<TPluginIdleThread>(this, 400);
    FTIdleThread->InitIdleThread("NetBox IdleThread");
  }
```

## Call Chain (Fixed)

```
GetGlobalInfoW
  └── CreatePlugin()
        └── CreateFarPlugin(HInstanceDLL)
              └── Initialize()          ← NO thread start here
SetStartupInfoW
  └── FarPlugin->SetStartupInfo()
        └── SetStartupInfo(Info)
              └── Start TPluginIdleThread ← Thread starts ONLY here
```

## Verification

- [x] Build passes with zero warnings (MSVC W4, x64 RelWithDebugInfo).
- [x] Commit: `5fb8a46e0` — `fix(netbox): start idle thread in SetStartupInfo to prevent early crash`.

## Risks

- **None.** This is a safe relocation of existing code to a later, guaranteed-safe call point.
- The idle thread is still created with the same parameters (`400` ms interval, same `TPluginIdleThread` class).
- No behavior change — thread simply starts later in the Far Manager plugin lifecycle.

## Related

- `TPluginIdleThread` class definition in `src/NetBox/FarPlugin.cpp` (lines 21–120).
- Thread uses `pthread_cond_timedwait` + `pthread_mutex` for idle-event signaling.
- Thread activation is controlled via `SetActivation(true/false)` in `OpenPlugin` / `ClosePlugin`.
