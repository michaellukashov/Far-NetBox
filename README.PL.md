NetBox: Klient SFTP/FTP/FTP(S)/SCP/WebDAV dla Far Manager 3.0 x86/x64
==============

[![Build status](https://ci.appveyor.com/api/projects/status/rc32omfcxkhn7kfk?svg=true)](https://ci.appveyor.com/project/FarGroup/far-netbox)


Bazuje na [WinSCP](http://winscp.net/eng/index.php) wersja 5.11.2 Copyright (c) 2000-2017 Martin Prikryl
Bazuje na [WinSCP jako wtyczka FAR: Klient SFTP/FTP/SCP dla FAR wersja 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl
Kod SSH i SCP bazuje na PuTTY 0.70 Copyright (c) 1997-2017 Simon Tatham
Kod FTP bazuje na FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

Jak skompilować ze źródeł
========================

Aby skompilować wtyczkę, będziesz potrzebować:


  * Visual Studio 2010 SP1
  * Microsoft Platform SDK, do pobrania ze strony [http://www.microsoft.com/msdownload/platformsdk/sdkupdate/](http://www.microsoft.com/msdownload/platformsdk/sdkupdate/).
  * Perl 5 (do kompilacji openssl), do pobrania ze strony [http://www.activestate.com/ActivePerl/](http://www.activestate.com/ActivePerl/)
  * UnxUtils [http://unxutils.sourceforge.net/](http://unxutils.sourceforge.net/)
  * nasm [http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/](http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/)



Pobierz źródło:

    cd C:/src
    git clone https://github.com/FarGroup/Far-NetBox.git

Od teraz przyjmujemy, że źródło znajduje się w folderze C:/src/Far-NetBox


Kompilacja openssl:

    cd libs/openssl
    call ../../src/NetBox/scripts/build_openssl.bat x86
    call ../../src/NetBox/scripts/build_openssl.bat x64

Teraz otwórz src/NetBox/NetBox.sln w Visual Studio, lub skompiluj wtyczkę NetBox z linii poleceń:

    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86 && devenv NetBox.sln /Build "Release|Win32" /USEENV /Project "NetBox"
    cmd /c "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64 && devenv NetBox.sln /Build "Release|x64" /USEENV /Project "NetBox"

Zamień 'Release' na 'Debug', aby skompilować w trybie debugowania. Skompilowanie binaria znajdą się w folderze build/Release/x86 (lub build/Release/x84).


Linki
========================

* Strona główna projektu: [https://github.com/FarGroup/Far-NetBox](https://github.com/FarGroup/Far-NetBox)
* Forum Far Manager: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Dyskusja o NetBox (po rosyjsku): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Dyskusja o NetBox (po angielsku): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)

Licencja
========================

NetBox is [free](http://www.gnu.org/philosophy/free-sw.html) software: you can use it, redistribute it and/or modify it under the terms of the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
NetBox is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. See the [GNU General Public License](http://www.gnu.org/licenses/gpl.html) for more details.
