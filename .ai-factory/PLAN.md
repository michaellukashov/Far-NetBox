# Plan - SFTP Binary Data Dump at Log Protocol Level 3

## Settings

- **Testing:** No
- **Logging:** Verbose (recommended for AI-generated code)
- **Docs:** No mandatory checkpoint (warn-only)
- **Roadmap:** Version 1.3

## Exploration

- [SFTP Binary Dump Log Protocol](.ai-factory/references/sftp-binary-dump-log-protocol.md)

### Exploration Summary

**Key Findings:**

| File | Current State | Required Change |
|------|--------------|-----------------|
| `src/base/MsgIDs.h` | Has `NB_LOGGING_LOG_PROTOCOL_0,1,2` | Add `_3` |
| `src/NetBox/NetBoxEng.lng` | Has 3 levels | Add level 3 |
| `src/NetBox/WinSCPDialogs.cpp:505` | Loop `Index <= 2` | Change to `<= 3` |
| `src/core/SftpFileSystem.cpp:2572` | `>= 2` triggers dump | Change to `>= 3` |
| `src/core/SessionInfo.cpp:1243` | `>= 2` shows "Debug 2" | Add `>= 3` for "Debug 3" |

**Protocols using Log Protocol levels:**
- SFTP (`>= 3` for binary) ← This feature
- FTP (`>= 2` for mod time)
- WebDAV (`>= 2` for XML)
- SCP (`>= 1` for blocks)
- S3 (`>= 0` for response)
- SSH (`>= 2` for events)

## Research Context

**User Request:** SFTP dump binary data only when debug level = 3 is set.

**Current Behavior:**
- `SftpFileSystem.cpp:2572` — Binary data dump triggers at `LogProtocol >= 2`
- The Log Protocol dropdown (`WinSCPDialogs.cpp:505`) only has 3 options (0, 1, 2), so level 3 is unreachable

**Example of current binary dump output:**
```
2026-04-26 18:17:55.620 . Read 318 bytes (0 pending)
2026-04-26 18:17:55.620 . Skipped  packets
2026-04-26 18:17:55.620 < Type: SSH_FXP_VERSION, Size: 318, Number: -1
2026-04-26 18:17:55.620 < 02,00,00,00,03,00,00,00,18,70,6F,73,69,78,2D,72,65,6E,61,6D,65,40,6F,70,65,
```

### Decision Made

- **Level 3 Label:** "Debug 3" (simple, consistent with Debug 1/2)
- **Level 2 Behavior Change:** After this change, level 2 will show packet headers ONLY (no binary dump)
  - This is intentional per user request: "dump binary data only when debug level = 3 is set"
- **Other Protocols:** Unchanged - they continue to use level 2 for their debug output

## Tasks

### I. Implementation

1. **Add Log Protocol Level 3 Message IDs**
   - File: `src/base/MsgIDs.h`
   - Add `NB_LOGGING_LOG_PROTOCOL_3` message ID
   - Add to enum after `NB_LOGGING_LOG_PROTOCOL_2`

2. **Update Language Files for Log Protocol Level 3**
   - **Format:** Positional strings (NOT key-value). Add AFTER "Debug 2" line in each file.
   - After adding `NB_LOGGING_LOG_PROTOCOL_3` to MsgIDs.h, update all 4 language files:
     - `src/NetBox/NetBoxEng.lng` — add `"Debug 3"` after line with "Debug 2"
     - `src/NetBox/NetBoxRus.lng` — add Russian translation
     - `src/NetBox/NetBoxPol.lng` — add Polish translation
     - `src/NetBox/NetBoxSpa.lng` — add Spanish translation
   - **Note:** Strings must match enum order in MsgIDs.h (index 0=Normal, 1=Debug1, 2=Debug2, 3=Debug3)

3. **Extend Log Protocol Dropdown in Settings Dialog**
   - File: `src/NetBox/WinSCPDialogs.cpp`
   - Change loop: `Index <= 3` (from `Index <= 2`)
   - Add level 3 to combo box

4. **Update SFTP Binary Dump Condition**
   - File: `src/core/SftpFileSystem.cpp`
   - Change line 2572: `if (FTerminal->Configuration->ActualLogProtocol >= 3)` (from `>= 2`)

5. **Update Session Info Display**
   - File: `src/core/SessionInfo.cpp`
   - Add handling for level 3 in `LogProtocol` display (show "Debug 3")
   - Currently line 1243 uses `>= 2` for "Debug 2", change to `== 2` and add `>= 3` for "Debug 3"

### II. Build & Verify

6. **Build Plugin**
   - Run: `cmd /c build-x64.bat`
   - Verify: Zero warnings

7. **Test Log Protocol Level 3**
   - Launch Far Manager with NetBox plugin
   - Go to Settings → Logging → Log Protocol = "Debug 3"
   - Connect via SFTP
   - Verify: Binary hex dump appears in log

## Commit Plan

Single commit after all tasks complete:

```
feat(logging): add log protocol level 3 for SFTP binary data dump

- Add NB_LOGGING_LOG_PROTOCOL_3 message ID
- Update all 4 language files (Eng, Rus, Pol, Spa)
- Extend log protocol dropdown to 4 levels (0-3)
- Move SFTP binary dump trigger from level >=2 to >=3
- Update session info display for level 3
```