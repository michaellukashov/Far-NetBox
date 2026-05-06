[← Architecture](docs/ARCHITECTURE.md) · [Back to README](docs/README.md) · [Developer Guide →](docs/DEVELOPER.md)

# Logging Subsystem

Internal reference for how NetBox's logging architecture works — tinylog library internals, initialization paths, log file locations, and configuration options.

## Overview

NetBox uses the **tinylog** logging library (`libs/tinylog/`) for all logging. Tinylog is an **asynchronous** logger: log calls write to an in-memory double-buffer under a mutex, while a dedicated background worker thread drains buffers to disk via `fwrite`.

## Tinylog Internals

### Core Classes

| Class | File | Role |
|-------|------|------|
| `TinyLog` | `libs/tinylog/tinylog/TinyLog.h` | Public API, holds `TinyLogImpl` (pimpl) |
| `TinyLogImpl` | `libs/tinylog/src/TinyLog.cpp` | Singleton manager, worker thread loop |
| `LogStream` | `libs/tinylog/src/LogStream.cpp` | Log formatting (timestamp + prefix), double-buffered write |
| `Buffer` | `libs/tinylog/src/Buffer.cpp` | In-memory append buffer, `Flush()` to `fwrite` + `fflush` |

### Double-Buffering

Two `Buffer` instances (`front_buff_`, `back_buff_`):
- Producers write to the current front buffer
- When front fills up, buffers are **swapped** — the background thread drains the old front to disk while producers write to the new front
- Buffer swap is triggered via `drain_buffer_` atomic signal in `TinyLog.cpp:135`

### Thread Model

- **Producer threads:** Any number — write to in-memory buffer via `InternalWrite()` (`LogStream.cpp:175`)
- **Background thread:** One — `TinyLogImpl::Run()`, drains buffers, sleeps 3 seconds between drains (`TIME_OUT_SECOND` in `Config.h:5`)
- **Mutex:** `pthread_mutex_t` guards the double-buffer append
- **Condition variable:** `pthread_cond_t` signals the background thread when buffers need draining

### Constants

| Constant | Value | File |
|----------|-------|------|
| `TIME_OUT_SECOND` | 3 | `libs/tinylog/tinylog/Config.h:5` |
| `LOG_BUFFER_SIZE` | 16 KB | `libs/tinylog/tinylog/Config.h:6` |
| `LOG_PATH` | `"./test.log"` | `libs/tinylog/tinylog/Config.h:7` (standalone only) |

## Three Tinylog Instances in NetBox

NetBox uses **three separate** `TinyLog` instances for different log categories:

| Instance | Scope | Lifetime | Type |
|----------|-------|----------|------|
| `g_tinylog` | Plugin debug trace | Plugin load → unload | `TinyLog *` (raw, singleton) |
| `TSessionLog::FLogger` | Per-session protocol log | Session open → close | `unique_ptr<TinyLog>` |
| `TActionLog::FLogger` | Per-session action log | Session open → close | `unique_ptr<TinyLog>` |

### 1. `g_tinylog` — Global Debug Log

- **Initialization:** `src/NetBox/WinSCPPlugin.cpp:63-77` (TWinSCPPlugin constructor, `#ifdef DEBUG` only)
- **Log level:** Always `LEVEL_TRACE`
- **File open:** `_wfsopen()` via `base::LocalOpenFileForWriting()` at `src/base/Sysutils.cpp:2223`
- **Default path:** `%TEMP%\netbox-dbglog.txt`
- **Path source:** Hardcoded at `src/NetBox/WinSCPPlugin.cpp:70`
- **Shutdown:** `src/NetBox/WinSCPPlugin.cpp:89-91` — `g_tinylog->Close()` then `delete`

### 2. `TSessionLog::FLogger` — Session Protocol Log

- **Initialization:** `src/core/SessionInfo.cpp:1082-1103` (`TSessionLog::OpenLogFile()`)
- **Log level:** Controlled by user setting `LogProtocol`
- **File open:** Static `::OpenLogFile()` at `src/core/SessionInfo.cpp:814` via `_wfsopen()`
- **Default path:** `%TEMP%\&S.log` (from `TConfiguration::GetDefaultLogFileName()` at `src/core/Configuration.cpp:2449`)
- **Path configurable:** Yes — user sets via settings UI (`src/NetBox/WinSCPDialogs.cpp:547`)
- **Append mode:** Configurable via `GetLogFileAppend()` (`src/core/Configuration.cpp:279`)

### 3. `TActionLog::FLogger` — Action Log

- **Initialization:** `src/core/SessionInfo.cpp:1792-1804` (`TActionLog::OpenLogFile()`)
- **Log format:** XML
- **Default path:** `%TEMP%\&S.xml` (from `src/core/Configuration.cpp:292-293`)
- **Path configurable:** Yes — `SetActionsLogFileName()` at `src/core/Configuration.cpp:2338`

## Log File Path Expansion

The `&S` placeholder (and others) in log file paths is expanded by `GetExpandedLogFileName()` at `src/core/SessionData.cpp:6507`:

| Placeholder | Substitution |
|-------------|-------------|
| `&S` | Session name |
| `&Y` | Year |
| `&M` | Month |
| `&D` | Day |
| `&T` | Time |
| `&P` | Process ID |
| `&@` | Host name |

## Log File I/O

All log file opens use:
- `_wfsopen()` with `SH_DENYWR` sharing mode — prevents other processes from writing while NetBox writes
- `setvbuf` configured for unbuffered I/O with a 4 KB buffer

File handles are **owned by tinylog** (not the caller):
- `LogStream::~LogStream()` calls `fclose(file_)` at `libs/tinylog/src/LogStream.cpp:40`
## Crash-Resilient Logging

To reduce log data loss when NetBox crashes during protocol operations, an **emergency flush mechanism** is installed via an unhandled exception filter.

### Architecture

```
SEH Unhandled Exception
       |
       v
NetBoxExceptionFilter()          [src/NetBox/WinSCPPlugin.cpp]
       |
       +-- Flush all TinyLog instances via EmergencyFlushAll(200)
       |      |
       |      +-- Per-instance: LogStream::EmergencyFlush()
       |      |   - TryEnterCriticalSection (non-blocking)
       |      |   - If acquired: WriteBuffer() + optional SwapBuffer() + WriteBuffer()
       |      |   - Clear drain_buffer_ flag
       |      |
       |      +-- Signal worker thread exit (run_.store(false), cond_signal)
       |      +-- WaitForSingleObject(thrd_, 50ms) per instance
       |
       +-- Flush TApplicationLog via TryEnter + fflush
       |
       +-- Chain to previous filter (SetUnhandledExceptionFilter return value)
```

### Exception Filter Safety

The filter runs in a crash context where the heap may be corrupt and locks may be held:

| Safety Measure | Rationale |
|---|---|
| `__try` / `__except(EXCEPTION_EXECUTE_HANDLER)` | If the flush itself crashes, silently chain to the previous filter |
| `OutputDebugStringA` only (stack buffer) | No heap allocation; `AppLogFmt` is banned (uses heap + critical section + fwrite) |
| `pthread_mutex_tryenter` / `TryEnterCriticalSection` | Non-blocking; if the crashing thread holds the mutex, shared buffers are skipped |
| `WaitForSingleObject` with 50ms timeout | Do not block process termination; timeout is acceptable loss |
| `std::atomic<bool> run_` with release/acquire | Prevents data race between crash handler (writer) and worker thread (reader) |

### Loss Windows

| Layer | Data | Flush Interval | Loss on Crash |
|---|---|---|---|
| **Layer 1** | TLS staging buffer (`tls_buffer_`) | Every `Write()` call (implicit) | Up to 4KB of uncommitted log text from the crashing thread |
| **Layer 2** | Shared front/back buffer | Background thread ~1s timeout + explicit `SwapBuffer()` | Up to 2x buffer capacity (~64KB default) if mutex is busy |
| **Layer 3** | OS file cache / disk | `fwrite` is buffered by CRT; `fflush` on emergency flush | Up to 4KB CRT stream buffer if `fflush` is skipped due to busy mutex |

### TApplicationLog

`TApplicationLog` (`src/core/SessionInfo.cpp`) uses **synchronous** `fwrite` under `TCriticalSection`. Completed writes are already on disk (modulo the CRT 4KB stream buffer). Emergency flush attempts `TCriticalSection::TryEnter()` + `fflush`; if the crashing thread holds the critical section, only the CRT stream buffer is at risk.

### Platform Notes

On Win32, the bundled pthreads compatibility layer defines `typedef HANDLE pthread_t` (`libs/tinylog/tinylog/platform_win32.h:43`). Casting `pthread_t` to `HANDLE` for `WaitForSingleObject` is therefore safe.

## Configuration Options

| Option | Default | Configurable | Source |
|--------|---------|-------------|--------|
| Global debug log path | `%TEMP%\netbox-dbglog.txt` | Code only | `src/NetBox/WinSCPPlugin.cpp:70` |
| Session log path | `%TEMP%\&S.log` | User UI | `src/core/Configuration.cpp:277,2449` |
| Action log path | `%TEMP%\&S.xml` | User UI | `src/core/Configuration.cpp:292` |
| Session log level | `LogProtocol` setting | User UI | `src/core/Configuration.cpp:2372` |
| Global debug log level | `LEVEL_TRACE` | Code only | `src/NetBox/WinSCPPlugin.cpp:69` |
| Append mode (session) | `true` | User UI | `src/core/Configuration.cpp:279` |
| Buffer size | 16 KB | Compile-time | `libs/tinylog/tinylog/Config.h:6` |
| Flush timeout | 3 sec | Compile-time | `libs/tinylog/tinylog/Config.h:5` |

## Key Source Files

| File | Role |
|------|------|
| `libs/tinylog/tinylog/TinyLog.h` | Public API, `g_tinylog` macro (L104), `TINYLOG_*` macros (L106-122) |
| `libs/tinylog/src/TinyLog.cpp` | `TinyLogImpl` (pimpl), worker thread, buffer drain loop (L10-156), singleton (L158-174) |
| `libs/tinylog/tinylog/Config.h` | Constants: timeout, buffer size, default path |
| `libs/tinylog/src/LogStream.cpp` | Log formatting, double-buffered write, `fclose` in destructor (L40) |
| `libs/tinylog/src/Buffer.cpp` | Append buffer, `fwrite` + `fflush` in `Flush()` (L55-78) |
| `src/NetBox/WinSCPPlugin.cpp` | Global `g_tinylog` init (L63-77) and shutdown (L89-91) |
| `src/base/Sysutils.cpp` | `LocalOpenFileForWriting()` — `_wfsopen()` for debug log (L2223-2233) |
| `src/core/SessionInfo.cpp` | `OpenLogFile()` static (L814-830), `TSessionLog::OpenLogFile()` (L1082-1103), `TActionLog::OpenLogFile()` (L1792-1804) |
| `src/core/Configuration.cpp` | Default log paths and getters/setters (L2321-2452) |
| `src/core/SessionData.cpp` | `GetExpandedLogFileName()` path expansion (L6507-6549) |
| `src/base/Global.h` | `#include <tinylog/TinyLog.h>` — makes tinylog available project-wide |

## See Also

- [Architecture](docs/ARCHITECTURE.md) — project structure and dependency flow
- [Developer Guide](docs/DEVELOPER.md) — build instructions and development setup
- [Dependencies](docs/DEPENDENCIES.md) — third-party library inventory
