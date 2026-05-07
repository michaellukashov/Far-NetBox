# Implementation Plan: Phase 2 Verification Fixes

**Branch:** fix/phase2-verification-fixes
**Created:** 2026-05-10
**Plan Type:** fast (focused fixes from verification)
**Scope:** Fix 4 issues found during WinSCP UX Parity Phase 2 verification — **ALL DONE** (2026-05-10)

---

## Settings

- **Testing:** yes — manual test per fix
- **Logging:** verbose — DEBUG logs for all changes
- **Docs:** no — minor fixes, no new features
- **Roadmap Linkage:** None (maintenance fixes)

---

## Research Context

**Source:** Systematic verification of all 5 Phase 2 features via 5 parallel agent investigations + alignment script.

> **Verification report:** See conversation context above — all 5 features exist, are wired, and have correct alignment.

---

## Issues

| # | Issue | Severity | Files | Est. Lines |
|---|-------|----------|-------|------------|
| 1 | Location Profiles "Open" button doesn't navigate | 🔴 Functional | WinSCPDialogs.cpp, WinSCPDialogs.h | ~30 | ✅ DONE |
| 2 | Hardcoded English in Location Profiles InputBox | 🟡 Cosmetic | WinSCPDialogs.cpp, MsgIDs.h, 5×.lng | ~20 | ✅ DONE |
| 3 | No confirmation before removing bookmark | 🟡 UX | WinSCPDialogs.cpp, MsgIDs.h, 5×.lng | ~15 | ✅ DONE |
| 4 | Null check on FScriptFormatCombo | 🟢 Defensive | WinSCPDialogs.cpp | ~10 | ✅ DONE |

---

## Issue 1: Location Profiles "Open" Button Navigation

### Problem

`OpenBookmarkClick()` (WinSCPDialogs.cpp:2747) logs paths and closes the dialog but never navigates. The wrapper at line 2777 has an empty comment block. The selected bookmark is lost after `Execute()` returns.

### Design

**Pattern:** Store navigation data in dialog, expose via getters, navigate in wrapper after `Execute()`.

This follows the existing `TWinSCPFileSystem::OpenDirectory` pattern (WinSCPFileSystem.cpp:2170-2195) where:
1. Dialog stores navigation data
2. Wrapper reads via getters
3. Wrapper calls `FTerminal->ChangeDirectory()` + `UpdatePanel()` + `RedrawPanel()`

### Code Changes

#### 1. `src/NetBox/WinSCPDialogs.h` — Add members and getters

```cpp
// In TLocationProfilesDialog public section (after Execute()):
bool GetOpened() const { return FOpened; }
UnicodeString GetOpenedLocalDir() const { return FOpenedLocalDir; }
UnicodeString GetOpenedRemoteDir() const { return FOpenedRemoteDir; }

// In private section:
UnicodeString FOpenedLocalDir;
UnicodeString FOpenedRemoteDir;
bool FOpened{false};
```

#### 2. `src/NetBox/WinSCPDialogs.cpp` — `OpenBookmarkClick` (line 2747)

Replace body to store paths before closing:

```cpp
void TLocationProfilesDialog::OpenBookmarkClick(TFarButton * /*Sender*/, bool & /*Close*/)
{
  UnicodeString Name = GetCurrentBookmarkName();
  if (Name.IsEmpty()) return;
  TBookmarkList * Bookmarks = (GetTab() == tabSession) ? FSessionBookmarks : FSharedBookmarks;
  if (Bookmarks)
  {
    TBookmark * Bookmark = Bookmarks->FindByName(UnicodeString(), Name);
    if (Bookmark)
    {
      FOpenedLocalDir = Bookmark->GetLocal();
      FOpenedRemoteDir = Bookmark->GetRemote();
      FOpened = true;
      Close(OkButton);
    }
  }
}
```

#### 3. `src/NetBox/WinSCPDialogs.cpp` — Wrapper (line 2777)

Replace body to navigate after dialog closes:

```cpp
void TWinSCPPlugin::LocationProfilesDialog(TWinSCPFileSystem * FileSystem)
{
  if (!FileSystem) return;
  const UnicodeString SessionKey = FileSystem->GetSessionData()->GetSessionName();
  std::unique_ptr<TLocationProfilesDialog> Dialog(std::make_unique<TLocationProfilesDialog>(this, SessionKey));
  if (Dialog->Execute())
  {
    if (Dialog->GetOpened())
    {
      UnicodeString RemoteDir = Dialog->GetOpenedRemoteDir();
      if (!RemoteDir.IsEmpty())
      {
        FileSystem->GetTerminal()->ChangeDirectory(RemoteDir);
        if (FileSystem->UpdatePanel(true))
        {
          FileSystem->RedrawPanel();
        }
      }
      UnicodeString LocalDir = Dialog->GetOpenedLocalDir();
      if (!LocalDir.IsEmpty())
      {
        // Local panel follows via SynchronizeBrowsing if enabled
        // or can be set directly if needed
      }
    }
  }
}
```

**Note:** Verify that `GetTerminal()`, `ChangeDirectory()`, `UpdatePanel()`, and `RedrawPanel()` exist on `TWinSCPFileSystem`. If not, use the equivalent APIs found during implementation.

### MsgID / Language Changes

None — only internal navigation, no new user-visible strings.

---

## Issue 2: Hardcoded English in Location Profiles InputBox

### Problem

Two `InputBox()` calls use hardcoded English strings instead of localized `GetMsg()`:
- Line 2687: `L"Bookmark name:"`
- Line 2731: `L"New name:"`

### Design

Add 2 new MsgIDs and update all 5 `.lng` files.

### Code Changes

#### 1. `src/base/MsgIDs.h` — Add 2 new entries

After `NB_COPY_PRESET_CUSTOM` (ordinal 1439) or at end of enum:

```cpp
NB_LOCATION_BOOKMARK_NAME_PROMPT,   // ordinal 1443
NB_LOCATION_BOOKMARK_RENAME_PROMPT, // ordinal 1444
```

**Note:** Check current last ordinal to avoid conflicts. The alignment script confirmed 1309 entries. New ordinals must be sequential.

#### 2. `src/NetBox/WinSCPDialogs.cpp` — Replace hardcoded strings

```cpp
// Line 2687 — replace L"Bookmark name:" with:
FarPlugin->InputBox(GetMsg(NB_LOCATION_PROFILES_TITLE), GetMsg(NB_LOCATION_BOOKMARK_NAME_PROMPT), Name, 0)

// Line 2731 — replace L"New name:" with:
FarPlugin->InputBox(GetMsg(NB_LOCATION_PROFILES_TITLE), GetMsg(NB_LOCATION_BOOKMARK_RENAME_PROMPT), NewName, 0)
```

#### 3. Language files — Append 2 entries to each

| File | NB_LOCATION_BOOKMARK_NAME_PROMPT | NB_LOCATION_BOOKMARK_RENAME_PROMPT |
|------|----------------------------------|-------------------------------------|
| NetBoxEng.lng | `"Bookmark name:"` | `"New name:"` |
| NetBoxRus.lng | `"Имя закладки:"` | `"Новое имя:"` |
| NetBoxFr.lng | `"Nom du signet:"` | `"Nouveau nom:"` |
| NetBoxPol.lng | `"Nazwa zakładki:"` | `"Nowa nazwa:"` |
| NetBoxSpa.lng | `"Nombre del marcador:"` | `"Nuevo nombre:"` |

---

## Issue 3: No Confirmation Before Removing Bookmark

### Problem

`RemoveBookmarkClick()` (WinSCPDialogs.cpp:2707) silently deletes the selected bookmark without any confirmation dialog.

### Design

Add a `MessageDialog()` check before deletion. This follows the pattern used in `TMasterPasswordDialog` and other dialogs in the codebase.

### Code Changes

#### 1. `src/base/MsgIDs.h` — Add 1 new entry

```cpp
NB_LOCATION_PROFILES_REMOVE_CONFIRM, // ordinal 1445
```

#### 2. `src/NetBox/WinSCPDialogs.cpp` — `RemoveBookmarkClick` (line 2707)

Add confirmation before `Bookmarks->Delete()`:

```cpp
void TLocationProfilesDialog::RemoveBookmarkClick(TFarButton * /*Sender*/, bool & /*Close*/)
{
  UnicodeString Name = GetCurrentBookmarkName();
  if (!Name.IsEmpty())
  {
    TBookmarkList * Bookmarks = (GetTab() == tabSession) ? FSessionBookmarks : FSharedBookmarks;
    if (Bookmarks)
    {
      TBookmark * Bookmark = Bookmarks->FindByName(UnicodeString(), Name);
      if (Bookmark)
      {
        // NEW: Confirmation dialog
        if (MessageDialog(FORMAT(GetMsg(NB_LOCATION_PROFILES_REMOVE_CONFIRM), Name),
                          qtConfirmation, qaOK | qaCancel) != qaCancel)
        {
          Bookmarks->Delete(Bookmark);
          LoadBookmarks(GetTab());
          AppLogFmt(L"LocationProfiles: removed bookmark '%s'", Name);
        }
      }
    }
  }
}
```

#### 3. Language files — Add 1 entry to each

| File | NB_LOCATION_PROFILES_REMOVE_CONFIRM |
|------|-------------------------------------|
| NetBoxEng.lng | `"Do you wish to remove bookmark '%s'"` |
| NetBoxRus.lng | `"Удалить закладку '%s'"` |
| NetBoxFr.lng | `"Voulez-vous supprimer le signet '%s'"` |
| NetBoxPol.lng | `"Czy chcesz usunąć zakładkę '%s'"` |
| NetBoxSpa.lng | `"¿Desea eliminar el marcador '%s'"` |

---

## Issue 4: Null Check on FScriptFormatCombo

### Problem

`FScriptFormatCombo` and related pointers in `TGenerateUrlDialog` are initialized via `MakeOwnedObject` but never checked for null before dereference. Low probability but worth a defensive guard.

### Design

Add null guards at entry points of methods that access these pointers.

### Code Changes

#### `src/NetBox/WinSCPDialogs.cpp` — 3 methods

**GenerateScript()** (~line 2421):
```cpp
if (!FScriptFormatCombo || !FTransferModeBtn) return UnicodeString();
```

**UpdateScriptResult()** (~line 2413):
```cpp
if (!FScriptFormatCombo || !FScriptResultEdit) return;
```

**UpdateUrlResult()** (~line 2399):
```cpp
if (!FUrlResultEdit || !FSessionData) return;
```

No MsgID or `.lng` changes needed.

---

## Execution Order

```
1. Issue 4 (null checks) → verify: build zero warnings
2. Issue 2 (hardcoded English) → verify: grep for L"Bookmark name:" returns nothing
3. Issue 3 (remove confirmation) → verify: remove bookmark shows dialog
4. Issue 1 (Open navigation) → verify: click Open navigates to bookmarked dirs
```

---

## Commit Checkpoint

```
fix(ui): Phase 2 verification fixes

- Location Profiles "Open" button navigates to bookmarked directories
- Replace hardcoded English in Location Profiles InputBox with GetMsg()
- Add confirmation dialog before removing Location Profiles bookmark
- Add defensive null checks in TGenerateUrlDialog
```

---

## Success Criteria

| Issue | Verification |
|-------|-------------|
| 1 | Open button navigates remote panel to bookmarked remote dir + **local panel via SynchronizeBrowsing** |
| 2 | `grep -r "Bookmark name:" src/` returns no hits; all 5 .lng files have new entries |
| 3 | Removing bookmark shows confirmation; cancel prevents deletion |
| 4 | Build with zero warnings under /W4 (pre-existing warnings in WinConfiguration.h not counted) |
| All | `python scripts/verify_lng_alignment.py` passes |
| All | CRLF line endings, UTF-8 without BOM, no trailing whitespace |


## Additional Fixes (2026-05-10)

### Gap A: Local Dir Navigation (Resolved)
- **Location:** `WinSCPDialogs.cpp:2835` (wrapper)
- **Change:** Added `FileSystem->SynchronizeBrowsing(LocalDir)` to navigate local panel when opening a bookmark
- **Pattern:** Uses existing `TWinSCPFileSystem::SynchronizeBrowsing()` which calls `FarControl(FCTL_SETPANELDIRECTORY, ..., PANEL_PASSIVE)`
- **Status:** ✅ Done — build passes, zero new warnings

### Gap B: Clipboard Button Null Guards (Resolved)
- **Location:** `WinSCPDialogs.cpp:2348, 2368` (TGenerateUrlDialog)
- **Change:** Added `if (!FUrlResultEdit) return;` and `if (!FScriptResultEdit) return;` null guards
- **Status:** ✅ Done — build passes, zero new warnings
---

## Risks

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| `FTerminal->ChangeDirectory()` API differs from expected | Low | Medium | Check WinSCPFileSystem.h for correct API |
| Null checks mask real initialization bugs | Low | Low | Keep null checks as defensive only, log warning |
| New MsgIDs shift existing ordinals | Low | High | Verify ordinals are appended at end, not inserted |

---

## Changelog

- 2026-05-10: Initial plan created from Phase 2 verification findings
  - 2026-05-10: Gap A (local dir navigation) + Gap B (clipboard null guards) resolved, build verified
