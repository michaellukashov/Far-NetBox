setlocal

if "%platform_name%"=="" goto :build_vs2022_x64
if "%platform_name%"=="x64" goto :build_vs2022_x64
if "%platform_name%"=="x86" goto :build_vs2022_x86
goto :build_vs2022_x86

:build_vs2022_x64
echo Building VS2022-x64
call scripts/build_netbox_release.cmd vs2022-x64 || goto error
goto :EOF

:build_vs2022_x86
echo Building VS2022-x86
call scripts/build_netbox_release.cmd vs2022-x86 || goto error
goto :EOF

goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
