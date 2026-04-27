# Fix: Directory name autocomplete synchronization in CreateDirectoryDialog

**GitHub Issue**: [#515](https://github.com/michaellukashov/Far-NetBox/issues/515) - When creating a directory with F7 and accepting autocomplete, the folder is created with only the initial typed prefix instead of the full autocompleted name.

**Root Cause**: The Far Manager dialog's `TFarEdit` control has history-based autocomplete. When autocomplete completes the text visually, the internal `GetDialogItem()->Data` might not be synchronized with the actual dialog control text, causing `GetText()` to return stale prefix data instead of the autocompleted full name.

---

## Settings

| Setting | Value |
|---------|-------|
| **Mode** | Fast |
| **Tests** | No |
| **Logging** | Verbose |
| **Docs** | Yes |

---

## Research Context

The `CreateDirectoryDialog` function (src/NetBox/WinSCPDialogs.cpp:9364) creates a dialog for creating remote directories. The `DirectoryEdit` control has a history flag set (`SetHistory(L"NewFolder")`), enabling Far Manager's autocomplete feature.

Key observations:
- `TFarEdit::GetText()` returns `GetData()` which reads `GetDialogItem()->Data` (cached value)
- `TFarEdit::ItemProc` handles `DN_EDITCHANGE` to sync `GetDialogItem()->Data` from Far's dialog item
- When autocomplete completes visually, the dialog text is updated, but `DN_EDITCHANGE` notification may not propagate before the dialog closes with Enter key
- Result: `DirectoryEdit->GetText()` returns stale prefix instead of autocompleted full text

**Architecture boundary**: This is a Plugin Layer (`NetBox/`) issue. The fix should be contained within dialog handling code without affecting Core or Base layers.

---

## Exploration Results

**API Analysis Complete** - See [Far Dialog API: Text Retrieval](.ai-factory/references/far-dialog-api-text-retrieval.md)

Two viable Far Manager 3.0 Plugin APIs for retrieving text:

### Selected Approach: DM_GETDLGITEM (Message 5)

**Rationale:**
- Returns complete item state (flags, position, focus, text), not just text
- Aligns with NetBox's existing `FarDialogItem` usage
- Two-phase buffer sizing prevents overallocation/underallocation
- Reusable across any dialog with edit controls
- Complete item data available for `DEBUG_PRINTF` output

**Far API Structures:**
```cpp
// From src/PluginSDK/Far3/plugin.hpp:842-847
struct FarGetDialogItem
{
    size_t StructSize;           // sizeof(FarGetDialogItem)
    size_t Size;                 // Buffer size / required size
    struct FarDialogItem* Item;  // Pointer to FarDialogItem buffer
};

// Usage pattern:
intptr_t Size = SendDlgMessage(hDlg, DM_GETDLGITEM, ItemIdx, nullptr);
std::vector<uint8_t> Buffer(Size);
FarGetDialogItem ItemInfo{sizeof(FarGetDialogItem), Size, 
                          reinterpret_cast<FarDialogItem*>(Buffer.data())};
SendDlgMessage(hDlg, DM_GETDLGITEM, ItemIdx, &ItemInfo);
// Text is now in ItemInfo.Item->Data
```

**Alternative Rejected:** `DM_GETTEXT` (message 7) is simpler but less informative and doesn't match NetBox's architectural patterns.

---

## Tasks

### Task 1: Add GetTextFromDialog method declaration to TFarEdit [x]

**File:** `src/NetBox/FarDialog.h`

**Location:** In `class TFarEdit`, after `GetText()` declaration (around line 480).

**Change:**
Add new public method declaration:
```cpp
// Retrieves text directly from Far dialog control, bypassing cached Data.
// Use when autocomplete or other Far-controlled text changes may have
// updated the control without firing DN_EDITCHANGE notification.
// See far-dialog-api-text-retrieval.md for API details.
UnicodeString GetTextFromDialog() const;
```

---

### Task 2: Implement GetTextFromDialog method

**File:** `src/NetBox/FarDialog.cpp`

**Location:** After `TFarEdit::GetHistoryMask()` implementation (around line 2034).

**Change:**
```cpp
UnicodeString TFarEdit::GetTextFromDialog() const
{
    UnicodeString Result;
    
    // If dialog is not shown, fall back to cached data
    if (!GetDialog()->GetHandle())
    {
        Result = GetData();
        DEBUG_PRINTF("GetTextFromDialog: Dialog not active, returning cached '%s'", 
                     Result.c_str());
        return Result;
    }

    // Query Far Manager for actual item data size
    // Query Far Manager for actual item data size
    intptr_t Size = SendDialogMessage(DM_GETDLGITEM, nullptr);
    if (Size <= 0)
    {
      DEBUG_PRINTF("GetTextFromDialog: DM_GETDLGITEM returned %d, using cached",
        static_cast<int32_t>(Size));
      return GetData();
    }

    // Allocate buffer and retrieve full item data
    std::vector<uint8_t> Buffer(Size);
    FarGetDialogItem ItemInfo{};
    ItemInfo.StructSize = sizeof(FarGetDialogItem);
    ItemInfo.Size = Size;
    ItemInfo.Item = reinterpret_cast<FarDialogItem*>(Buffer.data());

    SendDialogMessage(DM_GETDLGITEM, &ItemInfo);

    // Extract text from retrieved item (Data field for DI_EDIT controls)
    // Note: This relies on Far's ABI - FarDialogItem.Data contains text for edit controls

    // Extract text from retrieved item (Data field for DI_EDIT controls)
    if (ItemInfo.Item && ItemInfo.Item->Data)
    {
        Result = UnicodeString(ItemInfo.Item->Data);
    }

    DEBUG_PRINTF("GetTextFromDialog: item=%d, cached='%s', fresh='%s'",
                 GetItemIdx(), GetData().c_str(), Result.c_str());

    return Result;
}
```

**Edge Cases Handled:**
- Dialog not shown (handle invalid) → returns cached `GetData()`
- Empty text → returns empty `UnicodeString` (not null)
- Far API error → falls back to cached `GetData()`
- Buffer allocation → uses RAII `std::vector` for safety


**Assumptions:**
- `FarDialogItem.Data` field contains the edit control text (Far Manager ABI contract)
- `std::vector` header is available via precompiled headers or existing includes
- `SendDialogMessage` item-level method (via `TFarDialogItem`) handles Param1/Param2 correctly
---

### Task 3: Fix CreateDirectoryDialog to use GetTextFromDialog

**File:** `src/NetBox/WinSCPDialogs.cpp`

**Location:** In `TWinSCPFileSystem::CreateDirectoryDialog` function, line 9408.

**Current code:**
```cpp
if (Result)
{
  Directory = DirectoryEdit->GetText();  // Line 9408 - STALE DATA BUG
```

**Fixed code:**
```cpp
if (Result)
{
  Directory = DirectoryEdit->GetTextFromDialog();  // Fresh data from Far
```

**Verification:** The `Result` boolean indicates `ShowModal() == brOK`, so the user pressed Enter to confirm. At this point, autocomplete may have updated the visual text without updating cached data.

---

## Testing & Verification

### Build Verification
```cmd
cmd /c build-x64.bat
```

### Manual Testing Checklist

**Primary Protocol (SFTP):**
1. Connect to an SFTP session (most commonly used with NetBox)
2. Press F7 to create directory (brings up `CreateDirectoryDialog`)
3. Type partial directory name (e.g., "te")
4. Accept autocomplete suggestion (e.g., "test-directory") via Tab/Enter
5. Press Enter to confirm dialog
6. Verify the created directory has the full autocompleted name, not just "te"

**Secondary Protocols (FTP/SCP/WebDAV/S3):**
7. Repeat steps 1-6 for at least one other protocol

### Debug Verification
Enable debug build and check `DEBUG_PRINTF` output shows:
```
GetTextFromDialog: item=1, cached='te', fresh='test-directory'
```

### Edge Case Testing
| Scenario | Expected Result |
|----------|---------------|
| Type without autocomplete | Works as before |
| Accept autocomplete with Tab/Enter | Full autocompleted name used |
| Cancel dialog | No directory created |
| Empty directory name | Validation error or empty string handling |
| Unicode in directory name | Full Unicode string preserved |

---

## Commit Plan

Single commit after Task 3 completion:

```
fix(dialog): synchronize edit text after autocomplete in CreateDirectory

Add TFarEdit::GetTextFromDialog() to retrieve fresh text directly from
Far Manager dialog control, bypassing potentially stale cached Data.
Fixes a bug where creating a directory with autocompleted name would
only use the initially typed prefix instead of the full autocompleted
name. The issue was that TFar