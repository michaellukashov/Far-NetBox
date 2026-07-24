[Back to README](../README.md) · [User Guide →](user-guide.md)

# Getting Started

Build Far-NetBox from source and install it as a Far Manager 3.0 plugin.

## Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| Windows | 7+ | WinXP compatibility maintained; see [`.ai-factory/windows-xp-compatibility.md`](../.ai-factory/windows-xp-compatibility.md) |
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

## Troubleshooting

### Transfer Cancellation Hang (Fixed in 2026-04)

**Symptom**: Pressing `Esc` during a file transfer does not immediately cancel; the cancellation dialog becomes unresponsive.

**Cause**: Interaction between the speed-limit (CPS) throttle and the cancellation dialog event loop.

**Fix**: Update to NetBox 2026-04 or newer. The cancellation path now correctly breaks out of the CPS throttle loop.

**Workaround for older builds**: Wait for the current file chunk to complete (~1–2 seconds at most), then press `Esc` again.

### SSH/SCP Buffer Corruption on High-Speed Transfers (Fixed in 2026-04)

**Symptom**: Large file transfers over SSH or SCP abort with "connection reset" or produce corrupted files.

**Cause**: PuTTY's dynamic send buffer (`SendBuf`) could overflow during high-speed transfers, causing packet corruption.

**Fix**: NetBox 2026-04 sets the default `SendBuf` to `0` (dynamic buffer disabled). Existing sessions automatically pick up the new default on the next connection.

**Manual tuning**: If you need higher throughput on high-latency links, increase `SendBuf` incrementally (e.g., `262144`, `524288`) in the session **SSH** → **Advanced** settings. Monitor for corruption.

### Far Manager Version Too Old for Folder History

**Symptom**: `Alt+F12` shows the session name but does not restore the remote directory.

**Cause**: Far Manager older than 3.0.5955 does not expose `FCTL_GETPANELDIRECTORY` / `FCTL_SETPANELDIRECTORY`.

**Fix**: Update Far Manager to 3.0.5955 or newer.

### OpenSSL Assembly Build Errors

**Symptom**: `cmake --build` fails with NASM errors on x86 or x64.

**Cause**: NASM is not installed or not in `PATH`.

**Fix**: Install NASM 2.15+ and ensure `nasm.exe` is accessible. For CI builds, the pre-built `buildtools/tools/nasm.exe` is used automatically.

### Unity Build Symbol Conflicts

**Symptom**: Build fails with "redefinition of ..." errors in Release mode.

**Cause**: Unity build (`OPT_USE_UNITY_BUILD=ON`) merges translation units that define the same static symbols.

**Fix**: Disable unity build for the affected configuration:

```cmd
cmake -S . -B build-RelWithDebugInfo -DOPT_USE_UNITY_BUILD=OFF
```

## Next Steps

- [User Guide](user-guide.md) — Supported protocols and features
- [Architecture](architecture.md) — Understand the codebase structure
- [Contributing](contributing.md) — Set up for development

## See Also

- [User Guide](user-guide.md) — Protocol reference and internationalization
- [Architecture](architecture.md) — Layered plugin architecture

