# Plan: Fix Cipher ListBox Overflow in Session Dialog

**Branch:** feature/fix-cipher-listbox-overflow
**Created:** 2026-04-22
**Status:** Ready for implementation

## Settings

- **Testing:** No — UI layout fix only
- **Logging:** Minimal — no logging changes needed
- **Docs:** No — internal UI fix

## Roadmap Linkage

**Milestone:** Version 2.0 – Full plugin refactor, modular architecture
**Rationale:** UI/UX improvements align with the refactor milestone

## Overview

Fix the "Encryption cipher selection policy" stringlist (CipherListBox) that overflows dialog boundaries in the SSH session configuration dialog. The listbox is currently too wide and extends beyond the dialog's client area, causing visual issues.

## Root Cause

Based on exploration findings:
- Dialog width: 69 characters
- Client area width: 59 characters (left: 5, right: 63)
- CipherListBox initial width: 58 characters (full client width)
- After adjustment (`SetRight(GetRight() - 15)`): 43 characters wide, right edge at position 48
- Up/Down buttons placed at position 51 (48 + 3 gap)
- The `-15` offset is insufficient to prevent overflow when button captions are considered

## Tasks

### Phase 1: Investigate Current Layout

#### Task 1: Measure actual button widths
**Files:**
- `src/NetBox/NetBoxEng.lng`
- `src/NetBox/WinSCPDialogs.cpp`

**Description:**
Determine the actual width of "Up" and "Down" button captions to calculate the correct listbox width reduction.

**Implementation:**
1. Read button caption strings from `NetBoxEng.lng` (NB_LOGIN_UP, NB_LOGIN_DOWN)
2. Calculate button width: `caption_length + 4` (2 chars padding on each side for button borders)
3. Calculate required gap: 3 characters (standard Far Manager spacing)
4. Calculate total space needed: `button_width + gap`

**Acceptance:**
- Button caption lengths determined
- Total space requirement calculated

---

#### Task 2: Verify dialog client area dimensions
**Files:**
- `src/NetBox/WinSCPDialogs.cpp`

**Description:**
Confirm the dialog's client area dimensions and verify the CipherListBox positioning.

**Implementation:**
1. Find dialog initialization code (search for dialog width/height setup)
2. Verify client rect calculation: `TRect(3, 1, -4, -2)` relative to dialog size
3. Confirm client width: 59 characters
4. Document current CipherListBox left position: 5

**Acceptance:**
- Client area dimensions confirmed
- CipherListBox left position verified

---

### Phase 2: Fix ListBox Width

#### Task 3: Adjust CipherListBox width reduction
**Files:**
- `src/NetBox/WinSCPDialogs.cpp` (line 2943)

**Description:**
Increase the width reduction from `-15` to a value that accommodates the Up/Down buttons with proper spacing.

**Implementation:**
1. Calculate new reduction value:
   - Button width: ~8 characters (assuming "Down" is longest, ~4 chars + 4 padding)
   - Gap: 3 characters
   - Total reduction: 11 characters minimum
   - Use 12 for safety margin
2. Change line 2943 from:
   ```cpp
   CipherListBox->SetRight(CipherListBox->GetRight() - 15);
   ```
   to:
   ```cpp
   CipherListBox->SetRight(CipherListBox->GetRight() - 12);
   ```
3. Verify buttons still fit within dialog boundaries

**Logging:**
- None required (UI layout only)

**Acceptance:**
- CipherListBox width reduced appropriately
- Up/Down buttons fit within dialog boundaries
- No overlap between listbox and buttons

---

#### Task 4: Apply same fix to KexListBox
**Files:**
- `src/NetBox/WinSCPDialogs.cpp` (line 3011)

**Description:**
The KEX (Key Exchange) algorithm listbox uses the same layout pattern. Apply the same fix for consistency.

**Implementation:**
1. Locate KexListBox creation (around line 3011)
2. Find similar `SetRight(GetRight() - 15)` call
3. Apply the same width reduction value as CipherListBox
4. Verify KEX Up/Down buttons fit properly

**Logging:**
- None required

**Acceptance:**
- KexListBox width matches CipherListBox adjustment
- KEX buttons fit within dialog boundaries

---

### Phase 3: Verification

#### Task 5: Build and visual verification
**Files:**
- All modified files

**Description:**
Build the plugin and verify the dialog layout visually in Far Manager.

**Implementation:**
1. Clean build: `cmd /c build-x64.bat`
2. Launch Far Manager from `Far3_x64/`
3. Open NetBox plugin (F11 → NetBox)
4. Create/edit a session to open the session dialog
5. Navigate to SSH tab → Encryption options section
6. Verify:
   - CipherListBox fits within dialog boundaries
   - Up/Down buttons are fully visible
   - No horizontal scrolling required
   - Proper spacing between listbox and buttons
7. Navigate to KEX tab and verify same for KexListBox

**Acceptance:**
- Build completes with zero warnings
- CipherListBox displays correctly without overflow
- KexListBox displays correctly without overflow
- All buttons fully visible and functional
- Dialog layout looks balanced

---

## Commit Plan

**Single commit** (small focused fix):

```
fix(ui): adjust cipher/kex listbox width to prevent dialog overflow

- Reduce CipherListBox width offset from -15 to -12 characters
- Apply same fix to KexListBox for consistency
- Ensures Up/Down buttons fit within dialog boundaries

Fixes visual overflow in SSH session configuration dialog
```

## Notes

- This is a minimal UI layout fix with no functional changes
- No protocol logic affected
- No third-party code modifications
- CRLF line endings preserved
- Build must complete with zero warnings (MSVC W4)
