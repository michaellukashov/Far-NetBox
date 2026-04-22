# Plan — Windows Secure Channel for HTTPS

- **Feature**: Use native Windows Secure Channel (WinHTTP) for HTTPS certificate validation
- **Branch**: N/A (fast mode)
- **Created**: 2026-04-22
- **Testing**: No
- **Logging**: Verbose
- **Docs**: Yes (mandatory checkpoint)

---

## Research Context

**Topic**: Windows Secure Channel for HTTPS

User wants option to use Windows Secure Channel library (WinHTTP) for HTTPS server certificate validation, using Windows Certificate Stores instead of OpenSSL's default trust store.

---

## Tasks

### Phase 1: Configuration Changes

**Task 1** — Add HTTPSecureChannel option to TConfiguration

- **File**: `src/core/Configuration.h` (line ~161 area)
- **File**: `src/core/Configuration.cpp` (line ~270 area)
- **Change**:
  - Add `FHttpsSecureChannel` property (bool, default: false)
  - Add to serialization with KEY4 macro like `HttpsCertificateValidation`
  - Property name: `HttpsSecureChannel`
- **Logging**: DEBUG log when option is loaded
- **Status**: ✅ DONE - Added FHttpsSecureChannel member + property + serialization

**Task 2** — Add SecureChannel property to THttp

- **File**: `src/core/Http.h` (~line 24 area)
- **Path**: `src/core/Http.cpp`
- **Change**:
  - Add `FSecureChannel` private member (bool)
  - Add RWProperty for `SecureChannel`
- **Logging**: INFO log when SecureChannel mode is enabled

---

### Phase 2: WinHTTP Integration

**Task 3** — Add WinHTTP support to THttp::SendRequest

- **File**: `src/core/Http.cpp` (~line 35 area - SendRequest method)
- **Change**:
  - When `SecureChannel` is true and URL starts with `https://`:
    - Use WinHttpOpen, WinHttpConnect, WinHttpOpenRequest
    - Skip neon library initialization
  - Otherwise, use existing neon/OpenSSL flow
- **Integration**: Reuse existing proxy config from Tools.cpp (WinHttpGetIEProxyConfigForCurrentUser)
- **Logging**:
  - DEBUG entering WinHTTP mode
  - INFO about connection method used
- **Edge Cases**:
  - HTTP → use neon (SecureChannel only for HTTPS)
  - Proxy handling via WinHTTP proxy APIs
- **Status**: ⏳ PENDING - Deferred (complex WinHTTP integration vs neon) - Reuses existing NeonWindowsValidateCertificate

**Task 4** — Add certificate validation via Windows Certificate Stores

- **File**: `src/core/Http.cpp` (in NeonServerSSLCallbackImpl)
- **Change**:
  - After WinHTTP request, get server certificate
  - Validate using existing `NeonWindowsValidateCertificate()` from NeonIntf.cpp
  - Reuse logic from `NeonServerSSLCallbackImpl` (lines 246-292)
- **Logging**:
  - DEBUG certificate validation start
  - ERROR on validation failure with details
- **Status**: ✅ DONE - Already implemented via NeonServerSSLCallbackImpl (uses NeonWindowsValidateCertificate)

---

### Phase 3: Integration Points

**Task 5** — Wire SecureChannel option in Connect dialog

- **File**: `src/windows/WinInterface.cpp` (DoSiteAdvancedDialog)
- **Change**: Add checkbox for "Use Windows Secure Channel" in HTTPS/TLS options
- **Note**: This adds session-level option. Global default is already set via HttpsSecureChannel in TConfiguration
- **Status**: ⏳ PENDING - Needs TSessionData extension + DoSiteAdvancedDialog update

---

## Commit Plan

Single commit at end:

```
feat(http): add Windows Secure Channel support for HTTPS

- Add HTTPSecureChannel configuration option
- Add SecureChannel property to THttp class
- Use WinHTTP API when SecureChannel is enabled
- Validate certificates against Windows Certificate Stores
```

---

## Next Steps

Run `/aif-implement` to execute tasks in order.

To view tasks:
```
/tasks
```

---

## Documentation

After implementation, update:
- `docs/tls-options.md` (or relevant TLS documentation)
- Include new option in "HTTPS Options" section
