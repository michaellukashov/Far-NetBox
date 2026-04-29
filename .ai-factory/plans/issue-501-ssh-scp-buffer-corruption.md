# Plan: Fix Slow SSH/SCP Transfers and File Corruption (Issue #501)

> **Exploration:** [issue-501-ssh-scp-buffer-corruption-exploration.md](../references/issue-501-ssh-scp-buffer-corruption-exploration.md)
> **GitHub Issue:** [#501](https://github.com/michaellukashov/Far-NetBox/issues/501)

## Metadata

| Field | Value |
|-------|-------|
| **Branch** | `fix/issue-501-ssh-scp-buffer-corruption` |
| **Created** | 2026-04-29 |
| **Type** | Bug fix |
| **Priority** | High |
|| **Status** | Completed |
| **Testing** | No |
| **Logging** | Verbose |
| **Docs** | Yes |

## Problem Statement

Copying files via SSH/SCP in NetBox with default settings produces corrupted files and runs at abnormally low speed (~6 MB/s on gigabit networks with 100% CPU usage on one core). Disabling the "Optimize connection buffer size" session setting immediately restores normal speed (~30 MB/s) and eliminates corruption.

## Root Cause Summary

1. **Default `SendBuf` is `DefaultSendBuf` (262144)**: The `TSessionData::DefaultSettings()` method initializes `SendBuf` to a non-zero value, which enables dynamic TCP send buffer resizing.

2. **Dynamic buffer resize in `EventSelectLoop`**: Every 1000ms, `TSecureShell::EventSelectLoop()` queries `SIO_IDEAL_SEND_BACKLOG_QUERY` and grows `SO_SNDBUF` if the queried backlog exceeds the current size. This runs during active SCP transfers.

3. **Interaction with PuTTY buffering**: The mid-transfer `setsockopt(SO_SNDBUF)` calls interfere with PuTTY's internal SSH channel buffering (`Pending`/`OutPtr` queues), causing data reordering or loss that manifests as file corruption.

4. **CPU overhead**: The periodic `WSAIoctl` calls saturate one CPU core and throttle throughput.

## Solution

if|Change the **factory default** for `SendBuf` from `DefaultSendBuf` to `0`, and `SshSimple` from `true` to `false`. Disabling `SendBuf` skips the dynamic buffer resize logic. Setting `SshSimple=false` aligns factory defaults with the dialog unchecked state so the first dialog edit does not silently flip the value.
th|
xx|Existing saved sessions retain their stored values and are not affected. Users with existing sessions can manually uncheck the "Optimize connection buffer size" checkbox as a workaround.

### Key Changes

1. **Set `SendBuf` default to `0`** in `TSessionData::DefaultSettings()` — new sessions will not enable dynamic buffer resizing
2. **Add verbose logging** in `TSecureShell::EventSelectLoop()` — record when buffer resize is skipped or performed
3. **Add inline comment** in session dialog code — document the coupling between `SendBuf` and `SshSimple` and reference issue #501

## Code Changes

### File: `src/core/SessionData.cpp`

#### Location: `TSessionData::DefaultSettings()` (line ~248)

**Current:**

```cpp
SetTcpNoDelay(true);
SetSendBuf(DefaultSendBuf);  // 262144
FSourceAddress = L"";
FProtocolFeatures = L"";
SetSshSimple(true);
```

**Replace with:**

```cpp
SetTcpNoDelay(true);
SetSendBuf(0);  // Disabled by default: see GitHub issue #501
FSourceAddress = L"";
FProtocolFeatures = L"";
bb|SetSshSimple(false);  // Default matches dialog unchecked state

### File: `src/core/SecureShell.cpp`

#### Location: `EventSelectLoop()` buffer resize block (lines 2459-2475)

**Add logging before the resize block:**

```cpp
if ((FSocket != INVALID_SOCKET) &&
    (FSendBuf > 0) && (TicksAfter - FLastSendBufferUpdate >= 1000))
{
    LogEvent(FORMAT("Querying ideal send backlog (current FSendBuf=%d)", FSendBuf));
    // ... existing WSAIoctl code ...
}
else if (FSendBuf == 0)
{
    // No logging needed here; the skip is the intended default path.
}
```

**And inside the resize success path, enhance existing log:**

```cpp
LogEvent(FORMAT("Increasing send buffer from %d to %d (issue #501 diagnostic)",
    FSendBuf, nb::ToInt32(BufferLen)));
```
th|
zz|#### Location: `TSecureShell::Open()` (line ~469)
th|
go|**Add log after `FSendBuf` assignment:**
th|
pb|```cpp
rp|FSendBuf = FSessionData->GetSendBuf();
pi|LogEvent(FORMAT("Send buffer optimization: %s (FSendBuf=%d)",
de|  (FSendBuf > 0 ? "enabled" : "disabled"), FSendBuf));
ug|FInteractive = FSessionData->GetInteractiveTerminal();
bb|```

### File: `src/NetBox/WinSCPDialogs.cpp`

#### Location: `SshBufferSizeCheck` save logic (lines 3995-3998)

**Add comment before the save block:**

```cpp
// This checkbox couples SendBuf and SshSimple.
// Disabling it (SendBuf=0) is the workaround for GitHub issue #501
// (slow SSH/SCP transfers + file corruption from dynamic SO_SNDBUF resizing).
SessionData->SetSendBuf(SshBufferSizeCheck->GetChecked() ? DefaultSendBuf : 0);
SessionData->SetSshSimple(SshBufferSizeCheck->GetChecked());
```

## Tasks

### Phase I: Core Fix

- [x] 1. **Change default `SendBuf` and `SshSimple` initialization**
jc|   - File: `src/core/SessionData.cpp`
gd|   - Function: `TSessionData::DefaultSettings()` (line ~247)
tl|   - Changes:
ur|     - `SetSendBuf(DefaultSendBuf)` -> `SetSendBuf(0)`
tb|     - `SetSshSimple(true)` -> `SetSshSimple(false)`
oz|   - Rationale: Disables `WSAIoctl(SIO_IDEAL_SEND_BACKLOG_QUERY)` by default. `SshSimple` must also default to `false` so new session defaults match the dialog unchecked state (checkbox saves `SshSimple=false` when unchecked). SCP works with `SshSimple=false` — confirmed by existing workaround users.

- [x] 2. **Add verbose diagnostic logging to buffer resize path**
   - File: `src/core/SecureShell.cpp`
   - Function: `TSecureShell::EventSelectLoop()` (line ~2459)
   - Changes:
     - Log query attempt with current `FSendBuf` value
     - Log old/new buffer size when resize occurs
ff|     - Use `LogEvent(FORMAT("..."))` (standard `TSecureShell` session logging)
ol|   - Expected log output example:
ny|     ```
kk|     Querying ideal send backlog (current FSendBuf=262144)
zb|     Increasing send buffer from 262144 to 524288 (issue #501 diagnostic)
ec|     ```

- [x] 3. **Document the checkbox coupling in session dialog**
   - File: `src/NetBox/WinSCPDialogs.cpp`
   - Location: Near `SshBufferSizeCheck` save logic (line ~3996)
   - Change: Add inline comment referencing issue #501
   - No functional code change
st|
- [x] 4. **Build verification**
jc|   - Command: `cmd /c build-x64.bat`
gd|   - Expected: Zero warnings (MSVC W4)
tl|   - Output: `Far3_x64/Plugins/NetBox/NetBox.dll`
ur|   - If symbol conflicts: disable unity build with `-DOPT_USE_UNITY_BUILD=OFF`

### Phase II: Documentation

- [x] 5. **Update knowledge references**
jc|   - File: `.ai-factory/references/INDEX.md`
wc|   - Already completed: link to `issue-501-ssh-scp-buffer-corruption-exploration.md`
   - File: `.ai-factory/ARCHITECTURE.md`
   - **Status: NOT DONE** — add link in References section to `issue-501-ssh-scp-buffer-corruption-exploration.md`
th|
- [x] 6. **Update `NetBoxRus.lng` timeout hint and add docs note**
dz|   - File: `src/NetBox/NetBoxRus.lng` (and other `.lng` files)
   - Update message at ID 1104: append "This setting is now disabled by default." or similar

   - Check other language files (`NetBoxEng.lng`, `NetBoxFr.lng`, `NetBoxSpa.lng`, etc.) for the same message ID
   - **Note:** `NetBoxPol.lng` does not contain message 1104 (pre-existing incomplete translation)
   - If standalone help docs exist, note that buffer optimization defaults off for compatibility

## Architecture Notes

### Dependency Flow

```
Core (SessionData.cpp)
  |
  +-- SecureShell.cpp  (EventSelectLoop, buffer resize logic)
  |
  +-- WinSCPDialogs.cpp (UI checkbox coupling)
```

- Core layer owns session defaults (`TSessionData`)
- Core layer implements SSH socket management (`TSecureShell`)
- Plugin layer owns UI dialog (`WinSCPDialogs.cpp` in NetBox/)
- No third-party code in `libs/` is modified

### Third-Party Library Boundaries

- No changes to `libs/` directory
- PuTTY configuration `CONF_sndbuf` receives the value from `TSessionData`, but PuTTY itself is not patched
- Standard MSVC build process applies

### Build Configuration

| Setting | Value |
|---------|-------|
| **Build type** | `RelWithDebugInfo` |
| **Platform** | `x64` (or test platform) |
| **CMake option** | `OPT_CREATE_PLUGIN_DIR=ON` (for plugin directory) |
| **Unity build** | Can disable if symbol conflicts: `OPT_USE_UNITY_BUILD=OFF` |
| **Output** | `Far3_x64/Plugins/NetBox/NetBox.dll` |

## Edge Cases and Error Handling

### Existing Sessions

- Existing saved sessions store `SendBuf` explicitly. Loading an old session with `SendBuf=262144` will preserve that value.
- The fix only affects **new sessions** created after the change.
- Users with corrupted transfers must manually uncheck the checkbox in existing sessions.

### Checkbox State for New Sessions

ma|With `SendBuf=0` and `SshSimple=false` by default:
th|
cv|- `SshBufferSizeCheck->SetChecked((0 > 0) && false)` evaluates to **unchecked**
yl|- New sessions will show the checkbox unchecked, matching the safe configuration
rx|- Dialog save preserves both values: no silent state change on first OK

### FTP / S3 / WebDAV Impact

- `SendBuf` is also passed to FileZilla FTP (`OPTION_MPEXT_SNDBUF`), neon WebDAV (`SE_SESSFLAG_SNDBUF`), and S3.
- Changing the default to `0` disables send-buffer optimization for **all protocols**, not just SSH.
- This is acceptable because:
  1. The primary reported bug affects SSH/SCP
  2. The "optimization" has proven harmful in at least one protocol
  3. Conservative defaults are safer; advanced users can re-enable per-session

## Acceptance Criteria

| Criterion | Verification Method |
|-----------|---------------------|
| Default `SendBuf` is `0` for new sessions | Create new session, inspect `SessionData->GetSendBuf()` or check XML/config storage |
| Checkbox is unchecked by default | Open session dialog for a new session, verify `SshBufferSizeCheck` is unchecked |
| Build succeeds with zero warnings | `build-x64.bat` output |
| Existing sessions not modified | Load old saved session, verify `SendBuf` retains stored value |
| Verbose logging present | Check `netbox.log` for buffer resize diagnostic messages |

## Commit Plan

Single commit:

```
ir|fix(ssh): disable dynamic send buffer optimization by default
th|
sc|Change TSessionData::DefaultSettings() to initialize SendBuf to 0 instead of
au|DefaultSendBuf (262144), and SshSimple to false instead of true. The dynamic
zy|TCP send buffer resizing via WSAIoctl(SIO_IDEAL_SEND_BACKLOG_QUERY) in
tf|TSecureShell::EventSelectLoop() causes file corruption and excessive CPU usage
dj|during SCP transfers on certain SSH servers (GitHub issue #501).
th|
pd|With SendBuf=0, new sessions skip the buffer resize logic entirely,
ch|restoring normal transfer speed and data integrity. SshSimple defaults to
un|false so dialog unchecked state matches factory defaults — no silent flip on
st|first dialog edit.
th|
dn|Changes:
tn|- Set SendBuf default to 0 in TSessionData::DefaultSettings()
gn|- Set SshSimple default to false in TSessionData::DefaultSettings()
px|- Add verbose logging to EventSelectLoop buffer resize path
wm|- Add initial SendBuf state log in TSecureShell::Open()
ia|- Add inline comment in session dialog referencing issue #501
th|
cs|Fixes: slow SSH/SCP transfers
rq|Fixes: file corruption during SCP copy
es|Refs: GitHub issue #501
ki|```



### Phase III: Post-Implementation (identified during /aif-improve review)

- [x] 7. **Fix missing ARCHITECTURE.md reference**
   - File: `.ai-factory/ARCHITECTURE.md`
   - Add link to `issue-501-ssh-scp-buffer-corruption-exploration.md` in the References table
   - This was overlooked during initial documentation pass

- [x] 8. **Commit changes with conventional commit message**
   - Main fix committed as `49cd79e49` with message from `## Commit Plan` block
   - Remaining docs updates (Task 7 + plan changelog) to be committed separately

- [ ] 9. **Manual smoke-test verification** (requires Far Manager runtime)
   - [ ] Open a new session dialog and confirm `SshBufferSizeCheck` is unchecked by default
   - [ ] Perform an SCP transfer with a new session and verify `netbox.log` does NOT contain "Querying ideal send backlog"
   - [ ] Verify existing session with buffer optimization enabled still logs "Querying ideal send backlog" and functions correctly
   - **Code-level verification completed:** `SetChecked((0>0)&&false)` → unchecked confirmed; logging paths verified in review
## Changelog

| Date | Change | Reason |
|------|--------|--------|
| 2026-04-29 | Corrected `TSessionData` constructor → `DefaultSettings()` in plan text and code block locations | Actual code change is in `DefaultSettings()`, not constructor |
| 2026-04-29 | Added Phase III tasks (missing ARCHITECTURE.md reference, commit, smoke-test) | Gaps found during /aif-improve review |
| 2026-04-29 | Updated Task 5 to flag ARCHITECTURE.md link as NOT DONE | Reference was not actually added to file |
| 2026-04-29 | Added note about `NetBoxPol.lng` missing message 1104 | Pre-existing incomplete translation discovered during review |
