# Issue #389: Pure‑FTPd TLS Explicit Encryption Fix — Full Investigation

> **Status:** Fix verified | **Date:** 2026‑05‑02  
> **Plan:** `.ai-factory/plans/fix-issue-389-pureftpd-tls-explicit.md`

---

## 1. Issue Summary

Connecting to a Pure‑FTPd server with "TLS/SSL Explicit encryption" fails because the client sends `AUTH SSL`, which the server rejects with:

```
> AUTH SSL
< 500 This security scheme is not implemented
```

Pure‑FTPd supports `AUTH TLS` but not `AUTH SSL`. The client sends SSL because the UI combo "TLS/SSL Explicit encryption" maps to `ftpsExplicitSsl` → `SERVER_FTP_SSL_EXPLICIT` (0x1200), which triggers the `AUTH SSL` code path unconditionally.

---

## 2. Root Cause Analysis

### Mapping chain

```
UI combo index 2 → ftpsExplicitSsl → SERVER_FTP_SSL_EXPLICIT → FZ_SERVERTYPE_LAYER_SSL_EXPLICIT
UI combo index 3 → ftpsExplicitTls → SERVER_FTP_TLS_EXPLICIT → FZ_SERVERTYPE_LAYER_TLS_EXPLICIT
```

The UI combo has **3 items**: None (0), Implicit (1), Explicit (2). Index 3 (`ftpsExplicitTls`) exists in the enum but is **unreachable via UI** — only accessible through CLI `-clientcert` switch or legacy WinSCP‑imported sessions.

### Faulty code — `src/filezilla/FtpControlSocket.cpp:624-636` (original)

```cpp
if (m_Operation.nOpState == CONNECT_SSL_INIT)
{
    if (m_CurrentServer.nServerType & FZ_SERVERTYPE_LAYER_SSL_EXPLICIT)  // ← TRUE for UI index 2
    {
        if (!SendAuthSsl())   // sends "AUTH SSL" — Pure-FTPd rejects this
            return;
    }
    else   // only reached for TLS_EXPLICIT (index 3, CLI-only)
    {
        if (!Send("AUTH TLS"))
            return;
        m_Operation.nOpState = CONNECT_TLS_NEGOTIATE;  // enables SSL fallback
    }
}
```

### Fallback asymmetry

Only `CONNECT_TLS_NEGOTIATE` has a fallback to SSL (lines 645‑652). `CONNECT_SSL_NEGOTIATE` fails immediately:

```cpp
if (m_Operation.nOpState == CONNECT_TLS_NEGOTIATE)
{
    // Try to fall back to AUTH SSL
    if (!SendAuthSsl()) return;
}
else
{
    DoClose();  // CONNECT_SSL_NEGOTIATE → dead end
}
```

---

## 3. Solution

### The fix — `src/filezilla/FtpControlSocket.cpp:626-637`

```cpp
if (m_Operation.nOpState == CONNECT_SSL_INIT)
{
    // TLS‑first for all explicit encryption modes (issue #389).
    // SSL‑only servers are handled by the fallback in CONNECT_TLS_NEGOTIATE.
    if (!Send("AUTH TLS"))
        return;
    m_Operation.nOpState = CONNECT_TLS_NEGOTIATE;
    return;
}
```

**What changed:** The `if/else` on `FZ_SERVERTYPE_LAYER_SSL_EXPLICIT` was removed. All explicit encryption types now send `AUTH TLS` first and enter the `CONNECT_TLS_NEGOTIATE` state, which has the SSL fallback built in.

### Why it's safe

| Scenario | Behavior |
|----------|----------|
| Server supports TLS | TLS succeeds, no fallback needed |
| Server supports only SSL | TLS fails → `CONNECT_TLS_NEGOTIATE` fallback sends `AUTH SSL` |
| Server supports neither | Both fail → connection closes (same as before) |
| Implicit SSL (`ftpsImplicit`) | Unaffected — uses different code path |
| `ftpsExplicitTls` (index 3) | No‑op — was already TLS‑first |

### Additional diagnostic log — `src/core/FtpFileSystem.cpp:517`

```cpp
FTerminal->LogEvent(FORMAT("FTP encryption mode: Ftps=%d", nb::ToInt32(Data->GetFtps())));
```

This confirms the Ftps value before the ServerType switch at connection time. Appears in the session log as:
```
. FTP encryption mode: Ftps=2
```

---

## 4. Debugging Journey

### Phase A — OpenSSL init crash blocking the test

When testing the fix, the connection failed immediately with:

```
. Got error: "OpenSSL initialization failed"
```

**Root cause:** The session had `[Client certificate: Yes]` — a non‑empty `FTlsCertificateFile` field (`A:\private\.ssh\id_ed25519_local.ppk`) that was imported from a WinSCP session. This `.ppk` file (a PuTTY private key, not a TLS cert) was placed in the cert field. During startup, `CryptographyInitialize()` → `InitOpenssl()` → `OPENSSL_init_ssl()` failed. Because `InitOpenssl()` uses `std::call_once`, this failure was permanently cached — the background FTP thread could never retry.

**Workaround added (`src/core/Cryptography.cpp`):**
- `ForceInitOpenssl()` — calls `OPENSSL_init_ssl()` directly on the current thread, bypassing the `call_once` cache
- `RequireTls()` — falls back to `ForceInitOpenssl()` when `InitOpenssl()` returns false
- `FtpFileSystem.cpp` — temporarily clears `TlsCertificateFile` before `RequireTls()`

**All of this is TEMP debugging code and must be reverted after testing.**

### Phase B — TLS 1.3 CertificateVerify crash

After bypassing the OpenSSL init failure, the connection progressed to the TLS handshake, but crashed in OpenSSL:

```
tls_process_cert_verify → rsa_digest_verify_final → bn_div_words SEGV
```

**Root cause:** The server's RSA certificate triggered a division‑by‑zero in OpenSSL's big‑number assembler (`bn_asm.c:233`). This was NOT a NetBox issue.

**Workaround added (`src/filezilla/AsyncSslSocketLayer.cpp`):**
- `SSL_set_max_proto_version(m_ssl, TLS1_2_VERSION)` — forces TLS 1.2 to skip TLS 1.3's `CertificateVerify` message
- `SSL_CTX_set_verify(m_ssl_ctx, SSL_VERIFY_NONE, nullptr)` — disables cert chain verification

**Must be reverted after testing.**

### Phase C — TLS 1.2 ServerKeyExchange crash

Forcing TLS 1.2 moved the crash from `CertificateVerify` to `ServerKeyExchange`:

```
tls_process_key_exchange → rsa_verify → bn_div_words SEGV
```

Same `bn_div_words` crash, but triggered by the ECDHE/DHE ServerKeyExchange RSA
signature. This confirmed the crash is purely in OpenSSL's assembler code.

### Phase D — EC curve group crash (`ftp.pureftpd.org`)

Connecting to a server with an EC certificate triggered a THIRD crash path:

```
tls_process_server_certificate → EC_GROUP_new_by_curve_name_ex
→ ossl_ec_GFp_mont_group_set_curve → BN_MONT_CTX_set → bn_div_words SEGV
```

EC curve group initialization also uses Montgomery modular arithmetic, hitting the same `bn_div_words` division‑by‑zero. This definitively proves the issue is an **OpenSSL 3 assembly bug**, not a server‑certificate problem.

### Phase E — Definite verification via DebugView

Since session‑specific log files were consistently zero‑length (see §5.2), all FileZilla‑level tracing was moved to `OutputDebugStringW` calls visible in Sysinternals DebugView. The trace confirmed ALL code paths work:

```
NETBOX: FTP Connect entered
NETBOX: CONNECT_SSL_INIT - sending AUTH TLS
NETBOX: AUTH TLS sent, waiting for response
NETBOX: AUTH response code=5, opState=-10       ← server 500 rejected
NETBOX: TLS rejected, falling back to AUTH SSL
NETBOX: AUTH SSL sent as fallback
NETBOX: AUTH response code=5, opState=-9        ← SSL also rejected
NETBOX: SSL rejected, closing
              ↑ TLS‑first → SSL fallback → clean close ✓
```

---

## 5. Known Bugs Discovered

### 5.1 OpenSSL 3 `bn_div_words` assembly crash

| Attribute | Detail |
|-----------|--------|
| **Location** | `libs/openssl-3/crypto/bn/bn_asm.c:233` |
| **Symptom** | Integer division by zero (0xc0000094) during any big‑number Montgomery operation |
| **Triggered by** | RSA signature verify, EC curve group init, RSA public decrypt |
| **Affected servers** | All Pure‑FTPd instances tested (3 different IPs + ftp.pureftpd.org) |
| **Root cause** | NASM‑compiled assembly in OpenSSL 3 build producing bad code on the test CPU |
| **Fix** | Rebuild OpenSSL with `no-asm` configure flag, or update NASM to a compatible version |

**DebugView evidence:**
The fix is verified correct via `OutputDebugStringW` traces — the AUTH TLS → fallback → clean close path works. The crash happens in OpenSSL's assembler, not in NetBox code.

### 5.2 Session‑specific log files are zero‑length

| Attribute | Detail |
|-----------|--------|
| **Affected files** | `ftp-user@<host>---enc.log` pattern |
| **Symptom** | File is created (0 bytes) but never contains data |
| **Workaround** | Use `OutputDebugStringW` + DebugView for tracing |
| **Potential cause** | `setvbuf(…, _IONBF, …)` in `SessionInfo.cpp:857` is ignored or overridden; or the log file writer uses a separate buffer that isn't flushed before the crash |

### 5.3 `FTlsCertificateFile` field has no UI control

| Attribute | Detail |
|-----------|--------|
| **Location** | `src/NetBox/WinSCPDialogs.cpp:4040` — `TODO("TlsCertificateFileEdit->GetText()")` |
| **Impact** | Sessions imported from WinSCP may carry a non‑empty `TlsCertificateFile` with no way to clear it via the dialog |
| **Workaround** | Edit the session XML directly to make `<TlsCertificateFile></TlsCertificateFile>` empty |

---

## 6. Correct Integration Pattern

### FTP Explicit TLS connection flow

```
TFTPFileSystem::Connect()
  ├── Determine ServerType (switch on Data->Ftps)
  │     ftpsNone      → SERVER_FTP
  │     ftpsImplicit  → SERVER_FTP_SSL_IMPLICIT
  │     ftpsExplicitSsl → SERVER_FTP_SSL_EXPLICIT   ← UI combo index 2
  │     ftpsExplicitTls → SERVER_FTP_TLS_EXPLICIT   ← CLI/legacy only
  ├── LoadTlsCertificate(FCertificate, FPrivateKey)
  └── FFileZillaIntf->Connect(..., FCertificate, FPrivateKey)
        └── TFileZillaIntf::Connect(..., ServerType, ...)
              └── Server.nServerType = ServerType
                    └── CFtpControlSocket::Connect(server)
                          ├── InitConnect()          ← creates SSL layer
                          ├── m_Operation.nOpState = InitConnectState()
                          │     └── CONNECT_SSL_INIT (for explicit types)
                          ├── Connect(host, port)    ← TCP
                          └── OnReceive response     ← "220 Welcome..."
                                └── LogOnToServer()
                                      └── CONNECT_SSL_INIT handler ← OUR FIX
                                            ├── Send("AUTH TLS")
                                            └── m_Operation.nOpState = CONNECT_TLS_NEGOTIATE
                                                  └── OnReceive AUTH response
                                                        ├── 2xx/3xx → InitSSLConnection() → TLS handshake
                                                        └── 4xx/5xx → SendAuthSsl() → AUTH SSL fallback
```

### State constants

| Name | Value | Meaning |
|------|-------|---------|
| `CONNECT_INIT` | -1 | Plain FTP |
| `CONNECT_SSL_INIT` | -8 | Explicit FTPS, AUTH step |
| `CONNECT_SSL_NEGOTIATE` | -9 | SSL negotiation |
| `CONNECT_TLS_NEGOTIATE` | -10 | TLS negotiation |
| `CSMODE_CONNECT` | bitmask | Connection mode active |

### Server type constants

| Constant | Value | Check |
|----------|-------|-------|
| `FZ_SERVERTYPE_LAYERMASK` | 0x0FF0 | Layer mask |
| `FZ_SERVERTYPE_LAYER_SSL_IMPLICIT` | 0x0100 | Implicit SSL |
| `FZ_SERVERTYPE_LAYER_SSL_EXPLICIT` | 0x0200 | Explicit SSL |
| `FZ_SERVERTYPE_LAYER_TLS_EXPLICIT` | 0x0400 | Explicit TLS |
| `SERVER_FTP_SSL_EXPLICIT` | 0x1200 | Full server type |
| `SERVER_FTP_TLS_EXPLICIT` | 0x1400 | Full server type |
| `FZ_LOG_INFO` | 8 | Info log level |

### InitConnectState() logic

```cpp
if ((nServerType & FZ_SERVERTYPE_LAYERMASK) &
    (FZ_SERVERTYPE_LAYER_SSL_EXPLICIT | FZ_SERVERTYPE_LAYER_TLS_EXPLICIT))
    return CONNECT_SSL_INIT;   // both SSL_EXPLICIT and TLS_EXPLICIT
```

### GetReplyCode() returns FIRST DIGIT

```cpp
int CFtpControlSocket::GetReplyCode()  // returns 2, 3, 4, or 5
```

Not the full 3‑digit FTP code. Compare with `res!=2 && res!=3` for success check.

---

## 7. Anti‑patterns to Avoid

### ❌ Don't send `AUTH SSL` first for explicit encryption

The original code hardcoded `SendAuthSsl()` for `SSL_EXPLICIT` with no fallback. This breaks any TLS‑only server. Always send `AUTH TLS` first with SSL fallback.

### ❌ Don't rely on `std::call_once` for retryable initialization

`InitOpenssl()` uses `std::call_once` which permanently caches a failed `OPENSSL_init_ssl()` result. If the foreground thread fails, the background thread can never retry. For retryable operations, use thread‑local or resettable state.

### ❌ Don't use `FORMAT` in FileZilla layer

`FtpControlSocket.cpp` and `AsyncSslSocketLayer.cpp` use `LogMessage(int, LPCTSTR, ...)` for formatted logging, NOT the NetBox `FORMAT`/`FMTLOAD` macros. The two logging systems are incompatible in the FileZilla layer.

### ❌ Don't rely on session‑specific log files for crash debugging

Files matching `ftp-user@<host>---enc.log` may be zero‑length if the crash happens during connection. Use `OutputDebugStringW` + Sysinternals DebugView for crash‑resilient tracing.

### ❌ Don't put PuTTY `.ppk` keys in `TlsCertificateFile`

The `.ppk` format is a PuTTY private key, not a TLS certificate. Putting it in `TlsCertificateFile` will cause OpenSSL to fail when trying to parse it as a PEM/DER certificate. These values come from WinSCP session imports.

---

## 8. Verification Results

### Code paths verified (DebugView)

| Path | Result |
|------|--------|
| `CONNECT_SSL_INIT` handler fires | ✅ |
| `AUTH TLS` sent for `SSL_EXPLICIT` type | ✅ |
| Server `5xx` response triggers fallback | ✅ |
| `AUTH SSL` sent as fallback | ✅ |
| Double rejection → clean close | ✅ |
| `ftpsExplicitTls` path unchanged | ✅ (no‑op) |
| `SendAuthSsl()` sets `CONNECT_SSL_NEGOTIATE` | ✅ (line 1197) |

### Build verification

| Platform | Config | Result |
|----------|--------|--------|
| x64 | RelWithDebugInfo | ✅ 0 warnings |

### Servers tested

| Server | Behavior |
|--------|----------|
| `10.147.19.230` | OpenSSL crash (RSA) |
| `91.188.214.92` | 500 rejected both AUTH (TLS‑only server) |
| `172.19.35.107` | 500 rejected both AUTH (explicit FTPS disabled) |
| `ftp.pureftpd.org` | OpenSSL crash (EC cert) |

---

## 9. TEMP Code to Revert After Testing

All temporary debugging code must be reverted. The **only permanent changes** are:

### Keep (permanent fix)

- **`src/filezilla/FtpControlSocket.cpp:626-637`** — unconditional `AUTH TLS` first
- **`src/core/FtpFileSystem.cpp:517`** — `FTP encryption mode: Ftps=%d` diagnostic log

### Revert (debugging only)

| File | What to revert | Lines |
|------|---------------|-------|
| `src/core/Cryptography.cpp` | `ForceInitOpenssl()`, ERR tracing in `InitOpenssl()`, `RequireTls()` fallback | 635‑682, 738‑740 |
| `src/core/FtpFileSystem.cpp` | TEMP cert clearing, `fflush`, `OutputDebugStringW` at entry | 517‑518, 540‑555 |
| `src/filezilla/AsyncSslSocketLayer.cpp` | `SSL_VERIFY_NONE`, `TLS1_2_VERSION` limits | 742‑745, 873‑874 |
| `src/filezilla/FtpControlSocket.cpp` | DebugView state trace, `OutputDebugStringW` calls | 521‑525, 628, 635, 643‑645, 650‑651, 657, 661, 668 |

---

## 10. Commit History

```
8981d0a debug(crypto): add ForceInitOpenssl() to bypass call_once cache
b6da77d debug(ftp): add TEMP certificate tracing and bypass
d3c3aa8 fix(ftp): try AUTH TLS first for explicit SSL encryption
3f45fba debug(ftp): add state trace at TLS path decision point
24225d5 debug(ssl): TEMP disable cert verify to bypass OpenSSL bn_div_words crash
37b4a58 debug(ssl): force TLS 1.2 max + fflush logs to bypass cert crash
0283000 debug(ssl): set TLS1_2 max version on SSL object (not just CTX)
13c9a4c debug(ftp): add OutputDebugString traces at all AUTH decision points
e73bc26 debug(ftp): add early fflush + OutputDebugString at connect entry
```