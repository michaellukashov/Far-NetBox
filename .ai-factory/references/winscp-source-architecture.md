# WinSCP Source Architecture Reference

> Source: D:\Projects\WinSCP-work\winscp-master\source
> Created: 2026-04-22
> Updated: 2026-04-22

## Overview

WinSCP is a Windows SFTP/FTP/SCP/WebDAV/S3 client built with C++ and Borland C++ Builder (VCL framework). The source tree is organized into protocol implementations, UI forms, Windows integration, and third-party libraries (PuTTY for SSH, FileZilla for FTP).

**Key characteristics:**
- C++ with VCL (Visual Component Library) framework
- Multi-protocol support: SFTP, SCP, FTP/FTPS, WebDAV, S3
- Built on top of PuTTY (SSH/SFTP), FileZilla (FTP), and Neon (WebDAV/HTTP)
- Windows-only application with GUI and console modes
- Uses Borland C++ Builder project files (.cbproj)

## Directory Structure

### Core Protocol Layer (`source/core/`)

Protocol implementations and session management:

|File|Purpose|
|---|---|
|`Terminal.h/cpp`|Main terminal session controller - coordinates all file operations|
|`SessionData.h/cpp`|Session configuration (host, port, credentials, protocol settings)|
|`SessionInfo.h/cpp`|Runtime session information and statistics|
|`FileSystems.h/cpp`|Abstract filesystem interface (`TCustomFileSystem`)|
|`SftpFileSystem.h/cpp`|SFTP protocol implementation via PuTTY|
|`ScpFileSystem.h/cpp`|SCP protocol implementation via PuTTY|
|`FtpFileSystem.h/cpp`|FTP/FTPS protocol implementation via FileZilla|
|`WebDAVFileSystem.h/cpp`|WebDAV protocol implementation via Neon|
|`S3FileSystem.h/cpp`|Amazon S3 protocol implementation via libs3|
|`SecureShell.h/cpp`|SSH connection wrapper around PuTTY|
|`PuttyIntf.h/cpp`|PuTTY integration interface|
|`NeonIntf.h/cpp`|Neon (WebDAV/HTTP) integration interface|
|`RemoteFiles.h/cpp`|Remote file/directory representation|
|`FileOperationProgress.h/cpp`|Transfer progress tracking|
|`CopyParam.h/cpp`|File transfer parameters (permissions, timestamps, etc.)|
|`Queue.h/cpp`|Background transfer queue|
|`Configuration.h/cpp`|Application configuration management|
|`Common.h/cpp`|Common utilities and helpers|
|`Exceptions.h/cpp`|Exception classes|
|`Cryptography.h/cpp`|Encryption and key management|
|`FileMasks.h/cpp`|File mask/pattern matching|
|`Bookmarks.h/cpp`|Directory bookmarks|
|`Script.h/cpp`|Scripting/automation support|

### UI Layer (`source/forms/`)

VCL forms for GUI dialogs and windows:

|Form|Purpose|
|---|---|
|`ScpExplorer.h/cpp/dfm`|Explorer-style dual-pane interface|
|`ScpCommander.h/cpp/dfm`|Commander-style dual-pane interface|
|`CustomScpExplorer.h/cpp/dfm`|Base class for main windows|
|`Login.h/cpp/dfm`|Session login/connection dialog|
|`SiteAdvanced.h/cpp/dfm`|Advanced session settings|
|`Preferences.h/cpp/dfm`|Application preferences dialog|
|`Properties.h/cpp/dfm`|File properties dialog|
|`Copy.h/cpp/dfm`|File copy/transfer dialog|
|`Synchronize.h/cpp/dfm`|Directory synchronization dialog|
|`SynchronizeChecklist.h/cpp/dfm`|Synchronization preview checklist|
|`Progress.h/cpp/dfm`|Transfer progress dialog|
|`MessageDlg.h/cpp/dfm`|Custom message dialogs|
|`Authenticate.h/cpp/dfm`|Authentication prompts (password, keyboard-interactive)|
|`Editor.h/cpp/dfm`|Internal text editor|
|`FileFind.h/cpp/dfm`|Remote file search dialog|
|`About.h/cpp/dfm`|About dialog|

### Windows Integration (`source/windows/`)

Windows-specific functionality:

|File|Purpose|
|---|---|
|`WinInterface.h/cpp`|Main Windows GUI interface implementation|
|`WinConfiguration.h/cpp`|Windows-specific configuration|
|`GUIConfiguration.h/cpp`|GUI configuration and settings|
|`WinMain.cpp`|Application entry point|
|`TerminalManager.h/cpp`|Multi-session terminal management|
|`Tools.h/cpp`|Windows utility functions|
|`VCLCommon.h/cpp`|VCL framework helpers|
|`WinApi.h`|Windows API declarations and wrappers|
|`Setup.h/cpp`|Installation/setup logic|

### Third-Party Libraries

#### PuTTY (`source/putty/`)

SSH/SFTP implementation from PuTTY project:

- `ssh.h` - SSH protocol definitions
- `putty.h` - PuTTY core definitions
- `network.h` - Network layer
- `conf.h` - Configuration structures
- `ssh/` - SSH protocol implementation
- `crypto/` - Cryptographic primitives
- `windows/` - Windows platform layer

#### FileZilla (`source/filezilla/`)

FTP/FTPS implementation from FileZilla project:

|File|Purpose|
|---|---|
|`FileZillaApi.h/cpp`|FileZilla API wrapper|
|`FtpControlSocket.h/cpp`|FTP control connection|
|`TransferSocket.h/cpp`|FTP data transfer socket|
|`FtpListResult.h/cpp`|FTP directory listing parser|
|`AsyncSocketEx.h/cpp`|Asynchronous socket wrapper|
|`AsyncSslSocketLayer.h/cpp`|SSL/TLS layer for FTP|
|`AsyncProxySocketLayer.h/cpp`|Proxy support|

#### Components (`source/components/`)

Third-party VCL components and libraries.

### Resources (`source/resource/`)

- Language files (`.lng`)
- Resource scripts (`.rc`)
- Icons and images
- License texts
- Help files

## Core Architecture Patterns

### Protocol Abstraction

All protocol implementations inherit from `TCustomFileSystem` (defined in `FileSystems.h`):

```cpp
class TCustomFileSystem
{
public:
  virtual void __fastcall Open() = 0;
  virtual void __fastcall Close() = 0;
  virtual bool __fastcall GetActive() = 0;
  virtual void __fastcall ReadDirectory(TRemoteFileList * FileList) = 0;
  virtual void __fastcall CopyToLocal(TStrings * FilesToCopy, ...) = 0;
  virtual void __fastcall CopyToRemote(TStrings * FilesToCopy, ...) = 0;
  virtual void __fastcall DeleteFile(const UnicodeString FileName, ...) = 0;
  virtual void __fastcall RenameFile(...) = 0;
  virtual void __fastcall CreateDirectory(...) = 0;
  // ... many more operations
};
```

Concrete implementations:
- `TSFTPFileSystem` - SFTP via PuTTY
- `TSCPFileSystem` - SCP via PuTTY
- `TFTPFileSystem` - FTP/FTPS via FileZilla
- `TWebDAVFileSystem` - WebDAV via Neon
- `TS3FileSystem` - Amazon S3 via libs3

### Session Management

`TTerminal` (in `Terminal.h/cpp`) is the central session controller:

```cpp
class TTerminal : public TObject
{
private:
  TSessionData * FSessionData;
  TCustomFileSystem * FFileSystem;
  TSessionLog * FLog;
  TActionLog * FActionLog;
  
public:
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall ProcessDirectory(const UnicodeString DirName, ...);
  void __fastcall CopyToLocal(TStrings * FilesToCopy, ...);
  void __fastcall CopyToRemote(TStrings * FilesToCopy, ...);
  // ... delegates to FFileSystem
};
```

**Key responsibilities:**
- Creates appropriate `TCustomFileSystem` based on `TSessionData::FSProtocol`
- Manages connection lifecycle
- Handles logging and error reporting
- Coordinates file operations
- Manages background queue

### Configuration Hierarchy

```
TConfiguration (base configuration)
  ├─ TSessionData (per-session settings)
  ├─ TWinConfiguration (Windows-specific)
  └─ TGUIConfiguration (GUI settings)
```

### VCL Framework Integration

WinSCP uses Borland VCL (Visual Component Library):

- Forms defined in `.dfm` files (Delphi Form Module)
- C++ classes in `.h/.cpp` files
- `__fastcall` calling convention for VCL methods
- `TObject` base class for most objects
- `UnicodeString` for all string handling
- `TStrings` / `TStringList` for string collections

## Protocol-Specific Details

### SFTP (via PuTTY)

**Entry point:** `TSFTPFileSystem` in `SftpFileSystem.h/cpp`

**Key classes:**
- `TSFTPPacket` - SFTP protocol packet handling
- `TSecureShell` - SSH connection wrapper
- `TSFTPQueue` - Asynchronous operation queue

**Features:**
- SFTP protocol versions 3-6
- Parallel transfers
- Resume support
- Checksum calculation (MD5, SHA-1, SHA-256)

### FTP/FTPS (via FileZilla)

**Entry point:** `TFTPFileSystem` in `FtpFileSystem.h/cpp`

**Key classes:**
- `TFileZillaIntf` - FileZilla API wrapper
- `TFTPServerCapabilities` - Server feature detection

**Features:**
- Active/passive mode
- Implicit/explicit SSL/TLS
- MLSD/MLST support
- IPv6 support
- Proxy support (SOCKS, HTTP)

### WebDAV (via Neon)

**Entry point:** `TWebDAVFileSystem` in `WebDAVFileSystem.h/cpp`

**Features:**
- HTTP/HTTPS
- WebDAV locking
- Chunked transfer encoding
- Digest authentication

### S3 (via libs3)

**Entry point:** `TS3FileSystem` in `S3FileSystem.h/cpp`

**Features:**
- Amazon S3 and compatible services
- Virtual-hosted-style and path-style URLs
- Multipart uploads
- Server-side encryption

## Common Patterns

### Error Handling

```cpp
FILE_OPERATION_LOOP_BEGIN
{
  // operation code
}
FILE_OPERATION_LOOP_END(FMTLOAD(OPERATION_ERROR, (FileName)))
```

Macro-based retry loop for file operations.

### String Types

- `UnicodeString` - UTF-16 string (VCL type)
- `AnsiString` - ANSI string (legacy)
- `RawByteString` - Raw byte buffer
- `UTF8String` - UTF-8 encoded string

### Callbacks and Events

VCL closure syntax:
```cpp
typedef void __fastcall (__closure *TQueryUserEvent)
  (TObject * Sender, const UnicodeString Query, ...);
```

### Resource Management

- RAII via VCL `TObject` and `__finally` blocks
- `std::unique_ptr` for modern C++ code
- Manual `delete` in older code

## Build System

**Project files:**
- `WinSCP.cbproj` - Main GUI application
- `Console.cbproj` - Console mode
- `WinSCP.groupproj` - Project group

**Dependencies:**
- Borland C++ Builder (now Embarcadero RAD Studio)
- VCL framework
- OpenSSL (for SSL/TLS)
- zlib (compression)

## Integration Points for NetBox

NetBox reuses WinSCP's core protocol layer. Key integration areas:

### 1. Protocol Implementations

NetBox uses these WinSCP classes directly:
- `TTerminal` - session management
- `TSessionData` - configuration
- `TSFTPFileSystem`, `TFTPFileSystem`, etc. - protocol implementations
- `TRemoteFile` / `TRemoteFileList` - file representations

### 2. Configuration

NetBox adapts WinSCP's configuration system:
- `TConfiguration` base class
- `TSessionData` for connection settings
- Custom storage backend (Far Manager registry vs. Windows registry)

### 3. UI Callbacks

WinSCP uses VCL events; NetBox replaces with Far Manager dialogs:
- `TQueryUserEvent` - user prompts
- `TPromptUserEvent` - password/passphrase input
- `TFileOperationProgressType` - progress display

### 4. Logging

WinSCP's logging system is adapted:
- `TSessionLog` - session activity log
- `TActionLog` - structured action log
- NetBox redirects to Far Manager's log or custom file

## Version Information

WinSCP source structure as of 2025-2026:
- PuTTY integration: recent upstream (2024-2025)
- FileZilla integration: older fork (pre-2020)
- Neon library: 0.32.x series
- libs3: custom fork

## Common Gotchas

1. **VCL dependencies** - Many classes require VCL runtime
2. **Unicode handling** - `UnicodeString` is UTF-16, not UTF-8
3. **Borland-specific syntax** - `__fastcall`, `__closure`, `__property`
4. **Exception handling** - VCL exceptions, not standard C++
5. **Threading** - VCL is not thread-safe; careful synchronization needed
6. **Memory management** - Mix of VCL ownership and manual `delete`

## Key Differences from NetBox

|Aspect|WinSCP|NetBox|
|---|---|---|
|UI Framework|VCL (Windows GUI)|Far Manager plugin API|
|Build System|C++ Builder|CMake + MSVC|
|String Type|`UnicodeString` (VCL)|`UnicodeString` (custom)|
|Configuration|Windows Registry|Far Manager settings|
|Logging|VCL file streams|tinylog / Far Manager|
|Threading|VCL threads|Win32 threads|
|Entry Point|`WinMain`|`GetGlobalInfoW` (Far plugin)|

## References

- WinSCP source: https://github.com/winscp/winscp
- PuTTY source: https://www.chiark.greenend.org.uk/~sgtatham/putty/
- FileZilla source: https://filezilla-project.org/
- Neon library: https://notroj.github.io/neon/

## Usage in NetBox Development

When working on NetBox protocol code:

1. **Understand the WinSCP equivalent** - Find the corresponding WinSCP file in `source/core/`
2. **Check for upstream changes** - Compare with latest WinSCP to identify bug fixes
3. **Preserve API compatibility** - NetBox's protocol layer mirrors WinSCP's interface
4. **Adapt VCL dependencies** - Replace VCL types with NetBox equivalents
5. **Test protocol behavior** - WinSCP's behavior is the reference implementation

## Common Tasks

### Adding a new protocol feature

1. Locate the protocol implementation (`*FileSystem.cpp`)
2. Check if WinSCP already supports it
3. If yes, port the WinSCP implementation
4. If no, implement following WinSCP patterns
5. Update `TCustomFileSystem` interface if needed

### Fixing a protocol bug

1. Check if WinSCP has the same bug
2. If fixed in WinSCP, port the fix
3. If not, implement fix and consider contributing upstream
4. Add test case to prevent regression

### Updating from WinSCP upstream

1. Identify changed files in WinSCP
2. Review changes for relevance to NetBox
3. Port changes to NetBox equivalents
4. Test affected protocols
5. Update this reference document
