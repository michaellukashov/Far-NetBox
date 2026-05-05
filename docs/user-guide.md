[← Getting Started](getting-started.md) · [Back to README](../README.md) · [Architecture →](architecture.md)

# User Guide

Far-NetBox is a Far Manager 3.0 plugin providing SFTP/FTP/SCP/WebDAV/S3 file transfer capabilities.

Based on [WinSCP](http://winscp.net/eng/index.php) version 6.5.6 Copyright (c) 2000-2025 Martin Prikryl

Based on [WinSCP as FAR Plugin: SFTP/FTP/SCP client for FAR version 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl

SSH and SCP code based on PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

Cryptography based on OpenSSL 3.3.7 Copyright (c) 1998-2025 The OpenSSL Project

## Supported Protocols

| Protocol | Description |
|----------|-------------|
| **SFTP** | SSH File Transfer Protocol (via PuTTY) |
| **SCP**  | Secure Copy Protocol (via PuTTY) |
| **FTP/FTPS** | File Transfer Protocol with TLS (via FileZilla) |
| **WebDAV** | Web-based Distributed Authoring and Versioning |
| **S3**   | Amazon Simple Storage Service (TLS version selection and custom CA certificate support) |

## Folder History Navigation

NetBox integrates with Far Manager's built-in folder history, allowing you to quickly return to previously visited remote sessions and directories.

### Using Folder History

Press **`Alt-F12`** (or navigate via Far Manager's history menu) to see a list of previously visited NetBox locations. Selecting an entry restores both the session connection and the remote directory you were viewing.

NetBox stores each panel location in Far Manager's history in the following format:

```
netbox://<session_name>/<remote_directory>
```

For example:

```
netbox://myserver/home/user/projects
```

This allows Far Manager to remember exactly which server and directory you were in, so history navigation works seamlessly across sessions.

### Legacy Format Compatibility

Older versions of NetBox and WinSCP-based plugins stored history entries in a legacy format (`netbox:SessionName\1RemoteDirectory`). NetBox automatically recognizes and handles both the new `netbox://` format and the legacy format, so your existing shortcuts and history entries continue to work without manual conversion.

### Requirements

- Far Manager 3.0.5955 or newer (provides `FCTL_GETPANELDIRECTORY` / `FCTL_SETPANELDIRECTORY` support)
- NetBox must be connected to the session when you navigate away; the history entry captures the current session and directory automatically

## Connection Keep-Alive

To prevent idle connections from being closed by firewalls or servers, NetBox sends periodic keep-alive commands.

### FTP NOOP Heartbeat

When the **Keep connection alive** option is enabled in session settings, NetBox sends `NOOP` commands at regular intervals during idle FTP control connections. This prevents the server from timing out the control channel during long file transfers or periods of inactivity.

- The interval is controlled by the **Send dummy SSH packets / Keep connection alive interval** setting (shared with SSH).
- Applies to FTP and FTPS connections.

### SSH Keep-Alive

For SFTP and SCP sessions, keep-alive uses PuTTY's built-in SSH null-packet mechanism (`CONF_tcp_keepalives`). The setting is enabled by default.

| Setting | Default | Description |
|---------|---------|-------------|
| Keep connection alive | Enabled | Sends periodic null packets on idle SSH connections |
| Keepalive interval | Server-dependent | Configured via session settings; sent via PuTTY backend |

## Speed Limits

NetBox supports per-session and global speed limits to throttle upload/download bandwidth.

### CPS (Characters Per Second) Limit

Set a maximum transfer speed in bytes per second for individual sessions:

1. Open the session login dialog (`F4` on a saved session).
2. Navigate to the **Environment** tab.
3. Set **Speed limit (CPS)** to a non-zero value.

The limit is enforced across all file operations in that session. A value of `0` means unlimited.

### Esc Cancellation Fix

During transfers, pressing `Esc` opens a cancellation dialog. Prior to the 2026-04 release, repeated cancellation attempts could cause the dialog to hang. This has been fixed — cancellation now completes cleanly even when the speed limit is active.

## Security Settings

### S3 TLS Version Selection

S3 sessions can enforce a specific TLS protocol version:

1. Open the session settings dialog.
2. Navigate to the **S3** tab.
3. Select the desired **TLS/SSL version** from the dropdown.

Available options range from TLS 1.0 through TLS 1.3 (depending on OpenSSL build). Selecting a newer version increases security; selecting an older version improves compatibility with legacy S3 endpoints.

### Custom CA Certificate

For S3, WebDAV, and FTPS connections, you can specify a custom CA certificate file instead of the system certificate store:

1. In session settings, set **Custom CA certificate** to the full path of a `.pem` or `.crt` file.
2. The certificate is loaded per-session and does not affect other connections.

This is useful for connecting to internal or development endpoints with self-signed certificates.

## Keyboard Shortcuts

NetBox follows Far Manager's keyboard conventions and adds shortcuts for common UI elements.

### Combo Box Shortcuts

When a combo box (dropdown list) has focus:

| Shortcut | Action |
|----------|--------|
| `Alt+Down` | Open the dropdown list |
| `Ctrl+Down` | Open the dropdown list (alternative) |

These shortcuts are available throughout NetBox dialogs, including the login dialog, transfer settings, and preferences.

## Session Configuration

### Non-Default FTP Ports

NetBox preserves explicit FTP port settings when saving and reconnecting sessions. If your FTP server runs on a non-standard port (e.g., `2121` instead of `21`):

1. Enter the port number in the **Port number** field of the login dialog.
2. Save the session — the port is stored and reused on reconnect.

Previously, some configurations could lose the custom port on session reload. This is now fixed.

### SSH Send Buffer (`SendBuf`)

The **Send buffer size** setting controls how much data PuTTY buffers before sending over SSH:

- **Default**: `0` (dynamic buffer disabled as of 2026-04)
- **Purpose**: Larger buffers improve throughput on high-latency links; smaller buffers reduce latency.
- **Recommendation**: Leave at `0` unless you have a specific performance tuning need. The 2026-04 default addresses a buffer corruption issue in high-speed transfers.

### SCP Simplicity Mode (`SshSimple`)

The **SshSimple** setting simplifies the SCP command set for compatibility with restricted shells:

- When enabled, NetBox avoids advanced SCP features that some shells (e.g., busybox, chroot) do not support.
- Disable for full feature access on standard OpenSSH servers.

## File Operations

### WebDAV Overwrite and Refresh

When copying files to a WebDAV server, NetBox now correctly handles overwrite prompts and refreshes the remote panel after the operation completes. Previously, the panel could display stale content after an overwrite.

- **Overwrite**: A confirmation dialog appears if the destination exists.
- **Refresh**: The remote directory listing is automatically reloaded after transfer.

### SFTP Remote Folder Creation

When uploading files to an SFTP session, NetBox automatically creates missing remote directories in the destination path. If the server returns a "cannot create remote folder" error:

- Verify your account has write permission to the parent directory.
- Check that the path does not contain invalid characters for the remote filesystem.

## Scripting & Automation

NetBox supports scriptable date/time formatting for automation and logging use cases.

### FormatDateTime Tokens

The `FormatDateTime` function accepts standard formatting tokens:

| Token | Output | Example |
|-------|--------|---------|
| `yyyy` | 4-digit year | `2026` |
| `mm` | 2-digit month | `05` |
| `dd` | 2-digit day | `04` |
| `hh` | 2-digit hour (24h) | `14` |
| `nn` | 2-digit minute | `30` |
| `ss` | 2-digit second | `45` |
| `zzz` | Milliseconds | `123` |

Example usage in session logging or scripting:

```
FormatDateTime("yyyy-mm-dd hh:nn:ss", Now())
→ 2026-05-04 14:30:45
```

### ISO 8601 Parsing

The `ISO8601ToDate` function parses ISO 8601 formatted timestamps commonly used in S3 and WebDAV metadata:

| Input Format | Parsed Result |
|--------------|---------------|
| `2026-05-04T14:30:45Z` | UTC datetime |
| `2026-05-04T14:30:45+03:00` | With timezone offset |

This enables accurate timestamp comparison and synchronization across protocols.


## Silent Mode

NetBox supports a silent mode for file operations that eliminates blocking confirmation dialogs and enables continue-on-error behavior with detailed error reporting.

### When to Use Silent Mode

- **Automation and scripting**: Run file operations without user intervention
- **Batch operations**: Transfer large numbers of files without per-file prompts
- **Deadlock prevention**: Avoid UI deadlocks when the Far Manager window cannot respond

### Configuration

Silent mode is controlled by the `SilentMode` configuration flag. When enabled:
- No confirmation dialogs appear during file transfers
- File operations always overwrite existing files
- Errors are collected instead of aborting the operation
- A detailed error report is generated after operations complete

The setting is global (applies to all sessions) and persists across application restarts. Default is `false` (interactive mode).

### Error Reporting

When silent mode is active and errors occur:
1. Errors are collected with file path, error message, operation side, and timestamp
2. After operations complete, an error report is logged and displayed
3. Reports are truncated after 100 detailed errors for readability

See [Silent Mode Documentation](silent-mode.md) for complete details.
## Links

- Project main page: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
- Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
- Far-NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
- Far-NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)
- Latest builds: [nightly.link](https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview)

## Internationalization

Read this document in other languages:

- [Polish](i18n/README.PL.md)
- [Russian](i18n/README.RU.md)

## License

Far-NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Far-NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.

## See Also

- [Getting Started](getting-started.md) — Installation and first run
- [Architecture](architecture.md) — Project structure and build system

