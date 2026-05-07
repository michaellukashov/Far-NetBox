# Implementation Plan: Phase 2 Verification Fixes

Branch: fix/phase2-verification-fixes
Created: 2026-05-10

## Settings

- Testing: no (skip tests)
- Logging: verbose (detailed DEBUG logs)
- Docs: yes (mandatory docs checkpoint)
- Roadmap Linkage: none

## Research Context

Source: .ai-factory/RESEARCH.md (Active Summary)

Goal: Achieve structural UX parity with WinSCP — all dialogs that exist in WinSCP must exist in NetBox

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- WinXP compatibility (_WIN32_WINNT=0x0501)
- Incremental evolution — no architectural rewrites
- C++17 standard only (no std::filesystem, std::variant)

Decisions:
- SCP Esc cancellation: Four-layer fix adopted (console buffer scan, csCancel semantics, EAbort("") dispatch, deferred reconnect). Post-cancel session reset via FNeedsSessionReset + FLastDirectory.
- SSH/SCP buffer corruption: Default SendBuf=0, SshSimple=false to disable dynamic TCP send backlog query.
- Multithreading: Far API marshaling via Synchronize/PostMainThreadSynchro; event-driven waits replace Sleep polling.
- CWE-134: EscapeFmtChars() utility for all untrusted args to FMTLOAD.
- OpenSSH certificate auth (WinSCP-aligned): Silent pre-connect conversion inside StoreToConfig(). Effective key file resolved before CONF_keyfile. Passphrase encryption uses effective key file path. Temp PPK cleaned in TSecureShell destructor. No interactive dialogs (Far plugin context).
- Master password infrastructure: Effectively complete. All security-critical gaps (TSecureString, atomic counters, rate limiting, error reporting) are implemented. Active-terminal recryption gap is non-issue in Far plugin context.
- UX parity scope: "Structural parity" only — same dialogs must exist as in WinSCP. Behavioral parity (keyboard shortcuts, menu structure, workflow sequencing) is out of scope.

Open questions:
- Silent mode file operations: Error collection mechanism designed but not implemented.

Success signals:
- Zero build warnings under /W4
- Plugin DLL in Far3_<platform>/Plugins/NetBox/
- `python scripts/verify_lng_alignment.py` passes
- All CRLF line endings, UTF-8 without BOM, no trailing whitespace

## Tasks

### Phase 1: Defensive Fixes

- [ ] **Task 1: Add null checks in TGenerateUrlDialog**
  - Files: `src/NetBox/WinSCPDialogs.cpp`
  - Add null guards at entry of `GenerateScript()`, `UpdateScriptResult()`, `UpdateUrlResult()`
  - Guard `FScriptFormatCombo`, `FTransferModeBtn`, `FScriptResultEdit`, `FUrlResultEdit`, `FSessionData`
  - LOG: `AppLogFmt(L"GenerateUrl: null guard triggered in %s", __FUNCTION__)`
  - Acceptance: No crashes if MakeOwnedObject fails; build zero warnings

- [ ] **Task 2: Build verification**
  - Run `cmd /c build-x64.bat`
  - Verify zero warnings under /W4
  - Acceptance: Build passes with zero warnings

### Phase 2: Localization Fixes

- [ ] **Task 3: Add MsgIDs for Location Profiles InputBox prompts**
  - Files: `src/base/MsgIDs.h`
  - Add `NB_LOCATION_BOOKMARK_NAME_PROMPT` and `NB_LOCATION_BOOKMARK_RENAME_PROMPT` after current last ordinal
  - LOG: `AppLogFmt(L"MsgIDs: added NB_LOCATION_BOOKMARK_NAME_PROMPT ordinal=%d")`
  - Acceptance: Ordinals are sequential, no conflicts

- [ ] **Task 4: Replace hardcoded English with GetMsg()**
  - Files: `src/NetBox/WinSCPDialogs.cpp`
  - Line 2687: Replace `L"Bookmark name:"` with `GetMsg(NB_LOCATION_BOOKMARK_NAME_PROMPT)`
  - Line 2731: Replace `L"New name:"` with `GetMsg(NB_LOCATION_BOOKMARK_RENAME_PROMPT)`
  - LOG: `AppLogFmt(L"LocationProfiles: replaced hardcoded InputBox prompt")`
  - Acceptance: No hardcoded English strings remain in Location Profiles dialog

- [ ] **Task 5: Update all 5 .lng files**
  - Files: `src/NetBox/NetBox{Eng,Rus,Fr,Pol,Spa}.lng`
  - Append 2 entries to each file at the correct ordinal position
  - NetBoxEng.lng: `"Bookmark name:"` and `"New name:"`
  - NetBoxRus.lng: `"Имя закладки:"` and `"Новое имя:"`
  - NetBoxFr.lng: `"Nom du signet:"` and `"Nouveau nom:"`
  - NetBoxPol.lng: `"Nazwa zakładki:"` and `"Nowa nazwa:"`
  - NetBoxSpa.lng: `"Nombre del marcador:"` and `"Nuevo nombre:"`
  - LOG: `AppLogFmt(L"Lang: added 2 entries to %s", Language)`
  - Acceptance: All 5 .lng files have entries at correct ordinals

- [ ] **Task 6: Run alignment verification**
  - Run `python scripts/verify_lng_alignment.py`
  - Acceptance: Exit code 0 — all files aligned

### Phase 3: UX Fixes

- [ ] **Task 7: Add confirmation dialog before removing bookmark**
  - Files: `src/NetBox/WinSCPDialogs.cpp` (line 2707)
  - Insert `MessageDialog()` check before `Bookmarks->Delete()` in `RemoveBookmarkClick()`
  - Use `qtConfirmation` query type with `qaOK | qaCancel` answer flags
  - LOG: `AppLogFmt(L"LocationProfiles: remove confirmation shown for '%s'", Name)`
  - Acceptance: Removing bookmark shows confirmation; cancel prevents deletion

- [ ] **Task 8: Add MsgID for remove confirmation**
  - Files: `src/base/MsgIDs.h`
  - Add `NB_LOCATION_PROFILES_REMOVE_CONFIRM` after current last ordinal
  - LOG: `AppLogFmt(L"MsgIDs: added NB_LOCATION_PROFILES_REMOVE_CONFIRM ordinal=%d")`
  - Acceptance: Ordinal is sequential, no conflicts

- [ ] **Task 9: Update all 5 .lng files for remove confirmation**
  - Files: `src/NetBox/NetBox{Eng,Rus,Fr,Pol,Spa}.lng`
  - Append 1 entry to each file at the correct ordinal position
  - NetBoxEng.lng: `"Do you wish to remove bookmark '%s'"`
  - NetBoxRus.lng: `"Удалить закладку '%s'"`
  - NetBoxFr.lng: `"Voulez-vous supprimer le signet '%s'"`
  - NetBoxPol.lng: `"Czy chcesz usunąć zakładkę '%s'"`
  - NetBoxSpa.lng: `"¿Desea eliminar el marcador '%s'"`
  - LOG: `AppLogFmt(L"Lang: added remove confirmation to %s", Language)`
  - Acceptance: All 5 .lng files have entries at correct ordinals

### Phase 4: Navigation Fix

- [ ] **Task 10: Add OpenBookmarkClick navigation members to TLocationProfilesDialog**
  - Files: `src/NetBox/WinSCPDialogs.h` (or `.cpp` if class defined there)
  - Add public getters: `GetOpened()`, `GetOpenedLocalDir()`, `GetOpenedRemoteDir()`
  - Add private members: `FOpenedLocalDir`, `FOpenedRemoteDir`, `FOpened`
  - LOG: `AppLogFmt(L"LocationProfiles: added navigation members")`
  - Acceptance: Members compile without warnings

- [ ] **Task 11: Update OpenBookmarkClick to store selected paths**
  - Files: `src/NetBox/WinSCPDialogs.cpp` (line 2747)
  - Replace body to store `FOpenedLocalDir`, `FOpenedRemoteDir`, `FOpened=true` before `Close(OkButton)`
  - LOG: `AppLogFmt(L"LocationProfiles: storing bookmark paths for navigation")`
  - Acceptance: OpenBookmarkClick stores paths and closes dialog

- [ ] **Task 12: Update LocationProfilesDialog wrapper to navigate after Execute()**
  - Files: `src/NetBox/WinSCPDialogs.cpp` (line 2777)
  - After `Execute()` returns true, check `GetOpened()` and navigate using `FTerminal->ChangeDirectory()` + `UpdatePanel()` + `RedrawPanel()`
  - LOG: `AppLogFmt(L"LocationProfiles: navigating to bookmark remote=%s", RemoteDir)`
  - Acceptance: Clicking Open navigates remote panel to bookmarked remote dir

- [ ] **Task 13: Verify navigation works**
  - Manual test: Open Location Profiles dialog, select a bookmark, click Open
  - Verify remote panel navigates to bookmarked remote directory
  - Verify local panel follows (if SynchronizeBrowsing enabled)
  - Acceptance: Navigation works as expected

## Commit Plan

- **Commit 1** (after tasks 1-2): "fix(defensive): add null guards in TGenerateUrlDialog"
- **Commit 2** (after tasks 3-6): "fix(i18n): localize Location Profiles InputBox prompts"
- **Commit 3** (after tasks 7-9): "fix(ux): add confirmation before removing Location Profiles bookmark"
- **Commit 4** (after tasks 10-13): "fix(ui): Location Profiles Open button navigates to bookmarked directories"

## Success Criteria

| Issue | Verification |
|-------|-------------|
| 1 (Navigation) | Open button navigates remote panel to bookmarked remote dir |
| 2 (Localization) | `grep -r "Bookmark name:" src/` returns no hits; all 5 .lng files have new entries |
| 3 (Remove confirm) | Removing bookmark shows confirmation; cancel prevents deletion |
| 4 (Null checks) | Build with zero warnings under /W4 |
| All | `python scripts/verify_lng_alignment.py` passes |
| All | CRLF line endings, UTF-8 without BOM, no trailing whitespace |

## Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `FTerminal->ChangeDirectory()` API differs from expected | Low | Medium | Check WinSCPFileSystem.h for correct API |
| Null checks mask real initialization bugs | Low | Low | Keep null checks as defensive only, log warning |
| New MsgIDs shift existing ordinals | Low | High | Verify ordinals are appended at end, not inserted |
