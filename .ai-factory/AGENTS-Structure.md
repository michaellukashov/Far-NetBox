# AGENTS-Structure.md — Build Structure Details

> Part of the AGENTS documentation series. See [AGENTS.md](../AGENTS.md) for the main entry point.

## CMake Structure

```
CMakeLists.txt                    # Main entry (~74 lines)
├── cmake/
│   ├── NetBox.cmake              # Compiler/linker flags
│   ├── Libraries.cmake           # Centralized library config
│   ├── PlatformDetection.cmake   # Platform auto-detection
│   └── Install.cmake             # Post-build installation
├── libs/*/CMakeLists.txt         # Third-party library builds
└── src/CMakeLists.txt            # Plugin target (~530 lines)
```

## CMake Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug`, `Release`, `RelWithDebugInfo` | `Debug` | Build configuration |
| `PROJECT_PLATFORM` | `x86`, `x64`, `ARM64` | Auto-detected | Target architecture |
| `OPT_CREATE_PLUGIN_DIR` | `ON`, `OFF` | `OFF` | Create plugin directory structure |
| `OPT_USE_UNITY_BUILD` | `ON`, `OFF` | `ON` (x86 Release only) | Faster compilation |
| `OPT_COMPILE_COMMANDS` | `ON`, `OFF` | `OFF` | Generate `compile_commands.json` |

## Adding a New Library

1. Create `cmake/NewLib.cmake` with library configuration
2. Create `libs/newlib/CMakeLists.txt` for the build
3. Add `add_subdirectory(libs/newlib)` in root `CMakeLists.txt`
4. Link to plugin: add `newlib` to `NETBOX_LIBRARIES` in `src/CMakeLists.txt`

## OpenSSL NASM Assembly

OpenSSL uses platform-specific assembly files for optimized crypto primitives (SHA, AES, BN, etc.).
The build system compiles them with NASM via `cmake/OpenSSL.cmake` → `openssl_setup_asm_files()`.

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

⚠️ The patch file should be versioned alongside the OpenSSL source. When updating OpenSSL, always check if the patch needs re-creation.

## Language Files

- English: `NetBoxEng.lng` (primary, always update this first)
- Other languages: `NetBoxRus.lng`, `NetBoxPol.lng`, etc.
- Use message IDs from `MsgIDs.h`
- Keep translations synchronized when modifying UI strings

## Build Output

| Artifact | Location |
|----------|----------|
| Plugin DLLs | `Far3_<platform>/Plugins/NetBox/` |
| Platform dirs | `Far3_x86/`, `Far3_x64/`, `Far3_ARM64/` |

**Requires:** `OPT_CREATE_PLUGIN_DIR=ON` in CMake configuration.

**Note:** Plugin DLLs go to `Far3_<platform>/Plugins/NetBox/` — **not** `build-*/src/`.
