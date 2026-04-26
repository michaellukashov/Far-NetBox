# Reference: WinSCP SessionData Encryption Settings Study

**Date:** 2026-04-26
**Source:** `D:\Projects\WinSCP-work\winscp-master\source\core\SessionData.h` and `SessionData.cpp`
**Analyzed by:** AI agent exploration
**Related to:** S3 encryption options alignment in NetBox

---

## 1. Encryption-Related Fields in TSessionData

### 1.1 Cipher and KEX Lists

WinSCP stores ordered lists of ciphers and key exchange algorithms:

```cpp
// From SessionData.h (line 13):
enum TCipher { cipWarn, cip3DES, cipBlowfish, cipAES, cipDES, cipArcfour, cipChaCha20, cipAESGCM, cipCount };
#define CIPHER_COUNT (cipCount)

// line 18-20:
enum TKex { kexWarn, kexDHGroup1, kexDHGroup14, kexDHGroup15, kexDHGroup16, kexDHGroup17, kexDHGroup18, kexDHGEx, kexRSA, kexECDH, kexNTRUHybrid, kexMLKEM25519Hybrid, kexMLKEMNISTHybrid, kexCount };
#define KEX_COUNT (kexCount)

// TSessionData members (lines 111, 112):
TCipher FCiphers[CIPHER_COUNT];
TKex FKex[KEX_COUNT];
```

**Property accessors:**
```cpp
void __fastcall SetCipher(int Index, TCipher value);
TCipher __fastcall GetCipher(int Index) const;
// Property: __property TCipher Cipher[int Index] = { read=GetCipher, write=SetCipher };
```

**Serialization:** (enumerated via `WRITE_DATA` macros for each index)

### 1.2 TLS Version Range

```cpp
// From SessionData.h:
TTlsVersion FMinTlsVersion;
TTlsVersion FMaxTlsVersion;

enum TTlsVersion { ssl2 = 2, ssl3 = 3, tls10 = 10, tls11 = 11, tls12 = 12, tls13 = 13, tlsMin = tls10, tlsDefaultMin = tls12, tlsMax = tls13 };
```

TLS versions are shared across all TLS-based protocols (S3, WebDAV, FTPS). They are loaded/saved via standard `ReadInteger`/`WriteData` patterns.

### 1.3 S3-Specific Security Fields

WinSCP's S3 implementation includes these additional security-related parameters:

```cpp
// From SessionData.h (lines 236-244):
UnicodeString FS3DefaultRegion;
UnicodeString FS3SessionToken;
UnicodeString FS3RoleArn;
UnicodeString FS3RoleSessionName;
UnicodeString FS3Profile;
TS3UrlStyle FS3UrlStyle;
TAutoSwitch FS3MaxKeys;
bool FS3CredentialsEnv;
bool FS3RequesterPays;
// Note: No S3-specific cipher list or KEX, because S3 uses HTTPS/TLS
```

**Property macros:** Standard `RWPropertySimple<UnicodeString>` for string fields, `RWPropertySimple<TS3UrlStyle>` for enum, `RWPropertySimple<bool>` for booleans.

**NOTABLE GAP:** WinSCP does not expose a per-session custom CA certificate field for S3 in SessionData.h. TLS certificate validation uses the global CA bundle, not per-session overrides.

---

## 2. Serialization Patterns

### 2.1 Load (`DoLoad`)

Location: `SessionData.cpp` around lines 2250-2500.

**Pattern for S3 fields:**
```cpp
FS3DefaultRegion = Storage->ReadString(L"S3DefaultRegion", FS3DefaultRegion);
FS3UrlStyle = (TS3UrlStyle)Storage->ReadInteger(L"S3UrlStyle", FS3UrlStyle);
FS3CredentialsEnv = Storage->ReadBool(L"S3CredentialsEnv", FS3CredentialsEnv);
```

**Pattern for TLS versions:**
```cpp
FMinTlsVersion = (TTlsVersion)Storage->ReadInteger(L"MinTlsVersion", FMinTlsVersion);
FMaxTlsVersion = (TTlsVersion)Storage->ReadInteger(L"MaxTlsVersion", FMaxTlsVersion);
```

**Pattern for Cipher/KEX arrays:** Loop over indices and `ReadInteger(L"cipher" + IntToStr(I), FCiphers[I])`.

### 2.2 Save (`DoSave`)

Location: `SessionData.cpp` around lines 1100-1400 (non-Putty path).

**Pattern using WRITE_DATA macro:**
```cpp
WRITE_DATA(String, S3DefaultRegion);
WRITE_DATA(Integer, S3UrlStyle);
WRITE_DATA(Bool, S3CredentialsEnv);
WRITE_DATA(Integer, MinTlsVersion);
WRITE_DATA(Integer, MaxTlsVersion);
```

**Pattern for cipher list:** Loop with `WRITE_DATA(Integer, Cipher[i])`.

---

## 3. Property Declaration Style

In `SessionData.h`, properties are declared using macro blocks:

```cpp
// Example from around lines 430-440:
void __fastcall SetS3UrlStyle(TS3UrlStyle value);
// ...
__property TS3UrlStyle S3UrlStyle = { read = FS3UrlStyle, write = SetS3UrlStyle };

// Another macro style (may vary):
PROPERTY(S3CredentialsEnv);
```

The exact macro depends on the codebase region; newer properties use explicit getter/setter pairs, older ones use `PROPERTY()`.

---

## 4. UI Dialog Handling (WinSCP)

WinSCP's S3 encryption options are exposed in the **TLS page** of the Site Advanced dialog (`forms/SiteAdvanced.cpp`). The dialog includes:

- **Cipher list box** (`CipherListBox`) — reorderable list of available ciphers (for SFTP/SCP)
- **KEX list box** (`KexListBox`) — reorderable list of key exchange algorithms (for SFTP/SCP)
- **TLS version selectors** (`MinTlsVersionCombo`, `MaxTlsVersionCombo`) — for FTPS, WebDAV, and S3 (these protocols share the same TLS controls)
- **"TLS/SSL Implicit" checkbox** — for FTPS only; S3 always uses explicit TLS (HTTPS)

**S3-specific options** in the dialog include:
- **CA certificate file** — If S3CACertificate property existed, this would be a text edit + browse button (but WinSCP does not have a per-session custom CA for S3; global certificate store is used)
- **Region, URL style, credentials environment** — these are in the dedicated **S3 page**, not on the TLS page

**Property linkage:**
```cpp
// S3 tab fields map to TSessionData properties:
S3DefaultReqionCombo->Text = FSessionData->S3DefaultRegion;
```

---

## 5. TLS Initialization Pattern

In protocols using HTTPS (S3, WebDAV), the TLS setup is centralized in `SetupSsl()` (e.g., `NeonIntf.cpp`).

**Steps:**
1. Call `SSL_CTX_new(TLS_client_method())`
2. Set security level via `SSL_CTX_set_security_level()`
3. Restrict protocols via `SSL_CTX_set_min_proto_version()` and `SSL_CTX_set_max_proto_version()` based on `MinTlsVersion`/`MaxTlsVersion`
4. Register `SSL_CTX_set_verify()` callback for certificate validation
5. **No per-session cipher list** — OpenSSL defaults controlled by security level and protocol version
6. **No custom CA per session** — uses embedded `cacert.pem` bundle

**Caveat:** WinSCP's S3 implementation (via neon) does NOT currently support loading a custom CA certificate from a per-session setting; the TODO in NetBox's `S3FileSystem.cpp` confirms this is unmet even there.

---

## 6. Key Takeaways for NetBox Alignment

|Item|WinSCP|NetBox Action|
|---|---|---|
|SessionData encryption fields|Cipher array, KEX array (SFTP only); Min/MaxTLS (shared); no S3 CA cert|Add S3CACertificate serialization; expose Min/MaxTLS in S3 tab UI|
|Property style|`RWPropertySimple` or `PROPERTY`|Use NetBox equivalents (`SESSION_PROPERTY`, `PROPERTY2` pattern)|
|Serialization|`WRITE_DATA`/`ReadString`/`ReadInteger`|Follow same macros in `DoLoad`/`DoSave`|
|UI for S3 encryption|TLS page controls shared across protocols; S3 page for region/URL style|Add TLS version combos to S3 tab (S3 has its own tab in NetBox)|
|CA certificate per session|Not implemented globally|Implement as extension (NetBox-specific enhancement)|
|Cipher selection for TLS protocols|Not exposed (OpenSSL default)|Do NOT add - follow WinSCP pattern (no per-protocol cipher list for S3)|
|Defaults|TLS 1.2 min default, max TLS 1.3|Use same defaults|
|Error handling|Warnings for downgrade, certificate validation failures|Add verbose logging and user notifications|

---

## 7. Gaps Between WinSCP and NetBox

1. **S3CACertificate not serialized** — NetBox declared property but missing from `DoLoad`/`DoSave`
2. **S3CACertificate not applied** — NetBox has TODO to implement via `ne_ssl_set_certificates_storage()`
3. **TLS version UI missing for S3** — WinSCP exposes on TLS page; NetBox has no UI controls for Min/Max TLS on S3 tab
4. **UI location differs** — WinSCP uses separate TLS page; NetBox uses tabbed dialog per protocol

---

## 8. Code References (WinSCP)

|File|Key Lines|What|
|---|---|---|
|`SessionData.h`|13-20|TCipher, TKex enums|
|`SessionData.h`|106+|class TSessionData definition and properties|
|`SessionData.cpp`|~2250-2500|DoLoad serialization patterns|
|`SessionData.cpp`|~1100-1400|DoSave serialization patterns|
|`forms/SiteAdvanced.cpp`|~270+|Cipher/KEX list population|
|`forms/SiteAdvanced.dfm`|TLS version combo controls|
|`core/WebDAVFileSystem.cpp`|~2188-2195|SetupSsl example for HTTPS Protocol|
|`neon` integration|`SetNeonTlsInit`|TLS callback registration|

---

## 9. Verbatim Property Examples (WinSCP)

```cpp
// String property with setter (SessionData.cpp):
void __fastcall TSessionData::SetS3DefaultRegion(UnicodeString value)
{
  SET_SESSION_PROPERTY(S3DefaultRegion);
}

// Macro used (SessionData.h):
__property UnicodeString S3DefaultRegion = { read = FS3DefaultRegion, write = SetS3DefaultRegion };
```

**Serialization macro:**
```cpp
WRITE_DATA(String, S3DefaultRegion);  // Save
FS3DefaultRegion = Storage->ReadString(L"S3DefaultRegion", FS3DefaultRegion);  // Load
```

---

## 10. Consistency Rules for NetBox (derived)

1. **Use existing property macros** — NetBox uses `SESSION_PROPERTY` and `RWPropertySimple`. Follow patterns seen in `SessionData.h` for other S3 fields.
2. **Load defaults with fallback** — Use `Storage->ReadString(L"S3CACertificate", FS3CACertificate)` (second parameter is default)
3. **Save via WRITE_DATA** — Apply to new fields in `DoSave` non-Putty branch
4. **Expose TLS versions in UI with combo boxes** — Populate with "TLS 1.0", "TLS 1.1", "TLS 1.2", "TLS 1.3"; store enum `TTlsVersion`
5. **Validate range in UI** — Max must be >= Min; show warning on invalid
6. **Default to TLS 1.2 min, 1.3 max** if user hasn't set
7. **No cipher list for S3** — Do not add per-session cipher selection; rely on OpenSSL defaults via `SetupSsl()`
8. **Apply custom CA via `ne_ssl_set_certificates_storage()`** — Write cert to temp file before session creation

---

*End of Reference*
