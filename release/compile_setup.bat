rem Run from project root NetBox
SET ISCC="C:\Program Files\Inno Setup 5\ISCC.exe"
if exist %ISCC% (
  %ISCC% /v3 release/netboxsetup.iss
  REM "/dFAR_VERSION=Far2" "/dROOT_DIR=.." "/dSOURCE_DIR=../src/NetBox" "/dPUTTY_SOURCE_DIR=C:/Program Files/Putty"
  if errorlevel 1 echo Error creating distributive & exit 1 /b
)
