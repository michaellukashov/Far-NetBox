# Fix Dialog Text Truncation — Off-By-One in Bounds Calculation (Issue #387)

**GitHub Reference:** [michaellukashov/Far-NetBox#387](https://github.com/michaellukashov/Far-NetBox/issues/387)
**Original upstream:** [FarGroup/Far-NetBox#25](https://github.com/FarGroup/Far-NetBox/issues/25) by @BestiaPL, fixed by @yulian5
**Plan Date:** 2026-05-02
**Mode:** Fast
**Plan ID:** fix-issue-387-dialog-bounds

---

## Settings

| Setting | Value |
|---------|-------|
| Testing | no |
| Logging | verbose |
| Docs    | no |

---

## Research Context

**Bug:** Dialog text fields have their bounds calculated incorrectly — one character is truncated from each text label. Most visible in the Configuration → Integration dialog where strings like "Putty path", "Pageant path", "PuTTYgen path" appear as "Putty pat", "Pageant pat", "PuTTYgen pat" (missing last character 'h').

**Root Cause:** `TFarDialogItem::SetWidth()` and `TFarDialogItem::SetHeight()` in `src/NetBox/FarDialog.cpp` use an off-by-one adjustment (`-1` for positive coordinates, `+1` for negative coordinates) that subtracts one character from the rendered text width.

**Current code (broken):**
```cpp
// SetWidth — lines 1584-1597
if (R.Left >= 0)
  R.Right = R.Left + Value - 1;      // off by one: -1 subtracts a character
else
  R.Left = R.Right - nb::ToInt32(Value + 1);  // off by one: +1 subtracts a character

// SetHeight — lines 1604-1617
if (R.Top >= 0)
  R.Bottom = nb::ToInt32(R.Top + Value - 1);    // same off-by-one
else
  R.Top = nb::ToInt32(R.Bottom - Value + 1);    // same off-by-one
```

**Why GetWidth() works correctly:**
`GetWidth()` at line 1599-1602 returns `GetActualBounds().Width() + 1` — it adds 1 to compensate for the SetWidth off-by-one. After fixing SetWidth, GetWidth() must also be corrected to remove the compensation.

**Patch reference:** [231219.NetBox.patch](https://github.com/FarGroup/Far-NetBox/files/13721264/231219.NetBox.patch) by @yulian5 (Dec 2023) — provided the fix for `SetWidth` but did not include `SetHeight` or `GetWidth()`.

**Fix strategy:** Remove the `+1`/`-1` adjustments in both `SetWidth` and `SetHeight`, and remove the compensating `+1` in `GetWidth()` (also fix `GetHeight()` if it has a similar pattern).

---

## Tasks

### Phase 1: Fix Bounds Calculation

 - [x] **Task 1:** Fix `TFarDialogItem::SetWidth()` off-by-one
   **File:** `src/NetBox/FarDialog.cpp`
   **Lines:** 1589, 1594
   **Change:**
   - Line 1589: `R.Right = R.Left + Value - 1` → `R.Right = R.Left + Value`
   - Line 1594: `R.Left = R.Right - nb::ToInt32(Value + 1)` → `R.Left = R.Right - nb::ToInt32(Value)`
   **Logging:** Add `DEBUG_PRINTF("SetWidth: Left=%d, Value=%d, Right=%d", nb::ToInt32(R.Left), Value, nb::ToInt32(R.Right))` after calculating bounds in SetWidth (guard with `#ifdef _DEBUG` or `DebugAssert` pattern). TFarDialogItem has no FTerminal context — use debug printf, not LogEvent.
 
 - [x] **Task 2:** Fix `TFarDialogItem::SetHeight()` off-by-one
   **File:** `src/NetBox/FarDialog.cpp`
   **Lines:** 1609, 1614
   **Change:**
   - Line 1609: `R.Bottom = nb::ToInt32(R.Top + Value - 1)` → `R.Bottom = nb::ToInt32(R.Top + Value)`
   - Line 1614: `R.Top = nb::ToInt32(R.Bottom - Value + 1)` → `R.Top = nb::ToInt32(R.Bottom - Value)`
   **Logging:** Same pattern as Task 1 for height.
 
 - [x] **Task 3:** Fix `TFarDialogItem::GetWidth()` and `GetHeight()` compensation
   **File:** `src/NetBox/FarDialog.cpp`
   **Lines:** 1599-1602 (GetWidth), 1619-1622 (GetHeight)
   **Change:**
   - `GetWidth()` line 1601: `return nb::ToInt32(GetActualBounds().Width() + 1)` → `return nb::ToInt32(GetActualBounds().Width())`
   - `GetHeight()` line 1621: `return nb::ToInt32(GetActualBounds().Height() + 1)` → `return nb::ToInt32(GetActualBounds().Height())`
   **Verification:** After fixing SetWidth/SetHeight, the getters should return plain bounds without compensation. Confirm `SetWidth(GetWidth() + (CurrHistory ? -1 : 1))` at line 2117 still produces the same net width (old: getter returns +1, setter subtracts 1 = net 0; new: getter returns plain, setter uses plain = net 0).
 
 - [x] **Task 4:** Audit all callers of SetWidth/SetHeight/GetWidth/GetHeight for compensating arithmetic
   **File:** `src/NetBox/FarDialog.cpp`, `src/NetBox/WinSCPDialogs.cpp`, `src/NetBox/FarDialog.h`
   **Action:** Search for patterns like `GetWidth() - 1`, `GetHeight() - 1`, `SetWidth(.*\+ 1`, `SetHeight(.*\+ 1` in src/NetBox/. Any caller that explicitly compensated for the old off-by-one will now double-count. Document findings; remove compensations if found.
   **Blocked by:** Task 1, 2, 3
 
 - [x] **Task 5:** Build verification
   **File:** `build-x64.bat`
   **Command:** `cmd /c build-x64.bat`
   **Expected:** Zero warnings from changed code under MSVC W4.
   **Blocked by:** Task 1, 2, 3, 4
---

## Files to Modify

| File | Change |
|------|--------|
| `src/NetBox/FarDialog.cpp` | Fix `SetWidth()` lines 1589, 1594; fix `SetHeight()` lines 1609, 1614; remove `+1` compensation in `GetWidth()` and `GetHeight()` |

---

## Risk Assessment

| Risk | Level | Mitigation |
|------|-------|------------|
| Text overflow in dialogs with minimal margins | Low | The correction adds exactly 1 character width — most dialogs have at least 1 character of padding. If overflow occurs, visible in Far Manager test. |
| Regression in negative-coordinate layouts | Low | Both positive and negative branches are fixed symmetrically. Negative coordinates use `Right` as anchor, same arithmetic correction. |
   All callers of SetWidth/SetHeight already compensate | Low | The `-1` was an implementation bug; callers pass logical widths (e.g., `::StripHotkey(Value).GetLength()` at line 1825). No caller workaround expected. |
   Accidentally touching dialog-level `+1` at lines 174, 235 | Low | Those compensate for Far Manager's inclusive border coordinates (dialog border box) and are CORRECT — do not modify. |

---

## Suggested Commit Message

```
fix(dialog): correct off-by-one in SetWidth/SetHeight bounds calculation (#387)

TFarDialogItem::SetWidth() and SetHeight() subtracted one character
from text field widths due to an incorrect -1/+1 adjustment for
positive and negative coordinates. This caused text truncation in
dialogs (e.g., "Putty path" rendered as "Putty pat").

Remove the off-by-one adjustments and the compensating +1 in GetWidth()
and GetHeight(). Fixes issue #387, based on patch by @yulian5.

Refs https://github.com/michaellukashov/Far-NetBox/issues/387
Refs https://github.com/FarGroup/Far-NetBox/issues/25
```

---

## References

- [FarGroup/Far-NetBox#25](https://github.com/FarGroup/Far-NetBox/issues/25) — original bug report by @BestiaPL
- [231219.NetBox.patch](https://github.com/FarGroup/Far-NetBox/files/13721264/231219.NetBox.patch) — fix by @yulian5
- [michaellukashov/Far-NetBox#387](https://github.com/michaellukashov/Far-NetBox/issues/387) — tracking issue