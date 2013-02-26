@echo off
@setlocal

if "s%FAR_VERSION%"=="s" set FAR_VERSION=Far2
if "s%PROJECT_ROOT%"=="s" set PROJECT_ROOT=%~dp0..\..\..

if "s%PROJECT_CONFIG%"=="s" set PROJECT_CONFIG=Debug
if "s%PROJECT_BUILD%"=="s" set PROJECT_BUILD=Build

if "s%PROJECT_CONF%"=="s" set PROJECT_CONF=x86
if "s%PROJECT_PLATFORM%"=="s" set PROJECT_PLATFORM=Win32
if "s%PROJECT_GEN%"=="s" set PROJECT_GEN=Visual Studio 10
if "s%PROJECT_VARS%"=="s" set PROJECT_VARS=x86

set PROJECT_BUIILDDIR=%PROJECT_ROOT%\build\%PROJECT_CONFIG%\%PROJECT_CONF%
if not exist %PROJECT_BUIILDDIR% ( mkdir %PROJECT_BUIILDDIR% > NUL )
cd %PROJECT_BUIILDDIR%

@call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" %PROJECT_VARS%
cmake.exe -D PROJECT_ROOT=%PROJECT_ROOT% -D CMAKE_BUILD_TYPE=%PROJECT_CONFIG% -G "%PROJECT_GEN%" -D CONF=%PROJECT_CONF% -D FAR_VERSION=%FAR_VERSION% %PROJECT_ROOT%\src\NetBox
msbuild.exe NetBox.sln /t:NetBox;%PROJECT_BUILD% /maxcpucount:2 /p:Configuration=%PROJECT_CONFIG% /p:Platform=%PROJECT_PLATFORM% /p:Project=NetBox
REM nmake

@endlocal
