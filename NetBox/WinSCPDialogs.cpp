//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include <StrUtils.hpp>
#include <math.h>
#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarTexts.h"
#include "FarDialog.h"
#include "FarConfiguration.h"
#include <PuttyTools.h>
#include <GUITools.h>
#include <CoreMain.h>
#include <Common.h>
#include <CopyParam.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <Bookmarks.h>
#include <Queue.h>
#include <farkeys.hpp>
#include <farcolor.hpp>
// FAR WORKAROUND
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
enum TButtonResult { brCancel = -1, brOK = 1, brConnect };
//---------------------------------------------------------------------------
class TWinSCPDialog : public TFarDialog
{
public:
  __fastcall TWinSCPDialog(TCustomFarPlugin * AFarPlugin);

  void __fastcall AddStandardButtons(int Shift = 0, bool ButtonsOnly = false);

  TFarSeparator * ButtonSeparator;
  TFarButton * OkButton;
  TFarButton * CancelButton;
};
//---------------------------------------------------------------------------
__fastcall TWinSCPDialog::TWinSCPDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin)
{
}
//---------------------------------------------------------------------------
void __fastcall TWinSCPDialog::AddStandardButtons(int Shift, bool ButtonsOnly)
{
  if (!ButtonsOnly)
  {
    NextItemPosition = ipNewLine;

    ButtonSeparator = new TFarSeparator(this);
    if (Shift >= 0)
    {
      ButtonSeparator->Move(0, Shift);
    }
    else
    {
      ButtonSeparator->Top = Shift;
      ButtonSeparator->Bottom = Shift;
    }
  }

  assert(OkButton == NULL);
  OkButton = new TFarButton(this);
  if (ButtonsOnly)
  {
    if (Shift >= 0)
    {
      OkButton->Move(0, Shift);
    }
    else
    {
      OkButton->Top = Shift;
      OkButton->Bottom = Shift;
    }
  }
  OkButton->Caption = GetMsg(MSG_BUTTON_OK);
  OkButton->Default = true;
  OkButton->Result = brOK;
  OkButton->CenterGroup = true;

  NextItemPosition = ipRight;

  assert(CancelButton == NULL);
  CancelButton = new TFarButton(this);
  CancelButton->Caption = GetMsg(MSG_BUTTON_Cancel);
  CancelButton->Result = brCancel;
  CancelButton->CenterGroup = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTabbedDialog : public TWinSCPDialog
{
friend class TTabButton;

public:
  __fastcall TTabbedDialog(TCustomFarPlugin * AFarPlugin, int TabCount);
  __property int Tab = { read = FTab };

protected:
  void __fastcall HideTabs();
  virtual void __fastcall SelectTab(int Tab);
  void __fastcall TabButtonClick(TFarButton * Sender, bool & Close);
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  virtual AnsiString __fastcall TabName(int Tab);
  TTabButton * __fastcall TabButton(int Tab);

private:
  AnsiString FOrigCaption;
  int FTab;
  int FTabCount;
};
//---------------------------------------------------------------------------
class TTabButton : public TFarButton
{
public:
  __fastcall TTabButton(TTabbedDialog * Dialog);

  __property int Tab = { read = FTab, write = FTab };
  __property AnsiString TabName = { read = FTabName, write = SetTabName };

private:
  AnsiString FTabName;
  int FTab;

  void __fastcall SetTabName(AnsiString value);
};
//---------------------------------------------------------------------------
__fastcall TTabbedDialog::TTabbedDialog(TCustomFarPlugin * AFarPlugin, int TabCount) :
  TWinSCPDialog(AFarPlugin)
{
  FTab = 0;
  FTabCount = TabCount;

  // FAR WORKAROUND
  // (to avoid first control on dialog be a button, that would be "pressed"
  // when listbox loses focus)
  TFarText * Text = new TFarText(this);
  // make next item be inserted to default position
  Text->Move(0, -1);
  // on FAR 1.70 alpha 6 and later, empty text control would overwrite the
  // dialog box caption
  Text->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TTabbedDialog::HideTabs()
{
  for (int i = 0; i < ItemCount; i++)
  {
    TFarDialogItem * I = Item[i];
    if (I->Group)
    {
      I->Visible = false;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TTabbedDialog::SelectTab(int Tab)
{
  if (FTab != Tab)
  {
    if (FTab)
    {
      ShowGroup(FTab, false);
    }
    ShowGroup(Tab, true);
    FTab = Tab;
  }

  for (int i = 0; i < ItemCount; i++)
  {
    TFarDialogItem * I = Item[i];
    if ((I->Group == Tab) && I->CanFocus())
    {
      I->SetFocus();
      break;
    }
  }

  if (FOrigCaption.IsEmpty())
  {
    FOrigCaption = Caption;
  }
  Caption = FORMAT("%s - %s", (TabName(Tab), FOrigCaption));
}
//---------------------------------------------------------------------------
TTabButton * __fastcall TTabbedDialog::TabButton(int Tab)
{
  TTabButton * Result = NULL;
  for (int i = 0; i < ItemCount; i++)
  {
    TTabButton * T = dynamic_cast<TTabButton*>(Item[i]);
    if ((T != NULL) && (T->Tab == Tab))
    {
      Result = T;
      break;
    }
  }

  assert(Result != NULL);

  return Result;
}
//---------------------------------------------------------------------------
AnsiString __fastcall TTabbedDialog::TabName(int Tab)
{
  return TabButton(Tab)->TabName;
}
//---------------------------------------------------------------------------
void __fastcall TTabbedDialog::TabButtonClick(TFarButton * Sender, bool & Close)
{
  TTabButton * Tab = dynamic_cast<TTabButton*>(Sender);
  assert(Tab != NULL);

  SelectTab(Tab->Tab);

  Close = false;
}
//---------------------------------------------------------------------------
bool __fastcall TTabbedDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
  bool Result = false;
  if (KeyCode == KEY_CTRLPGDN || KeyCode == KEY_CTRLPGUP)
  {
    int NewTab = FTab;
    do
    {
      if (KeyCode == KEY_CTRLPGDN)
      {
        NewTab = NewTab == FTabCount - 1 ? 1 : NewTab + 1;
      }
      else
      {
        NewTab = NewTab == 1 ? FTabCount - 1 : NewTab - 1;
      }
    }
    while (!TabButton(NewTab)->Enabled);
    SelectTab(NewTab);
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TTabButton::TTabButton(TTabbedDialog * Dialog) :
  TFarButton(Dialog)
{
  CenterGroup = true;
  OnClick = Dialog->TabButtonClick;
}
//---------------------------------------------------------------------------
void __fastcall TTabButton::SetTabName(AnsiString value)
{
  if (FTabName != value)
  {
    AnsiString C;
    int P = value.Pos("|");
    if (P > 0)
    {
      C = value.SubString(1, P - 1);
      value.Delete(1, P);
    }
    else
    {
      C = value;
    }
    Caption = C;
    FTabName = StripHotKey(value);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::ConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    TFarText * Text;

    Dialog->Size = TPoint(67, 23);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_INTERFACE))));

    TFarCheckBox * DisksMenuCheck = new TFarCheckBox(Dialog);
    DisksMenuCheck->Caption = GetMsg(CONFIG_DISKS_MENU);

    Text = new TFarText(Dialog);
    Text->Left += 4;
    Text->Caption = GetMsg(CONFIG_HOTKEY_LABEL);
    Text->EnabledDependency = DisksMenuCheck;

    Dialog->NextItemPosition = ipRight;

    TFarRadioButton * AutoHotKeyButton = new TFarRadioButton(Dialog);
    AutoHotKeyButton->Caption = GetMsg(CONFIG_HOTKEY_AUTOASSIGN);
    AutoHotKeyButton->EnabledDependency = DisksMenuCheck;

    TFarRadioButton * ManualHotKeyButton = new TFarRadioButton(Dialog);
    ManualHotKeyButton->Caption = GetMsg(CONFIG_HOTKEY_MANUAL);
    ManualHotKeyButton->EnabledDependency = DisksMenuCheck;

    TFarEdit * HotKeyEdit = new TFarEdit(Dialog);
    HotKeyEdit->Width = 1;
    HotKeyEdit->Fixed = true;
    HotKeyEdit->Mask = "9";
    HotKeyEdit->EnabledDependency = ManualHotKeyButton;

    Text = new TFarText(Dialog);
    Text->Caption = "(1 - 9)";
    Text->EnabledDependency = ManualHotKeyButton;

    Dialog->NextItemPosition = ipNewLine;

    TFarCheckBox * PluginsMenuCheck = new TFarCheckBox(Dialog);
    PluginsMenuCheck->Caption = GetMsg(CONFIG_PLUGINS_MENU);

    TFarCheckBox * PluginsMenuCommandsCheck = new TFarCheckBox(Dialog);
    PluginsMenuCommandsCheck->Caption = GetMsg(CONFIG_PLUGINS_MENU_COMMANDS);

    TFarCheckBox * HostNameInTitleCheck = new TFarCheckBox(Dialog);
    HostNameInTitleCheck->Caption = GetMsg(CONFIG_HOST_NAME_IN_TITLE);

    new TFarSeparator(Dialog);

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(CONFIG_COMAND_PREFIXES);

    TFarEdit * CommandPrefixesEdit = new TFarEdit(Dialog);

    new TFarSeparator(Dialog);

    TFarCheckBox * CustomPanelCheck = new TFarCheckBox(Dialog);
    CustomPanelCheck->Caption = GetMsg(CONFIG_PANEL_MODE_CHECK);

    Text = new TFarText(Dialog);
    Text->Left += 4;
    Text->EnabledDependency = CustomPanelCheck;
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_TYPES);

    Dialog->NextItemPosition = ipBelow;

    TFarEdit * CustomPanelTypesEdit = new TFarEdit(Dialog);
    CustomPanelTypesEdit->EnabledDependency = CustomPanelCheck;
    CustomPanelTypesEdit->Width = CustomPanelTypesEdit->Width / 2 - 1;

    Dialog->NextItemPosition = ipRight;

    Text = new TFarText(Dialog);
    Text->EnabledDependency = CustomPanelCheck;
    Text->Move(0, -1);
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_STATUS_TYPES);

    Dialog->NextItemPosition = ipBelow;

    TFarEdit * CustomPanelStatusTypesEdit = new TFarEdit(Dialog);
    CustomPanelStatusTypesEdit->EnabledDependency = CustomPanelCheck;

    Dialog->NextItemPosition = ipNewLine;

    Text = new TFarText(Dialog);
    Text->Left += 4;
    Text->EnabledDependency = CustomPanelCheck;
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_WIDTHS);

    Dialog->NextItemPosition = ipBelow;

    TFarEdit * CustomPanelWidthsEdit = new TFarEdit(Dialog);
    CustomPanelWidthsEdit->EnabledDependency = CustomPanelCheck;
    CustomPanelWidthsEdit->Width = CustomPanelTypesEdit->Width;

    Dialog->NextItemPosition = ipRight;

    Text = new TFarText(Dialog);
    Text->EnabledDependency = CustomPanelCheck;
    Text->Move(0, -1);
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_STATUS_WIDTHS);

    Dialog->NextItemPosition = ipBelow;

    TFarEdit * CustomPanelStatusWidthsEdit = new TFarEdit(Dialog);
    CustomPanelStatusWidthsEdit->EnabledDependency = CustomPanelCheck;

    Dialog->NextItemPosition = ipNewLine;

    TFarCheckBox * CustomPanelFullScreenCheck = new TFarCheckBox(Dialog);
    CustomPanelFullScreenCheck->Left += 4;
    CustomPanelFullScreenCheck->EnabledDependency = CustomPanelCheck;
    CustomPanelFullScreenCheck->Caption = GetMsg(CONFIG_PANEL_MODE_FULL_SCREEN);

    Text = new TFarText(Dialog);
    Text->Left += 4;
    Text->EnabledDependency = CustomPanelCheck;
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_HINT);
    Text = new TFarText(Dialog);
    Text->Left += 4;
    Text->EnabledDependency = CustomPanelCheck;
    Text->Caption = GetMsg(CONFIG_PANEL_MODE_HINT2);

    Dialog->AddStandardButtons();

    DisksMenuCheck->Checked = FarConfiguration->DisksMenu;
    AutoHotKeyButton->Checked = !FarConfiguration->DisksMenuHotKey;
    ManualHotKeyButton->Checked = FarConfiguration->DisksMenuHotKey;
    HotKeyEdit->Text = FarConfiguration->DisksMenuHotKey ?
      IntToStr(FarConfiguration->DisksMenuHotKey) : AnsiString();
    PluginsMenuCheck->Checked = FarConfiguration->PluginsMenu;
    PluginsMenuCommandsCheck->Checked = FarConfiguration->PluginsMenuCommands;
    HostNameInTitleCheck->Checked = FarConfiguration->HostNameInTitle;
    CommandPrefixesEdit->Text = FarConfiguration->CommandPrefixes;

    CustomPanelCheck->Checked = FarConfiguration->CustomPanelModeDetailed;
    CustomPanelTypesEdit->Text = FarConfiguration->ColumnTypesDetailed;
    CustomPanelWidthsEdit->Text = FarConfiguration->ColumnWidthsDetailed;
    CustomPanelStatusTypesEdit->Text = FarConfiguration->StatusColumnTypesDetailed;
    CustomPanelStatusWidthsEdit->Text = FarConfiguration->StatusColumnWidthsDetailed;
    CustomPanelFullScreenCheck->Checked = FarConfiguration->FullScreenDetailed;

    Result = (Dialog->ShowModal() == brOK);
    if (Result)
    {
      FarConfiguration->DisksMenu = DisksMenuCheck->Checked;
      FarConfiguration->DisksMenuHotKey =
        ManualHotKeyButton->Checked && !HotKeyEdit->IsEmpty ?
          StrToInt(HotKeyEdit->Text) : 0;
      FarConfiguration->PluginsMenu = PluginsMenuCheck->Checked;
      FarConfiguration->PluginsMenuCommands = PluginsMenuCommandsCheck->Checked;
      FarConfiguration->HostNameInTitle = HostNameInTitleCheck->Checked;

      FarConfiguration->CommandPrefixes = CommandPrefixesEdit->Text;

      FarConfiguration->CustomPanelModeDetailed = CustomPanelCheck->Checked;
      FarConfiguration->ColumnTypesDetailed = CustomPanelTypesEdit->Text;
      FarConfiguration->ColumnWidthsDetailed = CustomPanelWidthsEdit->Text;
      FarConfiguration->StatusColumnTypesDetailed = CustomPanelStatusTypesEdit->Text;
      FarConfiguration->StatusColumnWidthsDetailed = CustomPanelStatusWidthsEdit->Text;
      FarConfiguration->FullScreenDetailed = CustomPanelFullScreenCheck->Checked;
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::PanelConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    Dialog->Size = TPoint(65, 7);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_PANEL))));

    TFarCheckBox * AutoReadDirectoryAfterOpCheck = new TFarCheckBox(Dialog);
    AutoReadDirectoryAfterOpCheck->Caption = GetMsg(CONFIG_AUTO_READ_DIRECTORY_AFTER_OP);

    Dialog->AddStandardButtons();

    AutoReadDirectoryAfterOpCheck->Checked = Configuration->AutoReadDirectoryAfterOp;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        Configuration->AutoReadDirectoryAfterOp = AutoReadDirectoryAfterOpCheck->Checked;
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::LoggingConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    TFarSeparator * Separator;
    TFarText * Text;

    Dialog->Size = TPoint(65, 15);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_LOGGING))));

    TFarCheckBox * LoggingCheck = new TFarCheckBox(Dialog);
    LoggingCheck->Caption = GetMsg(LOGGING_ENABLE);

    Separator = new TFarSeparator(Dialog);
    Separator->Caption = GetMsg(LOGGING_OPTIONS_GROUP);

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(LOGGING_LOG_PROTOCOL);
    Text->EnabledDependency = LoggingCheck;

    Dialog->NextItemPosition = ipRight;

    TFarComboBox * LogProtocolCombo = new TFarComboBox(Dialog);
    LogProtocolCombo->DropDownList = true;
    LogProtocolCombo->Width = 10;
    for (int i = 0; i <= 2; i++)
    {
      LogProtocolCombo->Items->Add(GetMsg(LOGGING_LOG_PROTOCOL_0 + i));
    }
    LogProtocolCombo->EnabledDependency = LoggingCheck;

    Dialog->NextItemPosition = ipNewLine;

    new TFarSeparator(Dialog);

    TFarCheckBox * LogToFileCheck = new TFarCheckBox(Dialog);
    LogToFileCheck->Caption = GetMsg(LOGGING_LOG_TO_FILE);
    LogToFileCheck->EnabledDependency = LoggingCheck;

    TFarEdit * LogFileNameEdit = new TFarEdit(Dialog);
    LogFileNameEdit->Left += 4;
    LogFileNameEdit->History = LOG_FILE_HISTORY;
    LogFileNameEdit->EnabledDependency = LogToFileCheck;

    Dialog->NextItemPosition = ipBelow;

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(LOGGING_LOG_FILE_HINT1);
    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(LOGGING_LOG_FILE_HINT2);

    TFarRadioButton * LogFileAppendButton = new TFarRadioButton(Dialog);
    LogFileAppendButton->Caption = GetMsg(LOGGING_LOG_FILE_APPEND);
    LogFileAppendButton->EnabledDependency = LogToFileCheck;

    Dialog->NextItemPosition = ipRight;

    TFarRadioButton * LogFileOverwriteButton = new TFarRadioButton(Dialog);
    LogFileOverwriteButton->Caption = GetMsg(LOGGING_LOG_FILE_OVERWRITE);
    LogFileOverwriteButton->EnabledDependency = LogToFileCheck;

    Dialog->AddStandardButtons();

    LoggingCheck->Checked = Configuration->Logging;
    LogProtocolCombo->Items->Selected = Configuration->LogProtocol;
    LogToFileCheck->Checked = Configuration->LogToFile;
    LogFileNameEdit->Text =
      (!Configuration->LogToFile && Configuration->LogFileName.IsEmpty()) ?
      IncludeTrailingBackslash(SystemTemporaryDirectory()) + "&s.log" :
      Configuration->LogFileName;
    LogFileAppendButton->Checked = Configuration->LogFileAppend;
    LogFileOverwriteButton->Checked = !Configuration->LogFileAppend;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        Configuration->Logging = LoggingCheck->Checked;
        Configuration->LogProtocol = LogProtocolCombo->Items->Selected;
        Configuration->LogToFile = LogToFileCheck->Checked;
        if (LogToFileCheck->Checked)
        {
          Configuration->LogFileName = LogFileNameEdit->Text;
        }
        Configuration->LogFileAppend = LogFileAppendButton->Checked;
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::TransferConfigurationDialog()
{
  AnsiString Caption = FORMAT("%s - %s",
    (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_TRANSFER))));

  TCopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
  bool Result = CopyParamDialog(Caption, CopyParam, 0);
  if (Result)
  {
    GUIConfiguration->DefaultCopyParam = CopyParam;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::EnduranceConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    TFarSeparator * Separator;
    TFarText * Text;

    Dialog->Size = TPoint(76, 13);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_ENDURANCE))));

    Separator = new TFarSeparator(Dialog);
    Separator->Caption = GetMsg(TRANSFER_RESUME);

    TFarRadioButton * ResumeOnButton = new TFarRadioButton(Dialog);
    ResumeOnButton->Caption = GetMsg(TRANSFER_RESUME_ON);

    TFarRadioButton * ResumeSmartButton = new TFarRadioButton(Dialog);
    ResumeSmartButton->Caption = GetMsg(TRANSFER_RESUME_SMART);
    int ResumeThresholdLeft = ResumeSmartButton->Right;

    TFarRadioButton * ResumeOffButton = new TFarRadioButton(Dialog);
    ResumeOffButton->Caption = GetMsg(TRANSFER_RESUME_OFF);

    TFarEdit * ResumeThresholdEdit = new TFarEdit(Dialog);
    ResumeThresholdEdit->Move(0, -2);
    ResumeThresholdEdit->Left = ResumeThresholdLeft + 3;
    ResumeThresholdEdit->Fixed = true;
    ResumeThresholdEdit->Mask = "9999999";
    ResumeThresholdEdit->Width = 9;
    ResumeThresholdEdit->EnabledDependency = ResumeSmartButton;

    Dialog->NextItemPosition = ipRight;

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(TRANSFER_RESUME_THRESHOLD_UNIT);
    Text->EnabledDependency = ResumeSmartButton;

    Dialog->NextItemPosition = ipNewLine;

    Separator = new TFarSeparator(Dialog);
    Separator->Caption = GetMsg(TRANSFER_SESSION_REOPEN_GROUP);
    Separator->Move(0, 1);

    TFarCheckBox * SessionReopenAutoCheck = new TFarCheckBox(Dialog);
    SessionReopenAutoCheck->Caption = GetMsg(TRANSFER_SESSION_REOPEN_AUTO);

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(TRANSFER_SESSION_REOPEN_AUTO_LABEL);
    Text->EnabledDependency = SessionReopenAutoCheck;
    Text->Move(4, 0);

    Dialog->NextItemPosition = ipRight;

    TFarEdit * SessionReopenAutoEdit = new TFarEdit(Dialog);
    SessionReopenAutoEdit->EnabledDependency = SessionReopenAutoCheck;
    SessionReopenAutoEdit->Fixed = true;
    SessionReopenAutoEdit->Mask = "999";
    SessionReopenAutoEdit->Width = 5;

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(TRANSFER_SESSION_REOPEN_AUTO_UNIT);
    Text->EnabledDependency = SessionReopenAutoCheck;

    Dialog->AddStandardButtons();

    ResumeOnButton->Checked = GUIConfiguration->DefaultCopyParam.ResumeSupport == rsOn;
    ResumeSmartButton->Checked = GUIConfiguration->DefaultCopyParam.ResumeSupport == rsSmart;
    ResumeOffButton->Checked = GUIConfiguration->DefaultCopyParam.ResumeSupport == rsOff;
    ResumeThresholdEdit->AsInteger =
      static_cast<int>(GUIConfiguration->DefaultCopyParam.ResumeThreshold / 1024);

    SessionReopenAutoCheck->Checked = (Configuration->SessionReopenAuto > 0);
    SessionReopenAutoEdit->AsInteger = (Configuration->SessionReopenAuto > 0 ?
      (Configuration->SessionReopenAuto / 1000): 5);

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        TGUICopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;

        if (ResumeOnButton->Checked) CopyParam.ResumeSupport = rsOn;
        if (ResumeSmartButton->Checked) CopyParam.ResumeSupport = rsSmart;
        if (ResumeOffButton->Checked) CopyParam.ResumeSupport = rsOff;
        CopyParam.ResumeThreshold = ResumeThresholdEdit->AsInteger * 1024;

        GUIConfiguration->DefaultCopyParam = CopyParam;

        Configuration->SessionReopenAuto =
          (SessionReopenAutoCheck->Checked ? (SessionReopenAutoEdit->AsInteger * 1000) : 0);
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::QueueConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    TFarText * Text;

    Dialog->Size = TPoint(76, 11);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_BACKGROUND))));

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(TRANSFER_QUEUE_LIMIT);

    Dialog->NextItemPosition = ipRight;

    TFarEdit * QueueTransferLimitEdit = new TFarEdit(Dialog);
    QueueTransferLimitEdit->Fixed = true;
    QueueTransferLimitEdit->Mask = "9";
    QueueTransferLimitEdit->Width = 3;

    Dialog->NextItemPosition = ipNewLine;

    TFarCheckBox * QueueCheck = new TFarCheckBox(Dialog);
    QueueCheck->Caption = GetMsg(TRANSFER_QUEUE_DEFAULT);

    TFarCheckBox * QueueAutoPopupCheck = new TFarCheckBox(Dialog);
    QueueAutoPopupCheck->Caption = GetMsg(TRANSFER_AUTO_POPUP);

    TFarCheckBox * RememberPasswordCheck = new TFarCheckBox(Dialog);
    RememberPasswordCheck->Caption = GetMsg(TRANSFER_REMEMBER_PASSWORD);

    TFarCheckBox * QueueBeepCheck = new TFarCheckBox(Dialog);
    QueueBeepCheck->Caption = GetMsg(TRANSFER_QUEUE_BEEP);

    Dialog->AddStandardButtons();

    QueueTransferLimitEdit->AsInteger = FarConfiguration->QueueTransfersLimit;
    QueueCheck->Checked = FarConfiguration->DefaultCopyParam.Queue;
    QueueAutoPopupCheck->Checked = FarConfiguration->QueueAutoPopup;
    RememberPasswordCheck->Checked = GUIConfiguration->QueueRememberPassword;
    QueueBeepCheck->Checked = FarConfiguration->QueueBeep;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        TGUICopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;

        FarConfiguration->QueueTransfersLimit = QueueTransferLimitEdit->AsInteger;
        CopyParam.Queue = QueueCheck->Checked;
        FarConfiguration->QueueAutoPopup = QueueAutoPopupCheck->Checked;
        GUIConfiguration->QueueRememberPassword = RememberPasswordCheck->Checked;
        FarConfiguration->QueueBeep = QueueBeepCheck->Checked;

        GUIConfiguration->DefaultCopyParam = CopyParam;
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTransferEditorConfigurationDialog : public TWinSCPDialog
{
public:
  __fastcall TTransferEditorConfigurationDialog(TCustomFarPlugin * AFarPlugin);

  bool __fastcall Execute();

protected:
  virtual void __fastcall Change();

private:
  TFarCheckBox * EditorMultipleCheck;
  TFarCheckBox * EditorUploadOnSaveCheck;
  TFarRadioButton * EditorDownloadDefaultButton;
  TFarRadioButton * EditorDownloadOptionsButton;
  TFarRadioButton * EditorUploadSameButton;
  TFarRadioButton * EditorUploadOptionsButton;

  virtual void __fastcall UpdateControls();
};
//---------------------------------------------------------------------------
__fastcall TTransferEditorConfigurationDialog::TTransferEditorConfigurationDialog(
    TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin)
{
  TFarSeparator * Separator;

  Size = TPoint(55, 14);
  Caption = FORMAT("%s - %s",
    (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_TRANSFER_EDITOR))));

  EditorMultipleCheck = new TFarCheckBox(this);
  EditorMultipleCheck->Caption = GetMsg(TRANSFER_EDITOR_MULTIPLE);

  EditorUploadOnSaveCheck = new TFarCheckBox(this);
  EditorUploadOnSaveCheck->Caption = GetMsg(TRANSFER_EDITOR_UPLOAD_ON_SAVE);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(TRANSFER_EDITOR_DOWNLOAD);

  EditorDownloadDefaultButton = new TFarRadioButton(this);
  EditorDownloadDefaultButton->Caption = GetMsg(TRANSFER_EDITOR_DOWNLOAD_DEFAULT);

  EditorDownloadOptionsButton = new TFarRadioButton(this);
  EditorDownloadOptionsButton->Caption = GetMsg(TRANSFER_EDITOR_DOWNLOAD_OPTIONS);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(TRANSFER_EDITOR_UPLOAD);

  EditorUploadSameButton = new TFarRadioButton(this);
  EditorUploadSameButton->Caption = GetMsg(TRANSFER_EDITOR_UPLOAD_SAME);

  EditorUploadOptionsButton = new TFarRadioButton(this);
  EditorUploadOptionsButton->Caption = GetMsg(TRANSFER_EDITOR_UPLOAD_OPTIONS);

  AddStandardButtons();
}
//---------------------------------------------------------------------------
bool __fastcall TTransferEditorConfigurationDialog::Execute()
{
  EditorDownloadDefaultButton->Checked = FarConfiguration->EditorDownloadDefaultMode;
  EditorDownloadOptionsButton->Checked = !FarConfiguration->EditorDownloadDefaultMode;
  EditorUploadSameButton->Checked = FarConfiguration->EditorUploadSameOptions;
  EditorUploadOptionsButton->Checked = !FarConfiguration->EditorUploadSameOptions;
  EditorUploadOnSaveCheck->Checked = FarConfiguration->EditorUploadOnSave;
  EditorMultipleCheck->Checked = FarConfiguration->EditorMultiple;

  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    Configuration->BeginUpdate();
    try
    {
      FarConfiguration->EditorDownloadDefaultMode = EditorDownloadDefaultButton->Checked;
      FarConfiguration->EditorUploadSameOptions = EditorUploadSameButton->Checked;
      FarConfiguration->EditorUploadOnSave = EditorUploadOnSaveCheck->Checked;
      FarConfiguration->EditorMultiple = EditorMultipleCheck->Checked;
    }
    __finally
    {
      Configuration->EndUpdate();
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TTransferEditorConfigurationDialog::Change()
{
  TWinSCPDialog::Change();

  if (Handle)
  {
    LockChanges();
    try
    {
      UpdateControls();
    }
    __finally
    {
      UnlockChanges();
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TTransferEditorConfigurationDialog::UpdateControls()
{
  EditorDownloadDefaultButton->Enabled = !EditorMultipleCheck->Checked;
  EditorDownloadOptionsButton->Enabled = EditorDownloadDefaultButton->Enabled;

  EditorUploadSameButton->Enabled =
    !EditorMultipleCheck->Checked && !EditorUploadOnSaveCheck->Checked;
  EditorUploadOptionsButton->Enabled = EditorUploadSameButton->Enabled;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::TransferEditorConfigurationDialog()
{
  bool Result;
  TTransferEditorConfigurationDialog * Dialog = new TTransferEditorConfigurationDialog(this);
  try
  {
    Result = Dialog->Execute();
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::ConfirmationsConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    Dialog->Size = TPoint(65, 10);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_CONFIRMATIONS))));

    TFarCheckBox * ConfirmOverwritingCheck = new TFarCheckBox(Dialog);
    ConfirmOverwritingCheck->AllowGrayed = true;
    ConfirmOverwritingCheck->Caption = GetMsg(CONFIRMATIONS_CONFIRM_OVERWRITING);

    TFarCheckBox * ConfirmCommandSessionCheck = new TFarCheckBox(Dialog);
    ConfirmCommandSessionCheck->Caption = GetMsg(CONFIRMATIONS_OPEN_COMMAND_SESSION);

    TFarCheckBox * ConfirmResumeCheck = new TFarCheckBox(Dialog);
    ConfirmResumeCheck->Caption = GetMsg(CONFIRMATIONS_CONFIRM_RESUME);

    TFarCheckBox * ConfirmSynchronizedBrowsingCheck = new TFarCheckBox(Dialog);
    ConfirmSynchronizedBrowsingCheck->Caption = GetMsg(CONFIRMATIONS_SYNCHRONIZED_BROWSING);

    Dialog->AddStandardButtons();

    ConfirmOverwritingCheck->Selected = !FarConfiguration->ConfirmOverwritingOverride ?
      BSTATE_3STATE : (Configuration->ConfirmOverwriting ? BSTATE_CHECKED :
        BSTATE_UNCHECKED);
    ConfirmCommandSessionCheck->Checked = GUIConfiguration->ConfirmCommandSession;
    ConfirmResumeCheck->Checked = GUIConfiguration->ConfirmResume;
    ConfirmSynchronizedBrowsingCheck->Checked = FarConfiguration->ConfirmSynchronizedBrowsing;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        FarConfiguration->ConfirmOverwritingOverride =
          ConfirmOverwritingCheck->Selected != BSTATE_3STATE;
        GUIConfiguration->ConfirmCommandSession = ConfirmCommandSessionCheck->Checked;
        GUIConfiguration->ConfirmResume = ConfirmResumeCheck->Checked;
        if (FarConfiguration->ConfirmOverwritingOverride)
        {
          Configuration->ConfirmOverwriting = ConfirmOverwritingCheck->Checked;
        }
        FarConfiguration->ConfirmSynchronizedBrowsing = ConfirmSynchronizedBrowsingCheck->Checked;
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::IntegrationConfigurationDialog()
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    TFarText * Text;

    Dialog->Size = TPoint(65, 14);
    Dialog->Caption = FORMAT("%s - %s",
      (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_INTEGRATION))));

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(INTEGRATION_PUTTY);

    TFarEdit * PuttyPathEdit = new TFarEdit(Dialog);

    TFarCheckBox * PuttyPasswordCheck = new TFarCheckBox(Dialog);
    PuttyPasswordCheck->Caption = GetMsg(INTEGRATION_PUTTY_PASSWORD);
    PuttyPasswordCheck->EnabledDependency = PuttyPathEdit;

    TFarCheckBox * TelnetForFtpInPuttyCheck = new TFarCheckBox(Dialog);
    TelnetForFtpInPuttyCheck->Caption = GetMsg(INTEGRATION_TELNET_FOR_FTP_IN_PUTTY);
    TelnetForFtpInPuttyCheck->EnabledDependency = PuttyPathEdit;

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(INTEGRATION_PAGEANT);

    TFarEdit * PageantPathEdit = new TFarEdit(Dialog);

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(INTEGRATION_PUTTYGEN);

    TFarEdit * PuttygenPathEdit = new TFarEdit(Dialog);

    Dialog->AddStandardButtons();

    PuttyPathEdit->Text = GUIConfiguration->PuttyPath;
    PuttyPasswordCheck->Checked = GUIConfiguration->PuttyPassword;
    TelnetForFtpInPuttyCheck->Checked = GUIConfiguration->TelnetForFtpInPutty;
    PageantPathEdit->Text = FarConfiguration->PageantPath;
    PuttygenPathEdit->Text = FarConfiguration->PuttygenPath;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Configuration->BeginUpdate();
      try
      {
        GUIConfiguration->PuttyPath = PuttyPathEdit->Text;
        GUIConfiguration->PuttyPassword = PuttyPasswordCheck->Checked;
        GUIConfiguration->TelnetForFtpInPutty = TelnetForFtpInPuttyCheck->Checked;
        FarConfiguration->PageantPath = PageantPathEdit->Text;
        FarConfiguration->PuttygenPath = PuttygenPathEdit->Text;
      }
      __finally
      {
        Configuration->EndUpdate();
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
class TAboutDialog : public TFarDialog
{
public:
  __fastcall TAboutDialog(TCustomFarPlugin * AFarPlugin);

private:
  void __fastcall UrlButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall UrlTextClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
};
//---------------------------------------------------------------------------
AnsiString __fastcall ReplaceCopyright(AnsiString S)
{
  return StringReplace(S, "©", "(c)", TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
}
//---------------------------------------------------------------------------
__fastcall TAboutDialog::TAboutDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin)
{
  TFarText * Text;
  TFarButton * Button;

  AnsiString ProductName = Configuration->FileInfoString["ProductName"];
  AnsiString Comments;
  try
  {
    Comments = Configuration->FileInfoString["Comments"];
  }
  catch(...)
  {
    Comments = "";
  }
  AnsiString LegalCopyright = Configuration->FileInfoString["LegalCopyright"];

  int Height = 16;
  #ifndef NO_FILEZILLA
  Height += 2;
  #endif
  if (!ProductName.IsEmpty())
  {
    Height++;
  }
  if (!Comments.IsEmpty())
  {
    Height++;
  }
  if (!LegalCopyright.IsEmpty())
  {
    Height++;
  }
  Size = TPoint(55, Height);

  Caption = FORMAT("%s - %s",
    (GetMsg(PLUGIN_TITLE), StripHotKey(GetMsg(CONFIG_ABOUT))));

  Text = new TFarText(this);
  Text->Caption = Configuration->FileInfoString["FileDescription"];
  Text->CenterGroup = true;

  Text = new TFarText(this);
  Text->Caption = FORMAT(GetMsg(ABOUT_VERSION), (Configuration->Version));
  Text->CenterGroup = true;

  if (!ProductName.IsEmpty())
  {
    Text = new TFarText(this);
    Text->Move(0, 1);
    Text->Caption = FORMAT(GetMsg(ABOUT_PRODUCT_VERSION),
      (ProductName,
       Configuration->ProductVersion));
    Text->CenterGroup = true;
  }

  if (!Comments.IsEmpty())
  {
    Text = new TFarText(this);
    if (ProductName.IsEmpty())
    {
      Text->Move(0, 1);
    }
    Text->Caption = Comments;
    Text->CenterGroup = true;
  }

  if (!LegalCopyright.IsEmpty())
  {
    Text = new TFarText(this);
    Text->Move(0, 1);
    Text->Caption = Configuration->FileInfoString["LegalCopyright"];
    Text->CenterGroup = true;
  }

  Text = new TFarText(this);
  if (LegalCopyright.IsEmpty())
  {
    Text->Move(0, 1);
  }
  Text->Caption = GetMsg(ABOUT_URL);
  Text->Color = static_cast<char>((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09);
  Text->CenterGroup = true;
  Text->OnMouseClick = UrlTextClick;

  Button = new TFarButton(this);
  Button->Move(0, 1);
  Button->Caption = GetMsg(ABOUT_HOMEPAGE);
  Button->OnClick = UrlButtonClick;
  Button->Tag = 1;
  Button->CenterGroup = true;

  NextItemPosition = ipRight;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(ABOUT_FORUM);
  Button->OnClick = UrlButtonClick;
  Button->Tag = 2;
  Button->CenterGroup = true;

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->Caption = FMTLOAD(PUTTY_BASED_ON, (LoadStr(PUTTY_VERSION)));
  Text->CenterGroup = true;

  Text = new TFarText(this);
  Text->Caption = ReplaceCopyright(LoadStr(PUTTY_COPYRIGHT));
  Text->CenterGroup = true;

  #ifndef NO_FILEZILLA
  Text = new TFarText(this);
  Text->Caption = FMTLOAD(FILEZILLA_BASED_ON, (LoadStr(FILEZILLA_VERSION)));
  Text->CenterGroup = true;

  Text = new TFarText(this);
  Text->Caption = ReplaceCopyright(LoadStr(FILEZILLA_COPYRIGHT));
  Text->CenterGroup = true;
  #endif

  new TFarSeparator(this);

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_Close);
  Button->Default = true;
  Button->Result = brOK;
  Button->CenterGroup = true;
  Button->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TAboutDialog::UrlTextClick(TFarDialogItem * /*Item*/,
  MOUSE_EVENT_RECORD * /*Event*/)
{
  AnsiString Address = GetMsg(ABOUT_URL);
  ShellExecute(NULL, "open", Address.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void __fastcall TAboutDialog::UrlButtonClick(TFarButton * Sender, bool & /*Close*/)
{
  AnsiString Address;
  switch (Sender->Tag) {
    case 1: Address = GetMsg(ABOUT_URL) + "eng/docs/far"; break;
    case 2: Address = GetMsg(ABOUT_URL) + "forum/"; break;
  }
  ShellExecute(NULL, "open", Address.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TWinSCPPlugin::AboutDialog()
{
  TFarDialog * Dialog = new TAboutDialog(this);
  try
  {
    Dialog->ShowModal();
  }
  __finally
  {
    delete Dialog;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TPasswordDialog : public TFarDialog
{
public:
  __fastcall TPasswordDialog(TCustomFarPlugin * AFarPlugin,
    AnsiString SessionName, TPromptKind Kind, AnsiString Name,
    AnsiString Instructions, TStrings * Prompts, bool StoredCredentialsTried);

  bool __fastcall Execute(TStrings * Results);

private:
  TSessionData * FSessionData;
  AnsiString FPrompt;
  TList * FEdits;
  TFarCheckBox * SavePasswordCheck;

  void __fastcall ShowPromptClick(TFarButton * Sender, bool & Close);
  void __fastcall GenerateLabel(AnsiString Caption, bool & Truncated);
  TFarEdit * __fastcall GenerateEdit(bool Echo);
  void __fastcall GeneratePrompt(bool ShowSavePassword,
    AnsiString Instructions, TStrings * Prompts, bool & Truncated);
};
//---------------------------------------------------------------------------
__fastcall TPasswordDialog::TPasswordDialog(TCustomFarPlugin * AFarPlugin,
  AnsiString SessionName, TPromptKind Kind, AnsiString Name,
  AnsiString Instructions, TStrings * Prompts, bool StoredCredentialsTried) :
  TFarDialog(AFarPlugin)
{
  TFarButton * Button;

  bool ShowSavePassword = false;
  FSessionData = NULL;
  if (((Kind == pkPassword) || (Kind == pkTIS) || (Kind == pkCryptoCard) ||
       (Kind == pkKeybInteractive)) &&
      (Prompts->Count == 1) && !bool(Prompts->Objects[0]) &&
      !SessionName.IsEmpty() &&
      StoredCredentialsTried)
  {
    FSessionData = dynamic_cast<TSessionData *>(StoredSessions->FindByName(SessionName));
    ShowSavePassword = (FSessionData != NULL) && !FSessionData->Password.IsEmpty();
  }

  bool Truncated = false;
  GeneratePrompt(ShowSavePassword, Instructions, Prompts, Truncated);

  Caption = Name;

  if (ShowSavePassword)
  {
    SavePasswordCheck = new TFarCheckBox(this);
    SavePasswordCheck->Caption = GetMsg(PASSWORD_SAVE);
  }
  else
  {
    SavePasswordCheck = NULL;
  }

  new TFarSeparator(this);

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_OK);
  Button->Default = true;
  Button->Result = brOK;
  Button->CenterGroup = true;

  NextItemPosition = ipRight;

  if (Truncated)
  {
    Button = new TFarButton(this);
    Button->Caption = GetMsg(PASSWORD_SHOW_PROMPT);
    Button->OnClick = ShowPromptClick;
    Button->CenterGroup = true;
  }

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_Cancel);
  Button->Result = brCancel;
  Button->CenterGroup = true;
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::GenerateLabel(AnsiString Caption,
  bool & Truncated)
{
  TFarText * Result = new TFarText(this);

  if (!FPrompt.IsEmpty())
  {
    FPrompt += "\n\n";
  }
  FPrompt += Caption;

  if (Size.x - 10 < Caption.Length())
  {
    Caption.SetLength(Size.x - 10 - 4);
    Caption += " ...";
    Truncated = true;
  }

  Result->Caption = Caption;
}
//---------------------------------------------------------------------------
TFarEdit * __fastcall TPasswordDialog::GenerateEdit(bool Echo)
{
  TFarEdit * Result = new TFarEdit(this);
  Result->Password = !Echo;
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::GeneratePrompt(bool ShowSavePassword,
  AnsiString Instructions, TStrings * Prompts, bool & Truncated)
{
  FEdits = new TList;

  TPoint S = TPoint(40, ShowSavePassword ? 1 : 0);

  if (S.x < Instructions.Length())
  {
    S.x = Instructions.Length();
  }
  if (!Instructions.IsEmpty())
  {
    S.y += 2;
  }

  for (int Index = 0; Index < Prompts->Count; Index++)
  {
    if (S.x < Prompts->Strings[Index].Length())
    {
      S.x = Prompts->Strings[Index].Length();
    }
    S.y += 2;
  }

  if (S.x > 80 - 10)
  {
    S.x = 80 - 10;
  }

  Size = TPoint(S.x + 10, S.y + 6);

  if (!Instructions.IsEmpty())
  {
    GenerateLabel(Instructions, Truncated);
    // dumb way to add empty line
    GenerateLabel("", Truncated);
  }

  for (int Index = 0; Index < Prompts->Count; Index++)
  {
    GenerateLabel(Prompts->Strings[Index], Truncated);

    FEdits->Add(GenerateEdit(bool(Prompts->Objects[Index])));
  }
}
//---------------------------------------------------------------------------
void __fastcall TPasswordDialog::ShowPromptClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);

  WinSCPPlugin->MoreMessageDialog(FPrompt, NULL, qtInformation, qaOK);
}
//---------------------------------------------------------------------------
bool __fastcall TPasswordDialog::Execute(TStrings * Results)
{
  for (int Index = 0; Index < FEdits->Count; Index++)
  {
    reinterpret_cast<TFarEdit *>(FEdits->Items[Index])->Text = Results->Strings[Index];
  }

  bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    for (int Index = 0; Index < FEdits->Count; Index++)
    {
      Results->Strings[Index] = reinterpret_cast<TFarEdit *>(FEdits->Items[Index])->Text;
    }

    if ((SavePasswordCheck != NULL) && SavePasswordCheck->Checked)
    {
      assert(FSessionData != NULL);
      FSessionData->Password = Results->Strings[0];
      // modified only, explicit
      StoredSessions->Save(false, true);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::PasswordDialog(TSessionData * SessionData,
  TPromptKind Kind, AnsiString Name, AnsiString Instructions, TStrings * Prompts,
  TStrings * Results, bool StoredCredentialsTried)
{
  bool Result;
  TPasswordDialog * Dialog = new TPasswordDialog(FPlugin, SessionData->Name,
    Kind, Name, Instructions, Prompts, StoredCredentialsTried);
  try
  {
    Result = Dialog->Execute(Results);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::BannerDialog(AnsiString SessionName,
  const AnsiString & Banner, bool & NeverShowAgain, int Options)
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(FPlugin);
  try
  {
    Dialog->Size = TPoint(70, 21);
    Dialog->Caption = FORMAT(GetMsg(BANNER_TITLE), (SessionName));

    TFarLister * Lister = new TFarLister(Dialog);
    FarWrapText(Banner, Lister->Items, Dialog->BorderBox->Width - 4);
    Lister->Height = 15;
    Lister->Left = Dialog->BorderBox->Left + 1;
    Lister->Right = Dialog->BorderBox->Right - (Lister->ScrollBar ? 0 : 1);

    new TFarSeparator(Dialog);

    TFarCheckBox * NeverShowAgainCheck = NULL;
    if (FLAGCLEAR(Options, boDisableNeverShowAgain))
    {
      NeverShowAgainCheck = new TFarCheckBox(Dialog);
      NeverShowAgainCheck->Caption = GetMsg(BANNER_NEVER_SHOW_AGAIN);
      NeverShowAgainCheck->Visible = FLAGCLEAR(Options, boDisableNeverShowAgain);
      NeverShowAgainCheck->Checked = NeverShowAgain;

      Dialog->NextItemPosition = ipRight;
    }

    TFarButton * Button = new TFarButton(Dialog);
    Button->Caption = GetMsg(BANNER_CONTINUE);
    Button->Default = true;
    Button->Result = brOK;
    if (NeverShowAgainCheck != NULL)
    {
      Button->Left = Dialog->BorderBox->Right - Button->Width - 1;
    }
    else
    {
      Button->CenterGroup = true;
    }

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      if (NeverShowAgainCheck != NULL)
      {
        NeverShowAgain = NeverShowAgainCheck->Checked;
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSessionDialog : public TTabbedDialog
{
public:
  enum TSessionTab { tabSession = 1, tabEnvironment, tabDirectories, tabSFTP, tabSCP, tabFTP,
    tabConnection, tabTunnel, tabProxy, tabSsh, tabKex, tabAuthentication, tabBugs, tabCount };

  __fastcall TSessionDialog(TCustomFarPlugin * AFarPlugin, TSessionAction Action);

  bool __fastcall Execute(TSessionData * Data, TSessionAction & Action);

protected:
  virtual void __fastcall Change();
  virtual bool __fastcall CloseQuery();

private:
  TSessionAction FAction;
  TSessionData * FSessionData;
  int FTransferProtocolIndex;

  TTabButton * SshTab;
  TTabButton * AuthenticatonTab;
  TTabButton * KexTab;
  TTabButton * BugsTab;
  TTabButton * ScpTab;
  TTabButton * SftpTab;
  TTabButton * FtpTab;
  TTabButton * TunnelTab;
  TFarButton * ConnectButton;
  TFarEdit * HostNameEdit;
  TFarEdit * PortNumberEdit;
  TFarEdit * UserNameEdit;
  TFarEdit * PasswordEdit;
  TFarEdit * PrivateKeyEdit;
  TFarComboBox * TransferProtocolCombo;
  TFarCheckBox * AllowScpFallbackCheck;
  TFarText * InsecureLabel;
  TFarCheckBox * UpdateDirectoriesCheck;
  TFarCheckBox * CacheDirectoriesCheck;
  TFarCheckBox * CacheDirectoryChangesCheck;
  TFarCheckBox * PreserveDirectoryChangesCheck;
  TFarCheckBox * ResolveSymlinksCheck;
  TFarEdit * RemoteDirectoryEdit;
  TFarComboBox * EOLTypeCombo;
  TFarRadioButton * DSTModeWinCheck;
  TFarRadioButton * DSTModeKeepCheck;
  TFarRadioButton * DSTModeUnixCheck;
  TFarCheckBox * CompressionCheck;
  TFarRadioButton * SshProt1onlyButton;
  TFarRadioButton * SshProt1Button;
  TFarRadioButton * SshProt2Button;
  TFarRadioButton * SshProt2onlyButton;
  TFarListBox * CipherListBox;
  TFarButton * CipherUpButton;
  TFarButton * CipherDownButton;
  TFarCheckBox * Ssh2DESCheck;
  TFarComboBox * ShellEdit;
  TFarComboBox * ReturnVarEdit;
  TFarCheckBox * LookupUserGroupsCheck;
  TFarCheckBox * ClearAliasesCheck;
  TFarCheckBox * UnsetNationalVarsCheck;
  TFarComboBox * ListingCommandEdit;
  TFarCheckBox * IgnoreLsWarningsCheck;
  TFarCheckBox * SCPLsFullTimeAutoCheck;
  TFarCheckBox * Scp1CompatibilityCheck;
  TFarEdit * PostLoginCommandsEdits[3];
  TFarEdit * TimeDifferenceEdit;
  TFarEdit * TimeDifferenceMinutesEdit;
  TFarEdit * TimeoutEdit;
  TFarRadioButton * PingOffButton;
  TFarRadioButton * PingNullPacketButton;
  TFarRadioButton * PingDummyCommandButton;
  TFarEdit * PingIntervalSecEdit;
  TFarComboBox * SshProxyMethodCombo;
  TFarComboBox * FtpProxyMethodCombo;
  TFarEdit * ProxyHostEdit;
  TFarEdit * ProxyPortEdit;
  TFarEdit * ProxyUsernameEdit;
  TFarEdit * ProxyPasswordEdit;
  TFarText * ProxyLocalCommandLabel;
  TFarEdit * ProxyLocalCommandEdit;
  TFarText * ProxyTelnetCommandLabel;
  TFarEdit * ProxyTelnetCommandEdit;
  TFarCheckBox * ProxyLocalhostCheck;
  TFarRadioButton * ProxyDNSOffButton;
  TFarRadioButton * ProxyDNSAutoButton;
  TFarRadioButton * ProxyDNSOnButton;
  TFarCheckBox * TunnelCheck;
  TFarEdit * TunnelHostNameEdit;
  TFarEdit * TunnelPortNumberEdit;
  TFarEdit * TunnelUserNameEdit;
  TFarEdit * TunnelPasswordEdit;
  TFarEdit * TunnelPrivateKeyEdit;
  TFarComboBox * TunnelLocalPortNumberEdit;
  TFarComboBox * BugIgnore1Combo;
  TFarComboBox * BugPlainPW1Combo;
  TFarComboBox * BugRSA1Combo;
  TFarComboBox * BugHMAC2Combo;
  TFarComboBox * BugDeriveKey2Combo;
  TFarComboBox * BugRSAPad2Combo;
  TFarComboBox * BugPKSessID2Combo;
  TFarComboBox * BugRekey2Combo;
  TFarCheckBox * SshNoUserAuthCheck;
  TFarCheckBox * AuthTISCheck;
  TFarCheckBox * TryAgentCheck;
  TFarCheckBox * AuthKICheck;
  TFarCheckBox * AuthKIPasswordCheck;
  TFarCheckBox * AgentFwdCheck;
  TFarCheckBox * AuthGSSAPICheck2;
  TFarEdit * GSSAPIServerRealmEdit;
  TFarCheckBox * DeleteToRecycleBinCheck;
  TFarCheckBox * OverwrittenToRecycleBinCheck;
  TFarEdit * RecycleBinPathEdit;
  TFarComboBox * SFTPMaxVersionCombo;
  TFarComboBox * SftpServerEdit;
  TFarComboBox * SFTPBugSymlinkCombo;
  TFarComboBox * SFTPBugSignedTSCombo;
  TFarComboBox * UtfCombo;
  TFarListBox * KexListBox;
  TFarButton * KexUpButton;
  TFarButton * KexDownButton;
  TFarEdit * RekeyTimeEdit;
  TFarEdit * RekeyDataEdit;
  TFarRadioButton * IPAutoButton;
  TFarRadioButton * IPv4Button;
  TFarRadioButton * IPv6Button;
  TFarCheckBox * FtpPasvModeCheck;

  void __fastcall LoadPing(TSessionData * SessionData);
  void __fastcall SavePing(TSessionData * SessionData);
  int __fastcall FSProtocolToIndex(TFSProtocol FSProtocol, bool & AllowScpFallback);
  TFSProtocol __fastcall IndexToFSProtocol(int Index, bool AllowScpFallback);
  TFSProtocol __fastcall GetFSProtocol();
  bool __fastcall VerifyKey(AnsiString FileName, bool TypeOnly);
  void __fastcall CipherButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall KexButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall AuthGSSAPICheckAllowChange(TFarDialogItem * Sender, long NewState, bool & Allow);
  void __fastcall UnixEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall WindowsEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall UpdateControls();
  void __fastcall TransferProtocolComboChange();
};
//---------------------------------------------------------------------------
#define BUG(BUGID, MSG, PREFIX) \
  TRISTATE(PREFIX ## Bug ## BUGID ## Combo, PREFIX ## Bug[sb ## BUGID], MSG)
#define BUGS() \
  BUG(Ignore1, LOGIN_BUGS_IGNORE1, ); \
  BUG(PlainPW1, LOGIN_BUGS_PLAIN_PW1, ); \
  BUG(RSA1, LOGIN_BUGS_RSA1, ); \
  BUG(HMAC2, LOGIN_BUGS_HMAC2, ); \
  BUG(DeriveKey2, LOGIN_BUGS_DERIVE_KEY2, ); \
  BUG(RSAPad2, LOGIN_BUGS_RSA_PAD2, ); \
  BUG(PKSessID2, LOGIN_BUGS_PKSESSID2, ); \
  BUG(Rekey2, LOGIN_BUGS_REKEY2, );
#define SFTP_BUGS() \
  BUG(Symlink, LOGIN_SFTP_BUGS_SYMLINK, SFTP); \
  BUG(SignedTS, LOGIN_SFTP_BUGS_SIGNED_TS, SFTP);
#define UTF_TRISTATE() \
  TRISTATE(UtfCombo, Utf, LOGIN_UTF);
//---------------------------------------------------------------------------
static const TFSProtocol FSOrder[] = { fsSFTPonly, fsSCPonly, fsFTP };
//---------------------------------------------------------------------------
__fastcall TSessionDialog::TSessionDialog(TCustomFarPlugin * AFarPlugin,
  TSessionAction Action) : TTabbedDialog(AFarPlugin, tabCount),
  FAction(Action)
{
  TPoint S = TPoint(67, 23);
  bool Limited = (S.y > MaxSize.y);
  if (Limited)
  {
    S.y = MaxSize.y;
  }
  Size = S;

  #define TRISTATE(COMBO, PROP, MSG) \
    Text = new TFarText(this); \
    Text->Caption = GetMsg(MSG); \
    NextItemPosition = ipRight; \
    COMBO = new TFarComboBox(this); \
    COMBO->DropDownList = true; \
    COMBO->Width = 7; \
    COMBO->Items->BeginUpdate(); \
    try \
    { \
      COMBO->Items->Add(GetMsg(LOGIN_BUGS_AUTO)); \
      COMBO->Items->Add(GetMsg(LOGIN_BUGS_OFF)); \
      COMBO->Items->Add(GetMsg(LOGIN_BUGS_ON)); \
    } \
    __finally \
    { \
      COMBO->Items->EndUpdate(); \
    } \
    Text->EnabledFollow = COMBO; \
    NextItemPosition = ipNewLine;

  TRect CRect = ClientRect;

  TFarButton * Button;
  TTabButton * Tab;
  TFarSeparator * Separator;
  TFarText * Text;
  int GroupTop;
  int Pos;

  TFarButtonBrackets TabBrackets = brNone;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(LOGIN_TAB_SESSION);
  Tab->Tab = tabSession;
  Tab->Brackets = TabBrackets;

  NextItemPosition = ipRight;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(LOGIN_TAB_ENVIRONMENT);
  Tab->Tab = tabEnvironment;
  Tab->Brackets = TabBrackets;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(LOGIN_TAB_DIRECTORIES);
  Tab->Tab = tabDirectories;
  Tab->Brackets = TabBrackets;

  SftpTab = new TTabButton(this);
  SftpTab->TabName = GetMsg(LOGIN_TAB_SFTP);
  SftpTab->Tab = tabSFTP;
  SftpTab->Brackets = TabBrackets;

  ScpTab = new TTabButton(this);
  ScpTab->TabName = GetMsg(LOGIN_TAB_SCP);
  ScpTab->Tab = tabSCP;
  ScpTab->Brackets = TabBrackets;

  FtpTab = new TTabButton(this);
  FtpTab->TabName = GetMsg(LOGIN_TAB_FTP);
  FtpTab->Tab = tabFTP;
  FtpTab->Brackets = TabBrackets;

  NextItemPosition = ipNewLine;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(LOGIN_TAB_CONNECTION);
  Tab->Tab = tabConnection;
  Tab->Brackets = TabBrackets;

  NextItemPosition = ipRight;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(LOGIN_TAB_PROXY);
  Tab->Tab = tabProxy;
  Tab->Brackets = TabBrackets;

  TunnelTab = new TTabButton(this);
  TunnelTab->TabName = GetMsg(LOGIN_TAB_TUNNEL);
  TunnelTab->Tab = tabTunnel;
  TunnelTab->Brackets = TabBrackets;

  SshTab = new TTabButton(this);
  SshTab->TabName = GetMsg(LOGIN_TAB_SSH);
  SshTab->Tab = tabSsh;
  SshTab->Brackets = TabBrackets;

  KexTab = new TTabButton(this);
  KexTab->TabName = GetMsg(LOGIN_TAB_KEX);
  KexTab->Tab = tabKex;
  KexTab->Brackets = TabBrackets;

  AuthenticatonTab = new TTabButton(this);
  AuthenticatonTab->TabName = GetMsg(LOGIN_TAB_AUTH);
  AuthenticatonTab->Tab = tabAuthentication;
  AuthenticatonTab->Brackets = TabBrackets;

  BugsTab = new TTabButton(this);
  BugsTab->TabName = GetMsg(LOGIN_TAB_BUGS);
  BugsTab->Tab = tabBugs;
  BugsTab->Brackets = TabBrackets;

  // Sesion tab

  NextItemPosition = ipNewLine;
  DefaultGroup = tabSession;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_GROUP_SESSION);
  GroupTop = Separator->Top;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_HOST_NAME);

  HostNameEdit = new TFarEdit(this);
  HostNameEdit->Right = CRect.Right - 12 - 2;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PORT_NUMBER);
  Text->Move(0, -1);

  NextItemPosition = ipBelow;

  PortNumberEdit = new TFarEdit(this);
  PortNumberEdit->Fixed = true;
  PortNumberEdit->Mask = "99999";

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_USER_NAME);

  UserNameEdit = new TFarEdit(this);
  UserNameEdit->Width = UserNameEdit->Width / 2 - 1;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PASSWORD);
  Text->Move(0, -1);

  NextItemPosition = ipBelow;

  PasswordEdit = new TFarEdit(this);
  PasswordEdit->Password = true;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PRIVATE_KEY);

  PrivateKeyEdit = new TFarEdit(this);
  Text->EnabledFollow = PrivateKeyEdit;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_GROUP_PROTOCOL);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TRANSFER_PROTOCOL);

  NextItemPosition = ipRight;

  TransferProtocolCombo = new TFarComboBox(this);
  TransferProtocolCombo->DropDownList = true;
  TransferProtocolCombo->Width = 7;
  TransferProtocolCombo->Items->Add(GetMsg(LOGIN_SFTP));
  TransferProtocolCombo->Items->Add(GetMsg(LOGIN_SCP));
  #ifndef NO_FILEZILLA
  TransferProtocolCombo->Items->Add(GetMsg(LOGIN_FTP));
  #endif

  AllowScpFallbackCheck = new TFarCheckBox(this);
  AllowScpFallbackCheck->Caption = GetMsg(LOGIN_ALLOW_SCP_FALLBACK);

  InsecureLabel = new TFarText(this);
  InsecureLabel->Caption = GetMsg(LOGIN_INSECURE);
  InsecureLabel->MoveAt(AllowScpFallbackCheck->Left, AllowScpFallbackCheck->Top);

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->Top = CRect.Bottom - 3;
  Text->Bottom = Text->Top;
  Text->Caption = GetMsg(LOGIN_TAB_HINT1);
  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TAB_HINT2);

  // Environment tab

  DefaultGroup = tabEnvironment;
  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_ENVIRONMENT_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_EOL_TYPE);

  NextItemPosition = ipRight;

  EOLTypeCombo = new TFarComboBox(this);
  EOLTypeCombo->DropDownList = true;
  EOLTypeCombo->Width = 7;
  EOLTypeCombo->Items->Add("LF");
  EOLTypeCombo->Items->Add("CR/LF");

  NextItemPosition = ipNewLine;

  UTF_TRISTATE();

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TIME_DIFFERENCE);

  NextItemPosition = ipRight;

  TimeDifferenceEdit = new TFarEdit(this);
  TimeDifferenceEdit->Fixed = true;
  TimeDifferenceEdit->Mask = "###";
  TimeDifferenceEdit->Width = 4;
  Text->EnabledFollow = TimeDifferenceEdit;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TIME_DIFFERENCE_HOURS);
  Text->EnabledFollow = TimeDifferenceEdit;

  TimeDifferenceMinutesEdit = new TFarEdit(this);
  TimeDifferenceMinutesEdit->Fixed = true;
  TimeDifferenceMinutesEdit->Mask = "###";
  TimeDifferenceMinutesEdit->Width = 4;
  TimeDifferenceMinutesEdit->EnabledFollow = TimeDifferenceEdit;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TIME_DIFFERENCE_MINUTES);
  Text->EnabledFollow = TimeDifferenceEdit;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_DST_MODE_GROUP);

  DSTModeUnixCheck = new TFarRadioButton(this);
  DSTModeUnixCheck->Caption = GetMsg(LOGIN_DST_MODE_UNIX);

  DSTModeWinCheck = new TFarRadioButton(this);
  DSTModeWinCheck->Caption = GetMsg(LOGIN_DST_MODE_WIN);
  DSTModeWinCheck->EnabledFollow = DSTModeUnixCheck;

  DSTModeKeepCheck = new TFarRadioButton(this);
  DSTModeKeepCheck->Caption = GetMsg(LOGIN_DST_MODE_KEEP);
  DSTModeKeepCheck->EnabledFollow = DSTModeUnixCheck;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(LOGIN_ENVIRONMENT_UNIX);
  Button->OnClick = UnixEnvironmentButtonClick;
  Button->CenterGroup = true;

  NextItemPosition = ipRight;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(LOGIN_ENVIRONMENT_WINDOWS);
  Button->OnClick = WindowsEnvironmentButtonClick;
  Button->CenterGroup = true;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_RECYCLE_BIN_GROUP);

  DeleteToRecycleBinCheck = new TFarCheckBox(this);
  DeleteToRecycleBinCheck->Caption = GetMsg(LOGIN_RECYCLE_BIN_DELETE);

  OverwrittenToRecycleBinCheck = new TFarCheckBox(this);
  OverwrittenToRecycleBinCheck->Caption = GetMsg(LOGIN_RECYCLE_BIN_OVERWRITE);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_RECYCLE_BIN_LABEL);

  RecycleBinPathEdit = new TFarEdit(this);
  Text->EnabledFollow = RecycleBinPathEdit;

  NextItemPosition = ipNewLine;

  // Directories tab

  DefaultGroup = tabDirectories;
  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_DIRECTORIES_GROUP);

  UpdateDirectoriesCheck = new TFarCheckBox(this);
  UpdateDirectoriesCheck->Caption = GetMsg(LOGIN_UPDATE_DIRECTORIES);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_REMOTE_DIRECTORY);

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->History = REMOTE_DIR_HISTORY;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_DIRECTORY_OPTIONS_GROUP);

  CacheDirectoriesCheck = new TFarCheckBox(this);
  CacheDirectoriesCheck->Caption = GetMsg(LOGIN_CACHE_DIRECTORIES);

  CacheDirectoryChangesCheck = new TFarCheckBox(this);
  CacheDirectoryChangesCheck->Caption = GetMsg(LOGIN_CACHE_DIRECTORY_CHANGES);

  PreserveDirectoryChangesCheck = new TFarCheckBox(this);
  PreserveDirectoryChangesCheck->Caption = GetMsg(LOGIN_PRESERVE_DIRECTORY_CHANGES);
  PreserveDirectoryChangesCheck->Left += 4;

  ResolveSymlinksCheck = new TFarCheckBox(this);
  ResolveSymlinksCheck->Caption = GetMsg(LOGIN_RESOLVE_SYMLINKS);

  new TFarSeparator(this);

  // SCP Tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabSCP;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_SHELL_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_SHELL_SHELL);

  NextItemPosition = ipRight;

  ShellEdit = new TFarComboBox(this);
  ShellEdit->Items->Add(GetMsg(LOGIN_SHELL_SHELL_DEFAULT));
  ShellEdit->Items->Add("/bin/bash");
  ShellEdit->Items->Add("/bin/ksh");

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_SHELL_RETURN_VAR);

  NextItemPosition = ipRight;

  ReturnVarEdit = new TFarComboBox(this);
  ReturnVarEdit->Items->Add(GetMsg(LOGIN_SHELL_RETURN_VAR_AUTODETECT));
  ReturnVarEdit->Items->Add("?");
  ReturnVarEdit->Items->Add("status");

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_SCP_LS_OPTIONS_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_LISTING_COMMAND);

  NextItemPosition = ipRight;

  ListingCommandEdit = new TFarComboBox(this);
  ListingCommandEdit->Items->Add("ls -la");
  ListingCommandEdit->Items->Add("ls -gla");
  Text->EnabledFollow = ListingCommandEdit;

  NextItemPosition = ipNewLine;

  IgnoreLsWarningsCheck = new TFarCheckBox(this);
  IgnoreLsWarningsCheck->Caption = GetMsg(LOGIN_IGNORE_LS_WARNINGS);

  NextItemPosition = ipRight;

  SCPLsFullTimeAutoCheck = new TFarCheckBox(this);
  SCPLsFullTimeAutoCheck->Caption = GetMsg(LOGIN_SCP_LS_FULL_TIME_AUTO);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_SCP_OPTIONS);

  LookupUserGroupsCheck = new TFarCheckBox(this);
  LookupUserGroupsCheck->Caption = GetMsg(LOGIN_LOOKUP_USER_GROUPS);

  NextItemPosition = ipRight;

  UnsetNationalVarsCheck = new TFarCheckBox(this);
  UnsetNationalVarsCheck->Caption = GetMsg(LOGIN_CLEAR_NATIONAL_VARS);

  NextItemPosition = ipNewLine;

  ClearAliasesCheck = new TFarCheckBox(this);
  ClearAliasesCheck->Caption = GetMsg(LOGIN_CLEAR_ALIASES);

  NextItemPosition = ipRight;

  Scp1CompatibilityCheck = new TFarCheckBox(this);
  Scp1CompatibilityCheck->Caption = GetMsg(LOGIN_SCP1_COMPATIBILITY);

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  // SFTP Tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabSFTP;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_SFTP_PROTOCOL_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_SFTP_SERVER);
  NextItemPosition = ipRight;
  SftpServerEdit = new TFarComboBox(this);
  SftpServerEdit->Items->Add(GetMsg(LOGIN_SFTP_SERVER_DEFAULT));
  SftpServerEdit->Items->Add("/bin/sftp-server");
  SftpServerEdit->Items->Add("sudo su -c /bin/sftp-server");
  Text->EnabledFollow = SftpServerEdit;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_SFTP_MAX_VERSION);
  NextItemPosition = ipRight;
  SFTPMaxVersionCombo = new TFarComboBox(this);
  SFTPMaxVersionCombo->DropDownList = true;
  SFTPMaxVersionCombo->Width = 7;
  for (int i = 0; i <= 5; i++)
  {
    SFTPMaxVersionCombo->Items->Add(IntToStr(i));
  }
  Text->EnabledFollow = SFTPMaxVersionCombo;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_SFTP_BUGS_GROUP);

  SFTP_BUGS();

  new TFarSeparator(this);

  // FTP tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabFTP;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_FTP_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_FTP_POST_LOGIN_COMMANDS);

  for (int Index = 0; Index < LENOF(PostLoginCommandsEdits); Index++)
  {
    TFarEdit * Edit = new TFarEdit(this);
    PostLoginCommandsEdits[Index] = Edit;
  }

  new TFarSeparator(this);

  // Connection tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabConnection;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_CONNECTION_GROUP);

  FtpPasvModeCheck = new TFarCheckBox(this);
  FtpPasvModeCheck->Caption = GetMsg(LOGIN_FTP_PASV_MODE);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_TIMEOUTS_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TIMEOUT);

  NextItemPosition = ipRight;

  TimeoutEdit = new TFarEdit(this);
  TimeoutEdit->Fixed = true;
  TimeoutEdit->Mask = "####";
  TimeoutEdit->Width = 5;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TIMEOUT_SECONDS);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_PING_GROUP);

  PingOffButton = new TFarRadioButton(this);
  PingOffButton->Caption = GetMsg(LOGIN_PING_OFF);

  PingNullPacketButton = new TFarRadioButton(this);
  PingNullPacketButton->Caption = GetMsg(LOGIN_PING_NULL_PACKET);

  PingDummyCommandButton = new TFarRadioButton(this);
  PingDummyCommandButton->Caption = GetMsg(LOGIN_PING_DUMMY_COMMAND);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PING_INTERVAL);
  Text->EnabledDependencyNegative = PingOffButton;

  NextItemPosition = ipRight;

  PingIntervalSecEdit = new TFarEdit(this);
  PingIntervalSecEdit->Fixed = true;
  PingIntervalSecEdit->Mask = "####";
  PingIntervalSecEdit->Width = 6;
  PingIntervalSecEdit->EnabledDependencyNegative = PingOffButton;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_IP_GROUP);

  IPAutoButton = new TFarRadioButton(this);
  IPAutoButton->Caption = GetMsg(LOGIN_IP_AUTO);

  NextItemPosition = ipRight;

  IPv4Button = new TFarRadioButton(this);
  IPv4Button->Caption = GetMsg(LOGIN_IP_V4);

  IPv6Button = new TFarRadioButton(this);
  IPv6Button->Caption = GetMsg(LOGIN_IP_V6);

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  // Proxy tab

  DefaultGroup = tabProxy;
  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_PROXY_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_METHOD);

  NextItemPosition = ipRight;

  FtpProxyMethodCombo = new TFarComboBox(this);
  FtpProxyMethodCombo->DropDownList = true;
  FtpProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_NONE));
  FtpProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_SOCKS4));
  FtpProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_SOCKS5));
  FtpProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_HTTP));
  FtpProxyMethodCombo->Right = CRect.Right - 12 - 2;

  SshProxyMethodCombo = new TFarComboBox(this);
  SshProxyMethodCombo->Left = FtpProxyMethodCombo->Left;
  SshProxyMethodCombo->Width = FtpProxyMethodCombo->Width;
  FtpProxyMethodCombo->Right = FtpProxyMethodCombo->Right;
  SshProxyMethodCombo->DropDownList = true;
  SshProxyMethodCombo->Items->AddStrings(FtpProxyMethodCombo->Items);
  SshProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_TELNET));
  SshProxyMethodCombo->Items->Add(GetMsg(LOGIN_PROXY_LOCAL));

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_HOST);

  ProxyHostEdit = new TFarEdit(this);
  ProxyHostEdit->Right = FtpProxyMethodCombo->Right;
  Text->EnabledFollow = ProxyHostEdit;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_PORT);
  Text->Move(0, -1);

  NextItemPosition = ipBelow;

  ProxyPortEdit = new TFarEdit(this);
  ProxyPortEdit->Fixed = true;
  ProxyPortEdit->Mask = "99999";
  Text->EnabledFollow = ProxyPortEdit;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_USERNAME);

  ProxyUsernameEdit = new TFarEdit(this);
  ProxyUsernameEdit->Width = ProxyUsernameEdit->Width / 2 - 1;
  Text->EnabledFollow = ProxyUsernameEdit;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_PASSWORD);
  Text->Move(0, -1);

  NextItemPosition = ipBelow;

  ProxyPasswordEdit = new TFarEdit(this);
  ProxyPasswordEdit->Password = true;
  Text->EnabledFollow = ProxyPasswordEdit;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_PROXY_SETTINGS_GROUP);

  ProxyTelnetCommandLabel = new TFarText(this);
  ProxyTelnetCommandLabel->Caption = GetMsg(LOGIN_PROXY_TELNET_COMMAND);

  NextItemPosition = ipRight;

  ProxyTelnetCommandEdit = new TFarEdit(this);
  ProxyTelnetCommandLabel->EnabledFollow = ProxyTelnetCommandEdit;

  NextItemPosition = ipNewLine;

  ProxyLocalCommandLabel = new TFarText(this);
  ProxyLocalCommandLabel->Caption = GetMsg(LOGIN_PROXY_LOCAL_COMMAND);
  ProxyLocalCommandLabel->Move(0, -1);

  NextItemPosition = ipRight;

  ProxyLocalCommandEdit = new TFarEdit(this);
  ProxyLocalCommandLabel->EnabledFollow = ProxyLocalCommandEdit;

  NextItemPosition = ipNewLine;

  ProxyLocalhostCheck = new TFarCheckBox(this);
  ProxyLocalhostCheck->Caption = GetMsg(LOGIN_PROXY_LOCALHOST);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PROXY_DNS);

  ProxyDNSOffButton = new TFarRadioButton(this);
  ProxyDNSOffButton->Caption = GetMsg(LOGIN_PROXY_DNS_NO);
  Text->EnabledFollow = ProxyDNSOffButton;

  NextItemPosition = ipRight;

  ProxyDNSAutoButton = new TFarRadioButton(this);
  ProxyDNSAutoButton->Caption = GetMsg(LOGIN_PROXY_DNS_AUTO);
  ProxyDNSAutoButton->EnabledFollow = ProxyDNSOffButton;

  ProxyDNSOnButton = new TFarRadioButton(this);
  ProxyDNSOnButton->Caption = GetMsg(LOGIN_PROXY_DNS_YES);
  ProxyDNSOnButton->EnabledFollow = ProxyDNSOffButton;

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  // Tunnel tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabTunnel;
  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_TUNNEL_GROUP);

  TunnelCheck = new TFarCheckBox(this);
  TunnelCheck->Caption = GetMsg(LOGIN_TUNNEL_TUNNEL);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_TUNNEL_SESSION_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_HOST_NAME);
  Text->EnabledDependency = TunnelCheck;

  TunnelHostNameEdit = new TFarEdit(this);
  TunnelHostNameEdit->Right = CRect.Right - 12 - 2;
  TunnelHostNameEdit->EnabledDependency = TunnelCheck;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PORT_NUMBER);
  Text->Move(0, -1);
  Text->EnabledDependency = TunnelCheck;

  NextItemPosition = ipBelow;

  TunnelPortNumberEdit = new TFarEdit(this);
  TunnelPortNumberEdit->Fixed = true;
  TunnelPortNumberEdit->Mask = "99999";
  TunnelPortNumberEdit->EnabledDependency = TunnelCheck;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_USER_NAME);
  Text->EnabledDependency = TunnelCheck;

  TunnelUserNameEdit = new TFarEdit(this);
  TunnelUserNameEdit->Width = TunnelUserNameEdit->Width / 2 - 1;
  TunnelUserNameEdit->EnabledDependency = TunnelCheck;

  NextItemPosition = ipRight;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PASSWORD);
  Text->Move(0, -1);
  Text->EnabledDependency = TunnelCheck;

  NextItemPosition = ipBelow;

  TunnelPasswordEdit = new TFarEdit(this);
  TunnelPasswordEdit->Password = true;
  TunnelPasswordEdit->EnabledDependency = TunnelCheck;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_PRIVATE_KEY);
  Text->EnabledDependency = TunnelCheck;

  TunnelPrivateKeyEdit = new TFarEdit(this);
  TunnelPrivateKeyEdit->EnabledDependency = TunnelCheck;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_TUNNEL_OPTIONS_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER);
  Text->EnabledDependency = TunnelCheck;

  NextItemPosition = ipRight;

  TunnelLocalPortNumberEdit = new TFarComboBox(this);
  TunnelLocalPortNumberEdit->Left = TunnelPortNumberEdit->Left;
  TunnelLocalPortNumberEdit->EnabledDependency = TunnelCheck;
  TunnelLocalPortNumberEdit->Items->BeginUpdate();
  try
  {
    TunnelLocalPortNumberEdit->Items->Add(GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER_AUTOASSIGN));
    for (int Index = Configuration->TunnelLocalPortNumberLow;
         Index <= Configuration->TunnelLocalPortNumberHigh; Index++)
    {
      TunnelLocalPortNumberEdit->Items->Add(IntToStr(Index));
    }
  }
  __finally
  {
    TunnelLocalPortNumberEdit->Items->EndUpdate();
  }

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  // SSH tab

  NextItemPosition = ipNewLine;

  DefaultGroup = tabSsh;
  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_SSH_GROUP);

  CompressionCheck = new TFarCheckBox(this);
  CompressionCheck->Caption = GetMsg(LOGIN_COMPRESSION);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_SSH_PROTOCOL_GROUP);

  SshProt1onlyButton = new TFarRadioButton(this);
  SshProt1onlyButton->Caption = GetMsg(LOGIN_SSH1_ONLY);

  NextItemPosition = ipRight;

  SshProt1Button = new TFarRadioButton(this);
  SshProt1Button->Caption = GetMsg(LOGIN_SSH1);

  SshProt2Button = new TFarRadioButton(this);
  SshProt2Button->Caption = GetMsg(LOGIN_SSH2);

  SshProt2onlyButton = new TFarRadioButton(this);
  SshProt2onlyButton->Caption = GetMsg(LOGIN_SSH2_ONLY);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_ENCRYPTION_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_CIPHER);

  CipherListBox = new TFarListBox(this);
  CipherListBox->Right = CipherListBox->Right - 15;
  CipherListBox->Height = 1 + CIPHER_COUNT + 1;
  Pos = CipherListBox->Bottom;

  NextItemPosition = ipRight;

  CipherUpButton = new TFarButton(this);
  CipherUpButton->Caption = GetMsg(LOGIN_UP);
  CipherUpButton->Move(0, 1);
  CipherUpButton->Result = -1;
  CipherUpButton->OnClick = CipherButtonClick;

  NextItemPosition = ipBelow;

  CipherDownButton = new TFarButton(this);
  CipherDownButton->Caption = GetMsg(LOGIN_DOWN);
  CipherDownButton->Result = 1;
  CipherDownButton->OnClick = CipherButtonClick;

  NextItemPosition = ipNewLine;

  if (!Limited)
  {
    Ssh2DESCheck = new TFarCheckBox(this);
    Ssh2DESCheck->Move(0, Pos - Ssh2DESCheck->Top + 1);
    Ssh2DESCheck->Caption = GetMsg(LOGIN_SSH2DES);
    Ssh2DESCheck->EnabledDependencyNegative = SshProt1onlyButton;
  }
  else
  {
    Ssh2DESCheck = NULL;
  }

  // KEX tab

  DefaultGroup = tabKex;
  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_KEX_REEXCHANGE_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_KEX_REKEY_TIME);
  Text->EnabledDependencyNegative = SshProt1onlyButton;

  NextItemPosition = ipRight;

  RekeyTimeEdit = new TFarEdit(this);
  RekeyTimeEdit->Fixed = true;
  RekeyTimeEdit->Mask = "####";
  RekeyTimeEdit->Width = 6;
  RekeyTimeEdit->EnabledDependencyNegative = SshProt1onlyButton;

  NextItemPosition = ipNewLine;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_KEX_REKEY_DATA);
  Text->EnabledDependencyNegative = SshProt1onlyButton;

  NextItemPosition = ipRight;

  RekeyDataEdit = new TFarEdit(this);
  RekeyDataEdit->Width = 6;
  RekeyDataEdit->EnabledDependencyNegative = SshProt1onlyButton;

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_KEX_OPTIONS_GROUP);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_KEX_LIST);
  Text->EnabledDependencyNegative = SshProt1onlyButton;

  KexListBox = new TFarListBox(this);
  KexListBox->Right = KexListBox->Right - 15;
  KexListBox->Height = 1 + KEX_COUNT + 1;
  KexListBox->EnabledDependencyNegative = SshProt1onlyButton;

  NextItemPosition = ipRight;

  KexUpButton = new TFarButton(this);
  KexUpButton->Caption = GetMsg(LOGIN_UP);
  KexUpButton->Move(0, 1);
  KexUpButton->Result = -1;
  KexUpButton->OnClick = KexButtonClick;

  NextItemPosition = ipBelow;

  KexDownButton = new TFarButton(this);
  KexDownButton->Caption = GetMsg(LOGIN_DOWN);
  KexDownButton->Result = 1;
  KexDownButton->OnClick = KexButtonClick;

  NextItemPosition = ipNewLine;

  // Authentication tab

  DefaultGroup = tabAuthentication;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;

  SshNoUserAuthCheck = new TFarCheckBox(this);
  SshNoUserAuthCheck->Caption = GetMsg(LOGIN_AUTH_SSH_NO_USER_AUTH);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_AUTH_GROUP);

  TryAgentCheck = new TFarCheckBox(this);
  TryAgentCheck->Caption = GetMsg(LOGIN_AUTH_TRY_AGENT);

  AuthTISCheck = new TFarCheckBox(this);
  AuthTISCheck->Caption = GetMsg(LOGIN_AUTH_TIS);

  AuthKICheck = new TFarCheckBox(this);
  AuthKICheck->Caption = GetMsg(LOGIN_AUTH_KI);

  AuthKIPasswordCheck = new TFarCheckBox(this);
  AuthKIPasswordCheck->Caption = GetMsg(LOGIN_AUTH_KI_PASSWORD);
  AuthKIPasswordCheck->Move(4, 0);

  AuthGSSAPICheck2 = new TFarCheckBox(this);
  AuthGSSAPICheck2->Caption = GetMsg(LOGIN_AUTH_GSSAPI);
  AuthGSSAPICheck2->OnAllowChange = AuthGSSAPICheckAllowChange;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(LOGIN_AUTH_PARAMS_GROUP);

  AgentFwdCheck = new TFarCheckBox(this);
  AgentFwdCheck->Caption = GetMsg(LOGIN_AUTH_AGENT_FWD);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LOGIN_AUTH_GSSAPI_SERVER_REALM);
  GSSAPIServerRealmEdit = new TFarEdit(this);
  Text->EnabledFollow = GSSAPIServerRealmEdit;

  new TFarSeparator(this);

  // Bugs tab

  DefaultGroup = tabBugs;

  Separator = new TFarSeparator(this);
  Separator->Position = GroupTop;
  Separator->Caption = GetMsg(LOGIN_BUGS_GROUP);

  BUGS();

  BugIgnore1Combo->EnabledDependencyNegative = SshProt2onlyButton;
  BugPlainPW1Combo->EnabledDependencyNegative = SshProt2onlyButton;
  BugRSA1Combo->EnabledDependencyNegative = SshProt2onlyButton;
  BugHMAC2Combo->EnabledDependencyNegative = SshProt1onlyButton;
  BugDeriveKey2Combo->EnabledDependencyNegative = SshProt1onlyButton;
  BugRSAPad2Combo->EnabledDependencyNegative = SshProt1onlyButton;
  BugPKSessID2Combo->EnabledDependencyNegative = SshProt1onlyButton;
  BugRekey2Combo->EnabledDependencyNegative = SshProt1onlyButton;

  #undef TRISTATE

  new TFarSeparator(this);

  // Buttons

  NextItemPosition = ipNewLine;
  DefaultGroup = 0;

  Separator = new TFarSeparator(this);
  Separator->Position = CRect.Bottom - 1;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_OK);
  Button->Default = (Action != saConnect);
  Button->Result = brOK;
  Button->CenterGroup = true;

  NextItemPosition = ipRight;

  ConnectButton = new TFarButton(this);
  ConnectButton->Caption = GetMsg(LOGIN_CONNECT_BUTTON);
  ConnectButton->Default = (Action == saConnect);
  ConnectButton->Result = brConnect;
  ConnectButton->CenterGroup = true;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_Cancel);
  Button->Result = brCancel;
  Button->CenterGroup = true;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::Change()
{
  TTabbedDialog::Change();

  if (Handle && !ChangesLocked())
  {
    if (FTransferProtocolIndex != TransferProtocolCombo->Items->Selected)
    {
      TransferProtocolComboChange();
    }

    LockChanges();
    try
    {
      UpdateControls();
    }
    __finally
    {
      UnlockChanges();
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::TransferProtocolComboChange()
{
  // note that this modifies the session for good,
  // even if user cancels the dialog
  SavePing(FSessionData);

  FTransferProtocolIndex = TransferProtocolCombo->Items->Selected;

  LoadPing(FSessionData);
  if (GetFSProtocol() == fsFTP)
  {
    if (PortNumberEdit->AsInteger == 22)
    {
      PortNumberEdit->AsInteger = 21;
    }
  }
  else
  {
    if (PortNumberEdit->AsInteger == 21)
    {
      PortNumberEdit->AsInteger = 22;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::UpdateControls()
{
  TFSProtocol FSProtocol = GetFSProtocol();
  bool InternalSshProtocol =
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) || (FSProtocol == fsSCPonly);
  bool SshProtocol = InternalSshProtocol;
  bool SftpProtocol = (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP);
  bool ScpOnlyProtocol = (FSProtocol == fsSCPonly);
  bool FtpProtocol = (FSProtocol == fsFTP);

  ConnectButton->Enabled = !HostNameEdit->IsEmpty;

  // Basic tab
  AllowScpFallbackCheck->Visible =
    TransferProtocolCombo->Visible &&
    (IndexToFSProtocol(TransferProtocolCombo->Items->Selected, false) == fsSFTPonly);
  InsecureLabel->Visible = TransferProtocolCombo->Visible && !SshProtocol;
  PrivateKeyEdit->Enabled = SshProtocol;

  // Connection sheet
  FtpPasvModeCheck->Enabled = FtpProtocol;
  if (FtpProtocol && (FtpProxyMethodCombo->Items->Selected != pmNone) && !FtpPasvModeCheck->Checked)
  {
    FtpPasvModeCheck->Checked = true;
    TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
    WinSCPPlugin->MoreMessageDialog(GetMsg(FTP_PASV_MODE_REQUIRED),
      NULL, qtInformation, qaOK);
  }
  PingNullPacketButton->Enabled = SshProtocol;
  IPAutoButton->Enabled = SshProtocol;

  // SFTP tab
  SftpTab->Enabled = SftpProtocol;

  // FTP tab
  FtpTab->Enabled = FtpProtocol;

  // SSH tab
  SshTab->Enabled = SshProtocol;
  CipherUpButton->Enabled = CipherListBox->Items->Selected;
  CipherDownButton->Enabled =
    CipherListBox->Items->Selected < CipherListBox->Items->Count - 1;

  // Authentication tab
  AuthenticatonTab->Enabled = SshProtocol;
  SshNoUserAuthCheck->Enabled = !SshProt1onlyButton->Checked;
  bool Authentication = !SshNoUserAuthCheck->Enabled || !SshNoUserAuthCheck->Checked;
  TryAgentCheck->Enabled = Authentication;
  AuthTISCheck->Enabled = Authentication && !SshProt2onlyButton->Checked;
  AuthKICheck->Enabled = Authentication && !SshProt1onlyButton->Checked;
  AuthKIPasswordCheck->Enabled =
    Authentication &&
    ((AuthTISCheck->Enabled && AuthTISCheck->Checked) ||
     (AuthKICheck->Enabled && AuthKICheck->Checked));
  AuthGSSAPICheck2->Enabled =
    Authentication && !SshProt1onlyButton->Checked;
  GSSAPIServerRealmEdit->Enabled =
    AuthGSSAPICheck2->Enabled && AuthGSSAPICheck2->Checked;

  // Directories tab
  CacheDirectoryChangesCheck->Enabled =
    (FSProtocol != fsSCPonly) || CacheDirectoriesCheck->Checked;
  PreserveDirectoryChangesCheck->Enabled =
    CacheDirectoryChangesCheck->IsEnabled && CacheDirectoryChangesCheck->Checked;
  ResolveSymlinksCheck->Enabled = (FSProtocol != fsFTP);

  // Environment tab
  DSTModeUnixCheck->Enabled = (FSProtocol != fsFTP);
  UtfCombo->Enabled = (FSProtocol != fsSCPonly);
  TimeDifferenceEdit->Enabled = ((FSProtocol == fsFTP) || (FSProtocol == fsSCPonly));

  // Recycle bin tab
  OverwrittenToRecycleBinCheck->Enabled = (FSProtocol != fsSCPonly) &&
    (FSProtocol != fsFTP);
  RecycleBinPathEdit->Enabled =
    (DeleteToRecycleBinCheck->IsEnabled && DeleteToRecycleBinCheck->Checked) ||
    (OverwrittenToRecycleBinCheck->IsEnabled && OverwrittenToRecycleBinCheck->Checked);

  // Kex tab
  KexTab->Enabled = SshProtocol && !SshProt1onlyButton->Checked &&
    (BugRekey2Combo->Items->Selected != 2);
  KexUpButton->Enabled = (KexListBox->Items->Selected > 0);
  KexDownButton->Enabled =
    (KexListBox->Items->Selected < KexListBox->Items->Count - 1);

  // Bugs tab
  BugsTab->Enabled = SshProtocol;

  // Scp/Shell tab
  ScpTab->Enabled = InternalSshProtocol;
  // disable also for SFTP with SCP fallback, as if someone wants to configure
  // these he/she probably intends to use SCP and should explicitly select it.
  // (note that these are not used for secondary shell session)
  ListingCommandEdit->Enabled = ScpOnlyProtocol;
  IgnoreLsWarningsCheck->Enabled = ScpOnlyProtocol;
  SCPLsFullTimeAutoCheck->Enabled = ScpOnlyProtocol;
  LookupUserGroupsCheck->Enabled = ScpOnlyProtocol;
  UnsetNationalVarsCheck->Enabled = ScpOnlyProtocol;
  ClearAliasesCheck->Enabled = ScpOnlyProtocol;
  Scp1CompatibilityCheck->Enabled = ScpOnlyProtocol;

  // Connection/Proxy tab
  TFarComboBox * ProxyMethodCombo = (SshProtocol ? SshProxyMethodCombo : FtpProxyMethodCombo);
  ProxyMethodCombo->Visible = (Tab == ProxyMethodCombo->Group);
  TFarComboBox * OtherProxyMethodCombo = (!SshProtocol ? SshProxyMethodCombo : FtpProxyMethodCombo);
  OtherProxyMethodCombo->Visible = false;
  if (ProxyMethodCombo->Items->Selected >= OtherProxyMethodCombo->Items->Count)
  {
    OtherProxyMethodCombo->Items->Selected = pmNone;
  }
  else
  {
    OtherProxyMethodCombo->Items->Selected = ProxyMethodCombo->Items->Selected;
  }

  bool Proxy = (ProxyMethodCombo->Items->Selected != pmNone);
  AnsiString ProxyCommand =
    ((ProxyMethodCombo->Items->Selected == pmCmd) ?
      ProxyLocalCommandEdit->Text : ProxyTelnetCommandEdit->Text);
  ProxyHostEdit->Enabled = Proxy &&
    ((ProxyMethodCombo->Items->Selected != pmCmd) ||
     AnsiContainsText(ProxyCommand, "%proxyhost"));
  ProxyPortEdit->Enabled = Proxy &&
    ((ProxyMethodCombo->Items->Selected != pmCmd) ||
     AnsiContainsText(ProxyCommand, "%proxyport"));
  ProxyUsernameEdit->Enabled = Proxy &&
    // FZAPI does not support username for SOCKS4
    (((ProxyMethodCombo->Items->Selected == pmSocks4) && SshProtocol) ||
     (ProxyMethodCombo->Items->Selected == pmSocks5) ||
     (ProxyMethodCombo->Items->Selected == pmHTTP) ||
     (((ProxyMethodCombo->Items->Selected == pmTelnet) ||
       (ProxyMethodCombo->Items->Selected == pmCmd)) &&
      AnsiContainsText(ProxyCommand, "%user")));
  ProxyPasswordEdit->Enabled = Proxy &&
    ((ProxyMethodCombo->Items->Selected == pmSocks5) ||
     (ProxyMethodCombo->Items->Selected == pmHTTP) ||
     (((ProxyMethodCombo->Items->Selected == pmTelnet) ||
       (ProxyMethodCombo->Items->Selected == pmCmd)) &&
      AnsiContainsText(ProxyCommand, "%pass")));
  bool ProxySettings = Proxy && SshProtocol;
  ProxyTelnetCommandEdit->Enabled = ProxySettings && (ProxyMethodCombo->Items->Selected == pmTelnet);
  ProxyLocalCommandEdit->Visible = (Tab == ProxyMethodCombo->Group) && (ProxyMethodCombo->Items->Selected == pmCmd);
  ProxyLocalCommandLabel->Visible = ProxyLocalCommandEdit->Visible;
  ProxyTelnetCommandEdit->Visible = (Tab == ProxyMethodCombo->Group) && (ProxyMethodCombo->Items->Selected != pmCmd);
  ProxyTelnetCommandLabel->Visible = ProxyTelnetCommandEdit->Visible;
  ProxyLocalhostCheck->Enabled = ProxySettings;
  ProxyDNSOffButton->Enabled = ProxySettings;

  // Tunnel tab
  TunnelTab->Enabled = InternalSshProtocol;
}
//---------------------------------------------------------------------------
bool __fastcall TSessionDialog::Execute(TSessionData * SessionData, TSessionAction & Action)
{
  int Captions[] = { LOGIN_ADD, LOGIN_EDIT, LOGIN_CONNECT };
  Caption = GetMsg(Captions[Action]);

  FSessionData = SessionData;
  FTransferProtocolIndex = TransferProtocolCombo->Items->Selected;

  HideTabs();
  SelectTab(tabSession);

  // load session data

  // Basic tab
  HostNameEdit->Text = SessionData->HostName;
  PortNumberEdit->AsInteger = SessionData->PortNumber;
  UserNameEdit->Text = SessionData->UserName;
  PasswordEdit->Text = SessionData->Password;
  PrivateKeyEdit->Text = SessionData->PublicKeyFile;

  bool AllowScpFallback;
  TransferProtocolCombo->Items->Selected =
    FSProtocolToIndex(SessionData->FSProtocol, AllowScpFallback);
  AllowScpFallbackCheck->Checked = AllowScpFallback;

  // Directories tab
  RemoteDirectoryEdit->Text = SessionData->RemoteDirectory;
  UpdateDirectoriesCheck->Checked = SessionData->UpdateDirectories;
  CacheDirectoriesCheck->Checked = SessionData->CacheDirectories;
  CacheDirectoryChangesCheck->Checked = SessionData->CacheDirectoryChanges;
  PreserveDirectoryChangesCheck->Checked = SessionData->PreserveDirectoryChanges;
  ResolveSymlinksCheck->Checked = SessionData->ResolveSymlinks;

  // Environment tab
  if (SessionData->EOLType == eolLF)
  {
    EOLTypeCombo->Items->Selected = 0;
  }
  else
  {
    EOLTypeCombo->Items->Selected = 1;
  }
  switch (SessionData->Utf)
  {
    case asOn:
      UtfCombo->Items->Selected = 1;
      break;

    case asOff:
      UtfCombo->Items->Selected = 2;
      break;

    default:
      UtfCombo->Items->Selected = 0;
      break;
  }

  switch (SessionData->DSTMode)
  {
    case dstmWin:
      DSTModeWinCheck->Checked = true;
      break;

    case dstmKeep:
      DSTModeKeepCheck->Checked = true;
      break;

    default:
    case dstmUnix:
      DSTModeUnixCheck->Checked = true;
      break;
  }

  DeleteToRecycleBinCheck->Checked = SessionData->DeleteToRecycleBin;
  OverwrittenToRecycleBinCheck->Checked = SessionData->OverwrittenToRecycleBin;
  RecycleBinPathEdit->Text = SessionData->RecycleBinPath;

  // Shell tab
  if (SessionData->DefaultShell)
  {
    ShellEdit->Text = ShellEdit->Items->Strings[0];
  }
  else
  {
    ShellEdit->Text = SessionData->Shell;
  }
  if (SessionData->DetectReturnVar)
  {
    ReturnVarEdit->Text = ReturnVarEdit->Items->Strings[0];
  }
  else
  {
    ReturnVarEdit->Text = SessionData->ReturnVar;
  }
  LookupUserGroupsCheck->Checked = SessionData->LookupUserGroups;
  ClearAliasesCheck->Checked = SessionData->ClearAliases;
  IgnoreLsWarningsCheck->Checked = SessionData->IgnoreLsWarnings;
  Scp1CompatibilityCheck->Checked = SessionData->Scp1Compatibility;
  UnsetNationalVarsCheck->Checked = SessionData->UnsetNationalVars;
  ListingCommandEdit->Text = SessionData->ListingCommand;
  SCPLsFullTimeAutoCheck->Checked = (SessionData->SCPLsFullTime != asOff);
  int TimeDifferenceMin = DateTimeToTimeStamp(SessionData->TimeDifference).Time / 60000;
  if (double(SessionData->TimeDifference) < 0)
  {
    TimeDifferenceMin = -TimeDifferenceMin;
  }
  TimeDifferenceEdit->AsInteger = TimeDifferenceMin / 60;
  TimeDifferenceMinutesEdit->AsInteger = TimeDifferenceMin % 60;

  // SFTP tab

  #define TRISTATE(COMBO, PROP, MSG) \
    COMBO->Items->Selected = 2 - SessionData->PROP
  SFTP_BUGS();

  if (SessionData->SftpServer.IsEmpty())
  {
    SftpServerEdit->Text = SftpServerEdit->Items->Strings[0];
  }
  else
  {
    SftpServerEdit->Text = SessionData->SftpServer;
  }
  SFTPMaxVersionCombo->Items->Selected = SessionData->SFTPMaxVersion;

  // FTP tab
  TStrings * PostLoginCommands = new TStringList();
  try
  {
    PostLoginCommands->Text = SessionData->PostLoginCommands;
    for (int Index = 0; (Index < PostLoginCommands->Count) &&
                        (Index < LENOF(PostLoginCommandsEdits)); Index++)
    {
      PostLoginCommandsEdits[Index]->Text = PostLoginCommands->Strings[Index];
    }
  }
  __finally
  {
    delete PostLoginCommands;
  }

  // Connection tab
  FtpPasvModeCheck->Checked = SessionData->FtpPasvMode;
  LoadPing(SessionData);
  TimeoutEdit->AsInteger = SessionData->Timeout;

  switch (SessionData->AddressFamily)
  {
    case afIPv4:
      IPv4Button->Checked = true;
      break;

    case afIPv6:
      IPv6Button->Checked = true;
      break;

    case afAuto:
    default:
      IPAutoButton->Checked = true;
      break;
  }

  // Proxy tab
  SshProxyMethodCombo->Items->Selected = SessionData->ProxyMethod;
  if (SessionData->ProxyMethod >= FtpProxyMethodCombo->Items->Count)
  {
    FtpProxyMethodCombo->Items->Selected = pmNone;
  }
  else
  {
    FtpProxyMethodCombo->Items->Selected = SessionData->ProxyMethod;
  }
  ProxyHostEdit->Text = SessionData->ProxyHost;
  ProxyPortEdit->AsInteger = SessionData->ProxyPort;
  ProxyUsernameEdit->Text = SessionData->ProxyUsername;
  ProxyPasswordEdit->Text = SessionData->ProxyPassword;
  ProxyTelnetCommandEdit->Text = SessionData->ProxyTelnetCommand;
  ProxyLocalCommandEdit->Text = SessionData->ProxyLocalCommand;
  ProxyLocalhostCheck->Checked = SessionData->ProxyLocalhost;
  switch (SessionData->ProxyDNS) {
    case asOn: ProxyDNSOnButton->Checked = true; break;
    case asOff: ProxyDNSOffButton->Checked = true; break;
    default: ProxyDNSAutoButton->Checked = true; break;
  }

  // Tunnel tab
  TunnelCheck->Checked = SessionData->Tunnel;
  TunnelUserNameEdit->Text = SessionData->TunnelUserName;
  TunnelPortNumberEdit->AsInteger = SessionData->TunnelPortNumber;
  TunnelHostNameEdit->Text = SessionData->TunnelHostName;
  TunnelPasswordEdit->Text = SessionData->TunnelPassword;
  TunnelPrivateKeyEdit->Text = SessionData->TunnelPublicKeyFile;
  if (SessionData->TunnelAutoassignLocalPortNumber)
  {
    TunnelLocalPortNumberEdit->Text = TunnelLocalPortNumberEdit->Items->Strings[0];
  }
  else
  {
    TunnelLocalPortNumberEdit->Text = IntToStr(SessionData->TunnelLocalPortNumber);
  }

  // SSH tab
  CompressionCheck->Checked = SessionData->Compression;
  if (Ssh2DESCheck != NULL)
  {
    Ssh2DESCheck->Checked = SessionData->Ssh2DES;
  }

  switch (SessionData->SshProt) {
    case ssh1only:  SshProt1onlyButton->Checked = true; break;
    case ssh1:      SshProt1Button->Checked = true; break;
    case ssh2:      SshProt2Button->Checked = true; break;
    case ssh2only:  SshProt2onlyButton->Checked = true; break;
  }

  CipherListBox->Items->BeginUpdate();
  try
  {
    CipherListBox->Items->Clear();
    assert(CIPHER_NAME_WARN+CIPHER_COUNT-1 == CIPHER_NAME_ARCFOUR);
    for (int Index = 0; Index < CIPHER_COUNT; Index++)
    {
      CipherListBox->Items->AddObject(
        GetMsg(CIPHER_NAME_WARN + int(SessionData->Cipher[Index])),
        (TObject*)SessionData->Cipher[Index]);
    }
  }
  __finally
  {
    CipherListBox->Items->EndUpdate();
  }

  // KEX tab

  RekeyTimeEdit->AsInteger = SessionData->RekeyTime;
  RekeyDataEdit->Text = SessionData->RekeyData;

  KexListBox->Items->BeginUpdate();
  try
  {
    KexListBox->Items->Clear();
    assert(KEX_NAME_WARN+KEX_COUNT-1 == KEX_NAME_GSSGEX);
    for (int Index = 0; Index < KEX_COUNT; Index++)
    {
      KexListBox->Items->AddObject(
        GetMsg(KEX_NAME_WARN + int(SessionData->Kex[Index])),
        (TObject*)SessionData->Kex[Index]);
    }
  }
  __finally
  {
    KexListBox->Items->EndUpdate();
  }

  // Authentication tab
  SshNoUserAuthCheck->Checked = SessionData->SshNoUserAuth;
  TryAgentCheck->Checked = SessionData->TryAgent;
  AuthTISCheck->Checked = SessionData->AuthTIS;
  AuthKICheck->Checked = SessionData->AuthKI;
  AuthKIPasswordCheck->Checked = SessionData->AuthKIPassword;
  AuthGSSAPICheck2->Checked = SessionData->AuthGSSAPI;
  AgentFwdCheck->Checked = SessionData->AgentFwd;
  GSSAPIServerRealmEdit->Text = SessionData->GSSAPIServerRealm;

  // Bugs tab

  BUGS();
  #undef TRISTATE

  int Button = ShowModal();
  bool Result = (Button == brOK || Button == brConnect);
  if (Result)
  {
    if (Button == brConnect)
    {
      Action = saConnect;
    }
    else if (Action == saConnect)
    {
      Action = saEdit;
    }

    // save session data

    // Basic tab
    SessionData->HostName = HostNameEdit->Text;
    SessionData->PortNumber = PortNumberEdit->AsInteger;
    SessionData->UserName = UserNameEdit->Text;
    SessionData->Password = PasswordEdit->Text;
    SessionData->PublicKeyFile = PrivateKeyEdit->Text;

    SessionData->FSProtocol = GetFSProtocol();

    // Directories tab
    SessionData->RemoteDirectory = RemoteDirectoryEdit->Text;
    SessionData->UpdateDirectories = UpdateDirectoriesCheck->Checked;
    SessionData->CacheDirectories = CacheDirectoriesCheck->Checked;
    SessionData->CacheDirectoryChanges = CacheDirectoryChangesCheck->Checked;
    SessionData->PreserveDirectoryChanges = PreserveDirectoryChangesCheck->Checked;
    SessionData->ResolveSymlinks = ResolveSymlinksCheck->Checked;

    // Environment tab
    if (DSTModeUnixCheck->Checked) SessionData->DSTMode = dstmUnix;
      else
    if (DSTModeKeepCheck->Checked) SessionData->DSTMode = dstmKeep;
      else SessionData->DSTMode = dstmWin;
    if (EOLTypeCombo->Items->Selected == 0) SessionData->EOLType = eolLF;
      else SessionData->EOLType = eolCRLF;
    switch (UtfCombo->Items->Selected)
    {
      case 1:
        SessionData->Utf = asOn;
        break;

      case 2:
        SessionData->Utf = asOff;
        break;

      default:
        SessionData->Utf = asAuto;
        break;
    }

    SessionData->DeleteToRecycleBin = DeleteToRecycleBinCheck->Checked;
    SessionData->OverwrittenToRecycleBin = OverwrittenToRecycleBinCheck->Checked;
    SessionData->RecycleBinPath = RecycleBinPathEdit->Text;

    // SCP tab
    SessionData->DefaultShell = (ShellEdit->Text == ShellEdit->Items->Strings[0]);
    SessionData->Shell = (SessionData->DefaultShell ? AnsiString() : ShellEdit->Text);
    SessionData->DetectReturnVar = (ReturnVarEdit->Text == ReturnVarEdit->Items->Strings[0]);
    SessionData->ReturnVar = (SessionData->DetectReturnVar ? AnsiString() : ReturnVarEdit->Text);
    SessionData->LookupUserGroups = LookupUserGroupsCheck->Checked;
    SessionData->ClearAliases = ClearAliasesCheck->Checked;
    SessionData->IgnoreLsWarnings = IgnoreLsWarningsCheck->Checked;
    SessionData->Scp1Compatibility = Scp1CompatibilityCheck->Checked;
    SessionData->UnsetNationalVars = UnsetNationalVarsCheck->Checked;
    SessionData->ListingCommand = ListingCommandEdit->Text;
    SessionData->SCPLsFullTime = SCPLsFullTimeAutoCheck->Checked ? asAuto : asOff;
    SessionData->TimeDifference =
      (double(TimeDifferenceEdit->AsInteger) / 24) +
      (double(TimeDifferenceMinutesEdit->AsInteger) / 24 / 60);

    // SFTP tab

    #define TRISTATE(COMBO, PROP, MSG) \
      SessionData->PROP = (TAutoSwitch)(2 - COMBO->Items->Selected);
    SFTP_BUGS();

    SessionData->SftpServer =
      ((SftpServerEdit->Text == SftpServerEdit->Items->Strings[0]) ?
        AnsiString() : SftpServerEdit->Text);
    SessionData->SFTPMaxVersion = SFTPMaxVersionCombo->Items->Selected;

    // FTP tab
    TStrings * PostLoginCommands = new TStringList;
    try
    {
      for (int Index = 0; Index < LENOF(PostLoginCommandsEdits); Index++)
      {
        AnsiString Text = PostLoginCommandsEdits[Index]->Text;
        if (!Text.IsEmpty())
        {
          PostLoginCommands->Add(PostLoginCommandsEdits[Index]->Text);
        }
      }

      SessionData->PostLoginCommands = PostLoginCommands->Text;
    }
    __finally
    {
      delete PostLoginCommands;
    }

    // Connection tab
    SessionData->FtpPasvMode = FtpPasvModeCheck->Checked;
    if (PingNullPacketButton->Checked)
    {
      SessionData->PingType = ptNullPacket;
    }
    else if (PingDummyCommandButton->Checked)
    {
      SessionData->PingType = ptDummyCommand;
    }
    else
    {
      SessionData->PingType = ptOff;
    }
    SessionData->PingInterval = PingIntervalSecEdit->AsInteger;
    SessionData->Timeout = TimeoutEdit->AsInteger;

    if (IPv4Button->Checked)
    {
      SessionData->AddressFamily = afIPv4;
    }
    else if (IPv6Button->Checked)
    {
      SessionData->AddressFamily = afIPv6;
    }
    else
    {
      SessionData->AddressFamily = afAuto;
    }

    // Proxy tab
    SessionData->ProxyMethod = (TProxyMethod)SshProxyMethodCombo->Items->Selected;
    SessionData->ProxyHost = ProxyHostEdit->Text;
    SessionData->ProxyPort = ProxyPortEdit->AsInteger;
    SessionData->ProxyUsername = ProxyUsernameEdit->Text;
    SessionData->ProxyPassword = ProxyPasswordEdit->Text;
    SessionData->ProxyTelnetCommand = ProxyTelnetCommandEdit->Text;
    SessionData->ProxyLocalCommand = ProxyLocalCommandEdit->Text;
    SessionData->ProxyLocalhost = ProxyLocalhostCheck->Checked;

    if (ProxyDNSOnButton->Checked) SessionData->ProxyDNS = asOn;
      else
    if (ProxyDNSOffButton->Checked) SessionData->ProxyDNS = asOff;
      else SessionData->ProxyDNS = asAuto;

    // Tunnel tab
    SessionData->Tunnel = TunnelCheck->Checked;
    SessionData->TunnelUserName = TunnelUserNameEdit->Text;
    SessionData->TunnelPortNumber = TunnelPortNumberEdit->AsInteger;
    SessionData->TunnelHostName = TunnelHostNameEdit->Text;
    SessionData->TunnelPassword = TunnelPasswordEdit->Text;
    SessionData->TunnelPublicKeyFile = TunnelPrivateKeyEdit->Text;
    if (TunnelLocalPortNumberEdit->Text == TunnelLocalPortNumberEdit->Items->Strings[0])
    {
      SessionData->TunnelLocalPortNumber = 0;
    }
    else
    {
      SessionData->TunnelLocalPortNumber = StrToIntDef(TunnelLocalPortNumberEdit->Text, 0);
    }

    // SSH tab
    SessionData->Compression = CompressionCheck->Checked;
    if (Ssh2DESCheck != NULL)
    {
      SessionData->Ssh2DES = Ssh2DESCheck->Checked;
    }

    if (SshProt1onlyButton->Checked) SessionData->SshProt = ssh1only;
      else
    if (SshProt1Button->Checked) SessionData->SshProt = ssh1;
      else
    if (SshProt2Button->Checked) SessionData->SshProt = ssh2;
      else SessionData->SshProt = ssh2only;

    for (int Index = 0; Index < CIPHER_COUNT; Index++)
    {
      SessionData->Cipher[Index] = (TCipher)CipherListBox->Items->Objects[Index];
    }

    // KEX tab

    SessionData->RekeyTime = RekeyTimeEdit->AsInteger;
    SessionData->RekeyData = RekeyDataEdit->Text;

    for (int Index = 0; Index < KEX_COUNT; Index++)
    {
      SessionData->Kex[Index] = (TKex)KexListBox->Items->Objects[Index];
    }

    // Authentication tab
    SessionData->SshNoUserAuth = SshNoUserAuthCheck->Checked;
    SessionData->TryAgent = TryAgentCheck->Checked;
    SessionData->AuthTIS = AuthTISCheck->Checked;
    SessionData->AuthKI = AuthKICheck->Checked;
    SessionData->AuthKIPassword = AuthKIPasswordCheck->Checked;
    SessionData->AuthGSSAPI = AuthGSSAPICheck2->Checked;
    SessionData->AgentFwd = AgentFwdCheck->Checked;
    SessionData->GSSAPIServerRealm = GSSAPIServerRealmEdit->Text;

    // Bugs tab

    BUGS();
    #undef TRISTATE
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::LoadPing(TSessionData * SessionData)
{
  TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
    AllowScpFallbackCheck->Checked);

  switch (FSProtocol == fsFTP ? SessionData->FtpPingType : SessionData->PingType)
  {
    case ptNullPacket:
      PingNullPacketButton->Checked = true;
      break;

    case ptDummyCommand:
      PingDummyCommandButton->Checked = true;
      break;

    default:
      PingOffButton->Checked = true;
      break;
  }
  PingIntervalSecEdit->AsInteger =
    (FSProtocol == fsFTP ? SessionData->FtpPingInterval : SessionData->PingInterval);
}
//---------------------------------------------------------------------
void __fastcall TSessionDialog::SavePing(TSessionData * SessionData)
{
  TPingType PingType;
  if (PingNullPacketButton->Checked)
  {
    PingType = ptNullPacket;
  }
  else if (PingDummyCommandButton->Checked)
  {
    PingType = ptDummyCommand;
  }
  else
  {
    PingType = ptOff;
  }
  TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
    AllowScpFallbackCheck->Checked);
  (FSProtocol == fsFTP ? SessionData->FtpPingType : SessionData->PingType) =
    PingType;
  (FSProtocol == fsFTP ? SessionData->FtpPingInterval : SessionData->PingInterval) =
    PingIntervalSecEdit->AsInteger;
}
//---------------------------------------------------------------------------
int __fastcall TSessionDialog::FSProtocolToIndex(TFSProtocol FSProtocol,
  bool & AllowScpFallback)
{
  if (FSProtocol == fsSFTP)
  {
    AllowScpFallback = true;
    bool Dummy;
    return FSProtocolToIndex(fsSFTPonly, Dummy);
  }
  else
  {
    AllowScpFallback = false;
    for (int Index = 0; Index < TransferProtocolCombo->Items->Count; Index++)
    {
      if (FSOrder[Index] == FSProtocol)
      {
        return Index;
      }
    }
    // SFTP is always present
    return FSProtocolToIndex(fsSFTP, AllowScpFallback);
  }
}
//---------------------------------------------------------------------------
TFSProtocol __fastcall TSessionDialog::GetFSProtocol()
{
  return IndexToFSProtocol(TransferProtocolCombo->Items->Selected,
    AllowScpFallbackCheck->Checked);
}
//---------------------------------------------------------------------------
TFSProtocol __fastcall TSessionDialog::IndexToFSProtocol(int Index, bool AllowScpFallback)
{
  bool InBounds = (Index >= 0) && (Index < LENOF(FSOrder));
  assert(InBounds);
  TFSProtocol Result = fsSFTP;
  if (InBounds)
  {
    Result = FSOrder[Index];
    if ((Result == fsSFTPonly) && AllowScpFallback)
    {
      Result = fsSFTP;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TSessionDialog::VerifyKey(AnsiString FileName, bool TypeOnly)
{
  bool Result = true;

  if (!FileName.Trim().IsEmpty())
  {
    TKeyType Type = KeyType(FileName);
    AnsiString Message;
    switch (Type)
    {
      case ktOpenSSH:
        Message = FMTLOAD(KEY_TYPE_UNSUPPORTED, (FileName, "OpenSSH SSH-2"));
        break;

      case ktSSHCom:
        Message = FMTLOAD(KEY_TYPE_UNSUPPORTED, (FileName, "ssh.com SSH-2"));
        break;

      case ktSSH1:
      case ktSSH2:
        if (!TypeOnly)
        {
          if ((Type == ktSSH1) !=
                (SshProt1onlyButton->Checked || SshProt1Button->Checked))
          {
            Message = FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
              (FileName, (Type == ktSSH1 ? "SSH-1" : "PuTTY SSH-2")));
          }
        }
        break;

      default:
        assert(false);
        // fallthru
      case ktUnopenable:
      case ktUnknown:
        Message = FMTLOAD(KEY_TYPE_UNKNOWN, (FileName));
        break;
    }

    if (!Message.IsEmpty())
    {
      TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
      Result = (WinSCPPlugin->MoreMessageDialog(Message, NULL, qtWarning,
        qaIgnore | qaAbort) != qaAbort);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TSessionDialog::CloseQuery()
{
  bool CanClose = TTabbedDialog::CloseQuery();

  if (CanClose && (Result != brCancel))
  {
    CanClose =
      VerifyKey(PrivateKeyEdit->Text, false) &&
      // for tunnel key do not check SSH version as it is not configurable
      VerifyKey(TunnelPrivateKeyEdit->Text, true);
  }

  if (CanClose && !PasswordEdit->Text.IsEmpty() &&
      !Configuration->DisablePasswordStoring &&
      (PasswordEdit->Text != FSessionData->Password) &&
      (((Result == brOK)) ||
       ((Result == brConnect) && (FAction == saEdit))))
  {
    TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
    CanClose = (WinSCPPlugin->MoreMessageDialog(GetMsg(SAVE_PASSWORD), NULL,
      qtWarning, qaOK | qaCancel) == qaOK);
  }

  return CanClose;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::CipherButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->Enabled)
  {
    int Source = CipherListBox->Items->Selected;
    int Dest = Source + Sender->Result;

    CipherListBox->Items->Move(Source, Dest);
    CipherListBox->Items->Selected = Dest;
  }

  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::KexButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->Enabled)
  {
    int Source = KexListBox->Items->Selected;
    int Dest = Source + Sender->Result;

    KexListBox->Items->Move(Source, Dest);
    KexListBox->Items->Selected = Dest;
  }

  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::AuthGSSAPICheckAllowChange(TFarDialogItem * /*Sender*/,
  long NewState, bool & Allow)
{
  if ((NewState == BSTATE_CHECKED) && !Configuration->GSSAPIInstalled)
  {
    Allow = false;
    TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);

    WinSCPPlugin->MoreMessageDialog(GetMsg(GSSAPI_NOT_INSTALLED),
      NULL, qtError, qaOK);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::UnixEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->Items->Selected = 0;
  DSTModeUnixCheck->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TSessionDialog::WindowsEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->Items->Selected = 1;
  DSTModeWinCheck->Checked = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::SessionDialog(TSessionData * SessionData,
  TSessionAction & Action)
{
  bool Result;
  TSessionDialog * Dialog = new TSessionDialog(FPlugin, Action);
  try
  {
    Result = Dialog->Execute(SessionData, Action);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TRightsContainer : public TFarDialogContainer
{
public:
  __fastcall TRightsContainer(TFarDialog * ADialog, bool AAnyDirectories,
    bool ShowButtons, bool ShowSpecials,
    TFarDialogItem * EnabledDependency);
  __property bool AddXToDirectories = { read = GetAddXToDirectories, write = SetAddXToDirectories };
  __property TRights Rights = { read = GetRights, write = SetRights };
  __property TFarCheckBox * Checks[TRights::TRight Right] = { read = GetChecks };
  __property TRights::TState States[TRights::TRight Right] = { read = GetStates, write = SetStates };
  __property bool AllowUndef = { read = GetAllowUndef, write = SetAllowUndef };

protected:
  bool FAnyDirectories;
  TFarCheckBox * FCheckBoxes[12];
  TRights::TState FFixedStates[12];
  TFarEdit * OctalEdit;
  TFarCheckBox * DirectoriesXCheck;

  virtual void __fastcall Change();
  void __fastcall UpdateControls();

private:
  TRights __fastcall GetRights();
  void __fastcall SetRights(const TRights & value);
  void __fastcall SetAddXToDirectories(bool value);
  bool __fastcall GetAddXToDirectories();
  TFarCheckBox * __fastcall GetChecks(TRights::TRight Right);
  TRights::TState __fastcall GetStates(TRights::TRight Right);
  bool __fastcall GetAllowUndef();
  void __fastcall SetAllowUndef(bool value);
  void __fastcall SetStates(TRights::TRight Flag, TRights::TState value);
  void __fastcall OctalEditExit(TObject * Sender);
  void __fastcall RightsButtonClick(TFarButton * Sender, bool & Close);
};
//---------------------------------------------------------------------------
__fastcall TRightsContainer::TRightsContainer(TFarDialog * ADialog,
  bool AAnyDirectories, bool ShowButtons,
  bool ShowSpecials, TFarDialogItem * EnabledDependency) :
  TFarDialogContainer(ADialog)
{
  FAnyDirectories = AAnyDirectories;

  TFarButton * Button;
  TFarText * Text;
  TFarCheckBox * CheckBox;

  Dialog->NextItemPosition = ipNewLine;

  static int RowLabels[] = { PROPERTIES_OWNER_RIGHTS, PROPERTIES_GROUP_RIGHTS,
    PROPERTIES_OTHERS_RIGHTS };
  static int ColLabels[] = { PROPERTIES_READ_RIGHTS, PROPERTIES_WRITE_RIGHTS,
    PROPERTIES_EXECUTE_RIGHTS };
  static int SpecialLabels[] = { PROPERTIES_SETUID_RIGHTS, PROPERTIES_SETGID_RIGHTS,
    PROPERTIES_STICKY_BIT_RIGHTS };

  for (int RowIndex = 0; RowIndex < 3; RowIndex++)
  {
    Dialog->NextItemPosition = ipNewLine;
    Text = new TFarText(Dialog);
    if (RowIndex == 0)
    {
      Text->Top = 0;
    }
    Text->Left = 0;
    Add(Text);
    Text->EnabledDependency = EnabledDependency;
    Text->Caption = GetMsg(RowLabels[RowIndex]);

    Dialog->NextItemPosition = ipRight;

    for (int ColIndex = 0; ColIndex < 3; ColIndex++)
    {
      CheckBox = new TFarCheckBox(Dialog);
      FCheckBoxes[(RowIndex + 1)* 3 + ColIndex] = CheckBox;
      Add(CheckBox);
      CheckBox->EnabledDependency = EnabledDependency;
      CheckBox->Caption = GetMsg(ColLabels[ColIndex]);
    }

    if (ShowSpecials)
    {
      CheckBox = new TFarCheckBox(Dialog);
      Add(CheckBox);
      CheckBox->Visible = ShowSpecials;
      CheckBox->EnabledDependency = EnabledDependency;
      CheckBox->Caption = GetMsg(SpecialLabels[RowIndex]);
      FCheckBoxes[RowIndex] = CheckBox;
    }
    else
    {
      FCheckBoxes[RowIndex] = NULL;
      FFixedStates[RowIndex] = TRights::rsNo;
    }
  }

  Dialog->NextItemPosition = ipNewLine;

  Text = new TFarText(Dialog);
  Add(Text);
  Text->EnabledDependency = EnabledDependency;
  Text->Left = 0;
  Text->Caption = GetMsg(PROPERTIES_OCTAL);

  Dialog->NextItemPosition = ipRight;

  OctalEdit = new TFarEdit(Dialog);
  Add(OctalEdit);
  OctalEdit->EnabledDependency = EnabledDependency;
  OctalEdit->Width = 5;
  OctalEdit->Mask = "9999";
  OctalEdit->OnExit = OctalEditExit;

  if (ShowButtons)
  {
    Dialog->NextItemPosition = ipRight;

    Button = new TFarButton(Dialog);
    Add(Button);
    Button->EnabledDependency = EnabledDependency;
    Button->Caption = GetMsg(PROPERTIES_NONE_RIGHTS);
    Button->Tag = TRights::rfNo;
    Button->OnClick = RightsButtonClick;

    Button = new TFarButton(Dialog);
    Add(Button);
    Button->EnabledDependency = EnabledDependency;
    Button->Caption = GetMsg(PROPERTIES_DEFAULT_RIGHTS);
    Button->Tag = TRights::rfDefault;
    Button->OnClick = RightsButtonClick;

    Button = new TFarButton(Dialog);
    Add(Button);
    Button->EnabledDependency = EnabledDependency;
    Button->Caption = GetMsg(PROPERTIES_ALL_RIGHTS);
    Button->Tag = TRights::rfAll;
    Button->OnClick = RightsButtonClick;
  }

  Dialog->NextItemPosition = ipNewLine;

  if (FAnyDirectories)
  {
    DirectoriesXCheck = new TFarCheckBox(Dialog);
    Add(DirectoriesXCheck);
    DirectoriesXCheck->EnabledDependency = EnabledDependency;
    DirectoriesXCheck->Left = 0;
    DirectoriesXCheck->Caption = GetMsg(PROPERTIES_DIRECTORIES_X);
  }
  else
  {
    DirectoriesXCheck = NULL;
  }
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::RightsButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  TRights R = Rights;
  R.Number = (unsigned short)Sender->Tag;
  Rights = R;
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::OctalEditExit(TObject * /*Sender*/)
{
  if (!OctalEdit->Text.Trim().IsEmpty())
  {
    TRights R = Rights;
    R.Octal = OctalEdit->Text.Trim();
    Rights = R;
  }
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::UpdateControls()
{
  if (Dialog->Handle)
  {
    TRights R = Rights;

    if (DirectoriesXCheck)
    {
      DirectoriesXCheck->Enabled =
        !((R.NumberSet & TRights::rfExec) == TRights::rfExec);
    }

    if (!OctalEdit->Focused())
    {
      OctalEdit->Text = R.IsUndef ? AnsiString() : R.Octal;
    }
    else if (OctalEdit->Text.Trim().Length() >= 3)
    {
      try
      {
        OctalEditExit(NULL);
      }
      catch(...)
      {
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::Change()
{
  TFarDialogContainer::Change();

  if (Dialog->Handle)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
TFarCheckBox * __fastcall TRightsContainer::GetChecks(TRights::TRight Right)
{
  assert((Right >= 0) && (Right < LENOF(FCheckBoxes)));
  return FCheckBoxes[Right];
}
//---------------------------------------------------------------------------
TRights::TState __fastcall TRightsContainer::GetStates(TRights::TRight Right)
{
  TFarCheckBox * CheckBox = Checks[Right];
  if (CheckBox != NULL)
  {
    switch (CheckBox->Selected) {
      case BSTATE_UNCHECKED: return TRights::rsNo;
      case BSTATE_CHECKED: return TRights::rsYes;
      case BSTATE_3STATE:
      default: return TRights::rsUndef;
    }
  }
  else
  {
    return FFixedStates[Right];
  }
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::SetStates(TRights::TRight Right,
  TRights::TState value)
{
  TFarCheckBox * CheckBox = Checks[Right];
  if (CheckBox != NULL)
  {
    switch (value) {
      case TRights::rsNo: CheckBox->Selected = BSTATE_UNCHECKED; break;
      case TRights::rsYes: CheckBox->Selected = BSTATE_CHECKED; break;
      case TRights::rsUndef: CheckBox->Selected = BSTATE_3STATE; break;
    }
  }
  else
  {
    FFixedStates[Right]= value;
  }
}
//---------------------------------------------------------------------------
TRights __fastcall TRightsContainer::GetRights()
{
  TRights Result;
  Result.AllowUndef = AllowUndef;
  for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
  {
    Result.RightUndef[static_cast<TRights::TRight>(Right)] =
      States[static_cast<TRights::TRight>(Right)];
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::SetRights(const TRights & value)
{
  if (Rights != value)
  {
    Dialog->LockChanges();
    try
    {
      AllowUndef = true; // temporarily
      for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
      {
        States[static_cast<TRights::TRight>(Right)] =
          value.RightUndef[static_cast<TRights::TRight>(Right)];
      }
      AllowUndef = value.AllowUndef;
    }
    __finally
    {
      Dialog->UnlockChanges();
    }
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRightsContainer::GetAddXToDirectories()
{
  return DirectoriesXCheck ? DirectoriesXCheck->Checked : false;
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::SetAddXToDirectories(bool value)
{
  if (DirectoriesXCheck)
  {
    DirectoriesXCheck->Checked = value;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TRightsContainer::GetAllowUndef()
{
  assert(FCheckBoxes[LENOF(FCheckBoxes) - 1] != NULL);
  return FCheckBoxes[LENOF(FCheckBoxes) - 1]->AllowGrayed;
}
//---------------------------------------------------------------------------
void __fastcall TRightsContainer::SetAllowUndef(bool value)
{
  for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
  {
    if (FCheckBoxes[Right] != NULL)
    {
      FCheckBoxes[Right]->AllowGrayed = value;
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TPropertiesDialog : public TFarDialog
{
public:
  __fastcall TPropertiesDialog(TCustomFarPlugin * AFarPlugin, TStrings * FileList,
    const AnsiString Directory, TStrings * GroupList, TStrings * UserList,
    int AllowedChanges);

  bool __fastcall Execute(TRemoteProperties * Properties);

protected:
  virtual void __fastcall Change();
  void __fastcall UpdateProperties(TRemoteProperties & Properties);

private:
  bool FAnyDirectories;
  int FAllowedChanges;
  TRemoteProperties FOrigProperties;
  bool FMultiple;

  TRightsContainer * RightsContainer;
  TFarComboBox * OwnerComboBox;
  TFarComboBox * GroupComboBox;
  TFarCheckBox * RecursiveCheck;
  TFarButton * OkButton;
};
//---------------------------------------------------------------------------
__fastcall TPropertiesDialog::TPropertiesDialog(TCustomFarPlugin * AFarPlugin,
  TStrings * FileList, const AnsiString Directory, TStrings * GroupList,
  TStrings * UserList, int AAllowedChanges) : TFarDialog(AFarPlugin)
{
  FAllowedChanges = AAllowedChanges;

  assert(FileList->Count > 0);
  TRemoteFile * OnlyFile = dynamic_cast<TRemoteFile *>(FileList->Objects[0]);
  USEDPARAM(OnlyFile);
  assert(OnlyFile);
  FMultiple = (FileList->Count > 1);
  TRemoteFile * File;
  int Directories = 0;

  TStringList * UsedGroupList = NULL;
  TStringList * UsedUserList = NULL;

  try
  {
    if ((GroupList == NULL) || (GroupList->Count == 0))
    {
      UsedGroupList = new TStringList();
      UsedGroupList->Duplicates = dupIgnore;
      UsedGroupList->Sorted = true;
    }
    if ((UserList == NULL) || (UserList->Count == 0))
    {
      UsedUserList = new TStringList();
      UsedUserList->Duplicates = dupIgnore;
      UsedUserList->Sorted = true;
    }

    for (int Index = 0; Index < FileList->Count; Index++)
    {
      File = dynamic_cast<TRemoteFile *>(FileList->Objects[Index]);
      assert(File);
      if (UsedGroupList && !File->Group.IsEmpty())
      {
        UsedGroupList->Add(File->Group);
      }
      if (UsedUserList && !File->Owner.IsEmpty())
      {
        UsedUserList->Add(File->Owner);
      }
      if (File->IsDirectory)
      {
        Directories++;
      }
    }
    FAnyDirectories = (Directories > 0);

    Caption = GetMsg(PROPERTIES_CAPTION);

    Size = TPoint(56, 19);

    TFarButton * Button;
    TFarSeparator * Separator;
    TFarText * Text;
    TRect CRect = ClientRect;

    Text = new TFarText(this);
    Text->Caption = GetMsg(PROPERTIES_PROMPT);
    Text->CenterGroup = true;

    NextItemPosition = ipNewLine;

    Text = new TFarText(this);
    Text->CenterGroup = true;
    if (FileList->Count > 1)
    {
      Text->Caption = FORMAT(GetMsg(PROPERTIES_PROMPT_FILES), (FileList->Count));
    }
    else
    {
      Text->Caption = MinimizeName(FileList->Strings[0], ClientSize.x, true);
    }

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->Caption = GetMsg(PROPERTIES_OWNER);
    Text->Enabled = FAllowedChanges & cpOwner;

    NextItemPosition = ipRight;

    OwnerComboBox = new TFarComboBox(this);
    OwnerComboBox->Width = 20;
    OwnerComboBox->Enabled = FAllowedChanges & cpOwner;
    OwnerComboBox->Items->Assign(UsedUserList ? UsedUserList : UserList);

    NextItemPosition = ipNewLine;

    Text = new TFarText(this);
    Text->Caption = GetMsg(PROPERTIES_GROUP);
    Text->Enabled = FAllowedChanges & cpGroup;

    NextItemPosition = ipRight;

    GroupComboBox = new TFarComboBox(this);
    GroupComboBox->Width = OwnerComboBox->Width;
    GroupComboBox->Enabled = FAllowedChanges & cpGroup;
    GroupComboBox->Items->Assign(UsedGroupList ? UsedGroupList : GroupList);

    NextItemPosition = ipNewLine;

    Separator = new TFarSeparator(this);
    Separator->Caption = GetMsg(PROPERTIES_RIGHTS);

    RightsContainer = new TRightsContainer(this, FAnyDirectories,
      true, true, NULL);
    RightsContainer->Enabled = FAllowedChanges & cpMode;

    if (FAnyDirectories)
    {
      Separator = new TFarSeparator(this);
      Separator->Position = Separator->Position + RightsContainer->Top;

      RecursiveCheck = new TFarCheckBox(this);
      RecursiveCheck->Caption = GetMsg(PROPERTIES_RECURSIVE);
    }
    else
    {
      RecursiveCheck = NULL;
    }

    NextItemPosition = ipNewLine;

    Separator = new TFarSeparator(this);
    Separator->Position = CRect.Bottom - 1;

    OkButton = new TFarButton(this);
    OkButton->Caption = GetMsg(MSG_BUTTON_OK);
    OkButton->Default = true;
    OkButton->Result = brOK;
    OkButton->CenterGroup = true;

    NextItemPosition = ipRight;

    Button = new TFarButton(this);
    Button->Caption = GetMsg(MSG_BUTTON_Cancel);
    Button->Result = brCancel;
    Button->CenterGroup = true;
  }
  __finally
  {
    delete UsedUserList;
    delete UsedGroupList;
  }
}
//---------------------------------------------------------------------------
void __fastcall TPropertiesDialog::Change()
{
  TFarDialog::Change();

  if (Handle)
  {
    TRemoteProperties FileProperties;
    UpdateProperties(FileProperties);

    if (!FMultiple)
    {
      // when setting properties for one file only, allow undef state
      // only when the input right explicitly requires it or
      // when "recursive" is on (possible for directory only).
      bool AllowUndef =
        (FOrigProperties.Valid.Contains(vpRights) &&
         FOrigProperties.Rights.AllowUndef) ||
        ((RecursiveCheck != NULL) && (RecursiveCheck->Checked));
      if (!AllowUndef)
      {
        // when disallowing undef state, make sure, all undef are turned into unset
        RightsContainer->Rights = TRights(RightsContainer->Rights.NumberSet);
      }
      RightsContainer->AllowUndef = AllowUndef;
    }

    OkButton->Enabled =
      // group name is specified or we set multiple-file properties and
      // no valid group was specified (there are at least two different groups)
      (!GroupComboBox->Text.IsEmpty() ||
       (FMultiple && !FOrigProperties.Valid.Contains(vpGroup)) ||
       (FOrigProperties.Group == GroupComboBox->Text)) &&
      // same but with owner
      (!OwnerComboBox->Text.IsEmpty() ||
       (FMultiple && !FOrigProperties.Valid.Contains(vpOwner)) ||
       (FOrigProperties.Owner == OwnerComboBox->Text)) &&
      ((FileProperties != FOrigProperties) || (RecursiveCheck && RecursiveCheck->Checked));
  }
}
//---------------------------------------------------------------------------
void __fastcall TPropertiesDialog::UpdateProperties(TRemoteProperties & Properties)
{
  if (FAllowedChanges & cpMode)
  {
    Properties.Valid << vpRights;
    Properties.Rights = RightsContainer->Rights;
    Properties.AddXToDirectories = RightsContainer->AddXToDirectories;
  }

  #define STORE_NAME(PROPERTY) \
    if (!PROPERTY ## ComboBox->Text.IsEmpty() && \
        FAllowedChanges & cp ## PROPERTY) \
    { \
      Properties.Valid << vp ## PROPERTY; \
      Properties.PROPERTY = PROPERTY ## ComboBox->Text.Trim(); \
    }
  STORE_NAME(Group);
  STORE_NAME(Owner);
  #undef STORE_NAME

  Properties.Recursive = RecursiveCheck != NULL && RecursiveCheck->Checked;
}
//---------------------------------------------------------------------------
bool __fastcall TPropertiesDialog::Execute(TRemoteProperties * Properties)
{
  TValidProperties Valid;
  if (Properties->Valid.Contains(vpRights) && FAllowedChanges & cpMode) Valid << vpRights;
  if (Properties->Valid.Contains(vpOwner) && FAllowedChanges & cpOwner) Valid << vpOwner;
  if (Properties->Valid.Contains(vpGroup) && FAllowedChanges & cpGroup) Valid << vpGroup;
  FOrigProperties = *Properties;
  FOrigProperties.Valid = Valid;
  FOrigProperties.Recursive = false;

  if (Properties->Valid.Contains(vpRights))
  {
    RightsContainer->Rights = Properties->Rights;
    RightsContainer->AddXToDirectories = Properties->AddXToDirectories;
  }
    else
  {
    RightsContainer->Rights = TRights();
    RightsContainer->AddXToDirectories = false;
  }
  OwnerComboBox->Text = Properties->Valid.Contains(vpOwner) ?
    Properties->Owner : AnsiString();
  GroupComboBox->Text = Properties->Valid.Contains(vpGroup) ?
    Properties->Group : AnsiString();
  if (RecursiveCheck)
  {
    RecursiveCheck->Checked = Properties->Recursive;
  }

  bool Result = ShowModal() != brCancel;
  if (Result)
  {
    *Properties = TRemoteProperties();
    UpdateProperties(*Properties);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::PropertiesDialog(TStrings * FileList,
  const AnsiString Directory, TStrings * GroupList, TStrings * UserList,
  TRemoteProperties * Properties, int AllowedChanges)
{
  bool Result;
  TPropertiesDialog * Dialog = new TPropertiesDialog(FPlugin, FileList,
    Directory, GroupList, UserList, AllowedChanges);
  try
  {
    Result = Dialog->Execute(Properties);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCopyParamsContainer : public TFarDialogContainer
{
public:
  __fastcall TCopyParamsContainer(TFarDialog * ADialog,
    int Options, int CopyParamAttrs);

  __property TCopyParamType Params = { read = GetParams, write = SetParams };
  __property int Height = { read = GetHeight };

protected:
  TFarRadioButton * TMTextButton;
  TFarRadioButton * TMBinaryButton;
  TFarRadioButton * TMAutomaticButton;
  TFarEdit * AsciiFileMaskEdit;
  TRightsContainer * RightsContainer;
  TFarRadioButton * CCNoChangeButton;
  TFarRadioButton * CCUpperCaseButton;
  TFarRadioButton * CCLowerCaseButton;
  TFarRadioButton * CCFirstUpperCaseButton;
  TFarRadioButton * CCLowerCaseShortButton;
  TFarCheckBox * ReplaceInvalidCharsCheck;
  TFarCheckBox * PreserveRightsCheck;
  TFarCheckBox * PreserveTimeCheck;
  TFarCheckBox * PreserveReadOnlyCheck;
  TFarCheckBox * IgnorePermErrorsCheck;
  TFarCheckBox * ClearArchiveCheck;
  TFarComboBox * NegativeExcludeCombo;
  TFarEdit * ExcludeFileMaskCombo;
  TFarCheckBox * CalculateSizeCheck;
  TFarComboBox * SpeedCombo;

  void __fastcall ValidateMaskComboExit(TObject * Sender);
  void __fastcall ValidateSpeedComboExit(TObject * Sender);
  virtual void __fastcall Change();
  void __fastcall UpdateControls();

private:
  int FOptions;
  int FCopyParamAttrs;
  TCopyParamType FParams;

  void __fastcall SetParams(TCopyParamType value);
  TCopyParamType __fastcall GetParams();
  int __fastcall GetHeight();
};
//---------------------------------------------------------------------------
__fastcall TCopyParamsContainer::TCopyParamsContainer(TFarDialog * ADialog,
  int Options, int CopyParamAttrs) :
  TFarDialogContainer(ADialog),
  FOptions(Options), FCopyParamAttrs(CopyParamAttrs)
{
  TFarBox * Box;
  TFarSeparator * Separator;
  TFarText * Text;

  int TMWidth = 37;
  int TMTop;
  int TMBottom;

  Left--;

  Box = new TFarBox(Dialog);
  Box->Left = 0;
  Box->Top = 0;
  Box->Height = 1;
  Add(Box);
  Box->Width = TMWidth + 2;
  Box->Caption = GetMsg(TRANSFER_MODE);

  Dialog->NextItemPosition = ipRight;

  Box = new TFarBox(Dialog);
  Add(Box);
  Box->Left -= 2;
  Box->Right++;
  Box->Caption = GetMsg(TRANSFER_UPLOAD_OPTIONS);

  Dialog->NextItemPosition = ipNewLine;

  TMTextButton = new TFarRadioButton(Dialog);
  TMTextButton->Left = 1;
  Add(TMTextButton);
  TMTop = TMTextButton->Top;
  TMTextButton->Caption = GetMsg(TRANSFER_MODE_TEXT);
  TMTextButton->Enabled =
    FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode) &&
    FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly);

  TMBinaryButton = new TFarRadioButton(Dialog);
  TMBinaryButton->Left = 1;
  Add(TMBinaryButton);
  TMBinaryButton->Caption = GetMsg(TRANSFER_MODE_BINARY);
  TMBinaryButton->Enabled = TMTextButton->Enabled;

  TMAutomaticButton = new TFarRadioButton(Dialog);
  TMAutomaticButton->Left = 1;
  Add(TMAutomaticButton);
  TMAutomaticButton->Caption = GetMsg(TRANSFER_MODE_AUTOMATIC);
  TMAutomaticButton->Enabled = TMTextButton->Enabled;

  Text = new TFarText(Dialog);
  Text->Left = 1;
  Add(Text);
  Text->Caption = GetMsg(TRANSFER_MODE_MASK);
  Text->EnabledDependency = TMAutomaticButton;

  AsciiFileMaskEdit = new TFarEdit(Dialog);
  AsciiFileMaskEdit->Left = 1;
  Add(AsciiFileMaskEdit);
  AsciiFileMaskEdit->EnabledDependency = TMAutomaticButton;
  AsciiFileMaskEdit->Width = TMWidth;
  AsciiFileMaskEdit->History = ASCII_MASK_HISTORY;
  AsciiFileMaskEdit->OnExit = ValidateMaskComboExit;

  Box = new TFarBox(Dialog);
  Box->Left = 0;
  Add(Box);
  Box->Width = TMWidth + 2;
  Box->Caption = GetMsg(TRANSFER_FILENAME_MODIFICATION);

  CCNoChangeButton = new TFarRadioButton(Dialog);
  CCNoChangeButton->Left = 1;
  Add(CCNoChangeButton);
  CCNoChangeButton->Caption = GetMsg(TRANSFER_FILENAME_NOCHANGE);
  CCNoChangeButton->Enabled = FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly);

  Dialog->NextItemPosition = ipRight;

  CCUpperCaseButton = new TFarRadioButton(Dialog);
  Add(CCUpperCaseButton);
  CCUpperCaseButton->Caption = GetMsg(TRANSFER_FILENAME_UPPERCASE);
  CCUpperCaseButton->Enabled = CCNoChangeButton->Enabled;

  Dialog->NextItemPosition = ipNewLine;

  CCFirstUpperCaseButton = new TFarRadioButton(Dialog);
  CCFirstUpperCaseButton->Left = 1;
  Add(CCFirstUpperCaseButton);
  CCFirstUpperCaseButton->Caption = GetMsg(TRANSFER_FILENAME_FIRSTUPPERCASE);
  CCFirstUpperCaseButton->Enabled = CCNoChangeButton->Enabled;

  Dialog->NextItemPosition = ipRight;

  CCLowerCaseButton = new TFarRadioButton(Dialog);
  Add(CCLowerCaseButton);
  CCLowerCaseButton->Caption = GetMsg(TRANSFER_FILENAME_LOWERCASE);
  CCLowerCaseButton->Enabled = CCNoChangeButton->Enabled;

  Dialog->NextItemPosition = ipNewLine;

  CCLowerCaseShortButton = new TFarRadioButton(Dialog);
  CCLowerCaseShortButton->Left = 1;
  Add(CCLowerCaseShortButton);
  CCLowerCaseShortButton->Caption = GetMsg(TRANSFER_FILENAME_LOWERCASESHORT);
  CCLowerCaseShortButton->Enabled = CCNoChangeButton->Enabled;

  Dialog->NextItemPosition = ipRight;

  ReplaceInvalidCharsCheck = new TFarCheckBox(Dialog);
  Add(ReplaceInvalidCharsCheck);
  ReplaceInvalidCharsCheck->Caption = GetMsg(TRANSFER_FILENAME_REPLACE_INVALID);
  ReplaceInvalidCharsCheck->Enabled = CCNoChangeButton->Enabled;

  Dialog->NextItemPosition = ipNewLine;

  Box = new TFarBox(Dialog);
  Box->Left = 0;
  Add(Box);
  Box->Width = TMWidth + 2;
  Box->Caption = GetMsg(TRANSFER_DOWNLOAD_OPTIONS);

  PreserveReadOnlyCheck = new TFarCheckBox(Dialog);
  Add(PreserveReadOnlyCheck);
  PreserveReadOnlyCheck->Left = 1;
  PreserveReadOnlyCheck->Caption = GetMsg(TRANSFER_PRESERVE_READONLY);
  PreserveReadOnlyCheck->Enabled =
    FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly) &&
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveReadOnly);
  TMBottom = PreserveReadOnlyCheck->Top;

  PreserveRightsCheck = new TFarCheckBox(Dialog);
  Add(PreserveRightsCheck);
  PreserveRightsCheck->Left = TMWidth + 3;
  PreserveRightsCheck->Top = TMTop;
  PreserveRightsCheck->Bottom = TMTop;
  PreserveRightsCheck->Caption = GetMsg(TRANSFER_PRESERVE_RIGHTS);
  PreserveRightsCheck->Enabled =
    FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly) &&
    FLAGCLEAR(CopyParamAttrs, cpaNoRights);

  Dialog->NextItemPosition = ipBelow;

  RightsContainer = new TRightsContainer(Dialog, true, false,
    false, PreserveRightsCheck);
  RightsContainer->Left = PreserveRightsCheck->ActualBounds.Left;
  RightsContainer->Top = PreserveRightsCheck->ActualBounds.Top + 1;

  IgnorePermErrorsCheck = new TFarCheckBox(Dialog);
  Add(IgnorePermErrorsCheck);
  IgnorePermErrorsCheck->Left = PreserveRightsCheck->Left;
  IgnorePermErrorsCheck->Top = TMTop + 6;
  IgnorePermErrorsCheck->Caption = GetMsg(TRANSFER_PRESERVE_PERM_ERRORS);

  ClearArchiveCheck = new TFarCheckBox(Dialog);
  ClearArchiveCheck->Left = IgnorePermErrorsCheck->Left;
  Add(ClearArchiveCheck);
  ClearArchiveCheck->Top = TMTop + 7;
  ClearArchiveCheck->Caption = GetMsg(TRANSFER_CLEAR_ARCHIVE);
  ClearArchiveCheck->Enabled =
    FLAGCLEAR(FOptions, coTempTransfer) &&
    FLAGCLEAR(CopyParamAttrs, cpaNoClearArchive) &&
    FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly);

  Box = new TFarBox(Dialog);
  Box->Top = TMTop + 8;
  Add(Box);
  Box->Bottom = Box->Top;
  Box->Left = TMWidth + 3 - 1;
  Box->Caption = GetMsg(TRANSFER_COMMON_OPTIONS);

  PreserveTimeCheck = new TFarCheckBox(Dialog);
  Add(PreserveTimeCheck);
  PreserveTimeCheck->Left = TMWidth + 3;
  PreserveTimeCheck->Caption = GetMsg(TRANSFER_PRESERVE_TIMESTAMP);
  PreserveTimeCheck->Enabled =
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveTime) &&
    FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly);

  CalculateSizeCheck = new TFarCheckBox(Dialog);
  CalculateSizeCheck->Caption = GetMsg(TRANSFER_CALCULATE_SIZE);
  Add(CalculateSizeCheck);
  CalculateSizeCheck->Left = TMWidth + 3;

  Dialog->NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(Dialog);
  Add(Separator);
  Separator->Position = TMBottom + 1;
  Separator->Caption = GetMsg(TRANSFER_OTHER);

  NegativeExcludeCombo = new TFarComboBox(Dialog);
  NegativeExcludeCombo->Left = 1;
  Add(NegativeExcludeCombo);
  NegativeExcludeCombo->Items->Add(GetMsg(TRANSFER_EXCLUDE));
  NegativeExcludeCombo->Items->Add(GetMsg(TRANSFER_INCLUDE));
  NegativeExcludeCombo->DropDownList = true;
  NegativeExcludeCombo->ResizeToFitContent();
  NegativeExcludeCombo->Enabled =
    FLAGCLEAR(FOptions, coTempTransfer) &&
    (FLAGCLEAR(CopyParamAttrs, cpaNoExcludeMask) ||
     FLAGSET(CopyParamAttrs, cpaExcludeMaskOnly));

  Dialog->NextItemPosition = ipRight;

  Text = new TFarText(Dialog);
  Add(Text);
  Text->Caption = GetMsg(TRANSFER_EXCLUDE_FILE_MASK);
  Text->Enabled = NegativeExcludeCombo->Enabled;

  Dialog->NextItemPosition = ipNewLine;

  ExcludeFileMaskCombo = new TFarEdit(Dialog);
  ExcludeFileMaskCombo->Left = 1;
  Add(ExcludeFileMaskCombo);
  ExcludeFileMaskCombo->Width = TMWidth;
  ExcludeFileMaskCombo->History = EXCLUDE_FILE_MASK_HISTORY;
  ExcludeFileMaskCombo->OnExit = ValidateMaskComboExit;
  ExcludeFileMaskCombo->Enabled = NegativeExcludeCombo->Enabled;

  Dialog->NextItemPosition = ipNewLine;

  Text = new TFarText(Dialog);
  Add(Text);
  Text->Caption = GetMsg(TRANSFER_SPEED);
  Text->MoveAt(TMWidth + 3, NegativeExcludeCombo->Top);

  Dialog->NextItemPosition = ipRight;

  SpeedCombo = new TFarComboBox(Dialog);
  Add(SpeedCombo);
  SpeedCombo->Items->Add(LoadStr(SPEED_UNLIMITED));
  unsigned long Speed = 1024;
  while (Speed >= 8)
  {
    SpeedCombo->Items->Add(IntToStr(Speed));
    Speed = Speed / 2;
  }
  SpeedCombo->OnExit = ValidateSpeedComboExit;

  Dialog->NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(Dialog);
  Separator->Position = ExcludeFileMaskCombo->Bottom + 1;
  Separator->Left = 0;
  Add(Separator);
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamsContainer::UpdateControls()
{
  if (IgnorePermErrorsCheck != NULL)
  {
    IgnorePermErrorsCheck->Enabled =
      ((PreserveRightsCheck->Enabled && PreserveRightsCheck->Checked) ||
       (PreserveTimeCheck->Enabled && PreserveTimeCheck->Checked)) &&
      FLAGCLEAR(FCopyParamAttrs, cpaNoIgnorePermErrors) &&
      FLAGCLEAR(FCopyParamAttrs, cpaExcludeMaskOnly);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamsContainer::Change()
{
  TFarDialogContainer::Change();

  if (Dialog->Handle)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamsContainer::SetParams(TCopyParamType value)
{
  if (TMBinaryButton->Enabled)
  {
    switch (value.TransferMode)
    {
      case tmAscii:
        TMTextButton->Checked = true;
        break;

      case tmBinary:
        TMBinaryButton->Checked = true;
        break;

      default:
        TMAutomaticButton->Checked = true;
        break;
    }
  }
  else
  {
    TMBinaryButton->Checked = true;
  }

  AsciiFileMaskEdit->Text = value.AsciiFileMask.Masks;

  switch (value.FileNameCase)
  {
    case ncLowerCase:
      CCLowerCaseButton->Checked = true;
      break;

    case ncUpperCase:
      CCUpperCaseButton->Checked = true;
      break;

    case ncFirstUpperCase:
      CCFirstUpperCaseButton->Checked = true;
      break;

    case ncLowerCaseShort:
      CCLowerCaseShortButton->Checked = true;
      break;

    default:
    case ncNoChange:
      CCNoChangeButton->Checked = true;
      break;
  }

  RightsContainer->AddXToDirectories = value.AddXToDirectories;
  RightsContainer->Rights = value.Rights;
  PreserveRightsCheck->Checked = value.PreserveRights;
  IgnorePermErrorsCheck->Checked = value.IgnorePermErrors;

  PreserveReadOnlyCheck->Checked = value.PreserveReadOnly;
  ReplaceInvalidCharsCheck->Checked =
    (value.InvalidCharsReplacement != TCopyParamType::NoReplacement);

  ClearArchiveCheck->Checked = value.ClearArchive;

  NegativeExcludeCombo->Items->Selected = (value.NegativeExclude ? 1 : 0);
  ExcludeFileMaskCombo->Text = value.ExcludeFileMask.Masks;

  PreserveTimeCheck->Checked = value.PreserveTime;
  CalculateSizeCheck->Checked = value.CalculateSize;

  SpeedCombo->Text = SetSpeedLimit(value.CPSLimit);

  FParams = value;
}
//---------------------------------------------------------------------------
TCopyParamType __fastcall TCopyParamsContainer::GetParams()
{
  TCopyParamType Result = FParams;

  assert(TMTextButton->Checked || TMBinaryButton->Checked || TMAutomaticButton->Checked);
  if (TMTextButton->Checked) Result.TransferMode = tmAscii;
    else
  if (TMAutomaticButton->Checked) Result.TransferMode = tmAutomatic;
    else Result.TransferMode = tmBinary;

  if (Result.TransferMode == tmAutomatic)
  {
    Result.AsciiFileMask.Masks = AsciiFileMaskEdit->Text;
    assert(Result.AsciiFileMask.IsValid());
  }

  if (CCLowerCaseButton->Checked) Result.FileNameCase = ncLowerCase;
    else
  if (CCUpperCaseButton->Checked) Result.FileNameCase = ncUpperCase;
    else
  if (CCFirstUpperCaseButton->Checked) Result.FileNameCase = ncFirstUpperCase;
    else
  if (CCLowerCaseShortButton->Checked) Result.FileNameCase = ncLowerCaseShort;
    else Result.FileNameCase = ncNoChange;

  Result.AddXToDirectories = RightsContainer->AddXToDirectories;
  Result.Rights = RightsContainer->Rights;
  Result.PreserveRights = PreserveRightsCheck->Checked;
  Result.IgnorePermErrors = IgnorePermErrorsCheck->Checked;

  Result.ReplaceInvalidChars = ReplaceInvalidCharsCheck->Checked;
  Result.PreserveReadOnly = PreserveReadOnlyCheck->Checked;

  Result.ClearArchive = ClearArchiveCheck->Checked;

  Result.NegativeExclude = (NegativeExcludeCombo->Items->Selected == 1);
  Result.ExcludeFileMask.Masks = ExcludeFileMaskCombo->Text;

  Result.PreserveTime = PreserveTimeCheck->Checked;
  Result.CalculateSize = CalculateSizeCheck->Checked;

  Result.CPSLimit = GetSpeedLimit(SpeedCombo->Text);

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamsContainer::ValidateMaskComboExit(TObject * Sender)
{
  TFarEdit * Edit = dynamic_cast<TFarEdit *>(Sender);
  assert(Edit != NULL);
  TFileMasks Masks = Edit->Text;
  int Start, Length;
  if (!Masks.IsValid(Start, Length))
  {
    Edit->SetFocus();
    throw Exception(FORMAT(GetMsg(MASK_ERROR), (Masks.Masks.SubString(Start+1, Length))));
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyParamsContainer::ValidateSpeedComboExit(TObject * /*Sender*/)
{
  try
  {
    GetSpeedLimit(SpeedCombo->Text);
  }
  catch(...)
  {
    SpeedCombo->SetFocus();
    throw;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCopyParamsContainer::GetHeight()
{
  return 16;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCopyDialog : TFarDialog
{
public:
  __fastcall TCopyDialog(TCustomFarPlugin * AFarPlugin,
    bool ToRemote, bool Move, TStrings * FileList, int Options, int CopyParamAttrs);

  bool __fastcall Execute(AnsiString & TargetDirectory, TGUICopyParamType * Params);

protected:
  virtual bool __fastcall CloseQuery();
  virtual void __fastcall Change();
  void __fastcall CustomCopyParam();

  void __fastcall CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
  void __fastcall TransferSettingsButtonClick(TFarButton * Sender, bool & Close);

private:
  TFarEdit * DirectoryEdit;
  TFarLister * CopyParamLister;
  TFarCheckBox * NewerOnlyCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarCheckBox * QueueCheck;
  TFarCheckBox * QueueNoConfirmationCheck;

  bool FToRemote;
  int FOptions;
  int FCopyParamAttrs;
  TGUICopyParamType FCopyParams;
};
//---------------------------------------------------------------------------
__fastcall TCopyDialog::TCopyDialog(TCustomFarPlugin * AFarPlugin,
  bool ToRemote, bool Move, TStrings * FileList,
  int Options, int CopyParamAttrs) : TFarDialog(AFarPlugin)
{
  FToRemote = ToRemote;
  FOptions = Options;
  FCopyParamAttrs = CopyParamAttrs;

  TFarButton * Button;
  TFarSeparator * Separator;
  TFarText * Text;

  Size = TPoint(78, 12 + (FLAGCLEAR(FOptions, coTempTransfer) ? 4 : 0));
  TRect CRect = ClientRect;

  Caption = GetMsg(Move ? MOVE_TITLE : COPY_TITLE);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    AnsiString Prompt;
    if (FileList->Count > 1)
    {
      Prompt = FORMAT(GetMsg(Move ? MOVE_FILES_PROMPT : COPY_FILES_PROMPT), (FileList->Count));
    }
    else
    {
      Prompt = FORMAT(GetMsg(Move ? MOVE_FILE_PROMPT : COPY_FILE_PROMPT),
        (ToRemote ? ExtractFileName(FileList->Strings[0]) :
            UnixExtractFileName(FileList->Strings[0])));
    }

    Text = new TFarText(this);
    Text->Caption = Prompt;

    DirectoryEdit = new TFarEdit(this);
    DirectoryEdit->History = ToRemote ? REMOTE_DIR_HISTORY : "Copy";
  }

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(COPY_PARAM_GROUP);

  CopyParamLister = new TFarLister(this);
  CopyParamLister->Height = 3;
  CopyParamLister->Left = BorderBox->Left + 1;
  CopyParamLister->TabStop = false;
  CopyParamLister->OnMouseClick = CopyParamListerClick;

  new TFarSeparator(this);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck = new TFarCheckBox(this);
    NewerOnlyCheck->Caption = GetMsg(TRANSFER_NEWER_ONLY);
    NewerOnlyCheck->Enabled = FLAGCLEAR(FOptions, coDisableNewerOnly);

    QueueCheck = new TFarCheckBox(this);
    QueueCheck->Caption = GetMsg(TRANSFER_QUEUE);

    NextItemPosition = ipRight;

    QueueNoConfirmationCheck = new TFarCheckBox(this);
    QueueNoConfirmationCheck->Caption = GetMsg(TRANSFER_QUEUE_NO_CONFIRMATION);
    QueueNoConfirmationCheck->EnabledDependency = QueueCheck;

    NextItemPosition = ipNewLine;
  }
  else
  {
    assert(FLAGSET(FOptions, coDisableNewerOnly));
  }

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->Caption = GetMsg(TRANSFER_REUSE_SETTINGS);

  new TFarSeparator(this);

  Button = new TFarButton(this);
  Button->Caption = GetMsg(TRANSFER_SETTINGS_BUTTON);
  Button->Result = -1;
  Button->CenterGroup = true;
  Button->OnClick = TransferSettingsButtonClick;

  NextItemPosition = ipRight;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_OK);
  Button->Default = true;
  Button->Result = brOK;
  Button->CenterGroup = true;
  Button->EnabledDependency =
    ((Options & coTempTransfer) == 0) ? DirectoryEdit : NULL;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_Cancel);
  Button->Result = brCancel;
  Button->CenterGroup = true;
}
//---------------------------------------------------------------------------
bool __fastcall TCopyDialog::Execute(AnsiString & TargetDirectory,
  TGUICopyParamType * Params)
{
  FCopyParams = *Params;

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck->Checked = FLAGCLEAR(FOptions, coDisableNewerOnly) && Params->NewerOnly;

    DirectoryEdit->Text =
      (FToRemote ? UnixIncludeTrailingBackslash(TargetDirectory) :
        IncludeTrailingBackslash(TargetDirectory)) + Params->FileMask;

    QueueCheck->Checked = Params->Queue;
    QueueNoConfirmationCheck->Checked = Params->QueueNoConfirmation;
  }

  bool Result = ShowModal() != brCancel;

  if (Result)
  {
    *Params = FCopyParams;

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
      if (FToRemote)
      {
        Params->FileMask = UnixExtractFileName(DirectoryEdit->Text);
        TargetDirectory = UnixExtractFilePath(DirectoryEdit->Text);
      }
      else
      {
        Params->FileMask = ExtractFileName(DirectoryEdit->Text);
        TargetDirectory = ExtractFilePath(DirectoryEdit->Text);
      }

      Params->NewerOnly = FLAGCLEAR(FOptions, coDisableNewerOnly) && NewerOnlyCheck->Checked;

      Params->Queue = QueueCheck->Checked;
      Params->QueueNoConfirmation = QueueNoConfirmationCheck->Checked;
    }

    Configuration->BeginUpdate();
    try
    {
      if (SaveSettingsCheck->Checked)
      {
        GUIConfiguration->DefaultCopyParam = *Params;
      }
    }
    __finally
    {
      Configuration->EndUpdate();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCopyDialog::CloseQuery()
{
  bool CanClose = TFarDialog::CloseQuery();

  if (CanClose && Result >= 0)
  {
    if (!FToRemote && ((FOptions & coTempTransfer) == 0))
    {
      AnsiString Directory = ExtractFilePath(DirectoryEdit->Text);
      if (!DirectoryExists(Directory))
      {
        TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);

        if (WinSCPPlugin->MoreMessageDialog(FORMAT(GetMsg(CREATE_LOCAL_DIRECTORY), (Directory)),
              NULL, qtConfirmation, qaOK | qaCancel) != qaCancel)
        {
          if (!ForceDirectories(Directory))
          {
            DirectoryEdit->SetFocus();
            throw Exception(FORMAT(GetMsg(CREATE_LOCAL_DIR_ERROR), (Directory)));
          }
        }
        else
        {
          DirectoryEdit->SetFocus();
          Abort();
        }
      }
    }
  }
  return CanClose;
}
//---------------------------------------------------------------------------
void __fastcall TCopyDialog::Change()
{
  TFarDialog::Change();

  if (Handle)
  {
    AnsiString InfoStr = FCopyParams.GetInfoStr("; ", FCopyParamAttrs);
    TStringList * InfoStrLines = new TStringList();
    try
    {
      FarWrapText(InfoStr, InfoStrLines, BorderBox->Width - 4);
      CopyParamLister->Items = InfoStrLines;
      CopyParamLister->Right = BorderBox->Right - (CopyParamLister->ScrollBar ? 0 : 1);
    }
    __finally
    {
      delete InfoStrLines;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TCopyDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}
//---------------------------------------------------------------------------
void __fastcall TCopyDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, FCopyParamAttrs))
  {
    Change();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::CopyDialog(bool ToRemote,
  bool Move, TStrings * FileList,
  AnsiString & TargetDirectory, TGUICopyParamType * Params, int Options,
  int CopyParamAttrs)
{
  bool Result;
  TCopyDialog * Dialog = new TCopyDialog(FPlugin, ToRemote,
    Move, FileList, Options, CopyParamAttrs);
  try
  {
    Result = Dialog->Execute(TargetDirectory, Params);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::CopyParamDialog(AnsiString Caption,
  TCopyParamType & CopyParam, int CopyParamAttrs)
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(this);
  try
  {
    Dialog->Caption = Caption;

    // temporary
    Dialog->Size = TPoint(78, 10);

    TCopyParamsContainer * CopyParamsContainer = new TCopyParamsContainer(
      Dialog, 0, CopyParamAttrs);

    Dialog->Size = TPoint(78, 2 + CopyParamsContainer->Height + 3);

    Dialog->NextItemPosition = ipNewLine;

    Dialog->AddStandardButtons(2, true);

    CopyParamsContainer->Params = CopyParam;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      CopyParam = CopyParamsContainer->Params;
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPPlugin::CopyParamCustomDialog(TCopyParamType & CopyParam,
  int CopyParamAttrs)
{
  return CopyParamDialog(GetMsg(COPY_PARAM_CUSTOM_TITLE), CopyParam, CopyParamAttrs);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TLinkDialog : TFarDialog
{
public:
  __fastcall TLinkDialog(TCustomFarPlugin * AFarPlugin,
    bool Edit, bool AllowSymbolic);

  bool __fastcall Execute(AnsiString & FileName, AnsiString & PointTo,
    bool & Symbolic);

protected:
  virtual void __fastcall Change();

private:
  TFarEdit * FileNameEdit;
  TFarEdit * PointToEdit;
  TFarCheckBox * SymbolicCheck;
  TFarButton * OkButton;
};
//---------------------------------------------------------------------------
__fastcall TLinkDialog::TLinkDialog(TCustomFarPlugin * AFarPlugin,
    bool Edit, bool AllowSymbolic) : TFarDialog(AFarPlugin)
{
  TFarButton * Button;
  TFarSeparator * Separator;
  TFarText * Text;

  Size = TPoint(76, 12);
  TRect CRect = ClientRect;

  Caption = GetMsg(Edit ? LINK_EDIT_CAPTION : LINK_ADD_CAPTION);

  Text = new TFarText(this);
  Text->Caption = GetMsg(LINK_FILE);
  Text->Enabled = !Edit;

  FileNameEdit = new TFarEdit(this);
  FileNameEdit->Enabled = !Edit;
  FileNameEdit->History = LINK_FILENAME_HISTORY;

  Text = new TFarText(this);
  Text->Caption = GetMsg(LINK_POINT_TO);

  PointToEdit = new TFarEdit(this);
  PointToEdit->History = LINK_POINT_TO_HISTORY;

  new TFarSeparator(this);

  SymbolicCheck = new TFarCheckBox(this);
  SymbolicCheck->Caption = GetMsg(LINK_SYMLINK);
  SymbolicCheck->Enabled = AllowSymbolic && !Edit;

  Separator = new TFarSeparator(this);
  Separator->Position = CRect.Bottom - 1;

  OkButton = new TFarButton(this);
  OkButton->Caption = GetMsg(MSG_BUTTON_OK);
  OkButton->Default = true;
  OkButton->Result = brOK;
  OkButton->CenterGroup = true;

  NextItemPosition = ipRight;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(MSG_BUTTON_Cancel);
  Button->Result = brCancel;
  Button->CenterGroup = true;
}
//---------------------------------------------------------------------------
void __fastcall TLinkDialog::Change()
{
  TFarDialog::Change();

  if (Handle)
  {
    OkButton->Enabled = !FileNameEdit->Text.IsEmpty() &&
      !PointToEdit->Text.IsEmpty();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TLinkDialog::Execute(AnsiString & FileName, AnsiString & PointTo,
    bool & Symbolic)
{
  FileNameEdit->Text = FileName;
  PointToEdit->Text = PointTo;
  SymbolicCheck->Checked = Symbolic;

  bool Result = ShowModal() != brCancel;
  if (Result)
  {
    FileName = FileNameEdit->Text;
    PointTo = PointToEdit->Text;
    Symbolic = SymbolicCheck->Checked;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::LinkDialog(AnsiString & FileName,
  AnsiString & PointTo, bool & Symbolic, bool Edit, bool AllowSymbolic)
{
  bool Result;
  TLinkDialog * Dialog = new TLinkDialog(FPlugin, Edit, AllowSymbolic);
  try
  {
    Result = Dialog->Execute(FileName, PointTo, Symbolic);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
typedef void __fastcall (__closure *TFeedFileSystemData)
  (TObject * Control, int Label, AnsiString Value);
//---------------------------------------------------------------------------
class TLabelList;
class TFileSystemInfoDialog : TTabbedDialog
{
public:
  enum { tabProtocol = 1, tabCapabilities, tabSpaceAvailable, tabCount };

  __fastcall TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
    TGetSpaceAvailable OnGetSpaceAvailable);

  void __fastcall Execute(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, AnsiString SpaceAvailablePath);

protected:
  void __fastcall Feed(TFeedFileSystemData AddItem);
  AnsiString __fastcall CapabilityStr(TFSCapability Capability);
  AnsiString __fastcall CapabilityStr(TFSCapability Capability1,
    TFSCapability Capability2);
  AnsiString __fastcall SpaceStr(__int64 Bytes);
  void __fastcall ControlsAddItem(TObject * Control, int Label, AnsiString Value);
  void __fastcall CalculateMaxLenAddItem(TObject * Control, int Label, AnsiString Value);
  void __fastcall ClipboardAddItem(TObject * Control, int Label, AnsiString Value);
  void __fastcall FeedControls();
  void __fastcall UpdateControls();
  TLabelList * __fastcall CreateLabelArray(int Count);
  virtual void __fastcall SelectTab(int Tab);
  virtual void __fastcall Change();
  void __fastcall SpaceAvailableButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall ClipboardButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall CheckSpaceAvailable();
  void __fastcall NeedSpaceAvailable();
  bool __fastcall SpaceAvailableSupported();
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);

private:
  TGetSpaceAvailable FOnGetSpaceAvailable;
  TFileSystemInfo FFileSystemInfo;
  TSessionInfo FSessionInfo;
  bool FSpaceAvailableLoaded;
  TSpaceAvailable FSpaceAvailable;
  TObject * FLastFeededControl;
  int FLastListItem;
  AnsiString FClipboard;

  TLabelList * ServerLabels;
  TLabelList * ProtocolLabels;
  TLabelList * SpaceAvailableLabels;
  TTabButton * SpaceAvailableTab;
  TFarText * HostKeyFingerprintLabel;
  TFarEdit * HostKeyFingerprintEdit;
  TFarText * InfoLabel;
  TFarSeparator * InfoSeparator;
  TFarLister * InfoLister;
  TFarEdit * SpaceAvailablePathEdit;
  TFarButton * OkButton;
};
//---------------------------------------------------------------------------
class TLabelList : public TList
{
public:
  TLabelList() :
    TList(), MaxLen(0)
  {
  }

  int MaxLen;
};
//---------------------------------------------------------------------------
__fastcall TFileSystemInfoDialog::TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
  TGetSpaceAvailable OnGetSpaceAvailable) : TTabbedDialog(AFarPlugin, tabCount),
  FOnGetSpaceAvailable(OnGetSpaceAvailable),
  FSpaceAvailableLoaded(false)
{
  TFarText * Text;
  TFarSeparator * Separator;
  TFarButton * Button;
  TTabButton * Tab;
  int GroupTop;

  Size = TPoint(73, 22);
  Caption = GetMsg(SERVER_PROTOCOL_INFORMATION);

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(SERVER_PROTOCOL_TAB_PROTOCOL);
  Tab->Tab = tabProtocol;

  NextItemPosition = ipRight;

  Tab = new TTabButton(this);
  Tab->TabName = GetMsg(SERVER_PROTOCOL_TAB_CAPABILITIES);
  Tab->Tab = tabCapabilities;

  SpaceAvailableTab = new TTabButton(this);
  SpaceAvailableTab->TabName = GetMsg(SERVER_PROTOCOL_TAB_SPACE_AVAILABLE);
  SpaceAvailableTab->Tab = tabSpaceAvailable;

  // Server tab

  NextItemPosition = ipNewLine;
  DefaultGroup = tabProtocol;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(SERVER_INFORMATION_GROUP);
  GroupTop = Separator->Top;

  ServerLabels = CreateLabelArray(5);

  new TFarSeparator(this);

  HostKeyFingerprintLabel = new TFarText(this);
  HostKeyFingerprintLabel->Caption = GetMsg(SERVER_HOST_KEY);
  HostKeyFingerprintEdit = new TFarEdit(this);
  HostKeyFingerprintEdit->ReadOnly = true;

  // Protocol tab

  DefaultGroup = tabCapabilities;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(PROTOCOL_INFORMATION_GROUP);
  Separator->Position = GroupTop;

  ProtocolLabels = CreateLabelArray(9);

  InfoSeparator = new TFarSeparator(this);
  InfoSeparator->Caption = GetMsg(PROTOCOL_INFO_GROUP);

  InfoLister = new TFarLister(this);
  InfoLister->Height = 4;
  InfoLister->Left = BorderBox->Left + 1;
  // Right edge is adjusted in FeedControls

  // Space available tab

  DefaultGroup = tabSpaceAvailable;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(SPACE_AVAILABLE_GROUP);
  Separator->Position = GroupTop;

  Text = new TFarText(this);
  Text->Caption = GetMsg(SPACE_AVAILABLE_PATH);

  NextItemPosition = ipRight;

  SpaceAvailablePathEdit = new TFarEdit(this);
  SpaceAvailablePathEdit->Right =
    - (GetMsg(SPACE_AVAILABLE_CHECK_SPACE).Length() + 11);

  Button = new TFarButton(this);
  Button->Caption = GetMsg(SPACE_AVAILABLE_CHECK_SPACE);
  Button->EnabledDependency = SpaceAvailablePathEdit;
  Button->OnClick = SpaceAvailableButtonClick;

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  SpaceAvailableLabels = CreateLabelArray(5);

  // Buttons

  DefaultGroup = 0;

  Separator = new TFarSeparator(this);
  Separator->Position = ClientRect.Bottom - 1;

  Button = new TFarButton(this);
  Button->Caption = GetMsg(SERVER_PROTOCOL_COPY_CLIPBOARD);
  Button->OnClick = ClipboardButtonClick;
  Button->CenterGroup = true;

  NextItemPosition = ipRight;

  OkButton = new TFarButton(this);
  OkButton->Caption = GetMsg(MSG_BUTTON_OK);
  OkButton->Default = true;
  OkButton->Result = brOK;
  OkButton->CenterGroup = true;
}
//---------------------------------------------------------------------------
TLabelList * __fastcall TFileSystemInfoDialog::CreateLabelArray(int Count)
{
  TLabelList * List = new TLabelList();
  try
  {
    for (int Index = 0; Index < Count; Index++)
    {
      List->Add(new TFarText(this));
    }
  }
  catch(...)
  {
    delete List;
    throw;
  }
  return List;
}
//---------------------------------------------------------------------
AnsiString __fastcall TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability)
{
  return BooleanToStr(FFileSystemInfo.IsCapable[Capability]);
}
//---------------------------------------------------------------------
AnsiString __fastcall TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability1,
  TFSCapability Capability2)
{
  return FORMAT("%s/%s", (CapabilityStr(Capability1), CapabilityStr(Capability2)));
}
//---------------------------------------------------------------------
AnsiString __fastcall TFileSystemInfoDialog::SpaceStr(__int64 Bytes)
{
  AnsiString Result;
  if (Bytes == 0)
  {
    Result = GetMsg(SPACE_AVAILABLE_BYTES_UNKNOWN);
  }
  else
  {
    Result = FormatBytes(Bytes);
    AnsiString SizeUnorderedStr = FormatBytes(Bytes, false);
    if (Result != SizeUnorderedStr)
    {
      Result = FORMAT("%s (%s)", (Result, SizeUnorderedStr));
    }
  }
  return Result;
}
//---------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::Feed(TFeedFileSystemData AddItem)
{
  AddItem(ServerLabels, SERVER_REMOTE_SYSTEM, FFileSystemInfo.RemoteSystem);
  AddItem(ServerLabels, SERVER_SESSION_PROTOCOL, FSessionInfo.ProtocolName);
  AddItem(ServerLabels, SERVER_SSH_IMPLEMENTATION, FSessionInfo.SshImplementation);

  AnsiString Str = FSessionInfo.CSCipher;
  if (FSessionInfo.CSCipher != FSessionInfo.SCCipher)
  {
    Str += FORMAT("/%s", (FSessionInfo.SCCipher));
  }
  AddItem(ServerLabels, SERVER_CIPHER, Str);

  Str = DefaultStr(FSessionInfo.CSCompression, LoadStr(NO_STR));
  if (FSessionInfo.CSCompression != FSessionInfo.SCCompression)
  {
    Str += FORMAT("/%s", (DefaultStr(FSessionInfo.SCCompression, LoadStr(NO_STR))));
  }
  AddItem(ServerLabels, SERVER_COMPRESSION, Str);
  if (FSessionInfo.ProtocolName != FFileSystemInfo.ProtocolName)
  {
    AddItem(ServerLabels, SERVER_FS_PROTOCOL, FFileSystemInfo.ProtocolName);
  }

  AddItem(HostKeyFingerprintEdit, 0, FSessionInfo.HostKeyFingerprint);

  AddItem(ProtocolLabels, PROTOCOL_MODE_CHANGING, CapabilityStr(fcModeChanging));
  AddItem(ProtocolLabels, PROTOCOL_OWNER_GROUP_CHANGING, CapabilityStr(fcGroupChanging));
  AnsiString AnyCommand;
  if (!FFileSystemInfo.IsCapable[fcShellAnyCommand] &&
      FFileSystemInfo.IsCapable[fcAnyCommand])
  {
    AnyCommand = GetMsg(PROTOCOL_PROTOCOL_ANY_COMMAND);
  }
  else
  {
    AnyCommand = CapabilityStr(fcAnyCommand);
  }
  AddItem(ProtocolLabels, PROTOCOL_ANY_COMMAND, AnyCommand);
  AddItem(ProtocolLabels, PROTOCOL_SYMBOLIC_HARD_LINK, CapabilityStr(fcSymbolicLink, fcHardLink));
  AddItem(ProtocolLabels, PROTOCOL_USER_GROUP_LISTING, CapabilityStr(fcUserGroupListing));
  AddItem(ProtocolLabels, PROTOCOL_REMOTE_COPY, CapabilityStr(fcRemoteCopy));
  AddItem(ProtocolLabels, PROTOCOL_CHECKING_SPACE_AVAILABLE, CapabilityStr(fcCheckingSpaceAvailable));
  AddItem(ProtocolLabels, PROTOCOL_CALCULATING_CHECKSUM, CapabilityStr(fcCalculatingChecksum));
  AddItem(ProtocolLabels, PROTOCOL_NATIVE_TEXT_MODE, CapabilityStr(fcNativeTextMode));

  AddItem(InfoLister, 0, FFileSystemInfo.AdditionalInfo);

  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.BytesOnDevice));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.UnusedBytesOnDevice));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.BytesAvailableToUser));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.UnusedBytesAvailableToUser));
  AddItem(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_PER_ALLOCATION_UNIT, SpaceStr(FSpaceAvailable.BytesPerAllocationUnit));
}
//---------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::ControlsAddItem(TObject * Control,
  int Label, AnsiString Value)
{
  if (FLastFeededControl != Control)
  {
    FLastFeededControl = Control;
    FLastListItem = 0;
  }

  if (Control == HostKeyFingerprintEdit)
  {
    HostKeyFingerprintEdit->Text = Value;
    HostKeyFingerprintEdit->Enabled = !Value.IsEmpty();
    if (!HostKeyFingerprintEdit->Enabled)
    {
      HostKeyFingerprintEdit->Visible = false;
      HostKeyFingerprintEdit->Group = 0;
      HostKeyFingerprintLabel->Visible = false;
      HostKeyFingerprintLabel->Group = 0;
    }
  }
  else if (Control == InfoLister)
  {
    InfoLister->Items->Text = Value;
    InfoLister->Enabled = !Value.IsEmpty();
    if (!InfoLister->Enabled)
    {
      InfoLister->Visible = false;
      InfoLister->Group = 0;
      InfoSeparator->Visible = false;
      InfoSeparator->Group = 0;
    }
  }
  else
  {
    TLabelList * List = dynamic_cast<TLabelList *>(Control);
    assert(List != NULL);
    if (!Value.IsEmpty())
    {
      TFarText * Text = reinterpret_cast<TFarText *>(List->Items[FLastListItem]);
      FLastListItem++;

      Text->Caption = FORMAT("%-*s  %s", (List->MaxLen, GetMsg(Label), (Value)));
    }
  }
}
//---------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::CalculateMaxLenAddItem(TObject * Control,
  int Label, AnsiString Value)
{
  TLabelList * List = dynamic_cast<TLabelList *>(Control);
  if (List != NULL)
  {
    AnsiString S = GetMsg(Label);
    if (List->MaxLen < S.Length())
    {
      List->MaxLen = S.Length();
    }
  }
}
//---------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::ClipboardAddItem(TObject * AControl,
  int Label, AnsiString Value)
{
  TFarDialogItem * Control = dynamic_cast<TFarDialogItem *>(AControl);
  // check for Enabled instead of Visible, as Visible is false
  // when control is on non-active tab
  if (!Value.IsEmpty() &&
      ((Control == NULL) || Control->Enabled) &&
      (AControl != SpaceAvailableLabels) ||
       SpaceAvailableSupported())
  {
    if (FLastFeededControl != AControl)
    {
      if (FLastFeededControl != NULL)
      {
        FClipboard += AnsiString::StringOfChar('-', 60) + "\r\n";
      }
      FLastFeededControl = AControl;
    }

    if (dynamic_cast<TLabelList *>(AControl) == NULL)
    {
      AnsiString LabelStr;
      if (Control == HostKeyFingerprintEdit)
      {
        LabelStr = GetMsg(SERVER_HOST_KEY);
      }
      else if (Control == InfoLister)
      {
        LabelStr = GetMsg(PROTOCOL_INFO_GROUP).Trim();
      }
      else
      {
        assert(false);
      }

      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == ':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }

      if ((Value.Length() >= 2) && (Value.SubString(Value.Length() - 1, 2) == "\r\n"))
      {
        Value.SetLength(Value.Length() - 2);
      }

      FClipboard += FORMAT("%s\r\n%s\r\n", (LabelStr, Value));
    }
    else
    {
      assert(dynamic_cast<TLabelList *>(AControl) != NULL);
      AnsiString LabelStr = GetMsg(Label);
      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == ':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }
      FClipboard += FORMAT("%s = %s\r\n", (LabelStr, Value));
    }
  }
}
//---------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::FeedControls()
{
  FLastFeededControl = NULL;
  Feed(ControlsAddItem);
  InfoLister->Right = BorderBox->Right - (InfoLister->ScrollBar ? 0 : 1);
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::SelectTab(int Tab)
{
  TTabbedDialog::SelectTab(Tab);
  if (InfoLister->Visible)
  {
    // At first the dialog border box hides the eventual scrollbar of infolister,
    // so redraw to reshow it.
    Redraw();
  }

  if (Tab == tabSpaceAvailable)
  {
    NeedSpaceAvailable();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::Execute(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  AnsiString SpaceAvailablePath)
{
  FFileSystemInfo = FileSystemInfo;
  FSessionInfo = SessionInfo;
  SpaceAvailablePathEdit->Text = SpaceAvailablePath;
  UpdateControls();

  Feed(CalculateMaxLenAddItem);
  FeedControls();
  HideTabs();
  SelectTab(tabProtocol);

  ShowModal();
}
//---------------------------------------------------------------------------
bool __fastcall TFileSystemInfoDialog::Key(TFarDialogItem * Item, long KeyCode)
{
  bool Result;
  if ((Item == SpaceAvailablePathEdit) && (KeyCode == KEY_ENTER))
  {
    CheckSpaceAvailable();
    Result = true;
  }
  else
  {
    Result = TTabbedDialog::Key(Item, KeyCode);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::Change()
{
  TTabbedDialog::Change();

  if (Handle)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::UpdateControls()
{
  SpaceAvailableTab->Enabled = SpaceAvailableSupported();
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::ClipboardButtonClick(TFarButton * /*Sender*/,
  bool & Close)
{
  NeedSpaceAvailable();
  FLastFeededControl = NULL;
  FClipboard = "";
  Feed(ClipboardAddItem);
  FarPlugin->FarCopyToClipboard(FClipboard);
  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::SpaceAvailableButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CheckSpaceAvailable();
  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::CheckSpaceAvailable()
{
  assert(FOnGetSpaceAvailable != NULL);
  assert(!SpaceAvailablePathEdit->Text.IsEmpty());

  FSpaceAvailableLoaded = true;

  bool DoClose = false;

  FOnGetSpaceAvailable(SpaceAvailablePathEdit->Text, FSpaceAvailable, DoClose);

  FeedControls();
  if (DoClose)
  {
    Close(OkButton);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFileSystemInfoDialog::NeedSpaceAvailable()
{
  if (!FSpaceAvailableLoaded && SpaceAvailableSupported())
  {
    CheckSpaceAvailable();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TFileSystemInfoDialog::SpaceAvailableSupported()
{
  return (FOnGetSpaceAvailable != NULL);
}
//---------------------------------------------------------------------------
void __fastcall TWinSCPFileSystem::FileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  AnsiString SpaceAvailablePath, TGetSpaceAvailable OnGetSpaceAvailable)
{
  TFileSystemInfoDialog * Dialog = new TFileSystemInfoDialog(FPlugin, OnGetSpaceAvailable);
  try
  {
    Dialog->Execute(SessionInfo, FileSystemInfo, SpaceAvailablePath);
  }
  __finally
  {
    delete Dialog;
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::OpenDirectoryDialog(
  bool Add, AnsiString & Directory, TBookmarkList * BookmarkList)
{
  bool Result;
  bool Repeat;

  int ItemFocused = -1;

  do
  {
    TStrings * BookmarkPaths = new TStringList();
    TFarMenuItems * BookmarkItems = new TFarMenuItems();
    TList * Bookmarks = new TList();
    try
    {
      int BookmarksOffset = -1;

      int MaxLength = FPlugin->MaxMenuItemLength();
      int MaxHistory = 40;
      int FirstHistory = 0;

      if (FPathHistory->Count > MaxHistory)
      {
        FirstHistory = FPathHistory->Count - MaxHistory + 1;
      }

      for (int i = FirstHistory; i < FPathHistory->Count; i++)
      {
        AnsiString Path = FPathHistory->Strings[i];
        BookmarkPaths->Add(Path);
        BookmarkItems->Add(MinimizeName(Path, MaxLength, true));
      }

      int FirstItemFocused = -1;
      TStringList * BookmarkDirectories = new TStringList();
      try
      {
        BookmarkDirectories->Sorted = true;
        for (int i = 0; i < BookmarkList->Count; i++)
        {
          TBookmark * Bookmark = BookmarkList->Bookmarks[i];
          AnsiString RemoteDirectory = Bookmark->Remote;
          if (!RemoteDirectory.IsEmpty() && (BookmarkDirectories->IndexOf(RemoteDirectory) < 0))
          {
            int Pos;
            Pos = BookmarkDirectories->Add(RemoteDirectory);
            if (RemoteDirectory == Directory)
            {
              FirstItemFocused = Pos;
            }
            else if ((FirstItemFocused >= 0) && (FirstItemFocused >= Pos))
            {
              FirstItemFocused++;
            }
            Bookmarks->Insert(Pos, Bookmark);
          }
        }

        if (BookmarkDirectories->Count == 0)
        {
          FirstItemFocused = BookmarkItems->Add("");
          BookmarkPaths->Add("");
          BookmarksOffset = BookmarkItems->Count;
        }
        else
        {
          if (BookmarkItems->Count > 0)
          {
            BookmarkItems->AddSeparator();
            BookmarkPaths->Add("");
          }

          BookmarksOffset = BookmarkItems->Count;

          if (FirstItemFocused >= 0)
          {
            FirstItemFocused += BookmarkItems->Count;
          }
          else
          {
            FirstItemFocused = BookmarkItems->Count;
          }

          for (int ii = 0; ii < BookmarkDirectories->Count; ii++)
          {
            AnsiString Path = BookmarkDirectories->Strings[ii];
            BookmarkItems->Add(Path);
            BookmarkPaths->Add(MinimizeName(Path, MaxLength, true));
          }
        }
      }
      __finally
      {
        delete BookmarkDirectories;
      }

      if (ItemFocused < 0)
      {
        BookmarkItems->ItemFocused = FirstItemFocused;
      }
      else if (ItemFocused < BookmarkItems->Count)
      {
        BookmarkItems->ItemFocused = ItemFocused;
      }
      else
      {
        BookmarkItems->ItemFocused = BookmarkItems->Count - 1;
      }

      int BreakCode;

      Repeat = false;
      AnsiString Caption = GetMsg(Add ? OPEN_DIRECTORY_ADD_BOOMARK_ACTION :
        OPEN_DIRECTORY_BROWSE_CAPTION);
      const int BreakKeys[] = { VK_DELETE, VK_F8, VK_RETURN + (PKF_CONTROL << 16),
        'C' + (PKF_CONTROL << 16), VK_INSERT + (PKF_CONTROL << 16), 0 };

      ItemFocused = FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
        Caption, GetMsg(OPEN_DIRECTORY_HELP), BookmarkItems, BreakKeys, BreakCode);
      if (BreakCode >= 0)
      {
        assert(BreakCode >= 0 && BreakCode <= 4);
        if ((BreakCode == 0) || (BreakCode == 1))
        {
          assert(ItemFocused >= 0);
          if (ItemFocused >= BookmarksOffset)
          {
            TBookmark * Bookmark = static_cast<TBookmark *>(Bookmarks->Items[ItemFocused - BookmarksOffset]);
            BookmarkList->Delete(Bookmark);
          }
          else
          {
            FPathHistory->Clear();
            ItemFocused = -1;
          }
          Repeat = true;
        }
        else if (BreakCode == 2)
        {
          FarControl(FCTL_INSERTCMDLINE, BookmarkPaths->Strings[ItemFocused].c_str());
        }
        else if (BreakCode == 3 || BreakCode == 4)
        {
          FPlugin->FarCopyToClipboard(BookmarkPaths->Strings[ItemFocused]);
          Repeat = true;
        }
      }
      else if (ItemFocused >= 0)
      {
        Directory = BookmarkPaths->Strings[ItemFocused];
        if (Directory.IsEmpty())
        {
          // empty trailing line in no-bookmark mode selected
          ItemFocused = -1;
        }
      }

      Result = (BreakCode < 0) && (ItemFocused >= 0);
    }
    __finally
    {
      delete BookmarkItems;
      delete Bookmarks;
      delete BookmarkPaths;
    }
  }
  while (Repeat);

  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TApplyCommandDialog : public TWinSCPDialog
{
public:
  __fastcall TApplyCommandDialog(TCustomFarPlugin * AFarPlugin);

  bool __fastcall Execute(AnsiString & Command, int & Params);

protected:
  virtual void __fastcall Change();

private:
  int FParams;

  TFarEdit * CommandEdit;
  TFarText * LocalHintText;
  TFarRadioButton * RemoteCommandButton;
  TFarRadioButton * LocalCommandButton;
  TFarCheckBox * ApplyToDirectoriesCheck;
  TFarCheckBox * RecursiveCheck;
  TFarCheckBox * ShowResultsCheck;
  TFarCheckBox * CopyResultsCheck;

  AnsiString FPrompt;
  TFarEdit * PasswordEdit;
  TFarEdit * NormalEdit;
  TFarCheckBox * HideTypingCheck;
};
//---------------------------------------------------------------------------
__fastcall TApplyCommandDialog::TApplyCommandDialog(TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin)
{
  TFarText * Text;

  Size = TPoint(76, 18);
  Caption = GetMsg(APPLY_COMMAND_TITLE);

  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_PROMPT);

  CommandEdit = new TFarEdit(this);
  CommandEdit->History = APPLY_COMMAND_HISTORY;

  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_HINT1);
  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_HINT2);
  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_HINT3);
  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_HINT4);
  Text = new TFarText(this);
  Text->Caption = GetMsg(APPLY_COMMAND_HINT5);
  LocalHintText = new TFarText(this);
  LocalHintText->Caption = GetMsg(APPLY_COMMAND_HINT_LOCAL);

  new TFarSeparator(this);

  RemoteCommandButton = new TFarRadioButton(this);
  RemoteCommandButton->Caption = GetMsg(APPLY_COMMAND_REMOTE_COMMAND);

  NextItemPosition = ipRight;

  LocalCommandButton = new TFarRadioButton(this);
  LocalCommandButton->Caption = GetMsg(APPLY_COMMAND_LOCAL_COMMAND);

  LocalHintText->EnabledDependency = LocalCommandButton;

  NextItemPosition = ipNewLine;

  ApplyToDirectoriesCheck = new TFarCheckBox(this);
  ApplyToDirectoriesCheck->Caption =
    GetMsg(APPLY_COMMAND_APPLY_TO_DIRECTORIES);

  NextItemPosition = ipRight;

  RecursiveCheck = new TFarCheckBox(this);
  RecursiveCheck->Caption = GetMsg(APPLY_COMMAND_RECURSIVE);

  NextItemPosition = ipNewLine;

  ShowResultsCheck = new TFarCheckBox(this);
  ShowResultsCheck->Caption = GetMsg(APPLY_COMMAND_SHOW_RESULTS);
  ShowResultsCheck->EnabledDependency = RemoteCommandButton;

  NextItemPosition = ipRight;

  CopyResultsCheck = new TFarCheckBox(this);
  CopyResultsCheck->Caption = GetMsg(APPLY_COMMAND_COPY_RESULTS);
  CopyResultsCheck->EnabledDependency = RemoteCommandButton;

  AddStandardButtons();

  OkButton->EnabledDependency = CommandEdit;
}
//---------------------------------------------------------------------------
void __fastcall TApplyCommandDialog::Change()
{
  TWinSCPDialog::Change();

  if (Handle)
  {
    bool RemoteCommand = RemoteCommandButton->Checked;
    bool AllowRecursive = true;
    bool AllowApplyToDirectories = true;
    try
    {
      TRemoteCustomCommand RemoteCustomCommand;
      TLocalCustomCommand LocalCustomCommand;
      TFileCustomCommand * FileCustomCommand =
        (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

      TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);
      AnsiString Cmd = InteractiveCustomCommand.Complete(CommandEdit->Text, false);
      bool FileCommand = FileCustomCommand->IsFileCommand(Cmd);
      AllowRecursive = FileCommand && !FileCustomCommand->IsFileListCommand(Cmd);
      if (AllowRecursive && !RemoteCommand)
      {
        AllowRecursive = !LocalCustomCommand.HasLocalFileName(Cmd);
      }
      AllowApplyToDirectories = FileCommand;
    }
    catch(...)
    {
    }

    RecursiveCheck->Enabled = AllowRecursive;
    ApplyToDirectoriesCheck->Enabled = AllowApplyToDirectories;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TApplyCommandDialog::Execute(AnsiString & Command, int & Params)
{
  CommandEdit->Text = Command;
  FParams = Params;
  RemoteCommandButton->Checked = FLAGCLEAR(Params, ccLocal);
  LocalCommandButton->Checked = FLAGSET(Params, ccLocal);
  ApplyToDirectoriesCheck->Checked = FLAGSET(Params, ccApplyToDirectories);
  RecursiveCheck->Checked = FLAGSET(Params, ccRecursive);
  ShowResultsCheck->Checked = FLAGSET(Params, ccShowResults);
  CopyResultsCheck->Checked = FLAGSET(Params, ccCopyResults);

  bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    Command = CommandEdit->Text;
    Params &= ~(ccLocal | ccApplyToDirectories | ccRecursive | ccShowResults | ccCopyResults);
    Params |=
      FLAGMASK(!RemoteCommandButton->Checked, ccLocal) |
      FLAGMASK(ApplyToDirectoriesCheck->Checked, ccApplyToDirectories) |
      FLAGMASK(RecursiveCheck->Checked && RecursiveCheck->Enabled, ccRecursive) |
      FLAGMASK(ShowResultsCheck->Checked && ShowResultsCheck->Enabled, ccShowResults) |
      FLAGMASK(CopyResultsCheck->Checked && CopyResultsCheck->Enabled, ccCopyResults);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::ApplyCommandDialog(AnsiString & Command,
  int & Params)
{
  bool Result;
  TApplyCommandDialog * Dialog = new TApplyCommandDialog(FPlugin);
  try
  {
    Result = Dialog->Execute(Command, Params);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TFullSynchronizeDialog : public TWinSCPDialog
{
public:
  __fastcall TFullSynchronizeDialog(TCustomFarPlugin * AFarPlugin, int Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);

  bool __fastcall Execute(TTerminal::TSynchronizeMode & Mode,
    int & Params, AnsiString & LocalDirectory, AnsiString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode);

protected:
  virtual bool __fastcall CloseQuery();
  virtual void __fastcall Change();
  virtual long __fastcall DialogProc(int Msg, int Param1, long Param2);

  void __fastcall TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);

  int __fastcall ActualCopyParamAttrs();
  void __fastcall CustomCopyParam();
  void __fastcall AdaptSize();

private:
  TFarEdit * LocalDirectoryEdit;
  TFarEdit * RemoteDirectoryEdit;
  TFarRadioButton * SynchronizeBothButton;
  TFarRadioButton * SynchronizeRemoteButton;
  TFarRadioButton * SynchronizeLocalButton;
  TFarRadioButton * SynchronizeFilesButton;
  TFarRadioButton * MirrorFilesButton;
  TFarRadioButton * SynchronizeTimestampsButton;
  TFarCheckBox * SynchronizeDeleteCheck;
  TFarCheckBox * SynchronizeExistingOnlyCheck;
  TFarCheckBox * SynchronizeSelectedOnlyCheck;
  TFarCheckBox * SynchronizePreviewChangesCheck;
  TFarCheckBox * SynchronizeByTimeCheck;
  TFarCheckBox * SynchronizeBySizeCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarLister * CopyParamLister;

  bool FSaveMode;
  int FOptions;
  int FFullHeight;
  TTerminal::TSynchronizeMode FOrigMode;
  TUsableCopyParamAttrs FCopyParamAttrs;
  TCopyParamType FCopyParams;

  TTerminal::TSynchronizeMode __fastcall GetMode();
};
//---------------------------------------------------------------------------
__fastcall TFullSynchronizeDialog::TFullSynchronizeDialog(
  TCustomFarPlugin * AFarPlugin, int Options,
  const TUsableCopyParamAttrs & CopyParamAttrs) :
  TWinSCPDialog(AFarPlugin)
{
  FOptions = Options;
  FCopyParamAttrs = CopyParamAttrs;

  TFarText * Text;
  TFarSeparator * Separator;

  Size = TPoint(78, 25);
  Caption = GetMsg(FULL_SYNCHRONIZE_TITLE);

  Text = new TFarText(this);
  Text->Caption = GetMsg(FULL_SYNCHRONIZE_LOCAL_LABEL);

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->History = LOCAL_SYNC_HISTORY;

  Text = new TFarText(this);
  Text->Caption = GetMsg(FULL_SYNCHRONIZE_REMOTE_LABEL);

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->History = REMOTE_SYNC_HISTORY;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(FULL_SYNCHRONIZE_DIRECTION_GROUP);

  SynchronizeBothButton = new TFarRadioButton(this);
  SynchronizeBothButton->Caption = GetMsg(FULL_SYNCHRONIZE_BOTH);

  NextItemPosition = ipRight;

  SynchronizeRemoteButton = new TFarRadioButton(this);
  SynchronizeRemoteButton->Caption = GetMsg(FULL_SYNCHRONIZE_REMOTE);

  SynchronizeLocalButton = new TFarRadioButton(this);
  SynchronizeLocalButton->Caption = GetMsg(FULL_SYNCHRONIZE_LOCAL);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(FULL_SYNCHRONIZE_MODE_GROUP);

  SynchronizeFilesButton = new TFarRadioButton(this);
  SynchronizeFilesButton->Caption = GetMsg(SYNCHRONIZE_SYNCHRONIZE_FILES);

  NextItemPosition = ipRight;

  MirrorFilesButton = new TFarRadioButton(this);
  MirrorFilesButton->Caption = GetMsg(SYNCHRONIZE_MIRROR_FILES);

  SynchronizeTimestampsButton = new TFarRadioButton(this);
  SynchronizeTimestampsButton->Caption = GetMsg(SYNCHRONIZE_SYNCHRONIZE_TIMESTAMPS);
  SynchronizeTimestampsButton->Enabled = FLAGCLEAR(Options, fsoDisableTimestamp);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(FULL_SYNCHRONIZE_GROUP);

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->Caption = GetMsg(SYNCHRONIZE_DELETE);

  NextItemPosition = ipRight;

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->Caption = GetMsg(SYNCHRONIZE_EXISTING_ONLY);
  SynchronizeExistingOnlyCheck->EnabledDependencyNegative = SynchronizeTimestampsButton;

  NextItemPosition = ipNewLine;

  SynchronizePreviewChangesCheck = new TFarCheckBox(this);
  SynchronizePreviewChangesCheck->Caption = GetMsg(SYNCHRONIZE_PREVIEW_CHANGES);

  NextItemPosition = ipRight;

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->Caption = GetMsg(SYNCHRONIZE_SELECTED_ONLY);
  SynchronizeSelectedOnlyCheck->Enabled = FLAGSET(FOptions, fsoAllowSelectedOnly);

  NextItemPosition = ipNewLine;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(FULL_SYNCHRONIZE_CRITERIONS_GROUP);

  SynchronizeByTimeCheck = new TFarCheckBox(this);
  SynchronizeByTimeCheck->Caption = GetMsg(SYNCHRONIZE_BY_TIME);

  NextItemPosition = ipRight;

  SynchronizeBySizeCheck = new TFarCheckBox(this);
  SynchronizeBySizeCheck->Caption = GetMsg(SYNCHRONIZE_BY_SIZE);
  SynchronizeBySizeCheck->EnabledDependencyNegative = SynchronizeBothButton;

  NextItemPosition = ipNewLine;

  new TFarSeparator(this);

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->Caption = GetMsg(SYNCHRONIZE_REUSE_SETTINGS);

  Separator = new TFarSeparator(this);
  Separator->Group = 1;
  Separator->Caption = GetMsg(COPY_PARAM_GROUP);

  CopyParamLister = new TFarLister(this);
  CopyParamLister->Height = 3;
  CopyParamLister->Left = BorderBox->Left + 1;
  CopyParamLister->TabStop = false;
  CopyParamLister->OnMouseClick = CopyParamListerClick;
  CopyParamLister->Group = 1;
  // Right edge is adjusted in Change

  // align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->Position = -4;

  TFarButton * Button = new TFarButton(this);
  Button->Caption = GetMsg(TRANSFER_SETTINGS_BUTTON);
  Button->Result = -1;
  Button->CenterGroup = true;
  Button->OnClick = TransferSettingsButtonClick;

  NextItemPosition = ipRight;

  AddStandardButtons(0, true);

  FFullHeight = Size.y;
  AdaptSize();
}
//---------------------------------------------------------------------------
void __fastcall TFullSynchronizeDialog::AdaptSize()
{
  bool ShowCopyParam = (FFullHeight <= MaxSize.y);
  if (ShowCopyParam != CopyParamLister->Visible)
  {
    ShowGroup(1, ShowCopyParam);
    Height = FFullHeight - (ShowCopyParam ? 0 : CopyParamLister->Height + 1);
  }
}
//---------------------------------------------------------------------------
TTerminal::TSynchronizeMode __fastcall TFullSynchronizeDialog::GetMode()
{
  TTerminal::TSynchronizeMode Mode;

  if (SynchronizeRemoteButton->Checked)
  {
    Mode = TTerminal::smRemote;
  }
  else if (SynchronizeLocalButton->Checked)
  {
    Mode = TTerminal::smLocal;
  }
  else
  {
    Mode = TTerminal::smBoth;
  }

  return Mode;
}
//---------------------------------------------------------------------------
void __fastcall TFullSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TFullSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFullSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, ActualCopyParamAttrs()))
  {
    Change();
  }
}
//---------------------------------------------------------------------------
void __fastcall TFullSynchronizeDialog::Change()
{
  TWinSCPDialog::Change();

  if (Handle)
  {
    if (SynchronizeTimestampsButton->Checked)
    {
      SynchronizeExistingOnlyCheck->Checked = true;
      SynchronizeDeleteCheck->Checked = false;
      SynchronizeByTimeCheck->Checked = true;
    }
    if (SynchronizeBothButton->Checked)
    {
      SynchronizeBySizeCheck->Checked = false;
      if (MirrorFilesButton->Checked)
      {
        SynchronizeFilesButton->Checked = true;
      }
    }
    if (MirrorFilesButton->Checked)
    {
      SynchronizeByTimeCheck->Checked = true;
    }
    MirrorFilesButton->Enabled = !SynchronizeBothButton->Checked;
    SynchronizeDeleteCheck->Enabled = !SynchronizeBothButton->Checked &&
      !SynchronizeTimestampsButton->Checked;
    SynchronizeByTimeCheck->Enabled = !SynchronizeBothButton->Checked &&
      !SynchronizeTimestampsButton->Checked && !MirrorFilesButton->Checked;
    SynchronizeBySizeCheck->Caption = SynchronizeTimestampsButton->Checked ?
      GetMsg(SYNCHRONIZE_SAME_SIZE) : GetMsg(SYNCHRONIZE_BY_SIZE);

    if (!SynchronizeBySizeCheck->Checked && !SynchronizeByTimeCheck->Checked)
    {
      // suppose that in FAR the checkbox cannot be unchecked unless focused
      if (SynchronizeByTimeCheck->Focused())
      {
        SynchronizeBySizeCheck->Checked = true;
      }
      else
      {
        SynchronizeByTimeCheck->Checked = true;
      }
    }

    AnsiString InfoStr = FCopyParams.GetInfoStr("; ", ActualCopyParamAttrs());
    TStringList * InfoStrLines = new TStringList();
    try
    {
      FarWrapText(InfoStr, InfoStrLines, BorderBox->Width - 4);
      CopyParamLister->Items = InfoStrLines;
      CopyParamLister->Right = BorderBox->Right - (CopyParamLister->ScrollBar ? 0 : 1);
    }
    __finally
    {
      delete InfoStrLines;
    }
  }
}
//---------------------------------------------------------------------------
int __fastcall TFullSynchronizeDialog::ActualCopyParamAttrs()
{
  int Result;
  if (SynchronizeTimestampsButton->Checked)
  {
    Result = cpaExcludeMaskOnly;
  }
  else
  {
    switch (GetMode())
    {
      case TTerminal::smRemote:
        Result = FCopyParamAttrs.Upload;
        break;

      case TTerminal::smLocal:
        Result = FCopyParamAttrs.Download;
        break;

      default:
        assert(false);
        //fallthru
      case TTerminal::smBoth:
        Result = FCopyParamAttrs.General;
        break;
    }
  }
  return Result | cpaNoPreserveTime;
}
//---------------------------------------------------------------------------
bool __fastcall TFullSynchronizeDialog::CloseQuery()
{
  bool CanClose = TWinSCPDialog::CloseQuery();

  if (CanClose && (Result == brOK) &&
      SaveSettingsCheck->Checked && (FOrigMode != GetMode()) && !FSaveMode)
  {
    TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);

    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(SAVE_SYNCHRONIZE_MODE), NULL,
            qtConfirmation, qaYes | qaNo | qaCancel, 0))
    {
      case qaYes:
        FSaveMode = true;
        break;

      case qaCancel:
        CanClose = false;
        break;
    }
  }

  return CanClose;
}
//---------------------------------------------------------------------------
long __fastcall TFullSynchronizeDialog::DialogProc(int Msg, int Param1, long Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
bool __fastcall TFullSynchronizeDialog::Execute(TTerminal::TSynchronizeMode & Mode,
  int & Params, AnsiString & LocalDirectory, AnsiString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode)
{
  LocalDirectoryEdit->Text = LocalDirectory;
  RemoteDirectoryEdit->Text = RemoteDirectory;
  SynchronizeRemoteButton->Checked = (Mode == TTerminal::smRemote);
  SynchronizeLocalButton->Checked = (Mode == TTerminal::smLocal);
  SynchronizeBothButton->Checked = (Mode == TTerminal::smBoth);
  SynchronizeDeleteCheck->Checked = FLAGSET(Params, TTerminal::spDelete);
  SynchronizeExistingOnlyCheck->Checked = FLAGSET(Params, TTerminal::spExistingOnly);
  SynchronizePreviewChangesCheck->Checked = FLAGSET(Params, TTerminal::spPreviewChanges);
  SynchronizeSelectedOnlyCheck->Checked = FLAGSET(Params, spSelectedOnly);
  if (FLAGSET(Params, TTerminal::spTimestamp) && FLAGCLEAR(FOptions, fsoDisableTimestamp))
  {
    SynchronizeTimestampsButton->Checked = true;
  }
  else if (FLAGSET(Params, TTerminal::spMirror))
  {
    MirrorFilesButton->Checked = true;
  }
  else
  {
    SynchronizeFilesButton->Checked = true;
  }
  SynchronizeByTimeCheck->Checked = FLAGCLEAR(Params, TTerminal::spNotByTime);
  SynchronizeBySizeCheck->Checked = FLAGSET(Params, TTerminal::spBySize);
  SaveSettingsCheck->Checked = SaveSettings;
  FSaveMode = SaveMode;
  FOrigMode = Mode;
  FCopyParams = *CopyParams;

  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    RemoteDirectory = RemoteDirectoryEdit->Text;
    LocalDirectory = LocalDirectoryEdit->Text;

    Mode = GetMode();

    Params &= ~(TTerminal::spDelete | TTerminal::spNoConfirmation |
      TTerminal::spExistingOnly | TTerminal::spPreviewChanges |
      TTerminal::spTimestamp | TTerminal::spNotByTime | TTerminal::spBySize |
      spSelectedOnly | TTerminal::spMirror);
    Params |=
      FLAGMASK(SynchronizeDeleteCheck->Checked, TTerminal::spDelete) |
      FLAGMASK(SynchronizeExistingOnlyCheck->Checked, TTerminal::spExistingOnly) |
      FLAGMASK(SynchronizePreviewChangesCheck->Checked, TTerminal::spPreviewChanges) |
      FLAGMASK(SynchronizeSelectedOnlyCheck->Checked, spSelectedOnly) |
      FLAGMASK(SynchronizeTimestampsButton->Checked && FLAGCLEAR(FOptions, fsoDisableTimestamp),
        TTerminal::spTimestamp) |
      FLAGMASK(MirrorFilesButton->Checked, TTerminal::spMirror) |
      FLAGMASK(!SynchronizeByTimeCheck->Checked, TTerminal::spNotByTime) |
      FLAGMASK(SynchronizeBySizeCheck->Checked, TTerminal::spBySize);

    SaveSettings = SaveSettingsCheck->Checked;
    SaveMode = FSaveMode;
    *CopyParams = FCopyParams;
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
  int & Params, AnsiString & LocalDirectory, AnsiString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
  const TUsableCopyParamAttrs & CopyParamAttrs)
{
  bool Result;
  TFullSynchronizeDialog * Dialog = new TFullSynchronizeDialog(
    FPlugin, Options, CopyParamAttrs);
  try
  {
    Result = Dialog->Execute(Mode, Params, LocalDirectory, RemoteDirectory,
      CopyParams, SaveSettings, SaveMode);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSynchronizeChecklistDialog : public TWinSCPDialog
{
public:
  __fastcall TSynchronizeChecklistDialog(
    TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode Mode, int Params,
    const AnsiString LocalDirectory, const AnsiString RemoteDirectory);

  bool __fastcall Execute(TSynchronizeChecklist * Checklist);

protected:
  virtual long __fastcall DialogProc(int Msg, int Param1, long Param2);
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  void __fastcall CheckAllButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall VideoModeButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall ListBoxClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);

private:
  TFarText * Header;
  TFarListBox * ListBox;
  TFarButton * CheckAllButton;
  TFarButton * UncheckAllButton;
  TFarButton * VideoModeButton;

  TSynchronizeChecklist * FChecklist;
  AnsiString FLocalDirectory;
  AnsiString FRemoteDirectory;
  static const size_t FColumns = 8;
  int FWidths[FColumns];
  AnsiString FActions[TSynchronizeChecklist::ActionCount];
  int FScroll;
  bool FCanScrollRight;
  int FChecked;

  void __fastcall AdaptSize();
  int __fastcall ColumnWidth(int Index);
  void __fastcall LoadChecklist();
  void __fastcall RefreshChecklist(bool Scroll);
  void __fastcall UpdateControls();
  void __fastcall CheckAll(bool Check);
  AnsiString __fastcall ItemLine(const TSynchronizeChecklist::TItem * ChecklistItem);
  void AddColumn(AnsiString & List, AnsiString Value, int Width,
    bool Header = false);
  AnsiString __fastcall FormatSize(__int64 Size, int Column);
};
//---------------------------------------------------------------------------
__fastcall TSynchronizeChecklistDialog::TSynchronizeChecklistDialog(
  TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode /*Mode*/, int /*Params*/,
  const AnsiString LocalDirectory, const AnsiString RemoteDirectory) :
  TWinSCPDialog(AFarPlugin),
  FChecklist(NULL),
  FLocalDirectory(LocalDirectory),
  FRemoteDirectory(RemoteDirectory),
  FScroll(0),
  FCanScrollRight(false)
{
  Caption = GetMsg(CHECKLIST_TITLE);

  Header = new TFarText(this);
  Header->Oem = true;

  ListBox = new TFarListBox(this);
  ListBox->NoBox = true;
  // align list with bottom of the window
  ListBox->Bottom = -5;
  ListBox->OnMouseClick = ListBoxClick;
  ListBox->Oem = true;

  AnsiString Actions = GetMsg(CHECKLIST_ACTIONS);
  int Action = 0;
  while (!Actions.IsEmpty() && (Action < LENOF(FActions)))
  {
    FActions[Action] = CutToChar(Actions, '|', false);
    Action++;
  }

  // align buttons with bottom of the window
  ButtonSeparator = new TFarSeparator(this);
  ButtonSeparator->Top = -4;
  ButtonSeparator->Bottom = ButtonSeparator->Top;

  CheckAllButton = new TFarButton(this);
  CheckAllButton->Caption = GetMsg(CHECKLIST_CHECK_ALL);
  CheckAllButton->CenterGroup = true;
  CheckAllButton->OnClick = CheckAllButtonClick;

  NextItemPosition = ipRight;

  UncheckAllButton = new TFarButton(this);
  UncheckAllButton->Caption = GetMsg(CHECKLIST_UNCHECK_ALL);
  UncheckAllButton->CenterGroup = true;
  UncheckAllButton->OnClick = CheckAllButtonClick;

  VideoModeButton = new TFarButton(this);
  VideoModeButton->CenterGroup = true;
  VideoModeButton->OnClick = VideoModeButtonClick;

  AddStandardButtons(0, true);

  AdaptSize();
  UpdateControls();
  ListBox->SetFocus();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::AddColumn(AnsiString & List,
  AnsiString Value, int Column, bool Header)
{
  // OEM character set (Ansi does not have the ascii art we need)
  char Separator = '\xB3';
  StrToFar(Value);
  int Len = Value.Length();
  int Width = FWidths[Column];
  bool Right = (Column == 2) || (Column == 3) || (Column == 6) || (Column == 7);
  bool LastCol = (Column == FColumns - 1);
  if (Len <= Width)
  {
    int Added = 0;
    if (Header && (Len < Width))
    {
      Added += (Width - Len) / 2;
    }
    else if (Right && (Len < Width))
    {
      Added += Width - Len;
    }
    List += AnsiString::StringOfChar(' ', Added) + Value;
    Added += Value.Length();
    if (Width > Added)
    {
      List += AnsiString::StringOfChar(' ', Width - Added);
    }
    if (!LastCol)
    {
      List += Separator;
    }
  }
  else
  {
    int Scroll = FScroll;
    if ((Scroll > 0) && !Header)
    {
      if (List.IsEmpty())
      {
        List += '{';
        Width--;
        Scroll++;
      }
      else
      {
        List[List.Length()] = '{';
      }
    }
    if (Scroll > Len - Width)
    {
      Scroll = Len - Width;
    }
    else if (!Header && LastCol && (Scroll < Len - Width))
    {
      Width--;
    }
    List += Value.SubString(Scroll + 1, Width);
    if (!Header && (Len - Scroll > Width))
    {
      List += '}';
      FCanScrollRight = true;
    }
    else if (!LastCol)
    {
      List += Separator;
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::AdaptSize()
{
  FScroll = 0;
  Size = MaxSize;

  VideoModeButton->Caption = GetMsg(
    FarPlugin->ConsoleWindowState() == SW_SHOWMAXIMIZED ?
      CHECKLIST_RESTORE : CHECKLIST_MAXIMIZE);

  static const Ratio[FColumns] = { 140, 100, 80, 150, -2, 100, 80, 150 };

  int Width = ListBox->Width - 2 /*checkbox*/ - 1 /*scrollbar*/ - FColumns;
  double Temp[FColumns];

  int TotalRatio = 0;
  int FixedRatio = 0;
  for (int Index = 0; Index < FColumns; Index++)
  {
    if (Ratio[Index] >= 0)
    {
      TotalRatio += Ratio[Index];
    }
    else
    {
      FixedRatio += -Ratio[Index];
    }
  }

  int TotalAssigned = 0;
  for (int Index = 0; Index < FColumns; Index++)
  {
    if (Ratio[Index] >= 0)
    {
      double W = static_cast<float>(Ratio[Index]) * (Width - FixedRatio) / TotalRatio;
      FWidths[Index] = floor(W);
      Temp[Index] = W - FWidths[Index];
    }
    else
    {
      FWidths[Index] = -Ratio[Index];
      Temp[Index] = 0;
    }
    TotalAssigned += FWidths[Index];
  }

  while (TotalAssigned < Width)
  {
    int GrowIndex = 0;
    double MaxMissing = 0.0;
    for (int Index = 0; Index < FColumns; Index++)
    {
      if (MaxMissing < Temp[Index])
      {
        MaxMissing = Temp[Index];
        GrowIndex = Index;
      }
    }

    assert(MaxMissing > 0.0);

    FWidths[GrowIndex]++;
    Temp[GrowIndex] = 0.0;
    TotalAssigned++;
  }

  RefreshChecklist(false);
}
//---------------------------------------------------------------------------
AnsiString __fastcall TSynchronizeChecklistDialog::FormatSize(
  __int64 Size, int Column)
{
  int Width = FWidths[Column];
  AnsiString Result = FormatFloat("#,##0", Size);

  if (Result.Length() > Width)
  {
    Result = FormatFloat("0", Size);

    if (Result.Length() > Width)
    {
      Result = FormatFloat("0 'K'", Size / 1024);
      if (Result.Length() > Width)
      {
        Result = FormatFloat("0 'M'", Size / (1024*1024));
        if (Result.Length() > Width)
        {
          Result = FormatFloat("0 'G'", Size / (1024*1024*1024));
          if (Result.Length() > Width)
          {
            // back to default
            Result = FormatFloat("#,##0", Size);
          }
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
AnsiString __fastcall TSynchronizeChecklistDialog::ItemLine(
  const TSynchronizeChecklist::TItem * ChecklistItem)
{
  AnsiString Line;
  AnsiString S;

  S = ChecklistItem->FileName;
  if (ChecklistItem->IsDirectory)
  {
    S = IncludeTrailingBackslash(S);
  }
  AddColumn(Line, S, 0);

  if (ChecklistItem->Action == TSynchronizeChecklist::saDeleteRemote)
  {
    AddColumn(Line, "", 1);
    AddColumn(Line, "", 2);
    AddColumn(Line, "", 3);
  }
  else
  {
    S = ChecklistItem->Local.Directory;
    if (AnsiSameText(FLocalDirectory, S.SubString(1, FLocalDirectory.Length())))
    {
      S[1] = '.';
      S.Delete(2, FLocalDirectory.Length() - 1);
    }
    else
    {
      assert(false);
    }
    AddColumn(Line, S, 1);
    if (ChecklistItem->Action == TSynchronizeChecklist::saDownloadNew)
    {
      AddColumn(Line, "", 2);
      AddColumn(Line, "", 3);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, "", 2);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Local.Size, 2), 2);
      }
      AddColumn(Line, UserModificationStr(ChecklistItem->Local.Modification,
        ChecklistItem->Local.ModificationFmt), 3);
    }
  }

  int Action = int(ChecklistItem->Action) - 1;
  assert(Action < LENOF(FActions));
  AddColumn(Line, FActions[Action], 4);

  if (ChecklistItem->Action == TSynchronizeChecklist::saDeleteLocal)
  {
    AddColumn(Line, "", 5);
    AddColumn(Line, "", 6);
    AddColumn(Line, "", 7);
  }
  else
  {
    S = ChecklistItem->Remote.Directory;
    if (AnsiSameText(FRemoteDirectory, S.SubString(1, FRemoteDirectory.Length())))
    {
      S[1] = '.';
      S.Delete(2, FRemoteDirectory.Length() - 1);
    }
    else
    {
      assert(false);
    }
    AddColumn(Line, S, 5);
    if (ChecklistItem->Action == TSynchronizeChecklist::saUploadNew)
    {
      AddColumn(Line, "", 6);
      AddColumn(Line, "", 7);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, "", 6);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Remote.Size, 6), 6);
      }
      AddColumn(Line, UserModificationStr(ChecklistItem->Remote.Modification,
        ChecklistItem->Remote.ModificationFmt), 7);
    }
  }

  return Line;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::LoadChecklist()
{
  FChecked = 0;
  TFarList * List = new TFarList();
  try
  {
    List->BeginUpdate();
    for (int Index = 0; Index < FChecklist->Count; Index++)
    {
      const TSynchronizeChecklist::TItem * ChecklistItem = FChecklist->Item[Index];

      List->AddObject(ItemLine(ChecklistItem),
        const_cast<TObject *>(reinterpret_cast<const TObject *>(ChecklistItem)));
    }
    List->EndUpdate();

    // items must be checked in second pass once the internal array is allocated
    for (int Index = 0; Index < FChecklist->Count; Index++)
    {
      const TSynchronizeChecklist::TItem * ChecklistItem = FChecklist->Item[Index];

      List->Checked[Index] = ChecklistItem->Checked;
      if (ChecklistItem->Checked)
      {
        FChecked++;
      }
    }

    ListBox->Items = List;
  }
  __finally
  {
    delete List;
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::RefreshChecklist(bool Scroll)
{
  AnsiString HeaderStr = GetMsg(CHECKLIST_HEADER);
  AnsiString HeaderCaption(AnsiString::StringOfChar(' ', 2));

  for (int Index = 0; Index < FColumns; Index++)
  {
    AddColumn(HeaderCaption, CutToChar(HeaderStr, '|', false), Index, true);
  }
  Header->Caption = HeaderCaption;

  FCanScrollRight = false;
  TFarList * List = ListBox->Items;
  List->BeginUpdate();
  try
  {
    for (int Index = 0; Index < List->Count; Index++)
    {
      if (!Scroll || (List->Strings[Index].LastDelimiter("{}") > 0))
      {
        const TSynchronizeChecklist::TItem * ChecklistItem =
          reinterpret_cast<TSynchronizeChecklist::TItem *>(List->Objects[Index]);

        List->Strings[Index] = ItemLine(ChecklistItem);
      }
    }
  }
  __finally
  {
    List->EndUpdate();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::UpdateControls()
{
  ButtonSeparator->Caption =
    FORMAT(GetMsg(CHECKLIST_CHECKED), (FChecked, ListBox->Items->Count));
  CheckAllButton->Enabled = (FChecked < ListBox->Items->Count);
  UncheckAllButton->Enabled = (FChecked > 0);
}
//---------------------------------------------------------------------------
long __fastcall TSynchronizeChecklistDialog::DialogProc(int Msg, int Param1, long Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::CheckAll(bool Check)
{
  TFarList * List = ListBox->Items;
  List->BeginUpdate();
  try
  {
    int Count = List->Count;
    for (int Index = 0; Index < Count; Index++)
    {
      List->Checked[Index] = Check;
    }

    FChecked = (Check ? Count : 0);
  }
  __finally
  {
    List->EndUpdate();
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::CheckAllButtonClick(
  TFarButton * Sender, bool & Close)
{
  CheckAll(Sender == CheckAllButton);
  ListBox->SetFocus();

  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::VideoModeButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  FarPlugin->ToggleVideoMode();

  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeChecklistDialog::ListBoxClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * /*Event*/)
{
  int Index = ListBox->Items->Selected;
  if (Index >= 0)
  {
    if (ListBox->Items->Checked[Index])
    {
      ListBox->Items->Checked[Index] = false;
      FChecked--;
    }
    else if (!ListBox->Items->Checked[Index])
    {
      ListBox->Items->Checked[Index] = true;
      FChecked++;
    }

    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeChecklistDialog::Key(TFarDialogItem * Item, long KeyCode)
{
  bool Result = false;
  if (ListBox->Focused())
  {
    if ((KeyCode == KEY_SHIFTADD) || (KeyCode == KEY_SHIFTSUBTRACT))
    {
      CheckAll(KeyCode == KEY_SHIFTADD);
      Result = true;
    }
    else if ((KeyCode == KEY_SPACE) || (KeyCode == KEY_INS) ||
             (KeyCode == KEY_ADD) || (KeyCode == KEY_SUBTRACT))
    {
      int Index = ListBox->Items->Selected;
      if (Index >= 0)
      {
        if (ListBox->Items->Checked[Index] && (KeyCode != KEY_ADD))
        {
          ListBox->Items->Checked[Index] = false;
          FChecked--;
        }
        else if (!ListBox->Items->Checked[Index] && (KeyCode != KEY_SUBTRACT))
        {
          ListBox->Items->Checked[Index] = true;
          FChecked++;
        }

        // FAR WORKAROUND
        // Changing "checked" state is not always drawn.
        Redraw();
        UpdateControls();
        if ((KeyCode == KEY_INS) &&
            (Index < ListBox->Items->Count - 1))
        {
          ListBox->Items->Selected = Index + 1;
        }
      }
      Result = true;
    }
    else if (KeyCode == KEY_ALTLEFT)
    {
      if (FScroll > 0)
      {
        FScroll--;
        RefreshChecklist(true);
      }
      Result = true;
    }
    else if (KeyCode == KEY_ALTRIGHT)
    {
      if (FCanScrollRight)
      {
        FScroll++;
        RefreshChecklist(true);
      }
      Result = true;
    }
  }

  if (!Result)
  {
    Result = TWinSCPDialog::Key(Item, KeyCode);
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeChecklistDialog::Execute(TSynchronizeChecklist * Checklist)
{
  FChecklist = Checklist;
  LoadChecklist();
  bool Result = (ShowModal() == brOK);

  if (Result)
  {
    TFarList * List = ListBox->Items;
    int Count = List->Count;
    for (int Index = 0; Index < Count; Index++)
    {
      TSynchronizeChecklist::TItem * ChecklistItem =
        reinterpret_cast<TSynchronizeChecklist::TItem *>(List->Objects[Index]);
      ChecklistItem->Checked = List->Checked[Index];
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::SynchronizeChecklistDialog(
  TSynchronizeChecklist * Checklist, TTerminal::TSynchronizeMode Mode, int Params,
  const AnsiString LocalDirectory, const AnsiString RemoteDirectory)
{
  bool Result;
  TSynchronizeChecklistDialog * Dialog = new TSynchronizeChecklistDialog(
    FPlugin, Mode, Params, LocalDirectory, RemoteDirectory);
  try
  {
    Result = Dialog->Execute(Checklist);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSynchronizeDialog : TFarDialog
{
public:
  __fastcall TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
    TSynchronizeStartStopEvent OnStartStop,
    int Options, int CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions);
  virtual __fastcall ~TSynchronizeDialog();

  bool __fastcall Execute(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, bool & SaveSettings);

protected:
  virtual void __fastcall Change();
  void __fastcall UpdateControls();
  void __fastcall StartButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall StopButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void __fastcall CopyParamListerClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
  void __fastcall Stop();
  void __fastcall DoStartStop(bool Start, bool Synchronize);
  TSynchronizeParamType __fastcall GetParams();
  void __fastcall DoAbort(TObject * Sender, bool Close);
  void __fastcall DoLog(TSynchronizeController * Controller,
    TSynchronizeLogEntry Entry, const AnsiString Message);
  void __fastcall DoSynchronizeThreads(TObject * Sender, TThreadMethod Method);
  virtual long __fastcall DialogProc(int Msg, int Param1, long Param2);
  virtual bool __fastcall CloseQuery();
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  TCopyParamType __fastcall GetCopyParams();
  int __fastcall ActualCopyParamAttrs();
  void __fastcall CustomCopyParam();

private:
  bool FSynchronizing;
  bool FStarted;
  bool FAbort;
  bool FClose;
  TSynchronizeParamType FParams;
  TSynchronizeStartStopEvent FOnStartStop;
  int FOptions;
  TSynchronizeOptions * FSynchronizeOptions;
  TCopyParamType FCopyParams;
  TGetSynchronizeOptionsEvent FOnGetOptions;
  int FCopyParamAttrs;

  TFarEdit * LocalDirectoryEdit;
  TFarEdit * RemoteDirectoryEdit;
  TFarCheckBox * SynchronizeDeleteCheck;
  TFarCheckBox * SynchronizeExistingOnlyCheck;
  TFarCheckBox * SynchronizeSelectedOnlyCheck;
  TFarCheckBox * SynchronizeRecursiveCheck;
  TFarCheckBox * SynchronizeSynchronizeCheck;
  TFarCheckBox * SaveSettingsCheck;
  TFarButton * StartButton;
  TFarButton * StopButton;
  TFarButton * CloseButton;
  TFarLister * CopyParamLister;
};
//---------------------------------------------------------------------------
__fastcall TSynchronizeDialog::TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
  TSynchronizeStartStopEvent OnStartStop,
  int Options, int CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions) :
  TFarDialog(AFarPlugin)
{
  TFarText * Text;
  TFarSeparator * Separator;

  FSynchronizing = false;
  FStarted = false;
  FOnStartStop = OnStartStop;
  FAbort = false;
  FClose = false;
  FOptions = Options;
  FOnGetOptions = OnGetOptions;
  FSynchronizeOptions = NULL;
  FCopyParamAttrs = CopyParamAttrs;

  Size = TPoint(76, 20);

  DefaultGroup = 1;

  Text = new TFarText(this);
  Text->Caption = GetMsg(SYNCHRONIZE_LOCAL_LABEL);

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->History = LOCAL_SYNC_HISTORY;

  Text = new TFarText(this);
  Text->Caption = GetMsg(SYNCHRONIZE_REMOTE_LABEL);

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->History = REMOTE_SYNC_HISTORY;

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(SYNCHRONIZE_GROUP);
  Separator->Group = 0;

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->Caption = GetMsg(SYNCHRONIZE_DELETE);

  NextItemPosition = ipRight;

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->Caption = GetMsg(SYNCHRONIZE_EXISTING_ONLY);

  NextItemPosition = ipNewLine;

  SynchronizeRecursiveCheck = new TFarCheckBox(this);
  SynchronizeRecursiveCheck->Caption = GetMsg(SYNCHRONIZE_RECURSIVE);

  NextItemPosition = ipRight;

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->Caption = GetMsg(SYNCHRONIZE_SELECTED_ONLY);
  // have more complex enable rules
  SynchronizeSelectedOnlyCheck->Group = 0;

  NextItemPosition = ipNewLine;

  SynchronizeSynchronizeCheck = new TFarCheckBox(this);
  SynchronizeSynchronizeCheck->Caption = GetMsg(SYNCHRONIZE_SYNCHRONIZE);
  SynchronizeSynchronizeCheck->AllowGrayed = true;

  Separator = new TFarSeparator(this);
  Separator->Group = 0;

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->Caption = GetMsg(SYNCHRONIZE_REUSE_SETTINGS);

  Separator = new TFarSeparator(this);
  Separator->Caption = GetMsg(COPY_PARAM_GROUP);

  CopyParamLister = new TFarLister(this);
  CopyParamLister->Height = 3;
  CopyParamLister->Left = BorderBox->Left + 1;
  CopyParamLister->TabStop = false;
  CopyParamLister->OnMouseClick = CopyParamListerClick;
  // Right edge is adjusted in Change

  DefaultGroup = 0;

  // align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->Position = -4;

  TFarButton * Button = new TFarButton(this);
  Button->Caption = GetMsg(TRANSFER_SETTINGS_BUTTON);
  Button->Result = -1;
  Button->CenterGroup = true;
  Button->OnClick = TransferSettingsButtonClick;

  NextItemPosition = ipRight;

  StartButton = new TFarButton(this);
  StartButton->Caption = GetMsg(SYNCHRONIZE_START_BUTTON);
  StartButton->Default = true;
  StartButton->CenterGroup = true;
  StartButton->OnClick = StartButtonClick;

  StopButton = new TFarButton(this);
  StopButton->Caption = GetMsg(SYNCHRONIZE_STOP_BUTTON);
  StopButton->CenterGroup = true;
  StopButton->OnClick = StopButtonClick;

  NextItemPosition = ipRight;

  CloseButton = new TFarButton(this);
  CloseButton->Caption = GetMsg(MSG_BUTTON_Close);
  CloseButton->Result = brCancel;
  CloseButton->CenterGroup = true;
}
//---------------------------------------------------------------------------
__fastcall TSynchronizeDialog::~TSynchronizeDialog()
{
  delete FSynchronizeOptions;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
  // PreserveTime is forced for some settings, but avoid hard-setting it until
  // user really confirms it on cutom dialog
  TCopyParamType ACopyParams = GetCopyParams();
  if (WinSCPPlugin->CopyParamCustomDialog(ACopyParams, ActualCopyParamAttrs()))
  {
    FCopyParams = ACopyParams;
    Change();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeDialog::Execute(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, bool & SaveSettings)
{
  RemoteDirectoryEdit->Text = Params.RemoteDirectory;
  LocalDirectoryEdit->Text = Params.LocalDirectory;
  SynchronizeDeleteCheck->Checked = FLAGSET(Params.Params, TTerminal::spDelete);
  SynchronizeExistingOnlyCheck->Checked = FLAGSET(Params.Params, TTerminal::spExistingOnly);
  SynchronizeSelectedOnlyCheck->Checked = FLAGSET(Params.Params, spSelectedOnly);
  SynchronizeRecursiveCheck->Checked = FLAGSET(Params.Options, soRecurse);
  SynchronizeSynchronizeCheck->Selected =
    FLAGSET(Params.Options, soSynchronizeAsk) ? BSTATE_3STATE :
      (FLAGSET(Params.Options, soSynchronize) ? BSTATE_CHECKED : BSTATE_UNCHECKED);
  SaveSettingsCheck->Checked = SaveSettings;

  FParams = Params;
  FCopyParams = *CopyParams;

  ShowModal();

  Params = GetParams();
  SaveSettings = SaveSettingsCheck->Checked;

  return true;
}
//---------------------------------------------------------------------------
TSynchronizeParamType __fastcall TSynchronizeDialog::GetParams()
{
  TSynchronizeParamType Result = FParams;
  Result.RemoteDirectory = RemoteDirectoryEdit->Text;
  Result.LocalDirectory = LocalDirectoryEdit->Text;
  Result.Params =
    (Result.Params & ~(TTerminal::spDelete | TTerminal::spExistingOnly |
     spSelectedOnly | TTerminal::spTimestamp)) |
    FLAGMASK(SynchronizeDeleteCheck->Checked, TTerminal::spDelete) |
    FLAGMASK(SynchronizeExistingOnlyCheck->Checked, TTerminal::spExistingOnly) |
    FLAGMASK(SynchronizeSelectedOnlyCheck->Checked, spSelectedOnly);
  Result.Options =
    (Result.Options & ~(soRecurse | soSynchronize | soSynchronizeAsk)) |
    FLAGMASK(SynchronizeRecursiveCheck->Checked, soRecurse) |
    FLAGMASK(SynchronizeSynchronizeCheck->Selected == BSTATE_CHECKED, soSynchronize) |
    FLAGMASK(SynchronizeSynchronizeCheck->Selected == BSTATE_3STATE, soSynchronizeAsk);
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::DoStartStop(bool Start, bool Synchronize)
{
  if (FOnStartStop)
  {
    TSynchronizeParamType SParams = GetParams();
    SParams.Options =
      (SParams.Options & ~(soSynchronize | soSynchronizeAsk)) |
      FLAGMASK(Synchronize, soSynchronize);
    if (Start)
    {
      delete FSynchronizeOptions;
      FSynchronizeOptions = new TSynchronizeOptions;
      FOnGetOptions(SParams.Params, *FSynchronizeOptions);
    }
    FOnStartStop(this, Start, SParams, GetCopyParams(), FSynchronizeOptions, DoAbort,
      DoSynchronizeThreads, DoLog);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::DoSynchronizeThreads(TObject * /*Sender*/,
  TThreadMethod Method)
{
  if (FStarted)
  {
    Synchronize(Method);
  }
}
//---------------------------------------------------------------------------
long __fastcall TSynchronizeDialog::DialogProc(int Msg, int Param1, long Param2)
{
  if (FAbort)
  {
    FAbort = false;

    if (FSynchronizing)
    {
      Stop();
    }

    if (FClose)
    {
      assert(CloseButton->Enabled);
      Close(CloseButton);
    }
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeDialog::CloseQuery()
{
  return TFarDialog::CloseQuery() && !FSynchronizing;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::DoAbort(TObject * /*Sender*/, bool Close)
{
  FAbort = true;
  FClose = Close;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::DoLog(TSynchronizeController * /*Controller*/,
  TSynchronizeLogEntry /*Entry*/, const AnsiString /*Message*/)
{
  // void
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::StartButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  bool Synchronize;
  bool Continue = true;
  if (SynchronizeSynchronizeCheck->Selected == BSTATE_3STATE)
  {
    TMessageParams Params;
    Params.Params = qpNeverAskAgainCheck;
    TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(SYNCHRONISE_BEFORE_KEEPUPTODATE),
        NULL, qtConfirmation, qaYes | qaNo | qaCancel, &Params))
    {
      case qaNeverAskAgain:
        SynchronizeSynchronizeCheck->Selected = BSTATE_CHECKED;
        // fall thru

      case qaYes:
        Synchronize = true;
        break;

      case qaNo:
        Synchronize = false;
        break;

      default:
      case qaCancel:
        Continue = false;
        break;
    };
  }
  else
  {
    Synchronize = SynchronizeSynchronizeCheck->Checked;
  }

  if (Continue)
  {
    assert(!FSynchronizing);

    FSynchronizing = true;
    try
    {
      UpdateControls();

      DoStartStop(true, Synchronize);

      StopButton->SetFocus();
      FStarted = true;
    }
    catch(Exception & E)
    {
      FSynchronizing = false;
      UpdateControls();

      FarPlugin->HandleException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::StopButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  Stop();
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::Stop()
{
  FSynchronizing = false;
  FStarted = false;
  BreakSynchronize();
  DoStartStop(false, false);
  UpdateControls();
  StartButton->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::Change()
{
  TFarDialog::Change();

  if (Handle && !ChangesLocked())
  {
    UpdateControls();

    AnsiString InfoStr = FCopyParams.GetInfoStr("; ", ActualCopyParamAttrs());
    TStringList * InfoStrLines = new TStringList();
    try
    {
      FarWrapText(InfoStr, InfoStrLines, BorderBox->Width - 4);
      CopyParamLister->Items = InfoStrLines;
      CopyParamLister->Right = BorderBox->Right - (CopyParamLister->ScrollBar ? 0 : 1);
    }
    __finally
    {
      delete InfoStrLines;
    }
  }
}
//---------------------------------------------------------------------------
bool __fastcall TSynchronizeDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
  bool Result = false;
  if ((KeyCode == KEY_ESC) && FSynchronizing)
  {
    Stop();
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TSynchronizeDialog::UpdateControls()
{
  Caption = GetMsg(FSynchronizing ? SYNCHRONIZE_SYCHRONIZING : SYNCHRONIZE_TITLE);
  StartButton->Enabled = !FSynchronizing;
  StopButton->Enabled = FSynchronizing;
  CloseButton->Enabled = !FSynchronizing;
  EnableGroup(1, !FSynchronizing);
  SynchronizeSelectedOnlyCheck->Enabled =
    !FSynchronizing && FLAGSET(FOptions, soAllowSelectedOnly);
}
//---------------------------------------------------------------------------
TCopyParamType __fastcall TSynchronizeDialog::GetCopyParams()
{
  TCopyParamType Result = FCopyParams;
  Result.PreserveTime = true;
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TSynchronizeDialog::ActualCopyParamAttrs()
{
  return FCopyParamAttrs | cpaNoPreserveTime;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::SynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, int Options, int CopyParamAttrs, TGetSynchronizeOptionsEvent OnGetOptions)
{
  bool Result;
  TSynchronizeDialog * Dialog = new TSynchronizeDialog(FPlugin, OnStartStop,
    Options, CopyParamAttrs, OnGetOptions);
  try
  {
    Result = Dialog->Execute(Params, CopyParams, SaveSettings);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::RemoteTransferDialog(TStrings * FileList,
  AnsiString & Target, AnsiString & FileMask, bool Move)
{
  AnsiString Prompt = FileNameFormatString(
    GetMsg(Move ? REMOTE_MOVE_FILE : REMOTE_COPY_FILE),
    GetMsg(Move ? REMOTE_MOVE_FILES : REMOTE_COPY_FILES), FileList, true);

  AnsiString Value = UnixIncludeTrailingBackslash(Target) + FileMask;
  bool Result = FPlugin->InputBox(
    GetMsg(Move ? REMOTE_MOVE_TITLE : REMOTE_COPY_TITLE), Prompt,
    Value, 0, MOVE_TO_HISTORY) && !Value.IsEmpty();
  if (Result)
  {
    Target = UnixExtractFilePath(Value);
    FileMask = UnixExtractFileName(Value);
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::RenameFileDialog(TRemoteFile * File,
  AnsiString & NewName)
{
  return FPlugin->InputBox(GetMsg(RENAME_FILE_TITLE),
    FORMAT(GetMsg(RENAME_FILE), (File->FileName)), NewName, 0) &&
    !NewName.IsEmpty();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TQueueDialog : TFarDialog
{
public:
  __fastcall TQueueDialog(TCustomFarPlugin * AFarPlugin,
    TWinSCPFileSystem * AFileSystem, bool ClosingPlugin);

  bool __fastcall Execute(TTerminalQueueStatus * Status);

protected:
  virtual void __fastcall Change();
  virtual void __fastcall Idle();
  bool __fastcall UpdateQueue();
  void __fastcall LoadQueue();
  void __fastcall RefreshQueue();
  bool __fastcall FillQueueItemLine(AnsiString & Line,
    TQueueItemProxy * QueueItem, int Index);
  bool __fastcall QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem);
  void __fastcall UpdateControls();
  virtual bool __fastcall Key(TFarDialogItem * Item, long KeyCode);
  virtual bool __fastcall CloseQuery();

private:
  TTerminalQueueStatus * FStatus;
  TWinSCPFileSystem * FFileSystem;
  bool FClosingPlugin;

  TFarListBox * QueueListBox;
  TFarButton * ShowButton;
  TFarButton * ExecuteButton;
  TFarButton * DeleteButton;
  TFarButton * MoveUpButton;
  TFarButton * MoveDownButton;
  TFarButton * CloseButton;

  void __fastcall OperationButtonClick(TFarButton * Sender, bool & Close);
};
//---------------------------------------------------------------------------
__fastcall TQueueDialog::TQueueDialog(TCustomFarPlugin * AFarPlugin,
  TWinSCPFileSystem * AFileSystem, bool ClosingPlugin) :
  TFarDialog(AFarPlugin), FFileSystem(AFileSystem),
  FClosingPlugin(ClosingPlugin)
{
  TFarSeparator * Separator;
  TFarText * Text;

  Size = TPoint(80, 23);
  TRect CRect = ClientRect;
  int ListTop;
  int ListHeight = ClientSize.y - 4;

  Caption = GetMsg(QUEUE_TITLE);

  Text = new TFarText(this);
  Text->Caption = GetMsg(QUEUE_HEADER);

  Separator = new TFarSeparator(this);
  ListTop = Separator->Bottom;

  Separator = new TFarSeparator(this);
  Separator->Move(0, ListHeight);

  ExecuteButton = new TFarButton(this);
  ExecuteButton->Caption = GetMsg(QUEUE_EXECUTE);
  ExecuteButton->OnClick = OperationButtonClick;
  ExecuteButton->CenterGroup = true;

  NextItemPosition = ipRight;

  DeleteButton = new TFarButton(this);
  DeleteButton->Caption = GetMsg(QUEUE_DELETE);
  DeleteButton->OnClick = OperationButtonClick;
  DeleteButton->CenterGroup = true;

  MoveUpButton = new TFarButton(this);
  MoveUpButton->Caption = GetMsg(QUEUE_MOVE_UP);
  MoveUpButton->OnClick = OperationButtonClick;
  MoveUpButton->CenterGroup = true;

  MoveDownButton = new TFarButton(this);
  MoveDownButton->Caption = GetMsg(QUEUE_MOVE_DOWN);
  MoveDownButton->OnClick = OperationButtonClick;
  MoveDownButton->CenterGroup = true;

  CloseButton = new TFarButton(this);
  CloseButton->Caption = GetMsg(QUEUE_CLOSE);
  CloseButton->Result = brCancel;
  CloseButton->CenterGroup = true;
  CloseButton->Default = true;

  NextItemPosition = ipNewLine;

  QueueListBox = new TFarListBox(this);
  QueueListBox->Top = ListTop + 1;
  QueueListBox->Height = ListHeight;
  QueueListBox->NoBox = true;
  QueueListBox->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::OperationButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  TQueueItemProxy * QueueItem;
  if (QueueListBox->Items->Selected >= 0)
  {
    QueueItem = reinterpret_cast<TQueueItemProxy *>(
      QueueListBox->Items->Objects[QueueListBox->Items->Selected]);

    if (Sender == ExecuteButton)
    {
      if (QueueItem->Status == TQueueItem::qsProcessing)
      {
        QueueItem->Pause();
      }
      else if (QueueItem->Status == TQueueItem::qsPaused)
      {
        QueueItem->Resume();
      }
      else if (QueueItem->Status == TQueueItem::qsPending)
      {
        QueueItem->ExecuteNow();
      }
      else if (TQueueItem::IsUserActionStatus(QueueItem->Status))
      {
        QueueItem->ProcessUserAction();
      }
      else
      {
        assert(false);
      }
    }
    else if ((Sender == MoveUpButton) || (Sender == MoveDownButton))
    {
      QueueItem->Move(Sender == MoveUpButton);
    }
    else if (Sender == DeleteButton)
    {
      QueueItem->Delete();
    }
  }
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
  bool Result = false;
  if (QueueListBox->Focused())
  {
    TFarButton * DoButton = NULL;
    if (KeyCode == KEY_ENTER)
    {
      if (ExecuteButton->Enabled)
      {
        DoButton = ExecuteButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_DEL)
    {
      if (DeleteButton->Enabled)
      {
        DoButton = DeleteButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_CTRLUP)
    {
      if (MoveUpButton->Enabled)
      {
        DoButton = MoveUpButton;
      }
      Result = true;
    }
    else if (KeyCode == KEY_CTRLDOWN)
    {
      if (MoveDownButton->Enabled)
      {
        DoButton = MoveDownButton;
      }
      Result = true;
    }

    if (DoButton != NULL)
    {
      bool Close;
      OperationButtonClick(DoButton, Close);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::UpdateControls()
{
  TQueueItemProxy * QueueItem = NULL;
  if (QueueListBox->Items->Selected >= 0)
  {
    QueueItem = reinterpret_cast<TQueueItemProxy *>(
      QueueListBox->Items->Objects[QueueListBox->Items->Selected]);
  }

  if ((QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsProcessing))
  {
    ExecuteButton->Caption = GetMsg(QUEUE_PAUSE);
    ExecuteButton->Enabled = true;
  }
  else if ((QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsPaused))
  {
    ExecuteButton->Caption = GetMsg(QUEUE_RESUME);
    ExecuteButton->Enabled = true;
  }
  else if ((QueueItem != NULL) && TQueueItem::IsUserActionStatus(QueueItem->Status))
  {
    ExecuteButton->Caption = GetMsg(QUEUE_SHOW);
    ExecuteButton->Enabled = true;
  }
  else
  {
    ExecuteButton->Caption = GetMsg(QUEUE_EXECUTE);
    ExecuteButton->Enabled =
      (QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsPending);
  }
  DeleteButton->Enabled = (QueueItem != NULL) &&
    (QueueItem->Status != TQueueItem::qsDone);
  MoveUpButton->Enabled = (QueueItem != NULL) &&
    (QueueItem->Status == TQueueItem::qsPending) &&
    (QueueItem->Index > FStatus->ActiveCount);
  MoveDownButton->Enabled = (QueueItem != NULL) &&
    (QueueItem->Status == TQueueItem::qsPending) &&
    (QueueItem->Index < FStatus->Count - 1);
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::Idle()
{
  TFarDialog::Idle();

  if (UpdateQueue())
  {
    LoadQueue();
    UpdateControls();
  }
  else
  {
    RefreshQueue();
  }
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::CloseQuery()
{
  bool Result = TFarDialog::CloseQuery();
  if (Result)
  {
    TWinSCPPlugin* WinSCPPlugin = dynamic_cast<TWinSCPPlugin*>(FarPlugin);
    Result = !FClosingPlugin || (FStatus->Count == 0) ||
      (WinSCPPlugin->MoreMessageDialog(GetMsg(QUEUE_PENDING_ITEMS), NULL,
        qtWarning, qaOK | qaCancel) == qaCancel);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::UpdateQueue()
{
  assert(FFileSystem != NULL);
  TTerminalQueueStatus * Status = FFileSystem->ProcessQueue(false);
  bool Result = (Status != NULL);
  if (Result)
  {
    FStatus = Status;
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::Change()
{
  TFarDialog::Change();

  if (Handle)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::RefreshQueue()
{
  if (QueueListBox->Items->Count > 0)
  {
    bool Change = false;
    int TopIndex = QueueListBox->Items->TopIndex;
    int Index = TopIndex;

    int ILine = 0;
    while ((Index - ILine > 0) &&
           (QueueListBox->Items->Objects[Index] ==
              QueueListBox->Items->Objects[Index - ILine - 1]))
    {
      ILine++;
    }

    TQueueItemProxy * PrevQueueItem = NULL;
    TQueueItemProxy * QueueItem;
    AnsiString Line;
    while ((Index < QueueListBox->Items->Count) &&
           (Index < TopIndex + QueueListBox->Height))
    {
      QueueItem = reinterpret_cast<TQueueItemProxy*>(
        QueueListBox->Items->Objects[Index]);
      assert(QueueItem != NULL);
      if ((PrevQueueItem != NULL) && (QueueItem != PrevQueueItem))
      {
        ILine = 0;
      }

      if (QueueItemNeedsFrequentRefresh(QueueItem) &&
          !QueueItem->ProcessingUserAction)
      {
        FillQueueItemLine(Line, QueueItem, ILine);
        if (QueueListBox->Items->Strings[Index] != Line)
        {
          Change = true;
          QueueListBox->Items->Strings[Index] = Line;
        }
      }

      PrevQueueItem = QueueItem;
      Index++;
      ILine++;
    }

    if (Change)
    {
      Redraw();
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TQueueDialog::LoadQueue()
{
  TFarList * List = new TFarList();
  try
  {
    AnsiString Line;
    TQueueItemProxy * QueueItem;
    for (int Index = 0; Index < FStatus->Count; Index++)
    {
      QueueItem = FStatus->Items[Index];
      int ILine = 0;
      while (FillQueueItemLine(Line, QueueItem, ILine))
      {
        List->AddObject(Line, reinterpret_cast<TObject*>(QueueItem));
        List->Disabled[List->Count - 1] = (ILine > 0);
        ILine++;
      }
    }
    QueueListBox->Items = List;
  }
  __finally
  {
    delete List;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::FillQueueItemLine(AnsiString & Line,
  TQueueItemProxy * QueueItem, int Index)
{
  int PathMaxLen = 49;

  if ((Index > 2) ||
      ((Index == 2) && (QueueItem->Status == TQueueItem::qsPending)))
  {
    return false;
  }

  AnsiString ProgressStr;

  switch (QueueItem->Status)
  {
    case TQueueItem::qsPending:
      ProgressStr = GetMsg(QUEUE_PENDING);
      break;

    case TQueueItem::qsConnecting:
      ProgressStr = GetMsg(QUEUE_CONNECTING);
      break;

    case TQueueItem::qsQuery:
      ProgressStr = GetMsg(QUEUE_QUERY);
      break;

    case TQueueItem::qsError:
      ProgressStr = GetMsg(QUEUE_ERROR);
      break;

    case TQueueItem::qsPrompt:
      ProgressStr = GetMsg(QUEUE_PROMPT);
      break;

    case TQueueItem::qsPaused:
      ProgressStr = GetMsg(QUEUE_PAUSED);
      break;
  }

  bool BlinkHide = QueueItemNeedsFrequentRefresh(QueueItem) &&
    !QueueItem->ProcessingUserAction &&
    ((GetTickCount() % 2000) >= 1000);

  AnsiString Operation;
  AnsiString Direction;
  AnsiString Values[2];
  TFileOperationProgressType * ProgressData = QueueItem->ProgressData;
  TQueueItem::TInfo * Info = QueueItem->Info;

  if (Index == 0)
  {
    if (!BlinkHide)
    {
      switch (Info->Operation)
      {
        case foCopy:
          Operation = GetMsg(QUEUE_COPY);
          break;

        case foMove:
          Operation = GetMsg(QUEUE_MOVE);
          break;
      }
      Direction = GetMsg((Info->Side == osLocal) ? QUEUE_UPLOAD : QUEUE_DOWNLOAD);
    }

    Values[0] = MinimizeName(Info->Source, PathMaxLen, (Info->Side == osRemote));

    if ((ProgressData != NULL) &&
        (ProgressData->Operation == Info->Operation))
    {
      Values[1] = FormatBytes(ProgressData->TotalTransfered);
    }
  }
  else if (Index == 1)
  {
    Values[0] = MinimizeName(Info->Destination, PathMaxLen, (Info->Side == osLocal));

    if (ProgressStr.IsEmpty())
    {
      if (ProgressData != NULL)
      {
        if (ProgressData->Operation == Info->Operation)
        {
          Values[1] = FORMAT("%d%%", (ProgressData->OverallProgress()));
        }
        else if (ProgressData->Operation == foCalculateSize)
        {
          Values[1] = GetMsg(QUEUE_CALCULATING_SIZE);
        }
      }
    }
    else if (!BlinkHide)
    {
      Values[1] = ProgressStr;
    }
  }
  else
  {
    if (ProgressData != NULL)
    {
      Values[0] = MinimizeName(ProgressData->FileName, PathMaxLen,
        (Info->Side == osRemote));
      if (ProgressData->Operation == Info->Operation)
      {
        Values[1] = FORMAT("%d%%", (ProgressData->TransferProgress()));
      }
    }
    else
    {
      Values[0] = ProgressStr;
    }
  }

  Line = FORMAT("%1s %1s  %-*.*s %s",
    (Operation, Direction, PathMaxLen, PathMaxLen, Values[0], Values[1]));

  return true;
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::QueueItemNeedsFrequentRefresh(
  TQueueItemProxy * QueueItem)
{
  return
    (TQueueItem::IsUserActionStatus(QueueItem->Status) ||
     (QueueItem->Status == TQueueItem::qsPaused));
}
//---------------------------------------------------------------------------
bool __fastcall TQueueDialog::Execute(TTerminalQueueStatus * Status)
{
  FStatus = Status;

  UpdateQueue();
  LoadQueue();

  bool Result = (ShowModal() != brCancel);

  FStatus = NULL;

  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::QueueDialog(
  TTerminalQueueStatus * Status, bool ClosingPlugin)
{
  bool Result;
  TQueueDialog * Dialog = new TQueueDialog(FPlugin, this, ClosingPlugin);
  try
  {
    Result = Dialog->Execute(Status);
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TWinSCPFileSystem::CreateDirectoryDialog(AnsiString & Directory,
  TRemoteProperties * Properties, bool & SaveSettings)
{
  bool Result;
  TWinSCPDialog * Dialog = new TWinSCPDialog(FPlugin);
  try
  {
    TFarText * Text;
    TFarSeparator * Separator;

    Dialog->Caption = GetMsg(CREATE_FOLDER_TITLE);
    Dialog->Size = TPoint(66, 15);

    Text = new TFarText(Dialog);
    Text->Caption = GetMsg(CREATE_FOLDER_PROMPT);

    TFarEdit * DirectoryEdit = new TFarEdit(Dialog);
    DirectoryEdit->History = "NewFolder";

    Separator = new TFarSeparator(Dialog);
    Separator->Caption = GetMsg(CREATE_FOLDER_ATTRIBUTES);

    TFarCheckBox * SetRightsCheck = new TFarCheckBox(Dialog);
    SetRightsCheck->Caption = GetMsg(CREATE_FOLDER_SET_RIGHTS);

    TRightsContainer * RightsContainer = new TRightsContainer(Dialog, false, true,
      true, SetRightsCheck);

    TFarCheckBox * SaveSettingsCheck = new TFarCheckBox(Dialog);
    SaveSettingsCheck->Caption = GetMsg(CREATE_FOLDER_REUSE_SETTINGS);
    SaveSettingsCheck->Move(0, 6);

    Dialog->AddStandardButtons();

    DirectoryEdit->Text = Directory;
    SaveSettingsCheck->Checked = SaveSettings;
    assert(Properties != NULL);
    SetRightsCheck->Checked = Properties->Valid.Contains(vpRights);
    // expect sensible value even if rights are not set valid
    RightsContainer->Rights = Properties->Rights;

    Result = (Dialog->ShowModal() == brOK);

    if (Result)
    {
      Directory = DirectoryEdit->Text;
      SaveSettings = SaveSettingsCheck->Checked;
      if (SetRightsCheck->Checked)
      {
        Properties->Valid = Properties->Valid << vpRights;
        Properties->Rights = RightsContainer->Rights;
      }
      else
      {
        Properties->Valid = Properties->Valid >> vpRights;
      }
    }
  }
  __finally
  {
    delete Dialog;
  }
  return Result;
}
//---------------------------------------------------------------------------
