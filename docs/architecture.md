[← User Guide](user-guide.md) · [Back to README](../README.md) · [Contributing →](contributing.md)

# Architecture

Far-NetBox uses a layered plugin architecture specific to Far Manager integration. Code flows from the Plugin Layer through Core Protocols to Base Foundation classes, with third-party libraries providing protocol implementations.

## Folder Structure

```
src/
├── NetBox/              # Plugin Layer — Far Manager API entry point
│   ├── plugin.cpp       # PluginMain, GetPluginInfo, OpenPlugin
│   ├── plugin.hpp
│   └── menu.hpp         # Menu definitions
├── core/                # Core Layer — Protocol implementations
│   ├── SecureShell.h    # SFTP/SCP via PuTTY
│   ├── FTP.h            # FTP via FileZilla
│   ├── WebDAV.h         # WebDAV via neon
│   ├── S3.h             # S3 via libs3
│   └── CustomFileSystem.h  # Abstract base for protocols
├── filezilla/           # FileZilla engine (FTP/FTPS)
├── base/                # Base Layer — Foundation classes
│   ├── UnicodeString.h
│   ├── Classes.h
│   └── Exceptions.h
├── nbcore/              # Core utilities (strings, memory, logging)
├── include/             # Public headers (nbtypes.h, rtti.hpp)
├── PluginSDK/            # Far3 plugin SDK headers
├── resource/             # Resources (.rc, .lng, licenses)
└── windows/             # Windows-specific code (GUI, dialogs)
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

| Direction | Allowed | Rule |
|-----------|---------|------|
| Plugin → Core | Yes | Plugin calls protocol implementations |
| Core → Base | Yes | Protocols use foundation classes |
| Core → Third-Party | Yes | Protocols wrap library implementations |
| Plugin → Third-Party | No | Must go through Core |
| Base → Core | No | Foundation must be independent |

## Plugin Integration

### Required Exports

```cpp
void WINAPI GetPluginInfoW(PluginInfo *Info);
HANDLE WINAPI OpenPluginW(const OpenInfo *Info);
void WINAPI ClosePluginW(HANDLE Handle, int ExitCode);
```

Far Manager loads `NetBox.dll` and calls these entry points. The plugin registers menu items, handles panel events, and manages session lifecycle.

## Protocol Abstraction

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
  virtual bool MakeDirectory(const UnicodeString & DirName) = 0;
};
```

The protocol factory creates the correct implementation at runtime:

```cpp
TCustomFileSystem *CreateFileSystem(TSessionData * Data)
{
  switch (Data->FSProtocol)
  {
    case fsSFTP: return new TSecureShell(Data);
    case fsFTP:  return new TFTPFileSystem(Data);
    case fsWebDAV: return new TWebDAVFileSystem(Data);
    case fsS3:   return new TS3FileSystem(Data);
    default:     DebugAlwaysAssert(false); return nullptr;
  }
}
```

## Memory Management

- **RAII** — Smart pointers and destructor cleanup
- **No raw new/delete** — Wrap allocations in helpers
- **String management** — Use `UnicodeString` class
- **Handle cleanup** — Close handles in destructors or RAII wrappers

## Error Handling

- **Exceptions** — Throw from `Exception` hierarchy (`EAbort`, `EInOutError`)
- **Try/catch in UI** — Top-level handlers catch and display errors
- **Network errors** — Wrap with meaningful messages including URL/path
- **Debug output** — Use `FTerminal->LogEvent()` for tracing

## Key Principles

1. **Plugin first, core second** — Far Manager API requirements must be satisfied
2. **Protocol abstraction** — All protocols implement `TCustomFileSystem`
3. **Clean dependency flow** — Plugin → Core → Base → Third-Party
4. **No third-party modifications** — Use patches instead of direct changes
5. **WinXP compatibility** — Avoid modern Windows APIs
6. **Build verification** — Compile with MSVC W4 (zero warnings)
7. **Thread safety** — Far Manager API calls from main thread only; worker threads use event-driven waits

## Build Output

| Artifact | Location |
|----------|----------|
| Plugin DLL | `Far3_<platform>/Plugins/NetBox/` |
| Platform dirs | `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/` |
| Build dir | `build-<config>/` |

## See Also

- [Contributing](contributing.md) — Code conventions and development workflow
- [OpenSSL Sync Report](openssl_sync_cleanup_report.md) — OpenSSL 3 synchronization details
- [User Guide](user-guide.md) — Protocol descriptions and features
