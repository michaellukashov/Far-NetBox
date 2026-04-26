# FTP Crash When Connecting - Analysis Plan

**GitHub Issue:** https://github.com/michaellukashov/Far-NetBox/issues/506  
**Date:** 2026-04-26  
**Branch:** `lmv_dev`

## Problem Statement

After updating to latest version:
1. Cannot connect anywhere via FTP (all servers)
2. When connecting, starts parsing directory listings
3. ~10 minutes delay before crash
4. Far Manager crashes silently (no error message)

## Root Cause

**Unhandled C++ Exception in FTP Listing Parser**

Commit `faec96dd5` (2026-04-23) added safety limits to prevent infinite loops:
- `MAX_CONSECUTIVE_PARSE_FAILURES = 100`
- `MAX_TOTAL_LINES = 100000`
- `MAX_LISTING_BUFFER_SIZE = 10MB`

When limits are exceeded → `m_listingParseFailed = true` → `getList()` throws exception.

**The problem:** Exception is NOT caught at call sites in `FtpControlSocket.cpp`.

### Exception Throw Location
- File: `src/filezilla/FtpListResult.cpp:239`
- Code: `throw Exception(FMTLOAD(MSG_FTP_LISTING_PARSE_FAILED));`

### Unhandled Call Sites
| File | Line | Function |
|------|------|----------|
| FtpControlSocket.cpp | 1819 | CFtpControlSocket::List() |
| FtpControlSocket.cpp | 2499 | CFtpControlSocket::List() (MLSD fallback) |
| FtpControlSocket.cpp | 2944 | CFtpControlSocket::FileTransfer() |

## Why The Symptom Occurs

- **10 minutes delay:** Parser keeps processing until safety limits hit (~100k lines parsed)
- **All servers:** Any FTP server with unusual listing format triggers parse failures
- **Silent crash:** Unhandled C++ exception in C-style code → immediate termination without error

## Fix Plan

### Important: Code Patterns Differ Per Call Site

Each of the 3 locations has different code structure - need specific catch patterns.

#### Location 1: Line ~1819 (List function)
```cpp
try
{
  pData->pDirectoryListing = FtpListResult->getList(Num);
}
catch(Exception &e)
{
  LogMessage(FZ_LOG_ERROR, "FTP listing parse failed: %s", e.Message.c_str());
  delete pData->pDirectoryListing;
  pData->pDirectoryListing = nullptr;
  ResetOperation(FZ_REPLY_ERROR);
  return;
}
```

#### Location 2: Line ~2499 (MLSD fallback)
```cpp
// pListResult created with CreateListResult() - must delete in catch!
CFtpListResult * pListResult = CreateListResult(mlst);
pListResult->AddData(static_cast<const char *>(Buf), Buf.GetLength());
try
{
  pData->direntry = pListResult->getList(num);
}
catch(Exception &e)
{
  ShowStatus(MSG_FTP_LISTING_PARSE_FAILED, FZ_LOG_ERROR);
  delete pListResult;  // Must explicitly delete on exception!
  pData->direntry = nullptr;
  return;
}
delete pListResult;  // Normal cleanup
```

#### Location 3: Line ~2944 (FileTransfer)
```cpp
try
{
  pData->pDirectoryListing = FtpListResult->getList(Num);
}
catch(Exception &e)
{
  LogMessage(FZ_LOG_ERROR, "FTP listing parse failed: %s", e.Message.c_str());
  delete pData->pDirectoryListing;
  pData->pDirectoryListing = nullptr;
  ResetOperation(FZ_REPLY_ERROR);
  return;
}
```

### User-Facing Error (Priority 2)

Use `ShowStatus()` method (lines 234-271) for errors users can see in UI:
```cpp
catch(Exception &e)
{
  ShowStatus(MSG_FTP_LISTING_PARSE_FAILED, FZ_LOG_ERROR);
  // or with format:
  CString msg;
  msg.Format(MSG_FTP_LISTING_PARSE_FAILED, e.Message.c_str());
  ShowStatus(msg, FZ_LOG_ERROR);
  delete pData->pDirectoryListing;
  pData->pDirectoryListing = nullptr;
  ResetOperation(FZ_REPLY_ERROR);
  return;
}
```

### Step 2: Build & Test

- [ ] Add try/catch at line 1819 (m_pTransferSocket->m_pListResult->getList)
- [ ] Add try/catch at line 2499 (local pListResult->getList)  
- [ ] Add try/catch at line 2944 (m_pTransferSocket->m_pListResult->getList)
- [ ] Build with MSVC 2022 (zero warnings)
- [ ] Test with FTP server that triggers parse errors

### Step 3: Verification

- [ ] Error message is displayed instead of crash
- [ ] User can retry or disconnect gracefully
- [ ] Normal FTP servers work correctly (no regression)

## Verification (aif-improve)


| Check | Result |
|---|---|
| getList() call sites found | 3 (confirmed) |
| Try/catch safety | No existing - needs fix |
| ShowStatus pattern | Available at line 234 |
| Memory leak risk | Fixed in plan |

### Codebase Analysis
- `ShowStatus()` method exists at line 234 for user-facing errors
- No existing try/catch blocks in FtpControlSocket.cpp
- Line 2499: pListResult is local - must delete on exception
- Line 1819, 2944: m_pTransferSocket->m_pListResult - already allocated

## Risks
- Low: Simple exception handling addition
- Memory leak prevention included in catch blocks

## Related Files

- `src/filezilla/FtpListResult.cpp` (line 239 - throws exception)
- `src/filezilla/FtpListResult.h` (safety limits)
- `src/NetBox/NetBoxEng.lng` (error message text)