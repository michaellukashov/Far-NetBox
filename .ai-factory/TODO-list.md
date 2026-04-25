# TODO/FIXME List — src/base & src/core

> Generated 2026-04-25 | **66 total items** (16 base + 50 core)

---

## src/base (16 items)

### Feature (3)
| # | File | Line | Description |
|---|------|------|-------------|
| 1 | `Sysutils.cpp` | 1024 | `ExtractShortPathName()` is a stub — returns input path unchanged; should call `GetShortPathNameW` |
| 2 | `Classes.hpp` | 439 | Uncomment `ROIndexedProperty<TObject *> Objects` once template supports raw pointer types |
| 3 | `Classes.hpp` | 440 | Uncomment `ROIndexedProperty<UnicodeString> Names` once template supports it |

### Cleanup (10)
| # | File | Line | Description |
|---|------|------|-------------|
| 4 | `Sysutils.cpp` | 66 | `Exception` constructor accepts `AHelpContext` but casts to void — wire up `HelpContext` |
| 5 | `Global.cpp` | 9 | Debug build `#include <Interface.h>` pulls in src/core dep, violating layer separation |
| 6 | `Common.h` | 606 | `GetEnvironmentVariable()` declared here but belongs in `Sysutils.hpp` |
| 7 | `Common.cpp` | 119 | `UnixExtractFileDir()` hard-codes `L'/'` instead of `UnixPathSeparator` |
| 8 | `Common.cpp` | 134 | `UnixExtractFilePath()` hard-codes `L'/'` instead of `UnixPathSeparator` |
| 9 | `Classes.hpp` | 632 | `Now()`, `SpanOfNowAndThen()`, `MilliSecondSpan()` belong in `DateUtils.hpp` |
| 10 | `Classes.hpp` | 1038 | Bare `// FIXME` on `TShortCut` — needs investigation |
| 11 | `Classes.cpp` | 1116 | `ReadError()` throws generic `Exception` instead of `ERegistryException` |
| 12 | `Classes.cpp` | 1804 | `TRegistry` read throws generic `Exception` instead of `ERegistryException` |
| 13 | `Classes.cpp` | 1818 | `TRegistry` write throws generic `Exception` instead of `ERegistryException` |

### Refactor (2)
| # | File | Line | Description |
|---|------|------|-------------|
| 14 | `Common.cpp` | 48 | Create dedicated `Path` class instead of raw `UnicodeString` (handle relativity, normalization) |
| 15 | `Sysutils.cpp` | 2260 | File-write failure returns `false` — should throw exception for consistency |

### Bugfix (1)
| # | File | Line | Description |
|---|------|------|-------------|
| 16 | `Common.cpp` | 1627 | `SamePaths()` doesn't normalize UNC paths before comparison — false mismatches possible |

---

## src/core (50 items)

### Feature (20)
| # | File | Line | Description |
|---|------|------|-------------|
| 1 | `WebDAVFileSystem.cpp` | 615 | Session keep-alive (`Idle()`) is a no-op |
| 2 | `WebDAVFileSystem.cpp` | 2371 | `FileTransferProgress` callback is a stub |
| 3 | `Configuration.cpp` | 1594 | `ProgramPath` hardcoded to `""`; should use `ParamStr(0)` |
| 4 | `SessionData.cpp` | 1454 | `SaveToOptions()` throws `ThrowNotImplemented` |
| 5 | `SessionData.cpp` | 1719 | FTP proxy logon type 4 — map known sequences to enum |
| 6 | `SessionData.cpp` | 4097 | Raw settings handling in session params is not implemented |
| 7 | `SessionData.cpp` | 4734 | HTTPS proxy scheme handler stubbed to `pmHTTP` instead of proper `pmHTTPS` |
| 8 | `S3FileSystem.cpp` | 757 | Per-session custom CA cert support needs temp file handling |
| 9 | `S3FileSystem.cpp` | 795 | `LibS3ResponsePropertiesCallback` is a stub returning `S3StatusOK` |
| 10 | `Terminal.h` | 138 | `FILE_OPERATION_LOOP_BEGIN` macro needs better user query UI |
| 11 | `Terminal.cpp` | 8039 | Read-only directories skipped during deletion — should delete them |
| 12 | `Terminal.cpp` | 8127 | Read-only files skipped during local source deletion |
| 13 | `SftpFileSystem.cpp` | 4518 | Checksum mismatch — no retry/resume logic |
| 14 | `SessionInfo.cpp` | 1169 | `GetCmdLineLog()` cannot retrieve command line |
| 15 | `SessionInfo.cpp` | 1916 | `LogStartupInfo()` is a stub (BorlandC only) |
| 16 | `ScpFileSystem.cpp` | 1593 | Checksum error — no retry/resume logic |
| 17 | `ScpFileSystem.cpp` | 2323 | File modification timestamps not sent in SCP directory listings |
| 18 | `ScpFileSystem.cpp` | 2379 | Read-only directories skipped during SCP recursive deletion |
| 19 | `RemoteFiles.cpp` | 1304/1318/1363 | Timezone offset in file timestamps is skipped/ignored (3 locations) |
| 20 | `RemoteFiles.cpp` | 2991/3049 | `CommonProperties()` and property change detection ignore Modification/LastAccess |
| 21 | `RemoteFiles.cpp` | 3105/3118 | `TRemoteProperties::Load()`/`Save()` incomplete (only Rights serialized) |
| 22 | `FtpFileSystem.cpp` | 1431 | Checksum error — no retry/resume logic |

### Bugfix (6)
| # | File | Line | Description |
|---|------|------|-------------|
| 23 | `Terminal.cpp` | 1447 | Path normalization doesn't handle `../../xxx` components |
| 24 | `Terminal.cpp` | 4568 | Symlink resolution during recursive dir reads is broken for relative symlinks |
| 25 | `SftpFileSystem.cpp` | 3127 | `LocalCanonify()` doesn't handle `..` path components |
| 26 | `ScpFileSystem.cpp` | 2174 | ASCII mode truncates files >4GB (32-bit size limit) |
| 27 | `ScpFileSystem.cpp` | 2794 | Read-only attribute not cleared before overwriting local files |
| 28 | `RemoteFiles.cpp` | 949 | Symlink attributes: should they come from link or target? |
| 29 | `FtpFileSystem.cpp` | 993 | `AbsolutePath()` doesn't resolve `..` components |
| 30 | `FtpFileSystem.cpp` | 1302 | ProFTPD `SITE SYMLINK` double-quote parsing unknown — needs verification |

### Error-Handling (6)
| # | File | Line | Description |
|---|------|------|-------------|
| 31 | `Terminal.cpp` | 1588 | No warning when file system creation fails (FFileSystem == nullptr) |
| 32 | `Terminal.cpp` | 8040 | No error message on directory deletion failure |
| 33 | `ScpFileSystem.cpp` | 1735 | STDERR from SCP is discarded — show to user? |
| 34 | `ScpFileSystem.cpp` | 1995 | STDERR discarded (duplicate, different code path) |
| 35 | `ScpFileSystem.cpp` | 2619 | STDERR cleared without display after remote SCP abort |
| 36 | `FtpFileSystem.cpp` | 3391 | `REPLY_CRITICALERROR` silently ignored in reply handler |

### Refactor (6)
| # | File | Line | Description |
|---|------|------|-------------|
| 37 | `SecureShell.cpp` | 1315 | Replace inline UTF8→Wide conversion with `UTF8ToString` utility |
| 38 | `SecureShell.cpp` | 1320 | Replace inline MB→Wide conversion with `AnsiToString` utility |
| 39 | `Configuration.cpp` | 221 | Admin storage uses `TRegistryStorage` directly; should use `TFar3Storage` |
| 40 | `SftpFileSystem.cpp` | 5704 | `TStream *` raw owning pointer → `std::unique_ptr<TStream>` |
| 41 | `RemoteFiles.cpp` | 22 | Create dedicated `Path` class instead of raw `UnicodeString` |
| 42 | `Queue.cpp` | 2959 | `EConnectionFatal(nullptr, "")` should use more specific `ESshFatal` |
| 43 | `FtpFileSystem.cpp` | 4220 | Use `Sysutils::FormatDateTime` instead of inline date formatting |

### Config (4)
| # | File | Line | Description |
|---|------|------|-------------|
| 44 | `SecureShell.cpp` | 424 | TCP keepalive enabled by default — should it be configurable/false? |
| 45 | `SecureShell.cpp` | 2037 | Hardcoded 500ms EOF timeout → config parameter |
| 46 | `SecureShell.cpp` | 2040 | Hardcoded 100ms event-loop step → parameter |
| 47 | `Queue.cpp` | 2379 | Hardcoded 5-second parallel operation threshold → named constant |

### Cleanup (4)
| # | File | Line | Description |
|---|------|------|-------------|
| 48 | `ScpFileSystem.cpp` | 131 | Remove redundant `mf`/`cd` command-set entries (now handled by `TTerminal`) |
| 49 | `RemoteFiles.cpp` | 1203 | Re-evaluate special permission attributes once properly handled |
| 50 | `HierarchicalStorage.cpp` | 1124 | `AForceAnsi` parameter accepted but ignored in `DoValueExists` |
| 51 | `FtpFileSystem.cpp` | 578 | Investigate if FTP "account" field is ever used |

### Logging (4)
| # | File | Line | Description |
|---|------|------|-------------|
| 52 | `SecureShell.cpp` | 1662 | Guard `FLog->Add(llInput)` with `if LogLevel > 4` |
| 53 | `SecureShell.cpp` | 2329 | Guard "Looking for network events" with `if LogLevel > 4` |
| 54 | `SecureShell.cpp` | 2419 | Guard "Timeout waiting for network events" with `if LogLevel > 4` |
| 55 | `SessionInfo.cpp` | 922 | Catch-all in `DoAddToSelf` swallows exceptions without logging |

---

## Summary

| Area | Feature | Bugfix | Error-Handling | Refactor | Config | Cleanup | Logging | Total |
|------|---------|--------|----------------|----------|--------|---------|---------|-------|
| src/base | 3 | 1 | 0 | 2 | 0 | 10 | 0 | **16** |
| src/core | 20 | 6 | 6 | 6 | 4 | 4 | 4 | **50** |
| **Total** | **23** | **7** | **6** | **8** | **4** | **14** | **4** | **66** |

## Quick Wins (Low effort, high value)
1. `Common.cpp:119,134` — Replace `L'/'` with `UnixPathSeparator` (2 trivial one-liners)
2. `Classes.cpp:1116,1804,1818` — Use `ERegistryException` instead of generic `Exception`
3. `SecureShell.cpp:1662,2329,2419` — Add log-level guards
4. `Queue.cpp:2379` — Extract magic number 5000 into a named constant
5. `HierarchicalStorage.cpp:1124` — Use or remove `AForceAnsi` parameter
6. `SftpFileSystem.cpp:5704` — Replace `TStream*` with `std::unique_ptr<TStream>`

## Priority Areas
- **Path normalization** (`Terminal.cpp:1447`, `SftpFileSystem.cpp:3127`, `FtpFileSystem.cpp:993`) — affects all file operations
- **Retry/resume on checksum error** (`SftpFileSystem.cpp:4518`, `ScpFileSystem.cpp:1593`, `FtpFileSystem.cpp:1431`) — reliability during transfers
- **Read-only file handling** (`Terminal.cpp:8039,8127`, `ScpFileSystem.cpp:2379,2794`) — improves robustness
- **Error feedback to user** (`Terminal.cpp:1588,8040`, `ScpFileSystem.cpp:1735,1995,2619`) — UX improvements
