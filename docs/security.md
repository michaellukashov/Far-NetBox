# Security

[<- Architecture](architecture.md) · [Back to README](../README.md)

NetBox stores session credentials and sensitive configuration data. The master password feature provides optional encryption for stored passwords using AES-256.

## Master Password

### Overview

When enabled, the master password encrypts all stored session passwords with AES-256-CBC plus HMAC-SHA256 for integrity. The master password itself is never stored on disk; only a verifier hash is kept to validate the password on entry.

### Data Flow

```
User enters master password
         |
         v
    [Verifier check] ------> AES256Verify()
         |
    Valid password
         |
         v
    [Password storage]
    Session password ------> AES256EncryptWithMAC(master_password, plaintext)
         |
         v
    Encrypted blob stored in registry/INI
```

### Components

| Component | File | Purpose |
|-----------|------|---------|
| `TSecureString` | `src/base/SecureString.cpp` | Secure memory buffer with `VirtualLock` + `SecureZeroMemory` |
| `TWinConfiguration` | `src/windows/WinConfiguration.cpp` | Master password state, verifier, session counter |
| `AES256EncryptWithMAC` | `src/core/Cryptography.cpp` | AES-256-CBC encryption with HMAC-SHA256 |
| `AES256Verify` | `src/core/Cryptography.cpp` | Password verifier validation |

### Secure Memory (`TSecureString`)

Password plaintext is held in `TSecureString` rather than `UnicodeString`:

- **`VirtualLock`** — Attempts to pin the memory page to RAM (prevents swap-to-disk). Falls back gracefully if the privilege is unavailable.
- **`SecureZeroMemory`** — Overwrites buffer before deallocation.
- **Move-only** — No copy constructor; prevents accidental duplication.
- **RAII** — Destructor automatically wipes the buffer.

### Thread Safety

Master password sessions use `std::atomic` counters:

- `FMasterPasswordSession` — Tracks nested password-access sessions via `fetch_add`/`fetch_sub`
- `FMasterPasswordSessionAsked` — Atomic flag prevents redundant prompts during parallel transfers

Both counters use `std::memory_order_relaxed` on x86/x64 where lock-free atomics are guaranteed.

### Rate Limiting

To prevent brute-force attacks on the master password verifier:

- **5 consecutive failures** trigger a **30-second lockout**
- Successful validation resets the failure counter
- `GetTickCount()` is used for Windows XP compatibility
- Counter and timestamp are stored in `std::atomic<uint32_t>` fields

### Recryption on Change

When the master password is changed or cleared, all stored session passwords are re-encrypted:

1. `ChangeMasterPassword()` / `ClearMasterPassword()` calls `RecryptPasswords()`
2. `RecryptPasswords()` iterates all stored sessions via `TTerminalManager::RecryptPasswords()`
3. Errors are collected in a `TStringList` and displayed to the user
4. Exception-safe `try__finally` blocks ensure the decrypt key is always updated/shredded even if recryption fails

### Password Strength

`IsValidPassword()` enforces minimum strength:

- At least 6 characters
- Mixed case + digits + special characters recommended
- Weak passwords trigger a confirmation dialog

## Credential Storage Without Master Password

When no master password is set, session passwords are stored with a simple scramble (XOR-based obfuscation) rather than encryption. This provides basic protection against casual inspection but is **not cryptographically secure**.

## Security Best Practices

1. **Enable master password** on shared or unattended machines
2. **Use a strong password** — at least 12 characters with mixed case, digits, and symbols
3. **Do not store passwords** for highly sensitive environments; use key-based authentication instead
4. **Review logs** — Passwords are masked in `LogEvent()` output when master password is active

## See Also

- [Architecture](architecture.md) — Layered plugin architecture
- [Contributing](contributing.md) — Code conventions
