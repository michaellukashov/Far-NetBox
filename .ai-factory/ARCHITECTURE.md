# Architecture: Layered Plugin

## Overview

Far-NetBox uses a **layered plugin architecture** designed around Far Manager 3.0's plugin API contract. The architecture follows strict dependency flow from the Plugin Layer (Far Manager API integration) through the Core Layer (protocol implementations) to the Base Layer (foundation classes), with third-party libraries providing low-level protocol implementations.

This architecture was chosen because:
- Far Manager plugin API requires a specific entry point and callback structure that dictates the top layer
- Multiple protocol implementations (SFTP, FTP, SCP, WebDAV, S3) share common file transfer patterns abstracted through `TCustomFileSystem`
- The existing codebase from WinSCP/PuTTY/FileZilla provides proven implementations that map naturally to horizontal layers
- Incremental improvement is required — no major architectural rewrites, so a clean layered model provides the strongest guardrails for safe evolution

## Decision Rationale

- **Project type:** Desktop plugin for Far Manager (Windows)
- **Tech stack:** C++17, MSVC, CMake 3.15+
- **Key factor:** Far Manager 3.0 Plugin API integration with multiple protocol implementations behind a common interface

## Folder Structure

```
src/
├── NetBox/              # Plugin Layer — Far Manager API entry point
│   ├── NetBox.cpp       # DLL entry, global callbacks
│   ├── WinSCPPlugin.*   # WinSCP plugin adapter
│   ├── WinSCPFileSystem.* # WinSCP filesystem adapter
│   ├── FarPlugin.*      # Far Manager plugin wrapper
│   ├── FarInterface.*   # Far API abstraction
│   ├── FarDialog.*      # Dialog framework (TFarDialog, TTabbedDialog)
│   ├── FarConfiguration.* # Far-specific configuration storage
│   ├── Far3Storage.*    # Far 3.x storage backend
│   ├── WinSCPDialogs.cpp # Session/login dialogs
│   ├── MessageDlg.cpp   # Message dialog implementation
│   ├── FarPluginStrings.* # Localized string resources
│   ├── XmlStorage.*     # XML-based configuration persistence
│   ├── MsgIDs.h         # Message ID definitions
│   ├── *.lng            # Language files (EN, FR, POL, RUS, SPA)
│   ├── plugin_version.hpp # Version constants
│   ├── guid.h           # Plugin GUIDs
│   └── resource.h        # Resource identifiers
├── core/                # Core Layer — Protocol implementations & session management
│   ├── FileSystems.*    # TCustomFileSystem abstract base
│   ├── SftpFileSystem.* # SFTP protocol (via TSecureShell → PuTTY)
│   ├── ScpFileSystem.*  # SCP protocol (via TSecureShell → PuTTY)
│   ├── SecureShell.*    # SSH session management (PuTTY wrapper)
│   ├── FtpFileSystem.*  # FTP/FTPS protocol (via FileZilla engine)
│   ├── WebDAVFileSystem.* # WebDAV protocol (via neon)
│   ├── S3FileSystem.*   # Amazon S3 protocol (via libs3)
│   ├── NeonIntf.*       # neon library interface (WebDAV + S3 TLS)
│   ├── PuttyIntf.*      # PuTTY interface glue
│   ├── Terminal.*       # Terminal coordinator & session orchestration
│   ├── SessionData.*    # Session configuration container
│   ├── RemoteFiles.*    # Remote file representation & directory listings
│   ├── Configuration.*  # Global configuration management
│   ├── CopyParam.*      # Copy/move operation parameters
│   ├── Cryptography.*   # Cryptographic utilities
│   ├── Queue.*          # Background operation queue
│   └── ...              # (other core modules)
├── filezilla/           # FileZilla FTP engine (third-party, adapted)
├── base/                # Base Layer — Foundation classes
│   ├── UnicodeString.*  # Wide string class
│   ├── Exceptions.*     # Exception hierarchy (EAbort, EInOutError, ...)
│   ├── Classes.*        # Base class definitions
│   ├── Sysutils.*       # System utilities
│   ├── System.SyncObjs.* # Synchronization primitives
│   ├── System.IOUtils.*  # File I/O utilities
│   ├── FileBuffer.*     # File buffering
│   ├── LogContext.*     # Log context base
│   └── ...              # (other foundation modules)
├── nbcore/              # NetBox core utilities
│   ├── nbstring.cpp     # String utilities
│   ├── nbmemory.cpp     # Memory management
│   ├── nbutils.cpp      # General utilities
│   └── stdafx.h         # Precompiled header
├── include/             # Public headers & type definitions
│   ├── nbtypes.h        # Core type aliases
│   ├── nbstring.h       # String type aliases
│   ├── nbsystem.h       # System type aliases
│   ├── rtti.hpp         # RTTI utilities
│   ├── type_traits.h    # Type traits
│   └── FastDelegate.h   # Fast delegate implementation
├── PluginSDK/           # Far Manager 3.x plugin SDK headers (read-only)
├── resource/            # Binary resources (icons, bitmaps, licenses)
└── windows/             # Windows-specific code (GUI, COM, registry)
    ├── WinConfiguration.* # Windows configuration storage
    ├── GUIConfiguration.* # GUI-specific settings
    ├── GUITools.*       # GUI helper functions
    ├── TerminalManager.* # Terminal session manager
    └── ...              # (other Windows modules)
```

## Dependency Rules

```
┌─────────────────────────────────────────────────┐
│           Plugin Layer (src/NetBox/)             │
│   Far Manager API integration, dialogs, config   │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│            Core Layer (src/core/)                │
│   Protocol implementations, session management   │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│       Base Layer (src/base/, src/nbcore/)        │
│   Foundation classes, strings, exceptions, sync   │
└─────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────┐
│        Third-Party (libs/, src/filezilla/)        │
│   PuTTY, FileZilla, OpenSSL, neon, libs3, ...    │
└─────────────────────────────────────────────────┘
```

### Allowed Dependencies

- ✅ Plugin Layer → Core Layer (plugin calls protocol implementations)
- ✅ Plugin Layer → Base Layer (plugin uses foundation classes directly)
- ✅ Core Layer → Base Layer (protocols use foundation classes)
- ✅ Core Layer → Third-Party (protocols wrap library implementations)
- ✅ Base Layer → Third-Party (foundation may use zlib, fmt, GSL)
- ✅ `windows/` → Core Layer (Windows UI calls session management)

### Forbidden Dependencies

- ❌ Plugin Layer → Third-Party (must go through core or base)
- ❌ Base Layer → Core Layer (foundation must be independent of protocols)
- ❌ Core Layer → Plugin Layer (protocols must not know about Far Manager)
- ❌ Base Layer → Plugin Layer (foundation must not know about Far Manager)
- ❌ Any layer → `libs/` source modifications (use patches only)

## Layer/Module Communication

### Protocol Abstraction

All protocol implementations communicate with the Plugin Layer through the `TCustomFileSystem` interface. The Plugin Layer never knows which specific protocol it is using — it only calls the abstract interface methods.

```
Plugin Layer
    ↓ (calls TCustomFileSystem interface)
Core Layer
    ↓ (implements TCustomFileSystem)
    ├── TSFTPFileSystem  → TSecureShell → PuTTY
    ├── TSCPFileSystem   → TSecureShell → PuTTY
    ├── TFTPFileSystem   → FileZilla engine
    ├── TWebDAVFileSystem → neon library
    └── TS3FileSystem    → libs3 library
```

### Factory Pattern

Protocol-specific instances are created via the factory function. The caller provides `TSessionData` which includes the `FSProtocol` field, and receives a `TCustomFileSystem*`:

```cpp
TCustomFileSystem *CreateFileSystem(TSessionData *Data)
{
  switch (Data->FSProtocol)
  {
    case fsSFTP:   return new TSFTPFileSystem(Data);
    case fsSCP:    return new TSCPFileSystem(Data);
    case fsFTP:    return new TFTPFileSystem(Data);
    case fsWebDAV: return new TWebDAVFileSystem(Data);
    case fsS3:     return new TS3FileSystem(Data);
    default:       DebugAlwaysAssert(false); return nullptr;
  }
}
```

### Session Coordination

`TTerminal` orchestrates the lifecycle of a remote session. It owns the `TCustomFileSystem` instance and mediates between the Plugin Layer's Far Manager callbacks and the protocol operations:

```
Far Manager callback → WinSCPFileSystem → TTerminal → TCustomFileSystem
```

### Configuration Flow

Session and global configuration flows from XML storage through `TConfiguration` / `TSessionData` to protocol instances:

```
XmlStorage / Far3Storage → TConfiguration → TSessionData → TCustomFileSystem
```

## Key Principles

1. **Plugin first, core second** — Far Manager API requirements must be satisfied before any other concern
2. **Protocol abstraction** — All protocols implement `TCustomFileSystem` interface; callers never depend on concrete protocol types
3. **Clean dependency flow** — Plugin → Core → Base → Third-Party; never upward
4. **No third-party modifications** — Use patches instead of direct changes to `libs/`
5. **WinXP compatibility** — Avoid modern Windows APIs that break compatibility with older Windows versions
6. **Build verification** — All code must compile with MSVC `/W4` (zero warnings)
7. **Thread safety** — All Far Manager API calls from main thread only; worker threads use event-driven waits; static mutable state requires explicit synchronization
8. **RAII ownership** — Prefer `std::unique_ptr` over raw `new`/`delete`; handle cleanup in destructors
9. **Exception-based error handling** — Throw from the `Exception` hierarchy; catch and display at UI boundary
10. **Incremental evolution** — No major architectural rewrites; extend existing patterns

## Code Examples

### Protocol Implementation

Every protocol follows the same pattern — inherit `TCustomFileSystem`, override virtual methods, own the third-party connection:

```cpp
class TS3FileSystem : public TCustomFileSystem
{
public:
  explicit TS3FileSystem(TTerminal *Terminal);
  virtual ~TS3FileSystem() override;

  virtual bool Connect() override;
  virtual void Disconnect() override;
  virtual bool GetFile(const UnicodeString &FileName,
    const UnicodeString &LocalFile, ...) override;
  virtual bool PutFile(const UnicodeString &LocalFile,
    const UnicodeString &FileName, ...) override;
  virtual bool DeleteFile(const UnicodeString &FileName) override;
  virtual bool RenameFile(const UnicodeString &OldName,
    const UnicodeString &NewName) override;
  virtual bool MakeDirectory(const UnicodeString &DirName) override;

private:
  TTerminal *FTerminal;
  S3Connection FConnection;  // wraps libs3
};
```

### Dependency Rule Enforcement — Core Does Not Know Plugin

The Core Layer (`src/core/`) never includes Far Manager headers. This is enforced by include structure:

```cpp
// core/SftpFileSystem.cpp — CORRECT
#include "SftpFileSystem.h"    // core header
#include "SecureShell.h"       // core header
#include "RemoteFiles.h"       // core header
// NO Far Manager includes — protocol is agnostic

// NetBox/WinSCPFileSystem.cpp — CORRECT
#include "WinSCPFileSystem.h"  // plugin header
#include "FarPlugin.h"         // plugin header (Far API)
#include "Terminal.h"          // core header (used through interface)
// Plugin includes core, core never includes plugin
```

### RAII Resource Management

```cpp
// Use smart pointers for exclusive ownership
auto FileList = std::make_unique<TRemoteFile>();

// Handle cleanup via RAII wrapper
class ConnectionHandle
{
public:
  explicit ConnectionHandle(S3Connection Conn) : FConn(Conn) {}
  ~ConnectionHandle() { if (FConn) S3_release(FConn); }
  // non-copyable, movable
  ConnectionHandle(const ConnectionHandle &) = delete;
  ConnectionHandle &operator=(const ConnectionHandle &) = delete;
  ConnectionHandle(ConnectionHandle &&Other) noexcept : FConn(Other.FConn)
  { Other.FConn = nullptr; }
private:
  S3Connection FConn;
};
```

### Error Handling Pattern

```cpp
// Throw from Exception hierarchy
throw ExtException(L"Connection failed", L"Host: " + FHostName);

// Catch at UI boundary (Plugin Layer)
try
{
  FTerminal->Open();
}
catch (ExtException &E)
{
  FTerminal->LogEvent(FORMAT(L"Error: %s", E.Message));
  FarPlugin->ShowMessage(E.Message);
}
catch (const std::exception &E)
{
  FTerminal->LogEvent(FORMAT(L"std::exception: %s", UnicodeString(E.what())));
}
```

## Anti-Patterns

- ❌ **Skip protocol inheritance** — All protocols must inherit from `TCustomFileSystem`; never create a "special" protocol bypassing the interface
- ❌ **Direct third-party changes** — Never modify files in `libs/`; use patches and re-apply after upstream updates
- ❌ **Skip build verification** — Changes must compile without warnings under `/W4`
- ❌ **Silent error swallowing** — Network errors must be caught and reported, not silently ignored
- ❌ **Mix layers** — Plugin code must not call third-party APIs directly; Core code must not include Far Manager headers
- ❌ **Raw new/delete** — Use `std::make_unique` / `std::unique_ptr` instead of manual allocation
- ❌ **Far Manager API from worker threads** — All Far Manager API calls (dialogs, panels, messages) must be on the main thread
- ❌ **Dangling remote file pointers** — Directory listing objects can be invalidated by navigation; always duplicate `TRemoteFile` with `Standalone=true` when retaining references
- ❌ **Missing cycle detection in traversal** — Recursive directory operations must track visited paths to prevent infinite loops
- ❌ **MSG_* enum without .lng update** — Adding a message ID to `MsgIDs.h` without adding the corresponding entry in all `.lng` files causes crashes or missing strings
- ❌ **Unescaped server data in FMTLOAD** — Never pass server responses, redirect URIs, or remote shell output directly to `FMTLOAD` / `FORMAT` — `%` characters will be interpreted as format specifiers (CWE-134). Use `EscapeFmtChars()` to sanitize untrusted strings before passing them as format arguments

## References

| Reference | Topic |
|-----------|-------|
| [cwe134-fmtload-vulnerability-scan](references/cwe134-fmtload-vulnerability-scan.md) | CWE-134 format string vulnerability scan: all vulnerable FMTLOAD call sites, risk levels, EscapeFmtChars utility |
| [message-loading-system](references/message-loading-system.md) | GetMsg/FmtLoadStr resolution, ID mapping tables, debugging unmapped IDs |
| [fix-issue-389-pureftpd-tls-explicit](references/fix-issue-389-pureftpd-tls-explicit.md) | Full investigation: FTP explicit TLS AUTH flow, state machine, OpenSSL 3 bn_div_words assembly crash, session log debugging |
| [multithreading-audit-exploration](references/multithreading-audit-exploration.md) | Far API thread affinity violations, race conditions, busy-waiting, static state |
| [crash-second-file-open-analysis](references/crash-second-file-open-analysis.md) | Dangling TRemoteFile pointers, directory listing invalidation |
| [issue-501-ssh-scp-buffer-corruption-exploration](references/issue-501-ssh-scp-buffer-corruption-exploration.md) | Dynamic TCP send buffer resizing (SIO_IDEAL_SEND_BACKLOG_QUERY) causing SCP corruption and CPU saturation |
| [esc-cancellation-comprehensive-fix](references/esc-cancellation-comprehensive-fix.md) | Four-layer root-cause fix for SCP Esc cancellation, console input buffer, cancel-state semantics, exception conversion, and cleanup hang (Issue [#511](https://github.com/michaellukashov/Far-NetBox/issues/511)) |