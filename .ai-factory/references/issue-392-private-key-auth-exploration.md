# SSH Private Key Authentication Failure — Issue #392 Exploration

**GitHub Issue:** [#392](https://github.com/michaellukashov/Far-NetBox/issues/392) "Unable to connect with private key certificates"  
**Date:** 2026-05-02  
**Scope:** SSH/SFTP/SCP public-key authentication flow  

---

## Issue Summary

Users cannot connect using private key files (PPK or OpenSSH format) while WinSCP succeeds with the same key and host. Two failure modes:

1. **"Authorizing" blinks briefly, then falls back to session menu** — original report, OracleCloud Ubuntu
2. **`No supported authentication methods available (server sent: publickey)`** — NB 24.12.2.608

Additionally, NetBox asks for a password even when an explicit key file is configured.

---

## Key Files and Functions

| File | Role | Key Lines / Symbols |
|------|------|---------------------|
| `src/core/SecureShell.cpp` | SSH session glue — configures PuTTY, handles auth prompts | `StoreToConfig()` (line 200), `PromptUser()` (line ~750), `GotHostKey()` (line 1063), `FatalError()` (line 668) |
| `src/core/SessionData.cpp` | Session configuration — key file paths, passphrase | `ResolvePublicKeyFile()` (line 3536), `SetPublicKeyFile()` (line 3500), `DisableAuthenticationsExceptPassword()` (line 5269) |
| `src/core/PuttyIntf.cpp` | PuTTY interface — key loading and type detection | `GetKeyType()` (line 797), `LoadKey()` (line 854), `TestKey()` (line 923) |
| `src/core/SecureShell.h` | SecureShell class declaration | `FAuthenticating`, `FAuthenticated`, `FAuthenticationLog` flags |
| `src/windows/Tools.cpp` | Key verification and auto-conversion | `DoVerifyKey()` (line 1369), `VerifyAndConvertKey()` (line 1463), `AddMatchingKeyCertificate()` (line 1292) |
| `src/NetBox/WinSCPDialogs.cpp` | Session dialog UI — key file controls | `PrivateKeyEdit` (line 2218), `OpensshKeyEdit` (line 3197), `OpensshCertEdit` (line 3191) |
| `libs/putty/ssh/userauth2-client.c` | PuTTY SSH2 userauth implementation | `ssh2_userauth_process_queue()` (line 474), key loading at auth start (lines 499-531), auth method decision (lines 1182-1200), error `SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE` (line 2116) |

---

## Authentication Flow

```
NetBox SessionDialog / XML config
    ↓
TSessionData::PublicKeyFile (UnicodeString)
    ↓
TSessionData::ResolvePublicKeyFile()  → expand env vars, strip quotes
    ↓
TSecureShell::StoreToConfig()
    conf_set_filename(conf, CONF_keyfile, ...)   [SecureShell.cpp:288]
    ↓
PuTTY ssh2_userauth_process_queue()
    ppk_loadpub_f(keyfile, ...) → publickey_blob
    ↓
IF publickey_blob != NULL AND privatekey_available == TRUE
    → try publickey auth with configured key
ELSE
    → no publickey method available
    ↓
Server offers: publickey
Client has:   (none loaded)
    ↓
SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE
"No supported authentication methods available"
```

---

## Suspected Root Causes (Ordered by Likelihood)

### 1. Passphrase Prompt Misclassification

Encrypted PPK keys require a passphrase. PuTTY requests this via `seat_get_userpass_input()`. NetBox's `TSecureShell::PromptUser()` (line ~750) handles all auth prompts. The prompt kind is determined by `PromptKind` enum (`pkPassword`, `pkPassphrase`, `pkKeybInteractive`, etc.).

**Risk:** The passphrase prompt may be misclassified as a generic password prompt, causing the user to cancel or the auth to abort. The stored passphrase in `SessionData::Passphrase` (encrypted with `PublicKeyFile` as key) may not be passed to PuTTY before auth begins.

**Evidence needed:** Log `PromptKind` value and description text for every prompt during auth.

### 2. Path Encoding / Unicode / Format Issue

Key file path flow: `UnicodeString` → `UTF8String` → `filename_from_utf8()` → PuTTY `Filename*` → `ppk_loadpub_f()`.

**Risk:** Non-ASCII paths, paths with spaces, or relative paths may fail silently in `filename_from_utf8()` or `ppk_loadpub_f()`. The `ResolvePublicKeyFile()` function expands environment variables and strips quotes, but edge cases (UNC paths, long paths) may not be handled correctly.

**Evidence needed:** Log resolved path, `GetKeyType()` result, and whether the file is readable before auth begins.

### 3. Missing Upstream WinSCP Fix

WinSCP 6.5.6 uses the same PuTTY 0.81 version. Differences must be in NetBox's glue code (`SecureShell.cpp`, `Terminal.cpp`, `SessionData.cpp`).

**Risk:** WinSCP may have a patch in its `SecureShell.cpp` or `Terminal.cpp` that NetBox hasn't ported — e.g., additional auth prompt handling, key file path normalization, or passphrase pre-population.

**Evidence needed:** Diff `SecureShell.cpp`, `Terminal.cpp`, `SessionData.cpp` against WinSCP 6.5.6 upstream for auth-related changes.

### 4. Auto-Conversion Interference

`DoVerifyKey()` in `Tools.cpp` auto-converts OpenSSH keys to PPK when the session dialog is saved. Sessions created via import or command line may bypass this.

**Note:** PuTTY 0.81 natively supports OpenSSH keys, so conversion should not be required. The `GetKeyType()` function detects `ktOpenSSHPEM`, `ktOpenSSHNew`, and `ktSSHCom` formats, which are then handled by `import_ssh2()` in `LoadKey()`. This should work.

---

## Relevant PuTTY Auth Code

### Key Loading at Auth Start (userauth2-client.c:499-531)

```c
if (!filename_is_null(s->keyfile)) {
    int keytype;
    ppl_logevent(WINSCP_BOM "Reading key file \"%s\"", filename_to_str(s->keyfile));
    keytype = key_type(s->keyfile);
    if (keytype == SSH_KEYTYPE_SSH2 ||
        keytype == SSH_KEYTYPE_SSH2_PUBLIC_RFC4716 ||
        keytype == SSH_KEYTYPE_SSH2_PUBLIC_OPENSSH) {
        const char *error;
        s->publickey_blob = strbuf_new();
        if (ppk_loadpub_f(s->keyfile, &s->publickey_algorithm,
                          BinarySink_UPCAST(s->publickey_blob),
                          &s->publickey_comment, &error)) {
            s->privatekey_available = (keytype == SSH_KEYTYPE_SSH2);
            ...
        } else {
            ppl_logevent("Unable to load key (%s)", error);
            ...
            s->publickey_blob = NULL;
        }
    } else {
        ppl_logevent("Unable to use this key file (%s)", key_type_to_str(keytype));
        ...
        s->publickey_blob = NULL;
    }
}
```

### Auth Method Decision (userauth2-client.c:1182-1200)

```c
if (s->can_pubkey && s->publickey_blob &&
    s->privatekey_available && !s->tried_pubkey_config) {
    // Try the public key supplied in the configuration
    s->tried_pubkey_config = true;
    ...
}
```

If `publickey_blob` is NULL (key failed to load) OR `privatekey_available` is false (key is public-only), the client cannot attempt publickey auth with the configured key.

### Error Origin (userauth2-client.c:2116-2121)

```c
ssh_bpp_queue_disconnect(
    s->ppl.bpp,
    "No supported authentication methods available",
    SSH2_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE);
ssh_sw_abort(s->ppl.ssh, "No supported authentication methods "
             "available (server sent: %s)",
             s->last_methods_string->s);
```

This is emitted when the client has exhausted all auth methods and none intersect with the server's offered methods.

---

## NetBox-Specific Findings

### `DisableAuthenticationsExceptPassword()` (SessionData.cpp:5269)

```cpp
void TSessionData::DisableAuthenticationsExceptPassword()
{
    FSshNoUserAuth = false;
    FAuthKI = false;
    FAuthKIPassword = false;
    FAuthGSSAPI = false;
    FAuthGSSAPIKEX = false;
    PublicKeyFile = EmptyStr;
    DetachedCertificate = EmptyStr;
    TlsCertificateFile = EmptyStr;
    Passphrase = EmptyStr;
    FTryAgent = false;
}
```

This function clears the key file and passphrase when switching to password-only mode. If called unexpectedly (e.g., during session import or dialog initialization), it would break key auth.

### `SetPassphrase()` Encryption Key Dependency (SessionData.cpp:3548-3557)

```cpp
void TSessionData::SetPassphrase(const UnicodeString & AValue)
{
    const RawByteString value = EncryptPassword(AValue, GetPublicKeyFile());
    SET_SESSION_PROPERTY(Passphrase);
}
```

The passphrase is encrypted using `PublicKeyFile` as the encryption key. If `PublicKeyFile` changes after the passphrase is set, the stored encrypted passphrase becomes undecryptable.

### `ResolvePublicKeyFile()` (SessionData.cpp:3536-3546)

```cpp
UnicodeString TSessionData::ResolvePublicKeyFile()
{
    UnicodeString Result = PublicKeyFile;
    if (Result.IsEmpty())
    {
        Result = GetConfiguration()->DefaultKeyFile;
    }
    Result = StripPathQuotes(::ExpandEnvironmentVariables(Result));
    return Result;
}
```

Falls back to `DefaultKeyFile` if `PublicKeyFile` is empty. Returns the resolved path with expanded environment variables and stripped quotes.

---

## Recommended Diagnostic Actions

1. **Add logging to `ResolvePublicKeyFile()`** — log the raw `PublicKeyFile`, the resolved path, and whether the file exists
2. **Add logging to `GetKeyType()` / `LoadKey()`** — log the detected key type and any error during load
3. **Add logging to `PromptUser()`** — log the `PromptKind` enum value and description for every auth prompt
4. **Capture PuTTY `ppl_logevent()` during auth** — route `"Reading key file..."`, `"Unable to load key..."`, `"Server offered these authentication methods..."` to NetBox log
5. **Compare with WinSCP upstream** — diff `SecureShell.cpp`, `Terminal.cpp`, `SessionData.cpp` for auth-related changes

---

## Related References

- [Plan: Fix SSH private key auth](../plans/fix-ssh-private-key-cert-392.md)
- [PuTTY SSH userauth source](../../libs/putty/ssh/userauth2-client.c)
