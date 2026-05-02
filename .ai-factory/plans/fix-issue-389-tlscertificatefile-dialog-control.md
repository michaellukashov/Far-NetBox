# Plan: Add TlsCertificateFile UI Control to Session Dialog (Issue #389 follow-up)

> **GitHub Issue:** [#389](https://github.com/michaellukashov/Far-NetBox/issues/389) (discovered during fix)
> **Created:** 2026-05-02
> **Mode:** fast
> **Settings:** Testing=no, Logging=verbose, Docs=yes
> **References:**
> - WinSCP source pattern: `SiteAdvanced.dfm` / `SiteAdvanced.cpp` (`TlsCertificateFileEdit` as `TFilenameEdit`)
> - NetBox existing pattern: `S3CACertificateEdit` + `S3CACertificateLoadBtn` (`src/NetBox/WinSCPDialogs.cpp`)

## Problem Statement

WinSCP-imported sessions may carry a stale `.ppk` private-key path in `FTlsCertificateFile`. During #389 debugging, this caused `InitOpenssl()` to fail because OpenSSL tried to parse a PuTTY `.ppk` as a PEM certificate.

The root cause: **there is no UI control to view or clear `TlsCertificateFile`**. `WinSCPDialogs.cpp:4040` has:
```cpp
TODO("TlsCertificateFileEdit->GetText()");
// SessionData->SetTlsCertificateFile(PrivateKeyEdit->GetText());
```

The `SessionData` backend already supports the property fully (serialization, command-line, assembly). Only the dialog UI is missing.

## Scope

Add a **TLS client certificate file** edit control with a **browse button** to the session dialog, placed in the FTP tab (`tabFTP` group) below the "Session ID re-use" checkbox. The control should:
- Appear on the FTP tab when any FTPS encryption mode is selected
- Be disabled (grayed out) when encryption is "No encryption"
- Allow the user to type a certificate file path or browse for a `.pem`/`.crt`/`.pfx` file
- Save to / load from `SessionData->TlsCertificateFile`
- Use a simple browse handler that sets the file **path** in the edit (not reading file content)
## Out of Scope

- Certificate validation at dialog level (WinSCP calls `VerifyCertificate()`; skip for first iteration)
- Passphrase input for encrypted certificates
- Certificate preview or details display
- Changes to `libs/` (purely NetBox dialog code)

---

## Tasks

### Phase I. String Resources

#### Task 1: Add message string IDs
- **Files:**
  - `src/base/MsgIDs.h` — append `NB_LOGIN_TLS_CERTIFICATE_FILE` at the **end** of the enum (before the closing `};`), not in the middle of the FTP group. This avoids shifting 900+ subsequent .lng file lines across all 5 language files.
  - `src/NetBox/NetBoxEng.lng` — append `"TLS client &certificate:"` at the end of the file (after the last quoted string).
  - `src/NetBox/NetBoxRus.lng`, `NetBoxPol.lng`, `NetBoxSpa.lng`, `NetBoxFr.lng` — append the English string as a placeholder (same position as NetBoxEng.lng).
- **Note:** The enum index (0-based) must equal the 0-based string position in the .lng file (after stripping the `.Language=` header line). Appending at the end of both keeps them in sync without shifting.
- **Verification:** Build and ensure no `GetMsg` index-out-of-range assertion.

### Phase II. Dialog Controls

#### Task 2: Declare dialog controls in `TSessionDialog` class
- **File:** `src/NetBox/WinSCPDialogs.cpp` (class member declarations, around line 1906 near `SslSessionReuseCheck`)
- Add:
  ```cpp
  TFarText * TlsCertificateFileLabel{nullptr};
  TFarEdit * TlsCertificateFileEdit{nullptr};
  TFarButton * TlsCertificateFileBrowseBtn{nullptr};
  ```
- Add click handler declaration:
  ```cpp
  void TlsCertificateFileBrowseClick(TFarButton * Sender, bool & Close);
  ```

#### Task 3: Create controls in dialog constructor
- **File:** `src/NetBox/WinSCPDialogs.cpp` (control creation, around line 2536 after `SslSessionReuseCheck`)
- The controls are created inside the `tabFTP` group (set at line 2507). Tab-group visibility is handled by the tab system — **no `SetVisible()` calls needed.**
- **Layout:** New line after `SslSessionReuseCheck`:
  ```cpp
  SetNextItemPosition(ipNewLine);
  TlsCertificateFileLabel = MakeOwnedObject<TFarText>(this);
  TlsCertificateFileLabel->SetCaption(GetMsg(NB_LOGIN_TLS_CERTIFICATE_FILE));
  TlsCertificateFileLabel->SetWidth(20);

  SetNextItemPosition(ipRight);
  TlsCertificateFileEdit = MakeOwnedObject<TFarEdit>(this);
  TlsCertificateFileEdit->SetWidth(30);

  SetNextItemPosition(ipRight);
  TlsCertificateFileBrowseBtn = MakeOwnedObject<TFarButton>(this);
  TlsCertificateFileBrowseBtn->SetCaption(L"\u2026");  // ellipsis character
  TlsCertificateFileBrowseBtn->SetOnClick(nb::bind(&TSessionDialog::TlsCertificateFileBrowseClick, this));
  ```

#### Task 4: Implement browse button handler
- **File:** `src/NetBox/WinSCPDialogs.cpp` (new handler, near `S3CACertificateLoadClick` at line ~4813)
- **CRITICAL:** NOT the `S3CACertificateLoadClick` pattern — that handler reads **file content** via `TFile::ReadAllText()`. We store a **file path**, so only set the file name string.
- Use `OPENFILENAMEW` + `GetOpenFileNameW` with filter:
  ```
  Certificate Files (*.pem;*.crt;*.cer;*.pfx;*.p12)\0*.pem;*.crt;*.cer;*.pfx;*.p12\0All Files (*.*)\0*.*\0\0
  ```
- On success: `TlsCertificateFileEdit->SetText(FileName)`. No `ReadAllText`, no PEM validation.
- Keep `Close = false` on error so the dialog stays open.

  ```cpp
  void TSessionDialog::TlsCertificateFileBrowseClick(TFarButton *, bool & Close)
  {
    wchar_t FileName[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFile = FileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Certificate Files (*.pem;*.crt;*.cer;*.pfx;*.p12)\0*.pem;*.crt;*.cer;*.pfx;*.p12\0All Files (*.*)\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn))
    {
      TlsCertificateFileEdit->SetText(FileName);
    }
    Close = false;
  }
  ```
### Phase III. Dialog Logic (Enable / Load / Save)

#### Task 5: Enable control in `UpdateControls()`
- **File:** `src/NetBox/WinSCPDialogs.cpp` (around line 3454)
- Controls are in `tabFTP` group — the tab system handles show/hide. Only `SetEnabled()` is needed, following the same pattern as `SslSessionReuseCheck->SetEnabled(aFtpsProtocol)`.
- Add after `SslSessionReuseCheck->SetEnabled(aFtpsProtocol)` (line 3454):
  ```cpp
  TlsCertificateFileLabel->SetEnabled(aFtpsProtocol);
  TlsCertificateFileEdit->SetEnabled(aFtpsProtocol);
  TlsCertificateFileBrowseBtn->SetEnabled(aFtpsProtocol);
  ```
- When encryption is "No encryption" (`aFtpsProtocol = false`), all three controls are grayed out.

#### Task 6: Load control value from `SessionData`
- **File:** `src/NetBox/WinSCPDialogs.cpp` (around line 3718, after `SslSessionReuseCheck->SetChecked`)
- Add:
  ```cpp
  TlsCertificateFileEdit->SetText(SessionData->GetTlsCertificateFile());
  ```

#### Task 7: Save control value to `SessionData`
- **File:** `src/NetBox/WinSCPDialogs.cpp` (around line 4039, after `SessionData->SetSslSessionReuse`)
- Replace the TODO:
  ```cpp
  SessionData->SetTlsCertificateFile(TlsCertificateFileEdit->GetText());
  ```

### Phase IV. Verification

#### Task 8: Build verification
- Build `RelWithDebugInfo` for x64 using `build-x64.bat`.
- Ensure zero MSVC W4 warnings.

#### Task 9: Manual test scenario
1. Open NetBox session dialog (F11 → NetBox)
2. Create or edit an FTP session
3. Select "TLS/SSL Explicit encryption"
4. Verify the **"TLS client certificate:"** field appears below "Session ID re-use"
5. Type a file path or click **"..."** to browse for a certificate file
6. Save the session
7. Re-open the session and verify the path is preserved
8. Switch protocol to SFTP — verify the field hides
9. Switch back to FTPS — verify the field reappears with the saved path

### Phase V. Documentation

#### Task 10: Update knowledge references
- **File:** `.ai-factory/references/INDEX.md`
- Add entry linking to this plan and a short summary.

#### Task 11: Commit changes
- Use conventional commit message:
  ```
  feat(dialog): add TLS client certificate file UI control

  Add TlsCertificateFileEdit label, edit box, and browse button to the
  session dialog for FTPS/WebDAV/HTTPS/S3 protocols. This allows users to
  view and clear the TlsCertificateFile session property, which was
  previously only accessible via WinSCP import or manual XML editing.

  Fixes the UI gap discovered during issue #389 debugging where stale
  .ppk paths in TlsCertificateFile caused OpenSSL initialization failures.
  ```

---

## Architecture Notes

- **Layer:** `src/NetBox/` — Far Manager plugin UI layer (not FileZilla core)
- **Pattern:** Label + `TFarEdit` + `TFarButton` with `GetOpenFileNameW` for file path selection (NOT `S3CACertificateLoadClick` which reads file content)
- **String resources:** Append new enum value at the end of `MsgIDs.h`; append matching line at the end of each `.lng` file. No mid-file insertion needed.
- **No changes to `SessionData`**: property already fully implemented (getter, setter, serialization, command-line, assembly)
- **Placement:** Inside `tabFTP` group (set at line 2507). The tab system controls show/hide; `UpdateControls()` controls enable/disable via `SetEnabled(aFtpsProtocol)`.

## Edge Cases

1. **Empty certificate file** — clearing the edit sets `TlsCertificateFile` to empty string, which disables client-cert auth (correct behavior)
2. **Non-existent file path** — no validation at dialog level; consumers (`LoadTlsCertificate`, `ParseCertificate`) will fail gracefully at connection time
3. **Protocol switch** — switching to SFTP/SCP/WebDAV shows a different tab; the control is hidden by the tab system. `SessionData` value is preserved.
4. **WinSCP import** — imported sessions with `TlsCertificateFile` will now display the value on the FTP tab instead of silently failing
5. **Plain FTP** — when encryption is "No encryption", the control is grayed out (`SetEnabled(false)`) but still visible on the tab

## Acceptance Criteria

- [ ] `TlsCertificateFile` edit + browse control appears on the FTP tab when an FTPS encryption mode is selected
- [ ] Browse button opens file picker with certificate file filter
- [ ] File path is saved to and loaded from `SessionData->TlsCertificateFile`
- [ ] Control is grayed out when encryption is "No encryption"
- [ ] Build passes with zero warnings
- [ ] All 5 `.lng` language files updated with new string
- [ ] Commit message follows conventional format
