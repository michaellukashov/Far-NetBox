Far-NetBox: SFTP/FTP/SCP/WebDAV/S3 client for Far Manager 3.0 x86/x64/ARM64
==============

| Workflow                    | Build status  |
| --------------------------- | ------------- |
| Github Actions              | [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)  |
| Appveyor                    | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)  |


Based on [WinSCP](http://winscp.net/eng/index.php) version 6.3.3 Copyright (c) 2000-2024 Martin Prikryl  

Based on [WinSCP as FAR Plugin: SFTP/FTP/SCP client for FAR version 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl  

SSH and SCP code based on PuTTY 0.80 Copyright (c) 1997-2023 Simon Tatham  

FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse  

How to build from source
========================

To build plugin from source, you will need:  

  * Visual Studio 2022 build tools
  * CMake 3.15
  * Ninja (optional)

Download the source:

```
cd C:/src
git clone https://github.com/michaellukashov/Far-NetBox.git
```

From now on, we assume that your source tree is C:/src/Far-NetBox

Compile Far-NetBox plugin on the command line as follows:

```
"%VS170COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
cmake -S C:/src/Far-NetBox -B C:/build/Far-NetBox -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build C:/build/Far-NetBox
```

You can generate solution for VS2022 IDE:
```
cmake -S C:/src/Far-NetBox -B C:/build/Far-NetBox -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
cmake --build C:/build/Far-NetBox
```


Links
========================

* Project main page: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Far-NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Far-NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)
* Latest builds:
https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview

License
========================

Far-NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.  

Far-NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.
