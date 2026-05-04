<!-- handoff:task: -->
# Plan: WinSCP-Aligned Certificate Editor — WebDAV/S3 Session Dialog

**Branch:** lmv/dev
**Created:** 2026-04-24 | **Updated:** 2026-05-04 (WinSCP alignment)
**Original plan:** `.ai-factory/plans/gui-certificate-editor-webdav-s3.md`
**WinSCP reference:** `D:/Projects/WinSCP-work/winscp-master/source/`

## WinSCP Alignment Findings

Analysis of WinSCP master sources revealed the original plan was based on incorrect assumptions:

| Original Assumption | WinSCP Reality |
|---|---|
| Separate `WebDAVCertificate` property | Single `TlsCertificateFile` for all protocols |
| Load/Save PEM **content** in dialog | `TFilenameEdit` browses for a **file path** |
| S3 `S3CACertificate` editor needs parity | `S3CACertificate` removed from NetBox; WinSCP doesn't have it |
| File picker must be built from scratch | `BrowseForCertificateFile()` already exists |

**Result:** The core feature (TLS certificate file browse control) was already implemented
in NetBox via the issue #389 follow-up work. Only alignment refinements remain.

**Reference exploration:** [winscp-certificate-editor-patterns](../references/winscp-certificate-editor-patterns.md)

## Settings

- **Testing:** no (alignment refactoring; manual verification sufficient)
- **Logging:** minimal (only error paths)
- **Docs:** yes (reference file created)

## Scope

Align NetBox's `BrowseForCertificateFile()` with WinSCP's `TSslSheet` pattern:

1. Filter: match WinSCP's `*.pfx;*.p12;*.key;*.pem` (remove `.crt`, `.cer`)
2. Validation: call `CheckCertificate()` after browse, show Ignore/Abort dialog on failure
3. Build and verify

## Out of Scope

- Separate CA certificate editing (not a WinSCP feature)
- TLS/SSL shared sheet/tab (architectural refactor; separate effort)
- VCL `VerifyCertificate()`/`ExceptionMessageDialog` (unused in NetBox build)

## Tasks

### Phase 1: Filter Alignment

- [ ] **Task 1: Align certificate file filter with WinSCP**
  - Change `BrowseForCertificateFile()` filter from `*.pem;*.crt;*.cer;*.pfx;*.p12;*.key`
    to WinSCP's `*.pfx;*.p12;*.key;*.pem`
  - **File:** `src/NetBox/WinSCPDialogs.cpp` (line 5006)

### Phase 2: Certificate Validation

- [ ] **Task 2: Integrate CheckCertificate() into browse handler**
  - After `TargetEdit->SetText(FileName)`, wrap in try/catch calling `CheckCertificate(FileName)`
  - On failure, show `WinSCPPlugin->MoreMessageDialog(E.Message, nullptr, qtWarning, qaIgnore | qaAbort)`
  - If user clicks Abort, clear the edit (`TargetEdit->SetText(L"")`)
  - **File:** `src/NetBox/WinSCPDialogs.cpp` (in `BrowseForCertificateFile()`)
  - Uses existing `CheckCertificate()` from `src/base/Common.cpp:4524`

### Phase 3: Verification

- [ ] **Task 3: Build and verify**
  - Build x64 RelWithDebugInfo via `build-x64.bat`
  - Ensure zero MSVC W4 warnings

## Dependencies

Task 1 → Task 2 → Task 3

## Success Criteria

- Certificate file filter matches WinSCP (`*.pfx;*.p12;*.key;*.pem`)
- After browsing, invalid certificates trigger warning dialog
- Ignore keeps the path; Abort clears the edit
- Build passes with zero warnings
- No new files created (only existing `WinSCPDialogs.cpp` modified)

## Commit Plan

- **Commit:** `fix(dialog): align certificate file filter and validation with WinSCP`
- Remove `.crt`/`.cer` from filter
- Add `CheckCertificate()` validation after browse with Ignore/Abort dialog

## Changelog

- 2026-05-04: Complete rewrite based on WinSCP source analysis. Replaced 11 original tasks
  (based on incorrect assumptions about separate WebDAV/S3 CA cert editors) with 3
  WinSCP-aligned tasks. Removed all references to `FWebDAVCertificate`, `S3CACertificate`,
  PEM content editor, Catch2 tests, and docs/features/.hlf.