# Third-Party Dependencies

## Overview

NetBox integrates multiple third-party libraries to provide protocol support and core functionality. All dependencies are located in the `libs/` directory.

## Dependency List

### Core Protocol Libraries

#### 1. PuTTY
- **Location**: `libs/putty/`
- **Purpose**: SSH and SCP protocol implementation
- **Version**: 0.81 (based on README)
- **License**: MIT License (Simon Tatham)
- **Components Used**:
  - SSH protocol implementation
  - SCP protocol support
  - SFTP protocol support
  - SSH authentication methods
  - Cryptographic primitives
- **Integration**: `src/core/PuttyIntf.h/.cpp`
- **Build Config**: `cmake/PuTTY.cmake`
- **Notes**:
  - Heavily modified for integration with NetBox
  - SSH backend for SFTP and SCP protocols
  - Provides terminal emulation support

#### 2. FileZilla
- **Location**: `src/filezilla/` (integrated in source)
- **Purpose**: FTP protocol implementation
- **Version**: 2.2.32 (based on README)
- **License**: GPL
- **Components Used**:
  - FTP protocol state machine
  - FTPS (FTP over SSL/TLS)
  - Active and passive mode support
  - Proxy support
- **Integration**: Integrated into core
- **Build Config**: Part of core build
- **Notes**:
  - Modified from original FileZilla codebase
  - FTP backend for protocol implementation

#### 3. neon
- **Location**: `libs/neon/`
- **Purpose**: WebDAV and HTTP protocol implementation
- **Version**: Latest compatible
- **License**: LGPL/GPL dual license
- **Components Used**:
  - WebDAV protocol support
  - HTTP/HTTPS client
  - SSL/TLS support via OpenSSL
  - Authentication (Basic, Digest, NTLM)
- **Integration**: `src/core/NeonIntf.h/.cpp`
- **Build Config**: `cmake/Neon.cmake`
- **Notes**:
  - WebDAV backend
  - Requires OpenSSL for HTTPS

#### 4. libs3
- **Location**: `libs/libs3/`
- **Purpose**: Amazon S3 protocol implementation
- **Version**: Compatible fork
- **License**: MIT License
- **Components Used**:
  - S3 REST API implementation
  - Bucket operations
  - Object operations (upload, download, delete)
  - Multi-part upload support
- **Integration**: `src/core/S3FileSystem.h/.cpp`
- **Build Config**: `cmake/Libs3.cmake`
- **Notes**:
  - S3 storage backend
  - Compatible with AWS S3 and compatible services

### Cryptography

#### 5. OpenSSL 3
- **Location**: `libs/openssl-3/`
- **Purpose**: Cryptography and SSL/TLS support
- **Version**: 3.x (synced with WinSCP 6.5.6, April 2026)
- **License**: Apache License 2.0
- **Components Used**:
  - SSL/TLS implementation
  - Cryptographic primitives
  - Certificate handling
  - Random number generation
- **Integration**: Linked to all protocol implementations
- **Build Config**: `cmake/OpenSSL.cmake`
- **Notes**:
  - Critical security component
  - Used by neon, PuTTY, and direct TLS connections
  - Regular updates required for security patches
  - Sourced from WinSCP project (libs/openssl) with CMake integration
  - Excludes QUIC support, FIPS provider, and legacy ciphers not used by WinSCP

### XML Processing

#### 6. tinyxml2
- **Location**: `libs/tinyxml2/`
- **Purpose**: XML parsing and generation
- **Version**: Latest compatible
- **License**: zlib License
- **Components Used**:
  - XML document parsing
  - XML generation
  - DOM-like API
- **Integration**: Configuration storage, session data
- **Build Config**: `cmake/TinyXML2.cmake`
- **Notes**:
  - Lightweight XML parser
  - Used for configuration storage

#### 7. expat
- **Location**: `libs/expat/`
- **Purpose**: XML parsing (alternative)
- **Version**: Latest compatible
- **License**: MIT License
- **Components Used**:
  - Fast XML parsing
  - Stream-based parsing
- **Integration**: Alternative XML parser
- **Build Config**: `cmake/Expat.cmake`
- **Notes**:
  - Used where streaming XML parsing is needed

### String Formatting

#### 8. fmt
- **Location**: `libs/fmt/`
- **Purpose**: String formatting library
- **Version**: Latest compatible
- **License**: MIT License
- **Components Used**:
  - Format string processing
  - Type-safe formatting
  - Performance improvements over printf
- **Integration**: Throughout codebase
- **Build Config**: `cmake/FMT.cmake`
- **Notes**:
  - Modern C++ formatting
  - Replacement for printf/sprintf

### Logging

#### 9. tinylog
- **Location**: `libs/tinylog/`
- **Purpose**: Logging framework
- **Version**: Custom/minimal version
- **License**: Public domain / MIT
- **Components Used**:
  - Debug logging
  - Log levels
  - Log output management
- **Integration**: `src/nbcore/` logging utilities
- **Build Config**: `cmake/TinyLog.cmake`
- **Notes**:
  - Minimal logging implementation
  - Debug output via `FTerminal->LogEvent()`

### Memory Management

#### 10. dlmalloc
- **Location**: `libs/dlmalloc/`
- **Purpose**: Memory allocator
- **Version**: Doug Lea's malloc
- **License**: Public domain
- **Components Used**:
  - Dynamic memory allocation
  - Memory pool management
- **Integration**: Custom allocator support
- **Build Config**: `cmake/DLMalloc.cmake`
- **Notes**:
  - Alternative memory allocator
  - Performance optimization

### Windows Compatibility

#### 11. ATL/MFC Subset
- **Location**: `libs/atlmfc/`
- **Purpose**: Minimal ATL/MFC compatibility layer
- **Version**: Custom subset
- **License**: Microsoft (as part of Windows SDK)
- **Components Used**:
  - Windows API wrappers
  - COM utilities
  - String handling
- **Integration**: Windows-specific code
- **Build Config**: `cmake/ATLMFC.cmake`
- **Notes**:
  - Minimal subset for compatibility
  - Avoids full MFC dependency

### Utilities

#### 12. GSL (Guidelines Support Library)
- **Location**: `libs/GSL/`
- **Purpose**: C++ Core Guidelines support
- **Version**: Compatible version
- **License**: MIT License
- **Components Used**:
  - `gsl::span` for array views
  - `gsl::not_null` for non-null pointers
  - Bounds checking utilities
- **Integration**: Throughout codebase
- **Build Config**: Included in main build
- **Notes**:
  - Modern C++ best practices
  - Safety utilities

#### 13. icecream-cpp
- **Location**: `libs/icecream-cpp/`
- **Purpose**: Debugging aid
- **Version**: Compatible version
- **License**: MIT License
- **Components Used**:
  - Debug printing
  - Variable inspection
- **Integration**: Debug builds
- **Build Config**: Debug configuration
- **Notes**:
  - Development debugging tool
  - Not used in release builds

#### 14. zlib-ng
- **Location**: `libs/zlib-ng/`
- **Purpose**: Compression library
- **Version**: Next-generation zlib
- **License**: zlib License
- **Components Used**:
  - Deflate compression
  - Inflate decompression
  - GZip format support
- **Integration**: Compression support
- **Build Config**: `cmake/zlib-ng.cmake`
- **Notes**:
  - zlib-compatible API
  - Performance improvements over zlib

## Dependency Graph

```
NetBox Plugin
│
├── Protocol Layer
│   ├── PuTTY (SSH, SCP, SFTP)
│   │   └── OpenSSL 3 (cryptography)
│   ├── FileZilla (FTP)
│   │   └── OpenSSL 3 (FTPS)
│   ├── neon (WebDAV)
│   │   └── OpenSSL 3 (HTTPS)
│   │   └── zlib-ng (compression)
│   └── libs3 (S3)
│       └── OpenSSL 3 (HTTPS)
│
├── Core Utilities
│   ├── tinyxml2 (XML config)
│   ├── fmt (string formatting)
│   ├── tinylog (logging)
│   └── GSL (safety utilities)
│
├── Platform Support
│   ├── ATL/MFC subset (Windows API)
│   └── dlmalloc (memory allocation)
│
└── Development
    └── icecream-cpp (debugging)
```

## Build Configuration

### Library Configuration Files

Each library has a corresponding CMake configuration:

| Library | CMake Config | Purpose |
|---------|--------------|---------|
| PuTTY | `cmake/PuTTY.cmake` | SSH/SCP backend |
| neon | `cmake/Neon.cmake` | WebDAV backend |
| libs3 | `cmake/Libs3.cmake` | S3 backend |
| OpenSSL 3 | `cmake/OpenSSL.cmake` | Cryptography |
| tinyxml2 | `cmake/TinyXML2.cmake` | XML parsing |
| expat | `cmake/Expat.cmake` | XML parsing (alt) |
| FMT | `cmake/FMT.cmake` | String formatting |
| TinyLog | `cmake/TinyLog.cmake` | Logging |
| DLMalloc | `cmake/DLMalloc.cmake` | Memory allocator |
| zlib-ng | `cmake/zlib-ng.cmake` | Compression |
| ATLMFC | `cmake/ATLMFC.cmake` | Windows compatibility |

### Library Integration Pattern

```cmake
# Example: Library configuration pattern
add_subdirectory(libs/library-name)
target_link_libraries(NetBox PRIVATE library-name)
```

## Version Management

### Pinned Versions
- PuTTY: 0.81 (pinned in README)
- FileZilla: 2.2.32 (pinned in README)
- OpenSSL: 3.x (major version pinned)

### Version Updates
- Security updates: Immediate
- Feature updates: Tested against test suite
- Major version changes: Require compatibility testing

## License Compliance

### GPL Components
- FileZilla code (GPL)
- WinSCP derivative code (GPL)

### MIT/BSD Components
- PuTTY (MIT)
- tinyxml2 (zlib/BSD)
- fmt (MIT)
- GSL (MIT)

### Apache 2.0
- OpenSSL 3 (Apache 2.0)

### Attribution Requirements
- License files included in `src/resource/`
- About dialog lists all third-party components
- README.md acknowledges source codebases

## Security Considerations

### Critical Dependencies
1. **OpenSSL 3**: Must be kept current for security patches
2. **PuTTY**: SSH implementation requires security updates
3. **neon**: HTTPS support requires OpenSSL currency

### Update Strategy
- Monitor upstream security advisories
- Test security patches immediately
- Maintain compatibility with OpenSSL 3 API
- Regular dependency audits

## Build Dependencies

### Required Tools
- CMake 3.15+
- Visual Studio 2022 (MSVC)
- Ninja (recommended)

### Optional Tools
- Clang-Tidy (static analysis)
- Visual Studio Code Analysis

### Build-Time Dependencies
- Git (for version control)
- Windows SDK (for Windows APIs)

## Platform-Specific Notes

### Windows (x86/x64)
- Full dependency support
- ATL/MFC subset for compatibility

### Windows ARM64
- All dependencies must be compiled for ARM64
- OpenSSL ARM64 optimizations available

### Cross-Platform Considerations
- Currently Windows-only
- Dependencies chosen for Windows compatibility
- Some libraries (PuTTY, neon) are cross-platform

## Dependency Health

### Active Maintenance
| Library | Status | Notes |
|---------|--------|-------|
| PuTTY | Active | Regular security updates |
| OpenSSL 3 | Active | Long-term support |
| neon | Active | Maintained upstream |
| libs3 | Moderate | Community maintained |
| tinyxml2 | Active | Stable, minimal updates needed |
| fmt | Active | Rapid development |
| zlib-ng | Active | Performance improvements |

### Risk Assessment
- **Low Risk**: tinyxml2, fmt, GSL (stable, minimal)
- **Medium Risk**: neon, libs3 (smaller community)
- **High Attention**: OpenSSL, PuTTY (security-critical)

## Integration Points

### Source Code Integration
- Libraries integrated via CMake subdirectories
- Headers included via include paths
- Linking via target dependencies

### Configuration
- Library configurations in `cmake/` directory
- Compiler flags set per-library
- Warning levels adjusted for third-party code

### Patches and Modifications
- Third-party code should not be modified directly
- Use wrapper layers for integration
- Patches documented in commit history

## Future Considerations

### Potential Additions
- HTTP/2 support (via nghttp2 or similar)
- Modern compression algorithms (zstd, lz4)
- Enhanced logging (spdlog alternative)

### Deprecation Candidates
- icecream-cpp (development only)
- dlmalloc (if system allocator sufficient)
- expat (if tinyxml2 sufficient)

### Modernization Path
- C++17 features adoption
- std::filesystem instead of custom paths
- std::optional and std::variant usage
