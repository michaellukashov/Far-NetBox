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

#define FileSourceMain_x64 BINARIES_DIR_X64 + "/NetBox.dll"
#define FileSourceMain_x86 BINARIES_DIR_X86 + "/NetBox.dll"
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
Compression=lzma2
; /ultra
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
; Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
; Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

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
; Source: "licence"; DestName: "licence"; DestDir: "{app}"; Components: main_x86; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\LICENCE"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\putty.hlp"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\pageant.exe"; DestDir: "{app}\PuTTY"; Components: pageant; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\puttygen.exe"; DestDir: "{app}\PuTTY"; Components: puttygen; Flags: ignoreversion

[InstallDelete]
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

function x64Selected(): Boolean;
begin
  Result := IsWin64() and True;
end;

procedure ComponentsListClickCheck(Sender: TObject);
begin
  if not WizardForm.ComponentsList.Checked[0] then
    WizardForm.ComponentsList.Checked[0] := True;
end;

(* procedure InitializeWizard();
var
  InstallDir: string;
  PluginDir: String;
begin
  // allow installation without requiring user to accept licence
  WizardForm.LicenseAcceptedRadio.Checked := True;
  WizardForm.LicenseAcceptedRadio.Visible := False;
  WizardForm.LicenseLabel1.Visible := False;
  WizardForm.LicenseNotAcceptedRadio.Visible := False;
  WizardForm.LicenseMemo.Top := WizardForm.LicenseLabel1.Top;
  WizardForm.LicenseMemo.Height :=
    WizardForm.LicenseNotAcceptedRadio.Top +
    WizardForm.LicenseNotAcceptedRadio.Height -
    WizardForm.LicenseMemo.Top - 5;

  // prevent the main component to be unchecked
  WizardForm.ComponentsList.OnClickCheck := @ComponentsListClickCheck;

  if Is64BitInstallMode() then
  begin
    // MsgBox('Is64BitInstallMode: true', mbInformation, mb_Ok);
    if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) or
       RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) then
    begin
      // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
    end;
  end
  else
  if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir', InstallDir) then
  begin
    // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
  end;
  // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
  if InstallDir <> '' then
  begin
    PluginDir := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
    WizardForm.DirEdit.Text := PluginDir;
  end;
end;
*)

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

procedure CreateTheWizardPages;
begin
  { Input dirs }
  InputDirsPage := CreateInputDirPage(wpSelectDir,
  'Select {#FarVer} plugin location', 'Where {#FarVer} plugin should be installed?',
  '{#FarVer} plugin will be installed in the following folder.'#13#10#13#10 +
  'To continue, click Next. If you would like to select a different folder, click Browse.',
  False, '{#FarVer} plugin folder');
  InputDirsPage.Add('{#FarVer} x86 plugin location:');
  if IsWin64() then
    InputDirsPage.Add('{#FarVer} x64 plugin location:');
  InputDirsPage.Values[0] := GetDefaultFarx86Dir();
  if IsWin64() then
    InputDirsPage.Values[1] := GetDefaultFarx64Dir();
end;

procedure InitializeWizard();
var
  InstallDir: string;
  PluginDir: String;
begin
  { Custom wizard pages }
  CreateTheWizardPages;

  if Is64BitInstallMode() then
  begin
    // MsgBox('Is64BitInstallMode: true', mbInformation, mb_Ok);
    if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) or
       RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir_x64', InstallDir) then
    begin
      // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
    end;
  end
  else
  if RegQueryStringValue(HKCU, 'Software\{#FarVer}', 'InstallDir', InstallDir) or
     RegQueryStringValue(HKLM, 'Software\{#FarVer}', 'InstallDir', InstallDir) then
  begin
    // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
  end;
  // MsgBox('InstallDir: ' + InstallDir, mbInformation, mb_Ok);
  if InstallDir <> '' then
  begin
    PluginDir := AddBackslash(InstallDir) + 'Plugins\{#PluginSubDirName}';
    WizardForm.DirEdit.Text := PluginDir;
  end;
end;
