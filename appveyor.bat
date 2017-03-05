setlocal

IF %language%==cpp GOTO build_cpp

echo Unsupported language %language%. Exiting.
goto :error

:build_cpp
echo Building VS2015-x64
call src/NetBox/scripts/build_netbox_release.cmd vs2015-x64 || goto error
echo Building VS2015-x86
call src/NetBox/scripts/build_netbox_release.cmd vs2015-x86 || goto error
goto :EOF

goto :EOF

:error
echo Failed!
EXIT /b %ERRORLEVEL%
