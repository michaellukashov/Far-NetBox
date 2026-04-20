# Test Cases — dev branch

**Based on:** `test-plan.md`  
**Branch:** `dev` → `main`  
**Date:** 2026-04-20

---

This document provides concrete test scenarios for each test case in the test plan. Steps are written so any tester can execute without knowledge of the codebase.

---

## P1 — High Priority Test Cases

### 1. Terminal Emulation (T-001 to T-004)

#### T-001: Terminal Response Test
**Objective:** Verify terminal responds to commands
**Steps:**
1. Launch Far Manager with NetBox plugin
2. Create/edit session: Protocol=SFTP, Host=`test-server`, User=`testuser`, Password=`testpass`
3. Connect to session
4. In remote terminal, type: `echo "hello world"`
5. Press Enter
**Expected:** Output shows `hello world`
**Pass Criteria:** Terminal displays command output

#### T-002: Terminal Resize Test
**Objective:** Verify window resize is handled
**Steps:**
1. Connect via SFTP (T-001)
2. Resize Far Manager window (make larger)
3. Type: `cat /etc/hostname`
**Expected:** Output displays correctly after resize
**Pass Criteria:** No crash, output displays

#### T-003: Special Keys Test
**Objective:** Verify special keys work
**Steps:**
1. Connect via SFTP (T-001)
2. Press Up Arrow (previous command)
3. Press Down Arrow
4. Press Ctrl+C to interrupt
**Expected:** Keys are interpreted correctly
**Pass Criteria:** Up/Down cycles history, Ctrl+C stops

#### T-004: UTF-8 Output Test
**Objective:** Verify UTF-8 displays
**Steps:**
1. Connect via SFTP (T-001)
2. Type: `echo "Привет мир"`
3. Press Enter
**Expected:** UTF-8 characters display correctly
**Pass Criteria:** No garbled characters

---

### 2. S3 Protocol (S3-001 to S3-007)

#### S3-001: S3 Connection Test
**Objective:** Connect to S3
**Steps:**
1. Launch Far Manager with NetBox plugin
2. Create session: Protocol=S3, Host=`s3.amazonaws.com`, AccessKey=`xxx`, SecretKey=`xxx`
3. Connect
**Expected:** Connection successful, no errors
**Pass Criteria:** Connected state shown

#### S3-002: List Buckets Test
**Objective:** List S3 buckets
**Steps:**
1. Connect to S3 (S3-001)
2. Press Ctrl+R to refresh panel
**Expected:** Bucket list displayed
**Pass Criteria:** All accessible buckets shown

#### S3-003: Upload to S3
**Objective:** Upload file to S3
**Steps:**
1. Connect to S3 (S3-001)
2. Navigate to bucket
3. Press F5 to upload local file (create test file first)
4. Select local test file
**Expected:** Upload completes
**Pass Criteria:** File appears in bucket

#### S3-004: Download from S3
**Objective:** Download file from S3
**Steps:**
1. Connect to S3 (S3-001)
2. Select file in bucket
3. Press F5 to download
4. Choose local destination
**Expected:** Download completes
**Pass Criteria:** File saved locally

#### S3-005: Delete from S3
**Objective:** Delete object
**Steps:**
1. Connect to S3 (S3-001)
2. Navigate to bucket with file
3. Select file, press F8 to delete
4. Confirm deletion
**Expected:** Object deleted
**Pass Criteria:** File removed from bucket

#### S3-006: Create Bucket Test
**Objective:** Create new bucket
**Steps:**
1. Connect to S3 (S3-001)
2. Press F7 to create bucket
3. Enter bucket name: `test-bucket-[date]`
4. Confirm
**Expected:** Bucket created
**Pass Criteria:** New bucket in list

#### S3-007: S3 Error Handling
**Objective:** Handle S3 errors
**Steps:**
1. Create session with invalid credentials
2. Attempt to connect
**Expected:** Error message displayed
**Pass Criteria:** No crash, error shown

---

### 3. SSH/SFTP/SCP (SSH-001 to SSH-008)

#### SSH-001: SSH Password Connection
**Objective:** Connect via password
**Steps:**
1. Launch Far Manager with NetBox plugin
2. Create session: Protocol=SSH, Host=`test-server`, User=`testuser`, Password=`testpass`
3. Connect
**Expected:** Connection successful
**Pass Criteria:** Remote panel shows files

#### SSH-002: SSH Key Authentication
**Objective:** Connect via public key
**Steps:**
1. Create session: Protocol=SSH, Host=`test-server`, User=`testuser`
2. In session settings, enable "Public key" authentication
3. Select SSH key file (or use Pageant)
4. Connect
**Expected:** Key-based connection works
**Pass Criteria:** Connected without password prompt

#### SSH-003: SFTP File Listing
**Objective:** List SFTP files
**Steps:**
1. Connect via SFTP (SSH-001)
2. Navigate directories
3. Check file listing
**Expected:** Files and folders displayed
**Pass Criteria:** Correct file info

#### SSH-004: SFTP Upload
**Objective:** Upload via SFTP
**Steps:**
1. Connect via SFTP (SSH-001)
2. In local panel, select file
3. Press F5 to upload
4. Confirm
**Expected:** Upload completes
**Pass Criteria:** File on remote server

#### SSH-005: SFTP Download
**Objective:** Download via SFTP
**Steps:**
1. Connect via SFTP (SSH-001)
2. In remote panel, select file
3. Press F5 to download
4. Choose destination
**Expected:** Download completes
**Pass Criteria:** File saved locally

#### SSH-006: SCP Transfer
**Objective:** Use SCP protocol
**Steps:**
1. Create session: Protocol=SCP, Host=`test-server`, User=`testuser`, Password=`testpass`
2. Connect
3. Upload file with F5
**Expected:** SCP transfer works
**Pass Criteria:** File transferred

#### SSH-007: Connection Timeout
**Objective:** Handle timeout gracefully
**Steps:**
1. Create session with unreachable host
2. Set connection timeout to 10 seconds
3. Connect
**Expected:** Timeout after ~10 seconds
**Pass Criteria:** Error message, no hang

#### SSH-008: Reconnection
**Objective:** Reconnect after disconnect
**Steps:**
1. Connect via SFTP (SSH-001)
2. Press Ctrl+B or use menu to disconnect
3. Reconnect to same session
**Expected:** Reconnects successfully
**Pass Criteria:** New connection established

---

### 4. Build System (B-001 to B-004)

#### B-001: x64 Release Build
**Objective:** Build x64 Release
**Steps:**
1. Open VS2022 x64 Native Tools Command
2. Run: `build-x64.bat` (or `cmake --build build-RelWithDebugInfo`)
3. Wait for completion
**Expected:** Builds without errors
**Pass Criteria:** Exit code 0, DLL created

#### B-002: x86 Release Build
**Objective:** Build x86 Release
**Steps:**
1. Open VS2022 x86 Native Tools Command
2. Run: `build-x86.bat`
3. Wait for completion
**Expected:** Builds without errors
**Pass Criteria:** Exit code 0, DLL created

#### B-003: ARM64 Release Build
**Objective:** Build ARM64 Release
**Steps:**
1. Open VS2022 ARM Tools Command
2. Run: `build-arm64.bat`
3. Wait for completion
**Expected:** Builds without errors
**Pass Criteria:** Exit code 0, DLL created

#### B-004: Warnings Check
**Objective:** Verify zero warnings
**Steps:**
1. Run build with W4 enabled (default)
2. Review build output
3. Check for warnings
**Expected:** Zero warnings
**Pass Criteria:** No W3/W4 warnings

---

## P2 — Medium Priority Test Cases

### 5. Transfer Queue (Q-001 to Q-004)

#### Q-001: Queue Multiple Files
**Objective:** Queue multiple transfers
**Steps:**
1. Connect via SFTP (SSH-001)
2. Select multiple files in local panel
3. Press F5 to queue
4. Add more files to queue
**Expected:** All files in queue
**Pass Criteria:** Queue shows all files

#### Q-002: Cancel Transfer
**Objective:** Cancel queued transfer
**Steps:**
1. With files queued (Q-001)
2. View queue (Shift+F4)
3. Select transfer, press F8
4. Confirm cancel
**Expected:** Transfer cancelled
**Pass Criteria:** Removed from queue

#### Q-003: Pause/Resume Queue
**Objective:** Test queue control
**Steps:**
1. With queue (Q-001)
2. Press F6 to pause
3. Press F6 to resume
**Expected:** Queue pauses and resumes
**Pass Criteria:** State changes

#### Q-004: Duplicate Name Warning
**Objective:** Warning for duplicates
**Steps:**
1. Queue file `test.txt`
2. Queue same file again
**Expected:** Warning displayed
**Pass Criteria:** Warning shown

---

### 6. Windows Configuration (C-001 to C-004)

#### C-001: Save Configuration
**Objective:** Save settings
**Steps:**
1. Open NetBox settings (F9)
2. Change some options
3. Press Save
4. Exit Far Manager
**Expected:** Settings saved
**Pass Criteria:** No errors

#### C-002: Load Configuration
**Objective:** Load settings
**Steps:**
1. Start Far Manager with NetBox
2. Open settings
**Expected:** Previous settings loaded
**Pass Criteria:** Settings restored

#### C-003: Reset to Defaults
**Objective:** Reset settings
**Steps:**
1. Open NetBox settings
2. Press "Reset to Defaults"
3. Confirm
4. Exit and restart
**Expected:** Default settings applied
**Pass Criteria:** All defaults

#### C-004: Export/Import Config
**Objective:** Export/import settings
**Steps:**
1. Open settings
2. Export to file
3. Reset to defaults
4. Import from file
**Expected:** Import works
**Pass Criteria:** Settings restored

---

### 7. Session Data (S-001 to S-004)

#### S-001: Save Session
**Objective:** Save session
**Steps:**
1. Create new session with settings
2. Press Save in session dialog
3. Name: `Test Session`
**Expected:** Session saved
**Pass Criteria:** In session list

#### S-002: Load Session
**Objective:** Load session
**Steps:**
1. Open session list (Shift+F4)
2. Select saved session
3. Connect
**Expected:** Session loads
**Pass Criteria:** Settings restored

#### S-003: Duplicate Session
**Objective:** Copy session
**Steps:**
1. Select session
2. Press F5 to duplicate
3. Modify and save
**Expected:** Duplicate created
**Pass Criteria:** New session exists

#### S-004: Special Characters
**Objective:** Handle special chars
**Steps:**
1. Create session with name `Test /:_chars`
2. Save session
3. Load and verify
**Expected:** Handled correctly
**Pass Criteria:** No corruption

---

## P3 — Lower Priority Test Cases

### 8. Plugin Layer (P-001 to P-004)

#### P-001: Plugin Load
**Objective:** Load NetBox plugin
**Steps:**
1. Start Far Manager 3.0
2. Press F11
3. Find NetBox in plugins
**Expected:** Plugin in list
**Pass Criteria:** No errors

#### P-002: Connection Dialog
**Objective:** Open connection dialog
**Steps:**
1. With NetBox loaded
2. Press Shift+F4
**Expected:** Dialog opens
**Pass Criteria:** Dialog visible

#### P-003: External Modification
**Objective:** Detect external changes
**Steps:**
1. Connect via SFTP (SSH-001)
2. In another program, modify remote file
3. Refresh panel (Ctrl+R)
**Expected:** Detected
**Pass Criteria:** Warning shown

#### P-004: Native Edit
**Objective:** Edit file natively
**Steps:**
1. Connect via SFTP (SSH-001)
2. Select file
3. Press F4 to edit
**Expected:** Opens in editor
**Pass Criteria:** Editor launches

---

### 9. Help (H-001 to H-002)

#### H-001: Access Help
**Objective:** View help
**Steps:**
1. In NetBox, press F1
**Expected:** Help displays
**Pass Criteria:** Help visible

#### H-002: Search Help
**Objective:** Search help
**Steps:**
1. In help, type search term
**Expected:** Results shown
**Pass Criteria:** Matches found

---

## Environment Setup Notes

### Test Servers
If no test servers available:
- Use localhost SSH server (OpenSSH for Windows or Windows Subsystem for Linux)
- Use MinIO for S3 testing (Docker: `docker run -p 9000:9000 minio/minio server /data`)
- Use FileZilla Server for FTP

### Test File Creation
```cmd
:: Create test files
echo test content > test.txt
:: Create large file
fsutil file createnew large.bin 104857600
:: Unicode filename
echo test > file-тест.txt
```

---

## Test Execution Order

Recommend executing in this order:
1. **B-001** first — verify build works
2. **P-001** — verify plugin loads
3. **SSH** tests — verify basic connectivity
4. **S3** tests — if S3 available
5. **Queue, Config, Session** tests
6. **Terminal** tests
7. **Help** tests

---

## Pass/Fail Recording

| Test Case | Pass | Fail | Notes |
|----------|------|------|-------|
| T-001 | [ ] | [ ] | |
| T-002 | [ ] | [ ] | |
| ... | | | | |

---

## Known Limitations

- ARM64 testing requires ARM Windows device
- S3 tests require valid credentials
- Some tests need network access

EOF