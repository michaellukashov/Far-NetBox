# Implementation Plan: WinSCP Structural UX Parity

**Branch:** lmv/dev (staying on current branch; main is used by worktree)
**Created:** 2026-05-07
**Plan Type:** full
**Scope:** Implement 4 missing dialogs + 1 missing button for complete structural UX parity with WinSCP

---

## Settings

- **Testing:** yes — manual test protocol per task + build verification
- **Logging:** verbose — detailed DEBUG logs for all new features
- **Docs:** yes — mandatory docs checkpoint at completion (MsgIDs.h, .lng files, .hlf help)
- **Roadmap Linkage:** Milestone: Phase 2 — Productivity Features (Generate URL, KeyGen, Cleanup, FS Info)

---

## Roadmap Linkage

**Milestone:** Phase 2 — Productivity Features
**Rationale:** This plan replaces the existing Phase 2 in `winscp-feature-alignment-roadmap.md` with corrected scope. FileSystemInfo already exists (was Task 2.4), LocationProfiles and CopyParamCustom were missing. This plan delivers the corrected Phase 2 task set.

---

## Research Context

**Source:** `.ai-factory/RESEARCH.md` — Active Summary (2026-05-07 session)

Goal: Achieve structural UX parity with WinSCP — all dialogs that exist in WinSCP must exist in NetBox. Structural parity only (same dialogs); behavioral parity (shortcuts, menu structure, workflow) is out of scope.

**Identified gaps (complete list):**

| # | Dialog | Est. Lines | Complexity | Key Risk |
|---|--------|-----------|------------|----------|
| 1 | TGenerateUrlDialog | 250-300 | Medium | Script gen needs exe-name adjustment; Assembly tab omitted |
| 2 | TCleanupDialog | 150-180 | Low | Simplest; all backend exists |
| 3 | TLocationProfilesDialog | 200-250 | Medium | Data model exists; UI is new but well-patterned |
| 4 | CopyParamCustom | 100-150 | Low | Editor exists; just wiring |
| 5 | "Generate Key" button | 30-50 | Low | Plumbing exists; just add button |
| **Total** | | **730-930** | | |

---

## Architecture

**Dependency flow:** Plugin Layer (`src/NetBox/`) → Core Layer (`src/core/`) → Base Layer (`src/base/`)

New dialogs go in `src/NetBox/WinSCPDialogs.cpp` using the established pattern:
- Class extends `TWinSCPDialog` or `TFarDialog`
- Controls created via `MakeOwnedObject<TFarX>(Dialog)`
- `bool Execute()` method with `ShowModal()` → read control values
- Tabs via `TTabbedDialog::AddTab()` + `SetDefaultGroup(TabID)`

**Third-party library boundaries:** No modifications to `libs/`. All new code is in the Plugin Layer, calling existing Core Layer APIs.

**Build considerations:** Adding ~730-930 lines to `WinSCPDialogs.cpp` may trigger unity build symbol conflicts. If so, disable `OPT_USE_UNITY_BUILD=OFF`.

---

## Tasks

### Phase A: "Generate Key" Button (Simplest — Warm-up)

**Goal:** Add "Generate Key" button on Authentication tab to launch puttygen.exe.

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng` (+ RUS, POL, FR, SPA)

- [ ] **Task A1: Add "Generate Key" button to Authentication tab**
  - Add `NB_SESSION_GENERATE_KEY` message ID to `MsgIDs.h`
  - Add localized strings to all 5 `.lng` files
  - In TSessionDialog constructor (Auth tab, ~L3697 after PrivateKeyBrowseBtn):
    - Create `TFarButton * GenerateKeyBtn = MakeOwnedObject<TFarButton>(Dialog)`
    - Caption: `GetMsg(NB_SESSION_GENERATE_KEY)`
    - Position: `ipRight` of DisplayPublicKeyBtn
    - OnClick handler: `GenerateKeyButtonClick`
  - New method `TSessionDialog::GenerateKeyButtonClick()`:
    - Read `GetFarConfiguration()->GetPuttygenPath()`
    - If empty or file not found: show error message, return
    - Call `::ExecuteShellChecked(Program, Params)` (same pattern as `WinSCPPlugin.cpp:714-720`)
  - LOG: `FTerminal->LogEvent(L"KeyGen: launched puttygen.exe, path=%s", PuttygenPath)`
  - Blocked by: none

- [ ] **Task A2: Enable button only when PuttygenPath is valid**
  - In `TSessionDialog::UpdateControls()` (Auth section, ~L4028):
    - `GenerateKeyBtn->SetEnabled(aSshProtocol && !PuttygenPath.IsEmpty() && base::FileExists(ExpandEnvVars(ExtractProgram(PuttygenPath))))`
  - LOG: `FTerminal->LogEvent(L"Dialog: GenerateKeyBtn enabled=%d", Enabled)`
  - Blocked by: Task A1

- [ ] **Task A3: Build verification**
  - `build-x64.bat` with zero warnings
  - Manual test: open Session dialog → Auth tab → verify "Generate Key" button appears, is enabled when PuttygenPath is valid, launches puttygen.exe on click
  - Blocked by: Task A2

### Phase B: TCleanupDialog

**Goal:** Add Cleanup dialog to clean orphaned config, sessions, caches, INI, seed, temp folders.

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng` (+ RUS, POL, FR, SPA), `src/NetBox/WinSCPPlugin.cpp`

- [ ] **Task B1: Add TCleanupDialog class and message IDs**
  - Add message IDs to `MsgIDs.h`:
    - `NB_CLEANUP_TITLE`, `NB_CLEANUP_CONFIG`, `NB_CLEANUP_SESSIONS`, `NB_CLEANUP_CACHES`, `NB_CLEANUP_INIFILE`, `NB_CLEANUP_SEEDFILE`, `NB_CLEANUP_TEMP_FOLDERS`, `NB_CLEANUP_CHECK_ALL`, `NB_CLEANUP_SELECT_ALL`, `NB_CLEANUP_DESELECT_ALL`
  - Add localized strings to all 5 `.lng` files
  - Add `TCleanupDialog` class in `WinSCPDialogs.cpp`:
    - Extends `TWinSCPDialog`
    - Constructor: detect available cleanup categories using existing backend methods:
      - `Configuration->RegistryPathExists(Configuration->ConfigurationSubKey)` → add config entry
      - `Configuration->RegistryPathExists(Configuration->StoredSessionsSubKey)` → add sessions entry
      - `Configuration->HasAnyCache()` → add caches entry
      - INI file exists → add INI entry
      - `RandomSeedExists()` → add seed entry
      - `WinConfiguration->AnyTemporaryFolders()` → add temp entry
    - UI: `TFarListBox` with checkmark items (one per detected category)
    - "Select All" / "Deselect All" buttons (or Ins/Space toggle)
    - OK/Cancel standard buttons
  - LOG: `FTerminal->LogEvent(L"Cleanup: detected %d categories", Count)`
  - Blocked by: none

- [ ] **Task B2: Implement cleanup execution logic**
  - `TCleanupDialog::Execute()`:
    - On OK: iterate checked items, call corresponding cleanup callback:
      - `Configuration->CleanupConfiguration()`
      - `StoredSessions->Cleanup()` (via `TStoredSessionList::Cleanup()`)
      - `Configuration->CleanupCaches()`
      - `Configuration->CleanupIniFile()`
      - `Configuration->CleanupRandomSeedFile()`
      - `WinConfiguration->CleanupTemporaryFolders()`
    - Catch exceptions per-item (same pattern as WinSCP `Cleanup.cpp:161-168`)
    - Show error message if any cleanup fails, but continue with remaining items
  - LOG: `FTerminal->LogEvent(L"Cleanup: executing category '%s'", CategoryName)`
  - LOG: `FTerminal->LogEvent(L"Cleanup: completed %d of %d categories", Completed, Total)`
  - Blocked by: Task B1

- [ ] **Task B3: Add "Cleanup" menu item to plugin commands**
  - In `TWinSCPPlugin::CommandsMenu()` (~L618):
    - Add `NB_MENU_COMMANDS_CLEANUP` item after Configure
    - Wire to call `TCleanupDialog::Execute()` via new method
  - In `TWinSCPPlugin::PluginMenuExecute()`:
    - Handle new menu item → create and execute TCleanupDialog
  - Blocked by: Task B2

- [ ] **Task B4: Build verification & manual test**
  - `build-x64.bat` zero warnings
  - Manual test: F11 → NetBox → Cleanup → verify dialog shows detected categories, toggle checkboxes, click OK, verify items cleaned
  - Blocked by: Task B3

### Phase C: TGenerateUrlDialog

**Goal:** Generate session URL and script code (batch, PowerShell, command-line). Omit .NET Assembly tab (NetBox has no .NET assembly).

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng` (+ RUS, POL, FR, SPA), `src/NetBox/WinSCPFileSystem.cpp`

- [ ] **Task C1: Add TGenerateUrlDialog class — URL tab**
  - Add message IDs: `NB_GENERATE_URL_TITLE`, `NB_GENERATE_URL_URL`, `NB_GENERATE_URL_SCRIPT`, `NB_GENERATE_URL_USER_CHECK`, `NB_GENERATE_URL_PASSWORD_CHECK`, `NB_GENERATE_URL_HOSTKEY_CHECK`, `NB_GENERATE_URL_REMOTE_DIR_CHECK`, `NB_GENERATE_URL_WINSCP_SPECIFIC_CHECK`, `NB_GENERATE_URL_SAVE_EXTENSION_CHECK`, `NB_GENERATE_URL_CLIPBOARD`, `NB_GENERATE_URL_RESULT`
  - Add localized strings to all 5 `.lng` files
  - `TGenerateUrlDialog` class extending `TTabbedDialog`:
    - Two tabs: URL, Script
    - URL tab:
      - Checkboxes for: username, password, hostkey, remote directory, WinSCP-specific, save extension
      - Result text area (`TFarText` with word-wrap)
      - "Copy to Clipboard" button
    - Core logic: call `TSessionData::GenerateSessionUrl()` with flags derived from checkboxes
    - `TSessionData::GenerateSessionUrl()` already exists in core — no new backend needed
  - LOG: `FTerminal->LogEvent(L"GenerateUrl: URL tab, session=%s", SessionName)`
  - Blocked by: none

- [ ] **Task C2: Add Script tab to TGenerateUrlDialog**
  - Script tab:
    - Format combo: BatchFile / CommandLine / PowerShell (omit ScriptFile — it requires a temp .script file which is less useful in Far)
    - Result text area showing generated script
    - "Copy to Clipboard" button
  - Script generation logic (port from WinSCP `GenerateUrl.cpp:292-521`):
    - Build `open` command with session args from `TSessionData::GenerateOpenCommandArgs()`
    - For transfer mode: add `put`/`get` commands with file paths
    - For session mode: add placeholder comments
    - Add `exit` command
    - Format per script type:
      - BatchFile: `@echo off`, `winscp.com` → `netbox.com`, log/ini/command params, error handling
      - CommandLine: inline params with `"command"` quoting
      - PowerShell: `$PSNativeCommandArgumentPassing = "Legacy"`, `& "netbox.com"`, result checking
    - **Key difference from WinSCP:** Use `netbox.com` not `winscp.com` as the exe name
    - Use `Application->ExeName` or `ModuleFileName()` for the actual path
  - LOG: `FTerminal->LogEvent(L"GenerateUrl: Script tab, format=%d", Format)`
  - Blocked by: Task C1

- [ ] **Task C3: Add "Generate URL" menu item and wiring**
  - Add "Generate URL" to session context menu or commands menu
  - Entry point from `TWinSCPFileSystem` when a session is active
  - Create dialog with `TSessionData*` and optional `TStrings* Paths`
  - Two modes: session-only (from session list) and transfer (from file panel with selected files)
  - For Phase 2 scope: implement session-only mode first. Transfer mode can be added later.
  - Blocked by: Task C2

- [ ] **Task C4: Build verification & manual test**
  - `build-x64.bat` zero warnings
  - Manual test: Generate URL dialog → URL tab → toggle checkboxes → verify URL updates → copy to clipboard → paste and verify. Script tab → select format → verify script text → copy
  - Blocked by: Task C3

### Phase D: TLocationProfilesDialog

**Goal:** Full CRUD editor for bookmark profiles (local+remote directory pairs) with session/shared tabs.

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng` (+ RUS, POL, FR, SPA), `src/NetBox/WinSCPFileSystem.cpp`

- [ ] **Task D1: Add TLocationProfilesDialog class with list UI**
  - Add message IDs: `NB_LOCATION_PROFILES_TITLE`, `NB_LOCATION_PROFILES_SESSION_TAB`, `NB_LOCATION_PROFILES_SHARED_TAB`, `NB_LOCATION_PROFILES_LOCAL_DIR`, `NB_LOCATION_PROFILES_REMOTE_DIR`, `NB_LOCATION_PROFILES_ADD`, `NB_LOCATION_PROFILES_REMOVE`, `NB_LOCATION_PROFILES_RENAME`
  - Add localized strings to all 5 `.lng` files
  - `TLocationProfilesDialog` class extending `TTabbedDialog`:
    - Two tabs: Session Profiles, Shared Profiles
    - Each tab: `TFarListBox` with bookmark entries (name + local/remote paths)
    - Local directory edit field
    - Remote directory edit field
    - Buttons: Add, Remove, Rename, OK, Cancel
  - Data model: reuse existing `TBookmarkList` / `TBookmarks` from `src/core/Bookmarks.cpp`
  - Session bookmarks via `GetFarConfiguration()->GetBookmarks(SessionKey)`
  - Shared bookmarks via `GetFarConfiguration()->GetBookmarks(SharedKey)`
  - LOG: `FTerminal->LogEvent(L"LocationProfiles: loaded %d session bookmarks", Count)`
  - Blocked by: none

- [ ] **Task D2: Implement CRUD operations**
  - Add: prompt for bookmark name → create `TBookmark` with current local/remote dirs → add to `TBookmarkList`
  - Remove: delete selected bookmark from list
  - Rename: prompt for new name → update bookmark name
  - On OK: save modified bookmark lists back to configuration
    - `GetFarConfiguration()->SetBookmarks(SessionKey, SessionBookmarkList)`
    - `GetFarConfiguration()->SetBookmarks(SharedKey, SharedBookmarkList)`
  - Bookmark name validation: not empty, not a number (same as WinSCP `BookmarkNameValidateName`)
  - LOG: `FTerminal->LogEvent(L"LocationProfiles: add bookmark '%s'", Name)`
  - Blocked by: Task D1

- [ ] **Task D3: Wire Location Profiles into commands menu**
  - Replace or enhance existing `OpenDirectoryDialog` entry with "Location Profiles" menu item
  - Or add as separate "Location Profiles" command alongside existing "Open Directory"
  - When user selects bookmark: set `LocalDirectory` and `RemoteDirectory`, return OK
  - Blocked by: Task D2

- [ ] **Task D4: Build verification & manual test**
  - `build-x64.bat` zero warnings
  - Manual test: Location Profiles → add bookmark → verify appears in list → rename → remove → verify session/shared tabs work → select bookmark → verify directory navigation
  - Blocked by: Task D3

### Phase E: CopyParamCustom

**Goal:** Allow per-session custom copy parameter override (instead of global presets only).

**Files:** `src/NetBox/WinSCPDialogs.cpp`, `src/core/SessionData.h`, `src/core/SessionData.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBoxEng.lng`

- [ ] **Task E1: Add "Custom" option to Copy dialog preset combo**
  - Add `NB_COPY_PRESET_CUSTOM` message ID + localized strings
  - In `TCopyDialog` preset combo, add "<Custom>" as the last option after all named presets
  - When "Custom" is selected and user clicks OK:
    - Open the existing copy-param editor (reuse `TCopyParamsFrame`-equivalent controls or open `TSessionDialog`'s Directories/Environment tab in a standalone dialog)
    - Store the custom `TCopyParamType` on the `TSessionData` for this session
  - Blocked by: none

- [ ] **Task E2: Add FCopyParamOverride field to TSessionData**
  - Add `TCopyParamType FCopyParamOverride` member to `TSessionData`
  - Add property accessors: `GetCopyParamOverride()`, `SetCopyParamOverride()`
  - Add to `SESSION_PROPERTY` macro list for persistence
  - Load/save in `TSessionData::LoadOptions()` / `SaveOptions()`
  - When `FCopyParamOverride` is not default, the copy operation uses it instead of the selected preset
  - LOG: `FTerminal->LogEvent(L"CopyParam: session '%s' using custom override", SessionName)`
  - Blocked by: Task E1

- [ ] **Task E3: Build verification & manual test**
  - `build-x64.bat` zero warnings
  - Manual test: Copy dialog → select "<Custom>" → verify editor opens → change settings → verify custom params are applied to the copy operation → verify persistence across session reload
  - Blocked by: Task E2

---

## Commit Plan

**5+ tasks → commit checkpoints every phase:**

1. **After Phase A** (Tasks A1-A3):
   ```
   feat(auth): add Generate Key button to authentication tab

   - Launch puttygen.exe from Auth tab when PuttygenPath is configured
   - Button enabled only for SSH/FTPS protocols with valid path
   ```

2. **After Phase B** (Tasks B1-B4):
   ```
   feat(ui): add configuration cleanup dialog

   - Checkbox list of cleanup categories (config, sessions, caches, INI, seed, temp)
   - All backend methods already exist in core layer
   - Menu item added to plugin commands
   ```

3. **After Phase C** (Tasks C1-C4):
   ```
   feat(ui): add Generate URL/Script dialog

   - URL tab with session URL generation (username, password, hostkey options)
   - Script tab with batch/PowerShell/command-line format output
   - Uses netbox.com executable name (not winscp.com)
   - Omit .NET Assembly tab (not applicable to NetBox)
   ```

4. **After Phase D** (Tasks D1-D4):
   ```
   feat(ui): add location profiles dialog

   - Full CRUD for session and shared bookmark profiles
   - Reuses existing TBookmarkList/TBookmarks data model
   - Session/Shared tabs with add/remove/rename operations
   ```

5. **After Phase E** (Tasks E1-E3):
   ```
   feat(copy): add per-session custom copy parameter override

   - "Custom" option in copy dialog preset combo
   - TSessionData::CopyParamOverride field for persistence
   - Custom params override selected preset for the session
   ```

---

## Dependencies

```
Phase A ──→ Phase B ──→ Phase C ──→ Phase D ──→ Phase E
  │            │            │            │            │
  └─ 3 tasks   └─ 4 tasks   └─ 4 tasks   └─ 4 tasks   └─ 3 tasks
     (warm-up)  (simplest     (medium      (medium       (low
                 dialog)       complexity)  complexity)   wiring)
```

Phases are ordered by increasing complexity. Each phase is independent enough to be tested standalone, but sequential order ensures:
- Phase A warms up with the simplest change (30-50 lines)
- Phase B builds on the dialog pattern with a simple list dialog
- Phase C adds the most complex new dialog (tabbed, code generation)
- Phase D adds another tabbed dialog with CRUD operations
- Phase E finishes with lightweight wiring

---

## Success Criteria

| Phase | Success Criteria |
|-------|-----------------|
| A | "Generate Key" button visible on Auth tab for SSH; launches puttygen.exe; disabled when path invalid |
| B | Cleanup dialog shows detected categories; checkbox toggle works; cleanup executes per-item with error handling |
| C | Generate URL produces valid session URL; Script tab produces correct batch/PS/cmd output using netbox.com |
| D | Location Profiles CRUD works; bookmarks persist across Far restart; session/shared tabs functional |
| E | "Custom" option in copy preset combo; custom params applied to copy operation; persisted in session data |

**Global:**
- [ ] All modified files use CRLF line endings
- [ ] UTF-8 without BOM encoding
- [ ] No trailing whitespace
- [ ] Naming conventions followed (T/F prefixes, PascalCase methods)
- [ ] No modifications to `libs/`
- [ ] Plugin DLL output to `Far3_x64/Plugins/NetBox/`
- [ ] Build with zero MSVC `/W4` warnings

---

## Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Unity build symbol conflicts from large WinSCPDialogs.cpp additions | Medium | Low | Disable `OPT_USE_UNITY_BUILD=OFF` if needed |
| Script generation uses wrong exe name (winscp.com vs netbox.com) | Low | Medium | Use `Application->ExeName` / `ModuleFileName()` for path; derive .com from .exe |
| MsgIDs.h / .lng file desync causes crash | Medium | Critical | Add all IDs and strings in same commit; test all 5 language files |
| CopyParamCustom serialization format change breaks existing sessions | Low | High | Use optional field with default; gracefully handle missing field on load |
| LocationProfiles dialog too complex for Far text-mode | Low | Medium | Start with flat list (no folders); add folder support later if needed |

---

## Testing Strategy

### Per-Phase Manual Test Protocol

1. **Build verification:** `build-x64.bat` → zero warnings → DLL in `Far3_x64/Plugins/NetBox/`
2. **Protocol smoke test:** Connect SFTP → FTP → WebDAV → S3 (one file each)
3. **Feature-specific test:** Exercise all new UI paths, error paths, cancel paths
4. **Regression test:** Synchronize, queue, custom command, editor, bookmarks
5. **i18n verification:** Switch Far to RU/PL language → verify new dialog strings display correctly

### Automated Testing

- URL generation validation against known-good outputs (Catch2 unit test)
- CopyParamOverride serialization round-trip test
- BookmarkList CRUD test (existing `SessionHistoryTest.cpp` pattern)

---

## Documentation Checkpoints

After all phases complete:
- [ ] Update `docs/user-guide.md` with new feature descriptions (5 new UI elements)
- [ ] Update `.hlf` help files (EN, RU, PL, FR, ES) with new menu items and dialogs
- [ ] Update `NetBoxEng.lng` / `NetBoxRus.lng` with new message strings (sync `MsgIDs.h`)
- [ ] Add CHANGELOG entry
- [ ] Update `.ai-factory/plans/winscp-feature-alignment-roadmap.md` Phase 2 to mark complete

---

## WinSCP Porting Architecture Mapping

Per skill-context rule "WinSCP porting architecture mapping", documenting for each dialog:

| NetBox Dialog | WinSCP Source | Original Bug/Feature | NetBox Equivalent | What Was Missing |
|---------------|--------------|---------------------|-------------------|-----------------|
| GenerateKeyBtn | `SiteAdvanced.cpp:1672-1688` | Tools dropdown with Generate/Upload key items | TFarButton on Auth tab | No button; puttygen only accessible via F11 menu |
| TCleanupDialog | `Cleanup.cpp:16-174` | Checkbox list of cleanup categories | TFarListBox with checkmark items | No cleanup UI; only manual registry/file deletion |
| TGenerateUrlDialog | `GenerateUrl.cpp:32-1020` | 3-tab URL/Script/Assembly generator | 2-tab URL/Script (omit Assembly) | No URL generation; no script output |
| TLocationProfilesDialog | `LocationProfiles.cpp:23-1102` | TreeView CRUD with folders + drag-drop | TFarListBox CRUD (flat, no drag-drop) | Only flat menu (OpenDirectoryDialog); no editor, no shared profiles |
| CopyParamCustom | `CopyParamCustom.cpp:17-73` | Wrapper around CopyParamsFrame | "Custom" combo option → existing editor | Only global presets; no per-session override |
