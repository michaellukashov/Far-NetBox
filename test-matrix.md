# Manual Regression Test Matrix — FTP Port Preservation Fix

**Date:** 2026-04-29
**Tester:** Human (Far Manager 3.x on Windows x64)
**Build Required:** `Far3_x64/Plugins/NetBox/NetBox.dll` built with `OPT_CREATE_PLUGIN_DIR=ON`

## Prerequisite

1. Build the project (`cmd /c build-x64.bat`) and confirm `Far3_x64/Plugins/NetBox/NetBox.dll` exists.
2. Launch `Far3_x64/Far.exe`.
3. Press `F11` → `NetBox` to open the plugin.

---

## Test Cases

### 1. New FTP session, port 443
- **Steps:**
  1. Create a new FTP session.
  2. Set port to `443`.
  3. Save and close the dialog.
  4. Reopen the session edit dialog.
- **Expected:** Port field shows `443`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 2. Existing FTP session, default port 21
- **Steps:**
  1. Create a new FTP session.
  2. Leave port at default (`21`).
  3. Save and close the dialog.
  4. Reopen the session edit dialog.
- **Expected:** Port field shows `21`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 3. Protocol switch SFTP -> FTP
- **Steps:**
  1. Create an SFTP session (port should default to `22`).
  2. In the dialog, switch protocol from SFTP to FTP.
- **Expected:** Port field changes to `21`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 4. Protocol switch FTP -> SFTP
- **Steps:**
  1. Create an FTP session (port should default to `21`).
  2. In the dialog, switch protocol from FTP to SFTP.
- **Expected:** Port field changes to `22`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 5. Protocol switch FTP -> FTPS implicit
- **Steps:**
  1. Create an FTP session (port should default to `21`).
  2. In the dialog, switch encryption to **Implicit SSL/TLS** (FTPS implicit).
- **Expected:** Port field changes to `990`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 6. Protocol switch WebDAV -> HTTPS
- **Steps:**
  1. Create a WebDAV session (port should default to `80`).
  2. In the dialog, switch to HTTPS (WebDAVS).
- **Expected:** Port field changes to `443`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 7. WebDAV with custom port
- **Steps:**
  1. Create a WebDAV session.
  2. Set port to `8080`.
  3. Save and close the dialog.
  4. Reopen the session edit dialog.
- **Expected:** Port field shows `8080`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 8. S3 with custom port
- **Steps:**
  1. Create an S3 session.
  2. Set port to `8443`.
  3. Save and close the dialog.
  4. Reopen the session edit dialog.
- **Expected:** Port field shows `8443`.
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL

---

### 9. Protocol switch FTP -> S3 (regression test for S3 adjustment)
- **Steps:**
  1. Create an FTP session (port should default to `21`).
  2. In the dialog, switch protocol to S3.
- **Expected:** Port field changes to `443` (S3 default HTTPS port).
- **Actual:** (fill after testing)
- **Status:** ⬜ PASS / ⬜ FAIL
  - **Note:** If port stays at `21`, this confirms the known S3 no-op bug documented in `findings.md` Task 9. It is **not** a failure of the FTP port preservation fix itself.

---

## Summary

| # | Test Case | Status |
|---|-----------|--------|
| 1 | New FTP session, port 443 | ⬜ |
| 2 | Existing FTP session, default port 21 | ⬜ |
| 3 | Protocol switch SFTP -> FTP | ⬜ |
| 4 | Protocol switch FTP -> SFTP | ⬜ |
| 5 | Protocol switch FTP -> FTPS implicit | ⬜ |
| 6 | Protocol switch WebDAV -> HTTPS | ⬜ |
| 7 | WebDAV with custom port | ⬜ |
| 8 | S3 with custom port | ⬜ |
| 9 | Protocol switch FTP -> S3 | ⬜ |

---

## Tester Instructions

1. Run each test case in Far Manager.
2. Fill the **Actual** and **Status** columns above.
3. If all cases 1–8 pass and case 9 behaves as documented, reply with:
   `Task 6: PASS — all test cases confirmed.`
4. If any case fails, reply with:
   `Task 6: FAIL — case #<N> failed. Expected: <X>, Actual: <Y>.`
