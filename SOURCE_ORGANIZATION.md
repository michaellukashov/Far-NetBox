# Source Code Organization

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

## Module Details

### src/base/ - Foundation Layer

**Purpose**: Core foundation classes, string handling, exceptions, and system abstractions.

**Key Files**:
| File | Purpose |
|------|---------|
| `Classes.hpp` | Base class definitions and macros |
| `Common.h` | Common utilities and macros |
| `Exceptions.h/.cpp` | Exception hierarchy and handling |
| `Global.h/.cpp` | Global state and initialization |
| `UnicodeString.hpp/.cpp` | Unicode string implementation |
| `Sysutils.hpp/.cpp` | System utilities |
| `StrUtils.hpp/.cpp` | String manipulation utilities |
| `FormatUtils.h/.cpp` | String formatting utilities |
| `System.IOUtils.hpp/.cpp` | File I/O utilities |
| `System.SyncObjs.hpp/.cpp` | Synchronization primitives |
| `LibraryLoader.hpp/.cpp` | Dynamic library loading |
| `FileBuffer.h/.cpp` | File buffering utilities |
| `Masks.hpp/.cpp` | File mask matching |
| `XMLDoc.hpp/.cpp` | XML document handling |
| `WideStrUtils.hpp/.cpp` | Wide string utilities |
| `Function2.hpp` | std::function alternative |
| `Property.hpp` | Property implementation |

**Naming Conventions**:
- Classes: PascalCase (e.g., `UnicodeString`, `Exception`)
- Functions: PascalCase (e.g., `Format`, `ToInt`)
- Member variables: `F` prefix (e.g., `FData`, `FLength`)
- Constants: UPPER_CASE (e.g., `MAX_PATH`, `DEFAULT_TIMEOUT`)

### src/core/ - Protocol Implementations

**Purpose**: Protocol-specific filesystem implementations and session management.

**File Organization by Protocol**:

**SFTP (SSH File Transfer Protocol)**:
- `SftpFileSystem.h/.cpp` - SFTP protocol implementation
- Dependencies: PuTTY SSH library

**FTP (File Transfer Protocol)**:
- `FtpFileSystem.h/.cpp` - FTP protocol implementation
- Dependencies: FileZilla engine

**SCP (Secure Copy Protocol)**:
- `ScpFileSystem.h/.cpp` - SCP protocol implementation
- Dependencies: PuTTY SSH library

**WebDAV (Web Distributed Authoring and Versioning)**:
- `WebDAVFileSystem.h/.cpp` - WebDAV protocol implementation
- `NeonIntf.h/.cpp` - neon library interface
- Dependencies: neon library

**S3 (Amazon Simple Storage Service)**:
- `S3FileSystem.h/.cpp` - S3 protocol implementation
- Dependencies: libs3 library

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
| `SecureShell.h/.cpp` | SSH session management |
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
| `PuttyTools.h` | PuTTY utilities |

**Key Classes**:
- `TCustomFileSystem` - Base interface for all protocols
- `TSFTPFileSystem` - SFTP implementation
- `TFTPFileSystem` - FTP implementation
- `TSCPFileSystem` - SCP implementation
- `TWebDAVFileSystem` - WebDAV implementation
- `TS3FileSystem` - S3 implementation
- `TTerminal` - Terminal coordinator
- `TSessionData` - Session configuration
- `TSecureShell` - SSH session handler

### src/filezilla/ - FTP Engine

**Purpose**: FileZilla-based FTP implementation.

**Key Components**:
- FTP protocol state machine
- FTP command implementation
- Data connection handling
- FTPS (FTP over SSL/TLS) support

### src/NetBox/ - Plugin Implementation

**Purpose**: Far Manager plugin interface and integration.

**Key Files**:
| File | Purpose |
|------|---------|
| `NetBox.cpp` | Plugin entry point and initialization |
| `FarPlugin.h/.cpp` | Far Manager plugin wrapper |
| `FarInterface.h/.cpp` | Far API abstraction |
| `FarUtils.h/.cpp` | Far utility functions |
| `FarDialog.h/.cpp` | Dialog implementation |
| `FarConfiguration.h/.cpp` | Configuration storage for Far |
| `Far3Storage.h/.cpp` | Storage backend for Far 3.x |
| `MessageDlg.cpp` | Message dialog implementation |
| `WinSCPPlugin.h/.cpp` | WinSCP plugin compatibility |
| `WinSCPFileSystem.h/.cpp` | WinSCP filesystem adapter |
| `WinSCPDialogs.cpp` | WinSCP dialog compatibility |
| `XmlStorage.h/.cpp` | XML-based configuration storage |
| `FarPluginStrings.h/.cpp` | String resources |
| `resource.h` | Resource identifiers |
| `guid.h` | Plugin GUIDs |
| `plugin_version.hpp` | Version information |

**Plugin Lifecycle**:
1. `SetStartupInfo()` - Plugin initialization
2. `GetPluginInfo()` - Plugin information retrieval
3. `Open()` - Plugin activation
4. `Close()` - Plugin cleanup

### src/PluginSDK/ - Far Manager SDK

**Purpose**: Far Manager 3.x plugin development headers.

**Contents**:
- Far API function declarations
- Plugin structures and types
- Plugin constants and macros
- Plugin interface definitions

### src/include/ - Public Headers

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

### src/nbcore/ - Core Utilities

**Purpose**: String, memory, and utility functions.

**Files**:
- `nbstring.cpp` - String utilities implementation
- `nbmemory.cpp` - Memory management utilities
- `nbutils.cpp` - General utilities
- `stdafx.h` - Precompiled header

### src/resource/ - Resources

**Purpose**: Icons, bitmaps, version info, and licenses.

**Contents**:
- Application icons
- Toolbar bitmaps
- Dialog images
- License files
- Version resources

### src/windows/ - Windows Implementations

**Purpose**: Windows-specific functionality.

**Contents**:
- Windows API wrappers
- COM integration
- Shell integration
- Registry access
- Windows-specific utilities

## File Naming Conventions

### Extensions
- `.h` - C++ header files (preferred)
- `.hpp` - C++ header files (alternative)
- `.cpp` - C++ source files
- `.rc` - Resource files
- `.lng` - Language files

### Naming Patterns
- **Headers**: Match source name (e.g., `Foo.h` for `Foo.cpp`)
- **Classes**: PascalCase with `T` prefix (e.g., `TSessionData`)
- **Functions**: PascalCase (e.g., `GetSessionData`)
- **Globals**: PascalCase or UPPER_CASE for constants

## Include Order

Headers should be included in this order:
1. System headers (`<...>`)
2. Project headers (`"..."`)
3. Local headers (`"..."`)

## Code Organization Principles

1. **Layered Architecture**: base → core → plugin
2. **Protocol Isolation**: Each protocol in separate files
3. **Interface Segregation**: Clear separation between layers
4. **Dependency Direction**: Dependencies point downward (plugin → core → base)
5. **No Circular Dependencies**: Strict layering prevents cycles

## Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| `base/` | Foundation classes, string handling, exceptions |
| `core/` | Protocol implementations, session management |
| `filezilla/` | FTP engine integration |
| `NetBox/` | Far Manager plugin interface |
| `PluginSDK/` | Far API headers |
| `include/` | Public type definitions |
| `nbcore/` | Core utilities |
| `resource/` | Binary resources |
| `windows/` | Windows-specific code |

## Key Interfaces

### TCustomFileSystem Interface

All protocol implementations must implement:

```cpp
class TCustomFileSystem
{
public:
    virtual void Connect() = 0;
    virtual void Disconnect() = 0;
    virtual bool Connected() = 0;
    virtual void CreateDirectory(const UnicodeString& path) = 0;
    virtual void DeleteFile(const UnicodeString& path) = 0;
    virtual void RenameFile(const UnicodeString& oldPath, const UnicodeString& newPath) = 0;
    virtual void CopyFile(const UnicodeString& sourcePath, const UnicodeString& targetPath) = 0;
    virtual void MoveFile(const UnicodeString& sourcePath, const UnicodeString& targetPath) = 0;
    // ... more file operations
};
```

### Plugin Interface

```cpp
// Entry point
void WINAPI SetStartupInfo(const struct PluginStartupInfo* info);

// Plugin information
const struct PluginInfo* WINAPI GetPluginInfo(int info);

// Open plugin panel
HANDLE WINAPI Open(const struct OpenInfo* info);

// Cleanup
void WINAPI ClosePanel(HHANDLE hPanel);
```

## Build Artifacts

### Object Files
- Organized by source directory
- Intermediate files in build directory

### Output Files
- `NetBox.dll` - Plugin DLL
- `NetBox.pdb` - Debug symbols
- Language DLLs for localization

### Build Outputs by Platform
- `Far3_x86/Plugins/NetBox/` - 32-bit build
- `Far3_x64/Plugins/NetBox/` - 64-bit build
- `Far3_ARM64/Plugins/NetBox/` - ARM64 build
