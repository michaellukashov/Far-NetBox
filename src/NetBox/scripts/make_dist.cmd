@echo off

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
set PKGNAME=Far%PLUGINNAME%-%PLUGINVER%_Far2_Far3_x86_x64.7z

:: Create temp directory
set PKGDIR=..\..\build\%PLUGINNAME%\%FARVER%\
set PKGDIRARCH=%PKGDIR%\%PLUGINARCH%
if exist %PKGDIRARCH% rmdir /S /Q %PKGDIRARCH%

:: Copy files
if not exist %PKGDIR% ( mkdir %PKGDIR% > NUL )
mkdir %PKGDIRARCH% > NUL

if exist *.lng copy *.lng %PKGDIRARCH% > NUL
if exist *.hlf copy *.hlf %PKGDIRARCH% > NUL
if exist ..\..\ChangeLog copy ..\..\ChangeLog %PKGDIRARCH% > NUL
if exist ..\..\*.md copy ..\..\*.md %PKGDIRARCH% > NUL
if exist ..\..\LICENSE.txt copy ..\..\LICENSE.txt %PKGDIRARCH% > NUL

REM if exist "C:\Program Files\PESuite\PETrim.exe" "C:\Program Files\PESuite\PETrim.exe" ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME%\%PLUGINNAME%.dll /Sf:Y /Sd:Y
copy ..\..\%FARVER%_%PLUGINARCH%\Plugins\%PLUGINNAME%\%PLUGINNAME%.dll %PKGDIRARCH% > NUL
@rem copy ..\..\dlls\%PLUGINARCH%\%PLUGINNAME%.dll %PKGDIRARCH% > NUL

:: Make archive
if exist %PKGNAME% del %PKGNAME%
if exist ../../build/%PLUGINNAME%/Far2 (
  if exist ../../build/%PLUGINNAME%/Far3 (
    if exist "C:\Program Files\7-Zip\7z.exe" (
      call "C:\Program Files\7-Zip\7z.exe" a -mx9 -t7z -r ../../build/%PKGNAME% ../../build/%PLUGINNAME%/* > NUL
      if errorlevel 1 echo Error creating archive & exit 1 /b
      @rem rmdir /S /Q %PKGDIRARCH%
      echo Package %PKGNAME% created
    )
  )
)
exit 0 /b
