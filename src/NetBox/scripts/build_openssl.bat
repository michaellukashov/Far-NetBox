@rem @echo off

@rem 
rm -rf out32dll out32
rm -rf tmp32dll tmp32
rm -rf inc32

if "%1" == "" goto x86
if "%1" == "x64" goto x64
goto x86

:x64
rm -rf x64
call %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86_amd64
perl Configure VC-WIN64A no-asm enable-static-engine 2>&1 | tee res.txt
ms\do_win64a 2>&1 | tee -a res.txt
nmake -f ms\nt.mak 2>&1 | tee -a res.txt
mkdir x64
cp out32/ssleay32.lib out32/libeay32.lib x64
cp -R inc32 x64
goto end

:x86
rm -rf x86
call %VS100COMNTOOLS%\..\..\VC\vcvarsall.bat x86

perl Configure VC-WIN32 enable-static-engine 2>&1 | tee res.txt
ms\do_nasm 2>&1 | tee -a res.txt
nmake -f ms\nt.mak 2>&1 | tee -a res.txt
mkdir x86
cp out32/ssleay32.lib out32/libeay32.lib x86
cp -R inc32 x86
goto end

:end
