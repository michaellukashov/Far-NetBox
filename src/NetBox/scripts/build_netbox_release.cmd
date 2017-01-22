@echo off
@setlocal

set FAR_VERSION=Far2
set PROJECT_ROOT=%~dp0..\..\..

set PROJECT_BUILD_TYPE=Release
set PROJECT_BUILD=Build

if "%1" == "" goto x86
if "%1" == "x64" goto x64
if "%1" == "vs2015-x86" goto vs2015-x86
if "%1" == "vs2015-x64" goto vs2015-x64
goto x86

:vs2015-x64
set PROJECT_COMNTOOLS=%VS140COMNTOOLS%

:x64
set PROJECT_PLATFORM=x64
set PROJECT_GENERATOR=NMake Makefiles
set PROJECT_VARS=x86_amd64

call %~dp0\build_netbox.cmd


goto end

:vs2015-x86
set PROJECT_COMNTOOLS=%VS140COMNTOOLS%

:x86
set PROJECT_PLATFORM=x86
set PROJECT_GENERATOR=NMake Makefiles
set PROJECT_VARS=x86

call %~dp0\build_netbox.cmd

goto end

:end

@endlocal
