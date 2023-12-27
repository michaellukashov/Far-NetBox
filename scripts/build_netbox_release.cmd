@echo off
@setlocal

set FAR_VERSION=Far3
set PROJECT_ROOT=%~dp0..

set PROJECT_BUILD_TYPE=Release

if "%1" == "" goto x86
if "%1" == "x64" goto x64
if "%1" == "vs2022-x86" goto vs2022-x86
if "%1" == "vs2022-x64" goto vs2022-x64
goto x86

:vs2022-x64
set PROJECT_COMNTOOLS=%VS170COMNTOOLS%
set PROJECT_KIT=vs2022

:x64
set PROJECT_PLATFORM=x64
set PROJECT_GENERATOR=Ninja
set PROJECT_VARS=x64

call %~dp0\build_netbox.cmd


goto end

:vs2022-x86
set PROJECT_COMNTOOLS=%VS170COMNTOOLS%
set PROJECT_KIT=vs2022

:x86
set PROJECT_PLATFORM=x86
set PROJECT_GENERATOR=Ninja
set PROJECT_VARS=x86

call %~dp0\build_netbox.cmd

goto end

:end

@endlocal
