# Editor External Modification Detection (Issue 2426)

## Summary

This feature adds detection for externally modified remote files during editor upload, addressing WinSCP Issue 2426. When a user edits a remote file and attempts to save it back, the plugin now checks if the remote file has been modified by another process while the user was editing. If detected, a confirmation dialog is shown allowing the user to either overwrite the externally modified file or cancel the upload.

## Problem Background

In WinSCP 6.5.5, the bug was that the timestamp check used the wrong terminal when multiple sessions were active. NetBox's architecture differs (per-filesystem terminal vs GUI form terminal), so the bug manifested differently:

- **WinSCP bug**: Used wrong terminal for timestamp check (session A editing, session B became active, checked session B's file)
- **NetBox bug**: No timestamp check at all — files were uploaded unconditionally

## Implementation Details

### Files Modified

| File | Change |
|------|--------|
| `src/NetBox/WinSCPFileSystem.h` | Added `SourceTimestamp` to `TMultipleEdit`, added `FLastEditFileTimestamp` and `FLastMultipleEditTimestamp` to `TWinSCPFileSystem` |
| `src/NetBox/WinSCPFileSystem.cpp` | Capture timestamps on download, check timestamps before upload, show confirmation dialog |

### How It Works

1. **Timestamp Capture**: When a file is downloaded for editing, the remote file's modification timestamp is stored:
   - Native edit: `FLastEditFileTimestamp` member
   - Multiple edit: `SourceTimestamp` in `TMultipleEdit` struct

2. **Timestamp Check**: Before uploading (in `UploadFromEditor`):
   - Query current remote file timestamp via `FTerminal->TryReadFile()`
   - Compare with stored timestamp
   - If different, show confirmation dialog

3. **Confirmation Dialog**: Uses existing `EDIT_CHANGED_EXTERNALLY` message (ID 6374):
   - Title: (empty - uses default)
   - Message: "The remote file has been changed while you were editing it. Do you want to overwrite it anyway?"
   - Buttons: OK / Cancel (via `FMSG_WARNING | FMSG_MB_OKCANCEL`)

4. **Error Handling**:
   - If `TryReadFile()` throws, log warning and proceed with upload (conservative default)
   - If timestamp is empty/default, skip check entirely

### Logging

Debug logging (via `DEBUG_PRINTF` in debug builds):
- `Captured remote file timestamp for native edit: %s — %s`
- `Captured remote file timestamp for multiple edit: %s — %s`
- `Checking remote file timestamp: %s, stored: %s`
- `Remote file was modified externally — showing confirmation dialog`
- `User cancelled upload — remote file was modified externally`
- `User confirmed overwrite of externally modified file`
- `Failed to read remote file timestamp: %s — proceeding with upload`

## Test Scenarios

1. **Basic edit/save in active session** — file uploads normally, no dialog
2. **Edit/save after switching to another session** — upload uses correct session's terminal
3. **External modification detected** — dialog appears, Cancel skips upload, OK proceeds
4. **Multiple edit** — each file uploads to correct remote path with proper timestamp check
5. **Save as** — uploads under new name with timestamp check on new path
6. **Upload on save disabled** — pending save flag works correctly
7. **Inactive session with lost connection** — graceful handling, no crash
8. **Regression — panel refresh** — no duplicate file entries after upload
9. **Regression — temp file cleanup** — local temp files removed after editor close

## See Also

- WinSCP Issue 2426: https://winscp.net/tracker/show_bug.cgi?id=2426
- WinSCP 6.5.6 changelog: https://winscp.net/eng/docs/changelog#6.5.6