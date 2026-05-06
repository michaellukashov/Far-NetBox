# Plan: WinSCP Feature Alignment Phase 1

**Mode:** fast
**Created:** 2026-05-05
**Updated:** 2026-05-05 (improved via /aif-improve)
**Source:** `.ai-factory/plans/winscp-feature-alignment-roadmap.md` Phase 1

---

## Goal

Complete dialog UX alignment with WinSCP patterns and reconcile Terminal/SecureShell code differences. Previously blocking crash bugs (#497, #391, #392, #508) are already fixed — this phase is low-risk UX polish + proactive maintenance.

## Research Context

From RESEARCH.md Active Summary:
- Next step: resume WinSCP feature alignment roadmap Phase 1 (dialog UX refinements + upstream bug fix porting)
- Constraints: C++17 only, no std::filesystem, no libs/ modifications, Far API main-thread only, MSVC /W4 zero warnings
- Success signals: zero warnings, plugin DLL in Far3_<platform>/Plugins/NetBox/, manual test protocol passes

## Key Discovery (from /aif-improve audit)

NetBox is **ahead** of WinSCP in Terminal.cpp and SecureShell.cpp safety fixes (retry counter, InfiniteWait guard, null-handle guard, MAXIMUM_WAIT_OBJECTS guard, exception-safe FWaiting counter). The original plan framing of "port upstream fixes" was inaccurate — very few genuine upstream fixes are missing. Task 1 has been reframed accordingly.

---

## Tasks

### Task 1: Audit and reconcile Terminal/SecureShell differences ✅
- Compare WinSCP 6.5.6 `core/Terminal.cpp` and `core/SecureShell.cpp` against NetBox equivalents
- Backport any missing WinSCP fixes found in the comparison
- Document NetBox-specific safety enhancements already present (for upstream contribution awareness)
- **Specific item:** Resolve `CONF_tcp_keepalives` default — NetBox sets `true` (with TODO), WinSCP sets `0` (disabled). Align with WinSCP or document rationale
- **Specific item:** Review `CONF_nopty` handling — NetBox makes it configurable via `GetInteractiveTerminal()`, WinSCP always disables. Verify this is intentional
- Add `FTerminal->LogEvent(L"Terminal/SecureShell reconciliation: <description>")` per change
- **Files:** `src/core/Terminal.cpp`, `src/core/SecureShell.cpp`
- **Blocked by:** none

### Task 2: Fix missing SetEnabled() calls in UpdateControls() ✅
- Audit all controls in `TSessionDialog::UpdateControls()` for missing `SetEnabled()` calls based on protocol selection
- **Known gaps to fix:**
  - `CompressionCheck` — never disabled when SSH tab is inactive
  - `AgentFwdCheck` — no explicit enablement gating
  - Any other controls that should be enabled/disabled per protocol but aren't
- Verify all `SetVisible(false)` controls on S3/TLS tabs are correctly handled (audit confirmed they are — no action needed)
- **Note:** No MakeOwnedObject leaks found — that item is removed from scope
- Add logging: `FTerminal->LogEvent(L"Dialog: control <name> enabled=%d")` for changed enablement logic
- **MsgIDs/.lng:** Add any new message IDs needed for control labels or tooltips; keep MsgIDs.h and all .lng files structurally aligned
- **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`, `src/base/MsgIDs.h`, `src/resource/*.lng`
- **Blocked by:** none

### Task 3a: Copy parameter preset infrastructure ✅
- Add `TCopyParamPreset` class with name + TCopyParamType data (in CopyParam.h/cpp)
- Add preset storage to `TFarConfiguration` under `TransferPresets` FarSettings key
  - Storage: Count + DefaultPreset + numbered subkeys (Name + CopyParamType fields)
  - Load/Save via existing `TCopyParamType::Load/Save` under numbered subkeys
- Add 4 shipped default presets: "Default", "Text files (ASCII)", "Binary all", "No preserve time"
- **Scope:** Manual preset selection only — no autoselection rules (WinSCP's rule engine deferred to later phase)
- **MsgIDs/.lng:** ~15 new NB_TRANSFER_PRESET_* IDs + corresponding .lng entries; keep MsgIDs.h and all .lng files structurally aligned
- Add `FTerminal->LogEvent(L"Copy param preset loaded: %s", PresetName)` 
- **Files:** `src/core/CopyParam.h`, `src/core/CopyParam.cpp`, `src/NetBox/FarConfiguration.h`, `src/NetBox/FarConfiguration.cpp`, `src/base/MsgIDs.h`, `src/resource/*.lng`
- **Blocked by:** none

### Task 3b: Copy parameter preset UI ✅
- Add `TFarComboBox` for preset selection to `TCopyDialog` (between CopyParamLister and TransferSettings button)
- On preset change: load FCopyParams from preset storage via `TCopyParamType::Load()`
- Add preset management dialog (minimal: listbox + Add/Edit/Delete buttons)
- Wire preset combo into `TFullSynchronizeDialog` and `TSynchronizeDialog` (same pattern)
- **Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/WinSCPDialogs.h`
- **Blocked by:** Task 2 (UpdateControls enablement must be correct before adding new controls) and Task 3a (infrastructure must exist first)

### Task 4: Build verification & regression test ✅
- Run `build-x64.bat` → zero warnings
- Verify plugin DLL in `Far3_x64/Plugins/NetBox/`
- Manual test: connect SFTP, FTP, WebDAV, S3; transfer file; cancel operation; navigate directories
- Test preset: select preset in copy dialog, verify transfer settings applied correctly
- **Blocked by:** Tasks 1, 2, 3a, 3b

## Dependency Graph

```
Task 1 ──────────────┐
Task 2 ─────┬────────┤
Task 3a ────┼────────┤
            └→ Task 3b ──→ Task 4
```

Tasks 1, 2, 3a are independent and can be done in parallel.
Task 3b depends on both Task 2 and Task 3a.
Task 4 depends on all previous tasks.

## Commit Checkpoints

**Checkpoint 1** (after Tasks 1, 2):
```
fix(ui): reconcile Terminal/SecureShell with WinSCP, fix UpdateControls enablement

- Resolve CONF_tcp_keepalives default alignment
- Add SetEnabled() for CompressionCheck, AgentFwdCheck per protocol
- Document NetBox-specific safety enhancements
```

**Checkpoint 2** (after Tasks 3a, 3b):
```
feat(transfer): add copy parameter presets with UI

- Add TCopyParamPreset storage in FarConfiguration
- Add preset combo to TCopyDialog and sync dialogs
- Ship 4 default presets (Default, Text, Binary, NoPreserve)
```

## Notes

- No architectural changes — all modifications are additive/refactoring within existing class boundaries
- No libs/ modifications required
- All Far Manager API calls must remain on main thread (verify in WinSCPDialogs.cpp changes)
- MsgIDs.h + .lng files must stay structurally aligned — GetMsg crashes if they diverge
- Preset autoselection rules (hostname/username matching) deferred to WinSCP alignment Phase 2
- If Terminal/SecureShell audit yields no new fixes to port, Task 1 becomes documentation-only
