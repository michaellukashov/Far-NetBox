# NetBox CMake Configuration

This document describes the modular CMake structure for NetBox.

## Directory Structure

```
cmake/
├── README.md                 # This file
├── ucm.cmake                 # Utility CMake macros (external)
├── copy_file.cmake           # File copy utilities
│
├── OpenSSL.cmake             # OpenSSL configuration
├── NetBox.cmake              # NetBox compiler/linker flags
├── Install.cmake             # Post-build installation logic
│
├── PuTTY.cmake               # PuTTY SSH library configuration
├── Neon.cmake                # WebDAV protocol library
├── zlib-ng.cmake             # Compression library
├── Expat.cmake               # XML parsing library
├── TinyXML2.cmake            # XML parser
├── Libs3.cmake               # S3 protocol library
├── DLMalloc.cmake            # Memory allocator
├── TinyLog.cmake             # Logging library
├── FMT.cmake                 # String formatting library
└── ATLMFC.cmake              # Minimal MFC subset
```

## File Descriptions

### Core Configuration Files

#### `ucm.cmake`
Utility CMake macros from the UCM (Useful CMake Macros) project. Provides functions for:
- Setting compiler/linker flags
- Managing runtime library settings
- Adding/removing flags per configuration

#### `copy_file.cmake`
Utility for copying files during post-build steps. Used by `Install.cmake`.

#### `NetBox.cmake`
Centralized compiler, linker, and platform-specific configuration.

**Variables:**
- `LIBNEON_DEFS` - WebDAV library defines
- `LIBEXPAT_DEFS` - XML parser defines
- `NETBOX_DEFS` - Main project defines (~20 items)
- `NETBOX_C_FLAGS`, `NETBOX_CXX_FLAGS` - C/C++ compiler flags
- `NETBOX_PLATFORM_FLAGS` - Platform-specific flags (x64, x86, ARM64)
- `NETBOX_UNICODE_FLAGS` - Unicode-related defines
- `NETBOX_DLL_LINK_FLAGS` - Linker flags for DLL builds

**Functions:**
- `netbox_apply_compile_options(TARGET)` - Apply compile options to a target
- `netbox_apply_link_options(TARGET)` - Apply linker options to a target
- `netbox_get_include_dirs(RESULT_VAR)` - Get standard include directories
- `netbox_compile_asm_files()` - Compile ASM files using NASM (MSVC only)

#### `Install.cmake`
Post-build installation and plugin directory setup.

**Functions:**
- `netbox_setup_plugin_installation(TARGET)` - Sets up plugin directory installation
- `netbox_add_post_build_customizations(TARGET)` - Applies MSVC post-build linker flags

### Library Configuration Files

Each library has its own `.cmake` file with:
- Compile flags (`*_COMPILE_FLAGS` or `*_DEFS`)
- Source organization functions (`*_get_sources()`)
- Include directory functions (`*_get_include_dirs()`)
- Compile option functions (`*_apply_compile_options()`)

#### `OpenSSL.cmake`
OpenSSL configuration for building `ssleay32` and `libeay32` libraries.

#### `PuTTY.cmake`
PuTTY SSH library configuration for building `putty` and `puttyvs` libraries.

#### `Neon.cmake`
WebDAV protocol library configuration.

#### `zlib-ng.cmake`
Compression library configuration with platform-specific sources (NEON, SSE2, AVX2, AVX512).

#### `Expat.cmake`
XML parsing library configuration.

#### `TinyXML2.cmake`
XML parser configuration.

#### `Libs3.cmake`
S3 protocol library configuration.

#### `DLMalloc.cmake`
Memory allocator configuration.

#### `TinyLog.cmake`
Logging library configuration.

#### `FMT.cmake`
String formatting library configuration.

#### `ATLMFC.cmake`
Minimal MFC subset configuration.

## Adding a New Library

1. Create `cmake/NewLib.cmake` with:
   - `NEWLIB_COMPILE_FLAGS` - Compile flags
   - `newlib_get_sources(RESULT_VAR)` - Source file discovery
   - `newlib_apply_compile_options(TARGET)` - Apply flags to target
   - `newlib_get_include_dirs(RESULT_VAR)` - Include directories

2. Create `libs/newlib/CMakeLists.txt`:
   ```cmake
   cmake_minimum_required(VERSION 3.15)
   include(${CMAKE_SOURCE_DIR}/cmake/ucm.cmake)
   include(${CMAKE_SOURCE_DIR}/cmake/NewLib.cmake)

   newlib_get_sources(NEWLIB_SOURCES)
   add_library(newlib STATIC ${NEWLIB_SOURCES})
   target_include_directories(newlib PRIVATE ...)
   newlib_apply_compile_options(newlib)
   ```

3. Add to `CMakeLists.txt`:
   ```cmake
   add_subdirectory(libs/newlib)
   ```

4. Link to `NetBox` in `src/CMakeLists.txt`:
   ```cmake
   set(NETBOX_LIBRARIES ${NETBOX_LIBRARIES} newlib)
   ```

## Build System Flow

```
CMakeLists.txt (main)
  ├── cmake/ucm.cmake (utilities)
  ├── cmake/NetBox.cmake (flags)
  │
  ├── libs/openssl-3/CMakeLists.txt
  │   └── cmake/OpenSSL.cmake
  │
  ├── libs/PuTTY/CMakeLists.txt
  │   └── cmake/PuTTY.cmake
  │
  ├── ... (other libraries)
  │
  └── src/CMakeLists.txt
      ├── cmake/NetBox.cmake (flags)
      └── cmake/Install.cmake (installation)
```

## Updating Flags

To modify compiler flags or defines:

1. **Global flags**: Edit `cmake/NetBox.cmake`
2. **Library-specific flags**: Edit the corresponding `cmake/[Lib].cmake`
3. **Plugin-specific flags**: Modify `src/CMakeLists.txt` directly

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `PROJECT_PLATFORM` | x86/x64/ARM64 | Target platform |
| `CMAKE_BUILD_TYPE` | Debug | Build type (Debug/Release/RelWithDebugInfo) |
| `OPT_USE_UNITY_BUILD` | OFF (x86 Release only) | Enable unity builds |
| `OPT_CREATE_PLUGIN_DIR` | OFF | Create plugin directory structure |

## Troubleshooting

### CMake Configuration Errors

If you see errors like "add_subdirectory given source which is not an existing directory":
- Verify the library directory exists in `libs/`
- Check that `libs/*/CMakeLists.txt` exists
- Ensure the directory name matches exactly (case-sensitive on Linux)

### Build Errors

If targets fail to build:
- Check that all `include()` statements reference existing files
- Verify library targets are built before dependent targets
- Check that include directories are properly set
