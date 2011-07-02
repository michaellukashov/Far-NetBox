@echo off

::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: Copy common plugin files from %1 to %2 directory ::
::::::::::::::::::::::::::::::::::::::::::::::::::::::

if exist %1\*.lng copy %1\*.lng %2 > NUL
if exist %1\*.hlf copy %1\*.hlf %2 > NUL
if exist %1\*.txt copy %1\*.txt %2 > NUL
if exist %1\ChangeLog copy %1\ChangeLog %2 > NUL
if exist %1\ToDo copy %1\ToDo %2 > NUL

exit /b 0
