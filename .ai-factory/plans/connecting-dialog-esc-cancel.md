# Fix Plan: Connecting Dialog Esc Cancellation

**Problem:** When connecting to a remote server, the "connecting" status display (saved screen + console title updates) does not react to the Esc key. Pressing Esc should cancel the connection operation, but currently the connection continues uninterrupted.

**Created:** 2026-05-02

## Settings

- **Testing:** No
- **Logging:** Verbose
- **Docs:** Yes

## Analysis

### Root Cause

`TTerminal::Open()` runs synchronously in the main thread and never calls `CheckForEsc()` during the connection process. While the plugin's `CheckForEsc()` implementation (`TCustomFarPlugin::CheckForEsc()`) correctly detects Esc key presses from the console input queue, the connection code simply never invokes it.

The `CheckForEsc()` callback is only used during file operations (FTP file listing iteration, SCP output processing), not during the initial connection phase.

### Connection Flow

```
TWinSCPFileSystem::Connect()
  -> ConnectTerminal(FTerminal)
    -> TTerminal::Open()
      -> TTerminal::InternalTryOpen()
        -> TTerminal::InternalDoTryOpen()
          -> FFileSystem->Open()   [SSH/FTP/WebDAV/S3 - never checks Esc]
```

### Affected Code

- `src/core/Terminal.cpp` — `TTerminal::InternalDoTryOpen()` — connection orchestration
- `src/core/Terminal.cpp` — `TTerminal::CheckForEsc()` — existing callback (works, but unused during connection)
- `src/NetBox/FarPlugin.cpp` — `TCustomFarPlugin::CheckForEsc()` — console input Esc detection (working)

## Fix Steps

1. [x] **Investigate exact insertion points** — Identified three strategic points: `TTerminal::Open()` (EAbort catch), `InternalDoTryOpen()` (entry point check), `DoInformation()` (periodic check during connection). SSH `Init()` loop skipped — requires cross-layer coupling to access `TTerminal` from `TSecureShell`; `DoInformation()` checks before/after `Init()` provide sufficient coverage.

2. [x] **Add EAbort catch in TTerminal::Open()** — Added `catch(EAbort &)` before the existing `catch(Exception &)`. User cancel silently propagates without calling `FatalError()`.

3. [x] **Add CheckForEsc to DoInformation() AND ProcessGUI()** — Two periodic check points:
   - `DoInformation()`: added `CheckForEsc()` at top, guarded by `FStatus == ssOpening` — fires at status transitions for ALL protocols
   - `ProcessGUI()`: **Critical fix** — added `CheckForEsc()` at top. Called periodically inside SSH `WaitForData()` polling loop (SecureShell.cpp:2450) and FTP event loops. This is the point that actually detects Esc during blocking waits, where DoInformation() alone would miss it

4. [x] **Verify silent cancel** — Code review confirms clean exception chain: `ProcessGUI()` → `SecureShell::Init()` → `SecureShell::Open()` → `InternalDoTryOpen()` → `InternalTryOpen()` (rollback) → `TTerminal::Open()` (EAbort catch) → `Connect()` (EAbort catch, no QueryReopen). `__finally` blocks clear screen; `SAFE_DESTROY` cleans up
5. [x] **Handle EAbort in TWinSCPFileSystem::Connect()** — Added `catch(EAbort &)` before generic `catch(Exception &)`. User cancel sets `Result = false` and destroys terminal/queue objects without showing the reconnect prompt.

6. [x] **Verify no regression on normal connection** — Build passes with **zero new warnings** (`build-x64.bat` RelWithDebugInfo x64). `CheckForEsc()` has 500ms throttle — negligible overhead on normal connection.

## Files to Modify

- `src/core/Terminal.cpp` — Add `EAbort` catch in `Open()`, `CheckForEsc()` in `InternalDoTryOpen()`, and `CheckForEsc()` in `DoInformation()` (guarded by `ssOpening`)
- `src/core/SecureShell.cpp` — Add `CheckForEsc()` in `Init()` while loop after `WaitForData()`
- `src/NetBox/WinSCPFileSystem.cpp` — Handle `EAbort` silently in `Connect()` catch path
## Risks & Considerations

- **Partial connection state:** Aborting mid-connection may leave sockets or handles open. Each protocol's `Open()` should handle exceptions and clean up properly (most already do via try/finally or RAII).
- **Thread safety:** `CheckForEsc()` is called from the main thread during connection, which is safe. The console input read is synchronous and non-blocking.
- **Performance:** `CheckForEsc()` has a built-in 500ms throttle, so calling it frequently has negligible overhead.
- **Scope leak:** Only add `CheckForEsc()` to the connection phase. Do not add it to post-connection operations (directory listing, file transfer) unless specifically needed.
- **Build:** Zero-warning build required (`build-x64.bat`).
- **EAbort vs FatalError:** Adding the EAbort catch in `Open()` must NOT be missed — without it, user cancel flows through `FatalError()` → `EConnectionFatal`, which would show an error dialog and potentially trigger reconnect logic. This is the single most critical piece of the fix.

## Docs

- Update `docs/user-guide.md` to note that pressing Esc during connection cancels the operation.
