# OpenSSH Certificate-Based Authentication Exploration

**Date:** 2026-05-03  
**Updated:** 2026-05-03 (WinSCP master cross-reference)

## Related GitHub Issues

- [#36](https://github.com/michaellukashov/Far-NetBox/issues/36) — Support for Unix-style SSH keys as well as PuTTY-style keys
- [#263](https://github.com/michaellukashov/Far-NetBox/issues/263) — Error when connecting to new OpenSSH servers
- [#323](https://github.com/michaellukashov/Far-NetBox/issues/323) — SFTP key auth is broken for OpenSSH 8.7p1 & 8.8p1
- [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — User-provided auth certificate
- [#388](https://github.com/michaellukashov/Far-NetBox/issues/388) — RSA-SHA256 support
- [#156](https://github.com/michaellukashov/Far-NetBox/issues/156) — Diffie-Hellman group exchange KEX is obsolete

## Key Directories and Files

### Core SSH Implementation

| File | Purpose |
|------|---------|
| `src/core/SecureShell.h` | `TSecureShell` — main SSH shell class wrapping PuTTY, handles authentication |
| `src/core/SecureShell.cpp` | SSH session management, `StoreToConfig()` → PuTTY Conf, key conversion |
| `src/core/SessionData.h` | `TSessionData` — central session config with SSH auth fields |
| `src/core/SessionData.cpp` | `TSessionData` implementation, passphrase encryption/decryption, serialization |
| `src/core/PuttyIntf.h` | PuTTY integration layer, callback bridge functions |
| `src/core/PuttyIntf.cpp` | `GetKeyType()`, `LoadKey()`, `SaveKey()`, `FreeKey()`, `ConvertKeyToTemporaryPPK()` |
| `src/core/PuttyTools.h` | PuTTY helper function declarations |
| `src/core/SftpFileSystem.h` | `TSFTPFileSystem` — SFTP using `TSecureShell` |
| `src/core/ScpFileSystem.h` | `TSCPFileSystem` — SCP using `TSecureShell` |

### UI and Configuration

| File | Purpose |
|------|---------|
| `src/NetBox/WinSCPDialogs.cpp` | `TSessionDialog` (tabbed config), `TPasswordDialog`, OpenSSH cert controls |
| `src/NetBox/FarDialog.h` | Base classes: `TFarDialog`, `TTabbedDialog`, `TFarEdit`, `TFarCheckBox`, etc. |
| `src/windows/GUIConfiguration.h` | `TGUIConfiguration` — GUI settings including PuTTY paths |
| `src/NetBox/FarConfiguration.cpp` | Far Manager config storage |

### PuTTY Library

| File | Purpose |
|------|---------|
| `libs/putty/ssh/userauth2-client.c` | Core userauth: `ssh2_userauth_new()`, `ppk_load_f()` at line 1282 |
| `libs/putty/crypto/openssh-certs.c` | OpenSSH certificate parsing, validation (sig, CA, time, principals) |
| `libs/putty/import.c:144` | `import_ssh2()` — converts OpenSSH → internal `ssh2_userkey` |
| `libs/putty/ssh/sshpubk.c` | Key file parsing: `key_type()`, `ppk_load_f()`, `ppk_save_f()` |
| `libs/putty/conf.h` | Config options: `CONF_detached_cert`, `CONF_keyfile` |

## Existing Authentication Patterns

### Session Data Fields (TSessionData)

| Field | Type | Purpose |
|-------|------|---------|
| `FAuthKI` | bool | Keyboard-interactive authentication |
| `FAuthGSSAPI` | bool | GSSAPI authentication |
| `FTryAgent` | bool | Try Pageant/SSH agent |
| `FPublicKeyFile` | UnicodeString | Path to PPK private key file |
| `FPassphrase` | UnicodeString | Passphrase for encrypted keys (encrypted with key file path) |
| `FDetachedCertificate` | UnicodeString | OpenSSH certificate file path |
| `FOpensshPrivateKeyFile` | UnicodeString | OpenSSH-format private key file path (NetBox-specific) |
| `FUseOpensshCertificate` | bool | Enable/disable OpenSSH certificate auth (NetBox-specific) |

### Passphrase Encryption

```cpp
// Encryption key is the effective key file path
void TSessionData::SetPassphrase(const UnicodeString & AValue)
{
  const RawByteString value = EncryptPassword(AValue, GetEffectiveKeyFile());
  SET_SESSION_PROPERTY(Passphrase);
}

UnicodeString TSessionData::GetPassphrase() const
{
  return DecryptPassword(FPassphrase, GetEffectiveKeyFile());
}

UnicodeString TSessionData::GetEffectiveKeyFile() const
{
  // When cert mode active and OpensshPrivateKeyFile set → use it
  // Otherwise → use PublicKeyFile
  if (GetUseOpensshCertificate() && !GetOpensshPrivateKeyFile().IsEmpty())
  {
    return GetOpensshPrivateKeyFile();
  }
  return GetPublicKeyFile();
}
```

Re-encryption occurs when `SetPublicKeyFile()`, `SetOpensshPrivateKeyFile()`, or `SetUseOpensshCertificate()` changes the effective key path.

### Storage Pattern

- `THierarchicalStorage` abstraction (registry + XML via `TFar3Storage` / `TXmlStorage`)
- Passwords encrypted via `LOAD_PASSWORD`/`SAVE_PASSWORD` macros
- Fields use `PROPERTY2()` and `PROPERTY()` macros for getter/setter generation

## PuTTY Certificate Support (Deep Analysis)

### Overview

PuTTY 0.81 includes **full OpenSSH certificate support** for SSH user authentication.

### Certificate Validation (Handled by PuTTY)

| Check | Description |
|-------|-------------|
| Signature validity | Verifies cert signature against CA public key |
| CA key validity | Ensures signing key is not itself a certificate |
| Certificate type | Validates user vs host certificate |
| Time bounds | Checks `valid_after` and `valid_before` timestamps |
| Principal matching | Verifies username matches one of the cert's principals |
| Critical options | Partially interprets force-command, source-address; rejects unknown |
| Key match | Certificate's base public key must match loaded private key |

### Error Messages (from PuTTY)

- Certificate signature verification failure
- CA key is itself a certificate (rejected)
- Certificate type mismatch (user vs host)
- Certificate expired or not yet valid
- Principal mismatch (username not in certificate)
- Unknown critical option (causes rejection)
- Certificate does not match private key

### Private Key Format Limitation (CRITICAL)

**PuTTY only accepts PPK format (`SSH_KEYTYPE_SSH2`) for private keys** in the authentication flow:

- `ppk_load_f()` at `userauth2-client.c:1282` handles private key loading
- OpenSSH PEM/New (`OPENSSH_PEM`, `OPENSSH_NEW`) and SSHCOM formats are **NOT auto-converted**
- `import_ssh2()` exists in `import.c:144` for conversion but is NOT used in auth flow
- `key_type()` at `sshpubk.c:2014` can detect formats but detection ≠ conversion

### Resolution: Runtime Conversion

Convert OpenSSH keys → PPK at runtime before `CONF_keyfile` is set:

```cpp
// In TSecureShell::StoreToConfig()
UnicodeString EffectiveKeyFile = Data->ResolveEffectiveKeyFile();
if (!EffectiveKeyFile.IsEmpty())
{
  const TKeyType KeyType = GetKeyType(EffectiveKeyFile);
  if ((KeyType == ktOpenSSHPEM) || (KeyType == ktOpenSSHNew) || (KeyType == ktSSHCom))
  {
    // Silent conversion: OpenSSH → PPK temp file
    EffectiveKeyFile = ConvertKeyToTemporaryPPK(EffectiveKeyFile, Data->GetPassphrase());
    // Track temp file for cleanup
    if (EffectiveKeyFile != Data->ResolveEffectiveKeyFile())
    {
      FTempPPKFile = EffectiveKeyFile;
    }
  }
}
conf_set_filename(conf, CONF_keyfile, EffectiveKeyFile);
```

## NetBox Alignment with WinSCP

### WinSCP Master Cross-Reference

WinSCP master source (`D:\Projects\WinSCP-work\winscp-master\source`) was cross-referenced to validate the NetBox implementation:

| Finding | WinSCP Master | NetBox |
|---------|---------------|--------|
| `OpensshPrivateKeyFile` field | ❌ Not present | ✅ Present (NetBox-specific) |
| `UseOpensshCertificate` field | ❌ Not present | ✅ Present (NetBox-specific) |
| `DetachedCertificate` field | ✅ Present | ✅ Present (shared) |
| UI controls for OpenSSH cert | ❌ Not present | ✅ Present (NetBox-specific) |
| Interactive key conversion | ✅ `VerifyAndConvertKey()` in GUI | ❌ Not applicable (Far plugin) |
| Silent conversion utility | ❌ Not present | ✅ `ConvertKeyToTemporaryPPK()` (NetBox) |

**Conclusion:** `OpensshPrivateKeyFile` and `UseOpensshCertificate` are NetBox-specific additions. WinSCP handles OpenSSH keys via interactive conversion in the GUI layer before connecting. NetBox must convert silently since Far plugins cannot show dialogs during session connect.

### NetBox Conversion Utility

```cpp
// PuttyIntf.cpp — ConvertKeyToTemporaryPPK()
UnicodeString ConvertKeyToTemporaryPPK(const UnicodeString & FileName,
  const UnicodeString & Passphrase)
{
  const TKeyType KeyType = GetKeyType(FileName);
  
  // Already PPK — no conversion needed
  if (KeyType == ktSSH2) return FileName;
  
  // Unsupported format — pass through
  if ((KeyType != ktOpenSSHPEM) && (KeyType != ktOpenSSHNew) && (KeyType != ktSSHCom))
    return FileName;
  
  // Generate unique temp file in %TEMP%\nbc_<basename>_NNN.ppk
  UnicodeString UniqueTempFileName = GenerateUniqueTempPPK(FileName);
  
  // Convert: OpenSSH → ssh2_userkey → PPK
  TPrivateKey * PrivateKey = LoadKey(KeyType, FileName, Passphrase);
  SaveKey(ktSSH2, UniqueTempFileName, Passphrase, PrivateKey);
  FreeKey(PrivateKey);
  
  return UniqueTempFileName;
}
```

### Temp File Lifecycle

```
TSecureShell::StoreToConfig()
  → ConvertKeyToTemporaryPPK() → FTempPPKFile
  → conf_set_filename(conf, CONF_keyfile, FTempPPKFile)

TSecureShell::Close()
  → DeleteFile(FTempPPKFile)
  → FTempPPKFile = L""

TSecureShell::~TSecureShell()
  → DeleteFile(FTempPPKFile) (safety net)
  → FTempPPKFile = L""
```

## Implementation Files

| Task | Files | Status |
|------|-------|--------|
| TASK-1b: Passphrase encryption fix | `SessionData.h`, `SessionData.cpp` | ✅ Complete |
| TASK-3: Silent conversion utility | `PuttyTools.h`, `PuttyIntf.cpp` | ✅ Complete |
| TASK-4: Wire in StoreToConfig | `SecureShell.h`, `SecureShell.cpp` | ✅ Complete |
| TASK-10: Temp file cleanup | `SecureShell.h`, `SecureShell.cpp` | ✅ Complete |

## Build Verification

- **Build:** Passes with zero new warnings
- **Platform:** x64 RelWithDebugInfo
- **Compiler:** MSVC /W4

## Data Flow (Complete)

```
┌─────────────────────────────────────────────────────────────┐
│  TSessionDialog (WinSCPDialogs.cpp)                           │
│  ─────────────────────────────────────────                │
│  When user enables [✓] Use OpenSSH certificate:              │
│    • OpensshKeyEdit   → FOpensshPrivateKeyFile               │
│    • OpensshCertEdit  → FDetachedCertificate                 │
│    • UseOpensshCertCheck → FUseOpensshCertificate            │
│  Save to registry/XML via TSessionData::DoSave()             │
│  Passphrase encrypted with OpensshPrivateKeyFile path       │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  TSecureShell::StoreToConfig()                                │
│  ─────────────────────────────────────────                │
│  1. ResolveEffectiveKeyFile() → OpensshPrivateKeyFile        │
│  2. GetKeyType() → ktOpenSSHPEM/New                          │
│  3. ConvertKeyToTemporaryPPK() → %TEMP%\nbc_*.ppk          │
│  4. CONF_keyfile = temp PPK                                   │
│  5. CONF_detached_cert = certificate file                   │
│  6. FTempPPKFile = temp path (for cleanup)                  │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  PuTTY 0.81 auth (userauth2-client.c:1282)                   │
│  ─────────────────────────────────────────                │
│  ppk_load_f() → accepts PPK ✅                               │
│  openssh-certs.c → validates cert + key match ✅             │
│  All certificate logic is already in PuTTY                  │
└─────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────┐
│  TSecureShell::Close() / ~TSecureShell()                     │
│  ─────────────────────────────────────────                │
│  → DeleteFile(FTempPPKFile)                                   │
│  → FTempPPKFile = L""                                         │
└─────────────────────────────────────────────────────────────┘
```

## Edge Cases

1. **Expired certificate** — PuTTY rejects with error; captured via ScpSeat callbacks
2. **Revoked certificate** — No CRL support; PuTTY doesn't check CRLs
3. **Unknown CA** — PuTTY validates cert signature independently
4. **Missing private key** — Error before connection attempt
5. **Certificate/key mismatch** — PuTTY validates match during auth
6. **OpenSSH key format detection** — `GetKeyType()` handles OPENSSH_PEM, OPENSSH_NEW, SSHCOM
7. **Certificate with multiple principals** — PuTTY matches against username
8. **Temp PPK file left after crash** — Cleanup in destructor; `FILE_FLAG_DELETE_ON_CLOSE` possible enhancement
9. **Both PPK and OpenSSH key configured** — OpenSSH key preferred when cert auth enabled
10. **Concurrent connections with cert auth** — Each session gets unique temp PPK via counter dedup
