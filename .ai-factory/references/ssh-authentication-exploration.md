# SSH Authentication Exploration Results

**Date:** 2026-04-26
**Updated:** 2026-04-26 (deep codebase analysis — aif-improve)
**Topic:** OpenSSH Certificate-Based Authentication Integration
**Related Plan:** [openssh-certificate-auth.md](../plans/openssh-certificate-auth.md)
**Architecture:** [ARCHITECTURE.md](../ARCHITECTURE.md)
**UI Patterns:** [session-config-ui-patterns.md](./session-config-ui-patterns.md)

## Related GitHub Issues

- [#263](https://github.com/michaellukashov/Far-NetBox/issues/263) — Error when connecting to new OpenSSH servers
- [#323](https://github.com/michaellukashov/Far-NetBox/issues/323) — SFTP key auth is broken for OpenSSH 8.7p1 & 8.8p1
- [#156](https://github.com/michaellukashov/Far-NetBox/issues/156) — Diffie-Hellman group exchange KEX is obsolete
- [#509](https://github.com/michaellukashov/Far-NetBox/issues/509) — User-provided auth certificate
- [#388](https://github.com/michaellukashov/Far-NetBox/issues/388) — RSA-SHA256 support
- [#36](https://github.com/michaellukashov/Far-NetBox/issues/36) — Support for Unix-style SSH keys as well as PuTTY-style keys

## Key Directories and Files

### Core SSH Implementation

| File | Purpose |
|------|---------|
| `src/core/SecureShell.h` | `TSecureShell` — main SSH shell class wrapping PuTTY, handles authentication via `PromptUser`, `VerifyHostKey`, etc. |
| `src/core/SessionData.h` | `TSessionData` — central session configuration class with all SSH auth fields (`FAuthKI`, `FAuthKIPassword`, `FAuthGSSAPI`, `FTryAgent`, `FPublicKeyFile`, `FPassphrase`, `FDetachedCertificate`, etc.) |
| `src/core/PuttyIntf.h` | PuTTY integration layer: `ScpSeat` implementation, callback bridge functions (`get_userpass_input`, `confirm_ssh_host_key`, etc.) |
| `src/core/PuttyTools.h` | PuTTY helper functions: key loading, GSSAPI detection, cipher lists, host key conversion |
| `src/core/SftpFileSystem.h` | `TSFTPFileSystem` — SFTP file system implementation using `TSecureShell` |
| `src/core/ScpFileSystem.h` | `TSCPFileSystem` — SCP file system implementation using `TSecureShell` |
| `src/core/KittyKeyboard.h` | Kitty keyboard protocol support for specialized keyboard handling |

### UI and Configuration

| File | Purpose |
|------|---------|
| `src/NetBox/WinSCPDialogs.cpp` | `TSessionDialog` (tabbed session configuration UI) and `TPasswordDialog` (authentication prompt UI) |
| `src/NetBox/WinSCPFileSystem.h` | `TWinSCPFileSystem` — plugin file system, creates session dialogs and manages connections |
| `src/NetBox/FarDialog.h` | Base classes: `TFarDialog`, `TTabbedDialog`, and control types (`TFarEdit`, `TFarCheckBox`, `TFarComboBox`, etc.) |
| `src/windows/GUIConfiguration.h` | `TGUIConfiguration` — GUI-specific settings including PuTTY paths |
| `src/NetBox/FarConfiguration.cpp` | Stores and retrieves PuTTY/Pageant integration paths |
| `src/core/SessionData.cpp` | `Load`/`Save` methods using storage macros and `LOAD_PASSWORD`/`SAVE_PASSWORD` for credentials |
| `src/NetBox/XmlStorage.cpp` | `TXmlStorage` for XML-based persistence (import/export) |

### Utility and Core Libraries

| Directory | Purpose |
|-----------|---------|
| `src/nbcore/` | Native utility library (string, memory, system helpers) |
| `src/base/` | Foundation classes (`UnicodeString`, `Classes`, `Exceptions`) |
| `src/include/` | Public headers (`nbtypes.h`, `rtti.hpp`) |

## Existing Authentication Patterns

### Session Data Fields (TSessionData)

The `TSessionData` class already contains authentication-related fields:

```cpp
bool FAuthKI{false};              // Keyboard-interactive authentication
bool FAuthKIPassword{false};      // KI with password fallback
bool FAuthGSSAPI{false};          // GSSAPI authentication
bool FTryAgent{false};            // Try Pageant/SSH agent
UnicodeString FPublicKeyFile;     // Path to public key file
UnicodeString FPassphrase;        // Passphrase for encrypted keys
UnicodeString FDetachedCertificate; // Detached certificate (existing field)
UnicodeString FS3CACertificate;   // S3 CA certificate
UnicodeString FTunnelPublicKeyFile; // Tunnel public key file
RawByteString FTunnelPassphrase;    // Tunnel passphrase
```

### Storage Pattern

- Session data is persisted via `THierarchicalStorage` abstraction
- `TFar3Storage` — registry-based storage (Windows)
- `TXmlStorage` — XML-based persistence (import/export)
- Passwords are encrypted using `LOAD_PASSWORD`/`SAVE_PASSWORD` macros
- Field serialization uses explicit `Storage->Read*()`/`Storage->Write*()` calls

### PuTTY Integration

- `ScpSeat` implements PuTTY's `Seat` interface for callbacks
- Authentication flow bridges:
  - `get_userpass_input` — handles username/password/key prompts
  - `confirm_ssh_host_key` — host key verification
  - `notify_remote_exit` — connection termination handling
- Key loading uses PuTTY's `ssh_key` API via `PuttyTools.h` helpers

## PuTTY Certificate Support (Deep Analysis)

### Overview

PuTTY 0.81 includes **full OpenSSH certificate support** for SSH user authentication. The implementation is in `libs/putty/crypto/openssh-certs.c` and is wired through the userauth layer.

### Certificate Loading Flow

```
TSessionData.FDetachedCertificate
    ↓ StoreToConfig() → conf_set_filename(CONF_detached_cert)
    ↓ backend_init() → ssh2_userauth_new(keyfile, detached_cert, ...)
    ↓ ssh2_userauth layer (userauth2-client.c)
    ↓ ppk_loadpub_f() loads detached cert file
    ↓ Certificate blob extracted and stored in detached_cert_blob
    ↓ During publickey auth: cert blob replaces base public key blob
```

### Key Files in PuTTY

| File | Purpose |
|------|---------|
| `libs/putty/ssh/ppl.h` | Defines `ssh2_userauth_new()` signature; includes `detached_cert` parameter |
| `libs/putty/ssh/userauth2-client.c` | Core userauth implementation: loads detached certificate via `ppk_loadpub_f`, validates it matches the private key, substitutes certificate blob in publickey auth packets |
| `libs/putty/crypto/openssh-certs.c` | OpenSSH certificate parsing, validation, and error checking; implements the full certificate v01@openssh.com format for DSA, RSA, RSA-SHA2, ECDSA, Ed25519 |
| `libs/putty/conf.h` | Configuration option definition: `CONF_detached_cert` (FILENAME, saved as 'DetachedCertificate') |
| `libs/putty/ssh/sshpubk.c` | Key file parsing: `ppk_loadpub_f()` supports RFC4716 and OpenSSH public key formats; determines key type including certificates |
| `libs/putty/ssh/transport2.c` | Host certificate acceptance logic; shows how CA trust store interacts with host key verification (separate from user certs) |
| `libs/putty/ssh/ssh.c:259-260` | `ssh2_userauth_new()` call with `CONF_keyfile` and `CONF_detached_cert` parameters |

### Certificate Format Requirements

- **File format:** RFC4716 (`.pub`) or OpenSSH public key format
- **Algorithm:** Must contain an OpenSSH certificate (algorithm ending in `-cert-v01@openssh.com`)
- **Supported types:** `ssh-rsa-cert-v01@openssh.com`, `ssh-ed25519-cert-v01@openssh.com`, `ecdsa-sha2-nistp*-cert-v01@openssh.com`, `ssh-dss-cert-v01@openssh.com`
- **Detached cert:** Certificate file is separate from private key file

### Certificate Validation (Handled by PuTTY)

PuTTY's `openssh-certs.c` performs comprehensive validation:

| Check | Description |
|-------|-------------|
| **Signature validity** | Verifies certificate signature against CA public key |
| **CA key validity** | Ensures signing key is not itself a certificate |
| **Certificate type** | Validates user vs host certificate type |
| **Time bounds** | Checks `valid_after` and `valid_before` timestamps |
| **Principal matching** | Verifies username matches one of the certificate's principals |
| **Critical options** | Partially interprets force-command, source-address; rejects unknown critical options |
| **Key match** | Certificate's base public key must match the loaded private key |

### Error Messages (from PuTTY)

- Certificate signature verification failure
- CA key is itself a certificate (rejected)
- Certificate type mismatch (user vs host)
- Certificate expired or not yet valid
- Principal mismatch (username not in certificate)
- Unknown critical option (causes rejection)
- Certificate does not match private key

### Private Key Format Limitation (CRITICAL)

**PuTTY only accepts PPK format for private keys** in the authentication flow:

- `ppk_load_f()` at `userauth2-client.c:1282` handles private key loading
- Only `SSH_KEYTYPE_SSH2` (PPK format) is supported
- OpenSSH PEM format (`OPENSSH_PEM`, `OPENSSH_NEW`) is **NOT** auto-converted
- `import_ssh2()` exists in `libs/putty/import.c:144` for format conversion but is **NOT used** in the authentication flow
- `key_type()` at `libs/putty/ssh/sshpubk.c:2014` can detect OPENSSH_PEM, OPENSSH_NEW, SSHCOM formats but detection ≠ conversion

**Resolution:** Must convert OpenSSH private keys → PPK format at runtime using `import_ssh2()` + `save_ssh2_privatekey()` before passing to `CONF_keyfile`.

### NetBox Wiring (Confirmed)

In `src/core/SecureShell.cpp:287-293`:

```cpp
// Set CONF_keyfile from FPublicKeyFile (PPK format)
Filename * AFileName = filename_from_utf8(UTF8String(Data->ResolvePublicKeyFile()).c_str());
conf_set_filename(conf, CONF_keyfile, AFileName);
filename_free(AFileName);

// Set CONF_detached_cert from FDetachedCertificate
AFileName = filename_from_utf8(UTF8String(ExpandEnvironmentVariables(Data->DetachedCertificate)).c_str());
conf_set_filename(conf, CONF_detached_cert, AFileName);
filename_free(AFileName);
```

**Conclusion:** Certificate auth is already wired in NetBox — `FDetachedCertificate` is passed to PuTTY. Only the UI and OpenSSH key conversion are missing.

## Extension Approach for OpenSSH Certificate Auth

1. **Reuse `FDetachedCertificate`** — already wired to PuTTY, just needs UI
2. **Add `FOpensshPrivateKeyFile`** — for OpenSSH-format private key path
3. **Add `FUseOpensshCertificate`** — enable/disable flag
4. **Runtime conversion** — OpenSSH key → PPK via `import_ssh2()` before `CONF_keyfile`
5. **Add UI controls** — simple `TFarEdit` fields on `tabAuthentication` (no browse buttons — matches existing patterns)
6. **Bind to `Execute()` handler** — read/write values after modal result
7. **No CA validation in v1** — PuTTY already validates certs; we just log the results

For full architecture details and dependency rules, see [ARCHITECTURE.md](../ARCHITECTURE.md).
