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
AlwaysShowDirOnReadyPage=yes

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
*)

procedure ButtonOnClick(Sender: TObject);
begin
  MsgBox('You clicked the button!', mbInformation, mb_Ok);
end;

procedure CreateTheWizardPages;
var
  Page: TWizardPage;
  Button, FormButton: TNewButton;
  Panel: TPanel;
  CheckBox: TNewCheckBox;
  Edit: TNewEdit;
  // PasswordEdit: TPasswordEdit;
  // Memo: TNewMemo;
  // ComboBox: TNewComboBox;
  // ListBox: TNewListBox;
  // StaticText, ProgressBarLabel: TNewStaticText;
  // ProgressBar, ProgressBar2, ProgressBar3: TNewProgressBar;
  // CheckListBox, CheckListBox2: TNewCheckListBox;
  // FolderTreeView: TFolderTreeView;
  // BitmapImage, BitmapImage2, BitmapImage3: TBitmapImage;
  // BitmapFileName: String;
  // RichEditViewer: TRichEditViewer;
begin
  { TButton and others }

  Page := CreateCustomPage(wpWelcome, 'Custom wizard page controls', 'TButton and others');

  Button := TNewButton.Create(Page);
  Button.Width := ScaleX(75);
  Button.Height := ScaleY(23);
  Button.Caption := 'TNewButton';
  Button.OnClick := @ButtonOnClick;
  Button.Parent := Page.Surface;

  Panel := TPanel.Create(Page);
  Panel.Width := Page.SurfaceWidth div 2 - ScaleX(8);
  Panel.Left :=  Page.SurfaceWidth - Panel.Width;
  Panel.Height := Button.Height * 2;
  Panel.Caption := 'TPanel';
  Panel.Color := clWindow;
  Panel.ParentBackground := False;
  Panel.Parent := Page.Surface;

  CheckBox := TNewCheckBox.Create(Page);
  CheckBox.Top := Button.Top + Button.Height + ScaleY(8);
  CheckBox.Width := Page.SurfaceWidth div 2;
  CheckBox.Height := ScaleY(17);
  CheckBox.Caption := 'TNewCheckBox';
  CheckBox.Checked := True;
  CheckBox.Parent := Page.Surface;

  Edit := TNewEdit.Create(Page);
  Edit.Top := CheckBox.Top + CheckBox.Height + ScaleY(8);
  Edit.Width := Page.SurfaceWidth div 2 - ScaleX(8);
  Edit.Text := 'TNewEdit';
  Edit.Parent := Page.Surface;

  (* PasswordEdit := TPasswordEdit.Create(Page);
  PasswordEdit.Left := Page.SurfaceWidth - Edit.Width;
  PasswordEdit.Top := CheckBox.Top + CheckBox.Height + ScaleY(8);
  PasswordEdit.Width := Edit.Width;
  PasswordEdit.Text := 'TPasswordEdit';
  PasswordEdit.Parent := Page.Surface; *)

  (* Memo := TNewMemo.Create(Page);
  Memo.Top := Edit.Top + Edit.Height + ScaleY(8);
  Memo.Width := Page.SurfaceWidth;
  Memo.Height := ScaleY(89);
  Memo.ScrollBars := ssVertical;
  Memo.Text := 'TNewMemo';
  Memo.Parent := Page.Surface; *)

  (* FormButton := TNewButton.Create(Page);
  FormButton.Top := Memo.Top + Memo.Height + ScaleY(8);
  FormButton.Width := ScaleX(75);
  FormButton.Height := ScaleY(23);
  FormButton.Caption := 'TSetupForm';
  FormButton.OnClick := @FormButtonOnClick;
  FormButton.Parent := Page.Surface;

  { TComboBox and others }

  Page := CreateCustomPage(Page.ID, 'Custom wizard page controls', 'TComboBox and others');

  ComboBox := TNewComboBox.Create(Page);
  ComboBox.Width := Page.SurfaceWidth;
  ComboBox.Parent := Page.Surface;
  ComboBox.Style := csDropDownList;
  ComboBox.Items.Add('TComboBox');
  ComboBox.ItemIndex := 0;

  ListBox := TNewListBox.Create(Page);
  ListBox.Top := ComboBox.Top + ComboBox.Height + ScaleY(8);
  ListBox.Width := Page.SurfaceWidth;
  ListBox.Height := ScaleY(97);
  ListBox.Parent := Page.Surface;
  ListBox.Items.Add('TListBox');
  ListBox.ItemIndex := 0;

  StaticText := TNewStaticText.Create(Page);
  StaticText.Top := ListBox.Top + ListBox.Height + ScaleY(8);
  StaticText.Caption := 'TNewStaticText';
  StaticText.AutoSize := True;
  StaticText.Parent := Page.Surface;

  ProgressBarLabel := TNewStaticText.Create(Page);
  ProgressBarLabel.Top := StaticText.Top + StaticText.Height + ScaleY(8);
  ProgressBarLabel.Caption := 'TNewProgressBar';
  ProgressBarLabel.AutoSize := True;
  ProgressBarLabel.Parent := Page.Surface;

  ProgressBar := TNewProgressBar.Create(Page);
  ProgressBar.Left := ProgressBarLabel.Width + ScaleX(8);
  ProgressBar.Top := ProgressBarLabel.Top;
  ProgressBar.Width := Page.SurfaceWidth - ProgressBar.Left;
  ProgressBar.Height := ProgressBarLabel.Height + ScaleY(8);
  ProgressBar.Parent := Page.Surface;
  ProgressBar.Position := 25;

  ProgressBar2 := TNewProgressBar.Create(Page);
  ProgressBar2.Left := ProgressBarLabel.Width + ScaleX(8);
  ProgressBar2.Top := ProgressBar.Top + ProgressBar.Height + ScaleY(4);
  ProgressBar2.Width := Page.SurfaceWidth - ProgressBar.Left;
  ProgressBar2.Height := ProgressBarLabel.Height + ScaleY(8);
  ProgressBar2.Parent := Page.Surface;
  ProgressBar2.Position := 50;
  { Note: TNewProgressBar.State property only has an effect on Windows Vista and newer }
  ProgressBar2.State := npbsError;

  ProgressBar3 := TNewProgressBar.Create(Page);
  ProgressBar3.Left := ProgressBarLabel.Width + ScaleX(8);
  ProgressBar3.Top := ProgressBar2.Top + ProgressBar2.Height + ScaleY(4);
  ProgressBar3.Width := Page.SurfaceWidth - ProgressBar.Left;
  ProgressBar3.Height := ProgressBarLabel.Height + ScaleY(8);
  ProgressBar3.Parent := Page.Surface;
  { Note: TNewProgressBar.Style property only has an effect on Windows XP and newer }
  ProgressBar3.Style := npbstMarquee;
  
  { TNewCheckListBox }

  Page := CreateCustomPage(Page.ID, 'Custom wizard page controls', 'TNewCheckListBox');

  CheckListBox := TNewCheckListBox.Create(Page);
  CheckListBox.Width := Page.SurfaceWidth;
  CheckListBox.Height := ScaleY(97);
  CheckListBox.Flat := True;
  CheckListBox.Parent := Page.Surface;
  CheckListBox.AddCheckBox('TNewCheckListBox', '', 0, True, True, False, True, nil);
  CheckListBox.AddRadioButton('TNewCheckListBox', '', 1, True, True, nil);
  CheckListBox.AddRadioButton('TNewCheckListBox', '', 1, False, True, nil);
  CheckListBox.AddCheckBox('TNewCheckListBox', '', 0, True, True, False, True, nil);

  CheckListBox2 := TNewCheckListBox.Create(Page);
  CheckListBox2.Top := CheckListBox.Top + CheckListBox.Height + ScaleY(8);
  CheckListBox2.Width := Page.SurfaceWidth;
  CheckListBox2.Height := ScaleY(97);
  CheckListBox2.BorderStyle := bsNone;
  CheckListBox2.ParentColor := True;
  CheckListBox2.MinItemHeight := WizardForm.TasksList.MinItemHeight;
  CheckListBox2.ShowLines := False;
  CheckListBox2.WantTabs := True;
  CheckListBox2.Parent := Page.Surface;
  CheckListBox2.AddGroup('TNewCheckListBox', '', 0, nil);
  CheckListBox2.AddRadioButton('TNewCheckListBox', '', 0, True, True, nil);
  CheckListBox2.AddRadioButton('TNewCheckListBox', '', 0, False, True, nil);
*)
  { TFolderTreeView }
(*
  Page := CreateCustomPage(Page.ID, 'Custom wizard page controls', 'TFolderTreeView');

  FolderTreeView := TFolderTreeView.Create(Page);
  FolderTreeView.Width := Page.SurfaceWidth;
  FolderTreeView.Height := Page.SurfaceHeight;
  FolderTreeView.Parent := Page.Surface;
  FolderTreeView.Directory := ExpandConstant('{src}');
*)
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
