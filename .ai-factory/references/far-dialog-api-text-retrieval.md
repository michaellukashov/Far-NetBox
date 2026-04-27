# Far Dialog API: Text Retrieval from Edit Controls

**Issue Reference:** [#515](https://github.com/michaellukashov/Far-NetBox/issues/515)  
**Exploration Date:** 2026-04-27  
**Purpose:** Determine correct Far Manager 3.0 Plugin API for retrieving fresh text from DI_EDIT controls when autocomplete may have updated the display without firing DN_EDITCHANGE.

---

## Summary

The autocomplete synchronization bug in `CreateDirectoryDialog` requires retrieving text directly from the Far dialog control, bypassing NetBox's cached `GetDialogItem()->Data`. Two viable APIs exist: `DM_GETTEXT` and `DM_GETDLGITEM`.

**Recommendation:** Use `DM_GETDLGITEM` with `FarGetDialogItem` structure for complete item data retrieval. This provides access to the full `FarDialogItem` structure including the `Data` field (text content).

---

## Far Manager API Options

### Option 1: DM_GETTEXT (Message 7)

```cpp
DM_GETTEXT = 7
```

**Usage Pattern:**
```cpp
// First call: get required buffer size
intptr_t Len = SendDlgMessage(hDlg, DM_GETTEXT, ItemIdx, nullptr);

// Second call: retrieve text into buffer
std::vector<wchar_t> Buffer(Len + 1);
FarDialogItemData Data{};
Data.StructSize = sizeof(FarDialogItemData);
Data.PtrData = Buffer.data();
Data.PtrLength = Len;
SendDlgMessage(hDlg, DM_GETTEXT, ItemIdx, &Data);
UnicodeString Result(Buffer.data(), static_cast<int32_t>(Len));
```

**Structure:**
```cpp
struct FarDialogItemData
{
    size_t StructSize;   // sizeof(FarDialogItemData)
    size_t PtrLength;    // Buffer size / received length
    wchar_t *PtrData;    // Text buffer (caller allocates, Far fills)
};
```

**Pros:**
- Direct text retrieval
- Single-purpose API

**Cons:**
- Two-phase call required (size query + data retrieval)
- Only returns text, not other item properties

---

### Option 2: DM_GETDLGITEM (Message 5) - RECOMMENDED

```cpp
DM_GETDLGITEM = 5
```

**Usage Pattern:**
```cpp
// First call: get required buffer size
intptr_t Size = SendDlgMessage(hDlg, DM_GETDLGITEM, ItemIdx, nullptr);

// Allocate buffer and structure
std::vector<uint8_t> Buffer(Size);
FarGetDialogItem ItemInfo{};
ItemInfo.StructSize = sizeof(FarGetDialogItem);
ItemInfo.Size = Size;
ItemInfo.Item = reinterpret_cast<FarDialogItem*>(Buffer.data());

// Second call: retrieve full item data
SendDlgMessage(hDlg, DM_GETDLGITEM, ItemIdx, &ItemInfo);

// Access text from retrieved item
UnicodeString Result(ItemInfo.Item->Data);
```

**Structures:**
```cpp
struct FarGetDialogItem
{
    size_t StructSize;           // sizeof(FarGetDialogItem)
    size_t Size;                 // Buffer size / required size
    struct FarDialogItem* Item;  // Pointer to FarDialogItem buffer
};

struct FarDialogItem
{
    FARMESSAGETYPES Type;
    int X1, Y1, X2, Y2;
    int Focus;
    DWORD_PTR Param;
    DWORD Flags;
    DWORD_PTR DefaultButton;
    // ... type-specific data follows
    // For DI_EDIT: Data contains the text
};
```

**Pros:**
- Returns complete item state (flags, position, focus, text)
- Matches NetBox's internal `FarDialogItem` usage
- Future-proof for additional item properties

**Cons:**
- Slightly more complex buffer management
- Requires understanding of `FarDialogItem` structure layout

---

## Implementation in NetBox Context

### Current Code Flow (Problematic)

```cpp
// WinSCPDialogs.cpp:9408
if (Result)
{
    Directory = DirectoryEdit->GetText();  // Returns cached Data
```

Where `GetText()` → `GetData()` → `GetDialogItem()->Data` (cached at dialog creation).

### Proposed Fix Flow

```cpp
// New method in TFarEdit (FarDialog.cpp)
UnicodeString TFarEdit::GetTextFromDialog() const
{
    if (!GetDialog()->GetHandle())
        return GetData();  // Dialog not shown yet

    // Query Far Manager for actual item data
    intptr_t Size = GetDialog()->SendDlgMessage(DM_GETDLGITEM, GetItemIdx(), nullptr);
    if (Size <= 0)
        return GetData();

    std::vector<uint8_t> Buffer(Size);
    FarGetDialogItem ItemInfo{};
    ItemInfo.StructSize = sizeof(FarGetDialogItem);
    ItemInfo.Size = Size;
    ItemInfo.Item = reinterpret_cast<FarDialogItem*>(Buffer.data());

    SendDlgMessage(DM_GETDLGITEM, GetItemIdx(), &ItemInfo);

    return UnicodeString(ItemInfo.Item->Data);
}
```

### Fixed Call Site

```cpp
// WinSCPDialogs.cpp:9408 (updated)
if (Result)
{
    Directory = DirectoryEdit->GetTextFromDialog();  // Fresh from Far
```

---

## Edge Cases

| Scenario | Handling |
|----------|----------|
| Dialog not shown (GetHandle() == nullptr) | Fall back to cached `GetData()` |
| Empty text | Return empty `UnicodeString` (valid state) |
| Buffer allocation failure | Fall back to cached `GetData()` |
| Far API returns error | Fall back to cached `GetData()` |
| Memory pressure | Use stack buffer for small allocations, heap for large |

---

## References

- **Far Manager API:** `src/PluginSDK/Far3/plugin.hpp:812-847`
  - `DM_GETTEXT = 7` (line 511)
  - `DM_GETDLGITEM = 5` (line 509)
  - `struct FarDialogItemData` (lines 812-817)
  - `struct FarGetDialogItem` (lines 842-847)

- **NetBox Implementation:**
  - `src/NetBox/FarDialog.h:470` - `TFarEdit` class definition
  - `src/NetBox/FarDialog.cpp:2017` - `ItemProc` handling `DN_EDITCHANGE`
  - `src/NetBox/FarDialog.cpp:856` - `SendDlgMessage` implementation
  - `src/NetBox/FarDialog.cpp:1225` - `GetData()` implementation

- **Bug Location:**
  - `src/NetBox/WinSCPDialogs.cpp:9364` - `CreateDirectoryDialog`
  - `src/NetBox/WinSCPDialogs.cpp:9408` - `GetText()` call (buggy)

---

## Decision Rationale

**Selected Approach:** `DM_GETDLGITEM` via `GetTextFromDialog()` method

**Reasons:**
1. **Completeness:** Returns full item state, not just text
2. **Compatibility:** Aligns with NetBox's existing `FarDialogItem` usage
3. **Safety:** Two-phase buffer sizing prevents overallocation/underallocation
4. **Reusability:** Method can be used for any edit control in any dialog
5. **Debugging:** Complete item data available for `DEBUG_PRINTF` output

**Alternative Rejected:** Direct `DM_GETTEXT` is simpler but less informative and doesn't match NetBox's architectural patterns.
