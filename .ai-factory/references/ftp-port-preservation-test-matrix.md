# FTP Port Preservation Fix - Manual Test Matrix

**Reference:** GitHub Issue #42  
**Branch:** Current working branch  
**Date:** 2026-04-28

## Build Prerequisites

Before testing, ensure:
1. Build completed with `build-x64.bat` (zero warnings)
2. `NetBox.dll` exists in `Far3_x64/Plugins/NetBox/`

## Test Cases

### Test 1: New FTP session, port 443
**Steps:**
1. Launch Far Manager: `Far3_x64/Far.exe`
2. Press F11 → NetBox → New Connection
3. Set Protocol: FTP
4. Set Port: 443
5. Set Host: example.com
6. Click "Save"
7. Reopen the saved session

**Expected:** Port field shows 443
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 2: Existing FTP session, default port 21
**Steps:**
1. Create new FTP session
2. Leave port at default (21)
3. Save
4. Reopen

**Expected:** Port field shows 21
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 3: Protocol switch SFTP → FTP
**Steps:**
1. Create SFTP session (port should be 22)
2. In dialog, change Protocol to FTP

**Expected:** Port changes to 21
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 4: Protocol switch FTP → SFTP
**Steps:**
1. Create FTP session (port should be 21)
2. In dialog, change Protocol to SFTP

**Expected:** Port changes to 22
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 5: Protocol switch FTP → FTPS implicit
**Steps:**
1. Create FTP session (port 21)
2. Change Encryption to "Implicit"

**Expected:** Port changes to 990
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 6: Protocol switch WebDAV → HTTPS
**Steps:**
1. Create WebDAV session (port should be 80)
2. Enable HTTPS/TLS

**Expected:** Port changes to 443
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 7: WebDAV with custom port
**Steps:**
1. Create WebDAV session
2. Set Port: 8080
3. Save
4. Reopen

**Expected:** Port shows 8080
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 8: S3 with custom port
**Steps:**
1. Create S3 session
2. Set Port: 8443
3. Save
4. Reopen

**Expected:** Port shows 8443
**Actual:** ____________
**Status:** ☐ PASS  ☐ FAIL

---

### Test 9: Protocol switch FTP → S3
**Steps:**
1. Create FTP session (port 21)
2. Change Protocol to S3

**Expected:** Port changes to 443 (S3 default)
**Actual:** ____________
**Note:** This is a KNOWN BUG - port may remain at 21. Document actual behavior.
**Status:** ☐ PASS  ☐ FAIL (known issue)

---

## Overall Result

**Tester:** ____________  
**Date:** ____________  
**Build:** ____________ (x64/x86)

| Metric | Value |
|--------|-------|
| Tests Passed | ___/9 |
| Tests Failed | ___/9 |
| Known Issues | 1 (S3 port adjustment) |

## Sign-off

**Ready for commit:** ☐ YES  ☐ NO (requires fix revision)
