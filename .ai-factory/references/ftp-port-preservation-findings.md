# FTP Port Preservation Fix - Verification Findings

## Task 1 Findings: Port Persistence Mechanisms

**Status: PASS**

### Verified Components

1. **FPortNumber Member Variable**
   - File: `src/core/SessionData.h` line 163
   - `int32_t FPortNumber{0};` - initialized to 0 but set via SetPortNumber

2. **GetPortNumber() Method**
   - File: `src/core/SessionData.h` line 948
   - Inline: `int32_t GetPortNumber() const { return FPortNumber; }`

3. **SetPortNumber() Method**
   - File: `src/core/SessionData.h` line 326 (declaration)
   - File: `src/core/SessionData.cpp` lines 3082-3085 (implementation)
   - Uses SET_SESSION_PROPERTY macro for modification tracking

4. **GetDefaultPort() Method**
   - File: `src/core/SessionData.cpp` lines 2996-2999
   - Returns protocol-appropriate default based on FSProtocol and Ftps

5. **Default Port Constants** (SessionData.cpp lines 70-74)
   - `SshPortNumber = 22`
   - `FtpPortNumber = 21`
   - `FtpsImplicitPortNumber = 990`
   - `HTTPPortNumber = 80`
   - `HTTPSPortNumber = 443`

6. **DefaultPort() Function** (SessionData.cpp lines 6598-6639)
   - Returns appropriate default for each protocol

### Conclusion
Port persistence mechanisms are correctly implemented.

**Next Action:** Proceed to Task 2 - Trace dialog initialization code path.

---

## Task 2 Findings: Dialog Initialization Code Path

**Status: PASS**

### Verified Components in TSessionDialog::Execute()

**File: src/NetBox/WinSCPDialogs.cpp**

1. **PortNumberEdit Population** (line 3502)
   ```cpp
   PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());
   ```
   - Port is loaded directly from session data before any combo box initialization

2. **TransferProtocolCombo Initialization** (lines 3508-3509)
   ```cpp
   TransferProtocolCombo->SetItemIndex(
       nb::ToInt32(FSProtocolToIndex(SessionData->GetFSProtocol(), AllowScpFallback)));
   ```

3. **FtpEncryptionCombo Initialization** (lines 3511-3512)
   ```cpp
   FtpEncryptionCombo->SetItemIndex(
       nb::ToInt32(FtpsToIndex(SessionData->GetFtps())));
   ```

4. **Index Caching** (lines 3515-3516)
   ```cpp
   FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();
   FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();
   ```
   - Caching happens AFTER combo boxes are initialized with saved values

**TMP-LOG Added:**
- Line 3501: Port loading trace
- Line 3517: Index caching trace

**Next Action:** Proceed to Task 3 - Analyze Far dialog message flow for DN_EDITCHANGE.

---

## Task 3 Findings: Analyze Far Dialog Message Flow

**Status: PASS**

### Verified Components

**1. TFarComboBox::ItemProc handles DN_EDITCHANGE** (`src/NetBox/FarDialog.cpp` lines 2612-2627)
```cpp
intptr_t TFarComboBox::ItemProc(intptr_t Msg, void * Param)
{
  if (Msg == DN_EDITCHANGE)
  {
    // ...
    FItemChanged = true;  // <-- Sets flag on spurious message
  }
  // ...
}
```

**2. GetSetChanged() resets flag and returns old value** (FarDialog.cpp line 679)
```cpp
bool GetSetChanged(bool Value) { const bool OldValue = FItemChanged; FItemChanged = Value; return OldValue; }
```

**3. TSessionDialog::Change() uses GetSetChanged()** (WinSCPDialogs.cpp lines 3160-3170)
```cpp
if (TransferProtocolCombo->GetSetChanged(false))
{
  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();  // Always updates cache
  DoChange = true;
}
if (FtpEncryptionCombo->GetSetChanged(false))
{
  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex(); // Always updates cache
  DoChange = true;
}
if (DoChange)
{
  TransferProtocolComboChange();  // Called even when indices unchanged!
}
```

**4. TransferProtocolComboChange() resets port** (WinSCPDialogs.cpp lines 3230-3263)
- Reads current port from PortNumberEdit
- If port doesn't match new protocol, resets to protocol default
- FTP with ftpsNone: sets port to 21 (line 3239)

### Critical Findings

**CONFIRMED:** During dialog initialization:
1. `ShowModal()` -> `DialogInit()` sends spurious `DN_EDITCHANGE`
2. `TFarComboBox::ItemProc` sets `FItemChanged = true`
3. `TSessionDialog::Change()` -> `GetSetChanged(false)` returns `true`
4. `TransferProtocolComboChange()` called
5. Port reset from 443 -> 21

**Fix Strategy Validated:** Approach A (Index comparison guards) will work.

**Next Action:** Proceed to Task 4 - Implement index comparison guards.

---

## Task 4 Findings: Implement Index Comparison Guards

**Status: PASS**

### Implementation

**File: src/NetBox/WinSCPDialogs.cpp**

Modified `TSessionDialog::Change()` to prevent spurious port resets:

**Before:**
```cpp
if (TransferProtocolCombo->GetSetChanged(false))
{
  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();
  DoChange = true;
}
if (FtpEncryptionCombo->GetSetChanged(false))
{
  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();
  DoChange = true;
}
```

**After:**
```cpp
const int32_t NewProtocolIndex = TransferProtocolCombo->GetItemIndex();
if (TransferProtocolCombo->GetSetChanged(false) && NewProtocolIndex != FTransferProtocolIndex)
{
  FTransferProtocolIndex = NewProtocolIndex;
  DoChange = true;
}
const int32_t NewEncryptionIndex = FtpEncryptionCombo->GetItemIndex();
if (FtpEncryptionCombo->GetSetChanged(false) && NewEncryptionIndex != FFtpEncryptionComboIndex)
{
  FFtpEncryptionComboIndex = NewEncryptionIndex;
  DoChange = true;
}
```

### How It Works

1. Capture current combo box index BEFORE checking `GetSetChanged()`
2. Only update cached index and trigger `TransferProtocolComboChange()` if:
   - `GetSetChanged()` returns true (flag was set)
   - AND the index has actually changed from cached value

This suppresses spurious resets during dialog initialization while preserving the user-initiated protocol change behavior.

**TMP-LOG:** Added trace on FFtpEncryptionComboIndex change for debugging.

**Next Action:** Proceed to Task 7 - Verify Proxy Method combo doesn't have similar issue.

---

## Task 7 Findings: Verify Proxy Method Combo

**Status: PASS - No changes required**

### Analysis

**Proxy Method Combo Block** (WinSCPDialogs.cpp lines 3156-3158):
```cpp
if (GetProxyMethodCombo()->GetSetChanged(false))
{
  FProxyComboIndex = GetProxyMethodCombo()->GetItemIndex();
}
```

### Findings

1. **No `DoChange` flag set** - Proxy combo changes don't trigger `TransferProtocolComboChange()`
2. **No side effects** - Only updates cached index `FProxyComboIndex`
3. **`FProxyComboIndex` usage**:
   - Used in `UpdateControls()` for UI state (line 3347)
   - Used in `TransferProtocolComboChange()` for logic (line 3439)
   - Never modifies PortNumberEdit, TransferProtocolCombo, or FtpEncryptionCombo

### Conclusion

Proxy method combo does NOT need the same fix. The pattern is different:
- Transfer/FTP encryption combos: update index + set DoChange + call side-effect function
- Proxy combo: update index only

No fix needed for proxy method combo.

**Next Action:** Proceed to Task 9 - Investigate S3 protocol port adjustment bug.

---

## Task 9 Findings: Investigate S3 Protocol Port Adjustment Bug

**Status: CONFIRMED - Bug exists, deferred**

### Bug Confirmed

**Location:** `src/NetBox/WinSCPDialogs.cpp` lines 3270-3282

```cpp
else if (FSProtocol == fsS3)
{
  if (Port == HTTPSPortNumber)
  {
    PortNumberEdit->SetAsInteger(HTTPSPortNumber);  // <-- NO-OP!
    // ... hostname handling
  }
}
```

### Issue

When switching from FTP (port 21) to S3:
1. `Port` = 21 (from FTP default)
2. `HTTPSPortNumber` = 443
3. Condition `Port == HTTPSPortNumber` is FALSE
4. Port remains 21 instead of being changed to 443

The fix should be: `if (Port != HTTPSPortNumber)` to set the default S3 port.

### Decision

**DEFERRED** - This is a separate bug from the FTP port preservation issue.
- The current fix preserves existing behavior
- S3 port adjustment bug is a pre-existing issue
- Should be fixed in a separate commit or follow-up issue

**Next Action:** Proceed to Task 5 - Cross-platform build verification.

---

## Task 5 Findings: Cross-Platform Build Verification

**Status: SKIP - No Windows build environment available**

### Environment

Linux kernel 6.19.13 - No Wine or Windows emulation available.
MSVC 2022 + Ninja build required per AGENTS.md specs.

### Build Script Review

**Scripts available:**
- `build-x64.bat` - x64 RelWithDebugInfo
- `build-x86.bat` - x86 RelWithDebugInfo  
- `build-arm64.bat` - ARM64 (optional)

### Manual Build Required

**Cannot be automated** - Requires Windows environment with Visual Studio 2022.
Human tester must verify build after applying fix.

**Next Action:** Proceed to Task 10 - Document TFullSynchronizeDialog bug pattern.

---

## Task 10 Findings: TFullSynchronizeDialog Initialization Pattern

**Status: CONFIRMED - Similar pattern exists**

### Analysis

**File:** `src/NetBox/WinSCPDialogs.cpp` lines 7553-7596

### Findings

`TFullSynchronizeDialog::Change()` forcibly sets check states during initialization:

```cpp
if (GetHandle())
{
  if (SynchronizeTimestampsButton->GetChecked())
  {
    SynchronizeExistingOnlyCheck->SetChecked(true);  // <-- forces state
    SynchronizeDeleteCheck->SetChecked(false);        // <-- forces state
    SynchronizeByTimeCheck->SetChecked(true);         // <-- forces state
  }
  if (SynchronizeBothButton->GetChecked())
  {
    // ... forces more states
  }
  // ... more unconditional state changes
}
```

### Risk Assessment

**Risk exists** - Similar to TSessionDialog bug:
- `SetChecked()` calls during dialog initialization
- `TFullSynchronizeDialog` uses `TWinSCPDialog::Change()` pattern
- No obvious `ChangesLocked()` guard around this logic
- Could override user preferences if spurious change events fire

### Recommendation

This should be documented as a **follow-up issue** for potential similar bugs in other dialogs.

---

## Summary

| Task | Status | Notes |
|------|--------|-------|
| Task 1 | PASS | Port persistence verified |
| Task 2 | PASS | Dialog init path documented |
| Task 3 | PASS | DN_EDITCHANGE flow confirmed |
| Task 4 | PASS | Index guards implemented |
| Task 5 | SKIP | No Windows build env |
| Task 6 | HUMAN | Requires manual testing |
| Task 7 | PASS | No proxy fix needed |
| Task 8 | TODO | Cleanup logging (move to cleanup) |
| Task 9 | DEFERRED | S3 bug confirmed, separate issue |
| Task 10 | CONFIRMED | Similar pattern in TFullSynchronizeDialog |

### Build Status

**x64/x86 builds cannot be verified** - Linux environment, no Windows/MSVC.
Human tester must build and verify before commit.

### Implementation Complete

**File modified:** `src/NetBox/WinSCPDialogs.cpp` (1 file only)
**Lines changed:** Index comparison guards added in `TSessionDialog::Change()`

---

## Task 8 Findings: Cleanup Temporary Debug Logging

**Status: PASS**

### Removed Lines

1. **Line 3169 (approx):** `// TMP-LOG: DEBUG_PRINTF("FFtpEncryptionComboIndex changed: %d", ...)`
2. **Line 3501 (approx):** `// TMP-LOG: DEBUG_PRINTF("PortNumberEdit loaded: %d", ...)`  
3. **Line 3516 (approx):** `// TMP-LOG: DEBUG_PRINTF("FTransferProtocolIndex cached: %d, ...)`

### Verification

```bash
$ grep -r "// TMP-LOG:" src/
(No matches found)

$ ls tmp-logs.txt
File does not exist
```

**All temporary logging removed successfully.**

**Next Action:** Checkpoint 2 commit ready - cleanup complete.
