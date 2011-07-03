:: NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)
:: Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>
::
:: This file is used to create a distribution package
::

@echo off

set PLUGINNAME=NetBox
set PLUGINARCH=%1
if "%PLUGINARCH%" equ "" set PLUGINARCH=src

:: Get plugin version from resource
for /F "tokens=2,3 skip=14" %%i in (resource.h) do set %%i=%%~j
if "%PLUGIN_VERSION_TXT%" equ "" echo Undefined version & exit 1
set PLUGINVER=%PLUGIN_VERSION_TXT%

:: Package name
set PKGNAME=Far%PLUGINNAME%_%PLUGINVER%_%PLUGINARCH%.7z
if exist %PKGNAME% del %PKGNAME%

:: Create temp directory
set PKGDIR=%PLUGINNAME%
if "%PLUGINARCH%" equ "src" set PKGDIR=%PKGDIR%_%PLUGINVER%
if exist %PKGDIR% rmdir /S /Q %PKGDIR%

:: Copy files
mkdir %PKGDIR%
call ..\Common\makeCopyCommon.cmd %~d0%~p0 %PKGDIR%
mkdir %PKGDIR%\Sessions
copy Sessions %PKGDIR%\Sessions > NUL
if "%PLUGINARCH%" equ "src"	(
	call ..\Common\makeCopySources.cmd %~d0%~p0 %PKGDIR%
	mkdir %PKGDIR%\tinyXML
	copy tinyXML %PKGDIR%\tinyXML > NUL
) else (
	copy openssl\%PLUGINARCH%\out32dll\*.dll %PKGDIR% > NUL
	copy ..\Far2_%PLUGINARCH%\Plugins\%PLUGINNAME%\*.dll %PKGDIR% > NUL
)
:: Make archive
call "C:\Program Files\7-Zip\7z.exe" a -t7z -r %PKGNAME% %PKGDIR% > NUL
if errorlevel 1 echo Error creating archive & exit 1 /b

rmdir /S /Q %PKGDIR%

echo Package %PKGNAME% created
exit 0 /b
