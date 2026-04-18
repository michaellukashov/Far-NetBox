# NetBox Developer Guide

This guide covers building NetBox from source and maintaining third-party dependencies.

## Prerequisites

- **Visual Studio 2022** with "Desktop development with C++" workload (includes MSVC and Windows SDK)
- **CMake** 3.15 or later
- **Ninja** (recommended) or you can use the Visual Studio generator

Ensure `cmake` and `ninja` are in your `PATH`.

## Building from Source

### Quick Build (Batch Files)

The repository provides batch files in the root directory to automate the build:

- `build-all.bat` - builds all supported platforms (x86, x64, ARM64)
- `build-x64.bat` - builds x64 release with debug info
- `build-x86.bat` - builds x86 release with debug info
- `build-arm64.bat` - builds ARM64 release with debug info

Run the desired batch file from the repository root. The scripts set up the Visual Studio environment and invoke CMake with appropriate settings.

**Note:** The batch files default to Visual Studio 2022 Professional. If you use a different edition (e.g., Community) or installation path, adjust the `vcvarsall.bat` path in the batch file.

### Manual Build

1. Open a command prompt and set up the Visual Studio environment:

   ```batch
   "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
   ```

   Or use the full path:

   ```batch
   "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
   ```

2. Configure and build using CMake (example for x64):

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   To generate a Visual Studio 2022 solution:

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   The built plugin will be in `build-RelWithDebugInfo\Plugins\NetBox\<platform>\`.

## Updating Third-Party Components

### Updating WinSCP

NetBox is based on the WinSCP codebase. When a new WinSCP version is released, follow these steps:

1. Download the new WinSCP source package (e.g., from https://winscp.net/eng/download.php).
2. Extract and copy the updated files into `libs/openssl-3/`. Typically you will copy all contents from WinSCP's `src/` directory, overwriting existing files.
   - You may use a tool like `robocopy` for a robust sync:
     ```batch
     robocopy /MIR path\to\winscp\src libs\openssl-3\
     ```
3. Update version strings:
   - In `src/NetBox/resource.h` (and `src/NetBox/resource.h.template`), change `WINSCP_VERSION_WTXT` to the new version (e.g., `L"6.5.6"`).
   - In language resource files (`src/NetBox/NetBox*.lng`), update the WinSCP version string (search for `"6.5.1"` and replace with `"6.5.6"`).
4. Rebuild the OpenSSL libraries (if needed) and the NetBox plugin to verify the changes.
5. Update documentation:
   - `docs/README.md` and `docs/i18n/*.md`: update the WinSCP version in the "Based on" line.
   - `docs/ARCHITECTURE.md`, `docs/CODEBASE.md`, `docs/PROJECT.md`, `docs/PROJECT_RULES.md`, `.ai-factory/DESCRIPTION.md`, `.ai-factory/AGENTS-Overview.md`: adjust any references to the WinSCP version.
   - `ChangeLog`: add an entry under `[Unreleased]` noting the WinSCP upgrade.
6. Commit the changes with a clear message (e.g., `chore(winscp): upgrade to WinSCP 6.5.6`).

### Updating OpenSSL

OpenSSL is vendored in `libs/openssl-3/`. To upgrade:

1. Obtain the new OpenSSL source (from WinSCP or directly from OpenSSL if WinSCP has already integrated it). Usually you'll replace the entire `libs/openssl-3/` tree with the new version from the WinSCP project.
2. Update the OpenSSL version header:
   - In `libs/openssl-3/include/openssl/opensslv.h`, change `OPENSSL_VERSION_MINOR` and `OPENSSL_VERSION_PATCH` to the new version (e.g., `3` and `5` → `5` and `6` for 3.3.7). Also update `OPENSSL_VERSION_STR` and `OPENSSL_FULL_VERSION_STR` to match (e.g., `"3.3.7"`). Keep `OPENSSL_VERSION_MAJOR` as 3.
3. Update NetBox's displayed version:
   - In `src/NetBox/resource.h` (and `resource.h.template`), change `OPENSSL_VERSION_WTXT` to the new version (e.g., `L"3.3.7"`).
   - In language resource files (`src/NetBox/NetBox*.lng`), update the OpenSSL version string (search for `"3.3.2"` and replace with `"3.3.7"`).
4. Rebuild both the OpenSSL libraries and the NetBox plugin. Ensure there are no compile errors; you may need to adjust `cmake/OpenSSL.cmake` if new files are added or removed (WinSCP usually handles this automatically).
5. Update documentation:
   - In `docs/README.md` and i18n READMEs, add/update the OpenSSL credit line (e.g., `Cryptography based on OpenSSL 3.3.7 Copyright (c) 1998-2025 The OpenSSL Project`).
   - Update any other references (e.g., `.ai-factory/DESCRIPTION.md` lists OpenSSL version).
   - `ChangeLog`: note the OpenSSL upgrade.
6. Commit as `chore(openssl): upgrade to OpenSSL 3.3.7`.

## Running Tests

If tests are present, run them via your IDE or command line. Ensure all tests pass before submitting changes.

## Submitting Changes

- Create a feature branch from `dev` (or `main` if more appropriate).
- Make your changes, following the project's coding conventions and logging requirements.
- Build and test your changes.
- Commit with clear, conventional commit messages.
- Create a pull request describing the changes and referencing any relevant issues.

## Notes

- The `libs/` directory contains third-party code. Do not modify files there directly unless upgrading the dependency as described. Use patches if you need to fix something in third-party code.
- Keep Windows XP compatibility in mind when upgrading dependencies and using APIs.
- For architecture details, see `docs/ARCHITECTURE.md`.
- For coding standards, see `AGENTS.md` and `.ai-factory/RULES.md`.