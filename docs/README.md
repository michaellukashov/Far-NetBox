Far-NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager 3.0 x86/x64/ARM64
==============

| Workflow       | Build status                                                                                                                                                                             |
| -------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Github Actions | [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg) |
| Appveyor       | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)                                     |

Based on [WinSCP](http://winscp.net/eng/index.php) version 6.5.1 Copyright (c) 2000-2025 Martin Prikryl

Based on [WinSCP as FAR Plugin: SFTP/FTP/SCP client for FAR version 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl

SSH and SCP code based on PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

How to build from source
-----------------------

### Prerequisites

* Visual Studio 2022 (with "Desktop development with C++" workload)
* CMake 3.15 or later
* Ninja (recommended; optional if using Visual Studio generator)

### Quick build (using batch files)

The repository provides batch files in the root directory to automate the build:

- `build-all.bat` - builds all supported platforms (x86, x64, ARM64)
- `build-x64.bat` - builds x64 release with debug info
- `build-x86.bat` - builds x86 release with debug info
- `build-arm64.bat` - builds ARM64 release with debug info

Simply run the desired batch file from the repository root. The scripts will set up the Visual Studio environment and invoke CMake with appropriate settings.

**Note:** The batch files reference Visual Studio 2022 Professional by default. If you use a different edition (e.g., Community) or installation path, adjust the `vcvarsall.bat` path accordingly within the batch file.

### Manual build

If you prefer to build manually, follow these steps:

1. Open a command prompt and set up the Visual Studio environment:

   ```batch
   "%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
   ```

   Or use the full path to your Visual Studio installation:

   ```batch
   "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
   ```

2. Configure and build using CMake (example for x64):

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   To generate a Visual Studio 2022 solution instead:

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   The built plugin will be located in `build-RelWithDebugInfo\Plugins\NetBox\x64\` (or the appropriate platform subfolder).

Links
-----

* Project main page: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Far-NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Far-NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39§t=6638)
* Latest builds: <https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview>

License
-------

Far-NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Far-NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.