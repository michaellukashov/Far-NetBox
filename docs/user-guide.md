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

