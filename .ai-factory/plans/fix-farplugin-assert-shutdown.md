# Fix: DebugAssert(FarPlugin != nullptr) crash in TFarEnvGuard during idle thread shutdown

**Type:** fix
**Date:** 2026-04-25
**Mode:** fast

## Settings
- **Testing:** No (Far Manager plugin, manual testing only)
- **Logging:** Standard
- **Docs:** No (code-only fix)

---

## Root Cause Analysis

`TPluginIdleThread::Execute()` (FarPlugin.cpp:53) calls `FPlugin->FarAdvControl(ACTL_SYNCHRO, 0, nullptr)` on a background thread. `FarAdvControl()` creates `TFarEnvGuard Guard` which asserts `DebugAssert(FarPlugin != nullptr)`.

During shutdown, `DestroyFarPlugin()` calls `SAFE_DESTROY(Plugin)` which:
1. Saves `PObj = Plugin`
2. Sets `Plugin = nullptr` (i.e., `FarPlugin = nullptr`)
3. Calls `std::destroy_at(PObj)`

The idle thread is still potentially mid-execution when step 2 runs. When the destructor destroys `FTIdleThread`, the thread's `Terminate()` + `WaitFor()` must join it — but the thread may be inside `FarAdvControl()` → `TFarEnvGuard` → `DebugAssert(FarPlugin != nullptr)` **after** `FarPlugin` was already set to `nullptr` in step 2.

## Fix

In `DestroyFarPlugin()` (WinSCPPlugin.cpp:24-29), replace `SAFE_DESTROY(Plugin)` with explicit `delete` + null assignment in the correct order — destroy first, then set null:

```cpp
void DestroyFarPlugin(TCustomFarPlugin *& Plugin)
{
  DebugAssert(FarPlugin);
  Plugin->Finalize();
  delete Plugin;    // destructor joins idle thread; FarPlugin still valid
  Plugin = nullptr; // now safe to clear
}
```

This ensures `FarPlugin` remains valid throughout the entire destruction sequence, including when the idle thread's final `FarAdvControl` call runs inside `TFarEnvGuard`.

## Dependency Flow

Plugin Layer (`NetBox/` FarPlugin.cpp `FarAdvControl` → `TFarEnvGuard`) → Base Layer (`Base/` Common.h `SAFE_DESTROY` macro) — the fix avoids modifying the macro (used widely) and instead fixes the ordering at the single call site in `DestroyFarPlugin`.

## Tasks

### task-1: Fix DestroyFarPlugin shutdown ordering
- **File:** `src/NetBox/WinSCPPlugin.cpp`
- **Change:** In `DestroyFarPlugin()` (line ~24-29), replace `SAFE_DESTROY(Plugin)` with:
  ```cpp
  delete Plugin;
  Plugin = nullptr;
  ```
- **Why:** `SAFE_DESTROY` sets the pointer to `nullptr` before calling the destructor, but the destructor joins the idle thread which calls `FarAdvControl()` → `TFarEnvGuard` → asserts `FarPlugin != nullptr`. Delaying the null assignment until after destruction ensures the global remains valid during the join.
- **Edge cases:** `SAFE_DESTROY` is used in many other places — do NOT modify the macro. This fix is scoped to `DestroyFarPlugin` only. The `Plugin` reference parameter means setting `Plugin = nullptr` after `delete` correctly clears the `FarPlugin` global.

## Build Verification
```
cmd /c build-x64.bat
```
Expected: clean build, zero warnings. Plugin DLL in `Far3_x64/Plugins/NetBox/NetBox.dll`.

## Result

✅ Build passed. 2 pre-existing C4552 warnings in `WinConfiguration.h` (unrelated). Fix applied and verified.
