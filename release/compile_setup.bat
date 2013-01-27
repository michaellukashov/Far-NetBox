rem Run from project root NetBox
SET ISCC="C:\Program Files\Inno Setup 5\ISCC.exe"
if exist %ISCC% (
  %ISCC% /v3 release/data/netboxsetup.iss
  REM "/dSOURCE_DIR=src/NetBox" "/dBINARIES_DIR=src/NetBox/NetBox" "/dPUTTY_SOURCE_DIR=C:\Program Files\Putty"
  if errorlevel 1 echo Error creating distributive & exit 1 /b
)
