# Change Summary — dev branch

**Branch:** `dev` → `main` comparison  
**Date:** 2026-04-20  
**Artifacts:** `.ai-factory/qa/dev-38f8e886/`

---

## Overview

The `dev` branch contains significant changes across the NetBox project including:
- Upstream library updates (OpenSSL 3.3.7, PuTTY 0.81, zlib-ng)
- New features in SFTP/FTP/SCP protocols
- New functionality in S3, WebDAV connections
- Terminal and session management improvements
- Bug fixes and code modernization

This is a major integration branch syncing the NetBox codebase with updated upstream dependencies while adding NetBox-specific features.

---

## Changes by Component

### NetBox Plugin Layer (`src/NetBox/`)

| File | Changes | Risk |
|------|---------|------|
| `WinSCPFileSystem.cpp` | +247/-0 lines: External modification detection, native edit fixes | Medium |
| `NetBox.en.hlf`, `NetBox.ru.hlf` | +500/+523 lines: Help file updates | Low |
| `WinSCPDialogs.cpp` | +97/-0 lines: UI dialog improvements | Medium |
| `WinSCPPlugin.cpp` | +14 lines: Plugin initialization | Low |
| `FarPlugin.cpp` | +13 lines: Plugin setup | Low |
| Other files | Minor fixes | Low |

**Summary:** Plugin layer has moderate changes to dialogs and file system handling. High-risk areas are external modification detection and native edit functionality.

### Core Protocols (`src/core/`)

| File | Changes | Risk |
|------|---------|------|
| `Terminal.cpp/h` | +389/+85 lines: Terminal emulation improvements | **High** |
| `S3FileSystem.cpp/h` | +999/+22 lines: S3 protocol enhancements | **High** |
| `SecureShell.cpp/h` | +225/+34 lines: SSH connection handling | **High** |
| `ScpFileSystem.cpp/h` | +196/+19 lines: SCP protocol | Medium |
| `SftpFileSystem.cpp/h` | +116/+2 lines: SFTP protocol | Medium |
| `Queue.cpp/h` | +143/+68 lines: Transfer queue | Medium |
| `RemoteFiles.cpp/h` | +119/+72 lines: Remote file handling | Medium |
| `FtpFileSystem.cpp/h` | +66/+9 lines: FTP protocol | Medium |
| `WebDAVFileSystem.cpp/h` | +96/+27 lines: WebDAV updates | Medium |
| `Win32Input.cpp/h` | +118/+36 lines: Terminal input | Medium |
| `PuttyIntf.cpp` | +172 lines: PuTTY interface | Medium |
| `SessionData.cpp/h` | +92/+47 lines: Session configuration | Medium |
| `KittyKeyboard.cpp/h` | +80/+45 lines: Keyboard handling | Medium |
| Other files | Various improvements | Low |

**Summary:** Core protocol layer has the most changes. High-risk areas are Terminal, S3, and SecureShell implementations due to their complexity and networking nature.

### Base Layer (`src/base/`)

| File | Changes | Risk |
|------|---------|------|
| `Sysutils.cpp` | +211 lines: System utilities | Medium |
| `Common.cpp/h` | +61 lines: Common utilities | Low |
| `FormatUtils.cpp/h` | +98/+4 lines: Formatting | Low |
| Other files | Minor updates | Low |

**Summary:** Base layer has moderate changes but lower risk - mostly utility functions.

### Windows UI (`src/windows/`)

| File | Changes | Risk |
|------|---------|------|
| `WinConfiguration.cpp/h` | +17/+181 lines: Windows configuration | Medium |
| `GUIConfiguration.cpp/h` | +116/+2 lines: GUI settings | Low |
| `WinInterface.cpp/h` | +59/+21 lines: Windows interface | Medium |
| `WinApi.h` | +96 lines: Windows API wrappers | Medium |
| Other files | Minor changes | Low |

**Summary:** Windows UI has moderate changes to configuration system.

### Build System

| File | Changes | Risk |
|------|---------|------|
| `src/CMakeLists.txt` | +72 lines: Build configuration | **High** |

**Summary:** CMake changes affect all builds.

### Dependencies Updated

| Library | Version | Impact |
|---------|---------|--------|
| OpenSSL | 3.3.7 | Cryptography, TLS/SSL |
| PuTTY | 0.81 | SSH/SCP/SFTP |
| zlib-ng | Latest | Compression |
| tinylog | Latest | Logging |
| tinyxml2 | Latest | XML parsing |

**Note:** Third-party changes in `libs/` are NOT covered by this QA - they are upstream updates.

---

## Risk Assessment

### High Risk Areas

1. **Terminal Emulation** (`Terminal.cpp`)
   - Complex state machine for terminal handling
   - Changes affect interactive sessions

2. **S3 Protocol** (`S3FileSystem.cpp`)
   - Amazon S3 implementation with many code paths
   - Authentication, bucket, object operations

3. **SecureShell/SSH** (`SecureShell.cpp`, `ScpFileSystem.cpp`, `SftpFileSystem.cpp`)
   - Connection establishment, key exchange
   - Authentication workflows

4. **Build System** (`CMakeLists.txt`)
   - Affects all platform builds
   - Configuration changes may break builds

### Medium Risk Areas

5. **Transfer Queue** (`Queue.cpp`)
   - Affects all file transfers
   - Threading and synchronization

6. **Windows Configuration** (`WinConfiguration.cpp`)
   - GUI settings persistence
   - Registry/XML handling

7. **Session Management** (`SessionData.cpp`, `SessionInfo.cpp`)
   - Configuration storage
   - Password/key storage

### Low Risk Areas

- Help files, language files
- Resource files
- Minor utility functions

---

## Test Priority

| Priority | Area | Reason |
|----------|------|--------|
| **P1** | Terminal | Core functionality, many code paths |
| **P1** | S3 Protocol | Complex, many operations |
| **P1** | SSH/SFTP/SCP | Primary protocols |
| **P1** | Build | Impacts all functionality |
| **P2** | Transfer Queue | Used in all transfers |
| **P2** | Windows Config | GUI functionality |
| **P2** | Session Data | Configuration issues |
| **P3** | Help/Languages | Documentation |

---

## Key Integration Points

1. **Terminal → PuTTY** - Terminal emulation relies on PuTTY's terminal handling
2. **S3 → libs3** - Amazon S3 operations via libs3 library
3. **SecureShell → OpenSSL** - Cryptography operations
4. **WinConfiguration → Registry** - Windows configuration storage

---

## Recommendations

1. **Focus testing on High Risk areas** - Terminal, S3, SSH/SFTP
2. **Verify build success** on all platforms (x86, x64, ARM64)
3. **Test protocol connections** - SSH, FTP, S3, WebDAV
4. **Verify configuration save/load** works correctly
5. **Check transfer operations** across all protocols