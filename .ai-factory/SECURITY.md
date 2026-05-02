# Security Policy — Far-NetBox

> **Scope:** Guidelines for handling sensitive data in logs and production builds.  
> **Audience:** Developers, reviewers, and maintainers.  
> **Last updated:** 2026-05-02

---

## 1. Logging Sensitive Information

NetBox is a Far Manager plugin that handles credentials (passwords, private key passphrases), host keys, and session configuration. All logging must respect the following:

### 1.1 Never Log Secrets

The following **MUST NOT** appear in any log output, regardless of log level:

- Passwords (plaintext or encrypted)
- Private key passphrases
- Private key file contents
- Session passwords stored in memory
- Any user-provided credential unless explicitly marked `LogSensitive` in session settings and then only with explicit user consent

### 1.2 Path Information

File paths to private keys, known_hosts, or configuration files are considered **semi-sensitive**. They can aid an attacker with local access.

**Requirement:** When logging paths in debug/diagnostic contexts (e.g., SSH auth troubleshooting), either:

- Gate the log behind a compile-time flag (e.g., `#ifdef DEBUG`), **or**
- Use a runtime verbosity level that is disabled by default in production builds (e.g., `LogLevel::Debug` vs `LogLevel::Info`), **or**
- Redact the path to show only the filename (e.g., `C:\Users\...\.ssh\id_ed25519.ppk` → `id_ed25519.ppk`)

### 1.3 Format String Safety

All logging MUST use static format strings. Never pass user-controlled strings as the format:

```cpp
// GOOD
LogEvent(FORMAT("SSH key file: raw=[%s], resolved=[%s]", raw.c_str(), resolved.c_str()));

// BAD
LogEvent(user_supplied_message);  // Could contain %n, %x, etc.
```

### 1.4 PuTTY Event Log

PuTTY's internal event log (routed through `ScpLogPolicyVTable`) may contain detailed connection info. This is acceptable for diagnostic builds but should be suppressed in production unless the user explicitly enables verbose logging.

---

## 2. Credential Handling

### 2.1 Credential Kind隔离

Password and passphrase prompts must be distinguished. The `FRememberedPasswordKind` field tracks what kind of credential was last stored. Do **not** use a remembered password as a passphrase or vice versa. See `Terminal.cpp:DoPromptUser()` for the guard pattern.

### 2.2 Stored Credentials

Session passwords/passphrases stored in the session XML are encrypted using a key derived from the private key file path. This design is legacy and accepted; however, changing the key file path invalidates stored passphrases.

---

## 3. Build Configuration

- Production releases SHOULD set `OPT_USE_UNITY_BUILD=ON` for security hardening (address space layout randomness).
- Debug builds can include verbose logging; release builds must default to info-level or higher.
- Consider defining a `SECURE_BUILD` flag that disables particularly verbose diagnostic logs.

---

## 4. Review Checklist

When reviewing code that handles credentials or logs:

- [ ] No passwords/passphrases in log strings
- [ ] Path logging is either gated or redacted
- [ ] Format strings are static
- [ ] No new hardcoded secrets
- [ ] Credential kind guard present when using remembered passwords
- [ ] No logging inside catch blocks that could throw (defensive)

---

## 5. Reporting Security Issues

Do not file public GitHub issues for security vulnerabilities. Contact the maintainer directly:

- Email: michael.lukashov@gmail.com
- Include detailed reproduction steps, affected versions, and recommended mitigations.

---

## Appendix: Relevant Files

| Area | Files |
|------|-------|
| SSH auth | `src/core/SecureShell.cpp`, `src/core/Terminal.cpp` |
| Logging | `src/nbcore/logging.cpp`, `libs/tinylog/` |
| Credential storage | `src/base/Sysutils.cpp` (`EncryptPassword`, `DecryptPassword`) |
| PuTTY integration | `libs/putty/` |
| Configuration | `src/core/SessionData.cpp`, `src/core/Storage.cpp` |

---

**Note:** This policy is enforced via code review and CI checks (where applicable). Violations should be fixed before merging.
