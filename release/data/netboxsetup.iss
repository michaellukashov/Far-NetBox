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
SolidCompression=yes
PrivilegesRequired=none
Uninstallable=no
MinVersion=5.1
; DisableDirPage=yes

ArchitecturesInstallIn64BitMode=x64

[Types]
Name: full; Description: "Full installation"
; Name: compact; Description: "Compact installation"
Name: custom; Description: "Custom installation"; Flags: iscustom
; Languages: en ru

[Components]
Name: main; Description: "NetBox plugin for {#FarVer}"; Types: full custom
; Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
; Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

[Files]
Source: "{#FileSourceMain_x64}"; DestName: "NetBox.dll"; DestDir: "{app}"; Components: main; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#FileSourceMain_x86}"; DestName: "NetBox.dll"; DestDir: "{app}"; Components: main; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#FileSourceEng}"; DestName: "NetBoxEng.lng"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "NetBoxRus.lng"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceRus}"; DestName: "ChangeLog"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceChangeLog}"; DestName: "ChangeLog"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceReadmeEng}"; DestName: "README.md"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceReadmeRu}"; DestName: "README.RU.md"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#FileSourceLicense}"; DestName: "LICENSE.txt"; DestDir: "{app}"; Components: main; Flags: ignoreversion
; Source: "WinSCP.ico"; DestDir: "{app}"; Components: main; Flags: ignoreversion
; Source: "licence"; DestName: "licence"; DestDir: "{app}"; Components: main; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\LICENCE"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\putty.hlp"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\pageant.exe"; DestDir: "{app}\PuTTY"; Components: pageant; Flags: ignoreversion
; Source: "C:\Program Files\PuTTY\puttygen.exe"; DestDir: "{app}\PuTTY"; Components: puttygen; Flags: ignoreversion

[InstallDelete]
Type: files; Name: "{app}\NetBoxEng.lng"
Type: files; Name: "{app}\NetBoxRus.lng"

[Code]

procedure ComponentsListClickCheck(Sender: TObject);
begin
  if not WizardForm.ComponentsList.Checked[0] then
    WizardForm.ComponentsList.Checked[0] := True;
end;

procedure InitializeWizard();
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
    MsgBox('Is64BitInstallMode: true', mbInformation, mb_Ok);
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
