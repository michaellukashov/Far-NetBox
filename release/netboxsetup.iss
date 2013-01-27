#ifndef FAR_VERSION
#define FAR_VERSION "Far2"
#endif

#define YEAR 2013
#define STATUS "release"
#ifndef ROOT_DIR
#define ROOT_DIR ".."
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
  (STATUS != "" ? " " + STATUS : "")

[Setup]
AppId=netbox
AppMutex=NetBox
AppName=NetBox plugin for {#FAR_VERSION}
AppPublisher=Michael Lukashov
AppPublisherURL=https://github.com/michaellukashov/Far-NetBox
AppSupportURL=http://forum.farmanager.com/viewtopic.php?f=39&t=6638
AppUpdatesURL=http://plugring.farmanager.com/plugin.php?pid=859&l=en
VersionInfoCompany=Michael Lukashov
VersionInfoDescription=Setup for NetBox plugin for {#FAR_VERSION} {#Version}
VersionInfoVersion={#Major}.{#Minor}.{#Rev}.{#Build}
VersionInfoTextVersion={#Version}
VersionInfoCopyright=(c) 2011-{#YEAR} Michael Lukashov
DefaultDirName={pf}\{#FAR_VERSION}\Plugins\{#PluginSubDirName}
UsePreviousAppDir=false
DisableProgramGroupPage=true
LicenseFile=licence.setup
; UninstallDisplayIcon={app}\winscp.ico
OutputDir={#OUTPUT_DIR}
DisableStartupPrompt=yes
AppVersion={#Version}
AppVerName=NetBox plugin for {#FAR_VERSION} {#Version}
OutputBaseFilename=FarNetBox-{#Major}.{#Minor}.{#Rev}_{#FAR_VERSION}_x86_x64
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
Name: main_x86; Description: "NetBox plugin for {#FAR_VERSION} x86"; Types: full custom; check: IsFarX86Installed
Name: main_x64; Description: "NetBox plugin for {#FAR_VERSION} x64"; Types: full custom; check: IsWin64 and IsFarX64Installed
; Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
; Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

[Files]
Source: "{#FileSourceMain_x86}"; DestName: "NetBox.dll"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceMain_x64}"; DestName: "NetBox.dll"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPluginX86Dir}"; Components: main_x86; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{code:GetPluginX64Dir}"; Components: main_x64; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\LICENCE"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\putty.hlp"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 pageant; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\pageant.exe"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 pageant; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginX86Dir}\PuTTY"; Components: main_x86 puttygen; Flags: ignoreversion
; Source: "{#PUTTY_SOURCE_DIR}\puttygen.exe"; DestDir: "{code:GetPluginX64Dir}\PuTTY"; Components: main_x64 puttygen; Flags: ignoreversion

[InstallDelete]

[Code]

var
  InputDirsPage: TInputDirWizardPage;

function GetFarX86InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKCU, 'Software\{#FAR_VERSION}', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FAR_VERSION}', 'InstallDir', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function GetFarX64InstallDir(): String;
var
  InstallDir: String;
begin
  if RegQueryStringValue(HKCU, 'Software\{#FAR_VERSION}', 'InstallDir_x64', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FAR_VERSION}', 'InstallDir_x64', InstallDir) then
  begin
    Result := InstallDir;
  end;
end;

function IsFarX86Installed(): Boolean;
begin
  Result := GetFarX86InstallDir() <> '';
end;

function IsFarX64Installed(): Boolean;
begin
  Result := GetFarX64InstallDir() <> '';
end;

function GetDefaultFarX86Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFarX86InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\{#FAR_VERSION}\Plugins\{#PluginSubDirName}');
  end;
end;

function GetDefaultFarX64Dir(): String;
var
  InstallDir: String;
begin
  InstallDir := GetFarX64InstallDir();
  if InstallDir <> '' then
  begin
    Result := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
  end
  else
  begin
    Result := ExpandConstant('{pf}\{#FAR_VERSION}\Plugins\{#PluginSubDirName}');
  end;
end;

function GetPluginX86Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[0];
end;

function GetPluginX64Dir(Param: String): String;
begin
  Result := InputDirsPage.Values[1];
end;

procedure CreateTheWizardPage;
begin
  // Input dirs
  InputDirsPage := CreateInputDirPage(wpSelectComponents,
  'Select {#FAR_VERSION} plugin location', 'Where {#FAR_VERSION} plugin should be installed?',
  '{#FAR_VERSION} plugin will be installed in the following folder.'#13#10#13#10 +
  'To continue, click Next. If you would like to select a different folder, click Browse.',
  False, '{#FAR_VERSION} plugin folder');
  begin
    InputDirsPage.Add('{#FAR_VERSION} x86 plugin location:');
    InputDirsPage.Values[0] := GetDefaultFarX86Dir();
  end;
  begin
    InputDirsPage.Add('{#FAR_VERSION} x64 plugin location:');
    InputDirsPage.Values[1] := GetDefaultFarX64Dir();
  end;
end;

(*procedure SetupComponents();
begin
  if IsFarX86Installed() then
  begin
    MsgBox('IsFarX86Installed: true', mbInformation, mb_Ok);
  end;
  if IsFarX64Installed() then
  begin
    MsgBox('IsFarX64Installed: true', mbInformation, mb_Ok);
  end;
end;*)

procedure SetupInputDirs();
begin
  InputDirsPage.Edits[0].Enabled := IsComponentSelected('main_x86');
  InputDirsPage.Buttons[0].Enabled := IsComponentSelected('main_x86');
  InputDirsPage.PromptLabels[0].Enabled := IsComponentSelected('main_x86');

  InputDirsPage.Edits[1].Visible := IsWin64();
  InputDirsPage.Buttons[1].Visible := IsWin64();
  InputDirsPage.PromptLabels[1].Visible := IsWin64();

  InputDirsPage.Edits[1].Enabled := IsComponentSelected('main_x64');
  InputDirsPage.Buttons[1].Enabled := IsComponentSelected('main_x64');
  InputDirsPage.PromptLabels[1].Enabled := IsComponentSelected('main_x64');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if CurPageID = wpWelcome then
  begin
    // SetupComponents();
  end
  else
  if CurPageID = wpSelectComponents then
  begin
    SetupInputDirs();
  end
  else
  if CurPageID = InputDirsPage.ID then
  begin
    WizardForm.DirEdit.Text := InputDirsPage.Values[0];
  end;
  Result := True;
end;

function BackButtonClick(CurPageID: Integer): Boolean;
begin
  // MsgBox('CurPageID: ' + IntToStr(CurPageID), mbInformation, mb_Ok);
  if CurPageID = InputDirsPage.ID then
  begin
    // SetupComponents();
  end;
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
