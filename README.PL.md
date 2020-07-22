﻿NetBox: Klient SFTP/FTP/FTP(S)/SCP/WebDAV dla Far Manager 2.0/3.0 x86/x64
==============

[![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)


Bazuje na [WinSCP](http://winscp.net/eng/index.php) version 5.11.2 Copyright (c) 2000-2017 Martin Prikryl  
Bazuje na [WinSCP jako wtyczka FAR: klient SFTP/FTP/SCP dla FAR wersja 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl  
Kod SSH i SCP bazuje na PuTTY 0.70 Copyright (c) 1997-2017 Simon Tatham  
Kod FTP bazuje na FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse  

Jak zainstalować
==============

You can either download an appropriate binary package for your  
platform or build from source. Binaries can be obtained from [here](http://plugring.farmanager.com/plugin.php?pid=859&l=en). 

Rozpakuj archiwum do folderu wtyczek w folderze Far (... Far \ Plugins).

Jak skompilować ze źrdeł
========================

To build plugin from source, you will need:


  * Visual Studio 2010 SP1   
  * Microsoft Platform SDK, available for download at [http://www.microsoft.com/msdownload/platformsdk/sdkupdate/](http://www.microsoft.com/msdownload/platformsdk/sdkupdate/).  
  * Perl 5 (to compile openssl), available at [http://www.activestate.com/ActivePerl/](http://www.activestate.com/ActivePerl/)  
  * UnxUtils [http://unxutils.sourceforge.net/](http://unxutils.sourceforge.net/)  
  * nasm [http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/](http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/)



Pobierz źródło:

You can either download a release source zip ball from [tags page](https://github.com/michaellukashov/Far-NetBox/tags)  
and unpack it in your source directory, say C:/src,  
or from git repository:

    cd C:/src
    git clone git://github.com/michaellukashov/Far-NetBox.git

From now on, we assume that your source tree is C:/src/Far-NetBox


Kompilacja openssl:

    cd libs/openssl  
    call ../../src/NetBox/scripts/build_openssl.bat x86  
    call ../../src/NetBox/scripts/build_openssl.bat x64  

Either open src/NetBox/NetBox.sln in Visual Studio, or compile NetBox plugin on the command line as follows:

    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86 && devenv NetBox.sln /Build "Release|Win32" /USEENV /Project "NetBox"
    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64 && devenv NetBox.sln /Build "Release|x64" /USEENV /Project "NetBox"

Replace Release with Debug to build in Debug mode. The built binaries will be in Far2_x86/Plugins/NetBox/ (or Far2_x64/Plugins/NetBox/) directory.


Linki
========================

* Główna strona projektu: [https://github.com/michaellukashov/Far-NetBox/](https://github.com/michaellukashov/Far-NetBox/)
* Strona pobierania:
[https://github.com/michaellukashov/Far-NetBox/releases](https://github.com/michaellukashov/Far-NetBox/releases) [http://plugring.farmanager.com/plugin.php?pid=859&l=en](http://plugring.farmanager.com/plugin.php?pid=859&l=en)
* Forum Far Manager: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Dyskusja o NetBox (po rosyjsku): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Dyskusja o NetBox (po angielsku): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)

Licencja
========================

NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.  
NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.  
