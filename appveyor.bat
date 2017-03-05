setlocal

IF %language%==cpp GOTO build_cpp

echo Unsupported language %language%. Exiting.
goto :error

:build_cpp
if %platform%==Win64 GOTO :build_vs2015_x64
if %platform%==Win32 GOTO :build_vs2015_x86
GOTO :build_vs2015_x64

:build_vs2015_x64
echo Building VS2015-x64
call src/NetBox/scripts/build_netbox_release.cmd vs2015-x64 || goto error
goto :EOF

:build_vs2015_x86
echo Building VS2015-x86
call src/NetBox/scripts/build_netbox_release.cmd vs2015-x86 || goto error
goto :EOF

goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
