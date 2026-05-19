# Far-NetBox

> SFTP/FTP/SCP/WebDAV/S3 client plugin for Far Manager 3.0 (x86/x64/ARM64)

Far-NetBox is a file transfer plugin for [Far Manager](https://www.farmanager.com/) supporting multiple network protocols. Built on proven codebases — WinSCP 6.5.6, PuTTY 0.81, and FileZilla 2.2.32 — it delivers reliable remote file management inside Far's text-mode interface.

| Workflow | Build status |
|----------|-------------|
| GitHub Actions | [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml) |
| AppVeyor | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox) |

## Quick Start

```cmd
:: Build for x64 (requires Visual Studio 2022)
cmd /c build-x64.bat

:: Launch Far Manager from the platform directory
Far3_x64\Far.exe

:: Press F11 and select NetBox to open the plugin
```

## Key Features

- **SFTP/SCP** — SSH file transfers via PuTTY
- **FTP/FTPS** — File Transfer Protocol with TLS via FileZilla
- **WebDAV/HTTPS** — Web-based distributed authoring via neon
- **Amazon S3** — S3-compatible storage with TLS and custom CA certificates
- **Session management** — XML-based configuration with password encryption
- **Multi-architecture** — x86, x64, and ARM64 builds

## Documentation

| Guide | Description |
|-------|-------------|
| [Getting Started](docs/getting-started.md) | Build, install, and first connection |
| [User Guide](docs/user-guide.md) | Protocols, features, and internationalization |
| [Architecture](docs/architecture.md) | Project structure, layers, and patterns |
| [Contributing](docs/contributing.md) | Code conventions, build rules, and development workflow |
| [Testing](docs/testing.md) | Manual regression testing and verification |

## License

Far-NetBox is free software licensed under the [GNU General Public License v3](LICENSE.txt).
