# Issue #501: SSH/SCP Buffer Corruption and Slow Transfer Speed

## Problem

Copying files via SSH/SCP in NetBox results in:

1. Very slow transfer speeds (~6 MB/s on gigabit networks)
2. Corrupted files (3000+ bad blocks in a 3 GB 7z archive)
3. 100% CPU usage on one core by the ssh process on the source machine

The corruption is reproducible approximately every second attempt.

## Root Cause Analysis

The "Optimize connection buffer size" checkbox (`SshBufferSizeCheck`) is enabled by default in NetBox session settings. This checkbox controls two session parameters:

| Parameter | Checked | Unchecked |
|-----------|---------|-----------|
| `SendBuf` | `DefaultSendBuf` (262144) | `0` |
| `SshSimple` | `true` | `false` |

### Dynamic Buffer Resizing Mechanism

When `FSendBuf > 0` (set from `SessionData->GetSendBuf()` in `TSecureShell::Open()`), the `EventSelectLoop()` in `src/core/SecureShell.cpp` periodically queries the ideal TCP send backlog via `WSAIoctl(SIO_IDEAL_SEND_BACKLOG_QUERY)` and increases `SO_SNDBUF` if the queried value exceeds the current buffer size.

```cpp
// src/core/SecureShell.cpp:2459-2475
if ((FSocket != INVALID_SOCKET) &&
    (FSendBuf > 0) && (TicksAfter - FLastSendBufferUpdate >= 1000))
{
    DWORD BufferLen = 0;
    DWORD OutBuffLen = 0;
    if (::WSAIoctl(FSocket, SIO_IDEAL_SEND_BACKLOG_QUERY, nullptr, 0,
                   &BufferLen, sizeof(BufferLen), &OutBuffLen, nullptr, nullptr) == 0)
    {
        if (FSendBuf < nb::ToInt32(BufferLen))
        {
            FSendBuf = nb::ToInt32(BufferLen);
            setsockopt(FSocket, SOL_SOCKET, SO_SNDBUF,
                       reinterpret_cast<const char *>(&BufferLen), sizeof(BufferLen));
        }
    }
}
```

### Why This Causes Corruption

The dynamic resizing of `SO_SNDBUF` during active SCP transfers interacts poorly with:

1. PuTTY's internal SSH channel buffering (`Pending`, `OutPtr`, `PendLen` in `TSecureShell`)
2. Windows TCP stack behavior when send buffers are resized mid-transfer
3. Potential race conditions between buffer growth and data dispatch

When the checkbox is unchecked (`SendBuf = 0`), the `FSendBuf > 0` guard skips the entire `WSAIoctl` block, eliminating both the corruption and the CPU overhead.

### Speed Impact

Contrary to its name, the "optimization" significantly reduces throughput:

- **Enabled**: ~6 MB/s, one CPU core at 100%
- **Disabled**: ~30 MB/s, normal CPU usage

The CPU saturation suggests the `WSAIoctl` polling (every 1000ms) combined with buffer resizing triggers excessive TCP stack churn or kernel/user mode transitions.

## Files Involved

| File | Role |
|------|------|
| `src/core/SessionData.cpp` | Constructor initializes `SendBuf` to `DefaultSendBuf` |
| `src/core/SessionData.h` | Defines `DefaultSendBuf = 256 * 1024` |
| `src/core/SecureShell.cpp` | `EventSelectLoop()` contains the dynamic buffer resize logic |
| `src/NetBox/WinSCPDialogs.cpp` | `SshBufferSizeCheck` UI control couples `SendBuf` and `SshSimple` |
| `src/core/SessionInfo.cpp` | Logs `SendBuf` value in session info |

## Proposed Fix

1. **Change the default**: Initialize `SendBuf` to `0` in `TSessionData` constructor. This disables the dynamic buffer optimization by default for new sessions while preserving `SshSimple = true` (required for SCP channel operation).

2. **Add logging**: Enhance `EventSelectLoop()` to log buffer resize operations at verbose level for diagnostic purposes.

3. **Document the coupling**: Add a code comment in `WinSCPDialogs.cpp` explaining that the checkbox controls both settings and referencing issue #501.

## References

- GitHub Issue: [#501](https://github.com/michaellukashov/Far-NetBox/issues/501)
- Related NetBox message: `NetBoxRus.lng:1104` — timeout hint already suggests disabling this setting

