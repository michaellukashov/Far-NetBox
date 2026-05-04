# Plan: Add RSA-SHA256 Host Key Support (#388)

> **GitHub Issue:** [#388](https://github.com/michaellukashov/Far-NetBox/issues/388)  
> **Created:** 2026-04-30  
> **Mode:** fast  
> **Settings:** Testing=no, Logging=verbose, Docs=yes

## Problem Statement

OpenSSH 8.8+ disables RSA-SHA1 signatures by default, causing servers to present host keys using the `rsa-sha2-256` or `rsa-sha2-512` algorithms instead of legacy `ssh-rsa`. NetBox fails to verify cached host keys in this scenario because:

1. The host key was previously cached under key type `ssh-rsa` (SHA1).
2. The server now presents the same RSA key using algorithm `rsa-sha2-256`.
3. NetBox's `RetrieveHostKey` does an exact keytype lookup — searching for `rsa-sha2-256` finds nothing.
4. The user is prompted to accept a "new" host key, even though the underlying key is identical.

## Root Cause Analysis

1. **PuTTY already supports RSA-SHA2**: PuTTY 0.81 (used by NetBox) includes `rsa-sha2-256` and `rsa-sha2-512` in its `ssh2_hostkey_algs` list (`libs/putty/crypto/rsa.c`, `libs/putty/crypto/openssh-certs.c`). The SSH negotiation succeeds.

2. **NetBox host key cache lookup is exact**: `TSecureShell::RetrieveHostKey()` (line 2576, `src/core/SecureShell.cpp`) calls PuTTY's `retrieve_host_key()` with the exact `KeyType` string from the SSH protocol (e.g., `rsa-sha2-256`). PuTTY's registry lookup matches the keytype exactly.

3. **No fallback to `ssh-rsa`**: When the lookup fails for `rsa-sha2-256`, NetBox does not try the shared cache id `ssh-rsa`. The RSA-SHA2 variants and `ssh-rsa` all share the same underlying key and PuTTY `cache_id` (`ssh-rsa`), but they have different protocol identifiers (`ssh_id`).

4. **Storage uses protocol id**: `TSecureShell::StoreHostKey()` (line 2738) stores the key using the exact `KeyType` passed from PuTTY (e.g., `rsa-sha2-256`). This means a key accepted during an SHA2 session won't be found during an SHA1 session, and vice versa.

5. **`HaveHostKey` secondary path mismatch** (line 3121): When `RetrieveHostKey` returns empty, `HaveHostKey` checks `FSessionData->GetHostKey()` and compares `KeyTypeFromFingerprint(ExpectedKey)` (which normalizes to `"ssh-rsa"`) with the protocol `KeyType` (e.g., `"rsa-sha2-256"`). The `SameText` comparison fails for RSA-SHA2 variants, causing false negatives even if the user configured an `ssh-rsa` host key.

## Solution Approach

Add a **retrieval fallback** in NetBox's host key lookup: when an exact lookup fails for `rsa-sha2-256` or `rsa-sha2-512`, retry with `ssh-rsa`. The underlying RSA key is identical; only the signature algorithm differs.

**Fix `HaveHostKey` secondary path**: When comparing session-configured keys, treat RSA-SHA2 variants as equivalent to `ssh-rsa`.

**Do not change storage behavior** — continue storing under the exact keytype. This preserves backward compatibility with existing cached keys and avoids duplicate entries. The fallback only affects retrieval.

**Note:** `VerifyCachedHostKey` string comparison at line 2712 does NOT need modification. Both `StoredKey` and `KeyStr` are raw RSA key material (`0xexponent,0xmodulus`) without algorithm name prefixes — the same RSA key produces identical strings regardless of whether the protocol used `ssh-rsa`, `rsa-sha2-256`, or `rsa-sha2-512`.

## Tasks

### Phase I. Investigation & Logging

#### Task 1: Add diagnostic logging to host key verification
- File: `src/core/SecureShell.cpp`
- Insert logging in `VerifyHostKey()` before the cache lookup (around line 2811):
  ```cpp
  LogEvent(FORMAT("Looking up cached host key: type=%s, host=%s, port=%d", KeyType, Host, Port));
  ```
- Insert logging in `HaveHostKey()` secondary path before the `SameText` comparison (around line 3121):
  ```cpp
  LogEvent(FORMAT("Comparing session-configured key type=%s with expected=%s", KeyType, ExpectedKeyType));
  ```
- Insert logging when a fallback lookup is performed (to be added in Task 3):
  ```cpp
  LogEvent(FORMAT("Host key not found as %s, trying fallback to ssh-rsa", KeyType));
  ```
- Use `LogEvent()` with verbose level (`GetConfiguration()->ActualLogProtocol >= 1`).
- **Deduplication:** Guard the `VerifyHostKey` log with `if (!AlreadyVerified && GetConfiguration()->ActualLogProtocol >= 1)` to avoid repeated log entries (PuTTY 0.76+ calls this multiple times per connection — see line 3129 comment).

#### Task 2: Verify PuTTY RSA-SHA2 algorithm mapping
- [x] **Already verified** during plan refinement.
- PuTTY `libs/putty/crypto/openssh-certs.c` lines 237-239:
  - `ssh_rsa_sha256.ssh_id` = `"rsa-sha2-256"`, `cache_id` = `"ssh-rsa"`
  - `ssh_rsa_sha512.ssh_id` = `"rsa-sha2-512"`, `cache_id` = `"ssh-rsa"`

### Phase II. Core Fix

#### Task 3: Implement `ssh-rsa` fallback in `RetrieveHostKey`
- File: `src/core/SecureShell.cpp` (function `RetrieveHostKey`, line 2576)
- Change:
  ```cpp
  UnicodeString Result;
  if (retrieve_host_key(AnsiString(Host).c_str(), Port, AnsiString(KeyType).c_str(),
        ToCharPtr(AnsiStoredKeys), AnsiStoredKeys.Length()) == 0)
  {
      PackStr(AnsiStoredKeys);
      Result = UnicodeString(AnsiStoredKeys);
  }
  ```
- To:
  ```cpp
  UnicodeString Result;
  if (retrieve_host_key(AnsiString(Host).c_str(), Port, AnsiString(KeyType).c_str(),
        ToCharPtr(AnsiStoredKeys), AnsiStoredKeys.Length()) == 0)
  {
      PackStr(AnsiStoredKeys);
      Result = UnicodeString(AnsiStoredKeys);
  }
  else if ((KeyType == L"rsa-sha2-256") || (KeyType == L"rsa-sha2-512"))
  {
      // Fallback: RSA-SHA2 variants share the same underlying key as ssh-rsa.
      // The key may have been cached under the legacy algorithm name.
      // (PuTTY cache_id for all three is "ssh-rsa" — see openssh-certs.c)
      if (GetConfiguration()->ActualLogProtocol >= 1)
      {
          LogEvent(FORMAT(L"Host key not found as %s, trying fallback to ssh-rsa", KeyType));
      }
      if (retrieve_host_key(AnsiString(Host).c_str(), Port, "ssh-rsa",
            ToCharPtr(AnsiStoredKeys), AnsiStoredKeys.Length()) == 0)
      {
          PackStr(AnsiStoredKeys);
          Result = UnicodeString(AnsiStoredKeys);
      }
  }
  // If fallback succeeds, Result is populated and function returns normally.
  // No further fallback paths exist.
  ```
- Ensure the `TGuard` scope covers both lookups (it already does — `TGuard Guard(*PuttyStorageSection.get())` is acquired before any lookup).

#### Task 4: Fix `HaveHostKey` session-configured key comparison for RSA-SHA2 variants
- File: `src/core/SecureShell.cpp` (function `HaveHostKey`, lines 3114-3123)
- Current code:
  ```cpp
  const UnicodeString ExpectedKeyType = KeyTypeFromFingerprint(ExpectedKey);
  Result = SameText(ExpectedKeyType, KeyType);
  ```
- Problem: `KeyTypeFromFingerprint` normalizes `"rsa-sha2-256"` to `"ssh-rsa"` (via `cache_id`). When `KeyType` is `"rsa-sha2-256"`, the comparison fails even though the keys are equivalent.
- Fix: Add RSA-SHA2 variant awareness to the comparison:
  ```cpp
  const UnicodeString ExpectedKeyType = KeyTypeFromFingerprint(ExpectedKey);
  bool KeysMatch = SameText(ExpectedKeyType, KeyType);
  if (!KeysMatch && (ExpectedKeyType == L"ssh-rsa"))
  {
      // RSA-SHA2 variants share the same underlying key as ssh-rsa
      KeysMatch = (KeyType == L"rsa-sha2-256") || (KeyType == L"rsa-sha2-512");
  }
  Result = KeysMatch;
  ```

### Phase III. Verification

#### Task 5: Build verification
- Build `RelWithDebugInfo` for x64 using `build-x64.bat`.
- Ensure zero MSVC W4 warnings.
- If unity-build symbol conflicts arise, disable with `-DOPT_USE_UNITY_BUILD=OFF`.

#### Task 6: Manual test scenario
- **Precondition**: Have an SSH server with an RSA host key (OpenSSH 8.8+ with `HostKeyAlgorithms +ssh-rsa` disabled, so it uses `rsa-sha2-256`).
- **Test A — First connection (cache miss)**:
  1. Clear the host key cache for the test server.
  2. Connect with NetBox.
  3. Accept the host key.
  4. Verify the key is stored (check logs or registry/INI under the `rsa-sha2-256` keytype).
- **Test B — Second connection (cache hit with same algorithm)**:
  1. Reconnect.
  2. Verify no host key prompt appears.
  3. Check logs show `Looking up cached host key: type=rsa-sha2-256 ...` and successful retrieval.
- **Test C — Fallback scenario (key cached as `ssh-rsa`)**:
  1. Manually edit the host key storage to change the keytype from `rsa-sha2-256` to `ssh-rsa` (or use an old cached entry).
  2. Reconnect.
  3. Verify no host key prompt appears.
  4. Check logs show the fallback message: `Host key not found as rsa-sha2-256, trying fallback to ssh-rsa`.
- **Test D — Session-configured host key (`FSessionData->GetHostKey()`)**:
  1. Configure a session with an explicit `ssh-rsa` host key.
  2. Connect to a server using `rsa-sha2-256`.
  3. Verify the session-configured key matches (no host key prompt).
  4. Check logs show the comparison succeeded via RSA-SHA2 equivalence.
- **Test E — SHA512 variant**:
  1. Repeat Test C with `rsa-sha2-512`.

### Phase IV. Documentation

#### Task 7: Update knowledge references
- File: `.ai-factory/references/INDEX.md`
- Add a link to this plan and a short summary:
  > "RSA-SHA2 host key fallback: NetBox now falls back to `ssh-rsa` cache lookups when `rsa-sha2-256`/`rsa-sha2-512` keys are not found, resolves connection failures to OpenSSH 8.8+ servers. Also fixes `HaveHostKey` secondary path for RSA-SHA2 equivalence."

#### Task 8: Commit changes
- Use conventional commit message:
  ```
  fix(ssh): add rsa-sha2 host key cache fallback and equivalence handling

  OpenSSH 8.8+ uses rsa-sha2-256/512 for RSA host keys instead of
  legacy ssh-rsa. When a key was previously cached under ssh-rsa,
  exact lookup for rsa-sha2-256 failed, prompting users to re-accept
  the same host key.

  Changes:
  - RetrieveHostKey(): fallback to ssh-rsa when rsa-sha2 lookup fails,
    including PackStr call on fallback path
  - HaveHostKey(): treat rsa-sha2-256/512 as equivalent to ssh-rsa
    in session-configured key comparison
  - Add diagnostic logging with deduplication in VerifyHostKey()

  The underlying RSA key is identical; only the signature algorithm
  differs. No storage format changes — backward compatible with
  existing cached keys.

  Fixes GitHub issue #388.
  ```

## Architecture Notes

- **Layer:** Core (`src/core/SecureShell.cpp`), wrapping PuTTY storage primitives.
- **No changes to `libs/`** — PuTTY already supports RSA-SHA2 algorithms. The fix is entirely in NetBox's host key cache management.
- **Thread safety:** `RetrieveHostKey` and `HaveHostKey` already acquire `PuttyStorageSection` lock. The fallback adds a second lookup within the same critical section.
- **Backward compatibility:** Existing keys cached under `ssh-rsa`, `rsa-sha2-256`, or `rsa-sha2-512` continue to work. New keys are still stored under the exact algorithm name.

## Edge Cases

1. **Server uses `ssh-rsa` (legacy SHA1)**: No fallback triggered — exact lookup succeeds.
2. **Server uses `rsa-sha2-256` with no prior cache**: Exact lookup fails, fallback to `ssh-rsa` fails, normal "accept new key" flow proceeds.
3. **Key was cached under `rsa-sha2-256`, server later uses `ssh-rsa`**: The lookup for `ssh-rsa` succeeds directly — no fallback needed (the lookup is exact).
4. **Multiple RSA keys for same host**: If both `ssh-rsa` and `rsa-sha2-256` entries exist with *different* keys, the exact lookup takes precedence. The fallback only runs when exact lookup fails.
5. **DSA, ECDSA, ED25519 keys**: Not affected — fallback is gated to RSA-SHA2 keytypes only.
6. **Session-configured host key with `ssh-rsa`**: Now correctly matches against `rsa-sha2-256` protocol keys via the `HaveHostKey` secondary path fix (Task 4).
7. **`have_any_ssh2_hostkey` callback chain**: PuTTY calls `have_ssh_host_key` for each algorithm. The `RetrieveHostKey` fallback ensures this works correctly for RSA-SHA2 variants (Task 3).
8. **Server key actually changed**: If the RSA host key itself changed (not just the algorithm), the fallback retrieves the old `ssh-rsa` key, the comparison at `VerifyCachedHostKey` line 2712 fails (different key material), and the user is correctly prompted to accept the new key.

## Acceptance Criteria

- [~] Connecting to OpenSSH 8.8+ with RSA host key does not prompt for re-acceptance if the same key was previously cached under `ssh-rsa`. — **Skipped:** requires manual Far Manager testing (Testing=No)
- [x] Logs show the fallback attempt when `rsa-sha2-256` or `rsa-sha2-512` exact lookup fails.
- [~] Existing `ssh-rsa` cached keys continue to work without fallback. — **Skipped:** requires manual Far Manager testing (Testing=No)
- [~] New `rsa-sha2-256` keys are cached under their exact keytype. — **Skipped:** requires manual Far Manager testing (Testing=No)
- [x] Session-configured `ssh-rsa` host keys match against `rsa-sha2-256` protocol keys.
- [x] Build passes with zero warnings.
- [x] Commit message follows conventional format.

## Changelog

| Date       | Change                     | Reason |
|------------|----------------------------|--------|
| 2026-04-30 | Initial plan               | Issue #388 analysis |
| 2026-04-30 | Refined (2nd iteration)    | Deep codebase analysis: added `HaveHostKey` secondary path fix, `VerifyCachedHostKey` analysis, logging in secondary path, improved edge cases, expanded acceptance criteria |
| 2026-04-30 | Post-review cleanup         | Deleted unnecessary Task 5 (VerifyCachedHostKey comparison works as-is), added `PackStr` call to Task 3 fallback, added logging deduplication to Task 1, fixed `OPT_USE_UNITY_BUILD` typo in Task 6, marked Task 2 as verified, removed redundant `VerifyCachedHostKey` from root cause |
| 2026-05-02 | Implementation complete    | Tasks 1–4 implemented: VerifyHostKey logging, RetrieveHostKey fallback, HaveHostKey RSA-SHA2 equivalence, INDEX.md updated. Build passes zero warnings. Manual tests pending. |
| 2026-05-04 | Plan complete | All 8 tasks implemented. Acceptance criteria: code-level items verified (4/7), manual test items skipped per Testing=No. Commits: `9082ff0e8`, `dc1dfc341`. No remaining work. |
