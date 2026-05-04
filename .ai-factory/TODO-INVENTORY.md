# NetBox TODO/FIXME/HACK Inventory

> Generated: 2026-05-04
> Scope: `src/` directory only (NetBox-specific code)
> Excludes: `libs/` third-party code, language translation files

---

## Legend

| Column | Meaning |
|--------|---------|
| File | Source file path relative to project root |
| Line | Approximate line number (anchor may drift over time) |
| Type | `TODO` — planned improvement / `FIXME` — known defect / `HACK` — workaround |
| Summary | Brief description of the item |
| Priority | `H` = High (user-facing or crash risk) / `M` = Medium (feature gap) / `L` = Low (cleanup, cosmetic) |
| Plan | Link to existing `.ai-factory/plans/` file if one exists |

---

## 1. NetBox UI (`src/NetBox/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 1.1 | `FarDialog.cpp` | 11 | TODO | Move `TSimpleThread` to `Sysutils` | L | — |
| 1.2 | `FarDialog.cpp` | 94 | TODO | Use `SAFE_DESTROY(Item)` instead of manual detach | M | — |
| 1.3 | `FarDialog.cpp` | 876 | TODO | Throw error instead of silent return in color path | M | — |
| 1.4 | `FarDialog.cpp` | 2725 | TODO | Hide cursor in `TFarLister::DoFocus()` | L | — |
| 1.5 | `FarInterface.cpp` | 36 | TODO | Output `NetBoxPluginGuid` instead of hardcoded string | L | — |
| 1.6 | `FarPlugin.cpp` | 6 | TODO | Move `TSimpleThread` to `Sysutils` (same as 1.1) | L | — |
| 1.7 | `FarPlugin.cpp` | 935 | FIXME | Re-enable `DebugAssert(FMSG_DOWN)` check | L | — |
| 1.8 | `FarPlugin.cpp` | 1736 | TODO | Report error on failed operation | M | — |
| 1.9 | `FarPlugin.cpp` | 2008 | TODO | Move cleanup code to `DestroyFarPlugin` | L | — |
| 1.10 | `FarPlugin.cpp` | 2824 | TODO | Move panel-item fetch to common function | L | — |
| 1.11 | `FarPlugin.cpp` | 503 | TODO | Inform Far about unknown `OpenFrom` values | M | — |
| 1.12 | `FarPlugin.cpp` | 548 | TODO | Implement `Options->ParseParams(CommandLineParams)` | M | — |
| 1.13 | `WinSCPDialogs.cpp` | 1348 | FIXME | Restore `Text->SetColor()` for about dialog URL | L | — |
| 1.14 | `WinSCPDialogs.cpp` | 4076 | TODO | Sync `SetFtps()` from `TLoginDialog::PortNumberEditChange` | M | — |
| 1.15 | `WinSCPDialogs.cpp` | 4353 | TODO | Sync OpenSSH private key file from WinSCP | M | — |
| 1.16 | `WinSCPDialogs.cpp` | 4702 | TODO | Check `PrivateKeyEdit`/`TunnelPrivateKeyEdit` were edited | L | — |
| 1.17 | `WinSCPDialogs.cpp` | 9261 | TODO | Check actual configuration for queue dialog size | L | — |
| 1.18 | `WinSCPFileSystem.cpp` | 1897 | TODO | Use `DontOverwrite` option in `CopyFiles()` | M | — |
| 1.19 | `WinSCPPlugin.cpp` | 70 | TODO | Read tinylog level from config file | L | — |
| 1.20 | `WinSCPPlugin.cpp` | 71 | TODO | Read log file path from config file | L | — |
| 1.21 | `WinSCPPlugin.cpp` | 76 | TODO | Enable `icecream::ic.output(logFile)` | L | — |

---

## 2. Base / Core Utilities (`src/base/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 2.1 | `Sysutils.cpp` | 66 | TODO | Use `AHelpContext` in `Exception` constructor | L | `implement-todos.md` |
| 2.2 | `Sysutils.cpp` | 1031 | FIXME | Implement `IsDriveRooted()` properly | M | `implement-todos.md` |
| 2.3 | `Sysutils.cpp` | 2267 | TODO | Throw exception on `FileWrite` error instead of returning `false` | M | `implement-todos.md` |
| 2.4 | `Sysutils.hpp` | 18 | — | `TODO()` macro definition (not a task) | — | — |
| 2.5 | `Classes.cpp` | 1120 | FIXME | Use `ERegistryException.CreateResFmt` for invalid reg type | L | — |
| 2.6 | `Classes.cpp` | 1808 | FIXME | Use `ERegistryException.CreateResFmt` for query failure | L | — |
| 2.7 | `Classes.cpp` | 1822 | FIXME | Use `ERegistryException.CreateResFmt` for set failure | L | — |
| 2.8 | `Classes.hpp` | 439 | TODO | Enable `ROIndexedProperty<TObject *> Objects` | L | — |
| 2.9 | `Classes.hpp` | 440 | TODO | Enable `ROIndexedProperty<UnicodeString> Names` | L | — |
| 2.10 | `Classes.hpp` | 632 | TODO | Move date/time exports to `DateUtils.hpp` | L | — |
| 2.11 | `Classes.hpp` | 1038 | FIXME | `TShortCut` class stub | L | — |
| 2.12 | `Common.cpp` | 48 | TODO | Use `Path` class instead of `UnicodeString` | M | — |
| 2.13 | `Common.cpp` | 119 | TODO | Use `UnixPathSeparator` constant | L | — |
| 2.14 | `Common.cpp` | 134 | TODO | Use `UnixPathSeparator` constant | L | — |
| 2.15 | `Common.cpp` | 1627 | TODO | Use `ExpandUNCFileName` | L | — |
| 2.16 | `Common.cpp` | 5256 | TODO | Implement `ParentProcessName` for MSVC | L | — |
| 2.17 | `Common.h` | 606 | TODO | Move `GetEnvironmentVariable` to `Sysutils.hpp` | L | — |
| 2.18 | `Global.cpp` | 9 | TODO | Remove `src/core` dependency from `Global.cpp` | L | — |
| 2.19 | `function2.hpp` | 722 | TODO | Optimize single formal argument | L | — |
| 2.20 | `function2.hpp` | 723 | TODO | Merge vtable tables if size-optimized | L | — |
| 2.21 | `function2.hpp` | 818 | TODO | Use unreachable intrinsic instead of `assert(false)` | L | — |
| 2.22 | `nbstring.h` | 26 | FIXME | `memcpy_s` implementation is unsafe | M | — |
| 2.23 | `nbstring.h` | 28 | FIXME | `_mbsstr` implementation is naive (`strstr` fallback) | L | — |
| 2.24 | `nbstring.h` | 35 | FIXME | `Langpack_GetDefaultCodePage()` assumption may be wrong | L | — |
| 2.25 | `rtti.hpp` | 32 | TODO | Rename / replace `simplify_type` cast traits | L | — |
| 2.26 | `rtti.hpp` | 54 | TODO | Add `detail` namespace once everyone switched | L | — |

---

## 3. Protocols (`src/core/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 3.1 | `Terminal.cpp` | 1479 | TODO | Handle complex relative paths (`../../xxx`) | M | — |
| 3.2 | `Terminal.cpp` | 1625 | TODO | Warn user on certain path conditions | M | — |
| 3.3 | `Terminal.cpp` | 3222 | HACK | Enable "skip to all" for `TRetryOperationLoop` | M | — |
| 3.4 | `Terminal.cpp` | 4646 | TODO | Avoid resolving symlinks when reading subdirectories | H | — |
| 3.5 | `Terminal.cpp` | 8183 | TODO | Delete read-only directories | M | — |
| 3.6 | `Terminal.cpp` | 8184 | TODO | Show error message on delete failure | M | — |
| 3.7 | `Terminal.cpp` | 8271 | TODO | Delete read-only files | M | — |
| 3.8 | `Terminal.h` | 138 | TODO | Better user interface for file operation loops | M | — |
| 3.9 | `SecureShell.cpp` | 450 | TODO | Consider `tcp_keepalives = false` | L | — |
| 3.10 | `SecureShell.cpp` | 757 | HACK | Gross hack for server version string parsing | L | — |
| 3.11 | `SecureShell.cpp` | 1449 | TODO | Use `UTF8ToString` instead of manual conversion | L | — |
| 3.12 | `SecureShell.cpp` | 1453 | TODO | Use `AnsiToString` instead of `MB2W` | L | — |
| 3.13 | `SecureShell.cpp` | 1795 | TODO | Log input only if `LogLevel > 4` | L | — |
| 3.14 | `SecureShell.cpp` | 2033 | HACK | Call `sk_tcp_close()` to release socket memory | M | — |
| 3.15 | `SecureShell.cpp` | 2170 | TODO | Make timeout configurable | L | — |
| 3.16 | `SecureShell.cpp` | 2173 | TODO | Make event loop step configurable | L | — |
| 3.17 | `SecureShell.cpp` | 2472 | TODO | Guard `LogEvent` behind `LogLevel > 4` | L | — |
| 3.18 | `SecureShell.cpp` | 2562 | TODO | Guard timeout log behind `LogLevel > 4` | L | — |
| 3.19 | `SessionData.cpp` | 1468 | TODO | Implement unimplemented function (id 3041) | M | — |
| 3.20 | `SessionData.cpp` | 1673 | TODO | Implement `PostLoginCommands` | M | — |
| 3.21 | `SessionData.cpp` | 1733 | TODO | Map known proxy sequences to enumeration | L | — |
| 3.22 | `SessionData.cpp` | 4168 | TODO | Implement unimplemented settings export | M | — |
| 3.23 | `SessionData.cpp` | 4795 | TODO | Support `pmHTTPS` proxy method | M | — |
| 3.24 | `SessionInfo.cpp` | 957 | TODO | Log error in `TSessionLog::DoAddToSelf` | L | — |
| 3.25 | `SessionInfo.cpp` | 1204 | TODO | Implement `GetCmdLine()` | L | — |
| 3.26 | `SessionInfo.cpp` | 1955 | TODO | Implement startup info logging for MSVC | L | — |
| 3.27 | `FtpFileSystem.cpp` | 592 | TODO | Handle account parameter (rarely used) | L | — |
| 3.28 | `FtpFileSystem.cpp` | 1007 | TODO | Improve path handling (handle `..` etc.) | M | — |
| 3.29 | `FtpFileSystem.cpp` | 1316 | TODO | Parse `SITE SYMLINK target link` format | L | — |
| 3.30 | `FtpFileSystem.cpp` | 1445 | TODO | Implement retries/resume on transfer abort | H | — |
| 3.31 | `FtpFileSystem.cpp` | 3434 | TODO | Handle `REPLY_CRITICALERROR` properly | M | — |
| 3.32 | `FtpFileSystem.cpp` | 4263 | TODO | Use `Sysutils::FormatDateTime` | L | — |
| 3.33 | `ScpFileSystem.cpp` | 132 | TODO | Remove `mf`/`cd` flags (already in `TTerminal`) | L | — |
| 3.34 | `ScpFileSystem.cpp` | 1619 | TODO | Implement retries/resume on batch abort | H | — |
| 3.35 | `ScpFileSystem.cpp` | 1761 | TODO | Show stderr to user | M | — |
| 3.36 | `ScpFileSystem.cpp` | 2021 | TODO | Show stderr to user (duplicate) | M | — |
| 3.37 | `ScpFileSystem.cpp` | 2200 | TODO | Support >32-bit file sizes in ASCII mode | H | — |
| 3.38 | `ScpFileSystem.cpp` | 2354 | TODO | Send filetime with file | M | — |
| 3.39 | `ScpFileSystem.cpp` | 2410 | TODO | Delete read-only directories | M | — |
| 3.40 | `ScpFileSystem.cpp` | 2411 | TODO | Show error message on delete failure | M | — |
| 3.41 | `ScpFileSystem.cpp` | 2660 | TODO | Show stderr to user (triplicate) | M | — |
| 3.42 | `ScpFileSystem.cpp` | 2835 | TODO | Turn off read-only attribute before operations | M | — |
| 3.43 | `SftpFileSystem.cpp` | 3127 | TODO | Improve path handling (handle `..` etc.) | M | — |
| 3.44 | `SftpFileSystem.cpp` | 4546 | TODO | Implement retries/resume on error | H | — |
| 3.45 | `SftpFileSystem.cpp` | 5732 | TODO | Use `std::unique_ptr<>` for `FileStream` | L | — |
| 3.46 | `WebDAVFileSystem.cpp` | 615 | TODO | Keep WebDAV session alive (noop placeholder) | M | — |
| 3.47 | `WebDAVFileSystem.cpp` | 2371 | TODO | Implement unimplemented function | M | — |
| 3.48 | `S3FileSystem.cpp` | 505 | TODO | Port AWS config file reading to MSVC | M | — |
| 3.49 | `S3FileSystem.cpp` | 836 | TODO | Implement unimplemented S3 status handler | M | — |
| 3.50 | `RemoteFiles.cpp` | 22 | TODO | Use `Path` class instead of `UnicodeString` | M | — |
| 3.51 | `RemoteFiles.cpp` | 949 | TODO | Handle linked file attributes correctly | M | — |
| 3.52 | `RemoteFiles.cpp` | 1203 | TODO | Handle special permission attributes (`S`, `t`) | L | — |
| 3.53 | `RemoteFiles.cpp` | 1304 | TODO | Parse timezone in listing (3 occurrences) | M | — |
| 3.54 | `RemoteFiles.cpp` | 2174 | HACK | Clear symlink change key on delete | M | — |
| 3.55 | `RemoteFiles.cpp` | 2991 | TODO | Handle `Modification` and `LastAccess` properties | M | — |
| 3.56 | `RemoteFiles.cpp` | 3049 | TODO | Handle `Modification` and `LastAccess` on save | M | — |
| 3.57 | `RemoteFiles.cpp` | 3105 | TODO | Implement `TRemoteProperties::Load` | M | — |
| 3.58 | `RemoteFiles.cpp` | 3118 | TODO | Implement `TRemoteProperties::Save` | M | — |
| 3.59 | `Queue.cpp` | 2421 | TODO | Use named constant for 5-second threshold | L | — |
| 3.60 | `Queue.cpp` | 3012 | TODO | Use `ESshFatal` instead of `EConnectionFatal` | M | — |
| 3.61 | `Configuration.cpp` | 221 | TODO | Use `TFar3Storage` instead of `TRegistryStorage` | M | — |
| 3.62 | `Configuration.cpp` | 1594 | TODO | Implement `ParamStr(0)` for MSVC | L | — |
| 3.63 | `HierarchicalStorage.cpp` | 1123 | TODO | Use `AForceAnsi` parameter | L | — |

---

## 4. FileZilla Integration (`src/filezilla/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 4.1 | `AsyncSslSocketLayer.cpp` | 1731 | TODO | Report TLS lookup error properly | M | — |
| 4.2 | `FtpControlSocket.cpp` | 4618 | HACK | Non-standard MDTM timestamp hack for IIS | L | — |

---

## 5. Windows UI (`src/windows/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 5.1 | `GUITools.cpp` | 601 | TODO | Implement `GetFileVersion()` | L | — |
| 5.2 | `GUITools.cpp` | 648 | TODO | Use `Application->ExeName` | L | — |
| 5.3 | `GUITools.cpp` | 933 | HACK | Centre-of-image color replacement for transparency | L | — |
| 5.4 | `GUITools.cpp` | 1031 | HACK | Strip consecutive tabs from translations | L | — |
| 5.5 | `GUITools.cpp` | 1100 | TODO | Implement file save dialog for MSVC | M | — |
| 5.6 | `Tools.cpp` | 44 | HACK | Undefine wininet/winhttp conflicting macros | L | — |
| 5.7 | `Tools.cpp` | 174 | HACK | `last-column-width;pixels-per-inch` format hack | L | — |
| 5.8 | `VCLCommon.cpp` | 623 | TODO | Handle minimize-to-tray | L | — |
| 5.9 | `VCLCommon.cpp` | 650 | TODO | Generalize window handling when main form hidden | M | — |
| 5.10 | `VCLCommon.cpp` | 2006 | TODO | Right-align hint window for RTL languages | L | — |
| 5.11 | `VCLCommon.cpp` | 2792 | HACK | Read `ClicksDisabled` via pointer cast | L | — |
| 5.12 | `GUIConfiguration.cpp` | 991 | FIXME | Set `HInstance` properly in `NewInstance` | L | — |
| 5.13 | `GUIConfiguration.cpp` | 1453 | HACK | Reset per-preset options via copy | L | — |
| 5.14 | `TerminalManager.cpp` | 994 | HACK | Transfer setting label timeout hack | L | — |
| 5.15 | `SynchronizeController.cpp` | 84 | FIXME | Re-enable `FSynchronizeMonitor` (3 occurrences) | M | — |
| 5.16 | `WinConfiguration.cpp` | 4077 | TODO | Implement `RecryptPasswords` for MSVC | M | — |
| 5.17 | `WinApi.h` | 81 | TODO | Verify `initialEditText` usage in list view | L | — |

---

## Statistics

| Category | TODO | FIXME | HACK | Total |
|----------|------|-------|------|-------|
| NetBox UI | 16 | 1 | 0 | 17 |
| Base / Core | 14 | 6 | 1 | 21 |
| Protocols | 43 | 1 | 2 | 46 |
| FileZilla | 1 | 0 | 1 | 2 |
| Windows UI | 6 | 2 | 6 | 14 |
| **Total** | **80** | **10** | **10** | **100** |

---

## Actionable Plans

The following existing plans already cover subsets of this inventory:

| Plan | Coverage | Items |
|------|----------|-------|
| `implement-todos.md` | `src/base/SysUtils.cpp` | 2.1, 2.2, 2.3, 2.4 (4 items) |

---

## Recommended Next Steps

1. **High Priority** — Retry/resume handling (3.30, 3.34, 3.44): User-facing reliability improvement
2. **High Priority** — Symlink/subdirectory issues (3.4, 3.28, 3.43): Data integrity risk
3. **Medium Priority** — WinSCP UI sync (1.14, 1.15): Feature parity with upstream
4. **Medium Priority** — `RecryptPasswords` (5.16): Security-related stub
5. **Low Priority** — Logging level configuration (1.19–1.21): Developer experience

---

*This inventory is a snapshot. New TODOs may be introduced; this file should be regenerated periodically (e.g., monthly) or when major refactoring occurs.*
