# Reference: NetBox S3FileSystem and Session Data Mapping

**Date:** 2026-04-26
**Source:** Exploration via `subagent_type: Explore` (task-2)
**Files Analyzed:**
- `src/core/SessionData.h` (NetBox)
- `src/core/SessionData.cpp`
- `src/core/S3FileSystem.cpp`
- `src/core/NeonIntf.cpp/h`
- `src/core/WebDAVFileSystem.cpp`
- `src/core/Certificates.cpp`

**Related:**
- Architecture: `.ai-factory/ARCHITECTURE.md`
- Plan: `.ai-factory/plans/s3-encryption-options-winscp-alignment.md`

---

## 1. Current S3-Related Fields in NetBox TSessionData

### 1.1 Declared Members (`SessionData.h`)

```cpp
UnicodeString FS3DefaultRegion;
UnicodeString FS3SessionToken;
UnicodeString FS3RoleArn;
UnicodeString FS3RoleSessionName;
UnicodeString FS3Profile;
TS3UrlStyle FS3UrlStyle;           // enum: s3usVirtualHost, s3usPath
TAutoSwitch FS3MaxKeys;            // auto-switch for max keys per request
bool FS3CredentialsEnv;            // use environment credentials
bool FS3RequesterPays;             // requester pays flag
UnicodeString FS3CACertificate;   // Custom CA certificate (PEM content) — **not serialized**
```

### 1.2 Property Declarations

Properties define getters/setters and change notification:

```cpp
// In SessionData.h (example pattern):
__property UnicodeString S3DefaultRegion = { read = FS3DefaultRegion, write = SetS3DefaultRegion };
__property TS3UrlStyle S3UrlStyle = { read = FS3UrlStyle, write = SetS3UrlStyle };
__property bool S3CredentialsEnv = { read = FS3CredentialsEnv, write = SetS3CredentialsEnv };
// S3CACertificate exists but may lack setter in PROPERTY2 block (gap)
```

---

## 2. Serialization Gaps

### 2.1 Load (`DoLoad` in `SessionData.cpp`, around lines 931-937)

**Current S3 fields loaded:**
```cpp
FS3DefaultRegion = Storage->ReadString(L"S3DefaultRegion", FS3DefaultRegion);
FS3SessionToken = Storage->ReadString(L"FS3SessionToken", FS3SessionToken);
FS3Profile = Storage->ReadString(L"S3Profile", FS3Profile);
FS3UrlStyle = (TS3UrlStyle)Storage->ReadInteger(L"S3UrlStyle", FS3UrlStyle);
FS3MaxKeys.Read(*Storage, L"S3MaxKeys");
FS3CredentialsEnv = Storage->ReadBool(L"S3CredentialsEnv", FS3CredentialsEnv);
FS3RequesterPays = Storage->ReadBool(L"S3RequesterPays", FS3RequesterPays);
// S3CACertificate NOT loaded!
```

**Gap:** `FS3CACertificate` is absent from `DoLoad`.

### 2.2 Save (`DoSave` in `SessionData.cpp`, around lines 1306-1310)

**Current S3 fields saved:**
```cpp
WRITE_DATA(String, S3DefaultRegion);
WRITE_DATA(String, S3SessionToken);
WRITE_DATA(String, S3Profile);
WRITE_DATA(Integer, S3UrlStyle);
WRITE_DATA(AutoSwitch, S3MaxKeys);
WRITE_DATA(Bool, S3CredentialsEnv);
WRITE_DATA(Bool, S3RequesterPays);
// S3CACertificate NOT saved!
```

**Gap:** `FS3CACertificate` is absent from `DoSave`.

---

## 3. TLS Setup in S3 Implementation

### 3.1 Connection Flow (`S3FileSystem.cpp`)

- `TS3FileSystem::Open()` → connects via libs3
- `LibS3SessionCallback()` registers `SetNeonTlsInit` with neon session
- `InitSslSession()` → `InitSslSessionImpl()` performs actual TLS configuration
- `SetupSsl()` (in `NeonIntf.cpp`) applies `MinTlsVersion`/`MaxTlsVersion` to `SSL_CTX`

### 3.2 `InitSslSessionImpl` Details (lines 726-780)

Key actions:
```cpp
void TS3FileSystem::InitSslSessionImpl(ne_session *session)
{
  // ... checks ...
  if (!FTerminal->InitSslContext(SSL_CTX, FProtocolName)) return;

  // Existing code reads S3CACertificate but only logs:
  UnicodeString Cert = FTerminal->GetSessionData()->S3CACertificate;
  if (!Cert.IsEmpty())
  {
    // TODO: Implement via ne_ssl_set_certificates_storage() after writing cert to temp file
    FTerminal->LogEvent(L"S3CACertificate is set but not applied yet");
  }

  // TLS setup via SetupSsl already applied
}
```

**Critical Gap:** The `S3CACertificate` is read but not applied; there is a TODO indicating implementation needed.

---

## 4. Neon TLS Integration Pattern

### 4.1 `SetupSsl` (`NeonIntf.cpp` around 648-680)

Universal for HTTPS-based protocols (WebDAV, S3):

```cpp
static void SetupSsl(ne_session *session, TSessionData *Data)
{
  SSL_CTX *ctx = ne_ssl_ctx_create();
  // Set min/max protocol from Data->FMinTlsVersion, Data->FMaxTlsVersion
  int min_version = ConvertTlsVersion(Data->FMinTlsVersion);
  int max_version = ConvertTlsVersion(Data->FMaxTlsVersion);
  SSL_CTX_set_min_proto_version(ctx, min_version);
  SSL_CTX_set_max_proto_version(ctx, max_version);
  // Set security level, verification callback
  SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, VerifyCallback);
  // ...
}
```

This function is used by both WebDAV and S3. **NetBox does not apply per-protocol cipher lists for HTTPS; it relies on OpenSSL defaults controlled by TLS version and security level** — this matches WinSCP pattern.

---

## 5. Certificate Validation

Certificate verification is centralized in `TCertificates.cpp`:

- `VerifyOrConfirmHttpCertificate()` — called from SSL verify callback
- Allows user to override failures in interactive sessions
- Uses embedded `cacert.pem` as default trust store

**No support for per-session custom CA** currently exists; it's a global setting.

---

## 6. UI Dialog Findings (from separate exploration)

See `netbox-ui-dialogs-s3-config.md` for details. Summary:

- Session dialog located in `src/NetBox/WinSCPDialogs.cpp` (TSessionDialog class)
- S3 tab (`tabS3`) contains:
  - Region combo, URL style combo, Requester Pays checkbox
  - `S3CACertificateEdit` text field (exists but lacks Load/Save buttons)
  - No MinTlsVersion/MaxTlsVersion controls (gap)
- NetBox does NOT use VCL like WinSCP; uses `TFarDialog`, `TFarEdit`, `TFarButton`

---

## 7. Mapping Compare: WinSCP vs NetBox

|Aspect|WinSCP|NetBox|Gap|
|---|---|---|---|
|S3CACertificate storage|Not present|Declared but not serialized|Serialization missing|
|S3CACertificate usage|N/A|TODO: apply via ne_ssl_set_certificates_storage|Not implemented|
|TLS version storage|`MinTlsVersion`/`MaxTlsVersion` in `TSessionData`|Same|Used by `SetupSsl()` ✅|
|TLS version UI|TLS page (shared across S3/WebDAV/FTPS)|Not exposed in S3 tab|UI controls missing|
|Cipher list for S3|Not exposed (HTTPS defaults)|Same (no field)|Correct — intentional|
|Property macros|Various (`RWProperty`, `PROPERTY`)|`SESSION_PROPERTY`, `RWPropertySimple`|Style differs but pattern same|

---

## 8. Files to Modify (per NetBox plan)

|Task|Files|What|
|---|---|---|
|1. Serialize S3CACertificate|`src/core/SessionData.{h,cpp}`|Add `FS3CACertificate` to `DoLoad`/`DoSave` and property block|
|2. Apply S3CACertificate|`src/core/S3FileSystem.cpp`|Implement temp file + `ne_ssl_set_certificates_storage()`|
|3. TLS version UI|`src/NetBox/WinSCPDialogs.{h,cpp}`|Add Min/Max TLS combo boxes to S3 tab|
|4. CA Load/Save buttons|`src/NetBox/WinSCPDialogs.{h,cpp}`|Add buttons next to `S3CACertificateEdit`|

---

## 9. Default Values and Conventions

|Field|Default|Source|
|---|---|---|
|`MinTlsVersion`|`tls12` (WinSCP `tlsDefaultMin`)|WinSCP `TTlsVersion` enum|
|`MaxTlsVersion`|`tls13`|WinSCP defaults|
|`S3CACertificate`|empty string (no custom CA)|Consistent with global default|
|`S3UrlStyle`|`s3usVirtualHost`|WinSCP default|

---

## 10. Blocks and Risks

- **No way to load PEM without buttons** — current `S3CACertificateEdit` is manual text entry; Load/Save buttons will improve UX.
- **neon API for custom CA** — must confirm `ne_ssl_set_certificates_storage()` exists in linked neon version.
- **Temp file security** — ensure file permissions restrict access, delete immediately after `SSL_CTX` load.
- **TLS version validation** — UI must enforce Max >= Min before saving.

---

*End of Reference*
