#define ParentRegistryKey "Software\Far2\Plugins"
#define RegistryKey ParentRegistryKey + "\NetBox 2"
#define Year 2013

#define Status "release"
#ifndef SOURCE_DIR
#define SOURCE_DIR "../../src/NetBox"
#endif
#ifndef OUTPUT_DIR
#define OUTPUT_DIR "."
#endif
#ifndef BINARIES_DIR
#define BINARIES_DIR "../../src/NetBox/NetBox/x86"
#endif

#ifndef PUTTY_SOURCE_DIR
#define PUTTY_SOURCE_DIR "C:/Program Files/Putty"
#endif

#define MainFileSource BINARIES_DIR + "/NetBox.dll"
#define EngFileSource SOURCE_DIR + "/NetBoxEng.lng"
#define PluginSubDirName "NetBox"

#define Major
#define Minor
#define Rev
#define Build
#expr ParseVersion(MainFileSource, Major, Minor, Rev, Build)
#define Version Str(Major)+"."+Str(Minor)+(Rev > 0 ? "."+Str(Rev) : "")+(Status != "" ? " "+Status : "")

#define FarVer "Far2"
#define FarArch "x86"

[Setup]
AppId=netbox
AppMutex=NetBox
AppName=NetBox plugin for FAR
AppPublisher=Michael Lukashov
AppPublisherURL=https://github.com/michaellukashov/Far-NetBox
AppSupportURL=http://forum.farmanager.com/viewtopic.php?f=39&t=6638
AppUpdatesURL=http://plugring.farmanager.com/plugin.php?pid=859&l=en
VersionInfoCompany=Michael Lukashov
VersionInfoDescription=Setup for NetBox plugin for FAR {#Version}
VersionInfoVersion="1.2"
; {#Major}.{#Minor}.{#Rev}.{#Build}
VersionInfoTextVersion={#Version}
VersionInfoCopyright=(c) 2011-{#Year} Michael Lukashov
DefaultDirName={pf}\Far\Plugins\{#PluginSubDirName}
DisableProgramGroupPage=true
LicenseFile=licence.setup
; UninstallDisplayIcon={app}\winscp.ico
OutputDir={#OUTPUT_DIR}
DisableStartupPrompt=yes
AppVersion={#Version}
AppVerName=NetBox plugin for FAR {#Version}
OutputBaseFilename=FarNetBox-{#Major}.{#Minor}.{#Rev}_{#FarVer}_{#FarArch}_setup
SolidCompression=yes
PrivilegesRequired=none

[Types]
Name: full; Description: "Full installation"
Name: compact; Description: "Compact installation"
Name: custom; Description: "Custom installation"; Flags: iscustom

[Components]
Name: main; Description: "NetBox plugin for FAR"; Types: full custom compact
Name: pageant; Description: "Pageant (SSH authentication agent)"; Types: full
Name: puttygen; Description: "PuTTYgen (key generator)"; Types: full

[Files]
Source: "{#MainFileSource}"; DestName: "NetBox.dll"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "{#EngFileSource}"; DestName: "NetBoxEng.lng"; DestDir: "{app}"; Components: main; Flags: ignoreversion
; Source: "WinSCP.ico"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "licence"; DestName: "licence"; DestDir: "{app}"; Components: main; Flags: ignoreversion
Source: "C:\Program Files\PuTTY\LICENCE"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
Source: "C:\Program Files\PuTTY\putty.hlp"; DestDir: "{app}\PuTTY"; Components: pageant puttygen; Flags: ignoreversion
Source: "C:\Program Files\PuTTY\pageant.exe"; DestDir: "{app}\PuTTY"; Components: pageant; Flags: ignoreversion
Source: "C:\Program Files\PuTTY\puttygen.exe"; DestDir: "{app}\PuTTY"; Components: puttygen; Flags: ignoreversion

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
  S: string;
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

  if RegQueryStringValue(HKCU, 'Software\Far\Plugins', 'NetBox 2', S) then
  begin
    S := ExtractFileDir(ExtractFileDir(S));
    if CompareText(ExtractFileName(S), 'Plugins') = 0 then
      WizardForm.DirEdit.Text := AddBackslash(S) + '{#PluginSubDirName}';
  end;
end;
