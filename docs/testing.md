[← Contributing](contributing.md) · [Back to README](../README.md)

# Testing

Far-NetBox is a Far Manager plugin with no automated unit test suite. Testing is performed manually inside Far Manager.

## Manual Regression Testing

### Prerequisites

1. Build the project with `OPT_CREATE_PLUGIN_DIR=ON`
2. Confirm `Far3_<platform>/Plugins/NetBox/NetBox.dll` exists
3. Launch `Far3_<platform>/Far.exe`
4. Press `F11` → NetBox to open the plugin

### Core Test Cases

| Feature | Steps | Expected Result |
|---------|-------|-----------------|
| New SFTP session | Create session → Set host/port → Connect | Remote panel opens with file listing |
| FTP with non-default port | Create FTP session → Set port 443 → Save/reopen | Port persists after save/load cycle |
| File download | Select file → `F5` (Copy) → Confirm | File appears in local panel |
| File upload | Select local file → `F5` → Confirm | File appears in remote panel |
| Directory creation | `F7` → Enter name | Directory created on remote host |
| Directory deletion | Select directory → `F8` → Confirm | Directory removed |
| Cancel transfer | Start large transfer → Press `Esc` → Confirm cancel | Transfer stops gracefully |
| Speed limit | Set CPS limit → Start transfer | Transfer rate limited correctly |
| Session save/load | Save session → Restart Far → Reopen session | Settings preserved |
| WebDAV TLS | Create WebDAV session → Enable HTTPS | TLS handshake succeeds |
| S3 bucket listing | Create S3 session → Connect | Bucket and object list displays |

### Protocol-Specific Tests

| Protocol | Specific Checks |
|----------|-----------------|
| **SFTP** | Key authentication, password auth, keyboard-interactive, host key caching |
| **FTP** | Active/passive mode, FTPS explicit/implicit, UTF-8 filenames |
| **SCP** | Recursive copy, preserve permissions |
| **WebDAV** | PROPFIND parsing, lock support, TLS certificate validation |
| **S3** | Bucket access, multipart upload, virtual-hosted vs path-style |

## Logging

Debug output is written to `%LOCALAPPDATA%\NetBox\netbox.log`. Increase log level via `tinylog` configuration or by enabling protocol-level debug output in session settings.

## Debugging in Visual Studio

1. Generate VS solution:
   ```cmd
   cmake -S . -B build-VS -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
   ```
2. Open `build-VS\NetBox.sln`
3. Set debug command to `Far3_x64\Far.exe`
4. Set breakpoints in protocol code
5. Launch and trigger the affected functionality

## Build Verification

Before declaring any change complete:

- [ ] Clean build with zero warnings (MSVC W4)
- [ ] No modifications to `libs/`
- [ ] Plugin DLL in `Far3_<platform>/Plugins/NetBox/`
- [ ] CRLF line endings on all modified files
- [ ] UTF-8 without BOM
- [ ] No trailing whitespace
- [ ] Naming conventions followed (`T`/`F` prefixes)

## See Also

- [Contributing](contributing.md) — Build instructions and development workflow
- [Getting Started](getting-started.md) — Installation and first run
