::
:: This file is used to create a distribution package
::


@rem @echo off
@rem exit 0 /b

set PLUGINNAME=NetBox
set PLUGINARCH=%1
if "%PLUGINARCH%" equ "" set PLUGINARCH=x86
set FARVER=%2
if "%FARVER%" equ "" set FARVER=Far2

:: Get plugin version from resource
for /F "tokens=2,3 skip=2" %%i in (resource.h) do set %%i=%%~j
if "%PLUGIN_VERSION_TXT%" equ "" echo Undefined version & exit 1
set PLUGINVER=%PLUGIN_VERSION_TXT%

:: Package name
set PKGNAME=Far%PLUGINNAME%-%PLUGINVER%_%FARVER%_x86_x64.7z

:: Create temp directory
set PKGDIR=%PLUGINNAME%
set PKGDIRARCH=%PLUGINNAME%\%PLUGINARCH%
if exist %PKGDIRARCH% rmdir /S /Q %PKGDIRARCH%

:: Copy files
if not exist %PKGDIR% ( mkdir %PKGDIR% > NUL )
mkdir %PKGDIRARCH% > NUL
call .\makeCopyCommon.cmd %~d0%~p0 %PKGDIRARCH%
call .\makeCopyCommon.cmd %~d0%~p0\..\.. %PKGDIRARCH%
copy ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME%\*.dll %PKGDIRARCH% > NUL
copy ..\..\dlls\%PLUGINARCH%\*.dll %PKGDIRARCH% > NUL
copy ..\..\libs\openssl\%PLUGINARCH%\*.dll %PKGDIRARCH% > NUL
call .\makeCopyCommon.cmd %~d0%~p0 ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME% > NUL

:: Make archive
if exist %PKGNAME% del %PKGNAME%
call "C:\Program Files\7-Zip\7z.exe" a -mx9 -t7z -r %PKGNAME% %PKGDIR%/* > NUL
if errorlevel 1 echo Error creating archive & exit 1 /b

@rem rmdir /S /Q %PKGDIRARCH%

echo Package %PKGNAME% created
exit 0 /b
