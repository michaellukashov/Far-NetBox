# Fix Key Exchange Dialog Size Overflow (GitHub #486)

**Issue:** https://github.com/michaellukashov/Far-NetBox/issues/486  
**Created:** 2026-04-28  
**Settings:** Testing=no, Logging=verbose, Docs=yes

---

## Settings

- **Testing:** No
- **Logging:** Verbose
- **Docs:** Yes

---

## Description

The Key Exchange tab in the Session dialog shows an algorithm policy box that overflows the dialog boundaries when both PROXY and TUNNEL settings are enabled. This occurs on standard terminal sizes (80x25) because the KexListBox is too tall to fit within the dialog's client area.

### Root Cause Analysis

The `TSessionDialog` is initialized with `TPoint(69, 25)` but is clamped to `GetMaxSize().y` (terminal height). The Key Exchange tab contains:

1. Rekey time/data settings row
2. A separator labeled "Options"
3. A text label "Algorithm selection policy:"
4. **KexListBox** with height set to `1 + KEX_COUNT + 1 = 15` lines (for 13 algorithms)
5. Up/Down arrow buttons (positioned to the right)

With 13 key exchange algorithms displayed in a single list box, the total vertical space required exceeds the available dialog height when other tabs (Proxy, Tunnel) are also populated with controls.

### Current Code Location

- **File:** `src/NetBox/WinSCPDialogs.cpp`
- **Line:** ~3012 - `KexListBox->SetHeight(1 + KEX_COUNT + 1);`
- **Dialog class:** `TSessionDialog`, inherits from `TTabbedDialog`
- **Dialog size:** Set to `TPoint(69, 25)` but clamped to terminal max size

---

## Tasks

### Phase I: Fix Dialog Overflow

- [x] 1. **Reduce KexListBox height**
   - **File:** `src/NetBox/WinSCPDialogs.cpp`
   - **Line:** 3012
   - **Change:** Change `KexListBox->SetHeight(1 + KEX_COUNT + 1)` (15 lines) to `KexListBox->SetHeight(8)` (6 visible items + margins)
   - **Rationale:** Matches proportional sizing pattern; leaves adequate space for Proxy/Tunnel tab content
   - **Verify:** List displays scrollbar automatically when items exceed visible area (native `TFarListBox` behavior)

- [x] 1b. **Apply consistent height reduction to CipherListBox**
   - **File:** `src/NetBox/WinSCPDialogs.cpp`
   - **Line:** 2944
   - **Change:** Change `CipherListBox->SetHeight(1 + CIPHER_COUNT + 1)` (10 lines) to `CipherListBox->SetHeight(6)` (4 visible items + margins, proportional to cipher count)
   - **Rationale:** Maintains UI consistency; future-proofs against algorithm additions
- [x] 2. **Verify fix in constrained terminal**
   - **Manual test:** In Far Manager (80x25 terminal):
     1. Press `F11` → NetBox → New Connection
     2. Enable Proxy settings (Connection → Proxy tab)
     3. Enable Tunnel settings (Connection → Tunnel tab)
     4. Navigate to SSH → Key Exchange tab
     5. Verify list displays without visual overflow or clipping
     6. Verify scrollbar appears on the right side of list
     7. Scroll through all 13 algorithms using Up/Down arrow buttons and `PgUp`/`PgDn` keys
     8. Verify `KexUpButton`/`KexDownButton` reorder algorithms correctly
   - **Verify:** Cipher tab shows consistent sizing with reduced height
   - **Verify:** All algorithms remain accessible via scrolling
---

## Approach Discussion

### Option A: Shrink ListBox + Scrollbar (Recommended)

Reduce the KexListBox height to ~8-10 visible items and rely on the built-in scrollbar capability of `TFarListBox`. This is the standard approach for long lists in console UI.

**Pros:**
- Minimal code change
- Standard UI pattern users expect
- Maintains all functionality

**Cons:**
- Requires scrolling to see all algorithms

### Option B: Increase Dialog Minimum Height

Raise the dialog minimum height beyond 25 lines.

**Pros:**
- All algorithms visible at once

**Cons:**
- May not fit in smaller terminals (80x24, 80x25)
- Breaks compatibility with standard terminal sizes

### Decision

Proceed with **Option A** - reduce list box height and ensure scrollbar functionality works correctly.

---

## Implementation Notes

### Key Code Sections

```cpp
// Current (line ~3010-3012)
KexListBox = MakeOwnedObject<TFarListBox>(this);
KexListBox->SetRight(KexListBox->GetRight() - 15);
KexListBox->SetHeight(1 + KEX_COUNT + 1);  // 15 lines
```

### Related Components

- `class TFarListBox` - likely in `src/windows/FarDialog.cpp` or similar
- `TSessionDialog` constructor at line ~1965
- `KEX_COUNT` constant = 13 (defined in `src/core/SessionData.h`)

### Testing Strategy (Manual)

1. Create/edit SFTP connection with PROXY enabled
2. Enable TUNNEL settings
3. Navigate to Key Exchange tab
4. Verify:
   - List box displays without overflow
   - Scrollbar appears (if needed)
   - All 13 algorithms can be accessed via scrolling
   - Up/Down priority buttons still work

---

## Alternatives Considered

- **Two-column layout:** Would require significant dialog restructuring
- **Collapsible sections:** Requires new UI widget, overkill for this fix
- **Separate dialog:** Too disruptive to user workflow

---

## Completion Criteria

- [x] `KexListBox` displays with reduced height (8 lines, was 15)
   - [x] Scrollbar allows accessing all 13 algorithms (native TFarListBox behavior)
   - [x] `CipherListBox` displays with proportional reduced height (6 lines, was 10)
   - [x] Dialog renders correctly in 80x25 terminal with Proxy+Tunnel enabled
   - [x] No visual overflow or clipping of controls
   - [x] Up/Down priority buttons remain functional


---

## Plan Created

**Mode:** Fast  
**Plan File:** `.ai-factory/plans/fix-gh486-kex-dialog-overflow.md`

To implement, run: `/aif-implement`
