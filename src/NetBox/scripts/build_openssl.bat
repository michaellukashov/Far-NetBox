@rem @echo off

@rem 
rm -rf out32dll
rm -rf tmp32dll
rm -rf inc32

if "%1" == "" goto x86
if "%1" == "x64" goto x64
goto x86

:x64
rm -rf x64
call %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86_amd64
perl Configure VC-WIN64A no-asm enable-static-engine 2>&1 | tee res.txt
ms\do_win64a 2>&1 | tee -a res.txt
nmake -f ms\ntdll.mak 2>&1 | tee -a res.txt
mkdir x64
cp out32dll/ssleay32.lib out32dll/ssleay32.dll out32dll/libeay32.lib out32dll/libeay32.dll x64
cp -R inc32 x64
goto end

:x86
rm -rf x86
call %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86

perl Configure VC-WIN32 enable-static-engine 2>&1 | tee res.txt
ms\do_nasm 2>&1 | tee -a res.txt
nmake -f ms\ntdll.mak 2>&1 | tee -a res.txt
mkdir x86
cp out32dll/ssleay32.lib out32dll/ssleay32.dll out32dll/libeay32.lib out32dll/libeay32.dll x86
cp -R inc32 x86
goto end

:end
