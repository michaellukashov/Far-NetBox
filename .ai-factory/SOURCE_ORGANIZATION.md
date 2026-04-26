# Source Code Organization

**Date:** 2026-04-26
**Updated:** 2026-04-26 (verified against codebase)
**Architecture:** [ARCHITECTURE.md](./ARCHITECTURE.md)

## Directory Structure

```
src/
├── base/              # Foundation classes and utilities
├── core/              # Core protocol implementations
├── filezilla/         # FileZilla-based FTP implementation
├── include/           # Public headers and type definitions
├── nbcore/            # NetBox core utilities
├── NetBox/            # Far Manager plugin implementation
├── PluginSDK/         # Far Manager plugin SDK headers
├── resource/          # Resources (icons, bitmaps, licenses)
└── windows/           # Windows-specific implementations
```

## Architecture Layers

```
Plugin Layer (src/NetBox/)
    ↓
Core Layer (src/core/)
    ↓
Base Layer (src/base/, src/nbcore/)
    ↓
Third-Party (libs/)
```

**Dependency Rules:**
- ✅ Plugin Layer → Core Layer (plugin calls protocol implementations)
- ✅ Core Layer → Base Layer (protocols use foundation classes)
- ✅ Core Layer → Third-Party (protocols wrap library implementations)
- ❌ Plugin Layer → Third-Party (must go through core)
- ❌ Base Layer → Core Layer (foundation must be independent)

## Module Details

### src/base/ — Foundation Layer

**Purpose**: Core foundation classes, string handling, exceptions, and system abstractions.

**Key Files**:

| File | Purpose |
|------|---------|
| `Classes.h` | Base class definitions and macros |
| `Common.h` | Common utilities and macros |
| `Exceptions.h/.cpp` | Exception hierarchy and handling |
| `Global.h/.cpp` | Global state and initialization |
| `UnicodeString.h/.cpp` | Unicode string implementation |
| `Sysutils.h/.cpp` | System utilities |
| `StrUtils.h/.cpp` | String manipulation utilities |
| `FormatUtils.h/.cpp` | String formatting utilities |
| `System.IOUtils.h/.cpp` | File I/O utilities |
| `System.SyncObjs.h/.cpp` | Synchronization primitives |
| `LibraryLoader.h/.cpp` | Dynamic library loading |
| `FileBuffer.h/.cpp` | File buffering utilities |
| `Masks.h/.cpp` | File mask matching |
| `XMLDoc.h/.cpp` | XML document handling |
| `WideStrUtils.h/.cpp` | Wide string utilities |
| `Function2.hpp` | std::function alternative |
| `Property.hpp` | Property implementation |

**Naming Conventions**:
- Classes: PascalCase (e.g., `UnicodeString`, `Exception`)
- Functions: PascalCase (e.g., `Format`, `ToInt`)
- Member variables: `F` prefix (e.g., `FData`, `FLength`)
- Constants: UPPER_CASE (e.g., `MAX_PATH`, `DEFAULT_TIMEOUT`)

### src/core/ — Protocol Implementations

**Purpose**: Protocol-specific filesystem implementations and session management.

**File Organization by Protocol**:

**SFTP (SSH File Transfer Protocol)**:
- `SftpFileSystem.h/.cpp` — SFTP protocol implementation
- `SecureShell.h/.cpp` — SSH session management (PuTTY integration)
- Dependencies: PuTTY SSH library (`libs/putty/`)

**FTP (File Transfer Protocol)**:
- `FtpFileSystem.h/.cpp` — FTP protocol implementation
- Dependencies: FileZilla engine (`src/filezilla/`)

**SCP (Secure Copy Protocol)**:
- `ScpFileSystem.h/.cpp` — SCP protocol implementation
- Dependencies: PuTTY SSH library (`libs/putty/`)

**WebDAV (Web Distributed Authoring and Versioning)**:
- `WebDAVFileSystem.h/.cpp` — WebDAV protocol implementation
- `NeonIntf.h/.cpp` — neon library interface
- Dependencies: neon library (`libs/neon/`)

**S3 (Amazon Simple Storage Service)**:
- `S3FileSystem.h/.cpp` — S3 protocol implementation
- Dependencies: libs3 library (`libs/libs3/`)

**Common Components**:

| File | Purpose |
|------|---------|
| `Terminal.h/.cpp` | Terminal emulation and session coordination |
| `SessionData.h/.cpp` | Session configuration container |
| `SessionInfo.h/.cpp` | Session metadata and status |
| `Configuration.h/.cpp` | Configuration management |
| `CopyParam.h/.cpp` | Copy/move operation parameters |
| `RemoteFiles.h/.cpp` | Remote file representation |
| `FileSystems.h/.cpp` | Filesystem abstraction |
| `Cryptography.h/.cpp` | Cryptographic functions |
| `Http.h/.cpp` | HTTP client implementation |
| `Queue.h/.cpp` | Operation queue management |
| `FileOperationProgress.h/.cpp` | Progress tracking |
| `FileMasks.h/.cpp` | File pattern matching |
| `FileInfo.h/.cpp` | File information container |
| `Bookmarks.h/.cpp` | Bookmark management |
| `Option.h/.cpp` | Configuration options |
| `Script.h/.cpp` | Scripting support |
| `NamedObjs.h/.cpp` | Named object registry |
| `Usage.h/.cpp` | Usage tracking |
| `WinSCPSecurity.h/.cpp` | Security utilities |
| `PuttyIntf.h/.cpp` | PuTTY interface |
| `PuttyTools.h/.cpp` | PuTTY utilities |
| `KittyKeyboard.h/.cpp` | Kitty keyboard protocol support |

**Key Classes**:
- `TCustomFileSystem` — Base interface for all protocols
- `TSFTPFileSystem` — SFTP implementation
- `TFTPFileSystem` — FTP implementation
- `TSCPFileSystem` — SCP implementation
- `TWebDAVFileSystem` — WebDAV implementation
- `TS3FileSystem` — S3 implementation
- `TTerminal` — Terminal coordinator
- `TSessionData` — Session configuration
- `TSecureShell` — SSH session handler (wraps PuTTY)

### src/filezilla/ — FTP Engine

**Purpose**: FileZilla-based FTP implementation.

**Key Components**:
- FTP protocol state machine
- FTP command implementation
- Data connection handling
- FTPS (FTP over SSL/TLS) support

### src/NetBox/ — Plugin Implementation

**Purpose**: Far Manager plugin interface and integration.

**Key Files**:

| File | Purpose |
|------|---------|
| `plugin.cpp` | Plugin entry point (`GetPluginInfoW`, `OpenPluginW`, `ClosePluginW`) |
| `FarPlugin.h/.cpp` | Far Manager plugin wrapper |
| `FarInterface.h/.cpp` | Far API abstraction |
| `FarUtils.h/.cpp` | Far utility functions |
| `FarDialog.h/.cpp` | Dialog implementation (`TFarDialog`, `TTabbedDialog`) |
| `FarConfiguration.h/.cpp` | Configuration storage for Far |
| `Far3Storage.h/.cpp` | Storage backend for Far 3.x |
| `MessageDlg.cpp` | Message dialog implementation |
| `WinSCPPlugin.h/.cpp` | WinSCP plugin compatibility |
| `WinSCPFileSystem.h/.cpp` | WinSCP filesystem adapter |
| `WinSCPDialogs.cpp` | WinSCP dialog compatibility (`TSessionDialog`) |
| `XmlStorage.h/.cpp` | XML-based configuration storage |
| `FarPluginStrings.h/.cpp` | String resources |
| `resource.h` | Resource identifiers |
| `guid.h` | Plugin GUIDs |
| `plugin_version.hpp` | Version information |

**Plugin Lifecycle**:
1. `GetPluginInfoW()` — Plugin information retrieval
2. `OpenPluginW()` — Plugin activation
3. `ClosePluginW()` — Plugin cleanup

### src/PluginSDK/ — Far Manager SDK

**Purpose**: Far Manager 3.x plugin development headers.

**Contents**:
- Far API function declarations
- Plugin structures and types
- Plugin constants and macros
- Plugin interface definitions

### src/include/ — Public Headers

**Purpose**: Type definitions and public interfaces.

**Key Files**:

| File | Purpose |
|------|---------|
| `nbtypes.h` | Core type definitions |
| `nbstring.h` | String type aliases |
| `nbsystem.h` | System type aliases |
| `nbcore.h` | Core function declarations |
| `nbglobals.h` | Global variable declarations |
| `rtti.hpp` | RTTI utilities |
| `type_traits.h` | Type traits utilities |
| `FastDelegate.h` | Fast delegate implementation |
| `FastDelegateBind.h` | Delegate binding utilities |
| `disable_warnings_in_std_begin.hpp` | Warning suppression |
| `disable_warnings_in_std_end.hpp` | Warning restoration |
| `disabled_warnings.hpp` | Warning configuration |

### src/nbcore/ — Core Utilities

**Purpose**: String, memory, and utility functions.

**Files**:
- `nbstring.cpp` — String utilities implementation
- `nbmemory.cpp` — Memory management utilities
- `nbutils.cpp` — General utilities
- `stdafx.h` — Precompiled header
- `logging.cpp` — Logging integration (tinylog)

### src/resource/ — Resources

**Purpose**: Icons, bitmaps, version info, and licenses.

**Contents**:
- Application icons
- Toolbar bitmaps
- Dialog images
- License files
- Version resources
- Language files (`.lng`)

### src/windows/ — Windows Implementations

**Purpose**: Windows-specific functionality.

**Contents**:
- Windows API wrappers
- COM integration
- Shell integration
- Registry access
- Windows-specific utilities
- GUI dialogs and controls

## File Naming Conventions

### Extensions

| Extension | Purpose |
|-----------|---------|
| `.h` | C++ header files (preferred) |
| `.hpp` | C++ header files (alternative) |
| `.cpp` | C++ source files |
| `.rc` | Resource files |
| `.lng` | Language files |

### Naming Patterns

- **Headers**: Match source name (e.g., `Foo.h` for `Foo.cpp`)
- **Classes**: PascalCase with `T` prefix (e.g., `TSessionData`)
- **Functions**: PascalCase (e.g., `GetSessionData`)
- **Member variables**: `F` prefix (e.g., `FSessionData`)
- **Globals**: PascalCase or UPPER_CASE for constants

## Include Order

Headers should be included in this order:
1. System headers (`<...>`)
2. Project headers (`"..."`)
3. Local headers (`"..."`)

## Code Organization Principles

1. **Layered Architecture**: Plugin → Core → Base → Third-Party
2. **Protocol Isolation**: Each protocol in separate files
3. **Interface Segregation**: Clear separation between layers
4. **Dependency Direction**: Dependencies point downward (plugin → core → base)
5. **No Circular Dependencies**: Strict layering prevents cycles
6. **TCustomFileSystem Interface**: All protocols implement this base interface
7. **Factory Pattern**: `CreateFileSystem()` creates protocol-specific instances

## Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| `base/` | Foundation classes, string handling, exceptions |
| `core/` | Protocol implementations, session management |
| `filezilla/` | FTP engine integration |
| `NetBox/` | Far Manager plugin interface |
| `PluginSDK/` | Far API headers |
| `include/` | Public type definitions |
| `nbcore/` | Core utilities, logging |
| `resource/` | Binary resources, language files |
| `windows/` | Windows-specific code, GUI dialogs |

## Key Interfaces

### TCustomFileSystem Interface

All protocol implementations must implement:

```cpp
class TCustomFileSystem
{
public:
    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    virtual void FindFirst(const UnicodeString & Path, TFindFileList & List) = 0;
    virtual bool GetFile(const UnicodeString & FileName,
      const UnicodeString & LocalFile, ...) = 0;
    virtual bool PutFile(const UnicodeString & LocalFile,
      const UnicodeString & FileName, ...) = 0;
    virtual bool DeleteFile(const UnicodeString & FileName) = 0;
    virtual bool RenameFile(const UnicodeString & OldName,
      const UnicodeString & NewName) = 0;
    virtual bool MakeDirectory(const UnicodeString & DirName) = 0;
    // ... more file operations
};
```

### Protocol Factory

```cpp
TCustomFileSystem *CreateFileSystem(TSessionData * Data)
{
  switch (Data->FSProtocol)
  {
    case fsSFTP: return new TSecureShell(Data);
    case fsFTP: return new TFTPFileSystem(Data);
    case fsWebDAV: return new TWebDAVFileSystem(Data);
    case fsS3: return new TS3FileSystem(Data);
    default: DebugAlwaysAssert(false); return nullptr;
  }
}
```

### Plugin Interface

```cpp
// Required exports
void WINAPI GetPluginInfoW(PluginInfo *Info);
HANDLE WINAPI OpenPluginW(const OpenInfo *Info);
void WINAPI ClosePluginW(HANDLE Handle, int ExitCode);
```

## Build Artifacts

### Object Files
- Organized by source directory
- Intermediate files in build directory (`build-<config>/`)

### Output Files
- `NetBox.dll` — Plugin DLL
- `NetBox.pdb` — Debug symbols
- Language DLLs for localization

### Build Outputs by Platform
- `Far3_x86/Plugins/NetBox/` — 32-bit build
- `Far3_x64/Plugins/NetBox/` — 64-bit build
- `Far3_ARM64/Plugins/NetBox/` — ARM64 build

## Third-Party Libraries (libs/)

| Library | Location | Purpose |
|---------|----------|---------|
| PuTTY 0.81 | `libs/putty/` | SSH/SCP/SFTP protocol |
| FileZilla 2.2.32 | `src/filezilla/` | FTP/FTPS protocol |
| OpenSSL 3.3.7 | `libs/openssl-3/` | Cryptography/SSL/TLS |
| neon | `libs/neon/` | WebDAV/HTTP protocol |
| libs3 | `libs/libs3/` | Amazon S3 protocol |
| tinyxml2 | `libs/tinyxml2/` | XML parser |
| fmt | `libs/fmt/` | String formatting |
| tinylog | `libs/tinylog/` | Logging framework |
| GSL | `libs/GSL/` | Guidelines Support Library |
| zlib-ng | `libs/zlib-ng/` | Compression |
| expat | `libs/expat/` | XML parsing |

**Critical rule:** NEVER modify files in `libs/` directly. Use patches instead.

## Reference Files

- [Architecture](./ARCHITECTURE.md) — layered plugin architecture, dependency rules, protocol interface
- [SSH Authentication Exploration](./references/ssh-authentication-exploration.md) — SSH code paths, PuTTY cert support
- [Session Configuration UI Patterns](./references/session-config-ui-patterns.md) — dialog architecture, tab UI patterns
