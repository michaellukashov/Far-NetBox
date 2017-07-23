@rem @echo off
@setlocal

pushd %~dp0..\..\..\libs\openssl

@rem 
rm -rf out32dll tmp32dll tmp32 inc32 out32

set CONF_PARAMS=no-unit-test no-cast no-err no-bf no-sctp no-asm enable-static-engine no-shared no-hw no-camellia no-seed no-rc4 no-rc5 no-whirlpool no-srp no-gost no-idea -Ox -Ob1 -Oi -Os -Oy -GF -GS- -Gy -DNDEBUG -DOPENSSL_NO_CAPIENG -DNO_CHMOD -DOPENSSL_NO_DGRAM -DOPENSSL_NO_RIJNDAEL -DDSO_WIN32 -DOPENSSL_NO_LOCKING -DWINSCP

if "%1" == "" goto vs2010-x86
if "%1" == "x86" goto vs2010-x86
if "%1" == "vs2010-x86" goto vs2010-x86
if "%1" == "vs2015-x86" goto vs2015-x86
if "%1" == "x64" goto vs2010-x64
if "%1" == "vs2010-x64" goto vs2010-x64
if "%1" == "vs2015-x64" goto vs2015-x64
goto vs2010-x86

:vs2010-x64
rm -rf x64
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
perl Configure VC-WIN64A %CONF_PARAMS%
call ms\do_win64a
nmake clean
nmake 
mkdir x64
cp libssl.lib libcrypto.lib x64
cp ossl_static.pdb x64
cp -R include x64
goto end

:vs2015-x64
rm -rf vs2015-x64
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
perl Configure VC-WIN64A %CONF_PARAMS%
call ms\do_win64a
nmake clean
nmake 
mkdir vs2015-x64
cp libssl.lib libcrypto.lib vs2015-x64
cp ossl_static.pdb vs2015-x64
cp -R include vs2015-x64
goto end

:vs2010-x86
rm -rf x86
rm -f makefile crypto/buildinf.h
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
perl Configure VC-WIN32 %CONF_PARAMS%
rem call ms\do_nasm
call ms\do_ms
nmake clean
nmake
mkdir x86
cp libcrypto.lib libssl.lib x86
cp app.pdb ossl_static.pdb x86
cp -R include x86
goto end

:vs2015-x86
rm -rf vs2015-x86
call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
perl Configure VC-WIN32 %CONF_PARAMS%
rem call ms\do_nasm
call ms\do_ms
nmake clean
nmake
mkdir vs2015-x86
cp libcrypto.lib libssl.lib vs2015-x86
cp app.pdb ossl_static.pdb vs2015-x86
cp -R include vs2015-x86
goto end

:end
popd
@endlocal
