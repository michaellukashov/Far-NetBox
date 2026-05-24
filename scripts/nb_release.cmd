@echo off
setlocal

set FAR_VERSION=Far3
set PROJECT_ROOT=%~dp0..

set PROJECT_BUILD_TYPE=Release

if "%1" == "" goto x86
if "%1" == "x64" goto x64
if "%1" == "vs2019-x86" goto vs2019-x86
if "%1" == "vs2019-x64" goto vs2019-x64
goto x86

:vs2019-x64
set PROJECT_COMNTOOLS=%VS160COMNTOOLS%
set PROJECT_KIT=vs2019

rem Fall through to :x64 for shared platform settings
:x64
set PROJECT_PLATFORM=x64
set PROJECT_GENERATOR=Ninja
set PROJECT_VARS=x64

call %~dp0\nb.cmd


goto end

:vs2019-x86
set PROJECT_COMNTOOLS=%VS160COMNTOOLS%
set PROJECT_KIT=vs2019
rem Fall through to :x86 for shared platform settings

:x86
set PROJECT_PLATFORM=x86
set PROJECT_GENERATOR=Ninja
set PROJECT_VARS=x86

call %~dp0\nb.cmd

goto end

:end

endlocal
