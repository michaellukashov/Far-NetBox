# WinSCP S3 Session Token and Role ARN Implementation Reference

> **Source:** WinSCP master codebase (`D:\Projects\WinSCP-work\winscp-master\source`)  
> **Extracted:** 2026-05-03  
> **Scope:** S3 authentication — Session Token (IMDSv2/temporary credentials) and Role ARN (STS AssumeRole)  
> **Related NetBox Plan:** `.ai-factory/plans/s3-encryption-options-winscp-alignment.md`

---

## 1. Overview

WinSCP supports two advanced S3 authentication mechanisms beyond static Access Key / Secret Key:

| Mechanism | Field | Purpose | Source |
|-----------|-------|---------|--------|
| **Session Token** | `S3SessionToken` | Temporary credentials (IMDSv2, STS, assumed role) | User input, env `AWS_SESSION_TOKEN`, or IMDSv2 |
| **Role ARN** | `S3RoleArn` | AWS STS AssumeRole target | User input, env `AWS_ROLE_ARN` |

Both are stored in `TSessionData`, serialized to XML, exposed in the **Site Advanced** dialog (not the Login dialog), and consumed by `TS3FileSystem` during connection establishment.

---

## 2. SessionData Properties

### 2.1 Declarations (`SessionData.h`)

```cpp
UnicodeString FS3SessionToken;
UnicodeString FS3RoleArn;
UnicodeString FS3RoleSessionName;

void __fastcall SetS3SessionToken(UnicodeString value);
void __fastcall SetS3RoleArn(UnicodeString value);
void __fastcall SetS3RoleSessionName(UnicodeString value);

__property UnicodeString S3SessionToken = { read = FS3SessionToken, write = SetS3SessionToken };
__property UnicodeString S3RoleArn = { read = FS3RoleArn, write = SetS3RoleArn };
__property UnicodeString S3RoleSessionName = { read = FS3RoleSessionName, write = SetS3RoleSessionName };
```

### 2.2 Serialization (`SessionData.cpp`)

**DoLoad (read):**
```cpp
S3SessionToken = Storage->ReadString(L"S3SessionToken", S3SessionToken);
S3RoleArn = Storage->ReadString(L"S3RoleArn", S3RoleArn);
S3RoleSessionName = Storage->ReadString(L"S3RoleSessionName", S3RoleSessionName);
```

**DoSave (write):**
```cpp
WRITE_DATA(String, S3SessionToken);
WRITE_DATA(String, S3RoleArn);
WRITE_DATA(String, S3RoleSessionName);
```

### 2.3 Defaults (`SessionData.cpp` constructor)
```cpp
S3SessionToken = EmptyStr;
S3RoleArn = EmptyStr;
S3RoleSessionName = EmptyStr;
```

---

## 3. S3FileSystem Implementation

### 3.1 File: `core/S3FileSystem.cpp`

#### Environment Variable Helpers

```cpp
#define AWS_SESSION_TOKEN L"AWS_SESSION_TOKEN"
#define AWS_ROLE_ARN L"AWS_ROLE_ARN"
#define AWS_ROLE_ARN_KEY L"role_arn"  // for .aws/config

UnicodeString S3EnvSessionToken(const UnicodeString & Profile, UnicodeString * Source, bool OnlyCached)
{
  return GetS3ConfigValue(Profile, AWS_SESSION_TOKEN, AWS_SESSION_TOKEN, L"Token", Source, OnlyCached);
}

UnicodeString S3EnvRoleArn(const UnicodeString & Profile, UnicodeString * Source, bool OnlyCached)
{
  return GetS3ConfigValue(Profile, AWS_ROLE_ARN, AWS_ROLE_ARN_KEY, EmptyStr, Source, OnlyCached);
}
```

> `GetS3ConfigValue` reads from: environment variables → shared credentials file (`~/.aws/credentials`) → config file (`~/.aws/config`).

#### Connection Flow (`Open()` → `Connect()`)

```cpp
void TS3FileSystem::Connect(const UnicodeString & /*APath*/)
{
  // ... access key / secret key resolution ...

  // 1. Resolve session token (priority: SessionData > env > IMDSv2)
  UnicodeString SessionToken = Data->S3SessionToken;
  if (SessionToken.IsEmpty() && Data->S3CredentialsEnv)
  {
    UnicodeString SessionTokenSource;
    SessionToken = S3EnvSessionToken(S3Profile, &SessionTokenSource);
    if (!SessionToken.IsEmpty())
    {
      FTerminal->LogEvent(FORMAT(L"Session token read from %s", (SessionTokenSource)));
    }
  }

  SetCredentials(AccessKeyId, SecretAccessKey, SessionToken);

  // ... host setup ...

  // 2. Resolve Role ARN and perform STS AssumeRole
  UnicodeString RoleArn = Data->S3RoleArn;
  if (RoleArn.IsEmpty())
  {
    UnicodeString RoleArnSource;
    RoleArn = S3EnvRoleArn(S3Profile, &RoleArnSource);
    if (!RoleArn.IsEmpty())
    {
      FTerminal->LogEvent(FORMAT(L"Role ARN read from %s", (RoleArnSource)));
    }
  }

  if (!RoleArn.IsEmpty())
  {
    AssumeRole(RoleArn);
  }
}
```

#### SetCredentials

```cpp
void TS3FileSystem::SetCredentials(
  const UnicodeString & AccessKeyId,
  const UnicodeString & SecretAccessKey,
  const UnicodeString & SessionToken)
{
  FAccessKeyId = UTF8String(AccessKeyId);
  // ... secret key ...
  FSecurityTokenBuf = UTF8String(SessionToken);
  FSecurityToken = static_cast<const char *>(FSecurityTokenBuf.data());
}
```

> `FSecurityToken` is passed to libs3 as the `securityToken` parameter on every S3 API call.

#### AssumeRole Implementation (`TS3FileSystem::AssumeRole`)

```cpp
const UnicodeString AssumeRoleVersion(TraceInitStr(L"2011-06-15"));
const UnicodeString AssumeRoleNamespace(
  TraceInitStr(FORMAT(L"https://sts.amazonaws.com/doc/%s/", (AssumeRoleVersion))));

void TS3FileSystem::AssumeRole(const UnicodeString & RoleArn)
{
  // AWS_ROLE_SESSION_NAME does NOT apply here per AWS CLI behavior
  UnicodeString RoleSessionName = DefaultStr(
    FTerminal->SessionData->S3RoleSessionName,
    AppNameString());

  UnicodeString QueryParams = FORMAT(
    L"Version=%s&Action=AssumeRole&RoleSessionName=%s&RoleArn=%s",
    (AssumeRoleVersion, EncodeUrlString(RoleSessionName), EncodeUrlString(RoleArn)));

  // Build HTTP request to STS endpoint
  RequestParams AssumeRoleRequestParams = {
    HttpRequestTypeGET,
    { /* auth using current credentials */ },
    /* ... */
  };

  request_perform(&AssumeRoleRequestParams, FRequestContext);
  CheckLibS3Error(Data);

  // Parse XML response
  _di_IXMLNode ResponseNode = AssumeRoleNeedNode(Document->ChildNodes, L"AssumeRoleResponse");
  _di_IXMLNode ResultNode = AssumeRoleNeedNode(ResponseNode->ChildNodes, L"AssumeRoleResult");
  _di_IXMLNode CredentialsNode = AssumeRoleNeedNode(ResultNode->ChildNodes, L"Credentials");

  UnicodeString AccessKeyId = AssumeRoleNeedNode(CredentialsNode->ChildNodes, L"AccessKeyId")->Text;
  UnicodeString SecretAccessKey = AssumeRoleNeedNode(CredentialsNode->ChildNodes, L"SecretAccessKey")->Text;
  UnicodeString SessionToken = AssumeRoleNeedNode(CredentialsNode->ChildNodes, L"SessionToken")->Text;
  UnicodeString ExpirationStr = AssumeRoleNeedNode(CredentialsNode->ChildNodes, L"Expiration")->Text;

  FTerminal->LogEvent(FORMAT(L"Assumed role \"%s\".", (RoleArn)));
  // ... new credentials replace current ones
  SetCredentials(AccessKeyId, SecretAccessKey, SessionToken);
}
```

#### IMDSv2 Session Token (EC2 Metadata Service)

WinSCP also supports IMDSv2 for retrieving instance credentials:

```cpp
// Static cache
static TDateTime S3CredentialsExpiration;
static UnicodeString S3SessionToken;  // IMDSv2 token (NOT the AWS session token)

// To fetch IMDSv2 token:
UnicodeString TokenUrl = AWSAPI + L"api/token";
std::unique_ptr<THttp> Http(CreateHttp(TokenUrl, ConnectTimeout));
RequestHeaders->Values[L"X-aws-ec2-metadata-token-ttl-seconds"] = IntToStr(TtlSeconds);
Http->Put(EmptyStr);
S3SessionToken = Http->Response.Trim();  // This is the IMDSv2 metadata token
```

> ⚠️ **Naming collision risk:** The static `S3SessionToken` in `S3FileSystem.cpp` is the **IMDSv2 metadata token** (used to authenticate to the EC2 metadata service), NOT the AWS session token. The AWS session token is stored in `TSessionData::S3SessionToken`.

---

## 4. UI Implementation

### 4.1 Dialog: `SiteAdvanced.cpp` / `SiteAdvanced.dfm`

Both controls are on the **S3 tab** of the Site Advanced dialog (not the main Login dialog).

```cpp
// Load from session data
UnicodeString S3SessionToken = FSessionData->S3SessionToken;
UnicodeString S3RoleArn = FSessionData->S3RoleArn;

if (FSessionData->HasAutoCredentials())
{
  // Pre-populate from environment if auto-credentials enabled
  S3SessionToken = S3EnvSessionToken(FSessionData->S3Profile);
  S3RoleArn = S3EnvRoleArn(FSessionData->S3Profile);
}

S3SessionTokenMemo->Lines->Text = S3SessionToken;  // TMemo (multi-line)
S3RoleArnEdit->Text = S3RoleArn;                   // TEdit (single-line)
```

**Save back to session data:**
```cpp
if (/* empty */)
{
  SessionData->S3SessionToken = EmptyStr;
}
else
{
  // Trim to avoid stray new-lines
  SessionData->S3SessionToken = S3SessionTokenMemo->Lines->Text.Trim();
}
FSessionData->S3RoleArn = S3RoleArnEdit->Text;
```

### 4.2 Visibility Rules

```cpp
EnableControl(S3SessionTokenMemo, S3Sheet->Enabled && !FSessionData->HasAutoCredentials());
EnableControl(S3SessionTokenLabel, S3SessionTokenMemo->Enabled);
EnableControl(S3RoleArnEdit, S3SessionTokenMemo->Enabled && IsAmazonS3SessionData(FSessionData));
EnableControl(S3RoleArnLabel, S3RoleArnEdit->Enabled);
```

- Session Token memo is **disabled** when auto-credentials (environment) are active
- Role ARN edit is **disabled** for non-Amazon S3 sessions (e.g., custom S3-compatible endpoints)

### 4.3 DFM Layout (`SiteAdvanced.dfm`)

```dfm
object S3SessionTokenLabel: TLabel
  Caption = '&Session token:'
  FocusControl = S3SessionTokenMemo
end

object S3SessionTokenMemo: TMemo
  // Multi-line memo for token (can be long)
end

object S3RoleArnLabel: TLabel
  Caption = '&Role ARN:'
  FocusControl = S3RoleArnEdit
end

object S3RoleArnEdit: TEdit
  // Single-line edit for ARN
  OnChange = DataChange
end
```

---

## 5. Login Dialog Integration

### 5.1 `Login.cpp` — Auto-populate from environment

When the user opens a session with auto-credentials:

```cpp
if (/* auto-credentials enabled */)
{
  FSessionData->S3SessionToken = S3EnvSessionToken(S3Profile);
  FSessionData->S3RoleArn = S3EnvRoleArn(S3Profile);
}
```

### 5.2 `Login.cpp` — Clear if matches environment

On save, if the value matches the environment default, clear it to avoid persisting redundant data:

```cpp
if (FSessionData->S3SessionToken == S3EnvSessionToken(S3Profile, NULL, true))
{
  FSessionData->S3SessionToken = EmptyStr;
}
if (FSessionData->S3RoleArn == S3EnvRoleArn(S3Profile, NULL, true))
{
  FSessionData->S3RoleArn = EmptyStr;
}
```

---

## 6. Logging

### 6.1 Session Token

```cpp
// In SessionInfo.cpp (session logging)
if (!Data->S3SessionToken.IsEmpty())
{
  ADF(L"S3: Session token: %s", (Data->S3SessionToken));
}
```

> **Security note:** The session token is logged **without** `LogSensitive` guard. In WinSCP this is accepted for diagnostic session logs (user-controlled). NetBox should evaluate whether to gate this.

### 6.2 Role ARN

```cpp
if (!Data->S3RoleArn.IsEmpty())
{
  ADF(L"S3: Role ARN: %s (session name: %s)",
    (Data->S3RoleArn, DefaultStr(Data->S3RoleSessionName, L"default")));
}
```

### 6.3 AssumeRole

```cpp
FTerminal->LogEvent(FORMAT(L"Assumed role \"%s\".", (RoleArn)));
FTerminal->LogEvent(FORMAT(L"New access key is: %s", (AccessKeyId)));
if (Configuration->LogSensitive)
{
  FTerminal->LogEvent(FORMAT(L"Secret access key: %s", (SecretAccessKey)));
  FTerminal->LogEvent(FORMAT(L"Session token: %s", (SessionToken)));
}
```

---

## 7. NetBox Mapping Notes

| WinSCP Component | NetBox Equivalent | Status |
|-----------------|-------------------|--------|
| `TSessionData::S3SessionToken` | `TSessionData::S3SessionToken` | ❌ **Missing** — not declared in NetBox `SessionData.h` |
| `TSessionData::S3RoleArn` | `TSessionData::S3RoleArn` | ❌ **Missing** — not declared in NetBox `SessionData.h` |
| `TSessionData::S3RoleSessionName` | `TSessionData::S3RoleSessionName` | ❌ **Missing** |
| `SiteAdvanced.cpp` UI | `WinSCPDialogs.cpp` (`TSessionDialog`) | ⚠️ Partial — S3 tab exists but lacks token/ARN controls |
| `AssumeRole()` | `TS3FileSystem::AssumeRole()` | ❌ **Missing** — not implemented in NetBox |
| `S3EnvSessionToken()` | Environment reader | ❌ **Missing** |
| `S3EnvRoleArn()` | Environment reader | ❌ **Missing** |
| `SetCredentials(sessionToken)` | `TS3FileSystem::SetCredentials()` | ⚠️ Partial — NetBox `SetCredentials` lacks `SessionToken` parameter |

### 7.1 Implementation Checklist for NetBox

If adding Session Token / Role ARN support to NetBox:

- [ ] Add `FS3SessionToken`, `FS3RoleArn`, `FS3RoleSessionName` to `SessionData.h`
- [ ] Add `PROPERTY` / `RWPropertySimple` declarations
- [ ] Add `SetS3SessionToken()`, `SetS3RoleArn()`, `SetS3RoleSessionName()` setters
- [ ] Add `ReadString` / `WRITE_DATA` serialization in `DoLoad` / `DoSave`
- [ ] Add `NB_S3_SESSION_TOKEN`, `NB_S3_ROLE_ARN`, `NB_S3_ROLE_SESSION_NAME` to `MsgIDs.h`
- [ ] Add English strings to `NetBoxEng.lng`, translations to other `.lng` files
- [ ] Add `S3SessionTokenEdit` (multi-line memo) and `S3RoleArnEdit` to `TSessionDialog` S3 tab
- [ ] Wire `InitDialog()` to populate from `SessionData`
- [ ] Wire `OkClick()` to save back to `SessionData`
- [ ] Add visibility rules (disable when auto-credentials enabled)
- [ ] Extend `TS3FileSystem::SetCredentials()` to accept `SessionToken` parameter
- [ ] Add `FSecurityTokenBuf` / `FSecurityToken` member variables to `TS3FileSystem`
- [ ] Wire `FSecurityToken` into all libs3 calls (`S3_list_service`, `S3_create_bucket`, etc.)
- [ ] Implement `AssumeRole()` method in `TS3FileSystem` (or defer — complex feature)
- [ ] Add `S3EnvSessionToken()` and `S3EnvRoleArn()` helpers (or defer)
- [ ] Add IMDSv2 metadata token support (or defer)

---

## 8. Security Considerations

| Concern | WinSCP Handling | NetBox Recommendation |
|---------|-----------------|----------------------|
| Session token in logs | Logged plaintext in session info | Gate behind `LogSensitive` or debug level only |
| Role ARN in logs | Logged plaintext (not sensitive) | Acceptable to log |
| AssumeRole credentials | Logged only if `LogSensitive` enabled | Follow same pattern |
| Temporary credential expiration | Parsed from STS response but not enforced | Add expiration tracking and refresh logic |
| IMDSv2 token TTL | 6 hours hardcoded | Make configurable or document |

---

## 9. Files Referenced

| File | Purpose |
|------|---------|
| `source/core/SessionData.h` | Property declarations |
| `source/core/SessionData.cpp` | Serialization (DoLoad/DoSave), setters |
| `source/core/S3FileSystem.h` | `AssumeRole()`, `SetCredentials()` signatures |
| `source/core/S3FileSystem.cpp` | Connection flow, AssumeRole implementation, env helpers |
| `source/core/SessionInfo.cpp` | Logging of S3 auth parameters |
| `source/forms/Login.cpp` | Auto-populate from environment |
| `source/forms/SiteAdvanced.cpp` | UI load/save, visibility rules |
| `source/forms/SiteAdvanced.dfm` | UI layout |
| `source/forms/SiteAdvanced.h` | UI control declarations |

---

## 10. Related NetBox Issues

This reference was extracted to support potential future work on S3 authentication in NetBox. The current NetBox S3 implementation only supports static Access Key / Secret Key pairs. Adding Session Token and Role ARN support would enable:

- **Temporary credentials** (STS, IAM role assumption)
- **EC2 instance profiles** (via IMDSv2)
- **Cross-account access** (AssumeRole)
- **SSO/IdP workflows** (session token from identity provider)

