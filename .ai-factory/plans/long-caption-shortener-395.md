# Implementation Plan: Long Caption Shortener

Branch: none
Created: 2026-05-10

## Settings
- Testing: no
- Logging: verbose
- Docs: yes

## GitHub Reference
[#395](https://github.com/michaellukashov/Far-NetBox/issues/395) — Localization text alignment

## Problem

Dialog labels with long localized text overflow their label width in Far Manager dialogs.
Example: "TLS client certificate (PEM)" should become "TLS client cert.. (PEM)" when
the label width is insufficient. Currently there is no reusable utility for this — the
only truncation in the codebase is:

1. `RightCutToLength()` in `Sysutils` — truncates from the right with a Unicode ellipsis `\u2026`.
   It keeps the left portion (e.g. `"Hello World!"` → `"Hello …"`) but does not preserve
   parenthesized suffixes and uses an ellipsis that renders poorly in Far Manager text-mode cells.
2. Ad-hoc truncation in `TPasswordDialog::GenerateLabel()` (WinSCPDialogs.cpp:2885) —
   uses `SetLength` + `" ..."` but doesn't preserve parenthesized suffixes.

## Design

### Function: `CutToLength`

Add to `Sysutils` namespace (next to existing `RightCutToLength`):

```cpp
```cpp
NB_CORE_EXPORT UnicodeString CutToLength(
    const UnicodeString & Str,
    int32_t MaxLength,
    const UnicodeString & Ellipsis = L"\u2026");
```

**Behavior:**
1. If `Str.Length() <= MaxLength`, return `Str` unchanged.
2. If `MaxLength <= 0`, return empty string.
3. If the string contains a parenthesized suffix, e.g. `(PEM)`, preserve it:
   - Split: `"TLS client certificate (PEM)"` → body=`"TLS client certificate"`, suffix=`" (PEM)"`
   - Shorten the body: body becomes `"TLS client cert.."` (using ellipsis)
   - Recombine: `"TLS client cert.. (PEM)"`
   - Suffix counts toward `MaxLength`; if even suffix alone exceeds `MaxLength`, fall back to plain truncation from the right.
4. If no parenthesized suffix, truncate from the right and append `Ellipsis`:
   - `"A very long label"` → `"A very long .."` (length = MaxLength)

**Suffix detection:** Find the **last** `(` that has a matching `)` at the end of the string
(trailing spaces allowed). This matches common patterns like `(PEM)`, `(TCP)`, `(IPv4)`.

**Ellipsis default:** `L"\u2026"` (Unicode ellipsis) — consistent with `RightCutToLength`
and standard UI conventions for truncated text.


### Key Decisions

|Decision|Rationale|
|---|---|
|Default ellipsis = `L"\u2026"` (Unicode ellipsis)|Consistent with sister function `RightCutToLength`; standard UI convention for truncated text|
|Preserve `(suffix)` pattern|Most long labels in NetBox have parenthesized qualifiers like `(PEM)`, `(TCP)`, `(SSH)` that are critical for user comprehension|
|Suffix detection: last `(`...`)` at end|Covers all existing label patterns without over-engineering|
|Fallback to plain truncation|When suffix alone exceeds MaxLength, plain truncation is the only option|
|Name `CutToLength` (not `ShortenLabel`)|User requested a "shortener" utility; `CutToLength` mirrors sister function `RightCutToLength` for discoverability and follows existing naming conventions in the namespace|
|Callers must strip hotkeys before calling|`&` hotkey markers count toward `Length()` but are invisible on-screen; passing pre-stripped strings ensures accurate truncation width

## Tasks

### Phase 1: Implementation

 - [x] Task 1: Add `CutToLength` declaration to `src/base/Sysutils.hpp`
  - Add after the existing `RightCutToLength` declaration (line 291)
  - Signature: `NB_CORE_EXPORT UnicodeString CutToLength(const UnicodeString & Str, int32_t MaxLength, const UnicodeString & Ellipsis = L"\u2026");`

 - [x] Task 2: Implement `CutToLength` in `src/base/Sysutils.cpp`
  - Add after `RightCutToLength` implementation (after line 238)
  - Logic:
    1. Guard: MaxLength <= 0 → return ""
    2. If Str.Length() <= MaxLength → return Str
    3. Detect parenthesized suffix: scan from end for `)` then back for `(`
    4. If suffix found and suffix.Length() < MaxLength:
       - body = Str without suffix
       - shorten body to (MaxLength - suffix.Length() - Ellipsis.Length())
       - return shortened_body + Ellipsis + suffix
    5. Else: plain right-truncation with Ellipsis
  - Verbose logging: DEBUG_PRINTF when truncation occurs
  - IMPORTANT: UnicodeString is 1-indexed (VCL/BCB6 convention). All Pos(), SubString(), Delete(), Replace() use 1-based positions. Never pass 0 as position — it throws ThrowIfOutOfRange. Use SubStr(1, n) not SubStr(0, n).

 - [x] Task 3: Refactor `TPasswordDialog::GenerateLabel` to use `CutToLength`
  - File: `src/NetBox/WinSCPDialogs.cpp` (lines 2885-2905)
  - Replace the ad-hoc `if (GetSize().x - 10 < Caption.Length())` block with:
    `Caption = ::Sysutils::CutToLength(::StripHotkey(ACaption), GetSize().x - 10);`
    (StripHotkey is called before CutToLength so invisible `&` markers don't inflate Length())
  - Add `#include <Sysutils.hpp>` if not already present (confirmed: NOT included in WinSCPDialogs.cpp)
  - Set `Truncated = Truncated || (Caption != ACaption);` — preserves the existing behavior
    where `Truncated` is `true` when any label in the prompt was shortened
  - Keep the existing `FPrompt += Caption` accumulator pattern unchanged

### Phase 2: Documentation

 - [x] Task 4: Update docs/architecture.md or contributing.md with the new utility
  - Brief note: `CutToLength` added to Sysutils for label text shortening with suffix preservation

## Commit Plan
- **Commit 1** (after tasks 1-3): "feat: add CutToLength label shortener to Sysutils"
- **Commit 2** (after task 4): "docs: document CutToLength utility"

## Edge Cases

|Case|Expected Result|
|---|---|
|Empty string|Return empty|
|String shorter than MaxLength|Return unchanged|
|No parenthesized suffix|Plain right-truncation with ellipsis|
|Suffix alone exceeds MaxLength|Plain right-truncation (no suffix preservation)|
|Multiple parenthesized groups `(A) (B)`|Preserve only the last `(B)` as suffix|
|Hotkey `&` in label|`CutToLength` does NOT strip hotkeys internally. Callers must call `::StripHotkey()` first — otherwise invisible `&` chars inflate `.Length()` and the truncated result will be shorter than expected on-screen|
|MaxLength = ellipsis length|Return just the ellipsis (degenerate case)|
|Ellipsis changed from `" ..."` to `L"\u2026"`|Intentional: aligns with `RightCutToLength` default. Visual appearance differs from original ad-hoc code (no leading space, Unicode ellipsis vs 3 chars) — acceptable for text-mode Far Manager dialogs|
