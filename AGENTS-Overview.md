# AGENTS-Overview.md — NetBox Project Overview

> Part of the AGENTS documentation series. See also: [AGENTS.md](AGENTS.md) (entry), [AGENTS-Structure.md](AGENTS-Structure.md), [AGENTS-Standards.md](AGENTS-Standards.md), [AGENTS-Workflows.md](AGENTS-Workflows.md).

## What is NetBox?

NetBox is a **Far Manager 3.0 plugin** providing file transfer capabilities:
- **SFTP** — SSH File Transfer Protocol
- **FTP** — File Transfer Protocol (plain and TLS/SSL)
- **SCP** — Secure Copy over SSH
- **WebDAV** — Web-based Distributed Authoring and Versioning
- **S3** — Amazon S3 storage

Loads as a DLL via Far Manager's F11 plugin menu.

## Version Matrix

| Component | Version |
|-----------|---------|
| **NetBox** | See `src/NetBox/plugin_version.hpp` |
| **OpenSSL** | 3.3.2 |
| **PuTTY** | 0.81 |
| **FileZilla** | 2.2.32 |
| **WinSCP** | 6.5.1 |
| **zlib-ng** | 2.2.2 |

## Dependencies

Third-party libraries in `libs/` — **never modify directly**, use patches if needed:

| Library | Location | Purpose |
|---------|----------|---------|
| PuTTY | `libs/putty/` | SSH/SCP protocol |
| FileZilla | `src/filezilla/` | FTP protocol |
| OpenSSL 3 | `libs/openssl-3/` | Cryptography |
| neon | `libs/neon/` | WebDAV protocol |
| libs3 | `libs/libs3/` | S3 protocol |
| ATL/MFC | `libs/atlmfc/` | Minimal MFC subset |
| dlmalloc | `libs/dlmalloc/` | Memory allocator |
| expat | `libs/expat/` | XML parsing |
| zlib-ng | `libs/zlib-ng/` | Compression |
| tinyxml2 | `libs/tinyxml2/` | XML parser |
| fmt | `libs/fmt/` | String formatting |
| tinylog | `libs/tinylog/` | Logging |
| GSL | `libs/GSL/` | Guidelines Support Library |
| icecream-cpp | `libs/icecream-cpp/` | Debug logging |

## Build Output

- Plugin DLLs: `Far3_<platform>/Plugins/NetBox/`
- Platforms: `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/`

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Missing `vcvarsall.bat` | Install VS2022 with "Desktop development with C++" |
| Ninja not found | `winget install Ninja-build.ninja` |
| Unity build errors | Add `-DOPT_USE_UNITY_BUILD=OFF` |
| Plugin fails to load | Check architecture match (x86/x64), verify dependencies |
| Connection failures | Check firewall, test with `ping`/`telnet`, review plugin log |
| WinXP build failures | Use v141_xp toolset: `-T v141_xp` in CMake or set in VS IDE |