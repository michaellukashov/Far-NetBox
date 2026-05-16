# NetBox TODO/FIXME/HACK Inventory

> Generated: 2026-05-16
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
| 1.2 | `FarDialog.cpp` | 876 | TODO | Throw error instead of silent return in color path | M | — |
| 1.3 | `FarDialog.cpp` | 2725 | TODO | Hide cursor in `TFarLister::DoFocus()` | L | — |
| 1.4 | `FarInterface.cpp` | 40 | TODO | Output `NetBoxPluginGuid` instead of hardcoded string | L | — |
| 1.5 | `FarPlugin.cpp` | 6 | TODO | Move `TSimpleThread` to `Sysutils` (same as 1.1) | L | — |
| 1.6 | `FarPlugin.cpp` | 954 | FIXME | Re-enable `DebugAssert(FMSG_DOWN)` check | L | — |
| 1.7 | `FarPlugin.cpp` | 1762 | TODO | Report error on failed operation | M | — |
| 1.8 | `FarPlugin.cpp` | 2040 | TODO | Move cleanup code to `DestroyFarPlugin` | L | — |
| 1.9 | `FarPlugin.cpp` | 2856 | TODO | Move panel-item fetch to common function | L | — |
| 1.10 | `WinSCPDialogs.cpp` | 1999 | FIXME | Restore `Text->SetColor()` for about dialog URL | L | — |
| 1.11 | `WinSCPDialogs.cpp` | 5777 | TODO | Sync `SetFtps()` from `TLoginDialog::PortNumberEditChange` | M | — |
| 1.12 | `WinSCPDialogs.cpp` | 6417 | TODO | Check `PrivateKeyEdit`/`TunnelPrivateKeyEdit` were edited | L | — |
| 1.13 | `WinSCPDialogs.cpp` | 11324 | TODO | Check actual configuration for queue dialog size | L | — |
| 1.14 | `WinSCPFileSystem.cpp` | 1990 | TODO | Use `DontOverwrite` option in `CopyFiles()` | M | — |
| 1.15 | `WinSCPPlugin.cpp` | 105 | TODO | Read tinylog level from config file | L | — |
| 1.16 | `WinSCPPlugin.cpp` | 106 | TODO | Read log file path from config file | L | — |
| 1.17 | `WinSCPPlugin.cpp` | 111 | TODO | Enable `icecream::ic.output(logFile)` | L | — |

**Resolved since 2026-05-04:** 1.2 (`SAFE_DESTROY` already in use), 1.11 (Inform Far about unknown `OpenFrom`), 1.12 (`Options->ParseParams`), 1.15 (OpenSSH private key sync).

---

## 2. Base / Core Utilities (`src/base/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 2.1 | `Sysutils.cpp` | 68 | TODO | Use `AHelpContext` in `Exception` constructor | L | `implement-todos.md` |
| 2.2 | `Sysutils.cpp` | 1101 | FIXME | Implement `IsDriveRooted()` properly | M | `implement-todos.md` |
| 2.3 | `Sysutils.cpp` | 2337 | TODO | Throw exception on `FileWrite` error instead of returning `false` | M | `implement-todos.md` |
| 2.4 | `Classes.cpp` | 1120 | FIXME | Use `ERegistryException.CreateResFmt` for invalid reg type | L | — |
| 2.5 | `Classes.cpp` | 1808 | FIXME | Use `ERegistryException.CreateResFmt` for query failure | L | — |
| 2.6 | `Classes.cpp` | 1822 | FIXME | Use `ERegistryException.CreateResFmt` for set failure | L | — |
| 2.7 | `Classes.hpp` | 438 | TODO | Enable `ROIndexedProperty<TObject *> Objects` | L | — |
| 2.8 | `Classes.hpp` | 439 | TODO | Enable `ROIndexedProperty<UnicodeString> Names` | L | — |
| 2.9 | `Classes.hpp` | 641 | TODO | Move date/time exports to `DateUtils.hpp` | L | — |
| 2.10 | `Classes.hpp` | 1047 | FIXME | `TShortCut` class stub | L | — |
| 2.11 | `Common.cpp` | 48 | TODO | Use `Path` class instead of `UnicodeString` | M | — |
| 2.12 | `Common.cpp` | 119 | TODO | Use `UnixPathSeparator` constant | L | — |
| 2.13 | `Common.cpp` | 134 | TODO | Use `UnixPathSeparator` constant | L | — |
| 2.14 | `Common.cpp` | 1627 | TODO | Use `ExpandUNCFileName` | L | — |
| 2.15 | `Common.h` | 606 | TODO | Move `GetEnvironmentVariable` to `Sysutils.hpp` | L | — |
| 2.16 | `Global.cpp` | 9 | TODO | Remove `src/core` dependency from `Global.cpp` | L | — |
| 2.17 | `function2.hpp` | 722 | TODO | Optimize single formal argument | L | — |
| 2.18 | `function2.hpp` | 723 | TODO | Merge vtable tables if size-optimized | L | — |
| 2.19 | `function2.hpp` | 818 | TODO | Use unreachable intrinsic instead of `assert(false)` | L | — |
| 2.20 | `nbstring.h` | 26 | FIXME | `memcpy_s` implementation is unsafe | M | — |
| 2.21 | `nbstring.h` | 28 | FIXME | `_mbsstr` implementation is naive (`strstr` fallback) | L | — |
| 2.22 | `nbstring.h` | 35 | FIXME | `Langpack_GetDefaultCodePage()` assumption may be wrong | L | — |
| 2.23 | `rtti.hpp` | 32 | TODO | Rename / replace `simplify_type` cast traits | L | — |
| 2.24 | `rtti.hpp` | 54 | TODO | Add `detail` namespace once everyone switched | L | — |

**Resolved since 2026-05-04:** 2.16 (`ParentProcessName` for MSVC).

---

## 3. Protocols (`src/core/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 3.1 | `Terminal.cpp` | 1474 | TODO | Handle complex relative paths (`../../xxx`) | M | — |
| 3.2 | `Terminal.cpp` | 1623 | TODO | Warn user on certain path conditions | M | — |
| 3.3 | `Terminal.cpp` | 3241 | HACK | Enable "skip to all" for `TRetryOperationLoop` | M | — |
| 3.4 | `Terminal.cpp` | 8277 | TODO | Delete read-only directories | M | — |
| 3.5 | `Terminal.cpp` | 8278 | TODO | Show error message on delete failure | M | — |
| 3.6 | `Terminal.cpp` | 8368 | TODO | Delete read-only files | M | — |
| 3.7 | `Terminal.h` | 138 | TODO | Better user interface for file operation loops | M | — |
| 3.8 | `SecureShell.cpp` | 759 | HACK | Gross hack for server version string parsing | L | — |
| 3.9 | `SecureShell.cpp` | 1451 | TODO | Use `UTF8ToString` instead of manual conversion | L | — |
| 3.10 | `SecureShell.cpp` | 1455 | TODO | Use `AnsiToString` instead of `MB2W` | L | — |
| 3.11 | `SecureShell.cpp` | 1797 | TODO | Log input only if `LogLevel > 4` | L | — |
| 3.12 | `SecureShell.cpp` | 2035 | HACK | Call `sk_tcp_close()` to release socket memory | M | — |
| 3.13 | `SecureShell.cpp` | 2172 | TODO | Make timeout configurable | L | — |
| 3.14 | `SecureShell.cpp` | 2175 | TODO | Make event loop step configurable | L | — |
| 3.15 | `SecureShell.cpp` | 2474 | TODO | Guard `LogEvent` behind `LogLevel > 4` | L | — |
| 3.16 | `SecureShell.cpp` | 2564 | TODO | Guard timeout log behind `LogLevel > 4` | L | — |
| 3.17 | `SessionData.cpp` | 1458 | TODO | Implement unimplemented function (id 3041) | M | — |
| 3.18 | `SessionData.cpp` | 1663 | TODO | Implement `PostLoginCommands` | M | — |
| 3.19 | `SessionData.cpp` | 1723 | TODO | Map known proxy sequences to enumeration | L | — |
| 3.20 | `SessionData.cpp` | 4112 | TODO | Implement unimplemented settings export | M | — |
| 3.21 | `SessionData.cpp` | 4739 | TODO | Support `pmHTTPS` proxy method | M | — |
| 3.22 | `SessionInfo.cpp` | 957 | TODO | Log error in `TSessionLog::DoAddToSelf` | L | — |
| 3.23 | `SessionInfo.cpp` | 1204 | TODO | Implement `GetCmdLine()` | L | — |
| 3.24 | `SessionInfo.cpp` | 1955 | TODO | Implement startup info logging for MSVC | L | — |
| 3.25 | `FtpFileSystem.cpp` | 595 | TODO | Handle account parameter (rarely used) | L | — |
| 3.26 | `FtpFileSystem.cpp` | 1010 | TODO | Improve path handling (handle `..` etc.) | M | — |
| 3.27 | `FtpFileSystem.cpp` | 1319 | TODO | Parse `SITE SYMLINK target link` format | L | — |
| 3.28 | `FtpFileSystem.cpp` | 1448 | TODO | Implement retries/resume on transfer abort | L | — |
| 3.29 | `FtpFileSystem.cpp` | 3437 | TODO | Handle `REPLY_CRITICALERROR` properly | M | — |
| 3.30 | `FtpFileSystem.cpp` | 4266 | TODO | Use `Sysutils::FormatDateTime` | L | — |
| 3.31 | `ScpFileSystem.cpp` | 132 | TODO | Remove `mf`/`cd` flags (already in `TTerminal`) | L | — |
| 3.32 | `ScpFileSystem.cpp` | 1619 | TODO | Implement retries/resume on batch abort | L | — |
| 3.33 | `ScpFileSystem.cpp` | 1761 | TODO | Show stderr to user | M | — |
| 3.34 | `ScpFileSystem.cpp` | 2021 | TODO | Show stderr to user (duplicate) | M | — |
| 3.35 | `ScpFileSystem.cpp` | 2200 | TODO | Support >32-bit file sizes in ASCII mode | H | — |
| 3.36 | `ScpFileSystem.cpp` | 2354 | TODO | Send filetime with file | M | — |
| 3.37 | `ScpFileSystem.cpp` | 2410 | TODO | Delete read-only directories | M | — |
| 3.38 | `ScpFileSystem.cpp` | 2411 | TODO | Show error message on delete failure | M | — |
| 3.39 | `ScpFileSystem.cpp` | 2660 | TODO | Show stderr to user (triplicate) | M | — |
| 3.40 | `SftpFileSystem.cpp` | 3121 | TODO | Improve path handling (handle `..` etc.) | M | — |
| 3.41 | `SftpFileSystem.cpp` | 4540 | TODO | Implement retries/resume on error | L | — |
| 3.42 | `WebDAVFileSystem.cpp` | 622 | TODO | Keep WebDAV session alive (noop placeholder) | M | — |
| 3.43 | `WebDAVFileSystem.cpp` | 2378 | TODO | Implement unimplemented function | M | — |
| 3.44 | `S3FileSystem.cpp` | 505 | TODO | Port AWS config file reading to MSVC | M | — |
| 3.45 | `S3FileSystem.cpp` | 801 | TODO | Implement unimplemented S3 status handler | M | — |
| 3.46 | `RemoteFiles.cpp` | 22 | TODO | Use `Path` class instead of `UnicodeString` | M | — |
| 3.47 | `RemoteFiles.cpp` | 949 | TODO | Handle linked file attributes correctly | M | — |
| 3.48 | `RemoteFiles.cpp` | 1203 | TODO | Handle special permission attributes (`S`, `t`) | L | — |
| 3.49 | `RemoteFiles.cpp` | 1304 | TODO | Parse timezone in listing | M | — |
| 3.50 | `RemoteFiles.cpp` | 2991 | TODO | Handle `Modification` and `LastAccess` properties | M | — |
| 3.51 | `RemoteFiles.cpp` | 3049 | TODO | Handle `Modification` and `LastAccess` on save | M | — |
| 3.52 | `Queue.cpp` | 2492 | TODO | Use named constant for 5-second threshold | L | — |
| 3.53 | `Queue.cpp` | 3102 | TODO | Use `ESshFatal` instead of `EConnectionFatal` | M | — |
| 3.54 | `Configuration.cpp` | 221 | TODO | Use `TFar3Storage` instead of `TRegistryStorage` | M | — |
| 3.55 | `Configuration.cpp` | 1596 | TODO | Implement `ParamStr(0)` for MSVC | L | — |
| 3.56 | `HierarchicalStorage.cpp` | 1127 | TODO | Use `AForceAnsi` parameter | L | — |

**Resolved since 2026-05-04:**
- 3.4 (symlink resolution during subdirectory delete) — fixed in `DeleteContentsIfDirectory`; stale TODO removed from `DeleteFiles`.
- 3.9 (`tcp_keepalives`) — already aligned with WinSCP 6.5.6 / PuTTY default.
- 3.42 (turn off read-only attribute before operations).
- 3.54 (clear symlink change key on delete).

**Resolved 2026-05-16:**
- 3.42 (FileStream `unique_ptr`) — converted `TStream *` to `std::unique_ptr<TStream>` in `TSFTPFileSystem::SFTPDownloadFile`.
- `Terminal.cpp:1190` (CustomCopyParam raw `new`) — wrapped in `std::unique_ptr<TCopyParamType>` with `.release()` to caller.
- `Terminal.cpp:3884` (bare `catch(...)`) — changed to `catch (Exception &)` for typed exception handling.
- `WinSCPDialogs.cpp` (duplicate password-match) — extracted `ValidatePasswordsMatch()` helper.

---

## 4. FileZilla Integration (`src/filezilla/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 4.1 | `AsyncSslSocketLayer.cpp` | 1731 | TODO | Report TLS lookup error properly | M | — |

**Resolved since 2026-05-04:** 4.2 (non-standard MDTM timestamp hack for IIS).

---

## 5. Windows UI (`src/windows/`)

| # | File | Line | Type | Summary | Priority | Plan |
|---|------|------|------|---------|----------|------|
| 5.1 | `GUITools.cpp` | 630 | TODO | Implement `GetFileVersion()` | L | — |
| 5.2 | `GUITools.cpp` | 677 | TODO | Use `Application->ExeName` | L | — |
| 5.3 | `GUITools.cpp` | 962 | HACK | Centre-of-image color replacement for transparency | L | — |
| 5.4 | `Tools.cpp` | 1101 | TODO | Implement file save dialog for MSVC | M | — |
| 5.5 | `VCLCommon.cpp` | 623 | TODO | Handle minimize-to-tray | L | — |
| 5.6 | `VCLCommon.cpp` | 650 | TODO | Generalize window handling when main form hidden | M | — |
| 5.7 | `VCLCommon.cpp` | 2012 | TODO | Right-align hint window for RTL languages | L | — |
| 5.8 | `VCLCommon.cpp` | 2798 | HACK | Read `ClicksDisabled` via pointer cast | L | — |
| 5.9 | `GUIConfiguration.cpp` | 991 | FIXME | Set `HInstance` properly in `NewInstance` | L | — |
| 5.10 | `WinApi.h` | 81 | TODO | Verify `initialEditText` usage in list view | L | — |

**Resolved since 2026-05-04:**
- 5.11 (strip consecutive tabs from translations).
- 5.12 (undefine wininet/winhttp conflicting macros).
- 5.13 (`last-column-width;pixels-per-inch` format hack).
- 5.14 (reset per-preset options).
- 5.15 (transfer setting label timeout hack).
- 5.16 (re-enable `FSynchronizeMonitor`).
- 5.17 (implement `RecryptPasswords` for MSVC — fully implemented).

---

## Statistics

| Category | TODO | FIXME | HACK | Total |
|----------|------|-------|------|-------|
| NetBox UI | 14 | 1 | 0 | 15 |
| Base / Core | 17 | 7 | 0 | 24 |
| Protocols | 45 | 0 | 2 | 47 |
| FileZilla | 1 | 0 | 0 | 1 |
| Windows UI | 6 | 1 | 2 | 9 |
| **Total** | **83** | **9** | **4** | **96** |

*(Previous total: 100 — 3 items resolved / stale TODO removed, net reduction of 3. Additional 4 items resolved 2026-05-16.)*

---

## Actionable Plans

The following existing plans already cover subsets of this inventory:

| Plan | Coverage | Items |
|------|----------|-------|
| `implement-todos.md` | `src/base/SysUtils.cpp` | 2.1, 2.2, 2.3 (3 items) |

---

## Recommended Next Steps

1. **High Priority** — Symlink/subdirectory issues (3.26, 3.40): Path handling for `..` in FTP/SFTP `AbsolutePath`
2. **High Priority** — Support >32-bit file sizes in ASCII mode (3.35): Data integrity risk for SCP
3. **Medium Priority** — WinSCP UI sync (1.11): Feature parity with upstream
4. **Medium Priority** — `ESshFatal` migration (3.54): Error-code correctness in queue
5. **Low Priority** — Checksum retry/resume (3.28, 3.32, 3.41): `TRetryOperationLoop` already handles retry UI; "resume" doesn't apply to checksums
6. **Low Priority** — Logging level configuration (1.15–1.17): Developer experience

---

*This inventory is a snapshot. New TODOs may be introduced; this file should be regenerated periodically (e.g., monthly) or when major refactoring occurs.*
