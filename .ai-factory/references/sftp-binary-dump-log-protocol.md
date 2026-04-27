# SFTP Binary Data Dump and Log Protocol Level Exploration

**Created:** 2026-04-27
**Source:** Plan: SFTP Binary Data Dump at Log Protocol Level 3

## Overview

Research into how SFTP protocol packet binary data is dumped in logs, and how the Log Protocol configuration levels work.

## Key Findings

### Current Binary Dump Behavior

**File:** `src/core/SftpFileSystem.cpp`

**Function:** `TSFTPFileSystem::LogPacket` (lines ~2547-2576)

```cpp
FTerminal->Log->Add(
  Type, FORMAT(L"Type: %s, Size: %d, Number: %d", Packet->TypeName(), nb::ToInt32(Packet->GetLength()), Packet->GetMessageNumber()));
if (FTerminal->Configuration->ActualLogProtocol >= 2)
{
  FTerminal->Log->Add(Type, Packet->Dump());
}
```

**Binary Dump Output Example:**
```
2026-04-26 18:17:55.620 < Type: SSH_FXP_VERSION, Size: 318, Number: -1
2026-04-26 18:17:55.620 < 02,00,00,00,03,00,00,00,18,70,6F,73,69,78,2D,72,65,6E,61,6D,65,40,6F,70,65,
```

### TSFTPPacket::Dump() Implementation

**File:** `src/core/SftpFileSystem.cpp` (lines ~975-990)

```cpp
UnicodeString Dump() const
{
  UnicodeString Result;
  for (uint32_t Index = 0; Index < GetLength(); ++Index)
  {
    Result += ByteToHex(GetData()[Index]) + L",";
    if (((Index + 1) % 25) == 0)
    {
      Result += L"\n";
    }
  }
  return Result;
}
```

### Log Protocol Configuration

**Configuration Class:** `TConfiguration` (`src/core/Configuration.h`)

| Field | Type | Default |
|-------|------|---------|
| `FLogProtocol` | `int32_t` | 0 |
| `FPermanentLogProtocol` | `int32_t` | 0 |
| `ActualLogProtocol` | computed | based on `FLogProtocol` |

**Method:** `GetActualLogProtocol()` returns the effective log protocol level.

### Log Protocol UI Levels

**Settings Dialog:** `src/NetBox/WinSCPDialogs.cpp` (lines ~500-510)

```cpp
for (int32_t Index = 0; Index <= 2; ++Index)
{
  LogProtocolCombo->GetItems()->Add(GetMsg(NB_LOGGING_LOG_PROTOCOL_0 + Index));
}
```

**Current Levels (0-2):**
- 0: Normal (no packet logging)
- 1: Debug 1 (packet headers only)
- 2: Debug 2 (packet headers + binary data)

**Message IDs:** `src/base/MsgIDs.h` (lines ~508-512)
- `NB_LOGGING_LOG_PROTOCOL`
- `NB_LOGGING_LOG_PROTOCOL_0` → "Normal"
- `NB_LOGGING_LOG_PROTOCOL_1` → "Debug 1"
- `NB_LOGGING_LOG_PROTOCOL_2` → "Debug 2"

### Session Info Display

**File:** `src/core/SessionInfo.cpp` (lines ~1230-1246)

```cpp
if (AConfiguration->LogProtocol <= -1)
  LogStr = L"Reduced";
else if (AConfiguration->LogProtocol <= 0)
  LogStr = L"Normal";
else if (AConfiguration->LogProtocol == 1)
  LogStr = L"Debug 1";
else if (AConfiguration->LogProtocol >= 2)
  LogStr = L"Debug 2";
```

### Other Protocols' Binary Dump Usage

| Protocol | File | Condition | What Dumps |
|----------|------|-----------|------------|
| **SFTP** | `SftpFileSystem.cpp:2572` | `>= 2` | Binary packet data |
| **FTP** | `FtpFileSystem.cpp:1887, 2319` | `>= 2` | Modification time enrichment |
| **WebDAV** | `NeonIntf.cpp:451` | `>= 2` | XML parsing details |
| **SCP** | `ScpFileSystem.cpp:2204` | `>= 1` | Binary data (blocks) |
| **S3** | `S3FileSystem.cpp:864` | `>= 0` | Response data |
| **SSH** | `SecureShell.cpp:1869+` | `>= 2` | Network events |

### Proposed Change

Add Log Protocol Level 3 to trigger binary dump for SFTP:

| Level | Label | SFTP Binary Dump |
|-------|-------|------------------|
| 0 | Normal | No |
| 1 | Debug 1 | No |
| 2 | Debug 2 | No |
| 3 | Debug 3 | **Yes** |

## Files to Modify

1. `src/base/MsgIDs.h` — Add `NB_LOGGING_LOG_PROTOCOL_3`
2. `src/NetBox/NetBoxEng.lng` — Add English string
3. `src/NetBox/NetBoxRus.lng` — Add Russian string
4. `src/NetBox/NetBoxPol.lng` — Add Polish string
5. `src/NetBox/NetBoxSpa.lng` — Add Spanish string
6. `src/NetBox/WinSCPDialogs.cpp` — Change loop to `Index <= 3`
7. `src/core/SftpFileSystem.cpp` — Change `>= 2` to `>= 3`
8. `src/core/SessionInfo.cpp` — Add "Debug 3" case

## References

- WinSCP similar functionality (reference from SessionInfo display logic)
- AGENTS.md — Project build configuration (`OPT_CREATE_PLUGIN_DIR`)
- Language file format: `KEY=VALUE` per line