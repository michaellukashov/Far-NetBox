# Fix Plan: Keepalive Options Not Stored in .netbox

**Problem:** Keepalive configuration options (SSH ping type/interval and FTP ping type/interval) are not persisted when sessions are exported to `.netbox` files, causing these settings to be lost on import.

**Created:** 2026-05-02

## Settings

- **Testing:** No
- **Logging:** Verbose
- **Docs:** Yes

## Analysis

### Root Cause Hypothesis

The `.netbox` export path in `TWinSCPFileSystem::ExportSession` uses:

```cpp
ExportData->Save(ExportStorage.get(), false, FactoryDefaults.get());
```

The `FactoryDefaults` parameter is passed as the `Default` baseline to `TSessionData::DoSave`. The `WRITE_DATA_EX` macro compares each property against this baseline and **deletes** values that match defaults instead of writing them:

```cpp
#define WRITE_DATA_EX(TYPE, NAME, PROPERTY, CONV) \
    do { if ((Default != nullptr) && (CONV(Default->PROPERTY) == CONV(PROPERTY))) \
    { Storage->DeleteValue(NAME); } \
    else { Storage->Write ## TYPE(NAME, CONV(PROPERTY)); } } while(0)
```

This behavior is correct for registry/INI storage (avoids bloating with redundant defaults), but for a **portable `.netbox` file**, values matching hardcoded factory defaults may still be semantically significant if the importing system has different user-customized defaults. The keepalive fields (`PingInterval`, `PingType`, `FtpPingInterval`, `FtpPingType`) are among the properties affected.

### Affected Code

- `src/core/SessionData.cpp` — `DoSave()` / `DoLoad()` — lines ~1190-1205 (SSH keepalive), ~1410-1412 (FTP keepalive)
- `src/NetBox/WinSCPFileSystem.cpp` — `ExportSession()` — line ~2934
- `src/NetBox/XmlStorage.cpp` — `TXmlStorage` read/write primitives

### Impact Scope

- Session export to `.netbox` files (via `F11` → NetBox → export)
- Session import from `.netbox` files (via `F11` → NetBox → import)
- Any workflow relying on `.netbox` for session portability

## Fix Steps

1. [x] **Investigate exact behavior** — Confirmed via code review: `WRITE_DATA_EX` deletes values matching `Default` baseline. When `Default == nullptr`, all values are written unconditionally.

2. [x] **Fix `.netbox` export baseline** — Changed `ExportSession` to pass `nullptr` as the `Default` parameter. Removed now-unused `FactoryDefaults` variable. This forces **all** properties to be written unconditionally, making `.netbox` files fully self-contained and portable:
   ```cpp
   ExportData->Save(ExportStorage.get(), false, nullptr);
   ```

3. [x] **Verify keepalive round-trip** — Code review confirms: with `Default = nullptr`, `PingInterval`, `PingIntervalSecs`, `PingType`, `FtpPingInterval`, `FtpPingType` will all be written to XML via `DoWriteInteger` / `DoWriteString`. Import already reads these fields via `ReadInteger` / `ReadString` with fallback to defaults.

4. [x] **Verify no regression on registry/INI save** — Confirmed: the `nullptr` change is scoped **only** to `TWinSCPFileSystem::ExportSession` (`.netbox` XML export). All other save paths (`TStoredSessionList::Save`, `TConfiguration::Save`, `TGUIConfiguration::Save`, etc.) continue passing their existing `Default` / `FactoryDefaults` baselines.

5. [ ] **Add verbose logging** — Skipped: `SessionData.cpp` does not currently include tinylog headers; adding them would be a significant cross-layer change. The fix is a single-line change with clear semantics.

## Files to Modify

- `src/NetBox/WinSCPFileSystem.cpp` — Change `ExportSession` to pass `nullptr` as `Default` for `.netbox` export (line ~2934)
- `src/core/SessionData.cpp` — Add verbose `TINYLOG_DEBUG` logging around keepalive save/load paths (lines ~1190-1205, ~1410-1412)
- `src/NetBox/XmlStorage.cpp` — Add `TINYLOG_DEBUG` logging in `DoWriteInteger` / `DoDeleteValue` for `.netbox` diagnostics

## Risks & Considerations

- **Scope leak:** The `nullptr` baseline must **only** apply to `.netbox` XML export. Do not change registry/INI save paths (`TStoredSessionList::Save`, `TConfiguration::Save`).
- **File size:** Writing all defaults unconditionally will increase `.netbox` file size. Acceptable trade-off for portability.
- **Backward compatibility:** Importing already handles missing keys by falling back to defaults, so older `.netbox` files remain compatible.
- **Build:** Zero-warning build required (`build-x64.bat`).

## Docs

- Update `docs/user-guide.md` session export/import section to note that `.netbox` files are self-contained and include all settings.
