Far-NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager 3.0 x86/x64/ARM64
==============

| Workflow | Build status |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Github Actions | [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg) |
| Appveyor | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox) |

Based on [WinSCP](http://winscp.net/eng/index.php) version 6.5.1 Copyright (c) 2000-2025 Martin Prikryl

Based on [WinSCP as FAR Plugin: SFTP/FTP/SCP client for FAR version 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl

SSH and SCP code based on PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

How to build from source
-----------------------

To build plugin from source, you will need:

* Visual Studio 2022 build tools
* CMake 3.15
* Ninja (optional)

Download the source:

```batch
cd C:/src
git clone https://github.com/michaellukashov/Far-NetBox.git
```

From now on, we assume that your source tree is C:/src/Far-NetBox

Compile Far-NetBox plugin on the command line as follows:

```batch
"%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
```

Or use the full path to Visual Studio 2022 Professional:

```batch
"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
```

Then configure and build:

```batch
cmake -S C:/src/Far-NetBox -B C:/build/Far-NetBox -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build C:/build/Far-NetBox -j
```

You can generate solution for VS2022 IDE:

```batch
cmake -S C:/src/Far-NetBox -B C:/build/Far-NetBox -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build C:/build/Far-NetBox -j
```

## FTP Configuration

### FTP Heartbeat (Keepalive)

To prevent FTP server timeouts during idle periods, Far-NetBox supports sending periodic heartbeat messages on the control connection. This is especially useful when:

- Working with FTP servers that have short timeout periods (typically 30-120 seconds)
- Behind NAT devices or firewalls that may drop idle connections
- Performing long-running operations with periods of inactivity

#### Heartbeat Types

Far-NetBox provides two heartbeat methods:

1. **NOOP Command** (Recommended)
   - Sends standard FTP `NOOP` command (RFC 959)
   - Semantically correct for keepalive purposes
   - Minimal server-side processing
   - Configuration: `FtpPingType = 4` (fptNoop)

2. **Dummy Commands** (Legacy)
   - Sends random commands: `PWD`, `REST 0`, `TYPE A`, or `TYPE I`
   - Workaround for servers with NOOP issues (rare)
   - Configuration: `FtpPingType = 2` (fptDummyCommand)

#### Configuration

**XML Session Configuration:**

```xml
<Session>
  <!-- Enable NOOP heartbeat every 30 seconds -->
  <FtpPingType>4</FtpPingType>
  <FtpPingInterval>30</FtpPingInterval>
</Session>
```

**FtpPingType values:**
- `0` - Off (no heartbeat)
- `1` - Dummy command (legacy, same as value 2)
- `2` - Dummy command (random PWD/REST 0/TYPE A/TYPE I)
- `3` - Directory listing (heavyweight, not recommended)
- `4` - NOOP command (recommended)

**FtpPingInterval:**
- Time in seconds between heartbeat messages
- Default: `30` seconds
- Recommended range: `15-120` seconds

#### How It Works

The heartbeat mechanism:

1. **Timer-based**: Checks every 1 second via FileZilla engine timer
2. **Interval check**: Sends heartbeat when configured interval elapses
3. **Conditional**: Only sends when:
   - Connection is established and idle
   - No active file transfers
   - Heartbeat type is not "Off"
4. **Logging**: DEBUG-level logging via `ADF()` macro (visible in debug builds)

#### Example Scenarios

**Scenario 1: Prevent timeout on aggressive server (30s timeout)**
```xml
<FtpPingType>4</FtpPingType>
<FtpPingInterval>20</FtpPingInterval>
```

**Scenario 2: NAT firewall with 2-minute timeout**
```xml
<FtpPingType>4</FtpPingType>
<FtpPingInterval>90</FtpPingInterval>
```

**Scenario 3: Disable heartbeat (default)**
```xml
<FtpPingType>0</FtpPingType>
```

#### Troubleshooting

**Issue**: Connection still times out

- **Solution**: Decrease `FtpPingInterval` to less than server timeout
- **Example**: If server timeout is 60s, use `FtpPingInterval=45`

**Issue**: Server doesn't support NOOP

- **Solution**: Switch to dummy commands: `<FtpPingType>2</FtpPingType>`

**Issue**: Excessive logging noise

- **Solution**: Heartbeat logs are DEBUG-level only; disable debug output in production

#### Technical Notes

- **RFC 959**: NOOP is the standard FTP keepalive command
- **Implementation**: Uses FileZilla engine's existing timer infrastructure
- **Performance**: Minimal impact - one command per interval (typically 30s)
- **Compatibility**: Works with all standard FTP servers; NOOP is universally supported

#### Implementation Details

For developers, the heartbeat implementation:

- **Files modified**:
  - `src/core/SessionData.h` - Enum and configuration
  - `src/filezilla/FileZillaOpt.h` - Option constant
  - `src/core/FtpFileSystem.cpp` - Option handler
  - `src/filezilla/FtpControlSocket.cpp` - Heartbeat logic

- **Code flow**:
  ```cpp
  // Called every 1 second by MainThread timer
  CFtpControlSocket::OnTimer()
    → Checks OPTION_KEEPALIVE
    → Checks elapsed time >= interval
    → Calls SendKeepAliveCommand()
      → If fptNoop: sends "NOOP"
      → Else: sends random dummy command
  ```

For more information, see:
- [RFC 959 - FTP Specification](https://tools.ietf.org/html/rfc959)
- [FileZilla Engine Documentation](src/filezilla/README.md)

Links
-----

* Project main page: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Far-NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Far-NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)
* Latest builds: <https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview>

License
-------

Far-NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Far-NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.
