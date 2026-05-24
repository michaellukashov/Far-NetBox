@echo off
@setlocal
rem parameter x86 or empty build x86, else value - build x64.

if "s%FAR_VERSION%"=="s" set FAR_VERSION=Far3
if "s%PROJECT_ROOT%"=="s" set PROJECT_ROOT=%~dp0..

if "s%PROJECT_BUILD_TYPE%"=="s" set PROJECT_BUILD_TYPE=Debug

if "s%PROJECT_COMNTOOLS%"=="s" set PROJECT_COMNTOOLS=%VS160COMNTOOLS%
if "s%PROJECT_KIT%"=="s" set PROJECT_KIT=vs2019
if "s%PROJECT_PLATFORM%"=="s" set PROJECT_PLATFORM=%1
if "s%PROJECT_GENERATOR%"=="s" set PROJECT_GENERATOR=Ninja
if "s%PROJECT_VARS%"=="s" set PROJECT_VARS=%1

set PROJECT_BUILDDIR=%PROJECT_ROOT%\build\%PROJECT_KIT%\%PROJECT_BUILD_TYPE%\%PROJECT_PLATFORM%
if not exist %PROJECT_BUILDDIR% ( mkdir %PROJECT_BUILDDIR% > NUL )
cd %PROJECT_BUILDDIR%

@call "%PROJECT_COMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" %PROJECT_VARS%
cmake.exe -D CMAKE_BUILD_TYPE=%PROJECT_BUILD_TYPE% -D OPT_CREATE_PLUGIN_DIR=ON -G "%PROJECT_GENERATOR%" %PROJECT_ROOT%\
if "s%PROJECT_GENERATOR%"=="sNinja" (
ninja
) else (
nmake
)

@endlocal
