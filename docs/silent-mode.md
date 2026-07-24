# Silent Mode for File Operations

Silent mode eliminates blocking dialogs during file operations and enables continue-on-error with detailed error reporting. This is useful for automation, batch operations, and preventing deadlocks when the UI cannot respond.

## What Is Silent Mode

When silent mode is enabled:

- No confirmation dialogs appear during file transfers (delete, create directory, synchronized browsing, create session folder, and more)
- File operations overwrite existing files **only if the source is newer** (`boOlder`)
- Errors are collected instead of aborting the operation
- A detailed error report is generated after operations complete

## When to Use Silent Mode

- **Automation and scripting**: Run file operations without user intervention
- **Batch operations**: Transfer large numbers of files without per-file prompts
- **Deadlock prevention**: Avoid UI deadlocks when the Far Manager window cannot respond

## Enabling and Disabling Silent Mode

Silent mode can be enabled in two ways:

### Via User Interface

1. Open **Preferences → Confirmations**
2. Check the **"Silent mode (suppress all confirmations)"** checkbox
3. When Silent mode is on, the other confirmation checkboxes are grayed out

### Via Configuration File

Silent mode is controlled by the `SilentMode` configuration flag:

- **Default**: `false` (interactive mode)
- **Persistence**: The setting persists across application restarts
- **Scope**: Global configuration (applies to all sessions)

### Configuration Storage

The `SilentMode` flag is stored in the NetBox configuration under the key `SilentMode` in the `Interface` section.

## Error Reporting

When silent mode is active and errors occur during file operations:

1. Errors are collected with:
   - File path
   - Error message
   - Operation side (local or remote)
   - Timestamp

2. After operations complete, an error report is:
   - Logged at INFO level
   - Written to a `.errors` file alongside the session log file (e.g., `session.log` → `session.errors`)
   - A brief summary (e.g., `"5 errors - see C:\temp\session.errors"`) is shown in the status line

3. Reports are truncated after 100 detailed errors for readability

### Error Categories

Errors are categorized as:
- **NetworkError**: Connection lost, timeout
- **PermissionDenied**: Access denied, read-only files
- **ResourceError**: Disk full, quota exceeded
- **FileNotFound**: Missing source or destination files
- **Other**: Unclassified errors

## Limitations and Caveats

- **Overwrite if newer**: Silent mode now uses `boOlder` — files are overwritten only when the source is newer. This is safer than the previous behavior (overwrite all).
- **Behavioral change note**: If you previously enabled `SilentMode` via XML configuration, the overwrite mode has changed from "overwrite all" to "overwrite if newer".
- **Persists across restarts**: The setting is saved in configuration and persists until explicitly disabled.
- **Error report truncation**: Reports show up to 100 detailed errors. Additional errors are counted but not listed individually.
- **All protocols**: Silent mode works across all supported protocols (SFTP, FTP, SCP, WebDAV, S3).

## Protocol Support

Silent mode is supported for all protocols:
- SFTP/SCP (via PuTTY)
- FTP/FTPS (via FileZilla)
- WebDAV (via neon)
- S3 (via libs3)

No modifications to third-party libraries are required.
