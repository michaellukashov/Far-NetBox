# Implementation Plan: WinSCP Dialog Alignment

Branch: none (stored plan)
Created: 2026-05-03
Plan Type: fast

## Settings
- Testing: no
- Logging: verbose
- Docs: yes

## Research Context
Source: .ai-factory/RESEARCH.md (Active Summary)

Goal: Align WinSCPDialogs.cpp tab structure, control naming, and enablement logic with WinSCP master (D:\Projects\WinSCP-work\winscp-master\source) where Far Manager text-mode UI allows.

Constraints:
- No modifications to `libs/` — use patches only
- Far Manager API calls on main thread only
- MSVC /W4 zero warnings
- Incremental evolution — no architectural rewrites
- C++17 standard only

## Architecture Context

WinSCP uses two-dialog model (LoginDialog + SiteAdvancedDialog with 18 VCL tabs). NetBox uses a single TSessionDialog with 16 text-mode tabs (tabSession=1 through tabWebDAV=16). WinSCP's SslSheet ("TLS/SSL") is a shared tab for FTP/WebDAV/S3. NetBox replicates TLS controls per-protocol since Far Manager cannot share tabs across protocols.

**Reference:** D:\Projects\WinSCP-work\winscp-master\source\forms\SiteAdvanced.cpp (UpdateControls, LoadSession, SaveSession)

**Key WinSCP patterns to replicate:**
- TLS version labels use full words: "Minimum TLS version:" / "Maximum TLS version:"
- S3 authentication: session token disabled when `HasAutoCredentials()`, RoleArn only for Amazon S3
- TLS client certificate: FTP/WebDAV only, NOT S3
- S3 authentication fields: session token, role ARN, role session name
- S3 credentials from AWS environment: checkbox + profile dropdown on S3 tab

## Tasks

### Phase 1: S3 Tab Cleanup & Alignment

- [x] **Task 1: Remove duplicate controls overwritten by TLS tab**
  - Remove `MinTlsVersionCombo` creation from S3 tab (line ~2677) — overwritten by TLS tab at ~2932, leaking first `MakeOwnedObject`
  - Remove `SslSessionReuseCheck` creation from FTP tab (line ~2557) — overwritten by TLS tab at ~2922, same leak
  - Lines ~3623-3624 in UpdateControls(): Remove S3-only `MinTlsVersionCombo->SetEnabled(aS3Protocol)` (TLS tab handles this at ~3629)
  - Note: `MaxTlsVersionCombo` is only on TLS tab (line ~2945) — no duplicate to remove
  - LOG: Log removal with `FTerminal->LogEvent(L"Dialog: removed duplicate controls overwritten by TLS tab")`

- [x] **Task 2: Remove S3 TLS client certificate controls**
  - WinSCP does NOT show TLS client certificate for S3 (only FTP and WebDAV)
  - Remove `S3TlsCertificateFileLabel`, `S3TlsCertificateFileEdit`, `S3TlsCertificateFileBrowseBtn` member variables (lines ~1929-1931)
  - Remove their creation in S3 tab constructor
  - Remove their enablement from UpdateControls() (lines ~3620-3622)
  - Remove `S3TlsCertificateFileBrowseClick` handler declaration + browse implementation
  - Remove data binding in Execute load/save
  - LOG: Log removal with `FTerminal->LogEvent(L"Dialog: removed S3 TLS client certificate controls (WinSCP alignment)")`

- [x] **Task 3: Uncomment S3CredentialsEnvCheck and S3ProfileCombo on S3 tab**
  - Data layer fully wired: `FS3CredentialsEnv` serialized (SessionData.cpp ~948/1328), `FS3Profile` serialized (~945/1325)
  - `S3FileSystem` already consumes both for AWS credential resolution
  - Uncomment member vars: `S3CredentialsEnvCheck` (~1946), `S3ProfileCombo` (~1947), `S3ProfileLabel` (~1948)
  - Uncomment creation: checkbox with `GetMsg(NB_S3_CREDENTIALS_ENV)` at ~2630, label + combo with `GetMsg(NB_S3_PROFILE)` at ~2636-2644
  - Uncomment UpdateControls: visibility at ~3576-3578, enablement at ~3614-3616 (basic `aS3Protocol` only)
  - Uncomment Execute load/save: read `SessionData->S3CredentialsEnv`/`SessionData->S3Profile` at ~3931-3950, write at ~4293-4298
  - `S3ProfileCombo` population: use `GetS3Profiles()` from S3FileSystem (MSVC stub at S3FileSystem.cpp ~503)
  - LOG: Log load/save with `FTerminal->LogEvent(L"Dialog: S3 credentials env = %d, profile = %s", ...)`

- [x] **Task 4: Add S3 Role Session Name edit to S3 tab**
  - Add `TFarText * S3RoleSessionNameLabel{nullptr};` and `TFarEdit * S3RoleSessionNameEdit{nullptr};` member variables
  - Create controls on S3 tab, below RoleArnEdit, using `GetMsg(NB_S3_ROLE_SESSION_NAME)`
  - Wire basic enablement: `SetEnabled(aS3Protocol)` — final enablement logic deferred to Task 5
  - Wire data binding: load from `SessionData->GetS3RoleSessionName()`, save to `SessionData->SetS3RoleSessionName(text)`
  - LOG: Log load/save with `FTerminal->LogEvent(L"Dialog: S3 role session name = %s", ...)`

- [x] **Task 5: Add WinSCP-style S3 authentication enablement logic**
  - Add `#include <S3FileSystem.h>` to WinSCPDialogs.cpp (`IsAmazonS3SessionData()` is a free function at S3FileSystem.h:216)
  - In UpdateControls() S3 section, refine basic `aS3Protocol` enablement from Tasks 3-4:
  - `S3SessionTokenEdit->SetEnabled(aS3Protocol && !SessionData->HasAutoCredentials())`
  - `S3SessionTokenLabel->SetEnabled(S3SessionTokenEdit->GetEnabled())`
  - `S3RoleArnEdit->SetEnabled(S3SessionTokenEdit->GetEnabled() && IsAmazonS3SessionData(SessionData))`
  - `S3RoleArnLabel->SetEnabled(S3RoleArnEdit->GetEnabled())`
  - Same logic for `S3RoleSessionNameEdit/Label` from Task 4
  - `S3ProfileCombo->SetEnabled(aS3Protocol && S3CredentialsEnvCheck->GetChecked())` (profile only needed when env enabled)
  - LOG: Log enablement with `FTerminal->LogEvent(L"Dialog: S3 auth: hasAutoCred=%d, isAmazon=%d", ...)`

### Phase 2: TLS Tab & Naming Alignment

- [x] **Task 6: Fix Min/MaxTlsVersionCombo visibility for WebDAV/HTTPS on TLS tab**
  - Current: combos visible only when `IsMainTab && aS3Protocol` (line ~3574-3575), but enabled for `aFtpsProtocol || HTTPSProtocol || aS3Protocol` (line ~3629-3630)
  - Result: WebDAV/HTTPS sessions show TLS tab but version combos are invisible while enabled
  - Fix visibility: `SetVisible(aS3Protocol || aFtpsProtocol || HTTPSProtocol)` (match to TLS tab's own enablement)
  - Verify TLS version combos show for all TLS-capable protocols
  - LOG: Log visibility change

- [x] **Task 7: Rename TLS version MsgIDs to remove "S3_" prefix**
  - `NB_S3_MIN_TLS_VERSION` → `NB_TLS_MIN_VERSION` in MsgIDs.h
  - `NB_S3_MAX_TLS_VERSION` → `NB_TLS_MAX_VERSION` in MsgIDs.h
  - Replace references in WinSCPDialogs.cpp
  - Verify .lng files: "Minimum TLS version:"/"Maximum TLS version:" strings already match WinSCP (commit aa4553b0b)
  - Rebuild with zero warnings

### Phase 3: Documentation

- [x] **Task 8: Update .hlf help files with dialog descriptions**
  - Update `NetBox.en.hlf`: S3 tab — session token, role ARN, role session name, credentials from AWS env, profile, protocol options
  - Update `NetBox.ru.hlf`: Same in Russian
  - Update TLS tab section: Minimum/Maximum TLS version, session reuse (FTP only), client certificate (FTP/WebDAV only)
  - Follow existing .hlf formatting conventions (topic headers, \b tags)

## Commit Plan
- **Commit 1** (after tasks 1-5): `fix: align S3 dialog tab with WinSCP — remove dupes, add credentials/env, role session name, auth enablement`
- **Commit 2** (after tasks 6-7): `fix: enable TLS version combos for HTTPS/WebDAV, rename MsgIDs to remove S3 prefix`
- **Commit 3** (after task 8): `docs: update help files with S3/TLS dialog descriptions`

## Changelog
- 2026-05-03: Refined plan — added 3 missing tasks (S3 credentials UI, SslSessionReuseCheck duplicate, WebDAV/HTTPS combo visibility); fixed Task 1 scope (Min only not Max); fixed Task 4/5 dependency order and IsAmazonS3SessionData location