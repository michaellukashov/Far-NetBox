# Code Review: SSH Private Key Authentication Fix (#392)

**Scope:** Implementation of private key authentication bug fix  
**Branch:** `lmv/dev`  
**Reviewed:** 2026-05-02  
**Reviewer:** aif-review + aif-security-checklist  
**Commit range:** Changes on `lmv/dev` affecting `SecureShell.cpp` and `Terminal.cpp`  
**Commits reviewed:** `e41274cd7`, `1326d7c24`, `b8e6b7174` (plus documentation commits `3ab770400`, `49e86a5a8`, `611cbf5b8`)  
**Files changed:**  
- `src/core/SecureShell.cpp`  
- `src/core/Terminal.cpp`  
- `ChangeLog`  
- `.ai-factory/Github-Issues.md`  
- `.ai-factory/plans/fix-ssh-private-key-cert-392.md`

---

## 1. Overview

This review assesses the fix for GitHub issue #392, where NetBox failed to authenticate with private key files on `publickey`-only SSH servers. The implemented changes:

- Add diagnostic logging for key file resolution and auth configuration.
- Add guard in `TTerminal::DoPromptUser()` to prevent using a remembered SSH password as a key passphrase (and vice versa).
- Fix inverted success check in `TSecureShell::PromptUser()` that incorrectly treated successful prompts as cancelled.
- Restore accidentally removed `FSeat` initialization and `DebugAssert`.
- Update documentation (ChangeLog, issue tracker, plan).

The build passes with zero warnings in the modified files.

---

## 2. Commit-Level Summary

| Commit | Summary | Verdict |
|--------|---------|---------|
| `e41274cd7` | Main fix: adds logging, guard, and check correction | ❌ Initial implementation accidentally removed `FSeat` and `DebugAssert` |
| `1326d7c24` | Restore `FSeat` initialization | ✅ Necessary fix |
| `b8e6b7174` | Remove stray blank line, restore `DebugAssert` | ✅ Style cleanup |
| `3ab770400`, `49e86a5a8`, `611cbf5b8` | Documentation updates | ✅ Properly tracked |

---

## 3. Detailed Findings

### 🟢 **No functional bugs or regressions detected** beyond the initial omission that was corrected in follow-up commits.

However, the following quality and security notes are provided.

---

### 🟡 **LOW: Diagnostic logging may expose local key file paths**

**Location:** `SecureShell.cpp:485-491`  
**Category:** Information Disclosure / Privacy

The fix adds verbose logging of the raw and resolved private key file path, file existence, and SSH auth configuration flags.

```cpp
LogEvent(FORMAT("SSH key file: raw=[%s], resolved=[%s], exists=%d",
  FSessionData->GetPublicKeyFile().c_str(), ResolvedKeyFile.c_str(),
  nb::ToInt32(::FileExists(ApiPath(ResolvedKeyFile)))));
LogEvent(FORMAT("SSH auth config: tryagent=%d, try_ki=%d, try_gssapi=%d, gssapi_kex=%d, ssh_no_userauth=%d, has_passphrase=%d", ...));
```

**Impact:**  
- The logs are written to `%LOCALAPPDATA%\NetBox\netbox.log` (user-only ACLs). An attacker with local access could learn about key file locations and auth preferences.
- The information is not highly sensitive but could aid reconnaissance.

**Recommendation:**  
- Gate this logging behind a debug-level verbosity flag or compile-time macro (e.g., `#ifdef DEBUG` or a runtime log level check).  
- Consider redacting the actual path, showing only a boolean existence check or the filename without the full path.

---

### 🟢 **Code Quality: Good**

- Changes follow the project's coding standards (T/F prefixes, Allman braces, 2-space indentation).
- Comments explain the rationale for the guard in `DoPromptUser()`.
- The guard logic is simple, correct, and safe:
  ```cpp
  const bool RememberedPasswordMatchesPrompt =
    (Kind == pkPassphrase) ?
      (PrimaryTerminal->FRememberedPasswordKind == pkPassphrase) :
      (PrimaryTerminal->FRememberedPasswordKind != pkPassphrase);
  ```
- No new memory leaks, raw pointers, or threading issues.
- Inverted-result fix in `TSecureShell::PromptUser()` correctly handles cancellation.

---

### 🟢 **Security: No new vulnerabilities**

- No hardcoded secrets, no new injection vectors.
- Logging uses static format strings; cannot be exploited for format string attacks.
- Credentials (passphrases/passwords) are not logged.
- The guard prevents cross-credential misuse, which is a correctness improvement that also enhances security by ensuring the right secret is submitted.

---

### 🟢 **Performance: Negligible overhead**

The added operations are a few string comparisons and logging calls during connection setup; not measurable.

---

## 4. Context Gates Compliance

- **Architecture:** ✅ No changes to `libs/`; only Core layer modifications (`SecureShell`, `Terminal`).
- **Coding Rules:** ✅ Naming, braces, CRLF, UTF-8 respected.
- **Roadmap:** ✅ Aligns with "OpenSSH certificate authentication" milestone (credential handling improvements).

---

## 5. Recommendations

1. **Reduce verbosity of diagnostic logs** in production builds. Consider:
   - Compile-time switch (DEBUG)
   - Runtime log level (verbose/debug)
   - Omit full paths or hash them.
2. **Add integration tests** for SSH authentication flows if test infrastructure expands.
3. **Spot-check `FRememberedPasswordKind` usage** to ensure it is always set when a remembered password is stored (outside this change).

---

## 6. Conclusion

The fix for issue #392 is **correct, clean, and ready for merge**.

- The two logical fixes (credential-kind guard and inverted check) resolve the observed failures.
- The temporary removal of `FSeat` was caught and corrected.
- Security exposure is low (local log only).
- Quality and style are excellent.

**Overall Risk:** 🟢 **Low**  
**Verdict:** ✅ **APPROVE**

No blocking issues. The minor logging verbosity note is for future refinement.
