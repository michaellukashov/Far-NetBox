# Reference: NetBox UI Dialogs for S3 Configuration

**Date:** 2026-04-26
**Source:** Exploration via `subagent_type: Explore` (task-3)
**Files Analyzed:**
- `src/NetBox/WinSCPDialogs.cpp` (~9400 lines)
- `src/NetBox/WinSCPDialogs.h`
- `src/NetBox/FarDialog.cpp/h`
- `src/core/SessionData.h` (for data model)

**Related:**
- Architecture: `.ai-factory/ARCHITECTURE.md`
- Plan: `.ai-factory/plans/s3-encryption-options-winscp-alignment.md`

---

## 1. Dialog Architecture Overview

NetBox does **not** use VCL (WinSCP's C++ Builder UI). Instead it uses:
- `TFarDialog` — base class for Far Manager dialogs
- `TFarEdit` — single-line text edit
- `TFarButton` — push button
- `TFarCheckBox` — check box
- `TFarComboBox` — drop-down list
- `TTabbedDialog` — tab container

**All dialogs are defined in `src/NetBox/WinSCPDialogs.cpp`.** This file contains the `TSessionDialog` class handling the login/session configuration UI for all protocols.

---

## 2. Current S3 Tab Layout

### 2.1 Tab Existence

Yes — there is a dedicated S3 tab (`tabS3`) within `TSessionDialog`.

### 2.2 Controls on S3 Tab (per exploration)

|Control|Variable Name|Purpose|
|---|---|---|
|Region combo|`S3DefaultRegionCombo`|Default AWS region|
|URL style combo|`S3UrlStyleCombo`|VirtualHost vs Path style|
|Requester Pays checkbox|`S3RequesterPaysCheck`|Enable requester pays|
|CA certificate edit|`S3CACertificateEdit`|PEM content (text field only, **no buttons**)|


## 3. Existing Encryption-Related UI Patterns

### 3.1 FTP Encryption Combo

On the FTP tab (not S3), there is:

- `FtpEncryptionCombo` — options: Plain FTP, Implicit FTPS, Explicit FTPS/TLS

Pattern:
```cpp
// Population:
FtpEncryptionCombo->Items->Add(LoadStr(FTPENCRYPTION_PLAIN));
FtpEncryptionCombo->Items->Add(LoadStr(FTPENCRYPTION_IMPLICIT));
FtpEncryptionCombo->Items->Add(LoadStr(FTPENCRYPTION_EXPLICIT));
FtpEncryptionCombo->ItemIndex = (int)FSessionData->FtpEncryption;

// Save:
FSessionData->FtpEncryption = (TFtpEncryption)FtpEncryptionCombo->ItemIndex;
```

### 3.2 WebDAV Compression

WebDAV has a compression checkbox only; no TLS selectors because it always uses HTTPS.

---

## 4. Missing TLS Version Controls

NetBox does **not** currently expose `MinTlsVersion`/`MaxTlsVersion` in any login dialog. These fields exist in `TSessionData` and are used by `SetupSsl()`, but the UI for overriding them is absent.

WinSCP exposes these on a **TLS page** shared by all HTTPS-based protocols (S3, WebDAV, FTPS). NetBox chooses to keep protocol-specific tabs, so the appropriate place for S3 TLS controls is **on the S3 tab**.

---

## 5. UI Implementation Pattern to Follow

### 5.1 Adding Combo Boxes

Example from WebDAV/Compression:
```cpp
// Member declaration in WinSCPDialogs.h:
TFarComboBox *WebDAVCompressionCheck;

// In dialog initialization (WinSCPDialogs.cpp, around dialog creation):
WebDAVCompressionCheck = new TFarCheckBox(this, L"&Compression", 10, 10, 50, 10);
// ... position and parent

// Loading from session data:
WebDAVCompressionCheck->Checked = FSessionData->FCompression;

// Saving on OK:
FSessionData->FCompression = CompressionCheck->Checked;
```

### 5.2 Adding Load/Save Buttons for Certificate

No existing Load/Save button pattern exists for certificate editing. Must implement Windows file picker:

```cpp
// Windows API COMDLG32
OPENFILENAME ofn = {0};
ofn.lStructSize = sizeof(ofn);
ofn.hwndOwner = GetActiveWindow();
ofn.lpstrFile = buffer; // wide char buffer
ofn.nMaxFile = MAX_PATH;
ofn.lpstrFilter = L"PEM Files (*.pem)\0*.pem\0All Files (*.*)\0*.*\0";
ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
// GetOpenFileNameW(&ofn);
// GetSaveFileNameW(&ofn);
```

Handler pattern:
```cpp
void TSessionDialog::S3CACertificateLoadClick()
{
  UnicodeString File = ShowOpenFileDialog(L"Open PEM Certificate", L"*.pem");
  if (!File.IsEmpty())
  {
    UnicodeString Content = LoadFileToString(File);
    S3CACertificateEdit->Text = Content;
    // optional: validate PEM header/footer
  }
}
```

---

## 6. Dialog Lifecycle Hooks

- **InitDialog()** — populate controls from `FSessionData`
- **OkClick()** — store control values back to `FSessionData`
- **Visibility toggles** — show/hide protocol-specific controls based on selected protocol (via `UpdateControls()` or similar)
- **Validation** — in `OkClick()` before closing; show error and return `false` to keep dialog open

---

## 7. File Path Resolution

The exploration identified:
- **Dialog implementations:** `src/NetBox/WinSCPDialogs.cpp` and `.h` (single file pair for all session dialogs)
- **Base dialog classes:** `src/NetBox/FarDialog.cpp` and `.h`
- **Session data:** `src/core/SessionData.h`

No separate VCL `.dfm` files exist; layout is created programmatically in `InitDialog()`.

---

## 8. UI Control Placement for S3 Tab

The plan should add TLS version combo boxes **near** existing S3 controls. Coordinates must be determined at implementation time by reading the existing `InitDialog()` S3 tab creation code.

**Suggested grouping:**

```
[ Region: [combo] ]   [ URL Style: [combo] ]
[ CA Certificate: [edit] [Load] [Save] ]
[ Min TLS: [combo]   Max TLS: [combo] ]
[ ] Requester Pays
```

TLS version combos should be placed **below** the CA certificate row to maintain logical flow.

---

## 9. Behavior for TLS Combo Changes

- When user changes Min or Max, validate that `Max >= Min`. If not, show warning: "Maximum TLS version must be greater than or equal to minimum version."
- On dialog load, set combo selection based on `FSessionData->MinTlsVersion` and `MaxTlsVersion`.
- Default indices (if unset): Min = TLS 1.2 (index 2), Max = TLS 1.3 (index 3) — assuming combo items order: TLS 1.0, TLS 1.1, TLS 1.2, TLS 1.3.

---

## 10. PVS-Studio / Warnings Considerations

- Ensure `new` operators for controls are properly parented to dialog (auto-deleted)
- Use `UnicodeString` for all text properties
- Check for `nullptr` before dereferencing controls in handlers
- Allocate buffer for file dialog with `std::vector<wchar_t>` or fixed array on stack

---

## 11. Testing Checklist for UI Changes

- [ ] S3 tab appears when protocol is `fsS3`
- [ ] Min/Max TLS combos are populated with all supported versions
- [ ] Combo selections save to `FSessionData->MinTlsVersion`/`MaxTlsVersion`
- [ ] Invalid range (Max < Min) shows error and prevents OK
- [ ] Load button opens file picker, populates edit with file content
- [ ] Save button writes edit content to selected file
- [ ] Session persistence round-trip works for TLS settings and CA certificate
- [ ] Backward compatibility: sessions without TLS overrides load with defaults

---

## 12. Reference Code Locations (NetBox)

|File|What to Look For|Approx Line|
|---|---|---|
|`WinSCPDialogs.cpp`|S3 tab initialization|Search `tabS3`|
|`WinSCPDialogs.cpp`|Region combo creation|`S3DefaultRegionCombo`|
|`WinSCPDialogs.cpp`|S3CACertificateEdit declaration|`S3CACertificateEdit`|
|`WinSCPDialogs.cpp`|Dialog OK handler|`TSessionDialog::OkClick()`|
|`WinSCPDialogs.cpp`|FTP encryption combo|`FtpEncryptionCombo`|
|`WinSCPDialogs.h`|Member variables|`TSessionDialog::FtpEncryptionCombo`, etc.|

**Use `grep` to locate quickly:**
```bash
rg "S3DefaultRegionCombo" src/NetBox/
rg "tabS3" src/NetBox/WinSCPDialogs.cpp
rg "FtpEncryptionCombo" src/NetBox/
```

---

## 13. Consistency with WinSCP Patterns

- **Control types:** WinSCP uses VCL `TComboBox`, `TCheckBox`, `TEdit`; NetBox uses `TFarComboBox`, `TFarCheckBox`, `TFarEdit` → **different classes, same purpose**
- **Property binding:** Direct assignment in initialization and OK handler (not two-way data binding)
- **Validation:** NetBox does some validation; add TLS range validation following same style (show error, `ModalResult = 0`)

---

*End of Reference*
