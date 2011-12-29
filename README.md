NetBox: SFTP/FTP/FTPS/SCP/WebDAV client for Far Manager 2.0
==============

WinSCP: SFTP/FTP/SCP client for FAR version 1.6.2 Copyright (c) 2000-2009 Martin Prikryl
Based on WinSCP version 4.1.9.0 Copyright (c) 2000-2009 Martin Prikryl
SSH and SCP code based on PuTTY 0.61 Copyright (c) 1997-2011 Simon Tatham
FTP code based on FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

HOW TO INSTALL
==============

You can either download an appropriate binary package for your
platform or build from source. Binaries can be obtained from
[here](https://github.com/michaellukashov/Far-NetBox/downloads/).

Unpack the archive in the plugin directory Far (... Far \ Plugins).

HOW TO BUILD FROM SOURCE
========================

1.  Download the source:

    You can either download a release source zip ball from the [tags
    page](https://github.com/michaellukashov/Far-NetBox/tags) and unpack it in your
    source directory say C:/src
    or from git repository:

        cd C:/src
        git clone git://github.com/michaellukashov/Far-NetBox.git

    From now on, we assume that your source tree is
    C:/src/Far-NetBox

2.  Compile boost libraries:
    
    Boost 1.48.0 or later is required to build NetBox.
    You can download the source code for boost 
    from [download page](http://sourceforge.net/projects/boost/files/boost/1.48.0/).

    Unpack archive to directory C:/src/Far-NetBox/libs

    Compile bjam:
    cd libs/boost/tools/build/v2/engine
    Run build.bat
    Copy the resulting bjam.exe binary (located in libs/boost/tools/build/v2/engine/bin.ntx86)
    to some directory which is listed in %PATH% variable, say C:/Windows

    Create directory libs/boost/stage

    Compile date_time:
    cd libs/boost/libs/date_time/build && bjam variant=debug link=static threading=multi runtime-debugging=on stage -j3
    copy all files from libs/boost/libs/date_time/build/stage to directory libs/boost/stage

    Compile signals:
    cd libs/boost/libs/signals/build && bjam variant=debug link=static threading=multi runtime-debugging=on stage -j3
    copy all files from libs/boost/libs/signals/build/stage to directory libs/boost/stage

3.  Compile gc-7.2 library:

    cd libs\gc-7.2alpha6
    cmake -G "Visual Studio 10"
    call devenv gc.sln /Build "Debug|Win32" /USEENV /Project "gcmt-lib"
    
4. Compile NetBox plugin:

   cmd /c %VS100COMNTOOLS%\vsvars32.bat && devenv NetBox.sln /Build "Debug|Win32" /USEENV /Project "NetBox"

DISCLAIMER
========================

This plugin is provided "as is". The authors are not responsible for the
consequences of use of this software.

LINKS
========================

* Project main page: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Download page: [https://github.com/michaellukashov/Far-NetBox/downloads/](https://github.com/michaellukashov/Far-NetBox/downloads/)
