# Windows Certificate Store Authentication for NetBox

**Branch:** feature/windows-cert-auth  
**Created:** 2026-04-23  
**Type:** Feature Enhancement  
**Priority:** High (User-requested, security-critical)

## Problem Statement

Users need to authenticate to SSH/SFTP servers using certificates stored in the Windows Certificate Store rather than file-based certificates. This is particularly important for:
- Organizations with automatic certificate rotation
- Environments using Windows certificate management infrastructure
- Cases where certificates are generated and managed by system administrators

**Reference Issue:** https://github.com/michaellukashov/Far-NetBox/issues/509

## Current State

### Existing Implementation
1. **DetachedCertificate Support** (`SessionData.cpp`):
   - Property: `DetachedCertificate` (UnicodeString)
   - Stores path to certificate file (e.g., `C:/Path/to/user-key-cert.pub`)
   - Used in SSH authentication via `SecureShell.cpp`
   - Limitation: Only supports file-based certificates

2. **Certificate Loading** (`PuttyIntf.cpp`):
   - `AddCertificateToKey()` function handles certificate attachment
   - Requires file path input
   - No support for Windows Certificate Store APIs

3. **Session Configuration**:
   - Export/Import via `.netbox` files
   - XML parameter: `<DetachedCertificate>`
   - No UI for certificate store selection

### Missing Capabilities
- No Windows Certificate Store integration
- No UI for selecting certificates from store
- No support for certificate thumbprint-based lookup
- No automatic certificate renewal handling

## Target State

### Features to Implement
1. **Certificate Store Access**:
   - Open Windows Certificate Store (MY/Personal store)
   - Enumerate available certificates
   - Filter by key usage (Client Authentication)
   - Support for both user and machine stores

2. **Certificate Selection**:
   - UI dialog to select certificate from store
   - Display certificate details (subject, issuer, validity)
   - Support thumbprint-based configuration
   - Cache selected certificate reference

3. **Authentication Integration**:
   - Extract private key from certificate store
   - Support for smart card certificates
   - Handle PIN prompts for protected keys
   - Integrate with existing SSH authentication flow

4. **Configuration Storage**:
   - Store certificate thumbprint in session config
   - Distinguish between file-based and store-based certificates
   - Backward compatible with existing detached certificate config

## Technical Approach

### Windows Certificate Store APIs
```cpp
// Key APIs to use:
HCERTSTORE hStore = CertOpenStore(
    CERT_STORE_PROV_SYSTEM,
    0,
    NULL,
    CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
    L"MY"
);

PCCERT_CONTEXT pCert = CertFindCertificateInStore(
    hStore,
    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
    0,
    CERT_FIND_SHA1_HASH,
    thumbprint,
    NULL
);

// Extract private key handle
HCRYPTKEY hKey;
CryptAcquireCertificatePrivateKey(
    pCert,
    0,
    NULL,
    &hKey,
    NULL,
    NULL
);
```

### Session Data Changes
```cpp
// New enum for certificate source
enum TCertSourceType {
    cstNone = 0,
    cstFile = 1,
    cstWindowsStore = 2
};

// New properties in TSessionData
TCertSourceType FCertSourceType;
UnicodeString FCertThumbprint;  // SHA1 hash for store lookup
UnicodeString FCertStoreLocation; // Optional: custom store path
```

### UI Components
1. **Session Options Dialog**:
   - Radio button: "Use file certificate" vs "Use Windows Certificate Store"
   - File picker (existing) vs Certificate picker (new)
   - Certificate details viewer

2. **Certificate Picker Dialog**:
   - List available certificates from Personal store
   - Show: Subject, Issuer, Valid From/To, Key Usage
   - Filter: Only show certificates with private keys
   - Search/filter by subject name

3. **Authentication Dialog**:
   - PIN prompt for smart card certificates
   - Cache PIN for session duration (not persistent)

## Implementation Phases

### Phase 1: Certificate Store Integration (Core)
- [ ] Add Windows Certificate Store wrapper class
- [ ] Implement certificate enumeration
- [ ] Add thumbprint-based lookup
- [ ] Handle certificate store errors gracefully

### Phase 2: Session Data & Configuration
- [ ] Extend TSessionData with certificate source type
- [ ] Add thumbprint storage and retrieval
- [ ] Update session export/import to handle new fields
- [ ] Maintain backward compatibility

### Phase 3: UI Implementation
- [ ] Create certificate picker dialog
- [ ] Modify session options dialog
- [ ] Add certificate details viewer
- [ ] Implement PIN prompt dialog

### Phase 4: Authentication Integration
- [ ] Modify SSH authentication to use store certificates
- [ ] Handle private key extraction
- [ ] Support smart card PIN prompts
- [ ] Test with OpenSSH server certificate auth

### Phase 5: Testing & Validation
- [ ] Unit tests for certificate store access
- [ ] Integration tests with test SSH server
- [ ] Test certificate rotation scenarios
- [ ] Verify backward compatibility

## Configuration Format

### Session File Example
```xml
<Session>
  <HostName>my-server.example.com</HostName>
  <UserName>user</UserName>
  
  <!-- New certificate configuration -->
  <CertSourceType>2</CertSourceType> <!-- Windows Store -->
  <CertThumbprint>SHA1_HASH_HERE</CertThumbprint>
  
  <!-- Legacy support (file-based) -->
  <!-- <DetachedCertificate>C:/path/to/cert.pub</DetachedCertificate> -->
</Session>
```

### Command Line (Future)
```
netbox.exe /cert-store:current-user /cert-thumbprint:ABC123...
```

## Security Considerations

1. **Private Key Protection**:
   - Never export private keys from store
   - Use key handles only within authentication context
   - Clear cached handles on session close

2. **Certificate Validation**:
   - Verify certificate chain before use
   - Check certificate validity period
   - Validate key usage flags

3. **PIN Handling**:
   - Never store PIN persistently
   - Clear from memory after use
   - Support for Windows Hello/Smart Card PIN caching

4. **Store Access**:
   - Use readonly access for certificate enumeration
   - Minimal required permissions
   - Handle access denied gracefully

## Dependencies

### External
- Windows SDK (WinCrypt.h, Wincrypt.lib)
- OpenSSL for key operations
- Existing PuTTY SSH integration

### Internal
- SessionData module
- Authentication module (SecureShell)
- UI framework (Far Manager dialogs)

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Certificate store access failure | High | Fallback to file-based, clear error messages |
| Smart card PIN timeout | Medium | Timeout handling, retry logic |
| Certificate rotation during session | Low | Detect and prompt for re-authentication |
| Performance impact on startup | Low | Lazy loading, cache certificate references |
| Backward compatibility break | High | Maintain legacy config support, migration path |

## Testing Strategy

1. **Unit Tests**:
   - Certificate store enumeration
   - Thumbprint lookup accuracy
   - Certificate validity checking

2. **Integration Tests**:
   - SSH authentication with store certificate
   - Certificate with PIN protection
   - Multiple certificate scenarios

3. **Manual Testing**:
   - Real smart card certificates
   - Corporate AD certificate infrastructure
   - Certificate rotation scenarios

## Success Criteria

- [ ] User can select certificate from Windows Certificate Store
- [ ] Authentication succeeds with store-based certificate
- [ ] Existing file-based certificates continue to work
- [ ] Session configuration persists certificate choice
- [ ] No degradation in authentication performance
- [ ] Clear error messages for common failure modes

## References

- Issue: https://github.com/michaellukashov/Far-NetBox/issues/509
- Windows Certificate Store: https://learn.microsoft.com/en-us/windows/win32/seccrypto/cryptography-portal
- OpenSSL Certificate Handling: Existing codebase patterns
- PuTTY Certificate Auth: `putty.h`, `ssh.h` patterns
