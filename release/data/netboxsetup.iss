#define FarVer "Far2"

#define ParentRegistryKey "Software\{#FarVer}\Plugins"
#define RegistryKey ParentRegistryKey + "\NetBox 2"
#define Year 2013

#define Status "release"
#ifndef ROOT_DIR
#define ROOT_DIR "../.."
#endif
#ifndef SOURCE_DIR
#define SOURCE_DIR ROOT_DIR + "/src/NetBox"
#endif
#ifndef OUTPUT_DIR
#define OUTPUT_DIR ROOT_DIR + "/build"
#endif
#ifndef BINARIES_DIR_X86
#define BINARIES_DIR_X86 ROOT_DIR + "/src/NetBox/NetBox/x86"
#endif
#ifndef BINARIES_DIR_X64
#define BINARIES_DIR_X64 ROOT_DIR + "/src/NetBox/NetBox/x64"
#endif

#ifndef PUTTY_SOURCE_DIR
#define PUTTY_SOURCE_DIR "C:/Program Files/Putty"
#endif

#define FileSourceMain_x86 BINARIES_DIR_X86 + "/NetBox.dll"
#define FileSourceMain_x64 BINARIES_DIR_X64 + "/NetBox.dll"
#define FileSourceEng SOURCE_DIR + "/NetBoxEng.lng"
#define FileSourceRus SOURCE_DIR + "/NetBoxRus.lng"
#define FileSourceChangeLog ROOT_DIR + "/ChangeLog"
#define FileSourceReadmeEng ROOT_DIR + "/README.md"
#define FileSourceReadmeRu ROOT_DIR + "/README.RU.md"
#define FileSourceLicense ROOT_DIR + "/LICENSE.txt"
#define PluginSubDirName "NetBox"

#define Major
#define Minor
#define Rev
#define Build
#expr ParseVersion(FileSourceMain_x86, Major, Minor, Rev, Build)
#define Version Str(Major) + "." + Str(Minor) + (Rev > 0 ? "." + Str(Rev) : "") + \
  (Status != "" ? " " + Status : "")

[Setup]
AppId=netbox
AppMutex=NetBox
AppName=NetBox plugin for {#FarVer}
AppPublisher=Michael Lukashov
AppPublisherURL=https://github.com/michaellukashov/Far-NetBox
AppSupportURL=http://forum.farmanager.com/viewtopic.php?f=39&t=6638
AppUpdatesURL=http://plugring.farmanager.com/plugin.php?pid=859&l=en
VersionInfoCompany=Michael Lukashov
VersionInfoDescription=Setup for NetBox plugin for {#FarVer} {#Version}
VersionInfoVersion={#Major}.{#Minor}.{#Rev}.{#Build}
VersionInfoTextVersion={#Version}
VersionInfoCopyright=(c) 2011-{#Year} Michael Lukashov
DefaultDirName={pf}\{#FarVer}\Plugins\{#PluginSubDirName}
DisableProgramGroupPage=true
; LicenseFile=licence.setup
; UninstallDisplayIcon={app}\winscp.ico
OutputDir={#OUTPUT_DIR}
DisableStartupPrompt=yes
AppVersion={#Version}
AppVerName=NetBox plugin for {#FarVer} {#Version}
OutputBaseFilename=FarNetBox-{#Major}.{#Minor}.{#Rev}_{#FarVer}_x86_x64
Compression=lzma2/ultra
SolidCompression=yes
PrivilegesRequired=none
Uninstallable=no
MinVersion=5.1
DisableDirPage=yes
; AlwaysShowDirOnReadyPage=yes

ArchitecturesInstallIn64BitMode=x64

[Types]
Name: full; Description: "Full installation"
; Name: compact; Description: "Compact installation"
Name: custom; Description: "Custom installation"; Flags: iscustom
; Languages: en ru

[Components]
Name: main_x86; Description: "NetBox plugin for {#FarVer} x86"; Types: full custom
Name: main_x64; Description: "NetBox plugin for {#FarVer} x64"; Types: full custom; check: IsWin64
Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

[Files]
Source: "{#FileSourceMain_x86}"; DestName: "NetBox.dll"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceMain_x64}"; DestName: "NetBox.dll"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPluginx86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPluginx64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginx86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginx64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginx86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginx64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginx86Dir}\PuTTY"; Components: main_x86 pageant; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginx64Dir}\PuTTY"; Components: main_x64 pageant; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginx86Dir}\PuTTY"; Components: main_x86 puttygen; Flags: ignoreversion
Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginx64Dir}\PuTTY"; Components: main_x64 puttygen; Flags: ignoreversion

[InstallDelete]
Type: files; Name: "{app}\NetBox.dll"
Type: files; Name: "{app}\NetBoxEng.lng"
Type: files; Name: "{app}\NetBoxRus.lng"
Type: files; Name: "{app}\ChangeLog"
Type: files; Name: "{app}\ChangeLog"
Type: files; Name: "{app}\README.md"
Type: files; Name: "{app}\README.RU.md"
Type: files; Name: "{app}\LICENSE.txt"

[Code]

var
  InputDirsPage: TInputDirWizardPage;

function GetDefaultFarx86Dir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir', InstallDir) then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\{#FarVer}\Plugins\{#PluginSubDirName}');
  end;
end;

function GetDefaultFarx64Dir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\{#FarVer}\Plugins\{#PluginSubDirName}');
  end;
end;

function GetPluginx86Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[0];
end;

function GetPluginx64Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[1];
end;

procedure CreateTheWizardPage;
begin
  { Input dirs }
  InputDirsPage := CreateInputDirPage(wpSelectComponents,
  'Select {#FarVer} plugin location', 'Where {#FarVer} plugin should be installed?',
  '{#FarVer} plugin will be installed in the following folder.'#13#10#13#10 +
  'To continue, click Next. If you would like to select a different folder, click Browse.',
  False, '{#FarVer} plugin folder');
  begin
    InputDirsPage.Add('{#FarVer} x86 plugin location:');
    InputDirsPage.Values[0] := GetDefaultFarx86Dir();
  end;
  begin
    InputDirsPage.Add('{#FarVer} x64 plugin location:');
    InputDirsPage.Values[1] := GetDefaultFarx64Dir();
  end;
end;

procedure SetupInputDirs();
begin
  InputDirsPage.Edits[0].Visible := IsComponentSelected('main_x86');
  InputDirsPage.Buttons[0].Visible := IsComponentSelected('main_x86');
  InputDirsPage.PromptLabels[0].Visible := IsComponentSelected('main_x86');

  InputDirsPage.Edits[1].Visible := IsComponentSelected('main_x64');
  InputDirsPage.Buttons[1].Visible := IsComponentSelected('main_x64');
  InputDirsPage.PromptLabels[1].Visible := IsComponentSelected('main_x64');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpSelectComponents then
  begin
    SetupInputDirs();
  end;
  Result := True;
end;

function PrevButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpReady then
  begin
    SetupInputDirs();
  end;
  Result := True;
end;

procedure InitializeWizard();
begin
  // Custom wizard page
  CreateTheWizardPage;
end;
