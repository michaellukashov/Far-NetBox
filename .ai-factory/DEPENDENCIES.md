# Third-Party Dependencies

**Date:** 2026-04-26
**Updated:** 2026-04-26 (verified against codebase and AGENTS.md)
**Architecture:** [ARCHITECTURE.md](./ARCHITECTURE.md)
**Source Organization:** [SOURCE_ORGANIZATION.md](./SOURCE_ORGANIZATION.md)

## Overview

NetBox integrates multiple third-party libraries to provide protocol support and core functionality. Most dependencies are located in the `libs/` directory.

**Critical rule:** NEVER modify files in `libs/` directly. Use patches instead.

## Dependency List

### Core Protocol Libraries

#### 1. PuTTY
- **Location:** `libs/putty/`
- **Purpose:** SSH, SCP, and SFTP protocol implementation
- **Version:** 0.81
- **License:** MIT License (Simon Tatham)
- **Components Used:**
  - SSH protocol implementation (ssh2_userauth, transport layer)
  - SCP protocol support
  - SFTP protocol support (via SSH subsystem)
  - SSH authentication methods (publickey, password, keyboard-interactive, GSSAPI)
  - Cryptographic primitives (via OpenSSL)
  - OpenSSH certificate support (openssh-certs.c)
- **Integration:** `src/core/PuttyIntf.h/.cpp`, `src/core/PuttyTools.h/.cpp`, `src/core/SecureShell.h/.cpp`
- **Build Config:** `cmake/PuTTY.cmake`
- **Notes:**
  - Heavily modified for integration with NetBox
  - SSH backend for SFTP and SCP protocols
  - Provides terminal emulation support
  - Full OpenSSH certificate authentication support (detached certs via `CONF_detached_cert`)

#### 2. FileZilla
- **Location:** `src/filezilla/` (integrated in source, not `libs/`)
- **Purpose:** FTP/FTPS protocol implementation
- **Version:** 2.2.32
- **License:** GPL
- **Components Used:**
  - FTP protocol state machine
  - FTPS (FTP over SSL/TLS)
  - Active and passive mode support
  - Proxy support
- **Integration:** Integrated into core build
- **Build Config:** Part of core build
- **Notes:**
  - Modified from original FileZilla codebase
  - FTP backend for protocol implementation

#### 3. neon
- **Location:** `libs/neon/`
- **Purpose:** WebDAV and HTTP protocol implementation
- **Version:** Latest compatible
- **License:** LGPL/GPL dual license
- **Components Used:**
  - WebDAV protocol support
  - HTTP/HTTPS client
  - SSL/TLS support via OpenSSL
  - Authentication (Basic, Digest, NTLM)
- **Integration:** `src/core/NeonIntf.h/.cpp`, `src/core/WebDAVFileSystem.h/.cpp`
- **Build Config:** `cmake/Neon.cmake`
- **Notes:**
  - WebDAV backend
  - Requires OpenSSL for HTTPS

#### 4. libs3
- **Location:** `libs/libs3/`
- **Purpose:** Amazon S3 protocol implementation
- **Version:** Compatible fork
- **License:** MIT License
- **Components Used:**
  - S3 REST API implementation
  - Bucket operations
  - Object operations (upload, download, delete)
  - Multi-part upload support
- **Integration:** `src/core/S3FileSystem.h/.cpp`
- **Build Config:** `cmake/Libs3.cmake`
- **Notes:**
  - S3 storage backend
  - Compatible with AWS S3 and compatible services

### Cryptography

#### 5. OpenSSL 3
- **Location:** `libs/openssl-3/`
- **Purpose:** Cryptography and SSL/TLS support
- **Version:** 3.3.7 (synced with WinSCP 6.5.6, April 2026)
- **License:** Apache License 2.0
- **Components Used:**
  - SSL/TLS implementation
  - Cryptographic primitives (AES, RSA, EC, SHA, etc.)
  - Certificate handling
  - Random number generation
- **Integration:** Linked to all protocol implementations
- **Build Config:** `cmake/OpenSSL.cmake`
- **Notes:**
  - Critical security component
  - Used by neon, PuTTY, and direct TLS connections
  - Regular updates required for security patches
  - Sourced from WinSCP project with CMake integration
  - Excludes QUIC support, FIPS provider, and legacy ciphers not used by WinSCP
  - Platform-specific assembly for x86/x64 (NASM required); pure C fallback for ARM64

### XML Processing

#### 6. tinyxml2
- **Location:** `libs/tinyxml2/`
- **Purpose:** XML parsing and generation
- **Version:** Latest compatible
- **License:** zlib License
- **Components Used:**
  - XML document parsing
  - XML generation
  - DOM-like API
- **Integration:** Configuration storage (`src/NetBox/XmlStorage.h/.cpp`), session data
- **Build Config:** `cmake/TinyXML2.cmake`
- **Notes:**
  - Lightweight XML parser
  - Used for XML-based session configuration import/export

#### 7. expat
- **Location:** `libs/expat/`
- **Purpose:** XML parsing (streaming)
- **Version:** Latest compatible
- **License:** MIT License
- **Components Used:**
  - Fast XML parsing
  - Stream-based parsing
- **Integration:** Used where streaming XML parsing is needed
- **Build Config:** `cmake/Expat.cmake`
- **Notes:**
  - Used by libs3 for S3 XML responses

### String Formatting

#### 8. fmt
- **Location:** `libs/fmt/`
- **Purpose:** String formatting library
- **Version:** Latest compatible
- **License:** MIT License
- **Components Used:**
  - Format string processing
  - Type-safe formatting
  - Performance improvements over printf
- **Integration:** Throughout codebase
- **Build Config:** `cmake/FMT.cmake`
- **Notes:**
  - Modern C++ formatting
  - Replacement for printf/sprintf

### Logging

#### 9. tinylog
- **Location:** `libs/tinylog/`
- **Purpose:** Logging framework
- **Version:** Custom/minimal version
- **License:** Public domain / MIT
- **Components Used:**
  - Asynchronous double-buffered logging
  - Debug logging
  - Log levels
  - Background worker thread (3-second flush interval)
- **Integration:** `src/nbcore/logging.cpp`, `FTerminal->LogEvent()` throughout codebase
- **Build Config:** `cmake/TinyLog.cmake`
- **Notes:**
  - Minimal logging implementation
  - Three instances: global debug log (`g_tinylog`), per-session protocol log (`TSessionLog`), per-session action log (`TActionLog`)
  - 16 KB buffer size, 3-second flush timeout

### Memory Management

#### 10. dlmalloc
- **Location:** `libs/dlmalloc/`
- **Purpose:** Memory allocator
- **Version:** Doug Lea's malloc
- **License:** Public domain
- **Components Used:**
  - Dynamic memory allocation
  - Memory pool management
- **Integration:** Custom allocator support
- **Build Config:** `cmake/DLMalloc.cmake`
- **Notes:**
  - Alternative memory allocator
  - Performance optimization

### Windows Compatibility

#### 11. ATL/MFC Subset
- **Location:** `libs/atlmfc/`
- **Purpose:** Minimal ATL/MFC compatibility layer
- **Version:** Custom subset
- **License:** Microsoft (as part of Windows SDK)
- **Components Used:**
  - Windows API wrappers
  - COM utilities
  - String handling
- **Integration:** Windows-specific code (`src/windows/`)
- **Build Config:** `cmake/ATLMFC.cmake`
- **Notes:**
  - Minimal subset for compatibility
  - Avoids full MFC dependency

### Utilities

#### 12. GSL (Guidelines Support Library)
- **Location:** `libs/GSL/`
- **Purpose:** C++ Core Guidelines support
- **Version:** Compatible version
- **License:** MIT License
- **Components Used:**
  - `gsl::span` for array views
  - `gsl::not_null` for non-null pointers
  - Bounds checking utilities
- **Integration:** Throughout codebase
- **Build Config:** Included in main build
- **Notes:**
  - Modern C++ best practices
  - Safety utilities

#### 13. icecream-cpp
- **Location:** `libs/icecream-cpp/`
- **Purpose:** Debugging aid
- **Version:** Compatible version
- **License:** MIT License
- **Components Used:**
  - Debug printing
  - Variable inspection
- **Integration:** Debug builds only
- **Build Config:** Debug configuration
- **Notes:**
  - Development debugging tool
  - Not used in release builds

#### 14. zlib-ng
- **Location:** `libs/zlib-ng/`
- **Purpose:** Compression library
- **Version:** 2.2.2 (next-generation zlib)
- **License:** zlib License
- **Components Used:**
  - Deflate compression
  - Inflate decompression
  - GZip format support
- **Integration:** Compression support (SSH, HTTP)
- **Build Config:** `cmake/zlib-ng.cmake`
- **Notes:**
  - zlib-compatible API
  - Performance improvements over zlib

## Dependency Graph

```
NetBox Plugin
│
├── Protocol Layer
│   ├── PuTTY (SSH, SCP, SFTP)
│   │   └── OpenSSL 3 (cryptography)
│   │   └── zlib-ng (compression)
│   ├── FileZilla (FTP/FTPS)
│   │   └── OpenSSL 3 (FTPS)
│   ├── neon (WebDAV)
│   │   └── OpenSSL 3 (HTTPS)
│   │   └── expat (XML parsing)
│   └── libs3 (S3)
│       └── OpenSSL 3 (HTTPS)
│       └── expat (XML parsing)
│
├── Core Utilities
│   ├── tinyxml2 (XML config storage)
│   ├── fmt (string formatting)
│   ├── tinylog (logging)
│   └── GSL (safety utilities)
│
├── Platform Support
│   ├── ATL/MFC subset (Windows API)
│   └── dlmalloc (memory allocation)
│
└── Development
    └── icecream-cpp (debugging, debug builds only)
```

## Build Configuration

### Library Configuration Files

| Library | CMake Config | Purpose |
|---------|--------------|---------|
| PuTTY | `cmake/PuTTY.cmake` | SSH/SCP/SFTP backend |
| neon | `cmake/Neon.cmake` | WebDAV backend |
| libs3 | `cmake/Libs3.cmake` | S3 backend |
| OpenSSL 3 | `cmake/OpenSSL.cmake` | Cryptography/SSL/TLS |
| tinyxml2 | `cmake/TinyXML2.cmake` | XML parsing |
| expat | `cmake/Expat.cmake` | XML parsing (streaming) |
| FMT | `cmake/FMT.cmake` | String formatting |
| TinyLog | `cmake/TinyLog.cmake` | Logging |
| DLMalloc | `cmake/DLMalloc.cmake` | Memory allocator |
| zlib-ng | `cmake/zlib-ng.cmake` | Compression |
| ATLMFC | `cmake/ATLMFC.cmake` | Windows compatibility |
| GSL | Included in main build | Safety utilities |

### Library Integration Pattern

```cmake
# Example: Library configuration pattern
add_subdirectory(libs/library-name)
target_link_libraries(NetBox PRIVATE library-name)
```

## Version Management

### Pinned Versions

| Library | Version | Notes |
|---------|---------|-------|
| PuTTY | 0.81 | Pinned in AGENTS.md |
| FileZilla | 2.2.32 | Pinned in AGENTS.md |
| OpenSSL | 3.3.7 | Synced with WinSCP 6.5.6 |
| zlib-ng | 2.2.2 | Next-generation zlib |
| WinSCP (upstream) | 6.5.6 | Source for OpenSSL patches |

### Version Updates

- **Security updates:** Immediate (especially OpenSSL, PuTTY)
- **Feature updates:** Tested against test suite
- **Major version changes:** Require compatibility testing
- **OpenSSL patches:** After updating from upstream (WinSCP), re-apply the NetBox patch:
  ```
  git -C libs\openssl-3 apply -p3 0001-openssl-apply-NetBox-patches.patch
  ```

## License Compliance

### GPL Components
- FileZilla code (GPL)
- WinSCP derivative code (GPL)

### MIT/BSD Components
- PuTTY (MIT)
- tinyxml2 (zlib/BSD)
- fmt (MIT)
- GSL (MIT)
- icecream-cpp (MIT)
- libs3 (MIT)
- expat (MIT)

### Apache 2.0
- OpenSSL 3 (Apache 2.0)

### Attribution Requirements
- License files included in `src/resource/`
- About dialog lists all third-party components
- README.md acknowledges source codebases

## Security Considerations

### Critical Dependencies

1. **OpenSSL 3:** Must be kept current for security patches
2. **PuTTY:** SSH implementation requires security updates
3. **neon:** HTTPS support requires OpenSSL currency

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
- NASM (`buildtools/tools/nasm.exe`) — required for OpenSSL x86/x64 assembly

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
- OpenSSL uses platform-specific assembly (25 ASM files for x64, 19 for x86)

### Windows ARM64

- All dependencies must be compiled for ARM64
- OpenSSL uses pure C fallback (no ASM optimizations)
- No NASM required for ARM64 builds

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

- **Low Risk:** tinyxml2, fmt, GSL, icecream-cpp (stable, minimal)
- **Medium Risk:** neon, libs3 (smaller community)
- **High Attention:** OpenSSL, PuTTY (security-critical)

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
- Use wrapper layers for integration (`src/core/PuttyIntf.h`, `src/core/NeonIntf.h`)
- Patches documented in commit history
- OpenSSL requires re-application of NetBox-specific patches after upstream sync

## Future Considerations

### Potential Additions

- HTTP/2 support (via nghttp2 or similar)
- Modern compression algorithms (zstd, lz4)
- Enhanced logging (spdlog alternative)

### Deprecation Candidates

- icecream-cpp (development only)
- dlmalloc (if system allocator sufficient)
- expat (if tinyxml2 sufficient for all XML needs)

### Modernization Path

- C++17 features adoption
- std::filesystem instead of custom paths
- std::optional and std::variant usage
