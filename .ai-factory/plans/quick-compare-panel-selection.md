# Plan: Quick Compare Directories with Panel Selection

## Goal
Implement WinSCP-style quick directory comparison in NetBox that selects differing files directly in Far Manager panels (no heavy checklist dialog), using configurable time/size criteria.

## References
- WinSCP: `TScpCommanderForm::CompareDirectories()` — `CustomDirView.pas:CompareFiles()`
- WinSCP: `TWinConfiguration::CompareCriterias()` — `WinConfiguration.h:81`
- NetBox existing: `TWinSCPFileSystem::CompareDirectories()` — `WinSCPFileSystem.cpp:1762`
- NetBox checklist: `TSynchronizeChecklistDialog` — `WinSCPDialogs.cpp:9909` (keep as-is for full sync)

## Phase 1: Configuration Storage + Dialog
**Files**: `src/NetBox/FarConfiguration.h`, `src/NetBox/FarConfiguration.cpp`, `src/NetBox/WinSCPDialogs.cpp`, `src/base/MsgIDs.h`, `src/NetBox/NetBox*.lng`

1. Add to `TFarConfiguration`:
   ```cpp
   bool FCompareByTime{true};
   bool FCompareBySize{false};
   ```
   with getters/setters + XML save/load in `TFarConfiguration::Load`/`Save`.
   Insert the `KEY(Bool, CompareByTime)` and `KEY(Bool, CompareBySize)` macros into the existing `REGISTRY_INI` / `XML` key list in `FarConfiguration.cpp` (after `SessionNameInTitle`, following existing alphabetical convention where possible).

2. Add `NB_COMPARE_BY_TIME` and `NB_COMPARE_BY_SIZE` to `MsgIDs.h` (2 new IDs).
   Insert them immediately before the closing `};` of the `MsgIDs` enum at line ~1459, in the "Compare Directories" comment section after `NB_COMPARE_DIRECTORIES_NO_DIFFERENCES`.

3. Add corresponding strings to all 5 `.lng` files at matching ordinals.
   Insert after the existing `NB_COMPARE_DIRECTORIES_NO_DIFFERENCES` entry (which reads `"No differences found."` in `NetBoxEng.lng`).

4. Add checkboxes to existing `TConfigurationDialog` (`WinSCPDialogs.cpp`, line ~338, after `SessionNameInTitleCheck`):
   ```cpp
   MakeOwnedObject<TFarSeparator>(Dialog)->SetCaption(GetMsg(NB_COMPARE_CRITERIA_GROUP));
   TFarCheckBox * CompareByTimeCheck = MakeOwnedObject<TFarCheckBox>(Dialog);
   CompareByTimeCheck->SetCaption(GetMsg(NB_COMPARE_BY_TIME));
   TFarCheckBox * CompareBySizeCheck = MakeOwnedObject<TFarCheckBox>(Dialog);
   CompareBySizeCheck->SetCaption(GetMsg(NB_COMPARE_BY_SIZE));
   ```
   Wire to `GetFarConfiguration()->SetCompareByTime()` / `SetCompareBySize()` in the dialog's save path (around line ~436).

## Phase 2: Quick Compare Algorithm
**Files**: `src/NetBox/WinSCPFileSystem.cpp` (add `#include <algorithm>` for `std::find`), `src/NetBox/WinSCPFileSystem.h` (no changes needed)

1. Rewrite `TWinSCPFileSystem::CompareDirectories()`:
   ```cpp
   void TWinSCPFileSystem::CompareDirectories()
   {
     TFarPanelInfo ** AnotherPanel = GetAnotherPanelInfo();
     RequireLocalPanel(*AnotherPanel, GetMsg(NB_SYNCHRONIZE_LOCAL_PATH_REQUIRED));

     const UnicodeString LocalDir = (*AnotherPanel)->GetCurrentDirectory();
     const UnicodeString RemoteDir = FTerminal->GetCurrentDirectory();

     // Build criteria params from stored config
     // CORRECT mapping (see "Key Design Decisions"):
     //  - Time comparison is DEFAULT (spNotByTime CLEAR)
     //  - spTimestamp is a special sync mode, NOT a comparison flag
     int32_t Params = 0;
     if (!GetFarConfiguration()->GetCompareByTime()) Params |= TTerminal::spNotByTime;
     if (GetFarConfiguration()->GetCompareBySize()) Params |= TTerminal::spBySize;

     try
     {
       const TCopyParamType & CopyParam = static_cast<const TCopyParamType &>(
         GetGUIConfiguration()->GetDefaultCopyParam());
       std::unique_ptr<TSynchronizeChecklist> Checklist(FTerminal->SynchronizeCollect(
         LocalDir, RemoteDir, TTerminal::smBoth, &CopyParam, Params, nullptr, nullptr));

       if (!Checklist || Checklist->GetCount() == 0)
       {
         MoreMessageDialog(GetMsg(NB_COMPARE_DIRECTORIES_NO_DIFFERENCES),
           nullptr, qtInformation, qaOK);
         return;
       }

       // Collect differing file names from both sides
       // Use std::vector + linear search instead of std::set to avoid
       // allocator complexity with custom allocators
       std::vector<UnicodeString> LocalDiffs, RemoteDiffs;
       for (int32_t i = 0; i < Checklist->GetCount(); ++i)
       {
         const TChecklistItem * Item = Checklist->GetItem(i);
         // IsDirectory is a bool field (not a method), skip directories
         if (Item->IsDirectory) continue;
         if (!Item->Local.FileName.IsEmpty()) LocalDiffs.push_back(Item->Local.FileName);
         if (!Item->Remote.FileName.IsEmpty()) RemoteDiffs.push_back(Item->Remote.FileName);
       }

       // Select differing files in local panel (another panel)
       if (AnotherPanel && *AnotherPanel)
       {
         TObjectList * Items = (*AnotherPanel)->GetItems();
         for (int32_t i = 0; i < Items->GetCount(); ++i)
         {
           TFarPanelItem * Item = Items->GetAs<TFarPanelItem>(i);
           if (Item->GetIsParentDirectory()) continue;
           const bool Selected = (std::find(LocalDiffs.begin(), LocalDiffs.end(),
             Item->GetFileName()) != LocalDiffs.end());
           Item->SetSelected(Selected);
         }
         (*AnotherPanel)->ApplySelection();
       }

       // Select differing files in remote panel (this plugin panel)
       {
         TFarPanelInfo ** Panel = GetPanelInfo();
         if (Panel && *Panel)
         {
           TObjectList * Items = (*Panel)->GetItems();
           for (int32_t i = 0; i < Items->GetCount(); ++i)
           {
             TFarPanelItem * Item = Items->GetAs<TFarPanelItem>(i);
             if (Item->GetIsParentDirectory()) continue;
             const bool Selected = (std::find(RemoteDiffs.begin(), RemoteDiffs.end(),
               Item->GetFileName()) != RemoteDiffs.end());
             Item->SetSelected(Selected);
           }
           (*Panel)->ApplySelection();
         }
       }

       // Redraw both panels
       RedrawPanel();
       RedrawPanel(true); // Another panel

       // Show summary message with counts
       MoreMessageDialog(
         FORMAT(GetMsg(NB_COMPARE_DIRECTORIES_RESULT),
           LocalDiffs.size(), RemoteDiffs.size()),
         nullptr, qtInformation, qaOK);
     }
     catch (Exception & E)
     {
       FTerminal->LogEvent(FORMAT(L"CompareDirectories: error: %s", E.Message));
       ShowExtendedException(&E);
     }
   }
   ```

2. Add `NB_COMPARE_DIRECTORIES_RESULT` MsgID:
   - String: `"Differences found: %d local files, %d remote files"`
   - Insert in `MsgIDs.h` after `NB_COMPARE_DIRECTORIES_NO_DIFFERENCES`
   - Add corresponding strings to all 5 `.lng` files

## Phase 3: Keep Full Synchronize Checklist
**Files**: `src/NetBox/WinSCPFileSystem.cpp`

- Existing `TSynchronizeChecklistDialog` workflow is unchanged.
- Existing `TFullSynchronizeDialog` (Commands → Synchronize) remains the path for full sync with preview + execution.
- The menu structure stays:
  - `Commands → Compare Directories` → quick panel selection (new behavior)
  - `Commands → Synchronize` → full checklist dialog (existing)

## Key Design Decisions

1. **Re-use `SynchronizeCollect`** — Already implements the full comparison algorithm (time precision handling, size comparison, subdirectories, filters). No need to reimplement `ProcessChangedFiles` from WinSCP.

2. **Correct synchronize params mapping** — This is the most critical detail:
   | Config | Params value | Behavior |
   |--------|-------------|----------|
   | Time only | `0` (default) | Compare by modification time |
   | Size only | `spNotByTime \| spBySize` | Compare by file size only |
   | Both | `spBySize` | Compare by time, then size if times equal |
   | Neither | `spNotByTime` | No files will ever differ |
   
   **CRITICAL BUG TO AVOID**: Do NOT use `spTimestamp` (0x100) as a comparison criteria flag. `spTimestamp` is a special synchronization mode that performs timestamp-only updates without transferring file contents. The core comparison logic at `Terminal.cpp:6918` uses `spNotByTime` (CLEAR = compare by time, SET = skip time comparison) and `spBySize` (SET = also compare by size when times match or spNotByTime is set).

3. **Do NOT use `spExistingOnly`** — WinSCP quick compare (`ExistingOnly = false`) treats files that exist only on one side as differences. `SynchronizeCollect` with default Params already behaves this way. Adding `spExistingOnly` would incorrectly hide these files.

4. **Directory handling** — Skip directories via `Item->IsDirectory` (a bool field, not a method). The panel iteration additionally skips `..` via `GetIsParentDirectory()`.

5. **Panel selection via `TFarPanelItem::SetSelected` + `ApplySelection()`** — Proven pattern in NetBox (`FarPlugin.cpp:2907`). No new Far API calls needed.

6. **Use `std::vector` instead of `std::set`** — Simpler, avoids potential issues with custom allocators (`nb::set_t` requires `std::less<UnicodeString>` which may not be fully defined).

7. **RedrawPanel() calls** — Ensure Far Manager refreshes the visual selection state.

## String Resources Required

| MsgID | English | .lng ordinal | Insert after |
|-------|---------|-------------|------------|
| `NB_COMPARE_BY_TIME` | "&Modification time" | After existing NB_COMPARE_* IDs | `NB_COMPARE_DIRECTORIES_NO_DIFFERENCES` |
| `NB_COMPARE_BY_SIZE` | "File si&ze" | After NB_COMPARE_BY_TIME | NB_COMPARE_BY_TIME |
| `NB_COMPARE_DIRECTORIES_RESULT` | "Differences found: %d local files, %d remote files" | After NB_COMPARE_BY_SIZE | NB_COMPARE_BY_SIZE |
| `NB_COMPARE_CRITERIA_GROUP` | " Compare criteria " | After NB_COMPARE_DIRECTORIES_RESULT | (for dialog separator) |

## Estimate

- Phase 1 (Config + strings): ~80 lines across 8 files
- Phase 2 (Algorithm): ~60 lines in WinSCPFileSystem.cpp
- Phase 3 (Verify existing sync): No changes needed
- Total: ~140 lines, 1 build cycle

## Verification

| Test Case | Expected Result | Status |
|-----------|----------------|--------|
| Build passes zero warnings | ✅ | ✅ PASS |
| All 5 `.lng` files aligned (use verify script) | ✅ | ✅ PASS (1319 entries each) |
| Both criteria ON, identical directories | "No differences found" message | ⏭️ Manual test |
| Time only, different timestamps | Files with different times selected in both panels | ⏭️ Manual test |
| Size only, different sizes | Files with different sizes selected in both panels | ⏭️ Manual test |
| Both ON, same time but different size | Files selected (time equal → size check triggers) | ⏭️ Manual test |
| Both OFF | All files skipped by comparison logic → "No differences found" | ⏭️ Manual test |
| File exists only on local side | Selected in local panel (WinSCP behavior: missing = different) | ⏭️ Manual test |
| File exists only on remote side | Selected in remote panel | ⏭️ Manual test |
| Full Synchronize checklist | Still works unchanged via Commands → Synchronize | ⏭️ Manual test |
| Config dialog saves/loads criteria | CompareByTime/CompareBySize persist across sessions | ⏭️ Manual test |