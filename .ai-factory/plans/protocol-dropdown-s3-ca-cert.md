# Implementation Plan: Protocol Dropdown — Show CA Certificate Instead of S3

Branch: feature/protocol-dropdown-s3-ca-cert
Created: 2026-04-29

## Settings

- Testing: no
- Logging: verbose
- Docs: yes

## Roadmap Linkage

Milestone: "Version 1.2 — Background copy & progress UI"
Rationale: The S3 protocol recently gained CA certificate support (feat(s3): add S3CACertificate session setting persistence). Updating the protocol dropdown label to reflect this capability improves UX and helps users identify the S3 protocol's distinguishing feature.

## Research Context

Source: User request — change protocol dropdown display text.
Goal: In the session dialog TransferProtocolCombo, replace the "S3" label with "CA certificate" to reflect the recently added S3 CA certificate support.

Constraints:
- Do not modify third-party code in `libs/`.
- Maintain WinXP compatibility.
- All changes must compile with MSVC W4, zero warnings.
- Preserve CRLF line endings.
- All language files must remain consistent (same number of lines).

Decisions:
- Update the `NB_LOGIN_S3` language string in all `.lng` files from "S3" to "CA certificate".
- Optionally also update `NB_LOGIN_TAB_S3` (tab label) from "S3" to "CA certificate" for consistency.
- No MsgIDs.h changes needed if only modifying existing string values.

## Tasks

### Task 1: Update English language string
- File: `src/NetBox/NetBoxEng.lng`
- Locate the `NB_LOGIN_S3` string (currently "S3") and change it to "CA certificate".
- Locate the `NB_LOGIN_TAB_S3` string (currently "S3") and change it to "CA certificate".
- Verify line count is unchanged.
- Ensure CRLF line endings.
- No trailing whitespace.

### Task 2: Update Russian language string
- File: `src/NetBox/NetBoxRus.lng`
- Translate "CA certificate" to Russian: "CA сертификат" (or similar appropriate translation).
- Update both the protocol dropdown and tab label strings.
- Verify line count matches English file.
- Ensure CRLF line endings.

### Task 3: Update Polish language string
- File: `src/NetBox/NetBoxPol.lng`
- Translate "CA certificate" to Polish: "Certyfikat CA".
- Update both the protocol dropdown and tab label strings.
- Verify line count matches English file.
- Ensure CRLF line endings.

### Task 4: Update Spanish language string
- File: `src/NetBox/NetBoxSpa.lng`
- Translate "CA certificate" to Spanish: "Certificado CA".
- Update both the protocol dropdown and tab label strings.
- Verify line count matches English file.
- Ensure CRLF line endings.

### Task 5: Update French language string
- File: `src/NetBox/NetBoxFr.lng`
- Translate "CA certificate" to French: "Certificat CA".
- Update both the protocol dropdown and tab label strings.
- Verify line count matches English file.
- Ensure CRLF line endings.

### Task 6: Verify WinSCPDialogs.cpp references
- File: `src/NetBox/WinSCPDialogs.cpp`
- Verify that `TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_S3))` uses the updated string.
- Verify that `AddTab(tabS3, GetMsg(NB_LOGIN_TAB_S3))` uses the updated string.
- No code changes needed unless new message IDs were added.

### Task 7: Build and validate
- Run `build-x64.bat` to verify compilation with zero warnings.
- Confirm plugin DLL is produced.
- Check that all `.lng` files have identical line counts using `wc -l`.
- Verify CRLF line endings on all modified files.

## Commit Plan

- **Commit 1** (after tasks 1-5): `feat(i18n): rename S3 protocol label to CA certificate in all languages`
- **Commit 2** (after task 6): `feat(ui): update protocol dropdown and tab to display CA certificate`
- **Commit 3** (after task 7): `docs: add implementation note for protocol label change`

## Acceptance Criteria

- [x] `NB_LOGIN_S3` displays "CA certificate" (or localized equivalent) in all 5 language files.
- [x] `NB_LOGIN_TAB_S3` displays "CA certificate" (or localized equivalent) in all 5 language files.
- [x] All `.lng` files have the same number of lines.
- [x] All modified files use CRLF line endings.
- [x] No trailing whitespace introduced.
- [x] Build completes with zero warnings on x64 RelWithDebugInfo.
- [x] Plugin DLL is present in `Far3_x64/Plugins/NetBox/`.
- [x] No modifications to `libs/`.

## Changelog

- **2026-04-29** — Initial plan created.
