# WinSCP Certificate Editor Patterns — Reference for NetBox Alignment

> **Date:** 2026-05-04
> **Source:** WinSCP master source (`D:/Projects/WinSCP-work/winscp-master/source/`)
> **Purpose:** Understand WinSCP's TLS certificate editing patterns to align NetBox implementation.

## Summary

WinSCP uses a **single `TlsCertificateFile` property** in `TSessionData` for all protocols
(FTPS, WebDAVS, HTTPS, S3). There is **no separate CA certificate editor** for S3 or WebDAV,
and **no per-session CA certificate property**. The certificate editor is a `TFilenameEdit`
(browse-enabled edit control) on the shared TLS/SSL sheet (`TSslSheet`), not embedded in
individual protocol tabs.

## Key Files

| File | Content |
|------|---------|
| `forms/SiteAdvanced.dfm` | TSslSheet with `TlsCertificateFileEdit` (TFilenameEdit), filter `*.pfx;*.p12;*.key;*.pem` |
| `forms/SiteAdvanced.cpp` | Load (line 432), save (line 730), VerifyCertificate call in DoValidate (line 1426), TlsCertificateFileEditAfterDialog (line 1652) |
| `core/SessionData.h` | `FTlsCertificateFile` (line 208), `SetTlsCertificateFile()` (line 394) |
| `core/SessionData.cpp` | Storage I/O (line 935, 1246), `--clientcert` CLI (line 2536), assembly property `TlsClientCertificatePath` (line 4011) |
| `windows/Tools.cpp` | `VerifyCertificate()` (line 1455) — calls `CheckCertificate()`, shows Ignore/Abort dialog |
| `core/Terminal.cpp` | `LoadTlsCertificate()` (line 8736) — parses X509+EVP_PKEY with optional passphrase |

## Certificate Flow

```
TSslSheet.TlsCertificateFileEdit (TFilenameEdit)
  │  Filter: *.pfx;*.p12;*.key;*.pem
  │
  ├─[Browse] → PathEditBeforeDialog/AfterDialog → VerifyCertificate(Name)
  │
  ├─[Load]   → TlsCertificateFileEdit->Text = FSessionData->TlsCertificateFile
  ├─[Save]   → SessionData->TlsCertificateFile = TlsCertificateFileEdit->Text
  │
  └─[Validate] → VerifyCertificate() in DoValidate() before dialog close
                    │
                    └─ CheckCertificate(Path)  → ParseCertificate(Path, "", ...)
                                                  → success: free cert+key
                                                  → fail:    throw Exception
                       │
                       └─ ExceptionMessageDialog(qtWarning, qaIgnore|qaAbort)
                          Ignore → proceed; Abort → cancel dialog close
```

## NetBox Alignment Status

| WinSCP Pattern | NetBox Status | Action |
|---------------|---------------|--------|
| Single `TlsCertificateFile` property | ✅ Exists (`SessionData.h:265`) | None |
| `TlsCertificateFileEdit` control | ✅ Exists on FTP tab (`WinSCPDialogs.cpp:2571`), WebDAV tab (`:3276`) | None |
| Browse handler | ✅ `BrowseForCertificateFile()` (`:4998`) | None |
| Load/save wiring | ✅ Load at `:3878`, save at `:4223` | None |
| Filter `*.pfx;*.p12;*.key;*.pem` | ⚠️ NetBox uses `*.pem;*.crt;*.cer;*.pfx;*.p12;*.key` | Align filter |
| `CheckCertificate()` | ✅ Exists (`Common.cpp:4524`) | Wire into browse handler |
| `VerifyCertificate()` dialog | ❌ Only in unused `src/windows/Tools.cpp` (VCL) | Integrate in browse handler |

## Implementation Plan

1. Align filter: remove `.crt`, `.cer`; match WinSCP's `*.pfx;*.p12;*.key;*.pem`
2. Wire `CheckCertificate()` into `BrowseForCertificateFile()` after `SetText()`
3. Show `MoreMessageDialog(qtWarning, qaIgnore|qaAbort)` on failure
4. Clear edit on Abort

## GitHub Issue

N/A — this is an alignment improvement, discovered during review of plan
`gui-certificate-editor-webdav-s3-improved.md` against actual WinSCP sources.