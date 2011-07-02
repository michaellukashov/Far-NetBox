@echo off

::::::::::::::::::::::::::::::::::::::::::::::::::::::
::    Copy source files from %1 to %2 directory     ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::

if exist %1\*.cpp copy %1\*.cpp %2 > NUL
if exist %1\*.h   copy %1\*.h %2   > NUL
if exist %1\*.rc  copy %1\*.rc %2  > NUL
if exist %1\*.def copy %1\*.def %2 > NUL
if exist %1\*.sln copy %1\*.sln %2 > NUL
if exist %1\*.vcxproj copy %1\*.vcxproj %2 > NUL
if exist %1\*.vcxproj.user copy %1\*.vcxproj.user %2 > NUL
if exist %1\makeDist.cmd copy %1\makeDist.cmd %2 > NUL

mkdir %2\Common
copy %~d0%~p0*.h %2\Common > NUL
copy %~d0%~p0*.cmd %2\Common > NUL
exit /b 0
