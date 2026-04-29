[Back to README](../README.md) · [User Guide →](user-guide.md)

# Getting Started

Build Far-NetBox from source and install it as a Far Manager 3.0 plugin.

## Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| Windows | 7+ | WinXP compatibility maintained |
| Visual Studio | 2022 (17.x) | Community, Professional, or Enterprise |
| CMake | 3.15+ | Ninja generator recommended |
| NASM | 2.15+ | For OpenSSL assembly (x86/x64) |
| Far Manager | 3.0+ | [farmanager.com](https://www.farmanager.com/) |

## Build

Pre-configured batch scripts handle the full configure-and-build cycle:

| Script | Platform | Config |
|--------|----------|--------|
| `build-x64.bat` | x64 (AMD64) | RelWithDebugInfo |
| `build-x86.bat` | x86 (Win32) | RelWithDebugInfo |
| `build-arm64.bat` | ARM64 | RelWithDebugInfo |
| `build-all.bat` | All three | RelWithDebugInfo |

```cmd
:: Single platform
cmd /c build-x64.bat

:: All platforms
cmd /c build-all.bat
```

Each script:
1. Calls `vcvarsall.bat` to set up the MSVC environment
2. Runs CMake configure with `-DOPT_CREATE_PLUGIN_DIR=ON`
3. Builds with Ninja and installs the plugin DLL to `Far3_<platform>/Plugins/NetBox/`

### Visual Studio Solution

Generate a VS solution for IDE debugging:

```cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
cmake -S . -B build-VS -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DOPT_CREATE_PLUGIN_DIR=ON
```

Open `build-VS\NetBox.sln`. Set debug command to `Far3_x64\Far.exe` in project properties.

### Clean Rebuild

```cmd
rmdir /s /q build-RelWithDebugInfo
cmd /c build-x64.bat
```

## Install

The build script places the plugin automatically when `OPT_CREATE_PLUGIN_DIR=ON`:

```
Far3_x64/
  Plugins/
    NetBox/
      NetBox.dll
      NetBox.map
      NetBoxEng.lng
      ...
```

Copy `Far3_x64/` next to your Far Manager installation, or launch `Far3_x64\Far.exe` directly.

## First Run

1. Launch Far Manager
2. Press `F11` to open the plugins menu
3. Select **NetBox**
4. Create a new session (`Shift+F4`) or open an existing one
5. Enter host, protocol, credentials, and connect

Session data is stored in `%LOCALAPPDATA%\NetBox\`.

## Verify It Works

After connecting, you should see the remote file listing in a Far panel. Try:

- Navigating directories with `Enter` and `Ctrl+PgUp`
- Copying a file with `F5`
- Editing a remote file with `F4`

Check the log at `%LOCALAPPDATA%\NetBox\netbox.log` if something goes wrong.

## Next Steps

- [User Guide](user-guide.md) — Supported protocols and features
- [Architecture](architecture.md) — Understand the codebase structure
- [Contributing](contributing.md) — Set up for development

## See Also

- [User Guide](user-guide.md) — Protocol reference and internationalization
- [Architecture](architecture.md) — Layered plugin architecture
