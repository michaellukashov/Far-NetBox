# Test Plan — dev branch

**Based on:** `change-summary.md`  
**Branch:** `dev` → `main`  
**Date:** 2026-04-20

---

## Test Strategy

### Scope
- **In Scope:** NetBox source code changes (`src/`), build system
- **Out of Scope:** Third-party libraries (`libs/`) - upstream updates

### Approach
- **Priority-based testing** - High-risk areas first
- **Protocol integration testing** - Test actual connections where possible
- **Regression testing** - Verify existing functionality still works

---

## Test Areas

### P1 — High Priority (Core Functionality)

#### 1. Terminal Emulation
**Files:** `Terminal.cpp`, `Terminal.h`, `Win32Input.cpp`, `Win32Input.h`
**Risk:** Complex state machine, interactive sessions

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| T-001 | Connect via SSH and run interactive command | Terminal responds correctly |
| T-002 | Test terminal resize during session | Window resize handled |
| T-003 | Test special key sequences (Fn, arrows) | Keys interpreted correctly |
| T-004 | Test non-ASCII output | UTF-8 output displays correctly |

#### 2. S3 Protocol
**Files:** `S3FileSystem.cpp`, `S3FileSystem.h`
**Risk:** Amazon S3 implementation, many operations

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| S3-001 | Connect to S3 with access key | Successfully connects |
| S3-002 | List buckets | Bucket list displayed |
| S3-003 | Upload file to bucket | File uploaded successfully |
| S3-004 | Download file from bucket | File downloaded correctly |
| S3-005 | Delete object from bucket | Object deleted |
| S3-006 | Create new bucket | Bucket created |
| S3-007 | Handle S3 errors gracefully | Error message displayed |

#### 3. SSH/SFTP/SCP Protocols
**Files:** `SecureShell.cpp`, `SftpFileSystem.cpp`, `ScpFileSystem.cpp`
**Risk:** Primary protocols, connection handling

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| SSH-001 | Connect via SSH (password) | Connection established |
| SSH-002 | Connect via SSH (public key) | Key-based auth works |
| SSH-003 | SFTP file listing | Remote files listed |
| SSH-004 | SFTP upload file | Upload completes |
| SSH-005 | SFTP download file | Download completes |
| SSH-006 | SCP copy file | SCP transfer works |
| SSH-007 | Handle connection timeout | Graceful timeout |
| SSH-008 | Reconnect after disconnect | Reconnection works |

#### 4. Build System
**Files:** `src/CMakeLists.txt`
**Risk:** Affects all builds

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| B-001 | Build x64 Release | Builds without errors |
| B-002 | Build x86 Release | Builds without errors |
| B-003 | Build ARM64 Release | Builds without errors |
| B-004 | Build with warnings enabled (W4) | Zero warnings |

---

### P2 — Medium Priority

#### 5. Transfer Queue
**Files:** `Queue.cpp`, `Queue.h`
**Risk:** Affects all file transfers

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| Q-001 | Queue multiple files | All files queued |
| Q-002 | Cancel queued transfer | Transfer stopped |
| Q-003 | Pause/resume queue | Queue state managed |
| Q-004 | Queue with same name | Warning displayed |

#### 6. Windows Configuration
**Files:** `WinConfiguration.cpp`, `WinConfiguration.h`
**Risk:** GUI settings persistence

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| C-001 | Save configuration | Settings persisted |
| C-002 | Load configuration | Settings restored |
| C-003 | Reset to defaults | Defaults applied |
| C-004 | Configuration import/export | Import/export works |

#### 7. Session Data
**Files:** `SessionData.cpp`, `SessionData.h`
**Risk:** Configuration issues

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| S-001 | Save session | Session saved to XML |
| S-002 | Load session | Session restored |
| S-003 | Duplicate session | Copy created |
| S-004 | Session with special chars | Handled correctly |

---

### P3 — Lower Priority

#### 8. Plugin Layer
**Files:** `WinSCPFileSystem.cpp`, `WinSCPDialogs.cpp`
**Risk:** UI functionality

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| P-001 | Load NetBox in Far Manager | Plugin loads |
| P-002 | Open connection dialog | Dialog displays |
| P-003 | External modification detection | Works correctly |
| P-004 | Native edit | Edit launches |

#### 9. Help and Languages
**Files:** `NetBox.en.hlf`, `NetBox.ru.hlf`
**Risk:** Documentation

| Test Case | Description | Expected Result |
|----------|-------------|-----------------|
| H-001 | Access help from menu | Help displays |
| H-002 | Search help | Search works |

---

## Test Environment

### Required
- Far Manager 3.0 (x64 or x86)
- Test server accounts: SSH, FTP, S3, WebDAV
- Network connectivity

### Platforms
- Windows x64 (primary)
- Windows x86
- Windows ARM64

### Test Data
- Various file sizes (empty, small, large)
- Unicode filenames
- Special characters in paths

---

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| No S3 test account | Use MinIO local server or mock |
| Network issues | Use localhost testing |
| Platform differences | Test all three platforms |
| Build failures | Run clean build first |

---

## Sign-Off Criteria

All P1 test cases must pass before sign-off:
- [ ] Terminal tests pass (T-001 to T-004)
- [ ] S3 tests pass (S3-001 to S3-007)
- [ ] SSH/SFTP tests pass (SSH-001 to SSH-008)
- [ ] Build tests pass (B-001 to B-004)

---

## Notes

- Third-party library changes (`libs/`) are upstream - not tested in this QA
- Use existing test servers where available
- Document any test limitations