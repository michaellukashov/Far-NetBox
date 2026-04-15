# Plan: Port WinSCP 6.5.6 Issue 2426 — Inactive Session Editor Upload Fix

**Branch:** `feature/winscp-2426-inactive-session-fix`
**Created:** 2026-04-12
**Mode:** Full

## Settings

- **Testing:** Manual (scenarios with active/inactive sessions)
- **Logging:** Verbose — detailed DEBUG logs for development
- **Docs:** Yes — mandatory docs checkpoint at completion
- **Roadmap Linkage:** Milestone: "none" — Roadmap artifact does not exist
- **Rationale:** No ROADMAP.md found in project; linkage skipped

## Research Context

### Active Summary (from WinSCP 6.5.5..6.5.6 analysis)

**Topic:** Port WinSCP Issue 2426 fix — external file modification check for inactive sessions

**Key findings:**
- WinSCP 6.5.6 contains exactly ONE significant bugfix in `source/` between 6.5.5 and 6.5.6:
  - Commit `b1c0eb16d`: `Issue 2426 – Checking if edited/opened file was modified externally didn't work for inactive sessions`
  - Change: `Terminal->TryReadFile()` → `Data->Terminal->TryReadFile()` in `ExecutedFileChanged()`
  - This fixes timestamp comparison using the correct terminal (the one the file was opened from), not the currently active terminal

- Other changes in 6.5.5..6.5.6 are EXCLUDED from porting:
  - Expat 2.7.4/2.7.5 updates — third-party library, separate task
  - OpenSSL 3.3.6 update — third-party library, separate task
  - Translation updates — not applicable to NetBox
  - Version bumps (6.5.6, copyright year) — not applicable
  - Inno Setup installer strings — not applicable
  - Project file changes (.cbproj) — not applicable

- **NetBox status:** The `EDIT_CHANGED_EXTERNALLY` message string exists in resources but is NEVER used. No timestamp checking exists in the FAR plugin editor upload flow. The bug is MORE severe in NetBox — not only does it use the wrong terminal for checking, it doesn't check at all.

## Analysis: WinSCP vs NetBox Architecture Mapping

### WinSCP Original Bug

In WinSCP GUI, `TCustomScpExplorerForm::ExecutedFileChanged()` checks if a remote file was modified externally while being edited. The bug:

```cpp
// BEFORE (6.5.5) — BUG: uses global active terminal
std::unique_ptr<TRemoteFile> File(Terminal->TryReadFile(RemoteFilePath));

// AFTER (6.5.6) — FIX: uses terminal from the edited file's data
std::unique_ptr<TRemoteFile> File(Data->Terminal->TryReadFile(RemoteFilePath));
```

When a session becomes inactive (user switches to another panel), `Terminal` (the form's active terminal) points to the wrong session. The fix uses `Data->Terminal` which is the terminal the file was originally opened from.

### NetBox Equivalent

NetBox has a different architecture — it's a FAR plugin, not a GUI app:

| WinSCP GUI | NetBox FAR Plugin |
|---|---|
| `TCustomScpExplorerForm::ExecutedFileChanged()` | `TWinSCPFileSystem::ProcessEditorEvent(EE_REDRAW/EE_SAVE/EE_CLOSE)` |
| `TEditorManager` with `OnFileChange` callback | No equivalent — editor events come from FAR |
| `Data->Terminal` (TEditedFileData) | `FTerminal` member of `TWinSCPFileSystem` |
| `Terminal` (form's active terminal) | `FTerminal` of whichever filesystem receives the event |
| Timestamp check in `ExecutedFileChanged()` | **DOES NOT EXIST** |

**Critical difference:** In WinSCP, the bug causes timestamp check against wrong session. In NetBox, there is NO timestamp check at all — the file is uploaded unconditionally.

### The Real Problem in NetBox

When editor events fire, `ProcessEditorEvent` is called on ALL opened filesystems (WinSCPPlugin.cpp:249-252):

```cpp
for (int32_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
{
  TWinSCPFileSystem * FileSystem = FOpenedPlugins->GetAs<TWinSCPFileSystem>(Index);
  FileSystem->ProcessEditorEvent(Info->Event, Info->Param);
}
```

This means if you edit a file from Session A, then switch to Session B, both sessions receive `EE_REDRAW`. Session A's `UploadOnSave()` uses its own `FTerminal` (correct). But the architecture doesn't store timestamps, so there's no "wrong terminal" bug per se — there's a "no check at all" gap.

### What Needs to Be Ported

The WinSCP fix has TWO aspects:
1. **Use the correct terminal** — In NetBox, `FTerminal` is already per-filesystem, so this is architecturally correct
2. **Add timestamp checking** — This is MISSING in NetBox and needs to be added

The port should:
1. Store the original remote file timestamp when downloading for edit
2. Before uploading, check if the remote file was modified externally
3. If so, show `EDIT_CHANGED_EXTERNALLY` dialog and let user decide

## Tasks

### Phase 1: Infrastructure — Store Timestamps on Edit Download

**Task 1.1: Add timestamp fields to TMultipleEdit and TWinSCPFileSystem**

Files: `src/NetBox/WinSCPFileSystem.h`

Changes:
- Add `TDateTime SourceTimestamp;` field to `TMultipleEdit` struct
- Add `TDateTime FLastEditFileTimestamp;` member to `TWinSCPFileSystem`
- Add `#include` for datetime types if not already present

Logging: Add debug log when timestamp is stored: `ADF(L"Stored edit timestamp: %s", StandardTimestamp(FLastEditFileTimestamp));`

---

**Task 1.2: Capture timestamp when downloading for native edit**

Files: `src/NetBox/WinSCPFileSystem.cpp`

Find the code path where a file is downloaded for editing (look for `OPM_EDIT` or `EditFile` in `GetFilesRemote` or equivalent). After downloading, store the remote file's modification timestamp in `FLastEditFileTimestamp`.

For multiple edit, find where `TMultipleEdit` is populated (around line 4003 in `ProcessEditorEvent(EE_READ)`) and store the timestamp there too.

Logging: `ADF(L"Captured remote file timestamp for edit: %s — %s", FileName, StandardTimestamp(Timestamp));`

---

### Phase 2: Timestamp Check Before Upload

**Task 2.1: Add timestamp comparison in UploadFromEditor**

Files: `src/NetBox/WinSCPFileSystem.cpp`

In `UploadFromEditor()`, BEFORE calling `UploadFiles()`:
1. Construct the remote file path: `UnixCombinePaths(DestPath, RealFileName)`
2. Call `FTerminal->TryReadFile(RemoteFilePath)` to get current remote file info
3. If file exists and timestamp differs from stored `SourceTimestamp`:
   - Show dialog with `EDIT_CHANGED_EXTERNALLY` message
   - If user cancels, abort the upload (return early)
   - If user confirms, proceed with upload

For native edit, use `FLastEditFileTimestamp`. For multiple edit, use `it->second.SourceTimestamp`.

Key method to use: `FTerminal->TryReadFile()` — already exists in `src/core/Terminal.cpp:4171`. It returns `TRemoteFile*` with `Modification` field.

Timestamp comparison logic (from WinSCP):
```cpp
if (File->Modification != Data->SourceTimestamp)
{
  // Show dialog
}
```

Logging:
- `ADF(L"Checking remote file timestamp: %s, stored: %s", StandardTimestamp(File->Modification), StandardTimestamp(StoredTimestamp));`
- `ADF(L"Remote file was modified externally — showing confirmation dialog");`
- `ADF(L"User confirmed overwrite of externally modified file");`

---

**Task 2.2: Handle the EDIT_CHANGED_EXTERNALLY dialog**

Files: `src/NetBox/WinSCPFileSystem.cpp`, `src/NetBox/WinSCPPlugin.h`

The message string already exists in `src/resource/TextsWin1.rc` (ID 6374). Use `GetMsg(EDIT_CHANGED_EXTERNALLY)` to retrieve it.

Show a confirmation dialog using the plugin's message dialog facility. The dialog should have OK/Cancel buttons:
- OK → proceed with upload
- Cancel → skip upload, keep local temp file

For the dialog, use `GetWinSCPPlugin()->Message()` or equivalent FAR plugin message facility. Check existing patterns in the codebase for how confirmation dialogs are shown.

Logging: `ADF(L"User cancelled upload — remote file was modified externally");`

---

### Phase 3: Edge Cases and Safety

**Task 3.1: Add inactive session validation**

Files: `src/NetBox/WinSCPFileSystem.cpp`

In `UploadOnSave()` and `UploadFromEditor()`, add a check:
```cpp
if (!FTerminal->GetActive())
{
  ADF(L"Session is not active — skipping upload for %s", AFileName);
  return;
}
```

This prevents upload attempts when the connection has been lost or the session is no longer connected.

Logging: `ADF(L"UploadOnSave skipped — session inactive for %s", AFileName);`

---

**Task 3.2: Handle TryReadFile failures gracefully**

Files: `src/NetBox/WinSCPFileSystem.cpp`

`TryReadFile()` can throw exceptions (see `Terminal.cpp:4171-4198`). Wrap the timestamp check in try-catch:
- If exception and session is active → log warning, proceed with upload (conservative)
- If exception and session is inactive → abort upload

Logging:
- `ADF(L"Failed to read remote file timestamp: %s — proceeding with upload", ExceptionMessage);`
- `ADF(L"Remote file not found — new file upload, skipping timestamp check");`

---

### Phase 4: Build and Manual Testing

**Task 4.1: Build verification**

Build the plugin with no warnings:
```cmd
cmake --build ../build-RelWithDebugInfo --clean-first -- -j4
```

Verify:
- No MSVC warnings (W4 level)
- Plugin DLL is generated in `Far3_x64/Plugins/NetBox/`

---

**Task 4.2: Manual test scenarios**

Test the following scenarios with the built plugin:

1. **Basic edit/save in active session:**
   - Open session A
   - Edit a file
   - Save in editor
   - Verify: file uploads normally, no dialog

2. **Edit/save after switching to another session (inactive session scenario):**
   - Open session A, edit a file
   - Switch to session B (making A inactive)
   - Save in editor
   - Verify: upload uses session A's terminal (correct), not session B's

3. **External modification detected:**
   - Open session A, edit a file
   - Modify the same file on the server externally (via another client)
   - Save in editor
   - Verify: dialog "The remote file has been changed while you were editing it" appears
   - Verify: Cancel skips upload, OK proceeds

4. **Multiple edit:**
   - Open multiple files for editing
   - Save them in different order
   - Verify: each uploads to correct remote path

5. **Save as:**
   - Edit a file, use "Save As" with different name
   - Verify: uploads under new name

6. **Upload on save disabled:**
   - Disable "Editor upload on save" in settings
   - Edit and save
   - Verify: file is NOT uploaded automatically, pending save flag is set
   - Close editor
   - Verify: upload happens on EE_CLOSE

7. **Inactive session with lost connection:**
   - Open session A, edit a file
   - Disconnect session A (or let it timeout)
   - Save in editor
   - Verify: no crash, appropriate error message

8. **Regression — panel refresh:**
   - After upload, verify panel content is refreshed
   - Verify no duplicate file entries

9. **Regression — temp file cleanup:**
   - After editor close, verify local temp files are removed
   - Verify temp directory is cleaned if empty

---

## Commit Plan

**Commit 1:** `fix(editor): store remote file timestamps on edit download`
- Tasks 1.1, 1.2

**Commit 2:** `fix(editor): check for external modifications before upload (Issue 2426)`
- Tasks 2.1, 2.2, 3.1, 3.2

**Commit 3:** `chore: build verification and manual test scenarios`
- Tasks 4.1, 4.2

## Files to Modify

| File | Change |
|------|--------|
| `src/NetBox/WinSCPFileSystem.h` | Add `SourceTimestamp` to `TMultipleEdit`, add `FLastEditFileTimestamp` member |
| `src/NetBox/WinSCPFileSystem.cpp` | Capture timestamps on download, check timestamps before upload, add inactive session validation |
| `src/resource/TextsWin1.rc` | No changes needed (message already exists) |
| `src/resource/TextsWin.h` | No changes needed (ID already defined) |

## Invariants

1. **Never upload to wrong session** — `FTerminal` must be the terminal the file was opened from
2. **Never lose user's edits** — if timestamp check fails, default to allowing upload (with warning)
3. **No regression in existing behavior** — basic edit/save must work exactly as before when no external modification occurs
4. **Temp files must be cleaned up** — regardless of upload success/failure

## Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Timestamp precision mismatch | Medium | Use same precision comparison as WinSCP (`!=` on `TDateTime`) |
| Dialog blocks FAR UI | Low | Use existing FAR plugin message dialog pattern |
| TryReadFile throws on inactive session | Medium | Wrap in try-catch, check `GetActive()` first |
| Multiple edit timestamp tracking | Low | Same pattern as native edit, just stored in map |

## Difference from WinSCP Original

| Aspect | WinSCP 6.5.6 | NetBox Adaptation |
|--------|-------------|-------------------|
| Bug | Used wrong terminal for timestamp check | No timestamp check at all |
| Fix scope | One-line change (`Data->Terminal`) | New functionality (store + check) |
| Architecture | GUI app with `TEditorManager` | FAR plugin with `ProcessEditorEvent` |
| Terminal reference | `Data->Terminal` (per-file data) | `FTerminal` (per-filesystem member) |
| Dialog | `MessageDialog()` with `qtConfirmation` | FAR plugin `Message()` or equivalent |

The NetBox adaptation is MORE than a one-line port — it implements missing functionality that WinSCP already had.
