@echo off

set PLUGINNAME=NetBox
set PLUGINARCH=%1
if "%PLUGINARCH%" equ "" set PLUGINARCH=x86
set FARVER=%2
if "%FARVER%" equ "" set FARVER=Far3

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

if exist %~d0%~p0\*.lng copy %~d0%~p0\*.lng %PKGDIRARCH% > NUL
if exist %~d0%~p0\*.hlf copy %~d0%~p0\*.hlf %PKGDIRARCH% > NUL
if exist %~d0%~p0\..\..\ChangeLog copy %~d0%~p0\..\..\ChangeLog %PKGDIRARCH% > NUL
if exist %~d0%~p0\..\..\*.md copy %~d0%~p0\..\..\*.md %PKGDIRARCH% > NUL
if exist %~d0%~p0\..\..\LICENSE.txt copy %~d0%~p0\..\..\LICENSE.txt %PKGDIRARCH% > NUL

copy ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME%\*.dll %PKGDIRARCH% > NUL
@rem copy ..\..\dlls\%PLUGINARCH%\*.dll %PKGDIRARCH% > NUL

:: Make archive
if exist %PKGNAME% del %PKGNAME%
call "C:\Program Files\7-Zip\7z.exe" a -mx9 -t7z -r %PKGNAME% %PKGDIR%/* > NUL
if errorlevel 1 echo Error creating archive & exit 1 /b

@rem rmdir /S /Q %PKGDIRARCH%

echo Package %PKGNAME% created
exit 0 /b
