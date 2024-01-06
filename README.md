NetBox: SFTP/FTP/FTP(S)/SCP/WebDAV client for Far Manager 3.0 x86/x64
==============

[![Build status](https://ci.appveyor.com/api/projects/status/rc32omfcxkhn7kfk?svg=true)](https://ci.appveyor.com/project/FarGroup/far-netbox)


Based on [WinSCP](http://winscp.net/eng/index.php) version 5.11.2 Copyright (c) 2000-2017 Martin Prikryl
Based on [WinSCP as FAR Plugin: SFTP/FTP/SCP client for FAR version 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl
SSH and SCP code based on PuTTY 0.70 Copyright (c) 1997-2017 Simon Tatham
FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

How to build from source
========================

To build plugin from source, you will need:


  * Visual Studio 2010 SP1
  * Microsoft Platform SDK, available for download at [http://www.microsoft.com/msdownload/platformsdk/sdkupdate/](http://www.microsoft.com/msdownload/platformsdk/sdkupdate/).
  * Perl 5 (to compile openssl), available at [http://www.activestate.com/ActivePerl/](http://www.activestate.com/ActivePerl/)
  * UnxUtils [http://unxutils.sourceforge.net/](http://unxutils.sourceforge.net/)
  * nasm [http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/](http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/)



Download the source:

    cd C:/src
    git clone https://github.com/FarGroup/Far-NetBox.git

From now on, we assume that your source tree is C:/src/Far-NetBox


Compile openssl:

    cd libs/openssl
    call ../../src/NetBox/scripts/build_openssl.bat x86
    call ../../src/NetBox/scripts/build_openssl.bat x64

Either open src/NetBox/NetBox.sln in Visual Studio, or compile NetBox plugin on the command line as follows:

    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86 && devenv NetBox.sln /Build "Release|Win32" /USEENV /Project "NetBox"
    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64 && devenv NetBox.sln /Build "Release|x64" /USEENV /Project "NetBox"

Replace Release with Debug to build in Debug mode. The built binaries will be in build/Release/x86 (or build/Release/x84) directory.


Links
========================

* Project main page: [https://github.com/FarGroup/Far-NetBox](https://github.com/FarGroup/Far-NetBox)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)

License
========================

NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.
