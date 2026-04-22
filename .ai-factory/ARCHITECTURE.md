# Architecture: Far-NetBox Plugin

## Overview

Far-NetBox uses a **layered plugin architecture** specific to Far Manager integration. The architecture follows dependency flow from the Plugin Layer (Far Manager API) through Core Protocols to Base Foundation classes, with third-party libraries providing底层 protocol implementations.

This architecture was chosen because:
- Far Manager plugin API requires a specific entry point and callback structure
- Multiple protocol implementations share common file transfer patterns
- Existing codebase from WinSCP/PuTTY/FileZilla provides proven implementations

## Decision Rationale

- **Project type:** Desktop plugin for Far Manager (Windows)
- **Tech stack:** C++17, MSVC, CMake
- **Key factor:** Integration with Far Manager 3.0 Plugin API, multiple protocol implementations

## Folder Structure

```
src/
├── NetBox/              # Plugin Layer - Far Manager API entry point
│   ├── plugin.cpp       # PluginMain, GetPluginInfo, OpenPlugin
│   ├── plugin.hpp
│   └── menu.hpp       # Menu definitions
├── core/               # Core Layer - Protocol implementations
│   ├── SecureShell.h   # SFTP/SCP via PuTTY
│   ├── FTP.h           # FTP via FileZilla
│   ├── WebDAV.h        # WebDAV via neon
│   ├── S3.h            # S3 via libs3
│   └── CustomFileSystem.h  # Abstract base for protocols
├── filezilla/          # FileZilla engine (FTP/FTPS)
├── base/               # Base Layer - Foundation classes
│   ├── UnicodeString.h
│   ├── Classes.h
│   └── Exceptions.h
├── nbcore/             # Core utilities (strings, memory, logging)
├── include/            # Public headers (nbtypes.h, rtti.hpp)
├── PluginSDK/           # Far3 plugin SDK headers
├── resource/            # Resources (.rc, .lng, licenses)
└── windows/            # Windows-specific code (GUI, dialogs)
```

## Dependency Rules

```
Plugin Layer (NetBox/)
        ↓
Core Layer (core/)
        ↓
Base Layer (base/, nbcore/)
        ↓
Third-Party (libs/)
```

- ✅ Plugin Layer → Core Layer (plugin calls protocol implementations)
- ✅ Core Layer → Base Layer (protocols use foundation classes)
- ✅ Core Layer → Third-Party (protocols wrap library implementations)
- ❌ Plugin Layer → Third-Party (must go through core)
- ❌ Base Layer → Core Layer (foundation must be independent)

## Plugin Integration

### Entry Points Required by Far Manager

```cpp
void WINAPI GetPluginInfoW(PluginInfo *Info);
HANDLE WINAPI OpenPluginW(const OpenInfo *Info);
void WINAPI ClosePluginW(HANDLE Handle, int ExitCode);
```

### Plugin Functions

```cpp
// Required exports
export_t GetPluginInfoW;
export_t OpenPluginW;
export_t ClosePluginW;
export_t GetVirtualFindFileW;
export_t SetDirectoryW;
export_t DeleteFileW;
export_t MakeDirectoryW;
export_t RenameFileW;
export_t GetFileW;
export_t PutFilesW;
```

## Protocol Interface

### TCustomFileSystem Base

All protocol implementations inherit from `TCustomFileSystem`:

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
  // ... more virtual methods
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

## Memory Management

- **RAII pattern:** Use smart pointers and destructor cleanup
- **No raw new/delete:** Wrap allocations in helper functions
- **String management:** Use `UnicodeString` class, avoid `new wchar_t[]`
- **Handle cleanup:** Always close handles in destructors or via RAII wrappers

## Error Handling

- **Exceptions:** Throw from `Exception` hierarchy (`EAbort`, `EInOutError`)
- **Try/catch in UI:** Top-level handlers catch and display errors
- **Network errors:** Wrap with meaningful messages including URL/path
- **Debug output:** Use `FTerminal->LogEvent()` for debug tracing

## Key Principles

1. **Plugin first, core second** — Far Manager API requirements must be satisfied
2. **Protocol abstraction** — All protocols implement `TCustomFileSystem` interface
3. **Clean dependency flow** — Plugin → Core → Base → Third-Party
4. **No third-party modifications** — Use patches instead of direct changes
5. **WinXP compatibility** — Avoid modern Windows APIs that break compatibility
6. **Build verification** — All code must compile with MSVC W4 (no warnings)

## Protocol Implementation Pattern

```cpp
class TMyProtocol : public TCustomFileSystem
{
public:
  explicit TMyProtocol(TSessionData * SessionData);
  virtual ~TMyProtocol() override;

  virtual bool Connect() override;
  virtual void Disconnect() override;
  virtual void FindFirst(const UnicodeString & Path,
    TFindFileList & List) override;

private:
  TSessionData * FSessionData;
  ConnectionHandle FConnection;
};
```

## Anti-Patterns

- ❌ **Skip protocol inheritance** — All protocols must inherit from `TCustomFileSystem`
- ❌ **Direct third-party changes** — modify libs/ only after confirmation
- ❌ **Skip build verification** — Changes must compile without warnings
- ❌ **No debug output** — Use FTerminal->LogEvent() for tracing protocol operations
- ❌ **Skip exception handling** — Network errors must be caught properly
- ❌ **Mix layers** — Plugin code should not call third-party directly

## Build Layers

```
┌─────────────────────────────────────────┐
│         Plugin Layer (NetBox/)           │
│    Far Manager API integration          │
└─────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│          Core Layer (core/)             │
│   Protocol implementations              │
└─────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│          Base Layer (base/)             │
│   Foundation classes                   │
└─────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────┐
│      Third-Party (libs/)                │
│   PuTTY, FileZilla, OpenSSL, neon...   │
└─────────────────────────────────────────┘
```