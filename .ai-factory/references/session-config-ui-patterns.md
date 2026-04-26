# UI Patterns for Session Configuration Exploration

**Date:** 2026-04-26
**Updated:** 2026-04-26 (deep codebase analysis — aif-improve)
**Topic:** Session Configuration UI Patterns for OpenSSH Certificate Integration
**Related Plan:** [openssh-certificate-auth.md](../plans/openssh-certificate-auth.md)
**Architecture:** [ARCHITECTURE.md](../ARCHITECTURE.md)
**SSH Auth Reference:** [ssh-authentication-exploration.md](./ssh-authentication-exploration.md)

## Dialog Architecture

### Class Hierarchy

```
TFarDialog (base dialog class)
    ↓
TTabbedDialog (multi-tab dialog support)
    ↓
TSessionDialog (session configuration - tabAuthentication for auth settings)
```

### Key Files

| File | Purpose |
|------|---------|
| `src/NetBox/WinSCPDialogs.cpp` | `TSessionDialog` implementation; controls for all tabs; data binding in `Execute()` after modal result |
| `src/NetBox/FarDialog.h` | Base classes: `TFarDialog`, `TTabbedDialog`, and control types (`TFarEdit`, `TFarCheckBox`, `TFarComboBox`, etc.) |
| `src/core/SessionData.h` | `TSessionData` with fields: `PublicKeyFile`, `Passphrase`, `DetachedCertificate`, `TlsCertificateFile` and property accessors |
| `src/core/SessionData.cpp` | `Load`/`Save` methods using storage macros and `LOAD_PASSWORD`/`SAVE_PASSWORD` for credentials |
| `src/NetBox/FarConfiguration.cpp` | `TFarConfiguration` creating `TFar3Storage` for registry-based storage |
| `src/windows/GUIConfiguration.h` | `TGUIConfiguration` base class for GUI settings including session options |
| `src/NetBox/XmlStorage.cpp` | `TXmlStorage` for XML-based persistence (import/export) |

## Control Types

### Available Controls (from FarDialog.h)

| Control Type | Usage |
|-------------|-------|
| `TFarEdit` | Single-line text input |
| `TFarCheckBox` | Boolean toggle |
| `TFarComboBox` | Dropdown selection |
| `TFarButton` | Action button (e.g., file browse) |
| `TFarRadioButton` | Radio button group |
| `TFarText` | Static label |
| `TFarSeparator` | Visual separator/group divider |

## Tab UI Patterns

### General Pattern

- **Dialog system:** VCL-like via `TFarDialog`/`TTabbedDialog`
- **Control creation:** `MakeOwnedObject<TFar...>(Owner)` pattern
- **Positioning:** `SetNextItemPosition(ipNewLine)`, `SetNextItemPosition(ipRight)`, `SetNextItemPosition(ipBelow)` plus explicit `Left`/`Right`/`Width`
- **Tab grouping:** `SetDefaultGroup(tabX)` assigns controls to a specific tab
- **Data binding:** Two-phase — `Init()` builds UI; `Execute()` reads/writes after modal result via `GetText()`/`SetText()` and property setters

### tabAuthentication — Detailed Structure

The authentication tab (`tabAuthentication`) in `TSessionDialog` contains **only checkboxes** separated by `TFarSeparator` groups:

| Control | Type | Purpose |
|---------|------|---------|
| `SshNoUserAuthCheck` | `TFarCheckBox` | Skip SSH userauth |
| `TryAgentCheck` | `TFarCheckBox` | Try Pageant/SSH agent |
| `AuthKICheck` | `TFarCheckBox` | Keyboard-interactive auth |
| `AuthKIPasswordCheck` | `TFarCheckBox` | KI with password fallback |
| `AgentFwdCheck` | `TFarCheckBox` | SSH agent forwarding |
| `AuthGSSAPICheck3` | `TFarCheckBox` | GSSAPI authentication |
| `GSSAPIFwdTGTCheck` | `TFarCheckBox` | GSSAPI credential forwarding |

**CRITICAL FINDING:** No file path controls exist on `tabAuthentication`. All controls are checkboxes with `TFarSeparator` group dividers.

**Available space:** There is space below the GSSAPI group (before a final `MakeOwnedObject<TFarSeparator>()` and the Bugs tab boundary) for new controls.

### Session Tab — Key File Pattern

| Control | Type | Pattern |
|---------|------|---------|
| `PrivateKeyEdit` | `TFarEdit` | Simple text field, **no browse button** |
| `TunnelPrivateKeyEdit` | `TFarEdit` | Simple text field, **no browse button** |

Both are simple text input fields. Users type or paste the path manually. No file selection dialog is provided.

### S3 Tab — Certificate Pattern

| Control | Type | Pattern |
|---------|------|---------|
| `S3CACertificateEdit` | `TFarEdit` | Text field with width 30, **no browse button** |

Despite initial assumptions, the S3 CA certificate control also uses a simple `TFarEdit` without a browse button. Users enter the file path directly.

### FTP Tab — TLS Certificate Pattern

| Control | Type | Pattern |
|---------|------|---------|
| `TlsCertificateFileEdit` | `TFarEdit` | Text field (marked TODO in code), **no browse button** |

Same pattern — simple text input for file path.

### Environment Tab — Path Pattern

| Control | Type | Pattern |
|---------|------|---------|
| `RecycleBinPathEdit` | `TFarEdit` | Text field, **no browse button** |

Consistent with all other file path controls in the dialog.

### CORRECTED: No Browse Buttons in WinSCPDialogs.cpp

**All file path controls in `WinSCPDialogs.cpp` use simple `TFarEdit` without browse buttons.** This is the established pattern across all tabs:

- Session tab: `PrivateKeyEdit`, `TunnelPrivateKeyEdit`
- S3 tab: `S3CACertificateEdit`
- FTP tab: `TlsCertificateFileEdit`
- Environment tab: `RecycleBinPathEdit`

**Recommendation:** Follow this established pattern for OpenSSH certificate controls — use `TFarEdit` without browse buttons. Users enter paths manually or paste them.

## Existing Certificate UI Patterns

### S3 CA Certificate Control (S3 Tab)

- Pattern: Simple `TFarEdit` for certificate file path
- Stores path in `FS3CACertificate` field of `TSessionData`
- No validation beyond non-empty check

### TLS Certificate File Control (FTP Tab)

- Pattern: Simple `TFarEdit` (marked as TODO in code)
- Stores path in `FTlsCertificateFile` field of `TSessionData`
- Can be used as template for OpenSSH certificate controls

## Extension Pattern for OpenSSH Certificate Controls

### 1. Add Controls to Dialog (tabAuthentication)

```cpp
// In TSessionDialog::Init() for tabAuthentication
// Below the GSSAPI group, before the final separator

// Checkbox
FUseOpensshCertCheck = MakeOwnedObject<TFarCheckBox>(this);
FUseOpensshCertCheck->SetNextItemPosition(ipNewLine);
FUseOpensshCertCheck->SetText(L"Use OpenSSH certificate authentication");
FUseOpensshCertCheck->SetDefaultGroup(tabAuthentication);

// Certificate file label + edit
FOpensshCertLabel = MakeOwnedObject<TFarText>(this);
FOpensshCertLabel->SetNextItemPosition(ipNewLine);
FOpensshCertLabel->SetText(L"Certificate file:");
FOpensshCertLabel->SetDefaultGroup(tabAuthentication);

FOpensshCertEdit = MakeOwnedObject<TFarEdit>(this);
FOpensshCertEdit->SetNextItemPosition(ipRight);
FOpensshCertEdit->SetWidth(30);
FOpensshCertEdit->SetDefaultGroup(tabAuthentication);

// Private key file label + edit
FOpensshKeyLabel = MakeOwnedObject<TFarText>(this);
FOpensshKeyLabel->SetNextItemPosition(ipNewLine);
FOpensshKeyLabel->SetText(L"Private key file (OpenSSH format):");
FOpensshKeyLabel->SetDefaultGroup(tabAuthentication);

FOpensshKeyEdit = MakeOwnedObject<TFarEdit>(this);
FOpensshKeyEdit->SetNextItemPosition(ipRight);
FOpensshKeyEdit->SetWidth(30);
FOpensshKeyEdit->SetDefaultGroup(tabAuthentication);
```

### 2. Bind to Session Data in Execute() Handler

```cpp
// In TSessionDialog::Execute() after modal result (IDOK)
// Reading values from controls:
SessionData->SetUseOpensshCertificate(FUseOpensshCertCheck->GetChecked());
SessionData->SetDetachedCertificate(FOpensshCertEdit->GetText());
SessionData->SetOpensshPrivateKeyFile(FOpensshKeyEdit->GetText());

// In Init() for dialog population:
// Writing values to controls:
FOpensshCertEdit->SetText(SessionData->DetachedCertificate);
FOpensshKeyEdit->SetText(SessionData->GetOpensshPrivateKeyFile());
FUseOpensshCertCheck->SetChecked(SessionData->GetUseOpensshCertificate());
```

### 3. Enable/Disable Controls Based on Checkbox

```cpp
// In dialog state update or after checkbox change
bool UseCert = FUseOpensshCertCheck->GetChecked();
FOpensshCertEdit->SetEnabled(UseCert);
FOpensshKeyEdit->SetEnabled(UseCert);
// Also disable labels if desired
FOpensshCertLabel->SetEnabled(UseCert);
FOpensshKeyLabel->SetEnabled(UseCert);
```

## Storage Serialization

### Registry Storage (TFar3Storage)

- Uses `Storage->WriteString()`/`Storage->ReadString()` for paths
- Passwords use `LOAD_PASSWORD`/`SAVE_PASSWORD` macros for encryption
- Fields are written with explicit tags (e.g., `OpensshCertificate`, `OpensshPrivateKeyFile`)

### XML Storage (TXmlStorage)

- Same field names as registry storage
- Supports import/export of session configurations
- Backward compatibility: missing fields default to empty/disabled

## Validation Patterns

### Key File Validation

- Existing `VerifyKey()` function checks private key format
- Can be extended to validate OpenSSH certificate format
- Validation occurs before connection attempt
- **Note:** `VerifyKey()` call in `CloseQuery()` is currently commented out

### Error Messages

- Missing file: "Certificate file not found: <path>"
- Invalid format: "Invalid certificate format. Expected OpenSSH certificate (.pem)"
- Expired certificate: "Certificate expired on <date>"
- Use `MessageDlg()` for error display (matches existing patterns)

## UI State Management

### Dialog State

- Controls are enabled/disabled based on checkbox state
- Apply button validates required fields before enabling
- Unsaved password warnings use existing `CloseQuery()` pattern

### Session State

- `TSessionData::GetCanLogin()` should check certificate auth requirements
- `TSessionData::GetCanOpen()` should validate certificate paths if enabled

## Extension Checklist

- [ ] Add checkbox for "Use OpenSSH certificate authentication" to `tabAuthentication`
- [ ] Add simple `TFarEdit` for certificate file path (use existing `FDetachedCertificate`)
- [ ] Add simple `TFarEdit` for OpenSSH private key file path
- [ ] Use `TFarText` labels for both fields (matches existing patterns)
- [ ] Wire controls to enable/disable based on checkbox state
- [ ] Bind to `TSessionData` properties in `Execute()` handler
- [ ] Add validation for missing/invalid files
- [ ] Ensure storage serialization includes new fields
- [ ] Test with both registry and XML storage backends
- [ ] Verify backward compatibility with existing sessions

## Related Issues

- [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — User-provided auth certificate
- [#36](https://github.com/michaellukashov/Far-NetBox/issues/36) — Support for Unix-style SSH keys as well as PuTTY-style keys
