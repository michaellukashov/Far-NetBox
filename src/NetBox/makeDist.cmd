::
:: This file is used to create a distribution package
::


@rem @echo off
@rem exit 0 /b

set PLUGINNAME=NetBox
set PLUGINARCH=%1
if "%PLUGINARCH%" equ "" set PLUGINARCH=src
set FARVER=%2
if "%FARVER%" equ "" set FARVER=Far2

:: Get plugin version from resource
for /F "tokens=2,3 skip=2" %%i in (resource.h) do set %%i=%%~j
if "%PLUGIN_VERSION_TXT%" equ "" echo Undefined version & exit 1
set PLUGINVER=%PLUGIN_VERSION_TXT%

:: Package name
set PKGNAME=Far%PLUGINNAME%-%PLUGINVER%_%FARVER%_%PLUGINARCH%.7z
if exist %PKGNAME% del %PKGNAME%

:: Create temp directory
set PKGDIR=%PLUGINNAME%
if "%PLUGINARCH%" equ "src" set PKGDIR=%PKGDIR%-%PLUGINVER%
if exist %PKGDIR% rmdir /S /Q %PKGDIR%

:: Copy files
mkdir %PKGDIR%
call .\makeCopyCommon.cmd %~d0%~p0 %PKGDIR%
call .\makeCopyCommon.cmd %~d0%~p0\..\.. %PKGDIR%
if "%PLUGINARCH%" equ "src"    (
    call .\makeCopySources.cmd %~d0%~p0 %PKGDIR%
    mkdir %PKGDIR%\tinyXML
    copy ..\..\libs\tinyXML %PKGDIR%\tinyXML > NUL
) else (
    copy ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME%\*.dll %PKGDIR% > NUL
    copy ..\..\dlls\%PLUGINARCH%\*.dll %PKGDIR% > NUL
    copy ..\..\libs\openssl\%PLUGINARCH%\*.dll %PKGDIR% > NUL
    call .\makeCopyCommon.cmd %~d0%~p0 ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME% > NUL
)
:: Make archive
call "C:\Program Files\7-Zip\7z.exe" a -t7z -r %PKGNAME% %PKGDIR%/* > NUL
if errorlevel 1 echo Error creating archive & exit 1 /b

rmdir /S /Q %PKGDIR%

echo Package %PKGNAME% created
exit 0 /b
