# NetBox Architecture Overview

## Project Summary

**NetBox** is a Far Manager 3.0 plugin providing SFTP/FTP/SCP/WebDAV/S3 client capabilities. It's based on WinSCP v6.5.1 codebase with PuTTY (SSH/SCP) and FileZilla (FTP) components.

## Build System Architecture

### CMake Structure
```
CMakeLists.txt (main entry point)
├── cmake/NetBox.cmake (compiler/linker flags, global configuration)
├── cmake/Libraries.cmake (library configuration orchestrator)
├── cmake/Install.cmake (post-build installation steps)
├── cmake/PlatformDetection.cmake (platform auto-detection)
├── cmake/TargetConfiguration.cmake (target-specific settings)
├── cmake/SourceGroups.cmake (IDE organization)
├── cmake/[Library].cmake (individual library configs)
└── src/CMakeLists.txt (plugin target definition)
```

### Build Configuration
- **C++ Standard**: C++17 (strict, no extensions)
- **Compiler**: MSVC (Visual Studio 2022)
- **Platforms**: x86, x64, ARM64 (auto-detected)
- **Build Types**: Debug, Release, RelWithDebugInfo
- **Unity Builds**: Enabled for x86 Release (faster compilation)

### Key Build Options
- `OPT_CREATE_PLUGIN_DIR`: Create plugin directory structure
- `OPT_USE_UNITY_BUILD`: Enable unity builds
- `OPT_COMPILE_COMMANDS`: Generate compile_commands.json for IDE

## High-Level Component Architecture

```
Far Manager Plugin Interface
         │
         ▼
┌─────────────────────────────────────────┐
│  Plugin Layer (src/NetBox/)             │
│  - Far Manager API integration          │
│  - Plugin lifecycle management          │
│  - Dialog/UI handling                   │
│  - Configuration storage                │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Core Layer (src/core/)                 │
│  - Protocol implementations             │
│  - Session management                   │
│  - File operations                      │
│  - Terminal emulation                   │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Base Layer (src/base/)                 │
│  - Foundation classes                   │
│  - String handling (UnicodeString)      │
│  - Exception handling                   │
│  - System abstractions                  │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Third-Party Libraries (libs/)          │
│  - PuTTY (SSH/SCP)                      │
│  - FileZilla (FTP)                      │
│  - neon (WebDAV)                        │
│  - libs3 (S3)                           │
│  - OpenSSL (Cryptography)               │
│  - Supporting libraries                 │
└─────────────────────────────────────────┘
```

## Protocol Architecture

### Protocol Implementations
All protocols inherit from `TCustomFileSystem` interface:

1. **SFTP** (`TSFTPFileSystem`)
   - File: `SftpFileSystem.cpp/h`
   - Based on PuTTY SSH library
   - Full SFTP protocol support

2. **FTP** (`TFTPFileSystem`)
   - File: `FtpFileSystem.cpp/h`
   - Based on FileZilla engine
   - FTP/FTPS support

3. **SCP** (`TSCPFileSystem`)
   - File: `ScpFileSystem.cpp/h`
   - Based on PuTTY SSH library
   - SCP protocol over SSH

4. **WebDAV** (`TWebDAVFileSystem`)
   - File: `WebDAVFileSystem.cpp/h`
   - Based on neon library
   - WebDAV/HTTPS support

5. **S3** (`TS3FileSystem`)
   - File: `S3FileSystem.cpp/h`
   - Based on libs3 library
   - Amazon S3 compatible storage

### Common Abstractions

**TCustomFileSystem** (in `Terminal.h`)
- Base interface for all protocol implementations
- Defines common file operations
- Session lifecycle management
- Error handling interface

**TSessionData** (in `SessionData.h`)
- Configuration container for sessions
- Protocol-agnostic connection settings
- Authentication configuration
- Advanced protocol-specific options

**TTerminal** (in `Terminal.h`)
- Manages active sessions
- Coordinates file operations
- Handles terminal emulation
- Provides unified interface to plugin layer

## Memory Management Strategy

- **RAII Pattern**: Preferred for resource management
- **Smart Pointers**: Used where appropriate
- **Custom Allocators**: dlmalloc for specific use cases
- **String Handling**: `UnicodeString` for wide strings, `AnsiString` for narrow strings

## Error Handling Architecture

```
Exception Hierarchy (src/base/Exceptions.h):
Exception (base)
├── EAbort
├── EExternalException
├── EOutOfMemory
├── EInOutError
├── EConvertError
└── ExternalException
    └── Network-related exceptions
```

- **Debug Logging**: `ADF()` macro for debug output
- **Assertions**: `DebugAssert()` for invariants
- **Logging**: tinylog library integration

## Plugin Integration Points

### Far Manager API
- Plugin entry point: `NetBox.cpp`
- Plugin interface: `FarPlugin.cpp/h`
- Storage interface: `Far3Storage.cpp/h`
- Configuration: `FarConfiguration.cpp/h`

### Dialog System
- Custom dialog implementation: `FarDialog.cpp/h`
- Message dialogs: `MessageDlg.cpp`
- WinSCP-compatible dialogs: `WinSCPDialogs.cpp`

### Resource Management
- Resources: `src/resource/`
- Language files: `NetBoxEng.lng`, `NetBoxRus.lng`, etc.
- Icons and bitmaps in resource DLL

## Platform Support

### Windows API Usage
- Windows-specific code in `src/windows/`
- ATL/MFC subset for Windows APIs
- COM/Shell integration where needed

### Platform Detection
- Auto-detection via `PlatformDetection.cmake`
- Separate output directories per platform
- Platform-specific optimizations

## Key Design Patterns

1. **Factory Pattern**: Protocol-specific filesystem creation
2. **Adapter Pattern**: Far Manager API adaptation to WinSCP core
3. **Strategy Pattern**: Protocol selection and execution
4. **Observer Pattern**: Event handling for UI updates
5. **RAII**: Resource management throughout

## Dependencies Graph

```
NetBox Plugin
├── Core (protocol implementations)
│   ├── PuTTY (SSH/SCP backend)
│   ├── FileZilla (FTP backend)
│   ├── neon (WebDAV backend)
│   └── libs3 (S3 backend)
├── Base (foundation classes)
│   └── System libraries
├── OpenSSL 3 (cryptography)
├── tinyxml2 (XML parsing)
├── fmt (string formatting)
├── tinylog (logging)
├── GSL (C++ Guidelines Support)
└── dlmalloc (memory allocation)
```

## Configuration Storage

- **Format**: XML-based storage
- **Location**: Far Manager profile or registry
- **Implementation**: `XmlStorage.cpp/h`, `HierarchicalStorage.cpp/h`
- **Sessions**: Stored with protocol-specific extensions

## Security Considerations

- **OpenSSL 3**: Modern cryptography backend
- **PuTTY Security**: Regular updates from upstream
- **Credential Storage**: Encrypted storage for passwords
- **Certificate Management**: Built-in certificate handling

## Performance Characteristics

- **Unity Builds**: Faster compilation for release builds
- **Caching**: Directory listing and file info caching
- **Async Operations**: Queue-based file operations
- **Memory Efficiency**: Custom allocators for specific use cases

## Extension Points

1. **Protocol Handlers**: Add new protocols via `TCustomFileSystem`
2. **Storage Backends**: Custom configuration storage
3. **Authentication**: Custom authentication methods
4. **Terminal Emulation**: Enhanced terminal support
5. **File Operations**: Custom copy/transfer operations
