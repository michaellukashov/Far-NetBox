[← Architecture](architecture.md) · [Back to README](../README.md)

# OpenSSL Sync and Cleanup Report

## Summary

| Attribute | Value |
|---|---|
| **Date** | 2026-04-17 |
| **Branch** | `feature/openssl-sync-cleanup` |
| **WinSCP Baseline** | 6.5.6 (OpenSSL 3.3.7) |
| **NetBox Before** | ~2067 files, ~55 MB |
| **NetBox After** | ~1767 files, ~43 MB |
| **Files Removed** | 300 |
| **Space Freed** | ~12 MB |
| **Patch Status** | Applied cleanly |

## Inventory

### Before Cleanup

| Category | Count |
|---|---|
| Total files in `libs/openssl-3/` | ~2067 |
| `.c` source files | ~900 |
| `.h` header files | ~600 |
| `.asm` assembly files | ~80 |
| Documentation/config | ~100 |

### After Cleanup

| Category | Count |
|---|---|
| Total files in `libs/openssl-3/` (excl. `removed/`) | ~1767 |
| `.c` source files | ~750 |
| `.h` header files | ~500 |
| `.asm` assembly files | ~50 |

## Sync Results

### Files Removed (Top Categories)

| Directory | Files Removed | Rationale |
|---|---|---|
| `crypto/asn1/` | 23 | Unused ASN.1 parsers/serializers |
| `crypto/bn/` | 22 | Unused big-num optimizations (platform-specific) |
| `crypto/ec/` | 17 | Unused EC curves and platform asm |
| `crypto/sha/` | 13 | Unused SHA variants and platform asm |
| `crypto/aes/` | 12 | Unused AES asm for non-target platforms |
| `providers/implementations/` | 34 | Unused provider implementations |
| `ssl/quic/` | 40+ | QUIC protocol support removed |
| `crypto/engine/` | 20+ | Engine API deprecated, unused |

### Major Removals

1. **QUIC Protocol Support** (`ssl/quic/*`, `crypto/bio/bss_dgram*`)
   - NetBox does not use QUIC/HTTP3
   - Removed ~40 files, ~8000 lines

2. **Engine API** (`crypto/engine/*`)
   - Deprecated in OpenSSL 3.0
   - Removed ~20 files

3. **Platform-Specific ASM** (non-x86/x64)
   - ARM64, PPC, S390X, RISC-V asm removed
   - x86/x64 ASM retained for target platforms

4. **Unused Crypto Algorithms**
   - Camellia, SEED, RC5, MDC2, Whirlpool
   - CMS, CMP, CRMF, OCSP, TS

5. **Documentation**
   - `ACKNOWLEDGEMENTS.md`, `AUTHORS.md`, `INSTALL.md`, `LICENSE.txt`, `SUPPORT.md`
   - `Makefile` (CMake used instead)

## Patch Application

| Patch | Status |
|---|---|
| `0001-openssl-NetBox-patches.patch` | Applied cleanly |
| Conflicts | None |
| Rejections | None |

### Patch Contents

- Windows compatibility fixes (`_stricmp`, `_strnicmp` mappings)
- Build flag adjustments (`DSO_WIN32`, `MK1MF_BUILD`)
- Warning suppression macros
- Threading model alignment with NetBox

## Build Verification

| Check | Status | Notes |
|---|---|---|
| CMake configure | ✅ Pass | Linux (Ninja generator) — validates CMake syntax and file references |
| MSVC W4 build | ⏭️ Pending | Requires Windows + Visual Studio 2022 |
| Plugin load test | ⏭️ Pending | Requires Far Manager on Windows |

**CMake Configure Output:**
```
-- CMake version: 3.31.11
-- System: Linux, PROJECT_PLATFORM: x64
-- Configuring done (0.8s)
-- Generating done (0.1s)
```

No CMake errors or missing file references detected after removal.

## NetBox-Only Files

Files present in NetBox but not in WinSCP baseline:

| File | Purpose |
|---|---|
| `CMakeLists.txt` | NetBox-specific build configuration |
| `0001-openssl-NetBox-patches.patch` | NetBox compatibility patches |
| `crypto/initthread.c` | Threading customization |
| `crypto/o_str.c` | String utility customizations |

These files are compiled and required for NetBox functionality.

## Commit History

| Commit | Description |
|---|---|
| `c7e87de8e` | Remove QUIC support and add warning suppressions |
| `acaa3c2ee` | Remove unused OpenSSL files and fix threading |
| `d42c90568` | Remove unused files and sync with WinSCP baseline |
| `444a2f3f4` | Document analysis and cleanup plan |

## Recommendations

1. **Windows Build Verification:** Run `build-x64.bat` on Windows with `OPT_CREATE_PLUGIN_DIR=ON` to confirm zero warnings and plugin loads.
2. **Future Syncs:** When syncing with future WinSCP releases, use the `removed/` directory as a reference for what to exclude.
3. **Patch Maintenance:** The `0001-openssl-NetBox-patches.patch` should be regenerated if WinSCP baseline changes significantly.

## Rollback

If build issues are discovered on Windows:

```bash
cd libs/openssl-3
# Restore removed files
rsync -a removed/ .
```

Full backup of removed files is preserved in `libs/openssl-3/removed/`.


## See Also

- [Architecture](architecture.md) — Project structure and third-party dependencies
- [Contributing](contributing.md) — Build instructions and development workflow
