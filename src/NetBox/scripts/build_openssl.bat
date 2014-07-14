@rem @echo off

@rem 
rm -rf out32dll tmp32dll tmp32 inc32 out32

if "%1" == "" goto x86
if "%1" == "x64" goto x64
goto x86

:x64
rm -rf x64
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86_amd64
perl Configure VC-WIN64A no-cast no-err no-bf no-sctp no-rsax no-asm enable-static-engine no-shared no-hw no-camellia no-seed no-rc4 no-rc5 no-krb5 no-whirlpool no-srp no-gost no-idea no-ripemd no-des -Ox -Ob1 -Oi -Os -Oy -GF -GS- -Gy  -DNDEBUG;OPENSSL_NO_CAPIENG;NO_CHMOD;OPENSSL_NO_DGRAM;OPENSSL_NO_RIJNDAEL;DSO_WIN32
call ms\do_win64a
nmake -f ms\nt.mak
mkdir x64
cp out32/ssleay32.lib out32/libeay32.lib x64
cp tmp32/lib.pdb x64
cp -R inc32 x64
goto end

:x86
rm -rf x86
call "%VS100COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
perl Configure VC-WIN32 no-cast no-err no-bf no-sctp no-rsax no-asm enable-static-engine no-shared no-hw no-camellia no-seed no-rc4 no-rc5 no-krb5 no-whirlpool no-srp no-gost no-idea no-ripemd no-des -Ox -Ob1 -Oi -Os -Oy -GF -GS- -Gy -DNDEBUG;OPENSSL_NO_CAPIENG;NO_CHMOD;OPENSSL_NO_DGRAM;OPENSSL_NO_RIJNDAEL;DSO_WIN32
rem call ms\do_nasm
call ms\do_ms
nmake -f ms\nt.mak
mkdir x86
cp out32/ssleay32.lib out32/libeay32.lib x86
cp tmp32/lib.pdb x86
cp -R inc32 x86
goto end

:end
