# AGENTS-Structure.md — File and Folder Structure

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

This file describes the complete file and folder structure of the NetBox project, including source tree layout, CMake configuration, third-party library locations, version information, OpenSSL patching, NASM assembly, and language files. It is the reference for where files live, where new files should be placed, and how the build system is organized.

---

## Source Tree

```
src/
├── base/        # Base classes (UnicodeString, Classes, etc.)
├── core/        # Protocol implementations (SSH, FTP, SCP, S3, WebDAV)
├── filezilla/   # FileZilla-based FTP engine
├── include/     # Public headers (nbtypes.h, rtti.hpp)
├── nbcore/      # Core utilities (strings, memory, logging)
├── NetBox/      # Far Manager plugin interface (entry point)
├── PluginSDK/   # Far3 plugin SDK headers
├── resource/    # Resources (.rc, .lng, licenses)
└── windows/     # Windows-specific code (GUI, dialogs)
```

### Detailed Directory Contents

```
src/
├── NetBox/              # Plugin Layer — Far Manager API entry point
│   ├── NetBox.cpp       # DLL entry, global callbacks
│   ├── WinSCPPlugin.*   # WinSCP plugin adapter
│   ├── WinSCPFileSystem.* # WinSCP filesystem adapter
│   ├── FarPlugin.*      # Far Manager plugin wrapper
│   ├── FarInterface.*   # Far API abstraction
│   ├── FarDialog.*      # Dialog framework (TFarDialog, TTabbedDialog)
│   ├── FarConfiguration.* # Far-specific configuration storage
│   ├── Far3Storage.*    # Far 3.x storage backend
│   ├── WinSCPDialogs.cpp # Session/login dialogs
│   ├── MessageDlg.cpp   # Message dialog implementation
│   ├── FarPluginStrings.* # Localized string resources
│   ├── XmlStorage.*     # XML-based configuration persistence
│   ├── MsgIDs.h         # Message ID definitions
│   ├── *.lng            # Language files (EN, FR, POL, RUS, SPA)
│   ├── plugin_version.hpp # Version constants
│   ├── guid.h           # Plugin GUIDs
│   └── resource.h        # Resource identifiers
├── core/                # Core Layer — Protocol implementations & session management
│   ├── FileSystems.*    # TCustomFileSystem abstract base
│   ├── SftpFileSystem.* # SFTP protocol (via TSecureShell → PuTTY)
│   ├── ScpFileSystem.*  # SCP protocol (via TSecureShell → PuTTY)
│   ├── SecureShell.*    # SSH session management (PuTTY wrapper)
│   ├── FtpFileSystem.*  # FTP/FTPS protocol (via FileZilla engine)
│   ├── WebDAVFileSystem.* # WebDAV protocol (via neon)
│   ├── S3FileSystem.*   # Amazon S3 protocol (via libs3)
│   ├── NeonIntf.*       # neon library interface (WebDAV + S3 TLS)
│   ├── PuttyIntf.*      # PuTTY interface glue
│   ├── Terminal.*       # Terminal coordinator & session orchestration
│   ├── SessionData.*    # Session configuration container
│   ├── RemoteFiles.*    # Remote file representation & directory listings
│   ├── Configuration.*  # Global configuration management
│   ├── CopyParam.*      # Copy/move operation parameters
│   ├── Cryptography.*   # Cryptographic utilities
│   ├── Queue.*          # Background operation queue
│   └── ...              # (other core modules)
├── filezilla/           # FileZilla FTP engine (third-party, adapted)
├── base/                # Base Layer — Foundation classes
│   ├── UnicodeString.*  # Wide string class
│   ├── Exceptions.*     # Exception hierarchy (EAbort, EInOutError, ...)
│   ├── Classes.*        # Base class definitions
│   ├── Sysutils.*       # System utilities
│   ├── System.SyncObjs.* # Synchronization primitives
│   ├── System.IOUtils.*  # File I/O utilities
│   ├── FileBuffer.*     # File buffering
│   ├── LogContext.*     # Log context base
│   └── ...              # (other foundation modules)
├── nbcore/              # NetBox core utilities
│   ├── nbstring.cpp     # String utilities
│   ├── nbmemory.cpp     # Memory management
│   ├── nbutils.cpp      # General utilities
│   └── stdafx.h         # Precompiled header
├── include/             # Public headers & type definitions
│   ├── nbtypes.h        # Core type aliases
│   ├── nbstring.h       # String type aliases
│   ├── nbsystem.h       # System type aliases
│   ├── rtti.hpp         # RTTI utilities
│   ├── type_traits.h    # Type traits
│   └── FastDelegate.h   # Fast delegate implementation
├── PluginSDK/           # Far Manager 3.x plugin SDK headers (read-only)
├── resource/            # Binary resources (icons, bitmaps, licenses)
└── windows/             # Windows-specific code (GUI, COM, registry)
    ├── WinConfiguration.* # Windows configuration storage
    ├── GUIConfiguration.* # GUI-specific settings
    ├── GUITools.*       # GUI helper functions
    ├── TerminalManager.* # Terminal session manager
    └── ...              # (other Windows modules)
```

---

## Build Output

| Artifact | Location |
|----------|----------|
| Plugin DLLs | `Far3_<platform>/Plugins/NetBox/` |
| Platform dirs | `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/` |
| Build dir | `build-<config>/` (e.g., `build-RelWithDebugInfo/`) |

**Requires:** `OPT_CREATE_PLUGIN_DIR=ON` in CMake configuration.

**Note:** Plugin DLLs go to `Far3_<platform>/Plugins/NetBox/` — **not** `build-*/src/`.

---

## CMake Structure

```
CMakeLists.txt                    # Main entry
├── cmake/
│   ├── NetBox.cmake              # Compiler/linker flags
│   ├── Libraries.cmake           # Centralized library config
│   ├── PlatformDetection.cmake   # Platform auto-detection
│   └── Install.cmake             # Post-build installation
├── libs/*/CMakeLists.txt         # Third-party library builds
└── src/CMakeLists.txt            # Plugin target (~530 lines)
```

---

## CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug`, `Release`, `RelWithDebugInfo` | `Debug` | Build configuration |
| `PROJECT_PLATFORM` | `x86`, `x64`, `ARM64` | Auto-detected | Target architecture |
| `OPT_CREATE_PLUGIN_DIR` | `ON`, `OFF` | `OFF` | Create plugin directory structure |
| `OPT_USE_UNITY_BUILD` | `ON`, `OFF` | `ON` (x86 Release only) | Faster compilation |
| `OPT_COMPILE_COMMANDS` | `ON`, `OFF` | `OFF` | Generate `compile_commands.json` |

---

## Adding a New Library

1. Create `cmake/NewLib.cmake` with library configuration
2. Create `libs/newlib/CMakeLists.txt` for the build
3. Add `add_subdirectory(libs/newlib)` in root `CMakeLists.txt`
4. Link to plugin: add `newlib` to `NETBOX_LIBRARIES` in `src/CMakeLists.txt`

---

## Third-party Libraries (`libs/`)

| Library | Location | Purpose |
|---------|----------|---------|
| PuTTY | `libs/putty/` | SSH/SCP protocol |
| FileZilla | `src/filezilla/` | FTP protocol |
| neon | `libs/neon/` | WebDAV protocol |
| libs3 | `libs/libs3/` | S3 protocol |
| OpenSSL | `libs/openssl-3/` | TLS/SSL cryptography |
| zlib-ng | `libs/zlib-ng/` | Compression |
| expat | `libs/expat/` | XML parsing |
| tinyxml2 | `libs/tinyxml2/` | XML parser |
| fmt | `libs/fmt/` | String formatting |
| tinylog | `libs/tinylog/` | Logging |
| GSL | `libs/GSL/` | Guidelines Support Library |
| icecream-cpp | `libs/icecream-cpp/` | Debug logging |
| dlmalloc | `libs/dlmalloc/` | Memory allocator |
| ATL/MFC | `libs/atlmfc/` | Minimal MFC subset |

---

## Version Matrix

| Component | Version |
|-----------|---------|
| **OpenSSL** | 3.3.7 |
| **PuTTY** | 0.81 |
| **FileZilla** | 2.2.32 |
| **WinSCP** (upstream) | 6.5.6 |
| **zlib-ng** | 2.2.2 |
| **NetBox** | See `src/NetBox/plugin_version.hpp` |

---

## NASM (OpenSSL Assembly)

OpenSSL uses platform-specific assembly files for optimized crypto primitives (SHA, AES, BN, etc.). The build system compiles them with NASM via `cmake/OpenSSL.cmake` → `openssl_setup_asm_files()`.

**NASM executable:** `buildtools/tools/nasm.exe`

### x64 Platform (25 `.asm` files → `.obj`)

```cmake
nasm.exe -f win64 -o <build-dir>/<filename>.asm.obj libs/openssl-3/<path>/<filename>.asm
```

Key x64 ASM files: `crypto/sha/sha*-x86_64.asm`, `crypto/aes/aesni-x86_64.asm`,
`crypto/bn/x86_64-mont*.asm`, `crypto/ec/ecp_nistz256-x86_64.asm`,
`crypto/modes/ghash-x86_64.asm`, `crypto/modes/aesni-gcm-x86_64.asm`.

### x86 Platform (19 `.obj.asm` files → `.obj`)

```cmake
nasm.exe -f win32 -o <build-dir>/<filename>.obj.asm.obj libs/openssl-3/<path>/<filename>.obj.asm
```

### ARM64

No ASM files (pure C fallback).

### Adding New ASM Files

Add to `_asm_file_list` in `cmake/OpenSSL.cmake` under the appropriate platform branch.
The `add_custom_command()` in `openssl_setup_asm_files()` handles compilation automatically.
ASM objects are linked into `libeay32` via `CRYPTO_SOURCES` + `ASM_OBJECTS`.

---

## OpenSSL Patch Application

NetBox maintains a local patch file `libs/openssl-3/0001-openssl-apply-NetBox-patches.patch` with MSVC/Win32 fixes.
After copying updated OpenSSL sources from upstream (e.g., WinSCP), **the patch must be re-applied**.

**Patch contents:** TSAN type casts (`volatile LONG*`), `FARPROC` fix in `cryptlib.c`,
platform detection (`_WIN32`/`_WIN64`/`_M_ARM64`), `OPENSSL_NO_*` defines,
directory path fixes, and rcu.h guard.

### How to Apply

From the **project root**:

```cmd
git -C libs\openssl-3 apply -p3 0001-openssl-apply-NetBox-patches.patch
```

**Important:** The `-p3` flag strips 3 path components from the patch. Always run from inside `libs/openssl-3/`.

### Verify the Patch Applied

```cmd
git apply -p3 --check 0001-openssl-apply-NetBox-patches.patch
```

Silent output = OK. Any "patch does not apply" message means upstream changed — manually inspect the hunk and re-create it.

### Common Failure After Update

If `git apply` skips patches, check context lines have changed. Use `--reject` to see what failed:

```cmd
git apply -p3 --reject 0001-openssl-apply-NetBox-patches.patch
```

Then manually fix rejected hunks by comparing with the patch diff.

### Critical Fixes (Break Win32 Build If Not Applied)

1. `crypto/cryptlib.c` line 45: `int (*f)(void)` → `FARPROC f` (calling convention mismatch on x86)
2. `include/crypto/bn_conf.h`: platform detection for `_WIN32` vs `_WIN64` vs `_M_ARM64`
3. `include/openssl/configuration.h`: `OPENSSL_SYS_WIN32`/`OPENSSL_SYS_WIN64A` defines

> The patch file should be versioned alongside the OpenSSL source. When updating OpenSSL, always check if the patch needs re-creation.

---

## Language Files

- English: `NetBoxEng.lng` (primary, always update this first)
- Other languages: `NetBoxRus.lng`, `NetBoxPol.lng`, etc.
- Use message IDs from `MsgIDs.h`
- Keep translations synchronized when modifying UI strings
