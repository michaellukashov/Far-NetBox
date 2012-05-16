NetBox: SFTP/FTP/FTPS/SCP/WebDAV client for Far Manager 2.0/3.0
==============

    Based on WinSCP: SFTP/FTP/SCP client for FAR version 1.6.2 Copyright (c) 2000-2009 Martin Prikryl  
    Based on WinSCP version 5.0.7 Copyright (c) 2000-2012 Martin Prikryl  
    SSH and SCP code based on PuTTY 0.62 Copyright (c) 1997-2011 Simon Tatham  
    FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse  

How to install
==============

You can either download an appropriate binary package for your  
platform or build from source. Binaries can be obtained from [here](https://github.com/michaellukashov/Far-NetBox/downloads/). 

Unpack the archive in the plugin directory Far (... Far \ Plugins).

How to build from source
========================

To build plugin from source, you will need:


  * Visual Studio 2010  
  * Microsoft Platform SDK, available for download at [http://www.microsoft.com/msdownload/platformsdk/sdkupdate/](http://www.microsoft.com/msdownload/platformsdk/sdkupdate/).  
  * Perl 5 (to compile openssl), available at [http://www.activestate.com/ActivePerl/](http://www.activestate.com/ActivePerl/)  
  * UnxUtils [http://unxutils.sourceforge.net/](http://unxutils.sourceforge.net/)  
  * nasm [http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/](http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/win32/)



Download the source:

You can either download a release source zip ball from [tags page](https://github.com/michaellukashov/Far-NetBox/tags)  
and unpack it in your source directory, say C:/src,  
or from git repository:

    cd C:/src
    git clone git://github.com/michaellukashov/Far-NetBox.git

From now on, we assume that your source tree is C:/src/Far-NetBox


Compile boost libraries:
    
Boost 1.48.0 or later is required to build NetBox.  
You can download the source code for boost from [download page](http://sourceforge.net/projects/boost/files/boost/1.48.0/).

Unpack archive to directory C:/src/Far-NetBox/libs

Compile bjam:

    cd libs/boost/tools/build/v2/engine
    call build.bat

Copy the resulting bjam.exe binary (located in libs/boost/tools/build/v2/engine/bin.ntx86)  
to some directory which is listed in %PATH% variable, say C:/Windows

Create 'boost/stage' directory:

    mkdir libs/boost/stage


Compile 'boost.signals' library:

    cd libs/boost/libs/signals/build  
    call bjam variant=debug link=static threading=multi runtime-debugging=on runtime-link=static stage -j3  
    cp -R libs/boost/libs/signals/build/stage/* libs/boost/stage/  
    call bjam architecture=x86 address-model=64 variant=debug link=static threading=multi runtime-debugging=on runtime-link=static stage -j3  
    cp -R libs/boost/libs/signals/build/stage/* libs/boost/stage/x64


Compile openssl:

    cd libs/openssl  
    call src/NetBox/scripts/build_openssl.bat x86  
    call src/NetBox/scripts/build_openssl.bat x64  


Compile NetBox plugin:

    cmd /c %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86 && devenv NetBox.sln /Build "Debug|Win32" /USEENV /Project "NetBox"
    cmd /c %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86_amd64 && devenv NetBox.sln /Build "Debug|x64" /USEENV /Project "NetBox"


Links
========================

* Project main page: [https://github.com/michaellukashov/Far-NetBox/](https://github.com/michaellukashov/Far-NetBox/)
* Download page: [https://github.com/michaellukashov/Far-NetBox/downloads/](https://github.com/michaellukashov/Far-NetBox/downloads/)
* Far Manager forum: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* NetBox discussions (in Russian): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* NetBox discussions (in English): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39&t=6638)

Boring Copyright Stuff
========================

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


