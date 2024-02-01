#include <vcl.h>
#pragma hdrstop

#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarDialog.h"
#include "FarConfiguration.h"
#include "FarInterface.h"
#include "FarUtils.h"

#include <shellapi.h>
#include <PuttyTools.h>
#include <GUITools.h>
#include <Tools.h>
#include <CoreMain.h>
#include <Common.h>
#include <CopyParam.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <Bookmarks.h>
#include <Queue.h>
#include <MsgIDs.h>
//#include <farcolor.hpp>
#include "plugin_version.hpp"
#include "resource.h"

enum TButtonResult
{
  brCancel = -1,
  brOK = 1,
  brConnect = 2
};

class TWinSCPDialog : public TFarDialog
{
public:
  explicit TWinSCPDialog(TCustomFarPlugin * AFarPlugin);

  void AddStandardButtons(int32_t Shift = 0, bool ButtonsOnly = false);

  TFarSeparator * ButtonSeparator{nullptr};
  TFarButton * OkButton{nullptr};
  TFarButton * CancelButton{nullptr};
};

TWinSCPDialog::TWinSCPDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin),
  ButtonSeparator(nullptr),
  OkButton(nullptr),
  CancelButton(nullptr)
{
  TFarDialog::InitDialog();
}

void TWinSCPDialog::AddStandardButtons(int32_t Shift, bool ButtonsOnly)
{
  if (!ButtonsOnly)
  {
    SetNextItemPosition(ipNewLine);

    ButtonSeparator = new TFarSeparator(this);
    if (Shift >= 0)
    {
      ButtonSeparator->Move(0, Shift);
    }
    else
    {
      ButtonSeparator->SetTop(Shift);
      ButtonSeparator->SetBottom(Shift);
    }
  }

  DebugAssert(OkButton == nullptr);
  OkButton = new TFarButton(this);
  if (ButtonsOnly)
  {
    if (Shift >= 0)
    {
      OkButton->Move(0, Shift);
    }
    else
    {
      OkButton->SetTop(Shift);
      OkButton->SetBottom(Shift);
    }
  }
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  DebugAssert(CancelButton == nullptr);
  CancelButton = new TFarButton(this);
  CancelButton->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  CancelButton->SetResult(brCancel);
  CancelButton->SetCenterGroup(true);
}

class TTabButton;

class TTabbedDialog : public TWinSCPDialog
{
  friend class TTabButton;
public:
  explicit TTabbedDialog(TCustomFarPlugin * AFarPlugin, int32_t TabCount) noexcept;
  virtual ~TTabbedDialog() override = default;

  int32_t GetTab() const { return FTab; }

protected:
  void HideTabs();
  virtual void SelectTab(int32_t Tab);
  void TabButtonClick(TFarButton * Sender, bool & Close);
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode) override;
  virtual UnicodeString GetTabName(int32_t Tab) const;
  TTabButton * GetTabButton(int32_t Tab) const;
  int32_t GetTabCount() const { return FTabCount; }

private:
  UnicodeString FOrigCaption;
  int32_t FTab{0};
  int32_t FTabCount{0};
};

class TTabButton : public TFarButton
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TTabButton); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TTabButton) || TFarButton::is(Kind); }
public:
  explicit TTabButton(TTabbedDialog * Dialog);

  int32_t GetTab() const { return FTab; }
  void SetTab(int32_t Value) { FTab = Value; }
  UnicodeString GetTabName() const { return FTabName; }
  void SetTabName(const UnicodeString & AValue);

private:
  UnicodeString FTabName;
  int32_t FTab{0};
};

TTabbedDialog::TTabbedDialog(TCustomFarPlugin * AFarPlugin, int32_t TabCount) noexcept :
  TWinSCPDialog(AFarPlugin),
  FTabCount(TabCount)
{
  // FAR WORKAROUND
  // (to avoid first control on dialog be a button, that would be "pressed"
  // when listbox loses focus)
  TFarText * Text = new TFarText(this);
  // make next item be inserted to default position
  Text->Move(0, -1);
  // on FAR 1.70 alpha 6 and later, empty text control would overwrite the
  // dialog box caption
  Text->SetVisible(false);
}

void TTabbedDialog::HideTabs()
{
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TFarDialogItem * Item = GetItem(Index);
    if (Item->GetGroup())
    {
      Item->SetVisible(false);
    }
  }
}

void TTabbedDialog::SelectTab(int32_t Tab)
{
  /*for (int32_t I = FTabCount - 1; I >= 1; I--)
  {
    TTabButton * Button = TabButton(I);
    Button->SetBrackets(Button->GetTab() == Tab ? brTight : brNone);
  }*/
  if (FTab != Tab)
  {
    if (FTab)
    {
      ShowGroup(FTab, false);
    }
    ShowGroup(Tab, true);
    FTab = Tab;
  }

  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TFarDialogItem * Item = GetItem(Index);
    if ((Item->GetGroup() == Tab) && Item->CanFocus())
    {
      Item->SetFocus();
      break;
    }
  }

  if (FOrigCaption.IsEmpty())
  {
    FOrigCaption = GetCaption();
  }
  SetCaption(FORMAT("%s - %s", GetTabName(Tab), FOrigCaption));
}

TTabButton * TTabbedDialog::GetTabButton(int32_t Tab) const
{
  TTabButton * Result = nullptr;
  for (int32_t Index = 0; Index < GetItemCount(); ++Index)
  {
    TObject * Item = GetItem(Index);
    if (rtti::isa<TTabButton>(Item))
    {
      TTabButton * T = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index));
      if ((T != nullptr) && (T->GetTab() == Tab))
      {
        Result = T;
        break;
      }
    }
  }

  DebugAssert(Result != nullptr);

  return Result;
}

UnicodeString TTabbedDialog::GetTabName(int32_t Tab) const
{
  return GetTabButton(Tab)->GetTabName();
}

void TTabbedDialog::TabButtonClick(TFarButton * Sender, bool & Close)
{
  const TTabButton * Tab = rtti::dyn_cast_or_null<TTabButton>(Sender);
  DebugAssert(Tab != nullptr);

  // HideTabs();
  if (Tab)
    SelectTab(Tab->GetTab());

  Close = false;
}

bool TTabbedDialog::Key(TFarDialogItem * /*Item*/, intptr_t KeyCode)
{
  bool Result = false;
  const WORD Key = KeyCode & 0xFFFF;
  const WORD ControlState = nb::ToWord(KeyCode >> 16);
  if ((((Key == VK_NEXT) || (Key == VK_NUMPAD3)) && (ControlState & CTRLMASK) != 0) ||
    (((Key == VK_PRIOR) || (Key == VK_NUMPAD9)) && (ControlState & CTRLMASK) != 0))
  {
    int32_t NewTab = FTab;
    do
    {
      if ((Key == VK_NEXT) || (Key == VK_NUMPAD3))
      {
        NewTab = NewTab == FTabCount - 1 ? 1 : NewTab + 1;
      }
      else
      {
        NewTab = NewTab == 1 ? FTabCount - 1 : NewTab - 1;
      }
    }
    while (!GetTabButton(NewTab)->GetEnabled());
    SelectTab(NewTab);
    Result = true;
  }
  return Result;
}

TTabButton::TTabButton(TTabbedDialog * Dialog) :
  TFarButton(OBJECT_CLASS_TTabButton, Dialog),
  FTab(0)
{
  TFarButton::SetCenterGroup(true);
  TFarButton::SetOnClick(nb::bind(&TTabbedDialog::TabButtonClick, Dialog));
}

void TTabButton::SetTabName(const UnicodeString & AValue)
{
  if (FTabName != AValue)
  {
    UnicodeString Value = AValue;
    UnicodeString C;
    const int32_t P = ::Pos(Value, L"|");
    if (P > 0)
    {
      C = Value.SubString(1, P - 1);
      Value.Delete(1, P);
    }
    else
    {
      C = Value;
    }
    SetCaption(C);
    FTabName = ::StripHotkey(Value);
  }
}

bool TWinSCPPlugin::ConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(67, 22));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_INTERFACE))));

  TFarCheckBox * DisksMenuCheck = new TFarCheckBox(Dialog);
  DisksMenuCheck->SetCaption(GetMsg(NB_CONFIG_DISKS_MENU));

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * PluginsMenuCheck = new TFarCheckBox(Dialog);
  PluginsMenuCheck->SetCaption(GetMsg(NB_CONFIG_PLUGINS_MENU));

  TFarCheckBox * PluginsMenuCommandsCheck = new TFarCheckBox(Dialog);
  PluginsMenuCommandsCheck->SetCaption(GetMsg(NB_CONFIG_PLUGINS_MENU_COMMANDS));

  TFarCheckBox * SessionNameInTitleCheck = new TFarCheckBox(Dialog);
  SessionNameInTitleCheck->SetCaption(GetMsg(NB_CONFIG_SESSION_NAME_IN_TITLE));

  new TFarSeparator(Dialog);

  TFarText * Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_CONFIG_COMMAND_PREFIXES));

  TFarEdit * CommandPrefixesEdit = new TFarEdit(Dialog);

  new TFarSeparator(Dialog);

  TFarCheckBox * CustomPanelCheck = new TFarCheckBox(Dialog);
  CustomPanelCheck->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_CHECK));
  CustomPanelCheck->SetEnabled(true);

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft() + 4);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_TYPES));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelTypesEdit = new TFarEdit(Dialog);
  CustomPanelTypesEdit->SetEnabledDependency(CustomPanelCheck);
  CustomPanelTypesEdit->SetWidth(CustomPanelTypesEdit->GetWidth() / 2 - 1);

  Dialog->SetNextItemPosition(ipRight);

  Text = new TFarText(Dialog);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->Move(0, -1);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_STATUS_TYPES));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelStatusTypesEdit = new TFarEdit(Dialog);
  CustomPanelStatusTypesEdit->SetEnabledDependency(CustomPanelCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft() + 4);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_WIDTHS));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelWidthsEdit = new TFarEdit(Dialog);
  CustomPanelWidthsEdit->SetEnabledDependency(CustomPanelCheck);
  CustomPanelWidthsEdit->SetWidth(CustomPanelTypesEdit->GetWidth());

  Dialog->SetNextItemPosition(ipRight);

  Text = new TFarText(Dialog);
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->Move(0, -1);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_STATUS_WIDTHS));

  Dialog->SetNextItemPosition(ipBelow);

  TFarEdit * CustomPanelStatusWidthsEdit = new TFarEdit(Dialog);
  CustomPanelStatusWidthsEdit->SetEnabledDependency(CustomPanelCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * CustomPanelFullScreenCheck = new TFarCheckBox(Dialog);
  CustomPanelFullScreenCheck->SetLeft(CustomPanelFullScreenCheck->GetLeft() + 4);
  CustomPanelFullScreenCheck->SetEnabledDependency(CustomPanelCheck);
  CustomPanelFullScreenCheck->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_FULL_SCREEN));

  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft());
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_HINT));
  Text = new TFarText(Dialog);
  Text->SetLeft(Text->GetLeft());
  Text->SetEnabledDependency(CustomPanelCheck);
  Text->SetCaption(GetMsg(NB_CONFIG_PANEL_MODE_HINT2));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  DisksMenuCheck->SetChecked(FarConfiguration->GetDisksMenu());
  PluginsMenuCheck->SetChecked(FarConfiguration->GetPluginsMenu());
  PluginsMenuCommandsCheck->SetChecked(FarConfiguration->GetPluginsMenuCommands());
  SessionNameInTitleCheck->SetChecked(FarConfiguration->GetSessionNameInTitle());
  CommandPrefixesEdit->SetText(FarConfiguration->GetCommandPrefixes());

  CustomPanelCheck->SetChecked(FarConfiguration->GetCustomPanelModeDetailed());
  CustomPanelTypesEdit->SetText(FarConfiguration->GetColumnTypesDetailed());
  CustomPanelWidthsEdit->SetText(FarConfiguration->GetColumnWidthsDetailed());
  CustomPanelStatusTypesEdit->SetText(FarConfiguration->GetStatusColumnTypesDetailed());
  CustomPanelStatusWidthsEdit->SetText(FarConfiguration->GetStatusColumnWidthsDetailed());
  CustomPanelFullScreenCheck->SetChecked(FarConfiguration->GetFullScreenDetailed());

  const bool Result = (Dialog->ShowModal() == brOK);
  if (Result)
  {
    FarConfiguration->SetDisksMenu(DisksMenuCheck->GetChecked());
    FarConfiguration->SetPluginsMenu(PluginsMenuCheck->GetChecked());
    FarConfiguration->SetPluginsMenuCommands(PluginsMenuCommandsCheck->GetChecked());
    FarConfiguration->SetSessionNameInTitle(SessionNameInTitleCheck->GetChecked());

    FarConfiguration->SetCommandPrefixes(CommandPrefixesEdit->GetText());

    FarConfiguration->SetCustomPanelModeDetailed(CustomPanelCheck->GetChecked());
    FarConfiguration->SetColumnTypesDetailed(CustomPanelTypesEdit->GetText());
    FarConfiguration->SetColumnWidthsDetailed(CustomPanelWidthsEdit->GetText());
    FarConfiguration->SetStatusColumnTypesDetailed(CustomPanelStatusTypesEdit->GetText());
    FarConfiguration->SetStatusColumnWidthsDetailed(CustomPanelStatusWidthsEdit->GetText());
    FarConfiguration->SetFullScreenDetailed(CustomPanelFullScreenCheck->GetChecked());
  }
  return Result;
}

bool TWinSCPPlugin::PanelConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> Dialog(std::make_unique<TWinSCPDialog>(this));
  Dialog->SetSize(TPoint(65, 7));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_PANEL))));

  TFarCheckBox * AutoReadDirectoryAfterOpCheck = new TFarCheckBox(Dialog.get());
  AutoReadDirectoryAfterOpCheck->SetCaption(GetMsg(NB_CONFIG_AUTO_READ_DIRECTORY_AFTER_OP));

  Dialog->AddStandardButtons();

  AutoReadDirectoryAfterOpCheck->SetChecked(GetConfiguration()->GetAutoReadDirectoryAfterOp());

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      GetConfiguration()->SetAutoReadDirectoryAfterOp(AutoReadDirectoryAfterOpCheck->GetChecked());
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

bool TWinSCPPlugin::LoggingConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(65, 15));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_LOGGING))));

  TFarCheckBox * LoggingCheck = new TFarCheckBox(Dialog);
  LoggingCheck->SetCaption(GetMsg(NB_LOGGING_ENABLE));

  TFarSeparator * Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(NB_LOGGING_OPTIONS_GROUP));

  TFarText * Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_LOGGING_LOG_PROTOCOL));
  Text->SetEnabledDependency(LoggingCheck);

  Dialog->SetNextItemPosition(ipRight);

  TFarComboBox * LogProtocolCombo = new TFarComboBox(Dialog);
  LogProtocolCombo->SetDropDownList(true);
  LogProtocolCombo->SetWidth(10);
  for (int32_t Index = 0; Index <= 2; ++Index)
  {
    LogProtocolCombo->GetItems()->Add(GetMsg(NB_LOGGING_LOG_PROTOCOL_0 + Index));
  }
  LogProtocolCombo->SetEnabledDependency(LoggingCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  new TFarSeparator(Dialog);

  TFarCheckBox * LogToFileCheck = new TFarCheckBox(Dialog);
  LogToFileCheck->SetCaption(GetMsg(NB_LOGGING_LOG_TO_FILE));
  LogToFileCheck->SetEnabledDependency(LoggingCheck);

  TFarEdit * LogFileNameEdit = new TFarEdit(Dialog);
  LogFileNameEdit->SetLeft(LogFileNameEdit->GetLeft() + 4);
  LogFileNameEdit->SetHistory(LOG_FILE_HISTORY);
  LogFileNameEdit->SetEnabledDependency(LogToFileCheck);

  Dialog->SetNextItemPosition(ipBelow);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_LOGGING_LOG_FILE_HINT1));
  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_LOGGING_LOG_FILE_HINT2));

  TFarRadioButton * LogFileAppendButton = new TFarRadioButton(Dialog);
  LogFileAppendButton->SetCaption(GetMsg(NB_LOGGING_LOG_FILE_APPEND));
  LogFileAppendButton->SetEnabledDependency(LogToFileCheck);

  Dialog->SetNextItemPosition(ipRight);

  TFarRadioButton * LogFileOverwriteButton = new TFarRadioButton(Dialog);
  LogFileOverwriteButton->SetCaption(GetMsg(NB_LOGGING_LOG_FILE_OVERWRITE));
  LogFileOverwriteButton->SetEnabledDependency(LogToFileCheck);

  Dialog->AddStandardButtons();

  LoggingCheck->SetChecked(GetConfiguration()->GetLogging());
  LogProtocolCombo->SetItemIndex(GetConfiguration()->GetLogProtocol());
  LogToFileCheck->SetChecked(GetConfiguration()->GetLogToFile());
  LogFileNameEdit->SetText(
    (!GetConfiguration()->GetLogToFile() && GetConfiguration()->GetLogFileName().IsEmpty()) ?
    ::IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"&s.log" :
    GetConfiguration()->GetLogFileName());
  LogFileAppendButton->SetChecked(GetConfiguration()->GetLogFileAppend());
  LogFileOverwriteButton->SetChecked(!GetConfiguration()->GetLogFileAppend());

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      GetConfiguration()->SetLogging(LoggingCheck->GetChecked());
      GetConfiguration()->SetLogProtocol(LogProtocolCombo->GetItemIndex());
      //GetConfiguration()->SetLogToFile(LogToFileCheck->GetChecked());
      if (LogToFileCheck->GetChecked())
      {
        GetConfiguration()->SetLogFileName(LogFileNameEdit->GetText());
      }
      GetConfiguration()->SetLogFileAppend(LogFileAppendButton->GetChecked());
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

bool TWinSCPPlugin::TransferConfigurationDialog()
{
  const UnicodeString Caption = FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_TRANSFER)));

  TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  const bool Result = CopyParamDialog(Caption, CopyParam, 0);
  if (Result)
  {
    GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);
  }

  return Result;
}

bool TWinSCPPlugin::EnduranceConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(76, 13));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_ENDURANCE))));

  TFarSeparator * Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(NB_TRANSFER_RESUME));

  TFarRadioButton * ResumeOnButton = new TFarRadioButton(Dialog);
  ResumeOnButton->SetCaption(GetMsg(NB_TRANSFER_RESUME_ON));

  TFarRadioButton * ResumeSmartButton = new TFarRadioButton(Dialog);
  ResumeSmartButton->SetCaption(GetMsg(NB_TRANSFER_RESUME_SMART));
  const int32_t ResumeThresholdLeft = ResumeSmartButton->GetRight();

  TFarRadioButton * ResumeOffButton = new TFarRadioButton(Dialog);
  ResumeOffButton->SetCaption(GetMsg(NB_TRANSFER_RESUME_OFF));

  TFarEdit * ResumeThresholdEdit = new TFarEdit(Dialog);
  ResumeThresholdEdit->Move(0, -2);
  ResumeThresholdEdit->SetLeft(ResumeThresholdLeft + 3);
  ResumeThresholdEdit->SetFixed(true);
  ResumeThresholdEdit->SetMask(L"9999999");
  ResumeThresholdEdit->SetWidth(9);
  ResumeThresholdEdit->SetEnabledDependency(ResumeSmartButton);

  Dialog->SetNextItemPosition(ipRight);

  TFarText * Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_TRANSFER_RESUME_THRESHOLD_UNIT));
  Text->SetEnabledDependency(ResumeSmartButton);

  Dialog->SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(Dialog);
  Separator->SetCaption(GetMsg(NB_TRANSFER_SESSION_REOPEN_GROUP));
  Separator->Move(0, 1);

  TFarCheckBox * SessionReopenAutoCheck = new TFarCheckBox(Dialog);
  SessionReopenAutoCheck->SetCaption(GetMsg(NB_TRANSFER_SESSION_REOPEN_AUTO_LABEL));

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * SessionReopenAutoEdit = new TFarEdit(Dialog);
  SessionReopenAutoEdit->SetEnabledDependency(SessionReopenAutoCheck);
  SessionReopenAutoEdit->SetFixed(true);
  SessionReopenAutoEdit->SetMask(L"999");
  SessionReopenAutoEdit->SetWidth(5);
  SessionReopenAutoEdit->Move(12, 0);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_TRANSFER_SESSION_REOPEN_AUTO_LABEL2));
  Text->SetEnabledDependency(SessionReopenAutoCheck);

  Dialog->SetNextItemPosition(ipNewLine);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL));
  Text->SetEnabledDependency(SessionReopenAutoCheck);
  Text->Move(4, 0);

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * SessionReopenNumberOfRetriesEdit = new TFarEdit(Dialog);
  SessionReopenNumberOfRetriesEdit->SetEnabledDependency(SessionReopenAutoCheck);
  SessionReopenNumberOfRetriesEdit->SetFixed(true);
  SessionReopenNumberOfRetriesEdit->SetMask(L"999");
  SessionReopenNumberOfRetriesEdit->SetWidth(5);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_TRANSFER_SESSION_REOPEN_NUMBER_OF_RETRIES_LABEL2));
  Text->SetEnabledDependency(SessionReopenAutoCheck);

  Dialog->AddStandardButtons();

  TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();
  ResumeOnButton->SetChecked(CopyParam.GetResumeSupport() == rsOn);
  ResumeSmartButton->SetChecked(CopyParam.GetResumeSupport() == rsSmart);
  ResumeOffButton->SetChecked(CopyParam.GetResumeSupport() == rsOff);
  ResumeThresholdEdit->SetAsInteger(
    nb::ToInt32(CopyParam.GetResumeThreshold() / 1024));

  SessionReopenAutoCheck->SetChecked((GetConfiguration()->GetSessionReopenAuto() > 0));
  SessionReopenAutoEdit->SetAsInteger((GetConfiguration()->GetSessionReopenAuto() > 0 ?
      (GetConfiguration()->GetSessionReopenAuto() / 1000) : 5));
  const int32_t Value = GetConfiguration()->GetSessionReopenAutoMaximumNumberOfRetries();
  SessionReopenNumberOfRetriesEdit->SetAsInteger(((Value < 0) || (Value > 99)) ?
    CONST_DEFAULT_NUMBER_OF_RETRIES : Value);

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      if (ResumeOnButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsOn);
      }
      if (ResumeSmartButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsSmart);
      }
      if (ResumeOffButton->GetChecked())
      {
        CopyParam.SetResumeSupport(rsOff);
      }
      CopyParam.SetResumeThreshold(ResumeThresholdEdit->GetAsInteger() * 1024);

      GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);

      GetConfiguration()->SetSessionReopenAuto(
        (SessionReopenAutoCheck->GetChecked() ? (SessionReopenAutoEdit->GetAsInteger() * 1000) : 0));
      GetConfiguration()->SetSessionReopenAutoMaximumNumberOfRetries(
        (SessionReopenAutoCheck->GetChecked() ? SessionReopenNumberOfRetriesEdit->GetAsInteger() : CONST_DEFAULT_NUMBER_OF_RETRIES));
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

bool TWinSCPPlugin::QueueConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(76, 11));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_BACKGROUND))));

  TFarText * Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_TRANSFER_QUEUE_LIMIT));

  Dialog->SetNextItemPosition(ipRight);

  TFarEdit * QueueTransferLimitEdit = new TFarEdit(Dialog);
  QueueTransferLimitEdit->SetFixed(true);
  QueueTransferLimitEdit->SetMask(L"9");
  QueueTransferLimitEdit->SetWidth(3);

  Dialog->SetNextItemPosition(ipNewLine);

  TFarCheckBox * QueueCheck = new TFarCheckBox(Dialog);
  QueueCheck->SetCaption(GetMsg(NB_TRANSFER_QUEUE_DEFAULT));

  TFarCheckBox * QueueAutoPopupCheck = new TFarCheckBox(Dialog);
  QueueAutoPopupCheck->SetCaption(GetMsg(NB_TRANSFER_AUTO_POPUP));

  TFarCheckBox * RememberPasswordCheck = new TFarCheckBox(Dialog);
  RememberPasswordCheck->SetCaption(GetMsg(NB_TRANSFER_REMEMBER_PASSWORD));

  TFarCheckBox * QueueBeepCheck = new TFarCheckBox(Dialog);
  QueueBeepCheck->SetCaption(GetMsg(NB_TRANSFER_QUEUE_BEEP));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  QueueTransferLimitEdit->SetAsInteger(FarConfiguration->QueueTransfersLimit());
  QueueCheck->SetChecked(FarConfiguration->GetDefaultCopyParam().GetQueue());
  QueueAutoPopupCheck->SetChecked(FarConfiguration->GetQueueAutoPopup());
  RememberPasswordCheck->SetChecked(GetGUIConfiguration()->GetSessionRememberPassword());
  QueueBeepCheck->SetChecked(FarConfiguration->GetQueueBeep());

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      TGUICopyParamType & CopyParam = GetGUIConfiguration()->GetDefaultCopyParam();

      FarConfiguration->SetQueueTransfersLimit(QueueTransferLimitEdit->GetAsInteger());
      CopyParam.SetQueue(QueueCheck->GetChecked());
      FarConfiguration->SetQueueAutoPopup(QueueAutoPopupCheck->GetChecked());
      GetGUIConfiguration()->SetSessionRememberPassword(RememberPasswordCheck->GetChecked());
      FarConfiguration->SetQueueBeep(QueueBeepCheck->GetChecked());

      GetGUIConfiguration()->SetDefaultCopyParam(CopyParam);
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

class TTransferEditorConfigurationDialog final : public TWinSCPDialog
{
public:
  explicit TTransferEditorConfigurationDialog(TCustomFarPlugin * AFarPlugin);

  bool Execute();

protected:
  virtual void Change() override;

private:
  void UpdateControls();

private:
  TFarCheckBox * EditorMultipleCheck{nullptr};
  TFarCheckBox * EditorUploadOnSaveCheck{nullptr};
  TFarRadioButton * EditorDownloadDefaultButton{nullptr};
  TFarRadioButton * EditorDownloadOptionsButton{nullptr};
  TFarRadioButton * EditorUploadSameButton{nullptr};
  TFarRadioButton * EditorUploadOptionsButton{nullptr};
};

TTransferEditorConfigurationDialog::TTransferEditorConfigurationDialog(
  TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin)
{
  SetSize(TPoint(65, 14));
  SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_TRANSFER_EDITOR))));

  EditorMultipleCheck = new TFarCheckBox(this);
  EditorMultipleCheck->SetCaption(GetMsg(NB_TRANSFER_EDITOR_MULTIPLE));

  EditorUploadOnSaveCheck = new TFarCheckBox(this);
  EditorUploadOnSaveCheck->SetCaption(GetMsg(NB_TRANSFER_EDITOR_UPLOAD_ON_SAVE));

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_TRANSFER_EDITOR_DOWNLOAD));

  EditorDownloadDefaultButton = new TFarRadioButton(this);
  EditorDownloadDefaultButton->SetCaption(GetMsg(NB_TRANSFER_EDITOR_DOWNLOAD_DEFAULT));

  EditorDownloadOptionsButton = new TFarRadioButton(this);
  EditorDownloadOptionsButton->SetCaption(GetMsg(NB_TRANSFER_EDITOR_DOWNLOAD_OPTIONS));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_TRANSFER_EDITOR_UPLOAD));

  EditorUploadSameButton = new TFarRadioButton(this);
  EditorUploadSameButton->SetCaption(GetMsg(NB_TRANSFER_EDITOR_UPLOAD_SAME));

  EditorUploadOptionsButton = new TFarRadioButton(this);
  EditorUploadOptionsButton->SetCaption(GetMsg(NB_TRANSFER_EDITOR_UPLOAD_OPTIONS));

  AddStandardButtons();
}

bool TTransferEditorConfigurationDialog::Execute()
{
  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  EditorDownloadDefaultButton->SetChecked(FarConfiguration->GetEditorDownloadDefaultMode());
  EditorDownloadOptionsButton->SetChecked(!FarConfiguration->GetEditorDownloadDefaultMode());
  EditorUploadSameButton->SetChecked(FarConfiguration->GetEditorUploadSameOptions());
  EditorUploadOptionsButton->SetChecked(!FarConfiguration->GetEditorUploadSameOptions());
  EditorUploadOnSaveCheck->SetChecked(FarConfiguration->GetEditorUploadOnSave());
  EditorMultipleCheck->SetChecked(FarConfiguration->GetEditorMultiple());

  const bool Result = (ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      FarConfiguration->SetEditorDownloadDefaultMode(EditorDownloadDefaultButton->GetChecked());
      FarConfiguration->SetEditorUploadSameOptions(EditorUploadSameButton->GetChecked());
      FarConfiguration->SetEditorUploadOnSave(EditorUploadOnSaveCheck->GetChecked());
      FarConfiguration->SetEditorMultiple(EditorMultipleCheck->GetChecked());
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }

  return Result;
}

void TTransferEditorConfigurationDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    LockChanges();
    try__finally
    {
      UpdateControls();
    }
    __finally
    {
      UnlockChanges();
    } end_try__finally
  }
}

void TTransferEditorConfigurationDialog::UpdateControls()
{
  EditorDownloadDefaultButton->SetEnabled(!EditorMultipleCheck->GetChecked());
  EditorDownloadOptionsButton->SetEnabled(EditorDownloadDefaultButton->GetEnabled());

  EditorUploadSameButton->SetEnabled(
    !EditorMultipleCheck->GetChecked() && !EditorUploadOnSaveCheck->GetChecked());
  EditorUploadOptionsButton->SetEnabled(EditorUploadSameButton->GetEnabled());
}

bool TWinSCPPlugin::TransferEditorConfigurationDialog()
{
  std::unique_ptr<TTransferEditorConfigurationDialog> Dialog(std::make_unique<TTransferEditorConfigurationDialog>(this));
  const bool Result = Dialog->Execute();
  return Result;
}

bool TWinSCPPlugin::ConfirmationsConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(67, 10));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_CONFIRMATIONS))));

  TFarCheckBox * ConfirmOverwritingCheck = new TFarCheckBox(Dialog);
  ConfirmOverwritingCheck->SetAllowGrayed(true);
  ConfirmOverwritingCheck->SetCaption(GetMsg(NB_CONFIRMATIONS_CONFIRM_OVERWRITING));

  TFarCheckBox * ConfirmCommandSessionCheck = new TFarCheckBox(Dialog);
  ConfirmCommandSessionCheck->SetCaption(GetMsg(NB_CONFIRMATIONS_OPEN_COMMAND_SESSION));

  TFarCheckBox * ConfirmResumeCheck = new TFarCheckBox(Dialog);
  ConfirmResumeCheck->SetCaption(GetMsg(NB_CONFIRMATIONS_CONFIRM_RESUME));

  TFarCheckBox * ConfirmSynchronizedBrowsingCheck = new TFarCheckBox(Dialog);
  ConfirmSynchronizedBrowsingCheck->SetCaption(GetMsg(NB_CONFIRMATIONS_SYNCHRONIZED_BROWSING));

  Dialog->AddStandardButtons();

  TFarConfiguration * FarConfiguration = GetFarConfiguration();
  ConfirmOverwritingCheck->SetSelected(!FarConfiguration->GetConfirmOverwritingOverride() ?
    BSTATE_3STATE : (GetConfiguration()->GetConfirmOverwriting() ? BSTATE_CHECKED : BSTATE_UNCHECKED));
  ConfirmCommandSessionCheck->SetChecked(GetGUIConfiguration()->GetConfirmCommandSession());
  ConfirmResumeCheck->SetChecked(GetGUIConfiguration()->GetConfirmResume());
  ConfirmSynchronizedBrowsingCheck->SetChecked(FarConfiguration->GetConfirmSynchronizedBrowsing());

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      FarConfiguration->SetConfirmOverwritingOverride(
        ConfirmOverwritingCheck->GetSelected() != BSTATE_3STATE);
      GetGUIConfiguration()->SetConfirmCommandSession(ConfirmCommandSessionCheck->GetChecked());
      GetGUIConfiguration()->SetConfirmResume(ConfirmResumeCheck->GetChecked());
      if (FarConfiguration->GetConfirmOverwritingOverride())
      {
        GetConfiguration()->SetConfirmOverwriting(ConfirmOverwritingCheck->GetChecked());
      }
      FarConfiguration->SetConfirmSynchronizedBrowsing(ConfirmSynchronizedBrowsingCheck->GetChecked());
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

bool TWinSCPPlugin::IntegrationConfigurationDialog()
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(65, 14));
  Dialog->SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_INTEGRATION))));

  TFarText * Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_INTEGRATION_PUTTY));

  TFarEdit * PuttyPathEdit = new TFarEdit(Dialog);

  TFarCheckBox * PuttyPasswordCheck = new TFarCheckBox(Dialog);
  PuttyPasswordCheck->SetCaption(GetMsg(NB_INTEGRATION_PUTTY_PASSWORD));
  PuttyPasswordCheck->SetEnabledDependency(PuttyPathEdit);

  TFarCheckBox * TelnetForFtpInPuttyCheck = new TFarCheckBox(Dialog);
  TelnetForFtpInPuttyCheck->SetCaption(GetMsg(NB_INTEGRATION_TELNET_FOR_FTP_IN_PUTTY));
  TelnetForFtpInPuttyCheck->SetEnabledDependency(PuttyPathEdit);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_INTEGRATION_PAGEANT));

  TFarEdit * PageantPathEdit = new TFarEdit(Dialog);

  Text = new TFarText(Dialog);
  Text->SetCaption(GetMsg(NB_INTEGRATION_PUTTYGEN));

  TFarEdit * PuttygenPathEdit = new TFarEdit(Dialog);

  Dialog->AddStandardButtons();

  PuttyPathEdit->SetText(GetGUIConfiguration()->GetPuttyPath());
  PuttyPasswordCheck->SetChecked(GetGUIConfiguration()->GetPuttyPassword());
  TelnetForFtpInPuttyCheck->SetChecked(GetGUIConfiguration()->GetTelnetForFtpInPutty());
  PageantPathEdit->SetText(GetFarConfiguration()->GetPageantPath());
  PuttygenPathEdit->SetText(GetFarConfiguration()->GetPuttygenPath());

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    GetConfiguration()->BeginUpdate();
    try__finally
    {
      GetGUIConfiguration()->SetPuttyPath(PuttyPathEdit->GetText());
      GetGUIConfiguration()->SetPuttyPassword(PuttyPasswordCheck->GetChecked());
      GetGUIConfiguration()->SetTelnetForFtpInPutty(TelnetForFtpInPuttyCheck->GetChecked());
      GetFarConfiguration()->SetPageantPath(PageantPathEdit->GetText());
      GetFarConfiguration()->SetPuttygenPath(PuttygenPathEdit->GetText());
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

class TAboutDialog final : public TFarDialog
{
public:
  explicit TAboutDialog(TCustomFarPlugin * AFarPlugin);

private:
  void UrlButtonClick(TFarButton * Sender, bool & Close);
  void UrlTextClick(TFarDialogItem * Item, MOUSE_EVENT_RECORD * Event);
};

UnicodeString ReplaceCopyright(const UnicodeString & S)
{
  return ::StringReplaceAll(S, L"©", L"(c)");
}

TAboutDialog::TAboutDialog(TCustomFarPlugin * AFarPlugin) :
  TFarDialog(AFarPlugin)
{
  TFarDialog::InitDialog();
  // UnicodeString ProductName = GetConfiguration()->GetFileInfoString("ProductName");
  const UnicodeString ProductName = LoadStr(WINSCPFAR_NAME);
  const UnicodeString Comments = GetConfiguration()->GetFileInfoString("Comments");
  const UnicodeString LegalCopyright = GetConfiguration()->GetFileInfoString("LegalCopyright");
  const UnicodeString FileDescription = GetConfiguration()->GetFileInfoString("FileDescription");

  int32_t Height = 16;
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
  SetSize(TPoint(60, Height));

  SetCaption(FORMAT("%s - %s",
    GetMsg(NB_PLUGIN_TITLE), ::StripHotkey(GetMsg(NB_CONFIG_ABOUT))));
  TFarText * Text;
  Text = new TFarText(this);
  Text->SetCaption(FileDescription);
  Text->SetCenterGroup(true);

  const UnicodeString PluginDescriptionText = GetMsg(NB_StringPluginDescriptionText);
  Text = new TFarText(this);
  Text->SetCaption(PluginDescriptionText);
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  UnicodeString VersionStr = GetConfiguration()->GetProductVersion();
  if (VersionStr.IsEmpty())
  {
    VersionStr = PLUGIN_VERSION_TXT;
  }
  Text->SetCaption(FORMAT(GetMsg(NB_ABOUT_VERSION), VersionStr, NETBOX_VERSION_BUILD));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->Move(0, 1);
  Text->SetCaption(LoadStr(WINSCPFAR_BASED_ON));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(FMTLOAD(WINSCPFAR_BASED_VERSION, LoadStr(WINSCPFAR_VERSION)));
  Text->SetCenterGroup(true);

  if (!ProductName.IsEmpty())
  {
    Text = new TFarText(this);
    Text->SetCaption(FORMAT(GetMsg(NB_ABOUT_PRODUCT_VERSION),
      ProductName,
      LoadStr(WINSCP_VERSION)));
    Text->SetCenterGroup(true);
  }

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(WINSCPFAR_BASED_COPYRIGHT));
  Text->SetCenterGroup(true);

  if (!Comments.IsEmpty())
  {
    Text = new TFarText(this);
    if (ProductName.IsEmpty())
    {
      Text->Move(0, 1);
    }
    Text->SetCaption(Comments);
    Text->SetCenterGroup(true);
  }
#if 0
  if (!LegalCopyright.IsEmpty())
  {
    Text = new TFarText(this);
    Text->Move(0, 1);
    Text->SetCaption(GetConfiguration()->GetFileInfoString("LegalCopyright"));
    Text->SetCenterGroup(true);
  }

  Text = new TFarText(this);
  if (LegalCopyright.IsEmpty())
  {
    Text->Move(0, 1);
  }
  Text->SetCaption(GetMsg(NB_ABOUT_URL));
  // FIXME Text->SetColor(ToInt((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09));
  Text->SetCenterGroup(true);
  Text->SetOnMouseClick(nb::bind(&TAboutDialog::UrlTextClick, this));

  TFarButton * Button = new TFarButton(this);
  Button->Move(0, 1);
  Button->SetCaption(GetMsg(NB_ABOUT_HOMEPAGE));
  Button->SetOnClick(nb::bind(&TAboutDialog::UrlButtonClick, this));
  Button->SetTag(1);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_ABOUT_FORUM));
  Button->SetOnClick(nb::bind(&TAboutDialog::UrlButtonClick, this));
  Button->SetTag(2);
  Button->SetCenterGroup(true);
  SetNextItemPosition(ipNewLine);
#endif

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(FMTLOAD(PUTTY_BASED_ON, LoadStr(PUTTY_VERSION)));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(PUTTY_COPYRIGHT));
  Text->SetCenterGroup(true);

#ifndef NO_FILEZILLA
  Text = new TFarText(this);
  Text->SetCaption(LoadStr(FILEZILLA_BASED_ON2));
  Text->SetCenterGroup(true);

  Text = new TFarText(this);
  Text->SetCaption(LoadStr(FILEZILLA_COPYRIGHT2));
  Text->SetCenterGroup(true);
#endif

  new TFarSeparator(this);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);
  Button->SetFocus();
}

void TAboutDialog::UrlTextClick(TFarDialogItem * /*Item*/,
  MOUSE_EVENT_RECORD * /*Event*/)
{
  UnicodeString Address = GetMsg(NB_ABOUT_URL);
  ::ShellExecute(nullptr, L"open", ToWCharPtr(Address), nullptr, nullptr, SW_SHOWNORMAL);
}

void TAboutDialog::UrlButtonClick(TFarButton * Sender, bool & /*Close*/)
{
  UnicodeString Address;
  switch (Sender->GetTag())
  {
  case 1:
    Address = GetMsg(NB_ABOUT_URL) + L"eng/docs/far";
    break;
  case 2:
    Address = GetMsg(NB_ABOUT_URL) + L"forum/";
    break;
  }
  ::ShellExecute(nullptr, L"open", ToWCharPtr(Address), nullptr, nullptr, SW_SHOWNORMAL);
}

void TWinSCPPlugin::AboutDialog()
{
  std::unique_ptr<TFarDialog> Dialog(std::make_unique<TAboutDialog>(this));
  Dialog->ShowModal();
}

class TPasswordDialog final : public TFarDialog
{
public:
  explicit TPasswordDialog(TCustomFarPlugin * AFarPlugin,
    const UnicodeString & SessionName, TPromptKind Kind, const UnicodeString & Name,
    const UnicodeString & Instructions, const TStrings * Prompts,
    bool StoredCredentialsTried) noexcept;
  virtual ~TPasswordDialog() noexcept override = default;
  bool Execute(TStrings * Results);

protected:
  virtual const UUID * GetDialogGuid() const override { return &PasswordDialogGuid; }

private:
  void ShowPromptClick(TFarButton * Sender, bool & Close);
  void GenerateLabel(const UnicodeString & ACaption, bool & Truncated);
  TFarEdit * GenerateEdit(bool Echo);
  void GeneratePrompt(bool ShowSavePassword,
    const UnicodeString & Instructions, const TStrings * Prompts, bool & Truncated);

private:
  gsl::owner<TSessionData *> FSessionData{nullptr};
  UnicodeString FPrompt;
  std::unique_ptr<TList> FEdits;
  gsl::owner<TFarCheckBox *> SavePasswordCheck{nullptr};
};

TPasswordDialog::TPasswordDialog(TCustomFarPlugin * AFarPlugin,
  const UnicodeString & SessionName, TPromptKind Kind, const UnicodeString & Name,
  const UnicodeString & Instructions, const TStrings * Prompts,
  bool /*StoredCredentialsTried*/) noexcept :
  TFarDialog(AFarPlugin),
  FEdits(std::make_unique<TList>())
{
  TFarDialog::InitDialog();
  bool ShowSavePassword = false;
  if (((Kind == pkPassword) || (Kind == pkTIS) || (Kind == pkCryptoCard) ||
      (Kind == pkKeybInteractive)) &&
    (Prompts->GetCount() == 1) && // FLAGSET(nb::ToInt32(Prompts->GetObject(0)), pupRemember) &&
    !SessionName.IsEmpty())
    // StoredCredentialsTried)
  {
    FSessionData = rtti::dyn_cast_or_null<TSessionData>(GetStoredSessions()->FindByName(SessionName));
    ShowSavePassword = (FSessionData != nullptr);
  }

  bool Truncated = false;
  GeneratePrompt(ShowSavePassword, Instructions, Prompts, Truncated);

  SetCaption(Name);

  if (ShowSavePassword)
  {
    SavePasswordCheck = new TFarCheckBox(this);
    SavePasswordCheck->SetCaption(GetMsg(NB_PASSWORD_SAVE));
  }
  else
  {
    SavePasswordCheck = nullptr;
  }

  new TFarSeparator(this);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  if (Truncated)
  {
    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(NB_PASSWORD_SHOW_PROMPT));
    Button->SetOnClick(nb::bind(&TPasswordDialog::ShowPromptClick, this));
    Button->SetCenterGroup(true);
  }

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

void TPasswordDialog::GenerateLabel(const UnicodeString & ACaption,
  bool & Truncated)
{
  UnicodeString Caption = ACaption;
  TFarText * Result = new TFarText(this);

  if (!FPrompt.IsEmpty())
  {
    FPrompt += L"\n\n";
  }
  FPrompt += Caption;

  if (GetSize().x - 10 < nb::ToInt32(Caption.Length()))
  {
    Caption.SetLength(GetSize().x - 10 - 4);
    Caption += L" ...";
    Truncated = true;
  }

  Result->SetCaption(Caption);
}

TFarEdit * TPasswordDialog::GenerateEdit(bool Echo)
{
  TFarEdit * Result = new TFarEdit(this);
  Result->SetPassword(!Echo);
  return Result;
}

void TPasswordDialog::GeneratePrompt(bool ShowSavePassword,
  const UnicodeString & Instructions, const TStrings * Prompts, bool & Truncated)
{
  FEdits->Clear();
  TPoint S = TPoint(40, ShowSavePassword ? 1 : 0);

  const int32_t X = nb::ToInt32(Instructions.Length());
  if (S.x < X)
  {
    S.x = X;
  }
  if (!Instructions.IsEmpty())
  {
    S.y += 2;
  }

  for (int32_t Index = 0; Index < Prompts->GetCount(); ++Index)
  {
    const int32_t L = nb::ToInt32(Prompts->GetString(Index).Length());
    if (S.x < L)
    {
      S.x = L;
    }
    S.y += 2;
  }

  if (S.x > 80 - 10)
  {
    S.x = 80 - 10;
  }

  SetSize(TPoint(S.x + 10, S.y + 6));

  if (!Instructions.IsEmpty())
  {
    GenerateLabel(Instructions, Truncated);
    // dumb way to add empty line
    GenerateLabel(L"", Truncated);
  }

  for (int32_t Index = 0; Index < Prompts->GetCount(); ++Index)
  {
    GenerateLabel(Prompts->GetString(Index), Truncated);

    FEdits->Add(GenerateEdit(FLAGSET(nb::ToUIntPtr(Prompts->GetObj(Index)), pupEcho)));
  }
}

void TPasswordDialog::ShowPromptClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  Ensures(WinSCPPlugin);
  WinSCPPlugin->MoreMessageDialog(FPrompt, nullptr, qtInformation, qaOK);
}

bool TPasswordDialog::Execute(TStrings * Results)
{
  for (int32_t Index = 0; Index < FEdits->GetCount(); ++Index)
  {
    FEdits->GetAs<TFarEdit>(Index)->SetText(Results->GetString(Index));
  }

  const bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    for (int32_t Index = 0; Index < FEdits->GetCount(); ++Index)
    {
      UnicodeString Text = FEdits->GetAs<TFarEdit>(Index)->GetText();
      Results->SetString(Index, Text);
    }

    if ((SavePasswordCheck != nullptr) && SavePasswordCheck->GetChecked())
    {
      DebugAssert(FSessionData != nullptr);
      FSessionData->SetPassword(Results->GetString(0));
      // modified only, explicit
      GetStoredSessions()->Save(false, true);
    }
  }
  return Result;
}

bool TWinSCPFileSystem::PasswordDialog(TSessionData * SessionData,
  TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
  TStrings * Prompts,
  TStrings * Results, bool StoredCredentialsTried)
{
  std::unique_ptr<TPasswordDialog> Dialog(std::make_unique<TPasswordDialog>(FPlugin, SessionData->GetName(),
      Kind, Name, Instructions, Prompts, StoredCredentialsTried));
  const bool Result = Dialog->Execute(Results);
  return Result;
}

bool TWinSCPFileSystem::BannerDialog(const UnicodeString & SessionName,
  const UnicodeString & Banner, bool & NeverShowAgain, int32_t Options)
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(FPlugin));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetSize(TPoint(70, 21));
  Dialog->SetCaption(FORMAT(GetMsg(NB_BANNER_TITLE), SessionName));

  TFarLister * Lister = new TFarLister(Dialog);
  FarWrapText(Banner, Lister->GetItems(), Dialog->GetBorderBox()->GetWidth() - 4);
  Lister->SetHeight(15);
  Lister->SetLeft(Dialog->GetBorderBox()->GetLeft() + 1);
  Lister->SetRight(Dialog->GetBorderBox()->GetRight() - (Lister->GetScrollBar() ? 0 : 1));

  new TFarSeparator(Dialog);

  TFarCheckBox * NeverShowAgainCheck = nullptr;
  if (FLAGCLEAR(Options, boDisableNeverShowAgain))
  {
    NeverShowAgainCheck = new TFarCheckBox(Dialog);
    NeverShowAgainCheck->SetCaption(GetMsg(NB_BANNER_NEVER_SHOW_AGAIN));
    NeverShowAgainCheck->SetVisible(FLAGCLEAR(Options, boDisableNeverShowAgain));
    NeverShowAgainCheck->SetChecked(NeverShowAgain);

    Dialog->SetNextItemPosition(ipRight);
  }

  TFarButton * Button = new TFarButton(Dialog);
  Button->SetCaption(GetMsg(NB_BANNER_CONTINUE));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  if (NeverShowAgainCheck != nullptr)
  {
    Button->SetLeft(Dialog->GetBorderBox()->GetRight() - Button->GetWidth() - 1);
  }
  else
  {
    Button->SetCenterGroup(true);
  }

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    if (NeverShowAgainCheck != nullptr)
    {
      NeverShowAgain = NeverShowAgainCheck->GetChecked();
    }
  }
  return Result;
}

class TSessionDialog final : public TTabbedDialog
{
public:
  enum TSessionTab
  {
    tabSession = 1,
    tabEnvironment,
    tabDirectories,
    tabSFTP,
    tabSCP,
    tabFTP,
    tabConnection,
    tabTunnel,
    tabProxy,
    tabSsh,
    tabKex,
    tabAuthentication,
    tabBugs,
    tabWebDAV,
    tabCount
  };

  explicit TSessionDialog(TCustomFarPlugin * AFarPlugin, TSessionActionEnum Action) noexcept;
  virtual ~TSessionDialog() noexcept override;

  bool Execute(TSessionData * SessionData, TSessionActionEnum Action);

protected:
  virtual void Change() override;
  virtual void Init() override;
  virtual bool CloseQuery() override;
  virtual void SelectTab(int32_t Tab) override;

private:
  void LoadPing(const TSessionData * SessionData);
  void SavePing(TSessionData * SessionData);
  int32_t ProxyMethodToIndex(TProxyMethod ProxyMethod, TFarList * Items) const;
  TProxyMethod IndexToProxyMethod(int32_t Index, TFarList * Items) const;
  TFarComboBox * GetProxyMethodCombo() const;
  TFarComboBox * GetOtherProxyMethodCombo() const;
  int32_t FSProtocolToIndex(TFSProtocol FSProtocol, bool & AllowScpFallback) const;
  TFSProtocol IndexToFSProtocol(int32_t Index, bool AllowScpFallback) const;
  TFSProtocol GetFSProtocol() const;
  inline int32_t GetLastSupportedFtpProxyMethod() const;
  bool GetSupportedFtpProxyMethod(int32_t Method) const;
  TProxyMethod GetProxyMethod() const;
  int32_t GetFtpProxyLogonType() const;
  TFtps IndexToFtps(int32_t Index) const;
  TFtps GetFtps() const;
  TLoginType IndexToLoginType(int32_t Index) const;
  bool VerifyKey(const UnicodeString & AFileName, bool TypeOnly);
  void PrevTabClick(TFarButton * /*Sender*/, bool & Close);
  void NextTabClick(TFarButton * /*Sender*/, bool & Close);
  void CipherButtonClick(TFarButton * Sender, bool & Close);
  void KexButtonClick(TFarButton * Sender, bool & Close);
  void AuthGSSAPICheckAllowChange(TFarDialogItem * Sender, void * NewState, bool & Allow);
  void UnixEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void WindowsEnvironmentButtonClick(TFarButton * Sender, bool & Close);
  void UpdateControls();
  void TransferProtocolComboChange();
  //void LoginTypeComboChange();
  void FillCodePageEdit();
  void CodePageEditAdd(uint32_t Cp);
  void FtpProxyMethodComboAddNewItem(int32_t ProxyTypeId, TProxyMethod ProxyType);
  void SshProxyMethodComboAddNewItem(int32_t ProxyTypeId, TProxyMethod ProxyType);
  static bool IsSshProtocol(TFSProtocol FSProtocol);
  bool IsWebDAVProtocol(TFSProtocol FSProtocol) const;
  bool IsSshOrWebDAVProtocol(TFSProtocol FSProtocol) const;

  void ChangeTabs(int32_t FirstVisibleTabIndex);
  int32_t GetVisibleTabsCount(int32_t TabIndex, bool Forward) const;
  int32_t AddTab(int32_t TabID, const UnicodeString & TabCaption);

  static TProxyMethod ToProxyMethod(int32_t Value) { return static_cast<TProxyMethod>(Value); }
private:
  TSessionActionEnum FAction;
  TSessionData * FSessionData{nullptr};
  int32_t FTransferProtocolIndex{0};
  int32_t FFtpEncryptionComboIndex{0};

  TTabButton * SshTab{nullptr};
  TTabButton * AuthenticationTab{nullptr};
  TTabButton * KexTab{nullptr};
  TTabButton * BugsTab{nullptr};
  TTabButton * WebDAVTab{nullptr};
  TTabButton * ScpTab{nullptr};
  TTabButton * SftpTab{nullptr};
  TTabButton * FtpTab{nullptr};
  TTabButton * TunnelTab{nullptr};
  TTabButton * PrevTab{nullptr};
  TTabButton * NextTab{nullptr};
  TFarButton * ConnectButton{nullptr};
  TFarEdit * HostNameEdit{nullptr};
  TFarEdit * PortNumberEdit{nullptr};
  TFarText * UserNameLabel{nullptr};
  TFarEdit * UserNameEdit{nullptr};
  TFarText * PasswordLabel{nullptr};
  TFarEdit * PasswordEdit{nullptr};
  TFarEdit * PrivateKeyEdit{nullptr};
  TFarComboBox * TransferProtocolCombo{nullptr};
  TFarCheckBox * AllowScpFallbackCheck{nullptr};
  TFarText * HostNameLabel{nullptr};
  TFarText * InsecureLabel{nullptr};
  TFarText * FtpEncryptionLabel{nullptr};
  TFarComboBox * FtpEncryptionCombo{nullptr};
  TFarCheckBox * UpdateDirectoriesCheck{nullptr};
  TFarCheckBox * CacheDirectoriesCheck{nullptr};
  TFarCheckBox * CacheDirectoryChangesCheck{nullptr};
  TFarCheckBox * PreserveDirectoryChangesCheck{nullptr};
  TFarCheckBox * ResolveSymlinksCheck{nullptr};
  TFarEdit * RemoteDirectoryEdit{nullptr};
  TFarComboBox * EOLTypeCombo{nullptr};
  TFarRadioButton * DSTModeWinCheck{nullptr};
  TFarRadioButton * DSTModeKeepCheck{nullptr};
  TFarRadioButton * DSTModeUnixCheck{nullptr};
  TFarCheckBox * CompressionCheck{nullptr};
  TFarRadioButton * SshProt1onlyButton{nullptr};
  TFarRadioButton * SshProt1Button{nullptr};
  TFarRadioButton * SshProt2Button{nullptr};
  TFarRadioButton * SshProt2onlyButton{nullptr};
  TFarListBox * CipherListBox{nullptr};
  TFarButton * CipherUpButton{nullptr};
  TFarButton * CipherDownButton{nullptr};
  TFarCheckBox * Ssh2DESCheck{nullptr};
  TFarComboBox * ShellEdit{nullptr};
  TFarComboBox * ReturnVarEdit{nullptr};
  TFarCheckBox * LookupUserGroupsCheck{nullptr};
  TFarCheckBox * ClearAliasesCheck{nullptr};
  TFarCheckBox * UnsetNationalVarsCheck{nullptr};
  TFarComboBox * ListingCommandEdit{nullptr};
  TFarCheckBox * IgnoreLsWarningsCheck{nullptr};
  TFarCheckBox * SCPLsFullTimeAutoCheck{nullptr};
  TFarCheckBox * Scp1CompatibilityCheck{nullptr};
  TFarEdit * PostLoginCommandsEdits[3]{nullptr};
  TFarEdit * TimeDifferenceEdit{nullptr};
  TFarEdit * TimeDifferenceMinutesEdit{nullptr};
  TFarEdit * TimeoutEdit{nullptr};
  TFarRadioButton * PingOffButton{nullptr};
  TFarRadioButton * PingNullPacketButton{nullptr};
  TFarRadioButton * PingDummyCommandButton{nullptr};
  TFarEdit * PingIntervalSecEdit{nullptr};
  TFarComboBox * CodePageEdit{nullptr};
  TFarComboBox * SshProxyMethodCombo{nullptr};
  TFarComboBox * FtpProxyMethodCombo{nullptr};
  TFarEdit * ProxyHostEdit{nullptr};
  TFarEdit * ProxyPortEdit{nullptr};
  TFarEdit * ProxyUsernameEdit{nullptr};
  TFarEdit * ProxyPasswordEdit{nullptr};
  TFarText * ProxyLocalCommandLabel{nullptr};
  TFarEdit * ProxyLocalCommandEdit{nullptr};
  TFarText * ProxyTelnetCommandLabel{nullptr};
  TFarEdit * ProxyTelnetCommandEdit{nullptr};
  TFarCheckBox * ProxyLocalhostCheck{nullptr};
  TFarRadioButton * ProxyDNSOffButton{nullptr};
  TFarRadioButton * ProxyDNSAutoButton{nullptr};
  TFarRadioButton * ProxyDNSOnButton{nullptr};
  TFarCheckBox * TunnelCheck{nullptr};
  TFarEdit * TunnelHostNameEdit{nullptr};
  TFarEdit * TunnelPortNumberEdit{nullptr};
  TFarEdit * TunnelUserNameEdit{nullptr};
  TFarEdit * TunnelPasswordEdit{nullptr};
  TFarEdit * TunnelPrivateKeyEdit{nullptr};
  TFarComboBox * TunnelLocalPortNumberEdit{nullptr};
//  TFarComboBox * BugIgnore1Combo{nullptr};
//  TFarComboBox * BugPlainPW1Combo{nullptr};
//  TFarComboBox * BugRSA1Combo{nullptr};
  TFarComboBox * BugHMAC2Combo{nullptr};
  TFarComboBox * BugDeriveKey2Combo{nullptr};
  TFarComboBox * BugRSAPad2Combo{nullptr};
  TFarComboBox * BugPKSessID2Combo{nullptr};
  TFarComboBox * BugRekey2Combo{nullptr};
  TFarCheckBox * SshNoUserAuthCheck{nullptr};
  TFarCheckBox * AuthTISCheck{nullptr};
  TFarCheckBox * TryAgentCheck{nullptr};
  TFarCheckBox * AuthKICheck{nullptr};
  TFarCheckBox * AuthKIPasswordCheck{nullptr};
  TFarCheckBox * AgentFwdCheck{nullptr};
  TFarCheckBox * AuthGSSAPICheck3{nullptr};
  TFarCheckBox * GSSAPIFwdTGTCheck{nullptr};
  TFarCheckBox * DeleteToRecycleBinCheck{nullptr};
  TFarCheckBox * OverwrittenToRecycleBinCheck{nullptr};
  TFarEdit * RecycleBinPathEdit{nullptr};
  TFarComboBox * SFTPMaxVersionCombo{nullptr};
  TFarComboBox * SftpServerEdit{nullptr};
  TFarComboBox * SFTPBugSymlinkCombo{nullptr};
  TFarComboBox * SFTPBugSignedTSCombo{nullptr};
  TFarListBox * KexListBox{nullptr};
  TFarButton * KexUpButton{nullptr};
  TFarButton * KexDownButton{nullptr};
  TFarEdit * SFTPMinPacketSizeEdit{nullptr};
  TFarEdit * SFTPMaxPacketSizeEdit{nullptr};
  TFarEdit * RekeyTimeEdit{nullptr};
  TFarEdit * RekeyDataEdit{nullptr};
  TFarRadioButton * IPAutoButton{nullptr};
  TFarRadioButton * IPv4Button{nullptr};
  TFarRadioButton * IPv6Button{nullptr};
  TFarCheckBox * SshBufferSizeCheck{nullptr};
  TFarComboBox * FtpUseMlsdCombo{nullptr};
  TFarCheckBox * FtpPasvModeCheck{nullptr};
  TFarCheckBox * FtpAllowEmptyPasswordCheck{nullptr};
  TFarCheckBox * FtpDupFFCheck{nullptr};
  TFarCheckBox * FtpUndupFFCheck{nullptr};
  TFarCheckBox * SslSessionReuseCheck{nullptr};
  TFarCheckBox * WebDAVCompressionCheck{nullptr};
  std::unique_ptr<TObjectList> FTabs;
  int32_t FFirstVisibleTabIndex{0};
};

//  BUG(Ignore1, NB_LOGIN_BUGS_IGNORE1, ); \
//  BUG(PlainPW1, NB_LOGIN_BUGS_PLAIN_PW1, ); \
//  BUG(RSA1, NB_LOGIN_BUGS_RSA1, ); \

#define BUG(BUGID, MSG, PREFIX) \
  TRISTATE(PREFIX ## Bug ## BUGID ## Combo, PREFIX ## Bug(sb ## BUGID), MSG)
#define BUGS() \
  BUG(HMAC2, NB_LOGIN_BUGS_HMAC2, ); \
  BUG(DeriveKey2, NB_LOGIN_BUGS_DERIVE_KEY2, ); \
  BUG(RSAPad2, NB_LOGIN_BUGS_RSA_PAD2, ); \
  BUG(PKSessID2, NB_LOGIN_BUGS_PKSESSID2, ); \
  BUG(Rekey2, NB_LOGIN_BUGS_REKEY2, )
#define SFTP_BUGS() \
  BUG(Symlink, NB_LOGIN_SFTP_BUGS_SYMLINK, SFTP); \
  BUG(SignedTS, NB_LOGIN_SFTP_BUGS_SIGNED_TS, SFTP)

static constexpr TFSProtocol FSOrder[] = { fsSFTPonly, fsSCPonly, fsFTP, fsWebDAV, fsS3 };

TSessionDialog::TSessionDialog(TCustomFarPlugin * AFarPlugin, TSessionActionEnum Action) noexcept :
  TTabbedDialog(AFarPlugin, tabCount),
  FAction(Action),
  FSessionData(nullptr),
  FTransferProtocolIndex(0),
  FFtpEncryptionComboIndex(0),
  FTabs(std::make_unique<TObjectList>()),
  FFirstVisibleTabIndex(0)
{
  TPoint S = TPoint(67, 23);
  bool Limited = (S.y > GetMaxSize().y);
  if (Limited)
  {
    S.y = GetMaxSize().y;
  }
  SetSize(S);

  FTabs->SetOwnsObjects(false);
  FFirstVisibleTabIndex = 0;

#define TRISTATE(COMBO, PROP, MSG) \
    Text = new TFarText(this); \
    Text->SetCaption(GetMsg((MSG))); \
    SetNextItemPosition(ipRight); \
    (COMBO) = new TFarComboBox(this); \
    (COMBO)->SetDropDownList(true); \
    (COMBO)->SetWidth(7); \
    (COMBO)->GetItems()->BeginUpdate(); \
    { \
      SCOPE_EXIT \
      { \
        (COMBO)->GetItems()->EndUpdate(); \
      }; \
      (COMBO)->GetItems()->Add(GetMsg(NB_LOGIN_BUGS_AUTO)); \
      (COMBO)->GetItems()->Add(GetMsg(NB_LOGIN_BUGS_OFF)); \
      (COMBO)->GetItems()->Add(GetMsg(NB_LOGIN_BUGS_ON)); \
    } \
    Text->SetEnabledFollow((COMBO)); \
    SetNextItemPosition(ipNewLine)

  TRect CRect = GetClientRect();

  int32_t Index1 = AddTab(tabSession, GetMsg(NB_LOGIN_TAB_SESSION));
  // Tab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index));

  SetNextItemPosition(ipRight);

  Index1 = AddTab(tabEnvironment, GetMsg(NB_LOGIN_TAB_ENVIRONMENT));

  Index1 = AddTab(tabDirectories, GetMsg(NB_LOGIN_TAB_DIRECTORIES));

  Index1 = AddTab(tabSFTP, GetMsg(NB_LOGIN_TAB_SFTP));
  SftpTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabSCP, GetMsg(NB_LOGIN_TAB_SCP));
  ScpTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  PrevTab = new TTabButton(this);
  PrevTab->SetTabName(UnicodeString(1, '\x11'));
  PrevTab->SetBrackets(brNone);
  PrevTab->SetCenterGroup(false);
  PrevTab->SetOnClick(nb::bind(&TSessionDialog::PrevTabClick, this));

  NextTab = new TTabButton(this);
  NextTab->SetTabName(UnicodeString(1, '\x10'));
  NextTab->SetBrackets(brNone);
  NextTab->SetCenterGroup(false);
  NextTab->SetOnClick(nb::bind(&TSessionDialog::NextTabClick, this));

  int32_t PWidth = PrevTab->GetWidth();
  int32_t NWidth = NextTab->GetWidth();
  int32_t R = S.x - 4;
  PrevTab->SetLeft(R - PWidth - NWidth - 2);
  PrevTab->SetWidth(PWidth);
  NextTab->SetLeft(R - NWidth - 1);
  NextTab->SetWidth(PWidth);

  Index1 = AddTab(tabFTP, GetMsg(NB_LOGIN_TAB_FTP));
  FtpTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabConnection, GetMsg(NB_LOGIN_TAB_CONNECTION));

  Index1 = AddTab(tabProxy, GetMsg(NB_LOGIN_TAB_PROXY));

  Index1 = AddTab(tabTunnel, GetMsg(NB_LOGIN_TAB_TUNNEL));
  TunnelTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabSsh, GetMsg(NB_LOGIN_TAB_SSH));
  SshTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabKex, GetMsg(NB_LOGIN_TAB_KEX));
  KexTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabAuthentication, GetMsg(NB_LOGIN_TAB_AUTH));
  AuthenticationTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabBugs, GetMsg(NB_LOGIN_TAB_BUGS));
  BugsTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  Index1 = AddTab(tabWebDAV, GetMsg(NB_LOGIN_TAB_WEBDAV));
  WebDAVTab = rtti::dyn_cast_or_null<TTabButton>(GetItem(Index1));

  // Session tab

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(tabSession);

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_GROUP_SESSION));
  int32_t GroupTop = Separator->GetTop();

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TRANSFER_PROTOCOL));

  SetNextItemPosition(ipRight);

  TransferProtocolCombo = new TFarComboBox(this);
  TransferProtocolCombo->SetDropDownList(true);
  TransferProtocolCombo->SetWidth(10);
  TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_SFTP));
  TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_SCP));
#ifndef NO_FILEZILLA
  TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_FTP));
#endif
  TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_WEBDAV));
  TransferProtocolCombo->GetItems()->Add(GetMsg(NB_LOGIN_S3));

  AllowScpFallbackCheck = new TFarCheckBox(this);
  AllowScpFallbackCheck->SetCaption(GetMsg(NB_LOGIN_ALLOW_SCP_FALLBACK));

  InsecureLabel = new TFarText(this);
  InsecureLabel->SetCaption(GetMsg(NB_LOGIN_INSECURE));
  InsecureLabel->MoveAt(AllowScpFallbackCheck->GetLeft(), AllowScpFallbackCheck->GetTop());

  SetNextItemPosition(ipNewLine);

  FtpEncryptionLabel = new TFarText(this);
  FtpEncryptionLabel->SetCaption(GetMsg(NB_LOGIN_FTP_ENCRYPTION));
  FtpEncryptionLabel->SetWidth(15);

  SetNextItemPosition(ipRight);

  FtpEncryptionCombo = new TFarComboBox(this);
  FtpEncryptionCombo->SetDropDownList(true);
  FtpEncryptionCombo->GetItems()->Add(GetMsg(NB_LOGIN_FTP_USE_PLAIN_FTP));
  FtpEncryptionCombo->GetItems()->Add(GetMsg(NB_LOGIN_FTP_REQUIRE_IMPLICIT_FTP));
  FtpEncryptionCombo->GetItems()->Add(GetMsg(NB_LOGIN_FTP_REQUIRE_EXPLICIT_FTP));
  FtpEncryptionCombo->SetRight(CRect.Right);
  FtpEncryptionCombo->SetWidth(30);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  HostNameLabel = new TFarText(this);
  HostNameLabel->SetCaption(GetMsg(NB_LOGIN_HOST_NAME));

  HostNameEdit = new TFarEdit(this);
  HostNameEdit->SetRight(CRect.Right - 12 - 2);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PORT_NUMBER));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  PortNumberEdit = new TFarEdit(this);
  PortNumberEdit->SetFixed(true);
  PortNumberEdit->SetMask(L"999999");

  SetNextItemPosition(ipNewLine);
  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  UserNameLabel = new TFarText(this);
  UserNameLabel->SetCaption(GetMsg(NB_LOGIN_USER_NAME));
  UserNameLabel->SetWidth(20);

  SetNextItemPosition(ipRight);

  UserNameEdit = new TFarEdit(this);
  UserNameEdit->SetWidth(20);
  UserNameEdit->SetRight(CRect.Right - 12 - 2);
  UserNameEdit->SetVisible(true);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  PasswordLabel = new TFarText(this);
  PasswordLabel->SetCaption(GetMsg(NB_LOGIN_PASSWORD));
  PasswordLabel->SetWidth(20);
  PasswordLabel->SetVisible(true);

  SetNextItemPosition(ipRight);

  PasswordEdit = new TFarEdit(this);
  PasswordEdit->SetPassword(true);
  PasswordEdit->SetWidth(20);
  PasswordEdit->SetRight(CRect.Right - 12 - 2);
  PasswordEdit->SetVisible(true);

  SetNextItemPosition(ipNewLine);
  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PRIVATE_KEY));

  PrivateKeyEdit = new TFarEdit(this);
  Text->SetEnabledFollow(PrivateKeyEdit);

  Separator = new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetTop(CRect.Bottom - 3);
  Text->SetBottom(Text->GetTop());
  Text->SetCaption(GetMsg(NB_LOGIN_TAB_HINT1));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TAB_HINT2));

  // Environment tab

  SetDefaultGroup(tabEnvironment);
  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_ENVIRONMENT_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_EOL_TYPE));

  SetNextItemPosition(ipRight);

  EOLTypeCombo = new TFarComboBox(this);
  EOLTypeCombo->SetDropDownList(true);
  EOLTypeCombo->SetWidth(7);
  EOLTypeCombo->SetRight(CRect.Right);
  EOLTypeCombo->GetItems()->Add(L"LF");
  EOLTypeCombo->GetItems()->Add(L"CR/LF");

  SetNextItemPosition(ipNewLine);

  // UTF_TRISTATE();
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_CODE_PAGE));

  SetNextItemPosition(ipRight);

  CodePageEdit = new TFarComboBox(this);
  CodePageEdit->SetWidth(30);
  CodePageEdit->SetRight(CRect.Right);
  FillCodePageEdit();

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TIME_DIFFERENCE));

  SetNextItemPosition(ipRight);

  TimeDifferenceEdit = new TFarEdit(this);
  TimeDifferenceEdit->SetFixed(true);
  TimeDifferenceEdit->SetMask(L"###");
  TimeDifferenceEdit->SetWidth(4);
  Text->SetEnabledFollow(TimeDifferenceEdit);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TIME_DIFFERENCE_HOURS));
  Text->SetEnabledFollow(TimeDifferenceEdit);

  TimeDifferenceMinutesEdit = new TFarEdit(this);
  TimeDifferenceMinutesEdit->SetFixed(true);
  TimeDifferenceMinutesEdit->SetMask(L"###");
  TimeDifferenceMinutesEdit->SetWidth(4);
  TimeDifferenceMinutesEdit->SetEnabledFollow(TimeDifferenceEdit);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TIME_DIFFERENCE_MINUTES));
  Text->SetEnabledFollow(TimeDifferenceEdit);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_DST_MODE_GROUP));

  DSTModeUnixCheck = new TFarRadioButton(this);
  DSTModeUnixCheck->SetCaption(GetMsg(NB_LOGIN_DST_MODE_UNIX));

  DSTModeWinCheck = new TFarRadioButton(this);
  DSTModeWinCheck->SetCaption(GetMsg(NB_LOGIN_DST_MODE_WIN));
  DSTModeWinCheck->SetEnabledFollow(DSTModeUnixCheck);

  DSTModeKeepCheck = new TFarRadioButton(this);
  DSTModeKeepCheck->SetCaption(GetMsg(NB_LOGIN_DST_MODE_KEEP));
  DSTModeKeepCheck->SetEnabledFollow(DSTModeUnixCheck);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_LOGIN_ENVIRONMENT_UNIX));
  Button->SetOnClick(nb::bind(&TSessionDialog::UnixEnvironmentButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_LOGIN_ENVIRONMENT_WINDOWS));
  Button->SetOnClick(nb::bind(&TSessionDialog::WindowsEnvironmentButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_RECYCLE_BIN_GROUP));

  DeleteToRecycleBinCheck = new TFarCheckBox(this);
  DeleteToRecycleBinCheck->SetCaption(GetMsg(NB_LOGIN_RECYCLE_BIN_DELETE));

  OverwrittenToRecycleBinCheck = new TFarCheckBox(this);
  OverwrittenToRecycleBinCheck->SetCaption(GetMsg(NB_LOGIN_RECYCLE_BIN_OVERWRITE));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_RECYCLE_BIN_LABEL));

  RecycleBinPathEdit = new TFarEdit(this);
  Text->SetEnabledFollow(RecycleBinPathEdit);

  SetNextItemPosition(ipNewLine);

  // Directories tab

  SetDefaultGroup(tabDirectories);
  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_DIRECTORIES_GROUP));

  UpdateDirectoriesCheck = new TFarCheckBox(this);
  UpdateDirectoriesCheck->SetCaption(GetMsg(NB_LOGIN_UPDATE_DIRECTORIES));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_REMOTE_DIRECTORY));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_DIR_HISTORY);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_DIRECTORY_OPTIONS_GROUP));

  CacheDirectoriesCheck = new TFarCheckBox(this);
  CacheDirectoriesCheck->SetCaption(GetMsg(NB_LOGIN_CACHE_DIRECTORIES));

  CacheDirectoryChangesCheck = new TFarCheckBox(this);
  CacheDirectoryChangesCheck->SetCaption(GetMsg(NB_LOGIN_CACHE_DIRECTORY_CHANGES));

  PreserveDirectoryChangesCheck = new TFarCheckBox(this);
  PreserveDirectoryChangesCheck->SetCaption(GetMsg(NB_LOGIN_PRESERVE_DIRECTORY_CHANGES));
  PreserveDirectoryChangesCheck->SetLeft(PreserveDirectoryChangesCheck->GetLeft() + 4);

  ResolveSymlinksCheck = new TFarCheckBox(this);
  ResolveSymlinksCheck->SetCaption(GetMsg(NB_LOGIN_RESOLVE_SYMLINKS));

  new TFarSeparator(this);

  // SCP Tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSCP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_SHELL_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SHELL_SHELL));

  SetNextItemPosition(ipRight);

  ShellEdit = new TFarComboBox(this);
  ShellEdit->GetItems()->Add(GetMsg(NB_LOGIN_SHELL_SHELL_DEFAULT));
  ShellEdit->GetItems()->Add(L"/bin/bash");
  ShellEdit->GetItems()->Add(L"/bin/ksh");

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SHELL_RETURN_VAR));

  SetNextItemPosition(ipRight);

  ReturnVarEdit = new TFarComboBox(this);
  ReturnVarEdit->GetItems()->Add(GetMsg(NB_LOGIN_SHELL_RETURN_VAR_AUTODETECT));
  ReturnVarEdit->GetItems()->Add(L"?");
  ReturnVarEdit->GetItems()->Add(L"status");

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_SCP_LS_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_LISTING_COMMAND));

  SetNextItemPosition(ipRight);

  ListingCommandEdit = new TFarComboBox(this);
  ListingCommandEdit->GetItems()->Add(L"ls -la");
  ListingCommandEdit->GetItems()->Add(L"ls -gla");
  Text->SetEnabledFollow(ListingCommandEdit);

  SetNextItemPosition(ipNewLine);

  IgnoreLsWarningsCheck = new TFarCheckBox(this);
  IgnoreLsWarningsCheck->SetCaption(GetMsg(NB_LOGIN_IGNORE_LS_WARNINGS));

  SetNextItemPosition(ipRight);

  SCPLsFullTimeAutoCheck = new TFarCheckBox(this);
  SCPLsFullTimeAutoCheck->SetCaption(GetMsg(NB_LOGIN_SCP_LS_FULL_TIME_AUTO));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_SCP_OPTIONS));

  LookupUserGroupsCheck = new TFarCheckBox(this);
  LookupUserGroupsCheck->SetCaption(GetMsg(NB_LOGIN_LOOKUP_USER_GROUPS));

  SetNextItemPosition(ipRight);

  UnsetNationalVarsCheck = new TFarCheckBox(this);
  UnsetNationalVarsCheck->SetCaption(GetMsg(NB_LOGIN_CLEAR_NATIONAL_VARS));

  SetNextItemPosition(ipNewLine);

  ClearAliasesCheck = new TFarCheckBox(this);
  ClearAliasesCheck->SetCaption(GetMsg(NB_LOGIN_CLEAR_ALIASES));

  SetNextItemPosition(ipRight);

  Scp1CompatibilityCheck = new TFarCheckBox(this);
  Scp1CompatibilityCheck->SetCaption(GetMsg(NB_LOGIN_SCP1_COMPATIBILITY));

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // SFTP Tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSFTP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_SFTP_PROTOCOL_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SFTP_SERVER));
  SetNextItemPosition(ipRight);
  SftpServerEdit = new TFarComboBox(this);
  SftpServerEdit->GetItems()->Add(GetMsg(NB_LOGIN_SFTP_SERVER_DEFAULT));
  SftpServerEdit->GetItems()->Add(L"/bin/sftp-server");
  SftpServerEdit->GetItems()->Add(L"sudo su -c /bin/sftp-server");
  Text->SetEnabledFollow(SftpServerEdit);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SFTP_MAX_VERSION));
  SetNextItemPosition(ipRight);
  SFTPMaxVersionCombo = new TFarComboBox(this);
  SFTPMaxVersionCombo->SetDropDownList(true);
  SFTPMaxVersionCombo->SetWidth(7);
  for (int32_t Index2 = 0; Index2 <= 6; ++Index2)
  {
    SFTPMaxVersionCombo->GetItems()->Add(::IntToStr(Index2));
  }
  Text->SetEnabledFollow(SFTPMaxVersionCombo);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_SFTP_BUGS_GROUP));

  SFTP_BUGS();

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SFTP_MIN_PACKET_SIZE));
  SetNextItemPosition(ipRight);

  SFTPMinPacketSizeEdit = new TFarEdit(this);
  SFTPMinPacketSizeEdit->SetFixed(true);
  SFTPMinPacketSizeEdit->SetMask(L"99999999");
  SFTPMinPacketSizeEdit->SetWidth(8);
  // SFTPMinPacketSizeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_SFTP_MAX_PACKET_SIZE));
  SetNextItemPosition(ipRight);

  SFTPMaxPacketSizeEdit = new TFarEdit(this);
  SFTPMaxPacketSizeEdit->SetFixed(true);
  SFTPMaxPacketSizeEdit->SetMask(L"99999999");
  SFTPMaxPacketSizeEdit->SetWidth(8);
  // SFTPMaxPacketSizeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  // FTP tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabFTP);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_FTP_GROUP));

  TRISTATE(FtpUseMlsdCombo, FtpUseMlsd, NB_LOGIN_FTP_USE_MLSD);

  FtpPasvModeCheck = new TFarCheckBox(this);
  FtpPasvModeCheck->SetCaption(GetMsg(NB_LOGIN_FTP_PASV_MODE));

  FtpAllowEmptyPasswordCheck = new TFarCheckBox(this);
  FtpAllowEmptyPasswordCheck->SetCaption(GetMsg(NB_LOGIN_FTP_ALLOW_EMPTY_PASSWORD));

  SetNextItemPosition(ipNewLine);

  FtpDupFFCheck = new TFarCheckBox(this);
  FtpDupFFCheck->SetCaption(GetMsg(NB_LOGIN_FTP_DUPFF));

  SetNextItemPosition(ipNewLine);

  FtpUndupFFCheck = new TFarCheckBox(this);
  FtpUndupFFCheck->SetCaption(GetMsg(NB_LOGIN_FTP_UNDUPFF));

  SetNextItemPosition(ipNewLine);

  SslSessionReuseCheck = new TFarCheckBox(this);
  SslSessionReuseCheck->SetCaption(GetMsg(NB_LOGIN_FTP_SSLSESSIONREUSE));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_FTP_POST_LOGIN_COMMANDS));

  for (int32_t Index3 = 0; Index3 < nb::ToInt32(_countof(PostLoginCommandsEdits)); ++Index3)
  {
    TFarEdit * Edit = new TFarEdit(this);
    PostLoginCommandsEdits[Index3] = Edit;
  }

  new TFarSeparator(this);

  // Connection tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabConnection);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_CONNECTION_GROUP));

  Text = new TFarText(this);
  SetNextItemPosition(ipNewLine);

  SshBufferSizeCheck = new TFarCheckBox(this);
  SshBufferSizeCheck->SetCaption(GetMsg(NB_LOGIN_SSH_OPTIMIZE_BUFFER_SIZE));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_TIMEOUTS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TIMEOUT));

  SetNextItemPosition(ipRight);

  TimeoutEdit = new TFarEdit(this);
  TimeoutEdit->SetFixed(true);
  TimeoutEdit->SetMask(L"####");
  TimeoutEdit->SetWidth(5);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TIMEOUT_SECONDS));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_PING_GROUP));

  PingOffButton = new TFarRadioButton(this);
  PingOffButton->SetCaption(GetMsg(NB_LOGIN_PING_OFF));

  PingNullPacketButton = new TFarRadioButton(this);
  PingNullPacketButton->SetCaption(GetMsg(NB_LOGIN_PING_NULL_PACKET));

  PingDummyCommandButton = new TFarRadioButton(this);
  PingDummyCommandButton->SetCaption(GetMsg(NB_LOGIN_PING_DUMMY_COMMAND));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PING_INTERVAL));
  Text->SetEnabledDependencyNegative(PingOffButton);

  SetNextItemPosition(ipRight);

  PingIntervalSecEdit = new TFarEdit(this);
  PingIntervalSecEdit->SetFixed(true);
  PingIntervalSecEdit->SetMask(L"####");
  PingIntervalSecEdit->SetWidth(6);
  PingIntervalSecEdit->SetEnabledDependencyNegative(PingOffButton);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_IP_GROUP));

  IPAutoButton = new TFarRadioButton(this);
  IPAutoButton->SetCaption(GetMsg(NB_LOGIN_IP_AUTO));

  SetNextItemPosition(ipRight);

  IPv4Button = new TFarRadioButton(this);
  IPv4Button->SetCaption(GetMsg(NB_LOGIN_IP_V4));

  IPv6Button = new TFarRadioButton(this);
  IPv6Button->SetCaption(GetMsg(NB_LOGIN_IP_V6));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);

  SetNextItemPosition(ipNewLine);

  // Proxy tab

  SetDefaultGroup(tabProxy);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_PROXY_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_METHOD));

  SetNextItemPosition(ipRight);

  FtpProxyMethodCombo = new TFarComboBox(this);
  FtpProxyMethodCombo->SetDropDownList(true);
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_NONE, pmNone);
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SOCKS4, pmSocks4);
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SOCKS5, pmSocks5);
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_HTTP, pmHTTP);
  // FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SYSTEM, pmSystem);
  TProxyMethod FtpProxyMethod = ToProxyMethod(GetLastSupportedFtpProxyMethod());
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_SITE, ToProxyMethod(FtpProxyMethod + 1));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_PROXYUSER_USERHOST, ToProxyMethod(FtpProxyMethod + 2));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_OPEN_HOST, ToProxyMethod(FtpProxyMethod + 3));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_PROXYUSER_USERUSER, ToProxyMethod(FtpProxyMethod + 4));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_USER_USERHOST, ToProxyMethod(FtpProxyMethod + 5));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_PROXYUSER_HOST, ToProxyMethod(FtpProxyMethod + 6));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_USERHOST_PROXYUSER, ToProxyMethod(FtpProxyMethod + 7));
  FtpProxyMethodComboAddNewItem(NB_LOGIN_PROXY_FTP_USER_USERPROXYUSERHOST, ToProxyMethod(FtpProxyMethod + 8));
  FtpProxyMethodCombo->SetWidth(40);

  SshProxyMethodCombo = new TFarComboBox(this);
  SshProxyMethodCombo->SetLeft(FtpProxyMethodCombo->GetLeft());
  SshProxyMethodCombo->SetWidth(FtpProxyMethodCombo->GetWidth());
  SshProxyMethodCombo->SetRight(FtpProxyMethodCombo->GetRight());
  SshProxyMethodCombo->SetDropDownList(true);

  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_NONE, pmNone);
  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SOCKS4, pmSocks4);
  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SOCKS5, pmSocks5);
  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_HTTP, pmHTTP);
  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_TELNET, pmTelnet);
  SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_LOCAL, pmCmd);
  // SshProxyMethodComboAddNewItem(NB_LOGIN_PROXY_SYSTEM, pmSystem);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_HOST));

  SetNextItemPosition(ipNewLine);

  ProxyHostEdit = new TFarEdit(this);
  ProxyHostEdit->SetWidth(42);
  Text->SetEnabledFollow(ProxyHostEdit);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_PORT));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  ProxyPortEdit = new TFarEdit(this);
  ProxyPortEdit->SetFixed(true);
  ProxyPortEdit->SetMask(L"999999");
  // ProxyPortEdit->SetWidth(12);
  Text->SetEnabledFollow(ProxyPortEdit);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_USERNAME));

  ProxyUsernameEdit = new TFarEdit(this);
  ProxyUsernameEdit->SetWidth(ProxyUsernameEdit->GetWidth() / 2 - 1);
  Text->SetEnabledFollow(ProxyUsernameEdit);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_PASSWORD));
  Text->Move(0, -1);

  SetNextItemPosition(ipBelow);

  ProxyPasswordEdit = new TFarEdit(this);
  ProxyPasswordEdit->SetPassword(true);
  Text->SetEnabledFollow(ProxyPasswordEdit);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_PROXY_SETTINGS_GROUP));

  ProxyTelnetCommandLabel = new TFarText(this);
  ProxyTelnetCommandLabel->SetCaption(GetMsg(NB_LOGIN_PROXY_TELNET_COMMAND));

  SetNextItemPosition(ipRight);

  ProxyTelnetCommandEdit = new TFarEdit(this);
  ProxyTelnetCommandLabel->SetEnabledFollow(ProxyTelnetCommandEdit);

  SetNextItemPosition(ipNewLine);

  ProxyLocalCommandLabel = new TFarText(this);
  ProxyLocalCommandLabel->SetCaption(GetMsg(NB_LOGIN_PROXY_LOCAL_COMMAND));
  ProxyLocalCommandLabel->Move(0, -1);

  SetNextItemPosition(ipRight);

  ProxyLocalCommandEdit = new TFarEdit(this);
  ProxyLocalCommandLabel->SetEnabledFollow(ProxyLocalCommandEdit);

  SetNextItemPosition(ipNewLine);

  ProxyLocalhostCheck = new TFarCheckBox(this);
  ProxyLocalhostCheck->SetCaption(GetMsg(NB_LOGIN_PROXY_LOCALHOST));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PROXY_DNS));

  ProxyDNSOffButton = new TFarRadioButton(this);
  ProxyDNSOffButton->SetCaption(GetMsg(NB_LOGIN_PROXY_DNS_NO));
  Text->SetEnabledFollow(ProxyDNSOffButton);

  SetNextItemPosition(ipRight);

  ProxyDNSAutoButton = new TFarRadioButton(this);
  ProxyDNSAutoButton->SetCaption(GetMsg(NB_LOGIN_PROXY_DNS_AUTO));
  ProxyDNSAutoButton->SetEnabledFollow(ProxyDNSOffButton);

  ProxyDNSOnButton = new TFarRadioButton(this);
  ProxyDNSOnButton->SetCaption(GetMsg(NB_LOGIN_PROXY_DNS_YES));
  ProxyDNSOnButton->SetEnabledFollow(ProxyDNSOffButton);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // Tunnel tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabTunnel);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_TUNNEL_GROUP));

  TunnelCheck = new TFarCheckBox(this);
  TunnelCheck->SetCaption(GetMsg(NB_LOGIN_TUNNEL_TUNNEL));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_TUNNEL_SESSION_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_HOST_NAME));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelHostNameEdit = new TFarEdit(this);
  TunnelHostNameEdit->SetRight(CRect.Right - 12 - 2);
  TunnelHostNameEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PORT_NUMBER));
  Text->Move(0, -1);
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipBelow);

  TunnelPortNumberEdit = new TFarEdit(this);
  TunnelPortNumberEdit->SetFixed(true);
  TunnelPortNumberEdit->SetMask(L"999999");
  TunnelPortNumberEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_USER_NAME));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelUserNameEdit = new TFarEdit(this);
  TunnelUserNameEdit->SetWidth(TunnelUserNameEdit->GetWidth() / 2 - 1);
  TunnelUserNameEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PASSWORD));
  Text->Move(0, -1);
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipBelow);

  TunnelPasswordEdit = new TFarEdit(this);
  TunnelPasswordEdit->SetPassword(true);
  TunnelPasswordEdit->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_PRIVATE_KEY));
  Text->SetEnabledDependency(TunnelCheck);

  TunnelPrivateKeyEdit = new TFarEdit(this);
  TunnelPrivateKeyEdit->SetEnabledDependency(TunnelCheck);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_TUNNEL_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_TUNNEL_LOCAL_PORT_NUMBER));
  Text->SetEnabledDependency(TunnelCheck);

  SetNextItemPosition(ipRight);

  TunnelLocalPortNumberEdit = new TFarComboBox(this);
  TunnelLocalPortNumberEdit->SetLeft(TunnelPortNumberEdit->GetLeft());
  TunnelLocalPortNumberEdit->SetEnabledDependency(TunnelCheck);
  TunnelLocalPortNumberEdit->GetItems()->BeginUpdate();
  {
    try__finally
    {
      TunnelLocalPortNumberEdit->GetItems()->Add(GetMsg(NB_LOGIN_TUNNEL_LOCAL_PORT_NUMBER_AUTOASSIGN));
      for (int32_t Index4 = GetConfiguration()->GetTunnelLocalPortNumberLow();
        Index4 <= GetConfiguration()->GetTunnelLocalPortNumberHigh(); ++Index4)
      {
        TunnelLocalPortNumberEdit->GetItems()->Add(::IntToStr(Index1));
      }
    }
    __finally
    {
      TunnelLocalPortNumberEdit->GetItems()->EndUpdate();
    } end_try__finally
  }

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  // SSH tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabSsh);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_SSH_GROUP));

  CompressionCheck = new TFarCheckBox(this);
  CompressionCheck->SetCaption(GetMsg(NB_LOGIN_COMPRESSION));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_SSH_PROTOCOL_GROUP));

  SshProt1onlyButton = new TFarRadioButton(this);
  SshProt1onlyButton->SetCaption(GetMsg(NB_LOGIN_SSH1_ONLY));

  SetNextItemPosition(ipRight);

  SshProt1Button = new TFarRadioButton(this);
  SshProt1Button->SetCaption(GetMsg(NB_LOGIN_SSH1));

  SshProt2Button = new TFarRadioButton(this);
  SshProt2Button->SetCaption(GetMsg(NB_LOGIN_SSH2));

  SshProt2onlyButton = new TFarRadioButton(this);
  SshProt2onlyButton->SetCaption(GetMsg(NB_LOGIN_SSH2_ONLY));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_ENCRYPTION_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_CIPHER));

  CipherListBox = new TFarListBox(this);
  CipherListBox->SetRight(CipherListBox->GetRight() - 15);
  CipherListBox->SetHeight(1 + CIPHER_COUNT + 1);
  int32_t Pos = CipherListBox->GetBottom();

  SetNextItemPosition(ipRight);

  CipherUpButton = new TFarButton(this);
  CipherUpButton->SetCaption(GetMsg(NB_LOGIN_UP));
  CipherUpButton->Move(0, 1);
  CipherUpButton->SetResult(-1);
  CipherUpButton->SetOnClick(nb::bind(&TSessionDialog::CipherButtonClick, this));

  SetNextItemPosition(ipBelow);

  CipherDownButton = new TFarButton(this);
  CipherDownButton->SetCaption(GetMsg(NB_LOGIN_DOWN));
  CipherDownButton->SetResult(1);
  CipherDownButton->SetOnClick(nb::bind(&TSessionDialog::CipherButtonClick, this));

  SetNextItemPosition(ipNewLine);

  if (!Limited)
  {
    Ssh2DESCheck = new TFarCheckBox(this);
    Ssh2DESCheck->Move(0, Pos - Ssh2DESCheck->GetTop() + 1);
    Ssh2DESCheck->SetCaption(GetMsg(NB_LOGIN_SSH2DES));
    Ssh2DESCheck->SetEnabledDependencyNegative(SshProt1onlyButton);
  }
  else
  {
    Ssh2DESCheck = nullptr;
  }

  // KEX tab

  SetDefaultGroup(tabKex);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_KEX_REEXCHANGE_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_KEX_REKEY_TIME));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  RekeyTimeEdit = new TFarEdit(this);
  RekeyTimeEdit->SetFixed(true);
  RekeyTimeEdit->SetMask(L"####");
  RekeyTimeEdit->SetWidth(6);
  RekeyTimeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_KEX_REKEY_DATA));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  RekeyDataEdit = new TFarEdit(this);
  RekeyDataEdit->SetWidth(6);
  RekeyDataEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_KEX_OPTIONS_GROUP));

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_LOGIN_KEX_LIST));
  Text->SetEnabledDependencyNegative(SshProt1onlyButton);

  KexListBox = new TFarListBox(this);
  KexListBox->SetRight(KexListBox->GetRight() - 15);
  KexListBox->SetHeight(1 + KEX_COUNT + 1);
  KexListBox->SetEnabledDependencyNegative(SshProt1onlyButton);

  SetNextItemPosition(ipRight);

  KexUpButton = new TFarButton(this);
  KexUpButton->SetCaption(GetMsg(NB_LOGIN_UP));
  KexUpButton->Move(0, 1);
  KexUpButton->SetResult(-1);
  KexUpButton->SetOnClick(nb::bind(&TSessionDialog::KexButtonClick, this));

  SetNextItemPosition(ipBelow);

  KexDownButton = new TFarButton(this);
  KexDownButton->SetCaption(GetMsg(NB_LOGIN_DOWN));
  KexDownButton->SetResult(1);
  KexDownButton->SetOnClick(nb::bind(&TSessionDialog::KexButtonClick, this));

  SetNextItemPosition(ipNewLine);

  // Authentication tab

  SetDefaultGroup(tabAuthentication);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);

  SshNoUserAuthCheck = new TFarCheckBox(this);
  SshNoUserAuthCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_SSH_NO_USER_AUTH));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_AUTH_GROUP));

  TryAgentCheck = new TFarCheckBox(this);
  TryAgentCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_TRY_AGENT));

  AuthTISCheck = new TFarCheckBox(this);
  AuthTISCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_TIS));

  AuthKICheck = new TFarCheckBox(this);
  AuthKICheck->SetCaption(GetMsg(NB_LOGIN_AUTH_KI));

  AuthKIPasswordCheck = new TFarCheckBox(this);
  AuthKIPasswordCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_KI_PASSWORD));
  AuthKIPasswordCheck->Move(4, 0);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_AUTH_PARAMS_GROUP));

  AgentFwdCheck = new TFarCheckBox(this);
  AgentFwdCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_AGENT_FWD));

  // GSSAPI
  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_LOGIN_AUTH_GSSAPI_PARAMS_GROUP));

  AuthGSSAPICheck3 = new TFarCheckBox(this);
  AuthGSSAPICheck3->SetCaption(GetMsg(NB_LOGIN_AUTH_ATTEMPT_GSSAPI_AUTHENTICATION));
  AuthGSSAPICheck3->SetOnAllowChange(nb::bind(&TSessionDialog::AuthGSSAPICheckAllowChange, this));

  GSSAPIFwdTGTCheck = new TFarCheckBox(this);
  GSSAPIFwdTGTCheck->SetCaption(GetMsg(NB_LOGIN_AUTH_ALLOW_GSSAPI_CREDENTIAL_DELEGATION));

  new TFarSeparator(this);

  // Bugs tab

  SetDefaultGroup(tabBugs);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_BUGS_GROUP));

  BUGS();

//  BugIgnore1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  //BugPlainPW1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  //BugRSA1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
  BugHMAC2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugDeriveKey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugRSAPad2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugPKSessID2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
  BugRekey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);

  // WebDAV tab

  SetNextItemPosition(ipNewLine);

  SetDefaultGroup(tabWebDAV);
  Separator = new TFarSeparator(this);
  Separator->SetPosition(GroupTop);
  Separator->SetCaption(GetMsg(NB_LOGIN_WEBDAV_GROUP));

  WebDAVCompressionCheck = new TFarCheckBox(this);
  WebDAVCompressionCheck->SetCaption(GetMsg(NB_LOGIN_COMPRESSION));

#undef TRISTATE

  new TFarSeparator(this);

  // Buttons

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(0);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(CRect.Bottom - 1);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(Action != saConnect);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  ConnectButton = new TFarButton(this);
  ConnectButton->SetCaption(GetMsg(NB_LOGIN_CONNECT_BUTTON));
  ConnectButton->SetDefault(Action == saConnect);
  ConnectButton->SetResult(brConnect);
  ConnectButton->SetCenterGroup(true);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

void TSessionDialog::FtpProxyMethodComboAddNewItem(int32_t ProxyTypeId, TProxyMethod ProxyType)
{
  FtpProxyMethodCombo->GetItems()->AddObject(GetMsg(ProxyTypeId),
    ToObj(nb::ToPtr(ProxyType)));
}

void TSessionDialog::SshProxyMethodComboAddNewItem(int32_t ProxyTypeId, TProxyMethod ProxyType)
{
  SshProxyMethodCombo->GetItems()->AddObject(GetMsg(ProxyTypeId),
    ToObj(nb::ToPtr(ProxyType)));
}

TSessionDialog::~TSessionDialog() noexcept
{
//  SAFE_DESTROY(FTabs);
}

void TSessionDialog::Change()
{
  TTabbedDialog::Change();

  if (GetHandle() && !ChangesLocked())
  {
    if ((FTransferProtocolIndex != TransferProtocolCombo->GetItemIndex()) ||
      (FFtpEncryptionComboIndex != FtpEncryptionCombo->GetItemIndex()))
    {
      TransferProtocolComboChange();
    }

    LockChanges();
    try__finally
    {
      UpdateControls();
    }
    __finally
    {
      UnlockChanges();
    } end_try__finally
  }
}

void TSessionDialog::Init()
{
  TTabbedDialog::Init();
}

static void AdjustRemoteDir(UnicodeString & HostName, TFarEdit * PortNumberEdit, TFarEdit * RemoteDirectoryEdit)
{
  UnicodeString Dir;
  const int32_t P = HostName.Pos(L'/');
  if (P > 0)
  {
    Dir = HostName.SubString(P, HostName.Length() - P + 1);
    const int32_t P2 = Dir.Pos(L':');
    if (P2 > 0)
    {
      const UnicodeString Port = Dir.SubString(P2 + 1, Dir.Length() - P2);
      Dir.SetLength(P2 - 1);
      if (Port.ToInt32())
        PortNumberEdit->SetAsInteger(Port.ToInt32());
    }
    HostName.SetLength(P - 1);
  }
  const UnicodeString RemoteDir = RemoteDirectoryEdit->GetText();
  if (RemoteDir.IsEmpty() && !Dir.IsEmpty())
  {
    RemoteDirectoryEdit->SetText(Dir);
  }
}

void TSessionDialog::TransferProtocolComboChange()
{
  const TFtps Ftps = GetFtps();
  // note that this modifies the session for good,
  // even if user cancels the dialog
  SavePing(FSessionData);

  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();
  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();
  const int32_t Port = PortNumberEdit->GetAsInteger();

  LoadPing(FSessionData);
  const TFSProtocol FSProtocol = GetFSProtocol();
  if (FSProtocol == fsSFTPonly || FSProtocol == fsSCPonly)
  {
    if (Port == FtpPortNumber)
    {
      PortNumberEdit->SetAsInteger(SshPortNumber);
    }
  }
  else if ((FSProtocol == fsFTP) && ((Ftps == ftpsNone) || (Ftps == ftpsExplicitSsl) || (Ftps == ftpsExplicitTls)))
  {
    if ((Port == SshPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(FtpPortNumber);
    }
  }
  else if ((FSProtocol == fsFTP) && (Ftps == ftpsImplicit))
  {
    if ((Port == SshPortNumber) || (Port == FtpPortNumber) || (Port == HTTPPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(FtpsImplicitPortNumber);
    }
  }
  else if ((FSProtocol == fsWebDAV) && (Ftps == ftpsNone))
  {
    if ((Port == FtpPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPSPortNumber))
    {
      PortNumberEdit->SetAsInteger(HTTPPortNumber);
      UnicodeString HostName = HostNameEdit->GetText();
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
      HostNameEdit->SetText(HostName);
    }
  }
  else if ((FSProtocol == fsWebDAV) && (Ftps != ftpsNone))
  {
    if ((Port == FtpPortNumber) || (Port == FtpsImplicitPortNumber) || (Port == HTTPPortNumber))
    {
      PortNumberEdit->SetAsInteger(HTTPSPortNumber);
      UnicodeString HostName = HostNameEdit->GetText();
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
      HostNameEdit->SetText(HostName);
    }
  }
  else if (FSProtocol == fsS3)
  {
    if (Port == HTTPSPortNumber)
    {
      PortNumberEdit->SetAsInteger(HTTPSPortNumber);
      UnicodeString HostName = HostNameEdit->GetText();
      if (HostName.IsEmpty())
      {
        HostName = S3HostName;
      }
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
      HostNameEdit->SetText(HostName);
    }
  }
}

bool TSessionDialog::IsSshProtocol(TFSProtocol FSProtocol)
{
  const bool Result =
    (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) || (FSProtocol == fsSCPonly);

  return Result;
}

bool TSessionDialog::IsWebDAVProtocol(TFSProtocol FSProtocol) const
{
  return FSProtocol == fsWebDAV;
}

bool TSessionDialog::IsSshOrWebDAVProtocol(TFSProtocol FSProtocol) const
{
  return IsSshProtocol(FSProtocol) || IsWebDAVProtocol(FSProtocol);
}

void TSessionDialog::UpdateControls()
{
  const TFSProtocol FSProtocol = GetFSProtocol();
  const TFtps Ftps = GetFtps();
  const bool InternalSshProtocol = IsSshProtocol(FSProtocol);
  const bool InternalWebDAVProtocol = IsWebDAVProtocol(FSProtocol);
  const bool HTTPSProtocol = (FSProtocol == fsWebDAV) && (Ftps != ftpsNone);
  const bool lS3Protocol = (FSProtocol == fsS3);
  const bool lSshProtocol = InternalSshProtocol;
  const bool lSftpProtocol = (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP);
  const bool ScpOnlyProtocol = (FSProtocol == fsSCPonly);
  const bool lFtpProtocol = (FSProtocol == fsFTP) && (Ftps == ftpsNone);
  const bool lFtpsProtocol = (FSProtocol == fsFTP) && (Ftps != ftpsNone);
  const bool LoginAnonymous = false;
  const bool IsMainTab = GetTab() == TransferProtocolCombo->GetGroup();

  ConnectButton->SetEnabled(!HostNameEdit->GetIsEmpty());

  // Basic tab
  AllowScpFallbackCheck->SetVisible(
    TransferProtocolCombo->GetVisible() &&
    (IndexToFSProtocol(TransferProtocolCombo->GetItemIndex(), false) == fsSFTPonly));
  InsecureLabel->SetVisible(TransferProtocolCombo->GetVisible() && !lSshProtocol && !lFtpsProtocol && !HTTPSProtocol && !lS3Protocol);
  const bool FtpEncryptionVisible = (GetTab() == FtpEncryptionCombo->GetGroup()) &&
    (lFtpProtocol || lFtpsProtocol || InternalWebDAVProtocol || HTTPSProtocol || lS3Protocol);
  FtpEncryptionLabel->SetVisible(FtpEncryptionVisible);
  FtpEncryptionCombo->SetVisible(FtpEncryptionVisible);
  PrivateKeyEdit->SetEnabled(lSshProtocol || lFtpsProtocol || HTTPSProtocol);
//  HostNameLabel->SetCaption(GetMsg(NB_LOGIN_HOST_NAME));

  UserNameEdit->SetEnabled(!LoginAnonymous);
  PasswordEdit->SetEnabled(!LoginAnonymous);

  UserNameLabel->SetVisible(IsMainTab && !lS3Protocol);
  UserNameEdit->SetVisible(IsMainTab);
  PasswordLabel->SetVisible(IsMainTab && !lS3Protocol);
  PasswordEdit->SetVisible(IsMainTab);

  // Connection sheet
  FtpPasvModeCheck->SetEnabled(lFtpProtocol);
  if (lFtpProtocol && (FtpProxyMethodCombo->GetItemIndex() != pmNone) && !FtpPasvModeCheck->GetChecked())
  {
    FtpPasvModeCheck->SetChecked(true);
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    WinSCPPlugin->MoreMessageDialog(GetMsg(NB_FTP_PASV_MODE_REQUIRED),
      nullptr, qtInformation, qaOK);
  }
  SshBufferSizeCheck->SetEnabled(lSshProtocol);
  PingNullPacketButton->SetEnabled(lSshProtocol);
  IPAutoButton->SetEnabled(lSshProtocol);

  // SFTP tab
  SftpTab->SetEnabled(lSftpProtocol);

  // FTP tab
  FtpTab->SetEnabled(lFtpProtocol || lFtpsProtocol);
  FtpAllowEmptyPasswordCheck->SetEnabled(lFtpProtocol || lFtpsProtocol);
  SslSessionReuseCheck->SetEnabled(lFtpsProtocol);

  // SSH tab
  SshTab->SetEnabled(lSshProtocol);
  CipherUpButton->SetEnabled(CipherListBox->GetItems()->GetSelected() != 0);
  CipherDownButton->SetEnabled(
    CipherListBox->GetItems()->GetSelected() < CipherListBox->GetItems()->GetCount() - 1);

  // Authentication tab
  AuthenticationTab->SetEnabled(lSshProtocol);
  SshNoUserAuthCheck->SetEnabled(!SshProt1onlyButton->GetChecked());
  const bool Authentication = !SshNoUserAuthCheck->GetEnabled() || !SshNoUserAuthCheck->GetChecked();
  TryAgentCheck->SetEnabled(Authentication);
  AuthTISCheck->SetEnabled(Authentication && !SshProt2onlyButton->GetChecked());
  AuthKICheck->SetEnabled(Authentication && !SshProt1onlyButton->GetChecked());
  AuthKIPasswordCheck->SetEnabled(
    Authentication &&
    ((AuthTISCheck->GetEnabled() && AuthTISCheck->GetChecked()) ||
      (AuthKICheck->GetEnabled() && AuthKICheck->GetChecked())));
  AuthGSSAPICheck3->SetEnabled(
    Authentication && !SshProt1onlyButton->GetChecked());
  GSSAPIFwdTGTCheck->SetEnabled(
    Authentication && !SshProt1onlyButton->GetChecked());

  // Directories tab
  CacheDirectoryChangesCheck->SetEnabled(
    (FSProtocol != fsSCPonly) || CacheDirectoriesCheck->GetChecked());
  PreserveDirectoryChangesCheck->SetEnabled(
    CacheDirectoryChangesCheck->GetIsEnabled() && CacheDirectoryChangesCheck->GetChecked());
  ResolveSymlinksCheck->SetEnabled(!InternalWebDAVProtocol);

  // Environment tab
  DSTModeUnixCheck->SetEnabled(!lFtpProtocol);
  TimeDifferenceEdit->SetEnabled((lFtpProtocol || (FSProtocol == fsSCPonly)));

  // Recycle bin tab
  OverwrittenToRecycleBinCheck->SetEnabled((FSProtocol != fsSCPonly) &&
    !lFtpProtocol);
  RecycleBinPathEdit->SetEnabled(
    (DeleteToRecycleBinCheck->GetIsEnabled() && DeleteToRecycleBinCheck->GetChecked()) ||
    (OverwrittenToRecycleBinCheck->GetIsEnabled() && OverwrittenToRecycleBinCheck->GetChecked()));

  // Kex tab
  KexTab->SetEnabled(lSshProtocol && !SshProt1onlyButton->GetChecked() &&
    (BugRekey2Combo->GetItemIndex() != 2));
  KexUpButton->SetEnabled((KexListBox->GetItems()->GetSelected() > 0));
  KexDownButton->SetEnabled(
    (KexListBox->GetItems()->GetSelected() < KexListBox->GetItems()->GetCount() - 1));

  // Bugs tab
  BugsTab->SetEnabled(lSshProtocol);

  // WebDAV tab
  WebDAVTab->SetEnabled(InternalWebDAVProtocol);

  // Scp/Shell tab
  ScpTab->SetEnabled(InternalSshProtocol);
  // disable also for SFTP with SCP fallback, as if someone wants to configure
  // these he/she probably intends to use SCP and should explicitly select it.
  // (note that these are not used for secondary shell session)
  ListingCommandEdit->SetEnabled(ScpOnlyProtocol);
  IgnoreLsWarningsCheck->SetEnabled(ScpOnlyProtocol);
  SCPLsFullTimeAutoCheck->SetEnabled(ScpOnlyProtocol);
  LookupUserGroupsCheck->SetEnabled(ScpOnlyProtocol);
  UnsetNationalVarsCheck->SetEnabled(ScpOnlyProtocol);
  ClearAliasesCheck->SetEnabled(ScpOnlyProtocol);
  Scp1CompatibilityCheck->SetEnabled(ScpOnlyProtocol);

  // Connection/Proxy tab
  TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  const TProxyMethod ProxyMethod = IndexToProxyMethod(ProxyMethodCombo->GetItemIndex(), ProxyMethodCombo->GetItems());
  ProxyMethodCombo->SetVisible(GetTab() == ProxyMethodCombo->GetGroup());
  TFarComboBox * OtherProxyMethodCombo = GetOtherProxyMethodCombo();
  OtherProxyMethodCombo->SetVisible(false);

  const bool Proxy = (ProxyMethod != pmNone);
  const UnicodeString ProxyCommand =
    (ProxyMethod == pmCmd) ?
    ProxyLocalCommandEdit->GetText() : ProxyTelnetCommandEdit->GetText();
  ProxyHostEdit->SetEnabled(Proxy && // (ProxyMethod != pmSystem) &&
    ((ProxyMethod != pmCmd) ||
      ::AnsiContainsText(ProxyCommand, L"%proxyhost")));
  ProxyPortEdit->SetEnabled(Proxy && // (ProxyMethod != pmSystem) &&
    ((ProxyMethod != pmCmd) ||
      ::AnsiContainsText(ProxyCommand, L"%proxyport")));
  ProxyUsernameEdit->SetEnabled(Proxy &&
    // FZAPI does not support username for SOCKS4
    (((ProxyMethod == pmSocks4) && lSshProtocol) ||
      (ProxyMethod == pmSocks5) ||
      (ProxyMethod == pmHTTP) ||
      (((ProxyMethod == pmTelnet) ||
          (ProxyMethod == pmCmd)) &&
        ::AnsiContainsText(ProxyCommand, L"%user")) /*||
      (ProxyMethod == pmSystem)*/));
  ProxyPasswordEdit->SetEnabled(Proxy &&
    ((ProxyMethod == pmSocks5) ||
      (ProxyMethod == pmHTTP) ||
      (((ProxyMethod == pmTelnet) ||
          (ProxyMethod == pmCmd)) &&
        ::AnsiContainsText(ProxyCommand, L"%pass")) /*||
      (ProxyMethod == pmSystem)*/));
  const bool ProxySettings = Proxy && lSshProtocol;
  ProxyTelnetCommandEdit->SetEnabled(ProxySettings && (ProxyMethod == pmTelnet));
  ProxyLocalCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethod == pmCmd));
  ProxyLocalCommandLabel->SetVisible(ProxyLocalCommandEdit->GetVisible());
  ProxyTelnetCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethod != pmCmd));
  ProxyTelnetCommandLabel->SetVisible(ProxyTelnetCommandEdit->GetVisible());
  ProxyLocalhostCheck->SetEnabled(ProxySettings);
  ProxyDNSOffButton->SetEnabled(ProxySettings);

  // Tunnel tab
  TunnelTab->SetEnabled(InternalSshProtocol);
}

bool TSessionDialog::Execute(TSessionData * SessionData, TSessionActionEnum Action)
{
  constexpr int32_t Captions[] =
  {
    NB_LOGIN_ADD,
    NB_LOGIN_EDIT,
    NB_LOGIN_CONNECT
  };
  SetCaption(GetMsg(Captions[Action]));

  FSessionData = SessionData;
  FTransferProtocolIndex = TransferProtocolCombo->GetItemIndex();

  FFtpEncryptionComboIndex = FtpEncryptionCombo->GetItemIndex();

  HideTabs();
  SelectTab(tabSession);

  // load session data

  // Basic tab
  HostNameEdit->SetText(SessionData->GetHostName());
  PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());

  UserNameEdit->SetText(SessionData->SessionGetUserName());
  PasswordEdit->SetText(SessionData->GetPassword());
  PrivateKeyEdit->SetText(SessionData->GetPublicKeyFile());

  bool AllowScpFallback;
  TransferProtocolCombo->SetItemIndex(
    nb::ToInt32(FSProtocolToIndex(SessionData->GetFSProtocol(), AllowScpFallback)));
  AllowScpFallbackCheck->SetChecked(AllowScpFallback);

  // Directories tab
  RemoteDirectoryEdit->SetText(SessionData->GetRemoteDirectory());
  UpdateDirectoriesCheck->SetChecked(SessionData->GetUpdateDirectories());
  CacheDirectoriesCheck->SetChecked(SessionData->GetCacheDirectories());
  CacheDirectoryChangesCheck->SetChecked(SessionData->GetCacheDirectoryChanges());
  PreserveDirectoryChangesCheck->SetChecked(SessionData->GetPreserveDirectoryChanges());
  ResolveSymlinksCheck->SetChecked(SessionData->GetResolveSymlinks());

  // Environment tab
  if (SessionData->GetEOLType() == eolLF)
  {
    EOLTypeCombo->SetItemIndex(0);
  }
  else
  {
    EOLTypeCombo->SetItemIndex(1);
  }

  switch (SessionData->GetDSTMode())
  {
  case dstmWin:
    DSTModeWinCheck->SetChecked(true);
    break;

  case dstmKeep:
    DSTModeKeepCheck->SetChecked(true);
    break;

  default:
  case dstmUnix:
    DSTModeUnixCheck->SetChecked(true);
    break;
  }

  DeleteToRecycleBinCheck->SetChecked(SessionData->GetDeleteToRecycleBin());
  OverwrittenToRecycleBinCheck->SetChecked(SessionData->GetOverwrittenToRecycleBin());
  RecycleBinPathEdit->SetText(SessionData->GetRecycleBinPath());

  // Shell tab
  if (SessionData->GetDefaultShell())
  {
    ShellEdit->SetText(ShellEdit->GetItems()->GetString(0));
  }
  else
  {
    ShellEdit->SetText(SessionData->GetShell());
  }
  if (SessionData->GetDetectReturnVar())
  {
    ReturnVarEdit->SetText(ReturnVarEdit->GetItems()->GetString(0));
  }
  else
  {
    ReturnVarEdit->SetText(SessionData->GetReturnVar());
  }
  LookupUserGroupsCheck->SetChecked(SessionData->GetLookupUserGroups() != asOff);
  ClearAliasesCheck->SetChecked(SessionData->GetClearAliases());
  IgnoreLsWarningsCheck->SetChecked(SessionData->GetIgnoreLsWarnings());
  Scp1CompatibilityCheck->SetChecked(SessionData->GetScp1Compatibility());
  UnsetNationalVarsCheck->SetChecked(SessionData->GetUnsetNationalVars());
  ListingCommandEdit->SetText(SessionData->GetListingCommand());
  SCPLsFullTimeAutoCheck->SetChecked((SessionData->GetSCPLsFullTime() != asOff));
  int32_t TimeDifferenceMin = ::DateTimeToTimeStamp(SessionData->GetTimeDifference()).Time / 60000;
  if (SessionData->GetTimeDifference().GetValue() < 0)
  {
    TimeDifferenceMin = -TimeDifferenceMin;
  }
  TimeDifferenceEdit->SetAsInteger(TimeDifferenceMin / 60);
  TimeDifferenceMinutesEdit->SetAsInteger(TimeDifferenceMin % 60);

  // SFTP tab

#define TRISTATE(COMBO, PROP, MSG) \
    (COMBO)->SetItemIndex(nb::ToInt32(2 - SessionData->Get ## PROP))
  SFTP_BUGS();

  if (SessionData->GetSftpServer().IsEmpty())
  {
    SftpServerEdit->SetText(SftpServerEdit->GetItems()->GetString(0));
  }
  else
  {
    SftpServerEdit->SetText(SessionData->GetSftpServer());
  }
  SFTPMaxVersionCombo->SetItemIndex(nb::ToInt32(SessionData->GetSFTPMaxVersion()));
  SFTPMinPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMinPacketSize());
  SFTPMaxPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMaxPacketSize());

  // FTP tab
  FtpUseMlsdCombo->SetItemIndex(nb::ToInt32(2 - SessionData->GetFtpUseMlsd()));
  FtpAllowEmptyPasswordCheck->SetChecked(SessionData->GetFtpAllowEmptyPassword());
  std::unique_ptr<TStrings> PostLoginCommands(std::make_unique<TStringList>());
  PostLoginCommands->SetText(SessionData->GetPostLoginCommands());
  for (int32_t Index = 0; (Index < PostLoginCommands->GetCount()) &&
    (Index < nb::ToInt32(_countof(PostLoginCommandsEdits))); ++Index)
  {
    PostLoginCommandsEdits[Index]->SetText(PostLoginCommands->GetString(Index));
  }

  FtpDupFFCheck->SetChecked(SessionData->GetFtpDupFF());
  FtpUndupFFCheck->SetChecked(SessionData->GetFtpUndupFF());
  SslSessionReuseCheck->SetChecked(SessionData->GetSslSessionReuse());

  switch (const TFtps Ftps = SessionData->GetFtps())
  {
  case ftpsNone:
    FtpEncryptionCombo->SetItemIndex(0);
    break;

  case ftpsImplicit:
    FtpEncryptionCombo->SetItemIndex(1);
    break;

  case ftpsExplicitSsl:
    FtpEncryptionCombo->SetItemIndex(2);
    break;

  case ftpsExplicitTls:
    FtpEncryptionCombo->SetItemIndex(3);
    break;

  default:
    FtpEncryptionCombo->SetItemIndex(0);
    break;
  }

  // Connection tab
  FtpPasvModeCheck->SetChecked(SessionData->GetFtpPasvMode());
  SshBufferSizeCheck->SetChecked((FSessionData->GetSendBuf() > 0) && FSessionData->GetSshSimple());
  LoadPing(SessionData);
  TimeoutEdit->SetAsInteger(SessionData->GetTimeout());

  switch (SessionData->GetAddressFamily())
  {
  case afIPv4:
    IPv4Button->SetChecked(true);
    break;

  case afIPv6:
    IPv6Button->SetChecked(true);
    break;

  case afAuto:
  default:
    IPAutoButton->SetChecked(true);
    break;
  }

  if (SessionData->GetCodePage().IsEmpty())
  {
    CodePageEdit->SetText(CodePageEdit->GetItems()->GetString(0));
  }
  else
  {
    CodePageEdit->SetText(SessionData->GetCodePage());
  }

  // Proxy tab
  TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  const int32_t Index = ProxyMethodToIndex(SessionData->GetProxyMethod(), ProxyMethodCombo->GetItems());
  ProxyMethodCombo->SetItemIndex(Index);
  // if (SessionData->GetProxyMethod() != pmSystem)
  {
    ProxyHostEdit->SetText(SessionData->GetProxyHost());
    ProxyPortEdit->SetAsInteger(SessionData->GetProxyPort());
  }
  ProxyUsernameEdit->SetText(SessionData->GetProxyUsername());
  ProxyPasswordEdit->SetText(SessionData->GetProxyPassword());
  ProxyTelnetCommandEdit->SetText(SessionData->GetProxyTelnetCommand());
  ProxyLocalCommandEdit->SetText(SessionData->GetProxyLocalCommand());
  ProxyLocalhostCheck->SetChecked(SessionData->GetProxyLocalhost());
  switch (SessionData->GetProxyDNS())
  {
  case asOn:
    ProxyDNSOnButton->SetChecked(true);
    break;
  case asOff:
    ProxyDNSOffButton->SetChecked(true);
    break;
  default:
    ProxyDNSAutoButton->SetChecked(true);
    break;
  }

  // Tunnel tab
  TunnelCheck->SetChecked(SessionData->GetTunnel());
  TunnelUserNameEdit->SetText(SessionData->GetTunnelUserName());
  TunnelPortNumberEdit->SetAsInteger(SessionData->GetTunnelPortNumber());
  TunnelHostNameEdit->SetText(SessionData->GetTunnelHostName());
  TunnelPasswordEdit->SetText(SessionData->GetTunnelPassword());
  TunnelPrivateKeyEdit->SetText(SessionData->GetTunnelPublicKeyFile());
  if (SessionData->GetTunnelAutoassignLocalPortNumber())
  {
    TunnelLocalPortNumberEdit->SetText(TunnelLocalPortNumberEdit->GetItems()->GetString(0));
  }
  else
  {
    TunnelLocalPortNumberEdit->SetText(::IntToStr(SessionData->GetTunnelLocalPortNumber()));
  }

  // SSH tab
  CompressionCheck->SetChecked(SessionData->GetCompression());
  if (Ssh2DESCheck != nullptr)
  {
    Ssh2DESCheck->SetChecked(SessionData->GetSsh2DES());
  }

  /*switch (SessionData->GetSshProt())
  {
  case ssh1only:
    SshProt1onlyButton->SetChecked(true);
    break;
  case ssh1deprecated:
    SshProt1Button->SetChecked(true);
    break;
  case ssh2deprecated:
    SshProt2Button->SetChecked(true);
    break;
  case ssh2only:
    SshProt2onlyButton->SetChecked(true);
    break;
  }*/

  CipherListBox->GetItems()->BeginUpdate();
  {
    try__finally
    {
      CipherListBox->GetItems()->Clear();
      static_assert(NB_CIPHER_NAME_WARN + CIPHER_COUNT - 1 == NB_CIPHER_NAME_AESGCM, "CIPHER_COUNT");
      for (int32_t Index2 = 0; Index2 < CIPHER_COUNT; ++Index2)
      {
        const TObject * Obj = ToObj(nb::ToPtr(SessionData->GetCipher(Index2)));
        CipherListBox->GetItems()->AddObject(
          GetMsg(NB_CIPHER_NAME_WARN + nb::ToInt32(SessionData->GetCipher(Index2))),
          Obj);
      }
    }
    __finally
    {
      CipherListBox->GetItems()->EndUpdate();
    } end_try__finally
  }

  // KEX tab

  RekeyTimeEdit->SetAsInteger(SessionData->GetRekeyTime());
  RekeyDataEdit->SetText(SessionData->GetRekeyData());

  KexListBox->GetItems()->BeginUpdate();
  try__finally
  {
    KexListBox->GetItems()->Clear();
    static_assert(NB_KEX_NAME_WARN + KEX_COUNT - 1 == NB_KEX_NAME_NTRU_HYBRID, "KEX_COUNT");
    for (int32_t Index3 = 0; Index3 < KEX_COUNT; ++Index3)
    {
      KexListBox->GetItems()->AddObject(
        GetMsg(NB_KEX_NAME_WARN + nb::ToInt32(SessionData->GetKex(Index3))),
        ToObj(nb::ToPtr(SessionData->GetKex(Index3))));
    }
  }
  __finally
  {
    KexListBox->GetItems()->EndUpdate();
  } end_try__finally

  // Authentication tab
  SshNoUserAuthCheck->SetChecked(SessionData->GetSshNoUserAuth());
  TryAgentCheck->SetChecked(SessionData->GetTryAgent());
  // AuthTISCheck->SetChecked(SessionData->GetAuthTIS());
  AuthKICheck->SetChecked(SessionData->GetAuthKI());
  AuthKIPasswordCheck->SetChecked(SessionData->GetAuthKIPassword());
  AgentFwdCheck->SetChecked(SessionData->GetAgentFwd());
  AuthGSSAPICheck3->SetChecked(SessionData->GetAuthGSSAPI());
  GSSAPIFwdTGTCheck->SetChecked(SessionData->GetGSSAPIFwdTGT());

  // Bugs tab

  BUGS();

  // WebDAV tab
  WebDAVCompressionCheck->SetChecked(SessionData->GetCompression());

#undef TRISTATE

  const int32_t Button = ShowModal();
  const bool Result = (Button == brOK || Button == brConnect);
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

    UnicodeString HostName = HostNameEdit->GetText();
    UnicodeString UserName = UserNameEdit->GetText();
    UnicodeString Password = PasswordEdit->GetText();
    SessionData->RemoveProtocolPrefix(HostName);
    // parse username, password and directory, if any
    {
      int32_t Pos = HostName.RPos(L'@');
      if (Pos > 0)
      {
        const UnicodeString UserNameAndPassword = HostName.SubString(1, Pos - 1);
        Pos = UserNameAndPassword.RPos(L':');
        if (Pos > 0)
        {
          UserName = UserNameAndPassword.SubString(1, Pos - 1);
          Password = UserNameAndPassword.SubString(Pos + 1, - 1);
        }
        else
        {
          UserName = UserNameAndPassword;
        }
      }
    }
    // if (GetFSProtocol() == fsWebDAV)
    {
      ::AdjustRemoteDir(HostName, PortNumberEdit, RemoteDirectoryEdit);
    }

    // save session data

    // Basic tab
    SessionData->SetFSProtocol(GetFSProtocol());

    SessionData->SetHostName(HostName);
    SessionData->SetPortNumber(PortNumberEdit->GetAsInteger());
    SessionData->SessionSetUserName(UserName);
    SessionData->SetPassword(Password);
    SessionData->SetLoginType(ltNormal);
    SessionData->SetPublicKeyFile(PrivateKeyEdit->GetText());
    if (GetFSProtocol() == fsS3)
    {
      SessionData->SessionSetUserName(UserName);
      SessionData->SetPassword(Password);
      SessionData->SetFtps(ftpsImplicit); // TODO: get code from TLoginDialog::PortNumberEditChange
    }

    // Directories tab
    SessionData->SetRemoteDirectory(RemoteDirectoryEdit->GetText());
    SessionData->SetUpdateDirectories(UpdateDirectoriesCheck->GetChecked());
    SessionData->SetCacheDirectories(CacheDirectoriesCheck->GetChecked());
    SessionData->SetCacheDirectoryChanges(CacheDirectoryChangesCheck->GetChecked());
    SessionData->SetPreserveDirectoryChanges(PreserveDirectoryChangesCheck->GetChecked());
    SessionData->SetResolveSymlinks(ResolveSymlinksCheck->GetChecked());

    // Environment tab
    if (DSTModeUnixCheck->GetChecked())
    {
      SessionData->SetDSTMode(dstmUnix);
    }
    else if (DSTModeKeepCheck->GetChecked())
    {
      SessionData->SetDSTMode(dstmKeep);
    }
    else
    {
      SessionData->SetDSTMode(dstmWin);
    }
    if (EOLTypeCombo->GetItemIndex() == 0)
    {
      SessionData->SetEOLType(eolLF);
    }
    else
    {
      SessionData->SetEOLType(eolCRLF);
    }

    SessionData->SetDeleteToRecycleBin(DeleteToRecycleBinCheck->GetChecked());
    SessionData->SetOverwrittenToRecycleBin(OverwrittenToRecycleBinCheck->GetChecked());
    SessionData->SetRecycleBinPath(RecycleBinPathEdit->GetText());

    // SCP tab
    SessionData->SetDefaultShell(ShellEdit->GetText() == ShellEdit->GetItems()->GetString(0));
    SessionData->SetShell((SessionData->GetDefaultShell() ? UnicodeString() : ShellEdit->GetText()));
    SessionData->SetDetectReturnVar(ReturnVarEdit->GetText() == ReturnVarEdit->GetItems()->GetString(0));
    SessionData->SetReturnVar((SessionData->GetDetectReturnVar() ? UnicodeString() : ReturnVarEdit->GetText()));
    SessionData->SetLookupUserGroups(LookupUserGroupsCheck->GetChecked() ? asOn : asOff);
    SessionData->SetClearAliases(ClearAliasesCheck->GetChecked());
    SessionData->SetIgnoreLsWarnings(IgnoreLsWarningsCheck->GetChecked());
    SessionData->SetScp1Compatibility(Scp1CompatibilityCheck->GetChecked());
    SessionData->SetUnsetNationalVars(UnsetNationalVarsCheck->GetChecked());
    SessionData->SetListingCommand(ListingCommandEdit->GetText());
    SessionData->SetSCPLsFullTime(SCPLsFullTimeAutoCheck->GetChecked() ? asAuto : asOff);
    SessionData->SetTimeDifference(TDateTime(
        (nb::ToDouble(TimeDifferenceEdit->GetAsInteger()) / 24) +
        (nb::ToDouble(TimeDifferenceMinutesEdit->GetAsInteger()) / 24 / 60)));

    // SFTP tab

#define TRISTATE(COMBO, PROP, MSG) \
      SessionData->Set##PROP(sb##PROP, static_cast<TAutoSwitch>(2 - (COMBO)->GetItemIndex()));
    // SFTP_BUGS();
    SessionData->SetSFTPBug(sbSymlink, static_cast<TAutoSwitch>(2 - SFTPBugSymlinkCombo->GetItemIndex()));
    SessionData->SetSFTPBug(sbSignedTS, static_cast<TAutoSwitch>(2 - SFTPBugSignedTSCombo->GetItemIndex()));

    SessionData->SetSftpServer(
      (SftpServerEdit->GetText() == SftpServerEdit->GetItems()->GetString(0)) ?
      UnicodeString() : SftpServerEdit->GetText());
    SessionData->SetSFTPMaxVersion(SFTPMaxVersionCombo->GetItemIndex());
    SessionData->SetSFTPMinPacketSize(SFTPMinPacketSizeEdit->GetAsInteger());
    SessionData->SetSFTPMaxPacketSize(SFTPMaxPacketSizeEdit->GetAsInteger());

    // FTP tab
    SessionData->SetFtpUseMlsd(static_cast<TAutoSwitch>(2 - FtpUseMlsdCombo->GetItemIndex()));
    SessionData->SetFtpAllowEmptyPassword(FtpAllowEmptyPasswordCheck->GetChecked());
    SessionData->SetFtpDupFF(FtpDupFFCheck->GetChecked());
    SessionData->SetFtpUndupFF(FtpUndupFFCheck->GetChecked());
    SessionData->SetSslSessionReuse(SslSessionReuseCheck->GetChecked());
    TODO("TlsCertificateFileEdit->GetText()");
    // SessionData->SetTlsCertificateFile(PrivateKeyEdit->GetText());
    std::unique_ptr<TStrings> PostLoginCommands2(std::make_unique<TStringList>());
    for (int32_t Index4 = 0; Index4 < nb::ToInt32(_countof(PostLoginCommandsEdits)); ++Index4)
    {
      UnicodeString Text = PostLoginCommandsEdits[Index4]->GetText();
      if (!Text.IsEmpty())
      {
        PostLoginCommands2->Add(PostLoginCommandsEdits[Index4]->GetText());
      }
    }

    SessionData->SetPostLoginCommands(PostLoginCommands2->GetText());
    // if ((GetFSProtocol() == fsFTP) && (GetFtps() != ftpsNone))
    {
      SessionData->SetFtps(GetFtps());
    }
    /*else if (GetFSProtocol() != fsS3)
    {
      SessionData->SetFtps(ftpsNone);
    }*/

    if (FtpEncryptionCombo->GetVisible())
    switch (FtpEncryptionCombo->GetItemIndex())
    {
    case 0:
      SessionData->SetFtps(ftpsNone);
      break;
    case 1:
      SessionData->SetFtps(ftpsImplicit);
      break;
    case 2:
      SessionData->SetFtps(ftpsExplicitSsl);
      break;
    case 3:
      SessionData->SetFtps(ftpsExplicitTls);
      break;
    default:
      SessionData->SetFtps(ftpsNone);
      break;
    }

    // Connection tab
    SessionData->SetFtpPasvMode(FtpPasvModeCheck->GetChecked());
    SessionData->SetSendBuf(SshBufferSizeCheck->GetChecked() ? DefaultSendBuf : 0);
    SessionData->SetSshSimple(SshBufferSizeCheck->GetChecked());
    if (PingOffButton->GetChecked())
    {
      SessionData->SetPingType(ptOff);
    }
    else if (PingNullPacketButton->GetChecked())
    {
      SessionData->SetPingType(ptNullPacket);
    }
    else if (PingDummyCommandButton->GetChecked())
    {
      SessionData->SetPingType(ptDummyCommand);
    }
    else
    {
      SessionData->SetPingType(ptOff);
    }
    if (GetFSProtocol() == fsFTP)
    {
      if (PingOffButton->GetChecked())
      {
        SessionData->SetFtpPingType(fptOff);
      }
      else if (PingNullPacketButton->GetChecked())
      {
        SessionData->SetFtpPingType(fptDummyCommand0);
      }
      else if (PingDummyCommandButton->GetChecked())
      {
        SessionData->SetFtpPingType(fptDummyCommand);
      }
      else
      {
        SessionData->SetFtpPingType(fptOff);
      }
      SessionData->SetFtpPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
    else
    {
      SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
    SessionData->SetTimeout(TimeoutEdit->GetAsInteger());

    if (IPv4Button->GetChecked())
    {
      SessionData->SetAddressFamily(afIPv4);
    }
    else if (IPv6Button->GetChecked())
    {
      SessionData->SetAddressFamily(afIPv6);
    }
    else
    {
      SessionData->SetAddressFamily(afAuto);
    }
    SessionData->SetCodePage(
      (CodePageEdit->GetText() == CodePageEdit->GetItems()->GetString(0)) ?
      UnicodeString() : CodePageEdit->GetText());

    // Proxy tab
    SessionData->SetProxyMethod(GetProxyMethod());
    SessionData->SetFtpProxyLogonType(GetFtpProxyLogonType());
    SessionData->SetProxyHost(ProxyHostEdit->GetText());
    SessionData->SetProxyPort(ProxyPortEdit->GetAsInteger());
    SessionData->SetProxyUsername(ProxyUsernameEdit->GetText());
    SessionData->SetProxyPassword(ProxyPasswordEdit->GetText());
    SessionData->SetProxyTelnetCommand(ProxyTelnetCommandEdit->GetText());
    SessionData->SetProxyLocalCommand(ProxyLocalCommandEdit->GetText());
    SessionData->SetProxyLocalhost(ProxyLocalhostCheck->GetChecked());

    if (ProxyDNSOnButton->GetChecked())
    {
      SessionData->SetProxyDNS(asOn);
    }
    else if (ProxyDNSOffButton->GetChecked())
    {
      SessionData->SetProxyDNS(asOff);
    }
    else
    {
      SessionData->SetProxyDNS(asAuto);
    }

    // Tunnel tab
    SessionData->SetTunnel(TunnelCheck->GetChecked());
    SessionData->SetTunnelUserName(TunnelUserNameEdit->GetText());
    SessionData->SetTunnelPortNumber(TunnelPortNumberEdit->GetAsInteger());
    SessionData->SetTunnelHostName(TunnelHostNameEdit->GetText());
    SessionData->SetTunnelPassword(TunnelPasswordEdit->GetText());
    SessionData->SetTunnelPublicKeyFile(TunnelPrivateKeyEdit->GetText());
    if (TunnelLocalPortNumberEdit->GetText() == TunnelLocalPortNumberEdit->GetItems()->GetString(0))
    {
      SessionData->SetTunnelLocalPortNumber(0);
    }
    else
    {
      SessionData->SetTunnelLocalPortNumber(::StrToIntDef(TunnelLocalPortNumberEdit->GetText(), 0));
    }

    // SSH tab
    SessionData->SetCompression(CompressionCheck->GetChecked());
    if (Ssh2DESCheck != nullptr)
    {
      SessionData->SetSsh2DES(Ssh2DESCheck->GetChecked());
    }

    /*if (SshProt1onlyButton->GetChecked())
    {
      SessionData->SetSshProt(ssh1only);
    }
    else if (SshProt1Button->GetChecked())
    {
      SessionData->SetSshProt(ssh1deprecated);
    }
    else if (SshProt2Button->GetChecked())
    {
      SessionData->SetSshProt(ssh2deprecated);
    }
    else
    {
      SessionData->SetSshProt(ssh2only);
    }*/

    for (int32_t Index5 = 0; Index5 < CIPHER_COUNT; ++Index5)
    {
      TObject * Obj = cast_to<TObject>(CipherListBox->GetItems()->Get(Index5));
      SessionData->SetCipher(Index5, static_cast<TCipher>(nb::ToUIntPtr(Obj)));
    }

    // KEX tab

    SessionData->SetRekeyTime(RekeyTimeEdit->GetAsInteger());
    SessionData->SetRekeyData(RekeyDataEdit->GetText());

    for (int32_t Index6 = 0; Index6 < KEX_COUNT; ++Index6)
    {
      SessionData->SetKex(Index6, static_cast<TKex>(nb::ToUIntPtr(KexListBox->GetItems()->GetObj(Index))));
    }

    // Authentication tab
    SessionData->SetSshNoUserAuth(SshNoUserAuthCheck->GetChecked());
    SessionData->SetTryAgent(TryAgentCheck->GetChecked());
    // SessionData->SetAuthTIS(AuthTISCheck->GetChecked());
    SessionData->SetAuthKI(AuthKICheck->GetChecked());
    SessionData->SetAuthKIPassword(AuthKIPasswordCheck->GetChecked());
    SessionData->SetAgentFwd(AgentFwdCheck->GetChecked());
    SessionData->SetAuthGSSAPI(AuthGSSAPICheck3->GetChecked());
    SessionData->SetGSSAPIFwdTGT(GSSAPIFwdTGTCheck->GetChecked());

    // Bugs tab
    // BUGS();

    // WebDAV tab
    if (GetFSProtocol() == fsWebDAV)
      SessionData->SetCompression(WebDAVCompressionCheck->GetChecked());

#undef TRISTATE
    //SessionData->SetBug(sbIgnore1, static_cast<TAutoSwitch>(2 - BugIgnore1Combo->GetItemIndex()));
    //SessionData->SetBug(sbPlainPW1, static_cast<TAutoSwitch>(2 - BugPlainPW1Combo->GetItemIndex()));
    // SessionData->SetBug(sbRSA1, static_cast<TAutoSwitch>(2 - BugRSA1Combo->GetItemIndex()));
    SessionData->SetBug(sbHMAC2, static_cast<TAutoSwitch>(2 - BugHMAC2Combo->GetItemIndex()));
    SessionData->SetBug(sbDeriveKey2, static_cast<TAutoSwitch>(2 - BugDeriveKey2Combo->GetItemIndex()));
    SessionData->SetBug(sbRSAPad2, static_cast<TAutoSwitch>(2 - BugRSAPad2Combo->GetItemIndex()));
    SessionData->SetBug(sbPKSessID2, static_cast<TAutoSwitch>(2 - BugPKSessID2Combo->GetItemIndex()));
    SessionData->SetBug(sbRekey2, static_cast<TAutoSwitch>(2 - BugRekey2Combo->GetItemIndex()));
  }

  return Result;
}

void TSessionDialog::LoadPing(const TSessionData * SessionData)
{
  const TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
      AllowScpFallbackCheck->GetChecked());

  switch ((FSProtocol == fsFTP) ? static_cast<TPingType>(SessionData->GetFtpPingType()) : SessionData->GetPingType())
  {
  case ptOff:
    PingOffButton->SetChecked(true);
    break;
  case ptNullPacket:
    PingNullPacketButton->SetChecked(true);
    break;

  case ptDummyCommand:
    PingDummyCommandButton->SetChecked(true);
    break;

  default:
    PingOffButton->SetChecked(true);
    break;
  }
  PingIntervalSecEdit->SetAsInteger(
    (GetFSProtocol() == fsFTP) ?
    SessionData->GetFtpPingInterval() : SessionData->GetPingInterval());
}

void TSessionDialog::SavePing(TSessionData * SessionData)
{
  TPingType PingType;
  TFtpPingType FtpPingType;
  if (PingOffButton->GetChecked())
  {
    PingType = ptOff;
    FtpPingType = fptOff;
  }
  else if (PingNullPacketButton->GetChecked())
  {
    PingType = ptNullPacket;
    FtpPingType = fptDummyCommand0;
  }
  else if (PingDummyCommandButton->GetChecked())
  {
    PingType = ptDummyCommand;
    FtpPingType = fptDummyCommand;
  }
  else
  {
    PingType = ptOff;
    FtpPingType = fptOff;
  }
  const TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
      AllowScpFallbackCheck->GetChecked());
  if (FSProtocol == fsFTP)
  {
    SessionData->SetFtpPingType(FtpPingType);
    SessionData->SetFtpPingInterval(PingIntervalSecEdit->GetAsInteger());
  }
  else
  {
    SessionData->SetPingType(PingType);
    SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
  }
}

int32_t TSessionDialog::FSProtocolToIndex(TFSProtocol FSProtocol,
  bool & AllowScpFallback) const
{
  if (FSProtocol == fsSFTP)
  {
    AllowScpFallback = true;
    bool Dummy;
    return FSProtocolToIndex(fsSFTPonly, Dummy);
  }
  AllowScpFallback = false;
  for (int32_t Index = 0; Index < TransferProtocolCombo->GetItems()->GetCount(); ++Index)
  {
    if (FSOrder[Index] == FSProtocol)
    {
      return Index;
    }
  }
  // SFTP is always present
  return FSProtocolToIndex(fsSFTP, AllowScpFallback);
}

int32_t TSessionDialog::ProxyMethodToIndex(TProxyMethod ProxyMethod, TFarList * Items) const
{
  for (int32_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    TObject * Obj = Items->Get(Index);
    const TProxyMethod Method = ToProxyMethod(nb::ToInt32(nb::ToUIntPtr(Obj)));
    if (Method == ProxyMethod)
      return Index;
  }
  return -1;
}

TProxyMethod TSessionDialog::IndexToProxyMethod(int32_t Index, TFarList * Items) const
{
  TProxyMethod Result = pmNone;
  if (Index >= 0 && Index < Items->GetCount())
  {
    TObject * Obj = Items->Get(Index);
    Result = ToProxyMethod(nb::ToInt32(nb::ToIntPtr(Obj)));
  }
  return Result;
}

TFarComboBox * TSessionDialog::GetProxyMethodCombo() const
{
  return IsSshOrWebDAVProtocol(GetFSProtocol()) ? SshProxyMethodCombo : FtpProxyMethodCombo;
}

TFarComboBox * TSessionDialog::GetOtherProxyMethodCombo() const
{
  return IsSshOrWebDAVProtocol(GetFSProtocol()) ? FtpProxyMethodCombo : SshProxyMethodCombo;
}

TFSProtocol TSessionDialog::GetFSProtocol() const
{
  return IndexToFSProtocol(TransferProtocolCombo->GetItemIndex(),
    AllowScpFallbackCheck->GetChecked());
}

inline int32_t TSessionDialog::GetLastSupportedFtpProxyMethod() const
{
  return pmNone; // pmSystem;
}

bool TSessionDialog::GetSupportedFtpProxyMethod(int32_t Method) const
{
  return (Method >= 0) && (Method <= GetLastSupportedFtpProxyMethod());
}

TProxyMethod TSessionDialog::GetProxyMethod() const
{
  const TFarComboBox * ProxyMethodCombo = GetProxyMethodCombo();
  const TProxyMethod Result = IndexToProxyMethod(ProxyMethodCombo->GetItemIndex(), ProxyMethodCombo->GetItems());
  return Result;
}

int32_t TSessionDialog::GetFtpProxyLogonType() const
{
  int32_t Result = GetProxyMethod();
  if (Result > GetLastSupportedFtpProxyMethod())
    Result -= GetLastSupportedFtpProxyMethod();
  else
    Result = 0;
  return Result;
}

TFtps TSessionDialog::IndexToFtps(int32_t Index) const
{
  const bool InBounds = (Index != nb::NPOS) && (Index < FtpEncryptionCombo->GetItems()->GetCount());
  DebugAssert(InBounds);
  TFtps Result = ftpsNone;
  if (InBounds)
  {
    switch (Index)
    {
    case 0:
      Result = ftpsNone;
      break;

    case 1:
      Result = ftpsImplicit;
      break;

    case 2:
      Result = ftpsExplicitSsl;
      break;

    case 3:
      Result = ftpsExplicitTls;
      break;

    default:
      break;
    }
  }
  return Result;
}

TFtps TSessionDialog::GetFtps() const
{
  // TFSProtocol AFSProtocol = GetFSProtocol();
  // const int32_t Index = (((AFSProtocol == fsWebDAV) || (AFSProtocol == fsS3)) ? 1 : FtpEncryptionCombo->GetItemIndex());
  const int32_t Index = FtpEncryptionCombo->GetItemIndex();
  TFtps Ftps;
  switch (Index)
  {
    case 0:
      Ftps = ftpsNone;
      break;

    case 1:
      Ftps = ftpsImplicit;
      break;

    case 2:
      Ftps = ftpsExplicitSsl;
      break;
    default:
      Ftps = static_cast<TFtps>(IndexToFtps(FtpEncryptionCombo->GetItemIndex()));
      break;
  }
  // return static_cast<TFtps>(IndexToFtps(FtpEncryptionCombo->GetItemIndex()));
  return Ftps;
}

TFSProtocol TSessionDialog::IndexToFSProtocol(int32_t Index, bool AllowScpFallback) const
{
  TFSProtocol Result = fsSFTP;
  const bool InBounds = (Index >= 0) && (Index < nb::ToInt32(_countof(FSOrder)));
  DebugAssert(InBounds || (Index == -1));
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

TLoginType TSessionDialog::IndexToLoginType(int32_t Index) const
{
  const bool InBounds = (Index != nb::NPOS) && (Index <= ltNormal);
  DebugAssert(InBounds);
  TLoginType Result = ltAnonymous;
  if (InBounds)
  {
    Result = static_cast<TLoginType>(Index);
  }
  return Result;
}

bool TSessionDialog::VerifyKey(const UnicodeString & AFileName, bool /*TypeOnly*/)
{
  bool Result = false;

//  ::VerifyKey(AFileName, TypeOnly);
//  ::VerifyAndConvertKey(AFileName, ssh2only, false);
#if 0
  if (!::Trim(AFileName).IsEmpty())
  {
    TKeyType KeyType = GetKeyType(AFileName);
    UnicodeString Message;
    switch (KeyType)
    {
    case ktOpenSSHAuto:
      Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName, L"OpenSSH SSH-2");
      break;

    case ktOpenSSHPEM:
    case ktOpenSSHNew:
    case ktSSHCom:
      Message = FMTLOAD(KEY_TYPE_UNSUPPORTED2, AFileName, L"ssh.com SSH-2");
      break;

    case ktSSH1Public:
    case ktSSH2PublicRFC4716:
    case ktSSH2PublicOpenSSH:
      // noop
      // Do not even bother checking SSH protocol version
      break;

    case ktSSH1:
    case ktSSH2:
      if (!TypeOnly)
      {
        if ((KeyType == ktSSH1) !=
          (SshProt1onlyButton->GetChecked() || SshProt1Button->GetChecked()))
        {
          Message = FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
              AFileName, (KeyType == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"));
        }
      }
      break;

    default:
      DebugAssert(false);
    // fallthru
    case ktUnopenable:
    case ktUnknown:
      Message = FMTLOAD(KEY_TYPE_UNKNOWN2, AFileName);
      break;
    }

    if (!Message.IsEmpty())
    {
      TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
      Expects(WinSCPPlugin);
      Result = (WinSCPPlugin->MoreMessageDialog(Message, nullptr, qtWarning,
            qaIgnore | qaAbort) != qaAbort);
    }
  }
#endif
  return Result;
}

bool TSessionDialog::CloseQuery()
{
  bool CanClose = TTabbedDialog::CloseQuery();

  if (CanClose && (GetResult() != brCancel))
  {
    // TODO: check only if PrivateKeyEdit and TunnelPrivateKeyEdit were edited
    /*CanClose =
      VerifyKey(PrivateKeyEdit->GetText(), false) &&
      // for tunnel key do not check SSH version as it is not configurable
      VerifyKey(TunnelPrivateKeyEdit->GetText(), true);*/
    // DEBUG_PRINTF("CanClose2: %d", CanClose);
  }

  if (CanClose && !PasswordEdit->GetText().IsEmpty() &&
    !GetConfiguration()->GetDisablePasswordStoring() &&
    (PasswordEdit->GetText() != FSessionData->GetPassword()) &&
    (((GetResult() == brOK)) ||
      ((GetResult() == brConnect) && (FAction == saEdit))))
  {
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    CanClose = (WinSCPPlugin->MoreMessageDialog(GetMsg(NB_SAVE_PASSWORD), nullptr,
          qtWarning, qaOK | qaCancel) == qaOK);
  }

  return CanClose;
}

void TSessionDialog::SelectTab(int32_t Tab)
{
  TTabbedDialog::SelectTab(Tab);
  const TTabButton * SelectedTabBtn = GetTabButton(Tab);
  int32_t Index;
  /*for (Index = 0; Index < FTabs->Count; ++Index)
  {
    TTabButton * TabBtn = rtti::dyn_cast_or_null<TTabButton>(FTabs->GetItem(Index));
    // Button->SetBrackets(Button->GetTab() == Tab ? brTight : brNone);
    if (TabBtn == SelectedTabBtn)
      TabBtn->SetColor(0, static_cast<char>((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09));
    else
      TabBtn->SetColor(0, static_cast<char>((GetSystemColor(COL_DIALOGTEXT) & 0xF0)));
  }*/
  for (Index = 0; Index < FTabs->GetCount(); ++Index)
  {
    const TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
    if (TabBtn == SelectedTabBtn)
    {
      break;
    }
  }
  const int32_t SelectedTabIndex = Index;
  const int32_t VisibleTabsCount = GetVisibleTabsCount(SelectedTabIndex, false);
  if ((FFirstVisibleTabIndex < SelectedTabIndex - VisibleTabsCount) ||
    (SelectedTabIndex - VisibleTabsCount == 0))
  {
    FFirstVisibleTabIndex = SelectedTabIndex - VisibleTabsCount;
    ChangeTabs(FFirstVisibleTabIndex);
  }
}

void TSessionDialog::PrevTabClick(TFarButton * /*Sender*/, bool & Close)
{
  Key(nullptr, VK_PRIOR | (CTRLMASK << 16));
  Close = false;
}

void TSessionDialog::NextTabClick(TFarButton * /*Sender*/, bool & Close)
{
  Key(nullptr, VK_NEXT | (CTRLMASK << 16));
  Close = false;
}

void TSessionDialog::ChangeTabs(int32_t FirstVisibleTabIndex)
{
  // Calculate which tabs are visible
  const int32_t VisibleTabsCount = GetVisibleTabsCount(FirstVisibleTabIndex, true);
  const int32_t LastVisibleTabIndex = FirstVisibleTabIndex + VisibleTabsCount;
  // Change visibility
  for (int32_t Index = 0; Index < FirstVisibleTabIndex; ++Index)
  {
    TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
    TabBtn->SetVisible(false);
  }
  int32_t LeftPos = GetBorderBox()->GetLeft() + 2;
  for (int32_t Index = FirstVisibleTabIndex; Index <= LastVisibleTabIndex; ++Index)
  {
    TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
    const int32_t Width = TabBtn->GetWidth();
    TabBtn->SetLeft(LeftPos);
    TabBtn->SetWidth(Width);
    LeftPos += Width + 1;
    TabBtn->SetVisible(true);
  }
  for (int32_t Index = LastVisibleTabIndex + 1; Index < FTabs->GetCount(); ++Index)
  {
    TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
    TabBtn->SetVisible(false);
  }
}

int32_t TSessionDialog::GetVisibleTabsCount(int32_t TabIndex, bool Forward) const
{
  int32_t Result = 0;
  const int32_t PWidth = PrevTab->GetWidth();
  const int32_t NWidth = NextTab->GetWidth();
  const int32_t DialogWidth = GetBorderBox()->GetWidth() - 2 - PWidth - NWidth - 2;
  int32_t TabsWidth = 0;
  if (Forward)
  {
    for (int32_t Index = TabIndex; Index < FTabs->GetCount() - 1; ++Index)
    {
      const TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
      TabsWidth += TabBtn->GetWidth() + 1;
      const TTabButton * NextTabBtn = FTabs->GetAs<TTabButton>(Index + 1);
      const int32_t NextTabWidth = NextTabBtn->GetWidth() + 1;
      if (TabsWidth + NextTabWidth >= DialogWidth)
        break;
      Result++;
    }
  }
  else
  {
    for (int32_t Index = TabIndex; Index >= 1; Index--)
    {
      const TTabButton * TabBtn = FTabs->GetAs<TTabButton>(Index);
      TabsWidth += TabBtn->GetWidth() + 1;
      const TTabButton * PrevTabBtn = FTabs->GetAs<TTabButton>(Index - 1);
      const int32_t PrevTabWidth = PrevTabBtn->GetWidth() + 1;
      if (TabsWidth + PrevTabWidth >= DialogWidth)
        break;
      Result++;
    }
  }
  return Result;
}

void TSessionDialog::CipherButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->GetEnabled())
  {
    const int32_t Source = CipherListBox->GetItems()->GetSelected();
    const int32_t Dest = Source + Sender->GetResult();

    CipherListBox->GetItems()->Move(Source, Dest);
    CipherListBox->GetItems()->SetSelected(Dest);
  }

  Close = false;
}

void TSessionDialog::KexButtonClick(TFarButton * Sender, bool & Close)
{
  if (Sender->GetEnabled())
  {
    const int32_t Source = KexListBox->GetItems()->GetSelected();
    const int32_t Dest = Source + Sender->GetResult();

    KexListBox->GetItems()->Move(Source, Dest);
    KexListBox->GetItems()->SetSelected(Dest);
  }

  Close = false;
}

void TSessionDialog::AuthGSSAPICheckAllowChange(TFarDialogItem * /*Sender*/,
  void * NewState, bool & Allow)
{
  if ((nb::ToIntPtr(NewState) == BSTATE_CHECKED) && !HasGSSAPI(L""))
  {
    Allow = false;
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    WinSCPPlugin->MoreMessageDialog(GetMsg(NB_GSSAPI_NOT_INSTALLED),
      nullptr, qtError, qaOK);
  }
}

void TSessionDialog::UnixEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->SetItemIndex(0);
  DSTModeUnixCheck->SetChecked(true);
}

void TSessionDialog::WindowsEnvironmentButtonClick(
  TFarButton * /*Sender*/, bool & /*Close*/)
{
  EOLTypeCombo->SetItemIndex(1);
  DSTModeWinCheck->SetChecked(true);
}

void TSessionDialog::FillCodePageEdit()
{
  // CodePageEditAdd(CP_UTF8);
  CodePageEdit->GetItems()->AddObject(L"65001 (UTF-8)",
    ToObj(nb::ToUIntPtr(65001)));
  CodePageEditAdd(CP_ACP);
  CodePageEditAdd(CP_OEMCP);
  CodePageEditAdd(20866); // KOI8-r
}

void TSessionDialog::CodePageEditAdd(uint32_t Cp)
{
  CPINFOEX cpInfoEx{};
  nb::ClearStruct(cpInfoEx);
  if (::GetCodePageInfo(Cp, cpInfoEx))
  {
    CodePageEdit->GetItems()->AddObject(cpInfoEx.CodePageName,
      ToObj(nb::ToUIntPtr(cpInfoEx.CodePage)));
  }
}

int32_t TSessionDialog::AddTab(int32_t TabID, const UnicodeString & TabCaption)
{
  constexpr TFarButtonBrackets TabBrackets = brNone; // brSpace; //
  TTabButton * Tab = new TTabButton(this);
  Tab->SetTabName(TabCaption);
  Tab->SetTab(TabID);
  Tab->SetBrackets(TabBrackets);
  // SetTabCount(GetTabCount() + 1);
  Tab->SetCenterGroup(false);
  FTabs->Add(Tab);
  return GetItemIdx(Tab);
}

bool TWinSCPFileSystem::SessionDialog(TSessionData * SessionData,
  TSessionActionEnum Action)
{
  std::unique_ptr<TSessionDialog> Dialog(std::make_unique<TSessionDialog>(FPlugin, Action));
  const bool Result = Dialog->Execute(SessionData, Action);
  return Result;
}

class TRightsContainer final : public TFarDialogContainer
{
  NB_DISABLE_COPY(TRightsContainer)
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TRightsContainer); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TRightsContainer) || TFarDialogContainer::is(Kind); }
public:
  explicit TRightsContainer(TFarDialog * ADialog, bool AAnyDirectories,
    bool ShowButtons, bool ShowSpecials,
    TFarDialogItem * EnabledDependency);
protected:
  bool FAnyDirectories{false};
  bool Recursive{false};
  TFarCheckBox * FCheckBoxes[12]{};
  TRights::TState FFixedStates[12]{};
  TFarEdit * FOctalEdit{nullptr};
  TFarCheckBox * FDirectoriesXCheck{nullptr};

  virtual void Changed() override;
  void UpdateControls();

public:
  TRights GetRights();
  void SetRecursive(bool ARecursive);
  void SetRights(const TRights & Value);
  void SetAddXToDirectories(bool Value);
  bool GetAddXToDirectories() const;
  TFarCheckBox * GetChecks(TRights::TRight Right);
  TRights::TState GetStates(TRights::TRight Right);
  bool GetAllowUndef() const;
  void SetAllowUndef(bool Value);
  void SetStates(TRights::TRight Right, TRights::TState Value);
  void OctalEditExit(TObject * Sender);
  void RightsButtonClick(TFarButton * Sender, bool & Close);
};

TRightsContainer::TRightsContainer(TFarDialog * ADialog,
  bool AAnyDirectories, bool ShowButtons,
  bool ShowSpecials, TFarDialogItem * EnabledDependency) :
  TFarDialogContainer(OBJECT_CLASS_TRightsContainer, ADialog),
  FAnyDirectories(AAnyDirectories)
{
  GetDialog()->SetNextItemPosition(ipNewLine);

  static int32_t RowLabels[] =
  {
    NB_PROPERTIES_OWNER_RIGHTS,
    NB_PROPERTIES_GROUP_RIGHTS,
    NB_PROPERTIES_OTHERS_RIGHTS
  };
  static int32_t ColLabels[] =
  {
    NB_PROPERTIES_READ_RIGHTS,
    NB_PROPERTIES_WRITE_RIGHTS,
    NB_PROPERTIES_EXECUTE_RIGHTS
  };
  static int32_t SpecialLabels[] =
  {
    NB_PROPERTIES_SETUID_RIGHTS,
    NB_PROPERTIES_SETGID_RIGHTS,
    NB_PROPERTIES_STICKY_BIT_RIGHTS
  };

  for (int32_t RowIndex = 0; RowIndex < 3; ++RowIndex)
  {
    GetDialog()->SetNextItemPosition(ipNewLine);
    TFarText * Text = new TFarText(GetDialog());
    if (RowIndex == 0)
    {
      Text->SetTop(0);
    }
    Text->SetLeft(0);
    Add(Text);
    Text->SetEnabledDependency(EnabledDependency);
    Text->SetCaption(GetMsg(RowLabels[RowIndex]));

    GetDialog()->SetNextItemPosition(ipRight);

    for (int32_t ColIndex = 0; ColIndex < 3; ++ColIndex)
    {
      TFarCheckBox * CheckBox = new TFarCheckBox(GetDialog());
      FCheckBoxes[(RowIndex + 1) * 3 + ColIndex] = CheckBox;
      Add(CheckBox);
      CheckBox->SetEnabledDependency(EnabledDependency);
      CheckBox->SetCaption(GetMsg(ColLabels[ColIndex]));
    }

    if (ShowSpecials)
    {
      TFarCheckBox * CheckBox = new TFarCheckBox(GetDialog());
      Add(CheckBox);
      CheckBox->SetVisible(ShowSpecials);
      CheckBox->SetEnabledDependency(EnabledDependency);
      CheckBox->SetCaption(GetMsg(SpecialLabels[RowIndex]));
      FCheckBoxes[RowIndex] = CheckBox;
    }
    else
    {
      FCheckBoxes[RowIndex] = nullptr;
      FFixedStates[RowIndex] = TRights::rsNo;
    }
  }

  GetDialog()->SetNextItemPosition(ipNewLine);

  TFarText * Text = new TFarText(GetDialog());
  Add(Text);
  Text->SetEnabledDependency(EnabledDependency);
  Text->SetLeft(0);
  Text->SetCaption(GetMsg(NB_PROPERTIES_OCTAL));

  GetDialog()->SetNextItemPosition(ipRight);

  FOctalEdit = new TFarEdit(GetDialog());
  Add(FOctalEdit);
  FOctalEdit->SetEnabledDependency(EnabledDependency);
  FOctalEdit->SetWidth(5);
  FOctalEdit->SetMask(L"9999");
  FOctalEdit->SetOnExit(nb::bind(&TRightsContainer::OctalEditExit, this));

  if (ShowButtons)
  {
    GetDialog()->SetNextItemPosition(ipRight);

    TFarButton * Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(NB_PROPERTIES_NONE_RIGHTS));
    Button->SetTag(TRights::rfNo);
    Button->SetOnClick(nb::bind(&TRightsContainer::RightsButtonClick, this));

    Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(NB_PROPERTIES_DEFAULT_RIGHTS));
    Button->SetTag(TRights::rfDefault);
    Button->SetOnClick(nb::bind(&TRightsContainer::RightsButtonClick, this));

    Button = new TFarButton(GetDialog());
    Add(Button);
    Button->SetEnabledDependency(EnabledDependency);
    Button->SetCaption(GetMsg(NB_PROPERTIES_ALL_RIGHTS));
    Button->SetTag(TRights::rfAll);
    Button->SetOnClick(nb::bind(&TRightsContainer::RightsButtonClick, this));
  }

  GetDialog()->SetNextItemPosition(ipNewLine);

  if (FAnyDirectories)
  {
    FDirectoriesXCheck = new TFarCheckBox(GetDialog());
    Add(FDirectoriesXCheck);
    FDirectoriesXCheck->SetEnabledDependency(EnabledDependency);
    FDirectoriesXCheck->SetLeft(0);
    FDirectoriesXCheck->SetCaption(GetMsg(NB_PROPERTIES_DIRECTORIES_X));
  }
  else
  {
    FDirectoriesXCheck = nullptr;
  }
  nb::ClearArray(FFixedStates);
}

void TRightsContainer::RightsButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  TRights R = GetRights();
  R.SetNumber(static_cast<uint16_t>(Sender->GetTag()));
  SetRights(R);
}

void TRightsContainer::OctalEditExit(TObject * /*Sender*/)
{
  if (!::Trim(FOctalEdit->GetText()).IsEmpty())
  {
    TRights R = GetRights();
    R.SetOctal(::Trim(FOctalEdit->GetText()));
    SetRights(R);
  }
}

void TRightsContainer::UpdateControls()
{
  if (GetDialog()->GetHandle())
  {
    const TRights R = GetRights();

    if (FDirectoriesXCheck)
    {
      FDirectoriesXCheck->SetEnabled(Recursive ||
        !((R.GetNumberSet() & TRights::rfExec) == TRights::rfExec));
      if (!Recursive && (R.GetNumberSet() & TRights::rfExec) == TRights::rfExec)
      {
        SetAddXToDirectories(false);
      }
    }

    if (!FOctalEdit->Focused())
    {
      FOctalEdit->SetText(R.GetIsUndef() ? UnicodeString() : R.GetOctal());
    }
    else if (::Trim(FOctalEdit->GetText()).Length() >= 3)
    {
      try
      {
        OctalEditExit(nullptr);
      }
      catch (...)
      {
        DEBUG_PRINTF("TRightsContainer::UpdateControls: error during OctalEditExit");
      }
    }
  }
}

void TRightsContainer::Changed()
{
  TFarDialogContainer::Changed();

  if (GetDialog()->GetHandle())
  {
    UpdateControls();
  }
}

TFarCheckBox * TRightsContainer::GetChecks(TRights::TRight Right)
{
  DebugAssert((Right >= 0) && (nb::ToSizeT(Right) < _countof(FCheckBoxes)));
  return FCheckBoxes[nb::ToSizeT(Right)];
}

TRights::TState TRightsContainer::GetStates(TRights::TRight Right)
{
  const TFarCheckBox * CheckBox = GetChecks(Right);
  if (CheckBox != nullptr)
  {
    switch (CheckBox->GetSelected())
    {
    case BSTATE_UNCHECKED: return TRights::rsNo;
    case BSTATE_CHECKED: return TRights::rsYes;
    case BSTATE_3STATE:
    default: return TRights::rsUndef;
    }
  }
  return FFixedStates[Right];
}

void TRightsContainer::SetStates(TRights::TRight Right,
  TRights::TState Value)
{
  TFarCheckBox * CheckBox = GetChecks(Right);
  if (CheckBox != nullptr)
  {
    switch (Value)
    {
    case TRights::rsNo:
      CheckBox->SetSelected(BSTATE_UNCHECKED);
      break;
    case TRights::rsYes:
      CheckBox->SetSelected(BSTATE_CHECKED);
      break;
    case TRights::rsUndef:
      CheckBox->SetSelected(BSTATE_3STATE);
      break;
    }
  }
  else
  {
    FFixedStates[Right] = Value;
  }
}

TRights TRightsContainer::GetRights()
{
  TRights Result;
  Result.SetAllowUndef(GetAllowUndef());
  for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
  {
    Result.SetRightUndef(static_cast<TRights::TRight>(Right),
      GetStates(static_cast<TRights::TRight>(Right)));
  }
  return Result;
}

void TRightsContainer::SetRecursive(bool ARecursive)
{
  if (Recursive != ARecursive)
  {
    Recursive = ARecursive;
  }
}

void TRightsContainer::SetRights(const TRights & Value)
{
  if (GetRights() != Value)
  {
    GetDialog()->LockChanges();
    try__finally
    {
      SetAllowUndef(true); // temporarily
      for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
      {
        SetStates(static_cast<TRights::TRight>(Right),
          Value.GetRightUndef(static_cast<TRights::TRight>(Right)));
      }
      SetAllowUndef(Value.GetAllowUndef());
    }
    __finally
    {
      GetDialog()->UnlockChanges();
    } end_try__finally
  }
}

bool TRightsContainer::GetAddXToDirectories() const
{
  return FDirectoriesXCheck ? FDirectoriesXCheck->GetChecked() : false;
}

void TRightsContainer::SetAddXToDirectories(bool Value)
{
  if (FDirectoriesXCheck)
  {
    FDirectoriesXCheck->SetChecked(Value);
  }
}

bool TRightsContainer::GetAllowUndef() const
{
  const TFarCheckBox * CheckBox = FCheckBoxes[_countof(FCheckBoxes) - 1];
  DebugAssert(CheckBox != nullptr);
  return CheckBox && CheckBox->GetAllowGrayed();
}

void TRightsContainer::SetAllowUndef(bool Value)
{
  for (size_t Right = 0; Right < _countof(FCheckBoxes); Right++)
  {
    if (FCheckBoxes[Right] != nullptr)
    {
      FCheckBoxes[Right]->SetAllowGrayed(Value);
    }
  }
}

class TPropertiesDialog final : public TFarDialog
{
  NB_DISABLE_COPY(TPropertiesDialog)
public:
  explicit TPropertiesDialog(TCustomFarPlugin * AFarPlugin, TStrings * AFileList,
    const UnicodeString & Directory,
    const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
    int32_t AllowedChanges);

  bool Execute(TRemoteProperties * Properties);

protected:
  virtual const UUID * GetDialogGuid() const override { return &PropertiesDialogGuid; }
  virtual void Change() override;
  void UpdateProperties(TRemoteProperties & Properties) const;

private:
  bool FAnyDirectories{false};
  int32_t FAllowedChanges{0};
  TRemoteProperties FOrigProperties;
  bool FMultiple{false};

  TRightsContainer * RightsContainer{nullptr};
  TFarComboBox * OwnerComboBox{nullptr};
  TFarComboBox * GroupComboBox{nullptr};
  TFarCheckBox * RecursiveCheck{nullptr};
  TFarButton * OkButton{nullptr};
};

TPropertiesDialog::TPropertiesDialog(TCustomFarPlugin * AFarPlugin,
  TStrings * AFileList, const UnicodeString & /*Directory*/,
  const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
  int32_t AAllowedChanges) :
  TFarDialog(AFarPlugin),
  FAllowedChanges(AAllowedChanges)
{
  TFarDialog::InitDialog();
  DebugAssert(AFileList->GetCount() > 0);
  const TRemoteFile * OnlyFile = AFileList->GetAs<TRemoteFile>(0);
  DebugUsedParam(OnlyFile);
  DebugAssert(OnlyFile);
  FMultiple = (AFileList->GetCount() > 1);

  {
    std::unique_ptr<TStringList> UsedGroupList = std::make_unique<TStringList>();
    std::unique_ptr<TStringList> UsedUserList = std::make_unique<TStringList>();
    UsedGroupList->SetDuplicates(dupIgnore);
    UsedGroupList->SetSorted(true);
    UsedUserList->SetDuplicates(dupIgnore);
    UsedUserList->SetSorted(true);

    for (int32_t Index = 0; Index < UserList->GetCount(); ++Index)
    {
        UsedUserList->Add(UserList->Token(Index)->GetName());
    }

    for (int32_t Index = 0; Index < GroupList->GetCount(); ++Index)
    {
        UsedGroupList->Add(GroupList->Token(Index)->GetName());
    }

    int32_t Directories = 0;
    for (int32_t Index = 0; Index < AFileList->GetCount(); ++Index)
    {
      TRemoteFile * File = AFileList->GetAs<TRemoteFile>(Index);
      DebugAssert(File);
      if (UsedGroupList.get() && !File->GetFileGroup().GetName().IsEmpty())
      {
        UsedGroupList->Add(File->GetFileGroup().GetName());
      }
      if (UsedUserList.get() && !File->GetFileOwner().GetName().IsEmpty())
      {
        UsedUserList->Add(File->GetFileOwner().GetName());
      }
      if (File->GetIsDirectory())
      {
        Directories++;
      }
    }
    FAnyDirectories = (Directories > 0);

    SetCaption(GetMsg(NB_PROPERTIES_CAPTION));

    SetSize(TPoint(56, 19));

    const TRect CRect = GetClientRect();

    TFarText * Text = new TFarText(this);
    Text->SetCaption(GetMsg(NB_PROPERTIES_PROMPT));
    Text->SetCenterGroup(true);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCenterGroup(true);
    if (AFileList->GetCount() > 1)
    {
      Text->SetCaption(FORMAT(GetMsg(NB_PROPERTIES_PROMPT_FILES), AFileList->GetCount()));
    }
    else
    {
      Text->SetCaption(base::MinimizeName(AFileList->GetString(0), nb::ToInt32(GetClientSize().x), true));
    }
    const TRemoteFile * File = AFileList->GetAs<TRemoteFile>(0);
    if (!File->GetLinkTo().IsEmpty())
    {
      SetHeight(GetHeight() + 1);
      Text = new TFarText(this);
      Text->SetCaption(GetMsg(NB_PROPERTIES_LINKTO));

      SetNextItemPosition(ipRight);

      TFarEdit * Edit = new TFarEdit(this);
      Edit->SetText(File->GetLinkTo());
      Edit->SetReadOnly(true);

      SetNextItemPosition(ipNewLine);
    }

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(NB_PROPERTIES_OWNER));
    Text->SetEnabled((FAllowedChanges & cpOwner) != 0);

    SetNextItemPosition(ipRight);

    OwnerComboBox = new TFarComboBox(this);
    OwnerComboBox->SetWidth(20);
    OwnerComboBox->SetEnabled((FAllowedChanges & cpOwner) != 0);
    if (UsedUserList.get())
    {
      OwnerComboBox->GetItems()->Assign(UsedUserList.get());
    }

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(NB_PROPERTIES_GROUP));
    Text->SetEnabled((FAllowedChanges & cpGroup) != 0);

    SetNextItemPosition(ipRight);

    GroupComboBox = new TFarComboBox(this);
    GroupComboBox->SetWidth(OwnerComboBox->GetWidth());
    GroupComboBox->SetEnabled((FAllowedChanges & cpGroup) != 0);
    if (UsedGroupList.get())
    {
      GroupComboBox->GetItems()->Assign(UsedGroupList.get());
    }

    SetNextItemPosition(ipNewLine);

    TFarSeparator * Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(NB_PROPERTIES_RIGHTS));

    RightsContainer = new TRightsContainer(this, FAnyDirectories,
      true, true, nullptr);
    RightsContainer->SetEnabled(FAllowedChanges & cpMode);

    if (FAnyDirectories)
    {
      Separator = new TFarSeparator(this);
      Separator->SetPosition(Separator->GetPosition() + RightsContainer->GetTop());

      RecursiveCheck = new TFarCheckBox(this);
      RecursiveCheck->SetCaption(GetMsg(NB_PROPERTIES_RECURSIVE));
    }
    else
    {
      RecursiveCheck = nullptr;
    }

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(CRect.Bottom - 1);

    OkButton = new TFarButton(this);
    OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
    OkButton->SetDefault(true);
    OkButton->SetResult(brOK);
    OkButton->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    TFarButton * Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
  }
}

void TPropertiesDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    TRemoteProperties FileProperties;
    UpdateProperties(FileProperties);

    if (!FMultiple)
    {
      // when setting properties for one file only, allow undef state
      // only when the input right explicitly requires it or
      // when "recursive" is on (possible for directory only).
      const bool AllowUndef =
        (FOrigProperties.Valid.Contains(vpRights) &&
          FOrigProperties.Rights.GetAllowUndef()) ||
        ((RecursiveCheck != nullptr) && (RecursiveCheck->GetChecked()));
      if (!AllowUndef)
      {
        // when disallowing undef state, make sure, all undef are turned into unset
        RightsContainer->SetRights(TRights(RightsContainer->GetRights().GetNumberSet()));
      }
      RightsContainer->SetAllowUndef(AllowUndef);
    }

    OkButton->SetEnabled(
      // group name is specified or we set multiple-file properties and
      // no valid group was specified (there are at least two different groups)
      (!GroupComboBox->GetText().IsEmpty() ||
        (FMultiple && !FOrigProperties.Valid.Contains(vpGroup)) ||
        (FOrigProperties.Group.GetName() == GroupComboBox->GetText())) &&
      // same but with owner
      (!OwnerComboBox->GetText().IsEmpty() ||
        (FMultiple && !FOrigProperties.Valid.Contains(vpOwner)) ||
        (FOrigProperties.Owner.GetName() == OwnerComboBox->GetText())) &&
      ((FileProperties != FOrigProperties) || (RecursiveCheck && RecursiveCheck->GetChecked())));
  }
}

void TPropertiesDialog::UpdateProperties(TRemoteProperties & Properties) const
{
  if (FAllowedChanges & cpMode)
  {
    Properties.Valid << vpRights;
    Properties.Rights = RightsContainer->GetRights();
    Properties.AddXToDirectories = RightsContainer->GetAddXToDirectories();
  }

#define STORE_NAME(PROPERTY) \
    do { if (!PROPERTY ## ComboBox->GetText().IsEmpty() && \
        FAllowedChanges & cp ## PROPERTY) \
    { \
      Properties.Valid << vp ## PROPERTY; \
      Properties.PROPERTY.SetName(::Trim(PROPERTY ## ComboBox->GetText())); \
    } } while(0)
  STORE_NAME(Group);
  STORE_NAME(Owner);
#undef STORE_NAME

  Properties.Recursive = RecursiveCheck != nullptr && RecursiveCheck->GetChecked();
  RightsContainer->SetRecursive(Properties.Recursive);
}

bool TPropertiesDialog::Execute(TRemoteProperties * Properties)
{
  TValidProperties<TValidProperty> Valid;
  if (Properties->Valid.Contains(vpRights) && FAllowedChanges & cpMode)
  {
    Valid << vpRights;
  }
  if (Properties->Valid.Contains(vpOwner) && FAllowedChanges & cpOwner)
  {
    Valid << vpOwner;
  }
  if (Properties->Valid.Contains(vpGroup) && FAllowedChanges & cpGroup)
  {
    Valid << vpGroup;
  }
  FOrigProperties = *Properties;
  FOrigProperties.Valid = Valid;
  FOrigProperties.Recursive = false;

  if (Properties->Valid.Contains(vpRights))
  {
    RightsContainer->SetRights(Properties->Rights);
    RightsContainer->SetAddXToDirectories(Properties->AddXToDirectories);
  }
  else
  {
    RightsContainer->SetRights(TRights());
    RightsContainer->SetAddXToDirectories(false);
  }
  OwnerComboBox->SetText(Properties->Valid.Contains(vpOwner) ?
    Properties->Owner.GetName() : UnicodeString());
  GroupComboBox->SetText(Properties->Valid.Contains(vpGroup) ?
    Properties->Group.GetName() : UnicodeString());
  if (RecursiveCheck)
  {
    RecursiveCheck->SetChecked(Properties->Recursive);
  }

  const bool Result = ShowModal() != brCancel;
  if (Result)
  {
    *Properties = TRemoteProperties();
    UpdateProperties(*Properties);
  }
  return Result;
}

bool TWinSCPFileSystem::PropertiesDialog(TStrings * AFileList,
  const UnicodeString & ADirectory,
  const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
  TRemoteProperties * Properties, int32_t AllowedChanges)
{
  std::unique_ptr<TPropertiesDialog> Dialog(std::make_unique<TPropertiesDialog>(FPlugin, AFileList,
      ADirectory, GroupList, UserList, AllowedChanges));
  const bool Result = Dialog->Execute(Properties);
  return Result;
}

class TCopyParamsContainer final : public TFarDialogContainer
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TCopyParamsContainer); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TCopyParamsContainer) || TFarDialogContainer::is(Kind); }
public:
  explicit TCopyParamsContainer(TFarDialog * ADialog,
    uint32_t Options, uint32_t CopyParamAttrs) noexcept;

  void SetParams(const TCopyParamType & Value);
  TCopyParamType GetParams() const;
  int32_t GetHeight() const;

protected:
  TFarRadioButton * TMTextButton{nullptr};
  TFarRadioButton * TMBinaryButton{nullptr};
  TFarRadioButton * TMAutomaticButton{nullptr};
  TFarEdit * AsciiFileMaskEdit{nullptr};
  TRightsContainer * RightsContainer{nullptr};
  TFarRadioButton * CCNoChangeButton{nullptr};
  TFarRadioButton * CCUpperCaseButton{nullptr};
  TFarRadioButton * CCLowerCaseButton{nullptr};
  TFarRadioButton * CCFirstUpperCaseButton{nullptr};
  TFarRadioButton * CCLowerCaseShortButton{nullptr};
  TFarCheckBox * ReplaceInvalidCharsCheck{nullptr};
  TFarCheckBox * PreserveRightsCheck{nullptr};
  TFarCheckBox * PreserveTimeCheck{nullptr};
  TFarCheckBox * PreserveReadOnlyCheck{nullptr};
  TFarCheckBox * IgnorePermErrorsCheck{nullptr};
  TFarCheckBox * ClearArchiveCheck{nullptr};
  TFarCheckBox * CalculateSizeCheck{nullptr};
  TFarText * FileMaskText{nullptr};
  TFarEdit * FileMaskEdit{nullptr};
  TFarComboBox * SpeedCombo{nullptr};

  void ValidateMaskComboExit(TObject * Sender);
  void ValidateSpeedComboExit(TObject * Sender);
  virtual void Changed() override;
  void UpdateControls();

private:
  uint32_t FOptions{0};
  uint32_t FCopyParamAttrs{0};
  TCopyParamType FParams;
};

TCopyParamsContainer::TCopyParamsContainer(TFarDialog * ADialog,
  uint32_t Options, uint32_t CopyParamAttrs) noexcept :
  TFarDialogContainer(OBJECT_CLASS_TCopyParamsContainer, ADialog),
  FOptions(Options),
  FCopyParamAttrs(CopyParamAttrs)
{
  int32_t TMWidth = 37;

  SetLeft(GetLeft() - 1);

  TFarBox * Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Box->SetTop(0);
  Box->SetHeight(1);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(NB_TRANSFER_MODE));

  GetDialog()->SetNextItemPosition(ipRight);

  Box = new TFarBox(GetDialog());
  Add(Box);
  Box->SetLeft(Box->GetLeft() - 2);
  Box->SetRight(Box->GetRight() + 1);
  Box->SetCaption(GetMsg(NB_TRANSFER_UPLOAD_OPTIONS));

  GetDialog()->SetNextItemPosition(ipNewLine);

  TMTextButton = new TFarRadioButton(GetDialog());
  TMTextButton->SetLeft(1);
  Add(TMTextButton);
  int32_t TMTop = TMTextButton->GetTop();
  TMTextButton->SetCaption(GetMsg(NB_TRANSFER_MODE_TEXT));
  TMTextButton->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode));

  TMBinaryButton = new TFarRadioButton(GetDialog());
  TMBinaryButton->SetLeft(1);
  Add(TMBinaryButton);
  TMBinaryButton->SetCaption(GetMsg(NB_TRANSFER_MODE_BINARY));
  TMBinaryButton->SetEnabled(TMTextButton->GetEnabled());

  TMAutomaticButton = new TFarRadioButton(GetDialog());
  TMAutomaticButton->SetLeft(1);
  Add(TMAutomaticButton);
  TMAutomaticButton->SetCaption(GetMsg(NB_TRANSFER_MODE_AUTOMATIC));
  TMAutomaticButton->SetEnabled(TMTextButton->GetEnabled());

  TFarText * Text = new TFarText(GetDialog());
  Text->SetLeft(1);
  Add(Text);
  Text->SetCaption(GetMsg(NB_TRANSFER_MODE_MASK));
  Text->SetEnabledDependency(TMAutomaticButton);

  AsciiFileMaskEdit = new TFarEdit(GetDialog());
  AsciiFileMaskEdit->SetLeft(1);
  Add(AsciiFileMaskEdit);
  AsciiFileMaskEdit->SetEnabledDependency(TMAutomaticButton);
  AsciiFileMaskEdit->SetWidth(TMWidth);
  AsciiFileMaskEdit->SetHistory(ASCII_MASK_HISTORY);
  AsciiFileMaskEdit->SetOnExit(nb::bind(&TCopyParamsContainer::ValidateMaskComboExit, this));

  Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(NB_TRANSFER_FILENAME_MODIFICATION));

  CCNoChangeButton = new TFarRadioButton(GetDialog());
  CCNoChangeButton->SetLeft(1);
  Add(CCNoChangeButton);
  CCNoChangeButton->SetCaption(GetMsg(NB_TRANSFER_FILENAME_NOCHANGE));
  CCNoChangeButton->SetEnabled(true);

  GetDialog()->SetNextItemPosition(ipRight);

  CCUpperCaseButton = new TFarRadioButton(GetDialog());
  Add(CCUpperCaseButton);
  CCUpperCaseButton->SetCaption(GetMsg(NB_TRANSFER_FILENAME_UPPERCASE));
  CCUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  CCFirstUpperCaseButton = new TFarRadioButton(GetDialog());
  CCFirstUpperCaseButton->SetLeft(1);
  Add(CCFirstUpperCaseButton);
  CCFirstUpperCaseButton->SetCaption(GetMsg(NB_TRANSFER_FILENAME_FIRSTUPPERCASE));
  CCFirstUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipRight);

  CCLowerCaseButton = new TFarRadioButton(GetDialog());
  Add(CCLowerCaseButton);
  CCLowerCaseButton->SetCaption(GetMsg(NB_TRANSFER_FILENAME_LOWERCASE));
  CCLowerCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  CCLowerCaseShortButton = new TFarRadioButton(GetDialog());
  CCLowerCaseShortButton->SetLeft(1);
  Add(CCLowerCaseShortButton);
  CCLowerCaseShortButton->SetCaption(GetMsg(NB_TRANSFER_FILENAME_LOWERCASESHORT));
  CCLowerCaseShortButton->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipRight);

  ReplaceInvalidCharsCheck = new TFarCheckBox(GetDialog());
  Add(ReplaceInvalidCharsCheck);
  ReplaceInvalidCharsCheck->SetCaption(GetMsg(NB_TRANSFER_FILENAME_REPLACE_INVALID));
  ReplaceInvalidCharsCheck->SetEnabled(CCNoChangeButton->GetEnabled());

  GetDialog()->SetNextItemPosition(ipNewLine);

  Box = new TFarBox(GetDialog());
  Box->SetLeft(0);
  Add(Box);
  Box->SetWidth(TMWidth + 2);
  Box->SetCaption(GetMsg(NB_TRANSFER_DOWNLOAD_OPTIONS));

  PreserveReadOnlyCheck = new TFarCheckBox(GetDialog());
  Add(PreserveReadOnlyCheck);
  PreserveReadOnlyCheck->SetLeft(1);
  PreserveReadOnlyCheck->SetCaption(GetMsg(NB_TRANSFER_PRESERVE_READONLY));
  PreserveReadOnlyCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveReadOnly));
  int32_t TMBottom = PreserveReadOnlyCheck->GetTop();

  PreserveRightsCheck = new TFarCheckBox(GetDialog());
  Add(PreserveRightsCheck);
  PreserveRightsCheck->SetLeft(TMWidth + 3);
  PreserveRightsCheck->SetTop(TMTop);
  PreserveRightsCheck->SetBottom(TMTop);
  PreserveRightsCheck->SetCaption(GetMsg(NB_TRANSFER_PRESERVE_RIGHTS));
  PreserveRightsCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoRights));

  GetDialog()->SetNextItemPosition(ipBelow);

  RightsContainer = new TRightsContainer(GetDialog(), true, false,
    false, PreserveRightsCheck);
  RightsContainer->SetLeft(PreserveRightsCheck->GetActualBounds().Left);
  RightsContainer->SetTop(PreserveRightsCheck->GetActualBounds().Top + 1);

  IgnorePermErrorsCheck = new TFarCheckBox(GetDialog());
  Add(IgnorePermErrorsCheck);
  IgnorePermErrorsCheck->SetLeft(PreserveRightsCheck->GetLeft());
  IgnorePermErrorsCheck->SetTop(TMTop + 6);
  IgnorePermErrorsCheck->SetCaption(GetMsg(NB_TRANSFER_PRESERVE_PERM_ERRORS));

  ClearArchiveCheck = new TFarCheckBox(GetDialog());
  ClearArchiveCheck->SetLeft(IgnorePermErrorsCheck->GetLeft());
  Add(ClearArchiveCheck);
  ClearArchiveCheck->SetTop(TMTop + 7);
  ClearArchiveCheck->SetCaption(GetMsg(NB_TRANSFER_CLEAR_ARCHIVE));
  ClearArchiveCheck->SetEnabled(
    FLAGCLEAR(FOptions, coTempTransfer) &&
    FLAGCLEAR(CopyParamAttrs, cpaNoClearArchive));

  Box = new TFarBox(GetDialog());
  Box->SetTop(TMTop + 8);
  Add(Box);
  Box->SetBottom(Box->GetTop());
  Box->SetLeft(TMWidth + 3 - 1);
  Box->SetCaption(GetMsg(NB_TRANSFER_COMMON_OPTIONS));

  PreserveTimeCheck = new TFarCheckBox(GetDialog());
  Add(PreserveTimeCheck);
  PreserveTimeCheck->SetLeft(TMWidth + 3);
  PreserveTimeCheck->SetCaption(GetMsg(NB_TRANSFER_PRESERVE_TIMESTAMP));
  PreserveTimeCheck->SetEnabled(
    FLAGCLEAR(CopyParamAttrs, cpaNoPreserveTime));

  CalculateSizeCheck = new TFarCheckBox(GetDialog());
  CalculateSizeCheck->SetCaption(GetMsg(NB_TRANSFER_CALCULATE_SIZE));
  Add(CalculateSizeCheck);
  CalculateSizeCheck->SetLeft(TMWidth + 3);

  GetDialog()->SetNextItemPosition(ipNewLine);

  TFarSeparator * Separator = new TFarSeparator(GetDialog());
  Add(Separator);
  Separator->SetPosition(TMBottom + 1);
  Separator->SetCaption(GetMsg(NB_TRANSFER_OTHER));

  FileMaskText = new TFarText(GetDialog());
  FileMaskText->SetLeft(1);
  Add(FileMaskText);
  FileMaskText->SetCaption(GetMsg(NB_TRANSFER_FILE_MASK));

  GetDialog()->SetNextItemPosition(ipNewLine);

  FileMaskEdit = new TFarEdit(GetDialog());
  FileMaskEdit->SetLeft(1);
  Add(FileMaskEdit);
  FileMaskEdit->SetWidth(TMWidth);
  FileMaskEdit->SetHistory(WINSCP_FILE_MASK_HISTORY);
  FileMaskEdit->SetOnExit(nb::bind(&TCopyParamsContainer::ValidateMaskComboExit, this));
  FileMaskEdit->SetEnabled(true);

  GetDialog()->SetNextItemPosition(ipNewLine);

  Text = new TFarText(GetDialog());
  Add(Text);
  Text->SetCaption(GetMsg(NB_TRANSFER_SPEED));
  Text->MoveAt(TMWidth + 3, FileMaskText->GetTop());

  GetDialog()->SetNextItemPosition(ipRight);

  SpeedCombo = new TFarComboBox(GetDialog());
  Add(SpeedCombo);
  SpeedCombo->GetItems()->Add(LoadStr(SPEED_UNLIMITED));
  int32_t Speed = 1024;
  while (Speed >= 8)
  {
    SpeedCombo->GetItems()->Add(::IntToStr(Speed));
    Speed = Speed / 2;
  }
  SpeedCombo->SetOnExit(nb::bind(&TCopyParamsContainer::ValidateSpeedComboExit, this));

  GetDialog()->SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(GetDialog());
  Separator->SetPosition(FileMaskEdit->GetBottom() + 1);
  Separator->SetLeft(0);
  Add(Separator);
}

void TCopyParamsContainer::UpdateControls()
{
  if (IgnorePermErrorsCheck != nullptr)
  {
    IgnorePermErrorsCheck->SetEnabled(
      ((PreserveRightsCheck->GetEnabled() && PreserveRightsCheck->GetChecked()) ||
       (PreserveTimeCheck->GetEnabled() && PreserveTimeCheck->GetChecked())) &&
      FLAGCLEAR(FCopyParamAttrs, cpaNoIgnorePermErrors));
  }
}

void TCopyParamsContainer::Changed()
{
  TFarDialogContainer::Changed();

  if (GetDialog()->GetHandle())
  {
    UpdateControls();
  }
}

void TCopyParamsContainer::SetParams(const TCopyParamType & Value)
{
  if (TMBinaryButton->GetEnabled())
  {
    switch (Value.GetTransferMode())
    {
    case tmAscii:
      TMTextButton->SetChecked(true);
      break;

    case tmBinary:
      TMBinaryButton->SetChecked(true);
      break;

    default:
      TMAutomaticButton->SetChecked(true);
      break;
    }
  }
  else
  {
    TMBinaryButton->SetChecked(true);
  }

  AsciiFileMaskEdit->SetText(Value.GetAsciiFileMask().Masks());

  switch (Value.GetFileNameCase())
  {
  case ncLowerCase:
    CCLowerCaseButton->SetChecked(true);
    break;

  case ncUpperCase:
    CCUpperCaseButton->SetChecked(true);
    break;

  case ncFirstUpperCase:
    CCFirstUpperCaseButton->SetChecked(true);
    break;

  case ncLowerCaseShort:
    CCLowerCaseShortButton->SetChecked(true);
    break;

  default:
  case ncNoChange:
    CCNoChangeButton->SetChecked(true);
    break;
  }

  RightsContainer->SetAddXToDirectories(Value.GetAddXToDirectories());
  RightsContainer->SetRights(Value.GetRights());
  PreserveRightsCheck->SetChecked(Value.GetPreserveRights());
  IgnorePermErrorsCheck->SetChecked(Value.GetIgnorePermErrors());

  PreserveReadOnlyCheck->SetChecked(Value.GetPreserveReadOnly());
  ReplaceInvalidCharsCheck->SetChecked(
    Value.GetInvalidCharsReplacement() != NoReplacement);

  ClearArchiveCheck->SetChecked(Value.GetClearArchive());

  FileMaskEdit->SetText(Value.GetIncludeFileMask().Masks());
  PreserveTimeCheck->SetChecked(Value.GetPreserveTime());
  CalculateSizeCheck->SetChecked(Value.GetCalculateSize());

  SpeedCombo->SetText(SetSpeedLimit(Value.GetCPSLimit()));

  FParams = Value;
}

TCopyParamType TCopyParamsContainer::GetParams() const
{
  TCopyParamType Result = FParams;

  DebugAssert(TMTextButton->GetChecked() || TMBinaryButton->GetChecked() || TMAutomaticButton->GetChecked());
  if (TMTextButton->GetChecked())
  {
    Result.SetTransferMode(tmAscii);
  }
  else if (TMAutomaticButton->GetChecked())
  {
    Result.SetTransferMode(tmAutomatic);
  }
  else
  {
    Result.SetTransferMode(tmBinary);
  }

  if (Result.GetTransferMode() == tmAutomatic)
  {
    Result.GetAsciiFileMask().Masks(AsciiFileMaskEdit->GetText());
  }

  if (CCLowerCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncLowerCase);
  }
  else if (CCUpperCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncUpperCase);
  }
  else if (CCFirstUpperCaseButton->GetChecked())
  {
    Result.SetFileNameCase(ncFirstUpperCase);
  }
  else if (CCLowerCaseShortButton->GetChecked())
  {
    Result.SetFileNameCase(ncLowerCaseShort);
  }
  else
  {
    Result.SetFileNameCase(ncNoChange);
  }

  Result.SetAddXToDirectories(RightsContainer->GetAddXToDirectories());
  Result.SetRights(RightsContainer->GetRights());
  Result.SetPreserveRights(PreserveRightsCheck->GetChecked());
  Result.SetIgnorePermErrors(IgnorePermErrorsCheck->GetChecked());

  Result.SetReplaceInvalidChars(ReplaceInvalidCharsCheck->GetChecked());
  Result.SetPreserveReadOnly(PreserveReadOnlyCheck->GetChecked());

  Result.SetClearArchive(ClearArchiveCheck->GetChecked());

  Result.GetIncludeFileMask().Masks(FileMaskEdit->GetText());
  Result.SetPreserveTime(PreserveTimeCheck->GetChecked());
  Result.SetCalculateSize(CalculateSizeCheck->GetChecked());

  Result.SetCPSLimit(GetSpeedLimit(SpeedCombo->GetText()));

  return Result;
}

void TCopyParamsContainer::ValidateMaskComboExit(TObject * Sender)
{
  ValidateMaskEdit(static_cast<TFarEdit *>(Sender));
}

void TCopyParamsContainer::ValidateSpeedComboExit(TObject * /*Sender*/)
{
  try
  {
    GetSpeedLimit(SpeedCombo->GetText());
  }
  catch (...)
  {
    SpeedCombo->SetFocus();
    throw;
  }
}

int32_t TCopyParamsContainer::GetHeight() const
{
  return 16;
}

class TCopyDialog final : public TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TCopyDialog(TCustomFarPlugin * AFarPlugin,
    bool ToRemote, bool Move, const TStrings * AFileList, uint32_t Options, uint32_t CopyParamAttrs) noexcept;

  bool Execute(UnicodeString & TargetDirectory, TGUICopyParamType * Params);

protected:
  virtual bool CloseQuery() override;
  virtual void Change() override;
  void CustomCopyParam();

  void CopyParamListerClick(TFarDialogItem * Item, const MOUSE_EVENT_RECORD * Event);
  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);

private:
  TFarEdit * DirectoryEdit{nullptr};
  TFarLister * CopyParamLister{nullptr};
  TFarCheckBox * NewerOnlyCheck{nullptr};
  TFarCheckBox * SaveSettingsCheck{nullptr};
  TFarCheckBox * QueueCheck{nullptr};
  TFarCheckBox * QueueNoConfirmationCheck{nullptr};

  const TStrings * FFileList{nullptr};
  uint32_t FOptions{0};
  uint32_t FCopyParamAttrs{0};
  TGUICopyParamType FCopyParams;
  bool FToRemote{false};
};

TCopyDialog::TCopyDialog(TCustomFarPlugin * AFarPlugin,
  bool ToRemote, bool Move, const TStrings * AFileList,
  uint32_t Options, uint32_t CopyParamAttrs) noexcept :
  TFarDialog(AFarPlugin),
  FFileList(AFileList),
  FOptions(Options),
  FCopyParamAttrs(CopyParamAttrs),
  FToRemote(ToRemote)
{
  TFarDialog::InitDialog();
  DebugAssert(FFileList);
  constexpr int32_t DlgLength = 78;
  SetSize(TPoint(DlgLength, 12 + (FLAGCLEAR(FOptions, coTempTransfer) ? 4 : 0)));

  SetCaption(GetMsg(Move ? NB_MOVE_TITLE : NB_COPY_TITLE));

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    UnicodeString Prompt;
    if (FFileList->GetCount() > 1)
    {
      Prompt = FORMAT(GetMsg(Move ? NB_MOVE_FILES_PROMPT : NB_COPY_FILES_PROMPT), FFileList->GetCount());
    }
    else
    {
      const UnicodeString PromptMsg = GetMsg(Move ? NB_MOVE_FILE_PROMPT : NB_COPY_FILE_PROMPT);
      const UnicodeString FileName = FFileList->GetString(0);
      const UnicodeString OnlyFileName = ToRemote ?
        base::ExtractFileName(FileName, false) :
        base::UnixExtractFileName(FileName);
      const UnicodeString MinimizedName = base::MinimizeName(OnlyFileName, DlgLength - PromptMsg.Length() - 6, false);
      Prompt = FORMAT(PromptMsg, MinimizedName);
    }

    TFarText * Text = new TFarText(this);
    Text->SetCaption(Prompt);

    DirectoryEdit = new TFarEdit(this);
    DirectoryEdit->SetHistory(ToRemote ? REMOTE_DIR_HISTORY : "Copy");
  }

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(nb::bind(&TCopyDialog::CopyParamListerClick, this));

  new TFarSeparator(this);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck = new TFarCheckBox(this);
    NewerOnlyCheck->SetCaption(GetMsg(NB_TRANSFER_NEWER_ONLY));
    NewerOnlyCheck->SetEnabled(FLAGCLEAR(FOptions, coDisableNewerOnly));

    QueueCheck = new TFarCheckBox(this);
    QueueCheck->SetCaption(GetMsg(NB_TRANSFER_QUEUE));

    SetNextItemPosition(ipRight);

    QueueNoConfirmationCheck = new TFarCheckBox(this);
    QueueNoConfirmationCheck->SetCaption(GetMsg(NB_TRANSFER_QUEUE_NO_CONFIRMATION));
    QueueNoConfirmationCheck->SetEnabledDependency(QueueCheck);

    SetNextItemPosition(ipNewLine);
  }
  else
  {
    DebugAssert(FLAGSET(FOptions, coDisableNewerOnly));
  }

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(NB_TRANSFER_REUSE_SETTINGS));

  new TFarSeparator(this);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(nb::bind(&TCopyDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_OK));
  Button->SetDefault(true);
  Button->SetResult(brOK);
  Button->SetCenterGroup(true);
  Button->SetEnabledDependency(
    ((Options & coTempTransfer) == 0) ? DirectoryEdit : nullptr);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

bool TCopyDialog::Execute(UnicodeString & TargetDirectory,
  TGUICopyParamType * Params)
{
  FCopyParams.Assign(Params);

  if (FLAGCLEAR(FOptions, coTempTransfer))
  {
    NewerOnlyCheck->SetChecked(FLAGCLEAR(FOptions, coDisableNewerOnly) && Params->GetNewerOnly());

    UnicodeString FileMask = Params->GetFileMask();
    const UnicodeString Directory = FToRemote ?
      base::UnixIncludeTrailingBackslash(TargetDirectory) :
      ::IncludeTrailingBackslash(TargetDirectory);
    if (FFileList->GetCount() == 1)
    {
      UnicodeString DestFileName = FFileList->GetString(0);
      DestFileName = FToRemote ? DestFileName : FCopyParams.ChangeFileName(DestFileName, osRemote, true);
      FileMask = base::ExtractFileName(DestFileName, false);
    }
    DirectoryEdit->SetText(Directory + FileMask);
    QueueCheck->SetChecked(Params->GetQueue());
    QueueNoConfirmationCheck->SetChecked(Params->GetQueueNoConfirmation());
  }

  const bool Result = ShowModal() != brCancel;

  if (Result)
  {
    Params->Assign(&FCopyParams);

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
      UnicodeString NewTargetDirectory;
      if (FToRemote)
      {
        // Params->SetFileMask(base::UnixExtractFileName(DirectoryEdit->GetText()));
        NewTargetDirectory = base::UnixExtractFilePath(DirectoryEdit->GetText());
        if (!NewTargetDirectory.IsEmpty())
          TargetDirectory = NewTargetDirectory;
      }
      else
      {
        Params->SetFileMask(base::ExtractFileName(DirectoryEdit->GetText(), false));
        NewTargetDirectory = ::ExtractFilePath(DirectoryEdit->GetText());
        if (!NewTargetDirectory.IsEmpty())
          TargetDirectory = NewTargetDirectory;
      }

      Params->SetNewerOnly(FLAGCLEAR(FOptions, coDisableNewerOnly) && NewerOnlyCheck->GetChecked());

      Params->SetQueue(QueueCheck->GetChecked());
      Params->SetQueueNoConfirmation(QueueNoConfirmationCheck->GetChecked());
    }

    GetConfiguration()->BeginUpdate();
    try__finally
    {
      if (SaveSettingsCheck->GetChecked())
      {
        GetGUIConfiguration()->SetDefaultCopyParam(*Params);
      }
    }
    __finally
    {
      GetConfiguration()->EndUpdate();
    } end_try__finally
  }
  return Result;
}

bool TCopyDialog::CloseQuery()
{
  const bool CanClose = TFarDialog::CloseQuery();

  if (CanClose && GetResult() >= 0)
  {
    if (!FToRemote && ((FOptions & coTempTransfer) == 0))
    {
      const UnicodeString Directory = ::ExtractFilePath(DirectoryEdit->GetText());
      if (!Directory.IsEmpty() && !base::DirectoryExists(Directory))
      {
        TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
        Ensures(WinSCPPlugin);

        if (WinSCPPlugin->MoreMessageDialog(FORMAT(GetMsg(NB_CREATE_LOCAL_DIRECTORY), Directory),
            nullptr, qtConfirmation, qaOK | qaCancel) != qaCancel)
        {
          if (!::SysUtulsForceDirectories(ApiPath(Directory)))
          {
            DirectoryEdit->SetFocus();
            throw ExtException(FORMAT(GetMsg(NB_CREATE_LOCAL_DIR_ERROR), Directory));
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

void TCopyDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    const UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", FCopyParamAttrs);
    std::unique_ptr<TStrings> InfoStrLines(std::make_unique<TStringList>());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

void TCopyDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TCopyDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, const MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TCopyDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, FCopyParamAttrs))
  {
    Change();
  }
}

bool TWinSCPFileSystem::CopyDialog(bool ToRemote,
  bool Move, const TStrings * AFileList,
  uint32_t Options,
  uint32_t CopyParamAttrs,
  UnicodeString & TargetDirectory,
  TGUICopyParamType * Params)
{
  std::unique_ptr<TCopyDialog> Dialog(std::make_unique<TCopyDialog>(FPlugin, ToRemote,
    Move, AFileList, Options, CopyParamAttrs));
  const bool Result = Dialog->Execute(TargetDirectory, Params);
  return Result;
}

bool TWinSCPPlugin::CopyParamDialog(const UnicodeString & Caption,
  TCopyParamType & CopyParam, uint32_t CopyParamAttrs)
{
  std::unique_ptr<TWinSCPDialog> DialogPtr(std::make_unique<TWinSCPDialog>(this));
  TWinSCPDialog * Dialog = DialogPtr.get();

  Dialog->SetCaption(Caption);

  // temporary
  Dialog->SetSize(TPoint(78, 10));

  TCopyParamsContainer * CopyParamsContainer = new TCopyParamsContainer(
    Dialog, 0, CopyParamAttrs);

  Dialog->SetSize(TPoint(78, 2 + nb::ToInt32(CopyParamsContainer->GetHeight()) + 3));

  Dialog->SetNextItemPosition(ipNewLine);

  Dialog->AddStandardButtons(2, true);

  CopyParamsContainer->SetParams(CopyParam);

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    CopyParam = CopyParamsContainer->GetParams();
  }
  return Result;
}

bool TWinSCPPlugin::CopyParamCustomDialog(TCopyParamType & CopyParam,
  uint32_t CopyParamAttrs)
{
  return CopyParamDialog(GetMsg(NB_COPY_PARAM_CUSTOM_TITLE), CopyParam, CopyParamAttrs);
}

class TLinkDialog : TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TLinkDialog(TCustomFarPlugin * AFarPlugin,
    bool Edit, bool AllowSymbolic);

  bool Execute(UnicodeString & AFileName, UnicodeString & PointTo,
    bool & Symbolic);

protected:
  virtual void Change() override;

private:
  TFarEdit * FileNameEdit{nullptr};
  TFarEdit * PointToEdit{nullptr};
  TFarCheckBox * SymbolicCheck{nullptr};
  TFarButton * OkButton{nullptr};
};

TLinkDialog::TLinkDialog(TCustomFarPlugin * AFarPlugin,
  bool Edit, bool AllowSymbolic) : TFarDialog(AFarPlugin)
{
  SetSize(TPoint(76, 12));
  const TRect CRect = GetClientRect();

  SetCaption(GetMsg(Edit ? NB_STRING_LINK_EDIT_CAPTION : NB_STRING_LINK_ADD_CAPTION));

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_STRING_LINK_FILE));
  Text->SetEnabled(!Edit);

  FileNameEdit = new TFarEdit(this);
  FileNameEdit->SetEnabled(!Edit);
  FileNameEdit->SetHistory(LINK_FILENAME_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_STRING_LINK_POINT_TO));

  PointToEdit = new TFarEdit(this);
  PointToEdit->SetHistory(LINK_POINT_TO_HISTORY);

  new TFarSeparator(this);

  SymbolicCheck = new TFarCheckBox(this);
  SymbolicCheck->SetCaption(GetMsg(NB_STRING_LINK_SYMLINK));
  SymbolicCheck->SetEnabled(AllowSymbolic && !Edit);

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetPosition(CRect.Bottom - 1);

  OkButton = new TFarButton(this);
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
  Button->SetResult(brCancel);
  Button->SetCenterGroup(true);
}

void TLinkDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    OkButton->SetEnabled(!FileNameEdit->GetText().IsEmpty() &&
      !PointToEdit->GetText().IsEmpty());
  }
}

bool TLinkDialog::Execute(UnicodeString & AFileName, UnicodeString & PointTo,
  bool & Symbolic)
{
  FileNameEdit->SetText(AFileName);
  PointToEdit->SetText(PointTo);
  SymbolicCheck->SetChecked(Symbolic);

  const bool Result = ShowModal() != brCancel;
  if (Result)
  {
    AFileName = FileNameEdit->GetText();
    PointTo = PointToEdit->GetText();
    Symbolic = SymbolicCheck->GetChecked();
  }
  return Result;
}

bool TWinSCPFileSystem::LinkDialog(UnicodeString & AFileName,
  UnicodeString & PointTo, bool & Symbolic, bool Edit, bool AllowSymbolic)
{
  std::unique_ptr<TLinkDialog> Dialog(std::make_unique<TLinkDialog>(FPlugin, Edit, AllowSymbolic));
  const bool Result = Dialog->Execute(AFileName, PointTo, Symbolic);
  return Result;
}

using TFeedFileSystemDataEvent = nb::FastDelegate3<void,
  TObject * /*Control*/, int32_t /*Label*/, const UnicodeString & /*Value*/>;

class TLabelList;
class TFileSystemInfoDialog final : TTabbedDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  enum
  {
    tabProtocol = 1,
    tabCapabilities,
    tabSpaceAvailable,
    tabCount
  };

  explicit TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
    TGetSpaceAvailableEvent && OnGetSpaceAvailable) noexcept;
  virtual ~TFileSystemInfoDialog() noexcept override;
  void Execute(const TSessionInfo & SessionInfo,
    const TFileSystemInfo & FileSystemInfo, const UnicodeString & SpaceAvailablePath);

protected:
  void Feed(TFeedFileSystemDataEvent && AddItem);
  UnicodeString CapabilityStr(TFSCapability Capability);
  UnicodeString CapabilityStr(TFSCapability Capability1,
    TFSCapability Capability2);
  UnicodeString SpaceStr(int64_t Bytes) const;
  void ControlsAddItem(TObject * AControl, int32_t Label, const UnicodeString & Value);
  void CalculateMaxLenAddItem(TObject * AControl, int32_t Label, const UnicodeString & Value) const;
  void ClipboardAddItem(TObject * AControl, int32_t Label, const UnicodeString & Value);
  void FeedControls();
  void UpdateControls();
  TLabelList * CreateLabelArray(int32_t Count);
  virtual void SelectTab(int32_t Tab) override;
  virtual void Change() override;
  void SpaceAvailableButtonClick(TFarButton * Sender, bool & Close);
  void ClipboardButtonClick(TFarButton * Sender, bool & Close);
  void CheckSpaceAvailable();
  void NeedSpaceAvailable();
  bool SpaceAvailableSupported() const;
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode) override;

private:
  TGetSpaceAvailableEvent FOnGetSpaceAvailable;
  TFileSystemInfo FFileSystemInfo;
  TSessionInfo FSessionInfo;
  bool FSpaceAvailableLoaded{false};
  TSpaceAvailable FSpaceAvailable;
  TObject * FLastFeededControl{nullptr};
  int32_t FLastListItem{0};
  UnicodeString FClipboard;

  gsl::owner<TLabelList *> ServerLabels{nullptr};
  gsl::owner<TLabelList *> ProtocolLabels{nullptr};
  gsl::owner<TLabelList *> SpaceAvailableLabels{nullptr};
  TTabButton * SpaceAvailableTab{nullptr};
  TFarText * HostKeyFingerprintLabel{nullptr};
  TFarEdit * HostKeyFingerprintEdit{nullptr};
  TFarText * InfoLabel{nullptr};
  TFarSeparator * InfoSeparator{nullptr};
  TFarLister * InfoLister{nullptr};
  TFarEdit * SpaceAvailablePathEdit{nullptr};
  TFarButton * OkButton{nullptr};
};

class TLabelList : public TList
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TLabelList); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TLabelList) || TList::is(Kind); }
public:
  explicit TLabelList() noexcept :
    TList(OBJECT_CLASS_TLabelList)
  {
  }

  int32_t MaxLen{0};
};

TFileSystemInfoDialog::TFileSystemInfoDialog(TCustomFarPlugin * AFarPlugin,
  TGetSpaceAvailableEvent && OnGetSpaceAvailable) noexcept :
  TTabbedDialog(AFarPlugin, tabCount)
{
  FOnGetSpaceAvailable = OnGetSpaceAvailable;

  SetSize(TPoint(73, 22));
  SetCaption(GetMsg(NB_SERVER_PROTOCOL_INFORMATION));

  TTabButton * Tab = new TTabButton(this);
  Tab->SetTabName(GetMsg(NB_SERVER_PROTOCOL_TAB_PROTOCOL));
  Tab->SetTab(tabProtocol);

  SetNextItemPosition(ipRight);

  Tab = new TTabButton(this);
  Tab->SetTabName(GetMsg(NB_SERVER_PROTOCOL_TAB_CAPABILITIES));
  Tab->SetTab(tabCapabilities);

  SpaceAvailableTab = new TTabButton(this);
  SpaceAvailableTab->SetTabName(GetMsg(NB_SERVER_PROTOCOL_TAB_SPACE_AVAILABLE));
  SpaceAvailableTab->SetTab(tabSpaceAvailable);

  // Server tab

  SetNextItemPosition(ipNewLine);
  SetDefaultGroup(tabProtocol);

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_SERVER_INFORMATION_GROUP));
  const int32_t GroupTop = Separator->GetTop();

  ServerLabels = CreateLabelArray(5);

  new TFarSeparator(this);

  HostKeyFingerprintLabel = new TFarText(this);
  HostKeyFingerprintLabel->SetCaption(GetMsg(NB_SERVER_HOST_KEY));
  HostKeyFingerprintEdit = new TFarEdit(this);
  HostKeyFingerprintEdit->SetReadOnly(true);

  // Protocol tab

  SetDefaultGroup(tabCapabilities);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_PROTOCOL_INFORMATION_GROUP));
  Separator->SetPosition(GroupTop);

  ProtocolLabels = CreateLabelArray(9);

  InfoSeparator = new TFarSeparator(this);
  InfoSeparator->SetCaption(GetMsg(NB_PROTOCOL_INFO_GROUP));

  InfoLister = new TFarLister(this);
  InfoLister->SetHeight(4);
  InfoLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  // Right edge is adjusted in FeedControls

  // Space available tab

  SetDefaultGroup(tabSpaceAvailable);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_SPACE_AVAILABLE_GROUP));
  Separator->SetPosition(GroupTop);

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_SPACE_AVAILABLE_PATH));

  SetNextItemPosition(ipRight);

  SpaceAvailablePathEdit = new TFarEdit(this);
  SpaceAvailablePathEdit->SetRight(-(nb::ToInt32(GetMsg(NB_SPACE_AVAILABLE_CHECK_SPACE).Length() + 11)));

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_SPACE_AVAILABLE_CHECK_SPACE));
  Button->SetEnabledDependency(SpaceAvailablePathEdit);
  Button->SetOnClick(nb::bind(&TFileSystemInfoDialog::SpaceAvailableButtonClick, this));

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  SpaceAvailableLabels = CreateLabelArray(5);

  // Buttons

  SetDefaultGroup(0);

  Separator = new TFarSeparator(this);
  Separator->SetPosition(GetClientRect().Bottom - 1);

  Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_SERVER_PROTOCOL_COPY_CLIPBOARD));
  Button->SetOnClick(nb::bind(&TFileSystemInfoDialog::ClipboardButtonClick, this));
  Button->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  OkButton = new TFarButton(this);
  OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
  OkButton->SetDefault(true);
  OkButton->SetResult(brOK);
  OkButton->SetCenterGroup(true);
}

TFileSystemInfoDialog::~TFileSystemInfoDialog() noexcept
{
  SAFE_DESTROY(ServerLabels);
  SAFE_DESTROY(ProtocolLabels);
  SAFE_DESTROY(SpaceAvailableLabels);
}

TLabelList * TFileSystemInfoDialog::CreateLabelArray(int32_t Count)
{
  std::unique_ptr<TLabelList> List(std::make_unique<TLabelList>());
  for (int32_t Index = 0; Index < Count; ++Index)
  {
    List->Add(new TFarText(this));
  }
  return List.release();
}

UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability)
{
  return BooleanToStr(FFileSystemInfo.IsCapable[Capability]);
}

UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability1,
  TFSCapability Capability2)
{
  return FORMAT("%s/%s", CapabilityStr(Capability1), CapabilityStr(Capability2));
}

UnicodeString TFileSystemInfoDialog::SpaceStr(int64_t Bytes) const
{
  UnicodeString Result;
  if (Bytes == 0)
  {
    Result = GetMsg(NB_SPACE_AVAILABLE_BYTES_UNKNOWN);
  }
  else
  {
    Result = base::FormatBytes(Bytes);
    const UnicodeString SizeUnorderedStr = base::FormatBytes(Bytes, false);
    if (Result != SizeUnorderedStr)
    {
      Result = FORMAT("%s (%s)", Result, SizeUnorderedStr);
    }
  }
  return Result;
}

void TFileSystemInfoDialog::Feed(TFeedFileSystemDataEvent && AddItem)
{
  AddItem(ServerLabels, NB_SERVER_REMOTE_SYSTEM, FFileSystemInfo.RemoteSystem);
  AddItem(ServerLabels, NB_SERVER_SESSION_PROTOCOL, FSessionInfo.ProtocolName);
  AddItem(ServerLabels, NB_SERVER_SSH_IMPLEMENTATION, FSessionInfo.SshImplementation);

  UnicodeString Str = FSessionInfo.CSCipher;
  if (FSessionInfo.CSCipher != FSessionInfo.SCCipher)
  {
    Str += FORMAT("/%s", FSessionInfo.SCCipher);
  }
  AddItem(ServerLabels, NB_SERVER_CIPHER, Str);

  Str = DefaultStr(FSessionInfo.CSCompression, LoadStr(NO_STR));
  if (FSessionInfo.CSCompression != FSessionInfo.SCCompression)
  {
    Str += FORMAT("/%s", DefaultStr(FSessionInfo.SCCompression, LoadStr(NO_STR)));
  }
  AddItem(ServerLabels, NB_SERVER_COMPRESSION, Str);
  if (FSessionInfo.ProtocolName != FFileSystemInfo.ProtocolName)
  {
    AddItem(ServerLabels, NB_SERVER_FS_PROTOCOL, FFileSystemInfo.ProtocolName);
  }

  AddItem(HostKeyFingerprintEdit, 0, FSessionInfo.HostKeyFingerprintSHA256);

  AddItem(ProtocolLabels, NB_PROTOCOL_MODE_CHANGING, CapabilityStr(fcModeChanging));
  AddItem(ProtocolLabels, NB_PROTOCOL_OWNER_GROUP_CHANGING, CapabilityStr(fcGroupChanging));
  UnicodeString AnyCommand;
  if (!FFileSystemInfo.IsCapable[fcShellAnyCommand] &&
    FFileSystemInfo.IsCapable[fcAnyCommand])
  {
    AnyCommand = GetMsg(NB_PROTOCOL_PROTOCOL_ANY_COMMAND);
  }
  else
  {
    AnyCommand = CapabilityStr(fcAnyCommand);
  }
  AddItem(ProtocolLabels, NB_PROTOCOL_ANY_COMMAND, AnyCommand);
  AddItem(ProtocolLabels, NB_PROTOCOL_SYMBOLIC_HARD_LINK, CapabilityStr(fcSymbolicLink, fcHardLink));
  AddItem(ProtocolLabels, NB_PROTOCOL_USER_GROUP_LISTING, CapabilityStr(fcUserGroupListing));
  AddItem(ProtocolLabels, NB_PROTOCOL_REMOTE_COPY, CapabilityStr(fcRemoteCopy));
  AddItem(ProtocolLabels, NB_PROTOCOL_CHECKING_SPACE_AVAILABLE, CapabilityStr(fcCheckingSpaceAvailable));
  AddItem(ProtocolLabels, NB_PROTOCOL_CALCULATING_CHECKSUM, CapabilityStr(fcCalculatingChecksum));
  AddItem(ProtocolLabels, NB_PROTOCOL_NATIVE_TEXT_MODE, CapabilityStr(fcNativeTextMode));

  AddItem(InfoLister, 0, FFileSystemInfo.AdditionalInfo);

  AddItem(SpaceAvailableLabels, NB_SPACE_AVAILABLE_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.BytesOnDevice));
  AddItem(SpaceAvailableLabels, NB_SPACE_AVAILABLE_UNUSED_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.UnusedBytesOnDevice));
  AddItem(SpaceAvailableLabels, NB_SPACE_AVAILABLE_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.BytesAvailableToUser));
  AddItem(SpaceAvailableLabels, NB_SPACE_AVAILABLE_UNUSED_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.UnusedBytesAvailableToUser));
  AddItem(SpaceAvailableLabels, NB_SPACE_AVAILABLE_BYTES_PER_ALLOCATION_UNIT, SpaceStr(FSpaceAvailable.BytesPerAllocationUnit));
}

void TFileSystemInfoDialog::ControlsAddItem(TObject * AControl,
  int32_t Label, const UnicodeString & Value)
{
  if (FLastFeededControl != AControl)
  {
    FLastFeededControl = AControl;
    FLastListItem = 0;
  }

  if (AControl == HostKeyFingerprintEdit)
  {
    HostKeyFingerprintEdit->SetText(Value);
    HostKeyFingerprintEdit->SetEnabled(!Value.IsEmpty());
    if (!HostKeyFingerprintEdit->GetEnabled())
    {
      HostKeyFingerprintEdit->SetVisible(false);
      HostKeyFingerprintEdit->SetGroup(0);
      HostKeyFingerprintLabel->SetVisible(false);
      HostKeyFingerprintLabel->SetGroup(0);
    }
  }
  else if (AControl == InfoLister)
  {
    InfoLister->GetItems()->SetText(Value);
    InfoLister->SetEnabled(!Value.IsEmpty());
    if (!InfoLister->GetEnabled())
    {
      InfoLister->SetVisible(false);
      InfoLister->SetGroup(0);
      InfoSeparator->SetVisible(false);
      InfoSeparator->SetGroup(0);
    }
  }
  else
  {
    const TLabelList * List = rtti::dyn_cast_or_null<TLabelList>(AControl);
    DebugAssert(List != nullptr);
    if (!Value.IsEmpty() && List)
    {
      TFarText * Text = List->GetAs<TFarText>(FLastListItem);
      FLastListItem++;

      Text->SetCaption(FORMAT("%d-%s  %s", List->MaxLen, GetMsg(Label), Value));
    }
  }
}

void TFileSystemInfoDialog::CalculateMaxLenAddItem(TObject * AControl,
  int32_t Label, const UnicodeString &) const
{
  TLabelList * List = rtti::dyn_cast_or_null<TLabelList>(AControl);
  if (List != nullptr)
  {
    const UnicodeString S = GetMsg(Label);
    if (List->MaxLen < S.Length())
    {
      List->MaxLen = S.Length();
    }
  }
}

void TFileSystemInfoDialog::ClipboardAddItem(TObject * AControl,
  int32_t Label, const UnicodeString & Value)
{
  const TFarDialogItem * Control = rtti::dyn_cast_or_null<TFarDialogItem>(AControl);
  // check for Enabled instead of Visible, as Visible is false
  // when control is on non-active tab
  if ((!Value.IsEmpty() &&
      ((Control == nullptr) || Control->GetEnabled()) &&
      (AControl != SpaceAvailableLabels)) ||
    SpaceAvailableSupported())
  {
    if (FLastFeededControl != AControl)
    {
      if (FLastFeededControl != nullptr)
      {
        FClipboard += ::StringOfChar('-', 60) + L"\r\n";
      }
      FLastFeededControl = AControl;
    }

    if (rtti::isa<TLabelList>(AControl))
    {
      UnicodeString LabelStr;
      if (Control == HostKeyFingerprintEdit)
      {
        LabelStr = GetMsg(NB_SERVER_HOST_KEY);
      }
      else if (Control == InfoLister)
      {
        LabelStr = ::Trim(GetMsg(NB_PROTOCOL_INFO_GROUP));
      }
      else
      {
        DebugAssert(false);
      }

      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == L':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }

      UnicodeString Value2 = Value;
      if ((Value2.Length() >= 2) && (Value2.SubString(Value2.Length() - 1, 2) == L"\r\n"))
      {
        Value2.SetLength(Value2.Length() - 2);
      }

      FClipboard += FORMAT("%s\r\n%s\r\n", LabelStr, Value2);
    }
    else
    {
      DebugAssert(rtti::isa<TLabelList>(AControl));
      UnicodeString LabelStr = GetMsg(Label);
      if (!LabelStr.IsEmpty() && (LabelStr[LabelStr.Length()] == L':'))
      {
        LabelStr.SetLength(LabelStr.Length() - 1);
      }
      FClipboard += FORMAT("%s = %s\r\n", LabelStr, Value);
    }
  }
}

void TFileSystemInfoDialog::FeedControls()
{
  FLastFeededControl = nullptr;
  Feed(nb::bind(&TFileSystemInfoDialog::ControlsAddItem, this));
  InfoLister->SetRight(GetBorderBox()->GetRight() - (InfoLister->GetScrollBar() ? 0 : 1));
}

void TFileSystemInfoDialog::SelectTab(int32_t Tab)
{
  TTabbedDialog::SelectTab(Tab);
  if (InfoLister->GetVisible())
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

void TFileSystemInfoDialog::Execute(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath)
{
  FFileSystemInfo = FileSystemInfo;
  FSessionInfo = SessionInfo;
  SpaceAvailablePathEdit->SetText(SpaceAvailablePath);
  UpdateControls();

  Feed(nb::bind(&TFileSystemInfoDialog::CalculateMaxLenAddItem, this));
  FeedControls();
  HideTabs();
  SelectTab(tabProtocol);

  ShowModal();
}

bool TFileSystemInfoDialog::Key(TFarDialogItem * Item, intptr_t KeyCode)
{
  bool Result;
  const WORD Key = KeyCode & 0xFFFF;
  // WORD ControlState = KeyCode >> 16;
  if ((Item == SpaceAvailablePathEdit) && (Key == VK_RETURN))
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

void TFileSystemInfoDialog::Change()
{
  TTabbedDialog::Change();

  if (GetHandle())
  {
    UpdateControls();
  }
}

void TFileSystemInfoDialog::UpdateControls()
{
  SpaceAvailableTab->SetEnabled(SpaceAvailableSupported());
}

void TFileSystemInfoDialog::ClipboardButtonClick(TFarButton * /*Sender*/,
  bool & Close)
{
  NeedSpaceAvailable();
  FLastFeededControl = nullptr;
  FClipboard.Clear();
  Feed(nb::bind(&TFileSystemInfoDialog::ClipboardAddItem, this));
  FarPlugin->FarCopyToClipboard(FClipboard);
  Close = false;
}

void TFileSystemInfoDialog::SpaceAvailableButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CheckSpaceAvailable();
  Close = false;
}

void TFileSystemInfoDialog::CheckSpaceAvailable()
{
  DebugAssert(FOnGetSpaceAvailable);
  DebugAssert(!SpaceAvailablePathEdit->GetText().IsEmpty());

  FSpaceAvailableLoaded = true;

  bool DoClose = false;

  FOnGetSpaceAvailable(SpaceAvailablePathEdit->GetText(), FSpaceAvailable, DoClose);

  FeedControls();
  if (DoClose)
  {
    Close(OkButton);
  }
}

void TFileSystemInfoDialog::NeedSpaceAvailable()
{
  if (!FSpaceAvailableLoaded && SpaceAvailableSupported())
  {
    CheckSpaceAvailable();
  }
}

bool TFileSystemInfoDialog::SpaceAvailableSupported() const
{
  return (FOnGetSpaceAvailable);
}

void TWinSCPFileSystem::FileSystemInfoDialog(
  const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent && OnGetSpaceAvailable)
{
  std::unique_ptr<TFileSystemInfoDialog> Dialog(std::make_unique<TFileSystemInfoDialog>(FPlugin, std::forward<TGetSpaceAvailableEvent>(OnGetSpaceAvailable)));
  Dialog->Execute(SessionInfo, FileSystemInfo, SpaceAvailablePath);
}

bool TWinSCPFileSystem::OpenDirectoryDialog(
  bool Add, UnicodeString & Directory, TBookmarkList * BookmarkList)
{
  bool Result;
  bool Repeat;

  int32_t ItemFocused = -1;

  do
  {
    std::unique_ptr<TStrings> BookmarkPaths(std::make_unique<TStringList>());
    std::unique_ptr<TFarMenuItems> BookmarkItems(std::make_unique<TFarMenuItems>());
    std::unique_ptr<TList> Bookmarks(std::make_unique<TList>());
    int32_t BookmarksOffset{0};

    const int32_t MaxLength = FPlugin->MaxMenuItemLength();
    constexpr int32_t MaxHistory = 40;
    int32_t FirstHistory = 0;

    if (FPathHistory->GetCount() > MaxHistory)
    {
      FirstHistory = FPathHistory->GetCount() - MaxHistory + 1;
    }

    for (int32_t Index = FirstHistory; Index < FPathHistory->GetCount(); ++Index)
    {
      UnicodeString Path = FPathHistory->GetString(Index);
      BookmarkPaths->Add(Path);
      BookmarkItems->Add(base::MinimizeName(Path, MaxLength, true));
    }

    int32_t FirstItemFocused = -1;
    std::unique_ptr<TStringList> BookmarkDirectories(std::make_unique<TStringList>());
    BookmarkDirectories->SetSorted(true);
    for (int32_t Index = 0; Index < BookmarkList->GetCount(); ++Index)
    {
      TBookmark * Bookmark = BookmarkList->GetBookmarks(Index);
      UnicodeString RemoteDirectory = Bookmark->GetRemote();
      if (!RemoteDirectory.IsEmpty() && (BookmarkDirectories->IndexOf(RemoteDirectory) == nb::NPOS))
      {
        const int32_t Pos = BookmarkDirectories->Add(RemoteDirectory);
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

    if (BookmarkDirectories->GetCount() == 0)
    {
      FirstItemFocused = BookmarkItems->Add(L"");
      BookmarkPaths->Add(L"");
      BookmarksOffset = BookmarkItems->GetCount();
    }
    else
    {
      if (BookmarkItems->GetCount() > 0)
      {
        BookmarkItems->AddSeparator();
        BookmarkPaths->Add(L"");
      }

      BookmarksOffset = BookmarkItems->GetCount();

      if (FirstItemFocused >= 0)
      {
        FirstItemFocused += BookmarkItems->GetCount();
      }
      else
      {
        FirstItemFocused = BookmarkItems->GetCount();
      }

      for (int32_t II = 0; II < BookmarkDirectories->GetCount(); II++)
      {
        UnicodeString Path = BookmarkDirectories->GetString(II);
        BookmarkItems->Add(Path);
        BookmarkPaths->Add(base::MinimizeName(Path, MaxLength, true));
      }
    }

    if (ItemFocused < 0)
    {
      BookmarkItems->SetItemFocused(FirstItemFocused);
    }
    else if (ItemFocused < BookmarkItems->GetCount())
    {
      BookmarkItems->SetItemFocused(ItemFocused);
    }
    else
    {
      BookmarkItems->SetItemFocused(BookmarkItems->GetCount() - 1);
    }

    intptr_t BreakCode;

    Repeat = false;
    UnicodeString Caption = GetMsg(Add ? NB_OPEN_DIRECTORY_ADD_BOOMARK_ACTION :
        NB_OPEN_DIRECTORY_BROWSE_CAPTION);
    constexpr FarKey BreakKeys[] =
    {
      { VK_DELETE, 0 },
      { VK_F8, 0 },
      { VK_RETURN, CTRLMASK },
      { 'C', CTRLMASK},
      { VK_INSERT, CTRLMASK },
      { 0, 0 }
    };

    ItemFocused = nb::ToInt32(FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
        Caption, GetMsg(NB_OPEN_DIRECTORY_HELP), BookmarkItems.get(), BreakKeys, BreakCode));
    if (BreakCode >= 0)
    {
      DebugAssert(BreakCode >= 0 && BreakCode <= 4);
      if ((BreakCode == 0) || (BreakCode == 1))
      {
        DebugAssert(ItemFocused >= 0);
        if (ItemFocused >= BookmarksOffset)
        {
          TBookmark * Bookmark = Bookmarks->GetAs<TBookmark>(ItemFocused - BookmarksOffset);
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
        FarControl(FCTL_INSERTCMDLINE, 0, nb::ToPtr(ToWCharPtr(BookmarkPaths->GetString(ItemFocused))));
      }
      else if (BreakCode == 3 || BreakCode == 4)
      {
        FPlugin->FarCopyToClipboard(BookmarkPaths->GetString(ItemFocused));
        Repeat = true;
      }
    }
    else if (ItemFocused >= 0)
    {
      Directory = BookmarkPaths->GetString(ItemFocused);
      if (Directory.IsEmpty())
      {
        // empty trailing line in no-bookmark mode selected
        ItemFocused = -1;
      }
    }

    Result = (BreakCode < 0) && (ItemFocused >= 0);
  }
  while (Repeat);

  return Result;
}

class TApplyCommandDialog final : public TWinSCPDialog
{
public:
  explicit TApplyCommandDialog(TCustomFarPlugin * AFarPlugin);

  bool Execute(UnicodeString & Command, int32_t & Params);

protected:
  virtual void Change() override;

private:
  int32_t FParams{0};

  TFarEdit * CommandEdit{nullptr};
  TFarText * LocalHintText{nullptr};
  TFarRadioButton * RemoteCommandButton{nullptr};
  TFarRadioButton * LocalCommandButton{nullptr};
  TFarCheckBox * ApplyToDirectoriesCheck{nullptr};
  TFarCheckBox * RecursiveCheck{nullptr};
  TFarCheckBox * ShowResultsCheck{nullptr};
  TFarCheckBox * CopyResultsCheck{nullptr};

  UnicodeString FPrompt;
  TFarEdit * PasswordEdit{nullptr};
  TFarEdit * NormalEdit{nullptr};
  TFarCheckBox * HideTypingCheck{nullptr};
};

TApplyCommandDialog::TApplyCommandDialog(TCustomFarPlugin * AFarPlugin) :
  TWinSCPDialog(AFarPlugin),
  FParams(0),
  PasswordEdit(nullptr),
  NormalEdit(nullptr),
  HideTypingCheck(nullptr)
{
  SetSize(TPoint(76, 18));
  SetCaption(GetMsg(NB_APPLY_COMMAND_TITLE));

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_PROMPT));

  CommandEdit = new TFarEdit(this);
  CommandEdit->SetHistory(APPLY_COMMAND_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT1));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT2));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT3));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT4));
  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT5));
  LocalHintText = new TFarText(this);
  LocalHintText->SetCaption(GetMsg(NB_APPLY_COMMAND_HINT_LOCAL));

  new TFarSeparator(this);

  RemoteCommandButton = new TFarRadioButton(this);
  RemoteCommandButton->SetCaption(GetMsg(NB_APPLY_COMMAND_REMOTE_COMMAND));

  SetNextItemPosition(ipRight);

  LocalCommandButton = new TFarRadioButton(this);
  LocalCommandButton->SetCaption(GetMsg(NB_APPLY_COMMAND_LOCAL_COMMAND));

  LocalHintText->SetEnabledDependency(LocalCommandButton);

  SetNextItemPosition(ipNewLine);

  ApplyToDirectoriesCheck = new TFarCheckBox(this);
  ApplyToDirectoriesCheck->SetCaption(
    GetMsg(NB_APPLY_COMMAND_APPLY_TO_DIRECTORIES));

  SetNextItemPosition(ipRight);

  RecursiveCheck = new TFarCheckBox(this);
  RecursiveCheck->SetCaption(GetMsg(NB_APPLY_COMMAND_RECURSIVE));

  SetNextItemPosition(ipNewLine);

  ShowResultsCheck = new TFarCheckBox(this);
  ShowResultsCheck->SetCaption(GetMsg(NB_APPLY_COMMAND_SHOW_RESULTS));
  ShowResultsCheck->SetEnabledDependency(RemoteCommandButton);

  SetNextItemPosition(ipRight);

  CopyResultsCheck = new TFarCheckBox(this);
  CopyResultsCheck->SetCaption(GetMsg(NB_APPLY_COMMAND_COPY_RESULTS));
  CopyResultsCheck->SetEnabledDependency(RemoteCommandButton);

  AddStandardButtons();

  OkButton->SetEnabledDependency(CommandEdit);
}

void TApplyCommandDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    const bool RemoteCommand = RemoteCommandButton->GetChecked();
    bool AllowRecursive = true;
    bool AllowApplyToDirectories = true;
    try
    {
      TRemoteCustomCommand RemoteCustomCommand;
      TLocalCustomCommand LocalCustomCommand;
      TFileCustomCommand * FileCustomCommand =
        (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

      TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);
      const UnicodeString Cmd = InteractiveCustomCommand.Complete(CommandEdit->GetText(), false);
      const bool FileCommand = FileCustomCommand->IsFileCommand(Cmd);
      AllowRecursive = FileCommand && !FileCustomCommand->IsFileListCommand(Cmd);
      if (AllowRecursive && !RemoteCommand)
      {
        AllowRecursive = !LocalCustomCommand.HasLocalFileName(Cmd);
      }
      AllowApplyToDirectories = FileCommand;
    }
    catch (...)
    {
      DEBUG_PRINTF("TApplyCommandDialog::Change: error");
    }

    RecursiveCheck->SetEnabled(AllowRecursive);
    ApplyToDirectoriesCheck->SetEnabled(AllowApplyToDirectories);
  }
}

bool TApplyCommandDialog::Execute(UnicodeString & Command, int32_t & Params)
{
  CommandEdit->SetText(Command);
  FParams = Params;
  RemoteCommandButton->SetChecked(FLAGCLEAR(Params, ccLocal));
  LocalCommandButton->SetChecked(FLAGSET(Params, ccLocal));
  ApplyToDirectoriesCheck->SetChecked(FLAGSET(Params, ccApplyToDirectories));
  RecursiveCheck->SetChecked(FLAGSET(Params, ccRecursive));
  ShowResultsCheck->SetChecked(FLAGSET(Params, ccShowResults));
  CopyResultsCheck->SetChecked(FLAGSET(Params, ccCopyResults));

  const bool Result = (ShowModal() != brCancel);
  if (Result)
  {
    Command = CommandEdit->GetText();
    Params &= ~(ccLocal | ccApplyToDirectories | ccRecursive | ccShowResults | ccCopyResults);
    Params |=
      FLAGMASK(!RemoteCommandButton->GetChecked(), ccLocal) |
      FLAGMASK(ApplyToDirectoriesCheck->GetChecked(), ccApplyToDirectories) |
      FLAGMASK(RecursiveCheck->GetChecked() && RecursiveCheck->GetEnabled(), ccRecursive) |
      FLAGMASK(ShowResultsCheck->GetChecked() && ShowResultsCheck->GetEnabled(), ccShowResults) |
      FLAGMASK(CopyResultsCheck->GetChecked() && CopyResultsCheck->GetEnabled(), ccCopyResults);
  }
  return Result;
}

bool TWinSCPFileSystem::ApplyCommandDialog(UnicodeString & Command,
  int32_t & Params) const
{
  std::unique_ptr<TApplyCommandDialog> Dialog(std::make_unique<TApplyCommandDialog>(FPlugin));
  const bool Result = Dialog->Execute(Command, Params);
  return Result;
}

class TFullSynchronizeDialog final : public TWinSCPDialog
{
public:
  explicit TFullSynchronizeDialog(TCustomFarPlugin * AFarPlugin, int32_t Options,
    const TUsableCopyParamAttrs & CopyParamAttrs);

  bool Execute(TTerminal::TSynchronizeMode & Mode,
    int32_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
    TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode);

protected:
  virtual bool CloseQuery() override;
  virtual void Change() override;
  virtual intptr_t DialogProc(intptr_t Msg, intptr_t Param1, void * Param2) override;

  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void CopyParamListerClick(TFarDialogItem * Item, const MOUSE_EVENT_RECORD * Event);

  int32_t ActualCopyParamAttrs() const;
  void CustomCopyParam();
  void AdaptSize();

private:
  TFarEdit * LocalDirectoryEdit{nullptr};
  TFarEdit * RemoteDirectoryEdit{nullptr};
  TFarRadioButton * SynchronizeBothButton{nullptr};
  TFarRadioButton * SynchronizeRemoteButton{nullptr};
  TFarRadioButton * SynchronizeLocalButton{nullptr};
  TFarRadioButton * SynchronizeFilesButton{nullptr};
  TFarRadioButton * MirrorFilesButton{nullptr};
  TFarRadioButton * SynchronizeTimestampsButton{nullptr};
  TFarCheckBox * SynchronizeDeleteCheck{nullptr};
  TFarCheckBox * SynchronizeExistingOnlyCheck{nullptr};
  TFarCheckBox * SynchronizeSelectedOnlyCheck{nullptr};
  TFarCheckBox * SynchronizePreviewChangesCheck{nullptr};
  TFarCheckBox * SynchronizeByTimeCheck{nullptr};
  TFarCheckBox * SynchronizeBySizeCheck{nullptr};
  TFarCheckBox * SaveSettingsCheck{nullptr};
  TFarLister * CopyParamLister{nullptr};

  bool FSaveMode{false};
  int32_t FOptions{0};
  int32_t FFullHeight{0};
  TTerminal::TSynchronizeMode FOrigMode{TTerminal::TSynchronizeMode::smRemote};
  TUsableCopyParamAttrs FCopyParamAttrs;
  TCopyParamType FCopyParams;

  TTerminal::TSynchronizeMode GetMode() const;
};

TFullSynchronizeDialog::TFullSynchronizeDialog(
  TCustomFarPlugin * AFarPlugin, int32_t Options,
  const TUsableCopyParamAttrs & CopyParamAttrs) :
  TWinSCPDialog(AFarPlugin),
  FOptions(Options),
  FCopyParamAttrs(CopyParamAttrs)
{
  SetSize(TPoint(78, 25));
  SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_TITLE));

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_LOCAL_LABEL));

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_REMOTE_LABEL));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_DIRECTION_GROUP));

  SynchronizeBothButton = new TFarRadioButton(this);
  SynchronizeBothButton->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_BOTH));

  SetNextItemPosition(ipRight);

  SynchronizeRemoteButton = new TFarRadioButton(this);
  SynchronizeRemoteButton->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_REMOTE));

  SynchronizeLocalButton = new TFarRadioButton(this);
  SynchronizeLocalButton->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_LOCAL));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_MODE_GROUP));

  SynchronizeFilesButton = new TFarRadioButton(this);
  SynchronizeFilesButton->SetCaption(GetMsg(NB_SYNCHRONIZE_SYNCHRONIZE_FILES));

  SetNextItemPosition(ipRight);

  MirrorFilesButton = new TFarRadioButton(this);
  MirrorFilesButton->SetCaption(GetMsg(NB_SYNCHRONIZE_MIRROR_FILES));

  SynchronizeTimestampsButton = new TFarRadioButton(this);
  SynchronizeTimestampsButton->SetCaption(GetMsg(NB_SYNCHRONIZE_SYNCHRONIZE_TIMESTAMPS));
  SynchronizeTimestampsButton->SetEnabled(FLAGCLEAR(Options, fsoDisableTimestamp));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_GROUP));

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_DELETE));

  SetNextItemPosition(ipRight);

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_EXISTING_ONLY));
  SynchronizeExistingOnlyCheck->SetEnabledDependencyNegative(SynchronizeTimestampsButton);

  SetNextItemPosition(ipNewLine);

  SynchronizePreviewChangesCheck = new TFarCheckBox(this);
  SynchronizePreviewChangesCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_PREVIEW_CHANGES));

  SetNextItemPosition(ipRight);

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_SELECTED_ONLY));
  SynchronizeSelectedOnlyCheck->SetEnabled(FLAGSET(FOptions, fsoAllowSelectedOnly));

  SetNextItemPosition(ipNewLine);

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_FULL_SYNCHRONIZE_CRITERIONS_GROUP));

  SynchronizeByTimeCheck = new TFarCheckBox(this);
  SynchronizeByTimeCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_BY_TIME));

  SetNextItemPosition(ipRight);

  SynchronizeBySizeCheck = new TFarCheckBox(this);
  SynchronizeBySizeCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_BY_SIZE));
  SynchronizeBySizeCheck->SetEnabledDependencyNegative(SynchronizeBothButton);

  SetNextItemPosition(ipNewLine);

  new TFarSeparator(this);

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_REUSE_SETTINGS));

  Separator = new TFarSeparator(this);
  Separator->SetGroup(1);
  Separator->SetCaption(GetMsg(NB_COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(nb::bind(&TFullSynchronizeDialog::CopyParamListerClick, this));
  CopyParamLister->SetGroup(1);
  // Right edge is adjusted in Change

  // Align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->SetPosition(-4);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(nb::bind(&TFullSynchronizeDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  AddStandardButtons(0, true);

  FFullHeight = GetSize().y;
  AdaptSize();
}

void TFullSynchronizeDialog::AdaptSize()
{
  const bool ShowCopyParam = (FFullHeight <= GetMaxSize().y);
  if (ShowCopyParam != CopyParamLister->GetVisible())
  {
    ShowGroup(1, ShowCopyParam);
    SetHeight(FFullHeight - (ShowCopyParam ? 0 : CopyParamLister->GetHeight() + 1));
  }
}

TTerminal::TSynchronizeMode TFullSynchronizeDialog::GetMode() const
{
  TTerminal::TSynchronizeMode Mode;

  if (SynchronizeRemoteButton->GetChecked())
  {
    Mode = TTerminal::smRemote;
  }
  else if (SynchronizeLocalButton->GetChecked())
  {
    Mode = TTerminal::smLocal;
  }
  else
  {
    Mode = TTerminal::smBoth;
  }

  return Mode;
}

void TFullSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TFullSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, const MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TFullSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, ActualCopyParamAttrs()))
  {
    Change();
  }
}

void TFullSynchronizeDialog::Change()
{
  TWinSCPDialog::Change();

  if (GetHandle())
  {
    if (SynchronizeTimestampsButton->GetChecked())
    {
      SynchronizeExistingOnlyCheck->SetChecked(true);
      SynchronizeDeleteCheck->SetChecked(false);
      SynchronizeByTimeCheck->SetChecked(true);
    }
    if (SynchronizeBothButton->GetChecked())
    {
      SynchronizeBySizeCheck->SetChecked(false);
      if (MirrorFilesButton->GetChecked())
      {
        SynchronizeFilesButton->SetChecked(true);
      }
    }
    if (MirrorFilesButton->GetChecked())
    {
      SynchronizeByTimeCheck->SetChecked(true);
    }
    MirrorFilesButton->SetEnabled(!SynchronizeBothButton->GetChecked());
    SynchronizeDeleteCheck->SetEnabled(!SynchronizeBothButton->GetChecked() &&
      !SynchronizeTimestampsButton->GetChecked());
    SynchronizeByTimeCheck->SetEnabled(!SynchronizeBothButton->GetChecked() &&
      !SynchronizeTimestampsButton->GetChecked() && !MirrorFilesButton->GetChecked());
    SynchronizeBySizeCheck->SetCaption(SynchronizeTimestampsButton->GetChecked() ?
      GetMsg(NB_SYNCHRONIZE_SAME_SIZE) : GetMsg(NB_SYNCHRONIZE_BY_SIZE));

    if (!SynchronizeBySizeCheck->GetChecked() && !SynchronizeByTimeCheck->GetChecked())
    {
      // suppose that in FAR the checkbox cannot be unchecked unless focused
      if (SynchronizeByTimeCheck->Focused())
      {
        SynchronizeBySizeCheck->SetChecked(true);
      }
      else
      {
        SynchronizeByTimeCheck->SetChecked(true);
      }
    }

    const UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
    std::unique_ptr<TStrings> InfoStrLines(std::make_unique<TStringList>());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

int32_t TFullSynchronizeDialog::ActualCopyParamAttrs() const
{
  int32_t Result;
  if (SynchronizeTimestampsButton->GetChecked())
  {
    Result = cpaIncludeMaskOnly;
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
      DebugAssert(false);
      // [[fallthrough]]
    case TTerminal::smBoth:
      Result = FCopyParamAttrs.General;
      break;
    }
  }
  return Result | cpaNoPreserveTime;
}

bool TFullSynchronizeDialog::CloseQuery()
{
  bool CanClose = TWinSCPDialog::CloseQuery();

  if (CanClose && (GetResult() == brOK) &&
    SaveSettingsCheck->GetChecked() && (FOrigMode != GetMode()) && !FSaveMode)
  {
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(NB_SAVE_SYNCHRONIZE_MODE), nullptr,
        qtConfirmation, qaYes | qaNo | qaCancel, nullptr))
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

intptr_t TFullSynchronizeDialog::DialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

bool TFullSynchronizeDialog::Execute(TTerminal::TSynchronizeMode & Mode,
  int32_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode)
{
  LocalDirectoryEdit->SetText(LocalDirectory);
  RemoteDirectoryEdit->SetText(RemoteDirectory);
  SynchronizeRemoteButton->SetChecked((Mode == TTerminal::smRemote));
  SynchronizeLocalButton->SetChecked((Mode == TTerminal::smLocal));
  SynchronizeBothButton->SetChecked((Mode == TTerminal::smBoth));
  SynchronizeDeleteCheck->SetChecked(FLAGSET(Params, TTerminal::spDelete));
  SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params, TTerminal::spExistingOnly));
  SynchronizePreviewChangesCheck->SetChecked(FLAGSET(Params, TTerminal::spPreviewChanges));
  SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params, TTerminal::spSelectedOnly));
  if (FLAGSET(Params, TTerminal::spTimestamp) && FLAGCLEAR(FOptions, fsoDisableTimestamp))
  {
    SynchronizeTimestampsButton->SetChecked(true);
  }
  else if (FLAGSET(Params, TTerminal::spMirror))
  {
    MirrorFilesButton->SetChecked(true);
  }
  else
  {
    SynchronizeFilesButton->SetChecked(true);
  }
  SynchronizeByTimeCheck->SetChecked(FLAGCLEAR(Params, TTerminal::spNotByTime));
  SynchronizeBySizeCheck->SetChecked(FLAGSET(Params, TTerminal::spBySize));
  SaveSettingsCheck->SetChecked(SaveSettings);
  FSaveMode = SaveMode;
  FOrigMode = Mode;
  FCopyParams = *CopyParams;

  const bool Result = (ShowModal() == brOK);

  if (Result)
  {
    RemoteDirectory = RemoteDirectoryEdit->GetText();
    LocalDirectory = LocalDirectoryEdit->GetText();

    Mode = GetMode();

    Params &= ~(TTerminal::spDelete | TTerminal::spNoConfirmation |
        TTerminal::spExistingOnly | TTerminal::spPreviewChanges |
        TTerminal::spTimestamp | TTerminal::spNotByTime | TTerminal::spBySize |
        TTerminal::spSelectedOnly | TTerminal::spMirror);
    Params |=
      FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
      FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
      FLAGMASK(SynchronizePreviewChangesCheck->GetChecked(), TTerminal::spPreviewChanges) |
      FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), TTerminal::spSelectedOnly) |
      FLAGMASK(SynchronizeTimestampsButton->GetChecked() && FLAGCLEAR(FOptions, fsoDisableTimestamp),
        TTerminal::spTimestamp) |
      FLAGMASK(MirrorFilesButton->GetChecked(), TTerminal::spMirror) |
      FLAGMASK(!SynchronizeByTimeCheck->GetChecked(), TTerminal::spNotByTime) |
      FLAGMASK(SynchronizeBySizeCheck->GetChecked(), TTerminal::spBySize);

    SaveSettings = SaveSettingsCheck->GetChecked();
    SaveMode = FSaveMode;
    *CopyParams = FCopyParams;
  }

  return Result;
}

bool TWinSCPFileSystem::FullSynchronizeDialog(TTerminal::TSynchronizeMode & Mode,
  int32_t & Params, UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int32_t Options,
  const TUsableCopyParamAttrs & CopyParamAttrs) const
{
  std::unique_ptr<TFullSynchronizeDialog> Dialog(std::make_unique<TFullSynchronizeDialog>(
      FPlugin, Options, CopyParamAttrs));
  const bool Result = Dialog->Execute(Mode, Params, LocalDirectory, RemoteDirectory,
                                      CopyParams, SaveSettings, SaveMode);
  return Result;
}

class TSynchronizeChecklistDialog final : public TWinSCPDialog
{
public:
  explicit TSynchronizeChecklistDialog(
    TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode Mode, int32_t Params,
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory);

  bool Execute(TSynchronizeChecklist * Checklist);

protected:
  virtual intptr_t DialogProc(intptr_t Msg, intptr_t Param1, void * Param2) override;
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode) override;
  void CheckAllButtonClick(TFarButton * Sender, bool & Close);
  void VideoModeButtonClick(TFarButton * Sender, bool & Close);
  void ListBoxClick(TFarDialogItem * Item, const MOUSE_EVENT_RECORD * Event);

private:
  TFarText * Header{nullptr};
  TFarListBox * ListBox{nullptr};
  TFarButton * CheckAllButton{nullptr};
  TFarButton * UncheckAllButton{nullptr};
  TFarButton * VideoModeButton{nullptr};

  TSynchronizeChecklist * FChecklist{nullptr};
  UnicodeString FLocalDirectory;
  UnicodeString FRemoteDirectory;
  static constexpr int32_t FColumns = 8;
  int32_t FWidths[FColumns]{};
  UnicodeString FActions[TSynchronizeChecklist::ActionCount];
  int32_t FScroll{0};
  bool FCanScrollRight{false};
  int32_t FChecked{0};

  void AdaptSize();
  //int32_t ColumnWidth(int32_t Index);
  void LoadChecklist();
  void RefreshChecklist(bool Scroll);
  void UpdateControls();
  void CheckAll(bool Check);
  UnicodeString ItemLine(const TChecklistItem * ChecklistItem);
  void AddColumn(UnicodeString & List, const UnicodeString & Value, size_t Column,
    bool AHeader = false);
  UnicodeString FormatSize(int64_t Size, int32_t Column);
};

TSynchronizeChecklistDialog::TSynchronizeChecklistDialog(
  TCustomFarPlugin * AFarPlugin, TTerminal::TSynchronizeMode /*Mode*/, int32_t /*Params*/,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory) :
  TWinSCPDialog(AFarPlugin),
  FLocalDirectory(LocalDirectory),
  FRemoteDirectory(RemoteDirectory)
{
  SetCaption(GetMsg(NB_CHECKLIST_TITLE));

  Header = new TFarText(this);

  ListBox = new TFarListBox(this);
  ListBox->SetNoBox(true);
  // align list with bottom of the window
  ListBox->SetBottom(-5);
  ListBox->SetOnMouseClick(nb::bind(&TSynchronizeChecklistDialog::ListBoxClick, this));

  UnicodeString Actions = GetMsg(NB_CHECKLIST_ACTIONS);
  size_t Action = 0;
  while (!Actions.IsEmpty() && (Action < _countof(FActions)))
  {
    FActions[Action] = CutToChar(Actions, '|', false);
    Action++;
  }

  // align buttons with bottom of the window
  ButtonSeparator = new TFarSeparator(this);
  ButtonSeparator->SetTop(-4);
  ButtonSeparator->SetBottom(ButtonSeparator->GetTop());

  CheckAllButton = new TFarButton(this);
  CheckAllButton->SetCaption(GetMsg(NB_CHECKLIST_CHECK_ALL));
  CheckAllButton->SetCenterGroup(true);
  CheckAllButton->SetOnClick(nb::bind(&TSynchronizeChecklistDialog::CheckAllButtonClick, this));

  SetNextItemPosition(ipRight);

  UncheckAllButton = new TFarButton(this);
  UncheckAllButton->SetCaption(GetMsg(NB_CHECKLIST_UNCHECK_ALL));
  UncheckAllButton->SetCenterGroup(true);
  UncheckAllButton->SetOnClick(nb::bind(&TSynchronizeChecklistDialog::CheckAllButtonClick, this));

  VideoModeButton = new TFarButton(this);
  VideoModeButton->SetCenterGroup(true);
  VideoModeButton->SetOnClick(nb::bind(&TSynchronizeChecklistDialog::VideoModeButtonClick, this));

  AddStandardButtons(0, true);

  AdaptSize();
  UpdateControls();
  ListBox->SetFocus();
}

void TSynchronizeChecklistDialog::AddColumn(UnicodeString & List,
  const UnicodeString & Value, size_t Column, bool AHeader)
{
  constexpr wchar_t Separator = L'|'; // '\xB3';
  const int32_t Len = Value.Length();
  int32_t Width = nb::ToInt32(FWidths[Column]);
  const bool Right = (Column == 2) || (Column == 3) || (Column == 6) || (Column == 7);
  const bool LastCol = (Column == FColumns - 1);
  if (Len <= Width)
  {
    int32_t Added = 0;
    if (AHeader && (Len < Width))
    {
      Added += (Width - Len) / 2;
    }
    else if (Right && (Len < Width))
    {
      Added += Width - Len;
    }
    List += ::StringOfChar(L' ', Added) + Value;
    Added += Value.Length();
    if (Width > Added)
    {
      List += ::StringOfChar(' ', Width - Added);
    }
    if (!LastCol)
    {
      List += Separator;
    }
  }
  else
  {
    int32_t Scroll = FScroll;
    if ((Scroll > 0) && !AHeader)
    {
      if (List.IsEmpty())
      {
        List += L'{';
        Width--;
        Scroll++;
      }
      else
      {
        List[List.Length()] = L'{';
      }
    }
    if (Scroll > Len - Width)
    {
      Scroll = Len - Width;
    }
    else if (!AHeader && LastCol && (Scroll < Len - Width))
    {
      Width--;
    }
    List += Value.SubString(Scroll + 1, Width);
    if (!Header && (Len - Scroll > Width))
    {
      List += L'}';
      FCanScrollRight = true;
    }
    else if (!LastCol)
    {
      List += Separator;
    }
  }
}

void TSynchronizeChecklistDialog::AdaptSize()
{
  FScroll = 0;
  SetSize(GetMaxSize());

  VideoModeButton->SetCaption(GetMsg(
      FarPlugin->ConsoleWindowState() == SW_SHOWMAXIMIZED ?
      NB_CHECKLIST_RESTORE : NB_CHECKLIST_MAXIMIZE));

  static constexpr int32_t Ratio[FColumns] = {140, 100, 80, 150, -2, 100, 80, 150};

  const int32_t Width = ListBox->GetWidth() - 2 /*checkbox*/ - 1 /*scrollbar*/ - FColumns;
  double Temp[FColumns];

  int32_t TotalRatio = 0;
  int32_t FixedRatio = 0;
  for (int32_t Index = 0; Index < FColumns; ++Index)
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

  int32_t TotalAssigned = 0;
  for (int32_t Index = 0; Index < FColumns; ++Index)
  {
    if (Ratio[Index] >= 0)
    {
      const double W = nb::ToDouble(Ratio[Index]) * (Width - FixedRatio) / TotalRatio;
      FWidths[Index] = nb::ToInt32(floor(W));
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
    size_t GrowIndex = 0;
    double MaxMissing = 0.0;
    for (int32_t Index = 0; Index < FColumns; ++Index)
    {
      if (MaxMissing < Temp[Index])
      {
        MaxMissing = Temp[Index];
        GrowIndex = Index;
      }
    }

    DebugAssert(MaxMissing > 0.0);

    FWidths[GrowIndex]++;
    Temp[GrowIndex] = 0.0;
    TotalAssigned++;
  }

  RefreshChecklist(false);
}

UnicodeString TSynchronizeChecklistDialog::FormatSize(
  int64_t Size, int32_t Column)
{
  const int32_t Width = nb::ToInt32(FWidths[Column]);
  UnicodeString Result = FORMAT("%lu", Size);

  if (Result.Length() > Width)
  {
    Result = FORMAT("%.2f 'K'", Size / 1024.0);
    if (Result.Length() > Width)
    {
      Result = FORMAT("%.2f 'M'", Size / (1024.0 * 1024));
      if (Result.Length() > Width)
      {
        Result = FORMAT("%.2f 'G'", Size / (1024.0 * 1024 * 1024));
        if (Result.Length() > Width)
        {
          // back to default
          Result = FORMAT("%lu", Size);
        }
      }
    }
  }

  return Result;
}

UnicodeString TSynchronizeChecklistDialog::ItemLine(const TChecklistItem * ChecklistItem)
{
  UnicodeString Line;

  UnicodeString S = ChecklistItem->GetFileName();
  if (ChecklistItem->IsDirectory)
  {
    S = ::IncludeTrailingBackslash(S);
  }
  AddColumn(Line, S, 0);

  if (ChecklistItem->Action == saDeleteRemote)
  {
    AddColumn(Line, L"", 1);
    AddColumn(Line, L"", 2);
    AddColumn(Line, L"", 3);
  }
  else
  {
    S = ChecklistItem->Local.Directory;
    if (::AnsiSameText(FLocalDirectory, S.SubString(1, FLocalDirectory.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FLocalDirectory.Length() - 1);
    }
    else
    {
      DebugAssert(false);
    }
    AddColumn(Line, S, 1);
    if (ChecklistItem->Action == saDownloadNew)
    {
      AddColumn(Line, L"", 2);
      AddColumn(Line, L"", 3);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, L"", 2);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Local.Size, 2), 2);
      }
      AddColumn(Line, base::UserModificationStr(ChecklistItem->Local.Modification,
          ChecklistItem->Local.ModificationFmt), 3);
    }
  }

  const int32_t Action = nb::ToInt32(ChecklistItem->Action - 1);
  DebugAssert((Action != nb::NPOS) && (Action < nb::ToInt32(_countof(FActions))));
  if ((Action != nb::NPOS) && (Action < nb::ToInt32(_countof(FActions))))
    AddColumn(Line, FActions[Action], 4);

  if (ChecklistItem->Action == saDeleteLocal)
  {
    AddColumn(Line, L"", 5);
    AddColumn(Line, L"", 6);
    AddColumn(Line, L"", 7);
  }
  else
  {
    S = ChecklistItem->Remote.Directory;
    if (::AnsiSameText(FRemoteDirectory, S.SubString(1, FRemoteDirectory.Length())))
    {
      S[1] = L'.';
      S.Delete(2, FRemoteDirectory.Length() - 1);
    }
    else
    {
      DebugAssert(false);
    }
    AddColumn(Line, S, 5);
    if (ChecklistItem->Action == saUploadNew)
    {
      AddColumn(Line, L"", 6);
      AddColumn(Line, L"", 7);
    }
    else
    {
      if (ChecklistItem->IsDirectory)
      {
        AddColumn(Line, L"", 6);
      }
      else
      {
        AddColumn(Line, FormatSize(ChecklistItem->Remote.Size, 6), 6);
      }
      AddColumn(Line, base::UserModificationStr(ChecklistItem->Remote.Modification,
          ChecklistItem->Remote.ModificationFmt), 7);
    }
  }

  return Line;
}

void TSynchronizeChecklistDialog::LoadChecklist()
{
  FChecked = 0;
  std::unique_ptr<TFarList> List(std::make_unique<TFarList>());
  List->BeginUpdate();
  for (int32_t Index = 0; Index < FChecklist->GetCount(); ++Index)
  {
    const TChecklistItem * ChecklistItem = FChecklist->GetItem(Index);

    List->AddObject(ItemLine(ChecklistItem),
      ToObj(nb::ToPtr(ChecklistItem)));
  }
  List->EndUpdate();

  // items must be checked in second pass once the internal array is allocated
  for (int32_t Index = 0; Index < FChecklist->GetCount(); ++Index)
  {
    const TChecklistItem * ChecklistItem = FChecklist->GetItem(Index);

    List->SetChecked(Index, ChecklistItem->Checked);
    if (ChecklistItem->Checked)
    {
      FChecked++;
    }
  }

  ListBox->SetItems(List.get());

  UpdateControls();
}

void TSynchronizeChecklistDialog::RefreshChecklist(bool Scroll)
{
  UnicodeString HeaderStr = GetMsg(NB_CHECKLIST_HEADER);
  UnicodeString HeaderCaption(::StringOfChar(' ', 2));

  for (int32_t Index = 0; Index < FColumns; ++Index)
  {
    AddColumn(HeaderCaption, CutToChar(HeaderStr, '|', false), Index, true);
  }
  Header->SetCaption(HeaderCaption);

  FCanScrollRight = false;
  TFarList * List = ListBox->GetItems();
  List->BeginUpdate();
  {
    try__finally
    {
      for (int32_t Index = 0; Index < List->GetCount(); ++Index)
      {
        if (!Scroll || (List->GetString(Index).LastDelimiter(L"{}") > 0))
        {
          const TChecklistItem * ChecklistItem = List->GetAs<TChecklistItem>(Index);

          List->SetString(Index, ItemLine(ChecklistItem));
        }
      }
    }
    __finally
    {
      List->EndUpdate();
    } end_try__finally
  }
}

void TSynchronizeChecklistDialog::UpdateControls()
{
  ButtonSeparator->SetCaption(
    FORMAT(GetMsg(NB_CHECKLIST_CHECKED), FChecked, ListBox->GetItems()->GetCount()));
  CheckAllButton->SetEnabled(FChecked < ListBox->GetItems()->GetCount());
  UncheckAllButton->SetEnabled(FChecked > 0);
}

intptr_t TSynchronizeChecklistDialog::DialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
{
  if (Msg == DN_RESIZECONSOLE)
  {
    AdaptSize();
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

void TSynchronizeChecklistDialog::CheckAll(bool Check)
{
  TFarList * List = ListBox->GetItems();
  List->BeginUpdate();
  {
    try__finally
    {
      const int32_t Count = List->GetCount();
      for (int32_t Index = 0; Index < Count; ++Index)
      {
        List->SetChecked(Index, Check);
      }

      FChecked = Check ? Count : 0;
    }
    __finally
    {
      List->EndUpdate();
    } end_try__finally
  }

  UpdateControls();
}

void TSynchronizeChecklistDialog::CheckAllButtonClick(
  TFarButton * Sender, bool & Close)
{
  CheckAll(Sender == CheckAllButton);
  ListBox->SetFocus();

  Close = false;
}

void TSynchronizeChecklistDialog::VideoModeButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  FarPlugin->ToggleVideoMode();

  Close = false;
}

void TSynchronizeChecklistDialog::ListBoxClick(
  TFarDialogItem * /*Item*/, const MOUSE_EVENT_RECORD * /*Event*/)
{
  const int32_t Index = ListBox->GetItems()->GetSelected();
  if (Index >= 0)
  {
    if (ListBox->GetItems()->GetChecked(Index))
    {
      ListBox->GetItems()->SetChecked(Index, false);
      FChecked--;
    }
    else if (!ListBox->GetItems()->GetChecked(Index))
    {
      ListBox->GetItems()->SetChecked(Index, true);
      FChecked++;
    }

    UpdateControls();
  }
}

bool TSynchronizeChecklistDialog::Key(TFarDialogItem * Item, intptr_t KeyCode)
{
  bool Result = false;
  const WORD Key = KeyCode & 0xFFFF;
  const WORD ControlState = nb::ToWord(KeyCode >> 16);
  if (ListBox->Focused())
  {
    if (((Key == VK_ADD) && (ControlState & SHIFTMASK) != 0) ||
      ((Key == VK_SUBTRACT) && (ControlState & SHIFTMASK) != 0))
    {
      CheckAll((Key == VK_ADD) && (ControlState & SHIFTMASK) != 0);
      Result = true;
    }
    else if ((Key == VK_SPACE) || (Key == VK_INSERT) ||
      (Key == VK_ADD) || (Key == VK_SUBTRACT))
    {
      const int32_t Index = ListBox->GetItems()->GetSelected();
      if (Index >= 0)
      {
        if (ListBox->GetItems()->GetChecked(Index) && (Key != VK_ADD))
        {
          ListBox->GetItems()->SetChecked(Index, false);
          FChecked--;
        }
        else if (!ListBox->GetItems()->GetChecked(Index) && (Key != VK_SUBTRACT))
        {
          ListBox->GetItems()->SetChecked(Index, true);
          FChecked++;
        }

        // FAR WORKAROUND
        // Changing "checked" state is not always drawn.
        Redraw();
        UpdateControls();
        if ((Key == VK_INSERT) &&
          (Index < ListBox->GetItems()->GetCount() - 1))
        {
          ListBox->GetItems()->SetSelected(Index + 1);
        }
        else
        {
          ListBox->GetItems()->SetSelected(Index);
        }
      }
      Result = true;
    }
    else if ((Key == VK_LEFT) && (ControlState & ALTMASK) != 0)
    {
      if (FScroll > 0)
      {
        FScroll--;
        RefreshChecklist(true);
      }
      Result = true;
    }
    else if (Key == VK_RIGHT)
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

bool TSynchronizeChecklistDialog::Execute(TSynchronizeChecklist * Checklist)
{
  FChecklist = Checklist;
  LoadChecklist();
  const bool Result = (ShowModal() == brOK);

  if (Result)
  {
    TFarList * List = ListBox->GetItems();
    const int32_t Count = List->GetCount();
    for (int32_t Index = 0; Index < Count; ++Index)
    {
      TChecklistItem * ChecklistItem = List->GetAs<TChecklistItem>(Index);
      ChecklistItem->Checked = List->GetChecked(Index);
    }
  }

  return Result;
}

bool TWinSCPFileSystem::SynchronizeChecklistDialog(
  TSynchronizeChecklist * Checklist, TTerminal::TSynchronizeMode Mode, int32_t Params,
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory)
{
  std::unique_ptr<TSynchronizeChecklistDialog> Dialog(std::make_unique<TSynchronizeChecklistDialog>(
      FPlugin, Mode, Params, LocalDirectory, RemoteDirectory));
  const bool Result = Dialog->Execute(Checklist);
  return Result;
}

class TSynchronizeDialog : TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
    TSynchronizeStartStopEvent && OnStartStop,
    uint32_t Options, uint32_t CopyParamAttrs, TGetSynchronizeOptionsEvent && OnGetOptions);
  virtual ~TSynchronizeDialog() noexcept override;

  bool Execute(TSynchronizeParamType & Params,
    const TCopyParamType * CopyParams, bool & SaveSettings);

protected:
  virtual void Change() override;
  void UpdateControls();
  void StartButtonClick(TFarButton * Sender, bool & Close);
  void StopButtonClick(TFarButton * Sender, bool & Close);
  void TransferSettingsButtonClick(TFarButton * Sender, bool & Close);
  void CopyParamListerClick(TFarDialogItem * Item, const MOUSE_EVENT_RECORD * Event);
  void Stop();
  void DoStartStop(bool Start, bool Synchronize);
  TSynchronizeParamType GetParams() const;
  void DoAbort(TObject * Sender, bool Close);
  void DoLog(TSynchronizeController * Controller,
    TSynchronizeLogEntry Entry, const UnicodeString & Message);
  void DoSynchronizeThreads(TObject * Sender, TThreadMethod Slot);
  virtual intptr_t DialogProc(intptr_t Msg, intptr_t Param1, void * Param2) override;
  virtual bool CloseQuery() override;
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode) override;
  TCopyParamType GetCopyParams() const;
  int32_t ActualCopyParamAttrs() const;
  void CustomCopyParam();

private:
  bool FSynchronizing{false};
  bool FStarted{false};
  bool FAbort{false};
  bool FClose{false};
  TSynchronizeParamType FParams;
  TSynchronizeStartStopEvent FOnStartStop;
  int32_t FOptions{0};
  gsl::owner<TSynchronizeOptions *> FSynchronizeOptions{nullptr};
  TCopyParamType FCopyParams;
  TGetSynchronizeOptionsEvent FOnGetOptions;
  int32_t FCopyParamAttrs{0};

  TFarEdit * LocalDirectoryEdit{nullptr};
  TFarEdit * RemoteDirectoryEdit{nullptr};
  TFarCheckBox * SynchronizeDeleteCheck{nullptr};
  TFarCheckBox * SynchronizeExistingOnlyCheck{nullptr};
  TFarCheckBox * SynchronizeSelectedOnlyCheck{nullptr};
  TFarCheckBox * SynchronizeRecursiveCheck{nullptr};
  TFarCheckBox * SynchronizeSynchronizeCheck{nullptr};
  TFarCheckBox * SaveSettingsCheck{nullptr};
  TFarButton * StartButton{nullptr};
  TFarButton * StopButton{nullptr};
  TFarButton * CloseButton{nullptr};
  TFarLister * CopyParamLister{nullptr};
};

TSynchronizeDialog::TSynchronizeDialog(TCustomFarPlugin * AFarPlugin,
  TSynchronizeStartStopEvent && OnStartStop,
  uint32_t Options, uint32_t CopyParamAttrs, TGetSynchronizeOptionsEvent && OnGetOptions) :
  TFarDialog(AFarPlugin)
{
  FSynchronizing = false;
  FStarted = false;
  FOnStartStop = OnStartStop;
  FAbort = false;
  FClose = false;
  FOptions = Options;
  FOnGetOptions = OnGetOptions;
  FSynchronizeOptions = nullptr;
  FCopyParamAttrs = CopyParamAttrs;

  SetSize(TPoint(76, 20));

  SetDefaultGroup(1);

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_SYNCHRONIZE_LOCAL_LABEL));

  LocalDirectoryEdit = new TFarEdit(this);
  LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

  Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_SYNCHRONIZE_REMOTE_LABEL));

  RemoteDirectoryEdit = new TFarEdit(this);
  RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

  TFarSeparator * Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_SYNCHRONIZE_GROUP));
  Separator->SetGroup(0);

  SynchronizeDeleteCheck = new TFarCheckBox(this);
  SynchronizeDeleteCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_DELETE));

  SetNextItemPosition(ipRight);

  SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
  SynchronizeExistingOnlyCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_EXISTING_ONLY));

  SetNextItemPosition(ipNewLine);

  SynchronizeRecursiveCheck = new TFarCheckBox(this);
  SynchronizeRecursiveCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_RECURSIVE));

  SetNextItemPosition(ipRight);

  SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
  SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_SELECTED_ONLY));
  // have more complex enable rules
  SynchronizeSelectedOnlyCheck->SetGroup(0);

  SetNextItemPosition(ipNewLine);

  SynchronizeSynchronizeCheck = new TFarCheckBox(this);
  SynchronizeSynchronizeCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_SYNCHRONIZE));
  SynchronizeSynchronizeCheck->SetAllowGrayed(true);

  Separator = new TFarSeparator(this);
  Separator->SetGroup(0);

  SaveSettingsCheck = new TFarCheckBox(this);
  SaveSettingsCheck->SetCaption(GetMsg(NB_SYNCHRONIZE_REUSE_SETTINGS));

  Separator = new TFarSeparator(this);
  Separator->SetCaption(GetMsg(NB_COPY_PARAM_GROUP));

  CopyParamLister = new TFarLister(this);
  CopyParamLister->SetHeight(3);
  CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
  CopyParamLister->SetTabStop(false);
  CopyParamLister->SetOnMouseClick(nb::bind(&TSynchronizeDialog::CopyParamListerClick, this));
  // Right edge is adjusted in Change

  SetDefaultGroup(0);

  // align buttons with bottom of the window
  Separator = new TFarSeparator(this);
  Separator->SetPosition(-4);

  TFarButton * Button = new TFarButton(this);
  Button->SetCaption(GetMsg(NB_TRANSFER_SETTINGS_BUTTON));
  Button->SetResult(-1);
  Button->SetCenterGroup(true);
  Button->SetOnClick(nb::bind(&TSynchronizeDialog::TransferSettingsButtonClick, this));

  SetNextItemPosition(ipRight);

  StartButton = new TFarButton(this);
  StartButton->SetCaption(GetMsg(NB_SYNCHRONIZE_START_BUTTON));
  StartButton->SetDefault(true);
  StartButton->SetCenterGroup(true);
  StartButton->SetOnClick(nb::bind(&TSynchronizeDialog::StartButtonClick, this));

  StopButton = new TFarButton(this);
  StopButton->SetCaption(GetMsg(NB_SYNCHRONIZE_STOP_BUTTON));
  StopButton->SetCenterGroup(true);
  StopButton->SetOnClick(nb::bind(&TSynchronizeDialog::StopButtonClick, this));

  SetNextItemPosition(ipRight);

  CloseButton = new TFarButton(this);
  CloseButton->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
  CloseButton->SetResult(brCancel);
  CloseButton->SetCenterGroup(true);
}

TSynchronizeDialog::~TSynchronizeDialog() noexcept
{
  SAFE_DESTROY(FSynchronizeOptions);
}

void TSynchronizeDialog::TransferSettingsButtonClick(
  TFarButton * /*Sender*/, bool & Close)
{
  CustomCopyParam();
  Close = false;
}

void TSynchronizeDialog::CopyParamListerClick(
  TFarDialogItem * /*Item*/, const MOUSE_EVENT_RECORD * Event)
{
  if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
  {
    CustomCopyParam();
  }
}

void TSynchronizeDialog::CustomCopyParam()
{
  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  Expects(WinSCPPlugin);
  // PreserveTime is forced for some settings, but avoid hard-setting it until
  // user really confirms it on custom dialog
  TCopyParamType ACopyParams = GetCopyParams();
  if (WinSCPPlugin->CopyParamCustomDialog(ACopyParams, ActualCopyParamAttrs()))
  {
    FCopyParams = ACopyParams;
    Change();
  }
}

bool TSynchronizeDialog::Execute(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, bool & SaveSettings)
{
  RemoteDirectoryEdit->SetText(Params.RemoteDirectory);
  LocalDirectoryEdit->SetText(Params.LocalDirectory);
  SynchronizeDeleteCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spDelete));
  SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spExistingOnly));
  SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spSelectedOnly));
  SynchronizeRecursiveCheck->SetChecked(FLAGSET(Params.Options, soRecurse));
  SynchronizeSynchronizeCheck->SetSelected(
    FLAGSET(Params.Options, soSynchronizeAsk) ? BSTATE_3STATE :
    (FLAGSET(Params.Options, soSynchronize) ? BSTATE_CHECKED : BSTATE_UNCHECKED));
  SaveSettingsCheck->SetChecked(SaveSettings);

  FParams = Params;
  FCopyParams = *CopyParams;

  ShowModal();

  Params = GetParams();
  SaveSettings = SaveSettingsCheck->GetChecked();

  return true;
}

TSynchronizeParamType TSynchronizeDialog::GetParams() const
{
  TSynchronizeParamType Result = FParams;
  Result.RemoteDirectory = RemoteDirectoryEdit->GetText();
  Result.LocalDirectory = LocalDirectoryEdit->GetText();
  Result.Params =
    (Result.Params & ~(TTerminal::spDelete | TTerminal::spExistingOnly |
        TTerminal::spSelectedOnly | TTerminal::spTimestamp)) |
    FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
    FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
    FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), TTerminal::spSelectedOnly);
  Result.Options =
    (Result.Options & ~(soRecurse | soSynchronize | soSynchronizeAsk)) |
    FLAGMASK(SynchronizeRecursiveCheck->GetChecked(), soRecurse) |
    FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_CHECKED, soSynchronize) |
    FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE, soSynchronizeAsk);
  return Result;
}

void TSynchronizeDialog::DoStartStop(bool Start, bool Synchronize)
{
  if (FOnStartStop)
  {
    TSynchronizeParamType SParams = GetParams();
    SParams.Options =
      (SParams.Options & ~(soSynchronize | soSynchronizeAsk)) |
      FLAGMASK(Synchronize, soSynchronize);
    if (Start)
    {
      SAFE_DESTROY(FSynchronizeOptions);
      FSynchronizeOptions = new TSynchronizeOptions;
      FOnGetOptions(SParams.Params, *FSynchronizeOptions);
    }
    FOnStartStop(this, Start, SParams, GetCopyParams(), FSynchronizeOptions,
      nb::bind(&TSynchronizeDialog::DoAbort, this),
      nb::bind(&TSynchronizeDialog::DoSynchronizeThreads, this),
      nb::bind(&TSynchronizeDialog::DoLog, this));
  }
}

void TSynchronizeDialog::DoSynchronizeThreads(TObject * /*Sender*/,
  TThreadMethod Slot)
{
  if (FStarted)
  {
    Synchronize(Slot);
  }
}

intptr_t TSynchronizeDialog::DialogProc(intptr_t Msg, intptr_t Param1, void * Param2)
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
      DebugAssert(CloseButton->GetEnabled());
      Close(CloseButton);
    }
  }

  return TFarDialog::DialogProc(Msg, Param1, Param2);
}

bool TSynchronizeDialog::CloseQuery()
{
  return TFarDialog::CloseQuery() && !FSynchronizing;
}

void TSynchronizeDialog::DoAbort(TObject * /*Sender*/, bool Close)
{
  FAbort = true;
  FClose = Close;
}

void TSynchronizeDialog::DoLog(TSynchronizeController * /*Controller*/,
  TSynchronizeLogEntry /*Entry*/, const UnicodeString & /*Message*/)
{
  // void
}

void TSynchronizeDialog::StartButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  bool Synchronize = false;
  bool Continue = true;
  if (SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE)
  {
    TMessageParams Params(nullptr);
    Params.Params = qpNeverAskAgainCheck;
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    switch (WinSCPPlugin->MoreMessageDialog(GetMsg(NB_SYNCHRONISE_BEFORE_KEEPUPTODATE),
        nullptr, qtConfirmation, qaYes | qaNo | qaCancel, &Params))
    {
    case qaNeverAskAgain:
      SynchronizeSynchronizeCheck->SetSelected(BSTATE_CHECKED);
      // [[fallthrough]]

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
    }
  }
  else
  {
    Synchronize = SynchronizeSynchronizeCheck->GetChecked();
  }

  if (Continue)
  {
    DebugAssert(!FSynchronizing);

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

void TSynchronizeDialog::StopButtonClick(TFarButton * /*Sender*/,
  bool & /*Close*/)
{
  Stop();
}

void TSynchronizeDialog::Stop()
{
  FSynchronizing = false;
  FStarted = false;
  BreakSynchronize();
  DoStartStop(false, false);
  UpdateControls();
  StartButton->SetFocus();
}

void TSynchronizeDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() && !ChangesLocked())
  {
    UpdateControls();

    const UnicodeString InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
    std::unique_ptr<TStrings> InfoStrLines(std::make_unique<TStringList>());
    FarWrapText(InfoStr, InfoStrLines.get(), GetBorderBox()->GetWidth() - 4);
    CopyParamLister->SetItems(InfoStrLines.get());
    CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
  }
}

bool TSynchronizeDialog::Key(TFarDialogItem * /*Item*/, intptr_t KeyCode)
{
  bool Result = false;
  const WORD Key = KeyCode & 0xFFFF;
  // const WORD ControlState = KeyCode >> 16;
  if ((Key == VK_ESCAPE) && FSynchronizing)
  {
    Stop();
    Result = true;
  }
  return Result;
}

void TSynchronizeDialog::UpdateControls()
{
  SetCaption(GetMsg(FSynchronizing ? NB_SYNCHRONIZE_SYCHRONIZING : NB_SYNCHRONIZE_TITLE));
  StartButton->SetEnabled(!FSynchronizing);
  StopButton->SetEnabled(FSynchronizing);
  CloseButton->SetEnabled(!FSynchronizing);
  EnableGroup(1, !FSynchronizing);
  SynchronizeSelectedOnlyCheck->SetEnabled(
    !FSynchronizing && FLAGSET(FOptions, soAllowSelectedOnly));
}

TCopyParamType TSynchronizeDialog::GetCopyParams() const
{
  TCopyParamType Result = FCopyParams;
  Result.SetPreserveTime(true);
  return Result;
}

int32_t TSynchronizeDialog::ActualCopyParamAttrs() const
{
  return FCopyParamAttrs | cpaNoPreserveTime;
}

bool TWinSCPFileSystem::SynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent && OnStartStop,
  bool & SaveSettings, uint32_t Options, uint32_t CopyParamAttrs, TGetSynchronizeOptionsEvent && OnGetOptions)
{
  std::unique_ptr<TSynchronizeDialog> Dialog(std::make_unique<TSynchronizeDialog>(FPlugin, std::forward<TSynchronizeStartStopEvent>(OnStartStop),
    Options, CopyParamAttrs, std::forward<TGetSynchronizeOptionsEvent>(OnGetOptions)));
  const bool Result = Dialog->Execute(Params, CopyParams, SaveSettings);
  return Result;
}

bool TWinSCPFileSystem::RemoteTransferDialog(TStrings * AFileList,
  UnicodeString & Target, UnicodeString & FileMask, bool Move)
{
  const UnicodeString Prompt = FileNameFormatString(
      GetMsg(Move ? NB_REMOTE_MOVE_FILE : NB_REMOTE_COPY_FILE),
      GetMsg(Move ? NB_REMOTE_MOVE_FILES : NB_REMOTE_COPY_FILES), AFileList, true);

  UnicodeString Value = base::UnixIncludeTrailingBackslash(Target) + FileMask;
  const bool Result = FPlugin->InputBox(
      GetMsg(Move ? NB_REMOTE_MOVE_TITLE : NB_REMOTE_COPY_TITLE), Prompt,
      Value, 0, MOVE_TO_HISTORY) && !Value.IsEmpty();
  if (Result)
  {
    Target = base::UnixExtractFilePath(Value);
    FileMask = base::UnixExtractFileName(Value);
  }
  return Result;
}

bool TWinSCPFileSystem::RenameFileDialog(TRemoteFile * AFile,
  UnicodeString & NewName)
{
  return FPlugin->InputBox(GetMsg(NB_RENAME_FILE_TITLE),
      FORMAT(GetMsg(NB_RENAME_FILE), AFile->GetFileName()), NewName, 0) &&
    !NewName.IsEmpty();
}

class TQueueDialog final : public TFarDialog
{
  CUSTOM_MEM_ALLOCATION_IMPL
  NB_DISABLE_COPY(TQueueDialog)
public:
  explicit TQueueDialog(gsl::not_null<TCustomFarPlugin *> AFarPlugin,
    gsl::not_null<TWinSCPFileSystem *> AFileSystem, bool ClosingPlugin) noexcept;
  virtual ~TQueueDialog() override;

  bool Execute(TTerminalQueueStatus * Status);

protected:
  virtual const UUID * GetDialogGuid() const override { return &QueueDialogGuid; }
  virtual void Change() override;
  virtual void Idle() override;
  bool UpdateQueue();
  void LoadQueue();
  void RefreshQueue();
  bool FillQueueItemLine(UnicodeString & Line,
    TQueueItemProxy * QueueItem, int32_t Index);
  bool QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem);
  void UpdateControls();
  virtual bool Key(TFarDialogItem * Item, intptr_t KeyCode) override;
  virtual bool CloseQuery() override;

private:
  void OperationButtonClick(TFarButton * Sender, bool & Close);
  TFarList * GetQueueItems() const { return QueueListBox->GetItems(); }
  TFarList * GetQueueItems() { return QueueListBox->GetItems(); }
  void OnIdle(TObject * /*Sender*/, void * /*Data*/);

private:
  TTerminalQueueStatus * FStatus{nullptr};
  gsl::not_null<TWinSCPFileSystem *> FFileSystem;
  bool FClosingPlugin{false};

  TFarListBox * QueueListBox{nullptr};
  TFarButton * ShowButton{nullptr};
  TFarButton * ExecuteButton{nullptr};
  TFarButton * DeleteButton{nullptr};
  TFarButton * MoveUpButton{nullptr};
  TFarButton * MoveDownButton{nullptr};
  TFarButton * CloseButton{nullptr};
};

TQueueDialog::TQueueDialog(gsl::not_null<TCustomFarPlugin *> AFarPlugin,
  gsl::not_null<TWinSCPFileSystem *> AFileSystem, bool ClosingPlugin) noexcept :
  TFarDialog(AFarPlugin),
  FFileSystem(AFileSystem),
  FClosingPlugin(ClosingPlugin)
{
  TFarDialog::InitDialog();
  SetSize(TPoint(80, 23)); // TODO: check actual configuration
  // TRect CRect = GetClientRect();
  const int32_t ListHeight = GetClientSize().y - 4;

  SetCaption(GetMsg(NB_QUEUE_TITLE));

  TFarText * Text = new TFarText(this);
  Text->SetCaption(GetMsg(NB_QUEUE_HEADER));

  TFarSeparator * Separator = new TFarSeparator(this);
  const int32_t ListTop = Separator->GetBottom();

  Separator = new TFarSeparator(this);
  Separator->Move(0, ListHeight);

  ExecuteButton = new TFarButton(this);
  ExecuteButton->SetCaption(GetMsg(NB_QUEUE_EXECUTE));
  ExecuteButton->SetOnClick(nb::bind(&TQueueDialog::OperationButtonClick, this));
  ExecuteButton->SetCenterGroup(true);

  SetNextItemPosition(ipRight);

  DeleteButton = new TFarButton(this);
  DeleteButton->SetCaption(GetMsg(NB_QUEUE_DELETE));
  DeleteButton->SetOnClick(nb::bind(&TQueueDialog::OperationButtonClick, this));
  DeleteButton->SetCenterGroup(true);

  MoveUpButton = new TFarButton(this);
  MoveUpButton->SetCaption(GetMsg(NB_QUEUE_MOVE_UP));
  MoveUpButton->SetOnClick(nb::bind(&TQueueDialog::OperationButtonClick, this));
  MoveUpButton->SetCenterGroup(true);

  MoveDownButton = new TFarButton(this);
  MoveDownButton->SetCaption(GetMsg(NB_QUEUE_MOVE_DOWN));
  MoveDownButton->SetOnClick(nb::bind(&TQueueDialog::OperationButtonClick, this));
  MoveDownButton->SetCenterGroup(true);

  CloseButton = new TFarButton(this);
  CloseButton->SetCaption(GetMsg(NB_QUEUE_CLOSE));
  CloseButton->SetResult(brCancel);
  CloseButton->SetCenterGroup(true);
  CloseButton->SetDefault(true);

  SetNextItemPosition(ipNewLine);

  QueueListBox = new TFarListBox(this);
  QueueListBox->SetTop(ListTop + 1);
  QueueListBox->SetHeight(ListHeight);
  QueueListBox->SetNoBox(true);
  QueueListBox->SetFocus();
}

TQueueDialog::~TQueueDialog()
{
  if (GetFarPlugin())
  {
    TSynchroParams & SynchroParams = GetFarPlugin()->FSynchroParams;
    SynchroParams.Sender = nullptr;
  }
}

void TQueueDialog::OperationButtonClick(TFarButton * Sender,
  bool & /*Close*/)
{
  if (GetQueueItems()->GetSelected() != nb::NPOS)
  {
    TQueueItemProxy * QueueItem = rtti::dyn_cast_or_null<TQueueItemProxy>(
        GetQueueItems()->Get(GetQueueItems()->GetSelected()));

    if (Sender == ExecuteButton)
    {
      if (QueueItem->GetStatus() == TQueueItem::qsProcessing)
      {
        QueueItem->Pause();
      }
      else if (QueueItem->GetStatus() == TQueueItem::qsPaused)
      {
        QueueItem->Resume();
      }
      else if (QueueItem->GetStatus() == TQueueItem::qsPending)
      {
        QueueItem->ExecuteNow();
      }
      else if (TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
      {
        QueueItem->ProcessUserAction();
      }
      else
      {
        DebugAssert(false);
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

void TQueueDialog::OnIdle(TObject *, void *)
{
  // DEBUG_PRINTF("FarThreadId: %d, GetCurrentThreadId: %d", FarPlugin->GetFarThreadId(), GetCurrentThreadId());
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

bool TQueueDialog::Key(TFarDialogItem * /*Item*/, intptr_t KeyCode)
{
  bool Result = false;
  const WORD Key = KeyCode & 0xFFFF;
  const WORD ControlState = nb::ToWord(KeyCode >> 16);
  if (QueueListBox->Focused())
  {
    TFarButton * DoButton = nullptr;
    if (Key == VK_RETURN)
    {
      if (ExecuteButton->GetEnabled())
      {
        DoButton = ExecuteButton;
      }
      Result = true;
    }
    else if (Key == VK_DELETE)
    {
      if (DeleteButton->GetEnabled())
      {
        DoButton = DeleteButton;
      }
      Result = true;
    }
    else if ((Key == VK_UP) && (ControlState & CTRLMASK) != 0)
    {
      if (MoveUpButton->GetEnabled())
      {
        DoButton = MoveUpButton;
      }
      Result = true;
    }
    else if ((Key == VK_DOWN) && (ControlState & CTRLMASK) != 0)
    {
      if (MoveDownButton->GetEnabled())
      {
        DoButton = MoveDownButton;
      }
      Result = true;
    }

    if (DoButton != nullptr)
    {
      bool Close;
      OperationButtonClick(DoButton, Close);
    }
  }
  return Result;
}

void TQueueDialog::UpdateControls()
{
  const TQueueItemProxy * QueueItem = nullptr;
  if (GetQueueItems()->GetSelected() >= 0)
  {
    QueueItem = rtti::dyn_cast_or_null<TQueueItemProxy>(
      GetQueueItems()->Get(GetQueueItems()->GetSelected()));
  }

  if ((QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsProcessing))
  {
    ExecuteButton->SetCaption(GetMsg(NB_QUEUE_PAUSE));
    ExecuteButton->SetEnabled(true);
  }
  else if ((QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsPaused))
  {
    ExecuteButton->SetCaption(GetMsg(NB_QUEUE_RESUME));
    ExecuteButton->SetEnabled(true);
  }
  else if ((QueueItem != nullptr) && TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
  {
    ExecuteButton->SetCaption(GetMsg(NB_QUEUE_SHOW));
    ExecuteButton->SetEnabled(true);
  }
  else
  {
    ExecuteButton->SetCaption(GetMsg(NB_QUEUE_EXECUTE));
    ExecuteButton->SetEnabled(
      (QueueItem != nullptr) && (QueueItem->GetStatus() == TQueueItem::qsPending));
  }
  DeleteButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() != TQueueItem::qsDone));
  MoveUpButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() == TQueueItem::qsPending) &&
    (QueueItem->GetIndex() > FStatus->GetActiveCount()));
  MoveDownButton->SetEnabled((QueueItem != nullptr) &&
    (QueueItem->GetStatus() == TQueueItem::qsPending) &&
    (QueueItem->GetIndex() < FStatus->GetCount() - 1));
}

void TQueueDialog::Idle()
{
  TFarDialog::Idle();

  if (GetFarPlugin())
  {
    TSynchroParams & SynchroParams = GetFarPlugin()->FSynchroParams;
    SynchroParams.SynchroEvent = nb::bind(&TQueueDialog::OnIdle, this);
    SynchroParams.Sender = this;
    GetFarPlugin()->FarAdvControl(ACTL_SYNCHRO, 0, &SynchroParams);
  }
}

bool TQueueDialog::CloseQuery()
{
  bool Result = TFarDialog::CloseQuery();
  if (Result)
  {
    TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
    Ensures(WinSCPPlugin);
    Result = !FClosingPlugin || (FStatus->GetCount() == 0) ||
      (WinSCPPlugin->MoreMessageDialog(GetMsg(NB_QUEUE_PENDING_ITEMS), nullptr,
          qtWarning, qaOK | qaCancel) == qaCancel);
  }
  return Result;
}

bool TQueueDialog::UpdateQueue()
{
  DebugAssert(FFileSystem != nullptr);
  TTerminalQueueStatus * Status = FFileSystem->ProcessQueue(false);
  const bool Result = (Status != nullptr);
  if (Result)
  {
    FStatus = Status;
  }
  return Result;
}

void TQueueDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle())
  {
    UpdateControls();
  }
}

void TQueueDialog::RefreshQueue()
{
  if (GetQueueItems()->GetCount() > 0)
  {
    bool Change = false;
    const int32_t TopIndex = GetQueueItems()->GetTopIndex();
    int32_t Index = TopIndex;

    int32_t ILine = 0;
    while ((Index > ILine) &&
      (GetQueueItems()->GetObj(Index) ==
        GetQueueItems()->GetObj(Index - ILine - 1)))
    {
      ILine++;
    }

    const TQueueItemProxy * PrevQueueItem = nullptr;
    UnicodeString Line;

    while ((Index < GetQueueItems()->GetCount()) &&
      (Index < TopIndex + QueueListBox->GetHeight()))
    {
      TQueueItemProxy * QueueItem = rtti::dyn_cast_or_null<TQueueItemProxy>(
        GetQueueItems()->Get(Index));
      DebugAssert(QueueItem != nullptr);
      if ((PrevQueueItem != nullptr) && (QueueItem != PrevQueueItem))
      {
        ILine = 0;
      }

      if (QueueItemNeedsFrequentRefresh(QueueItem) &&
        QueueItem && !QueueItem->GetProcessingUserAction())
      {
        FillQueueItemLine(Line, QueueItem, ILine);
        if (GetQueueItems()->GetString(Index) != Line)
        {
          Change = true;
          GetQueueItems()->SetString(Index, Line);
        }
      }

      PrevQueueItem = QueueItem;
      ++Index;
      ++ILine;
    }

    if (Change)
    {
      Redraw();
    }
  }
}

void TQueueDialog::LoadQueue()
{
  std::unique_ptr<TFarList> List(std::make_unique<TFarList>());
  UnicodeString Line;
  for (int32_t Index = 0; Index < FStatus->GetCount(); ++Index)
  {
    TQueueItemProxy * QueueItem = FStatus->GetItem(Index);
    int32_t ILine = 0;
    while (FillQueueItemLine(Line, QueueItem, ILine))
    {
      List->AddObject(Line, QueueItem->Clone());
      List->SetDisabled(List->GetCount() - 1, (ILine > 0));
      ILine++;
    }
  }
  QueueListBox->SetItems(List.get());
}

bool TQueueDialog::FillQueueItemLine(UnicodeString & Line,
  TQueueItemProxy * QueueItem, int32_t Index)
{
  constexpr int32_t PathMaxLen = 49;

  if ((Index > 2) ||
    ((Index == 2) && (QueueItem->GetStatus() == TQueueItem::qsPending)))
  {
    return false;
  }

  UnicodeString ProgressStr;

  switch (QueueItem->GetStatus())
  {
  case TQueueItem::qsPending:
    ProgressStr = GetMsg(NB_QUEUE_PENDING);
    break;

  case TQueueItem::qsConnecting:
    ProgressStr = GetMsg(NB_QUEUE_CONNECTING);
    break;

  case TQueueItem::qsQuery:
    ProgressStr = GetMsg(NB_QUEUE_QUERY);
    break;

  case TQueueItem::qsError:
    ProgressStr = GetMsg(NB_QUEUE_ERROR);
    break;

  case TQueueItem::qsPrompt:
    ProgressStr = GetMsg(NB_QUEUE_PROMPT);
    break;

  case TQueueItem::qsPaused:
    ProgressStr = GetMsg(NB_QUEUE_PAUSED);
    break;
  }

  const bool BlinkHide = QueueItemNeedsFrequentRefresh(QueueItem) &&
    !QueueItem->GetProcessingUserAction() &&
    ((::GetTickCount() % 2000) >= 1000);

  UnicodeString Operation;
  UnicodeString Direction;
  UnicodeString Values[2];
  const TFileOperationProgressType * ProgressData = QueueItem->GetProgressData();
  const TQueueItem::TInfo * Info = QueueItem->GetInfo();

  if (Index == 0)
  {
    if (!BlinkHide)
    {
      switch (Info->Operation)
      {
      case foCopy:
        Operation = GetMsg(NB_QUEUE_COPY);
        break;

      case foMove:
        Operation = GetMsg(NB_QUEUE_MOVE);
        break;
      }
      Direction = GetMsg((Info->Side == osLocal) ? NB_QUEUE_UPLOAD : NB_QUEUE_DOWNLOAD);
    }

    Values[0] = base::MinimizeName(Info->Source, PathMaxLen, Info->Side == osRemote);

    if ((ProgressData != nullptr) &&
      (ProgressData->GetOperation() == Info->Operation))
    {
      Values[1] = base::FormatBytes(ProgressData->GetTotalTransferred());
    }
  }
  else if (Index == 1)
  {
    Values[0] = base::MinimizeName(Info->Destination, PathMaxLen, Info->Side == osLocal);

    if (ProgressStr.IsEmpty())
    {
      if (ProgressData != nullptr)
      {
        if (ProgressData->GetOperation() == Info->Operation)
        {
          Values[1] = FORMAT("%d%%", ProgressData->OverallProgress());
        }
        else if (ProgressData->GetOperation() == foCalculateSize)
        {
          Values[1] = GetMsg(NB_QUEUE_CALCULATING_SIZE);
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
    if (ProgressData != nullptr)
    {
      Values[0] = base::MinimizeName(ProgressData->GetFileName(), PathMaxLen,
        (Info->Side == osRemote));
      if (ProgressData->GetOperation() == Info->Operation)
      {
        Values[1] = FORMAT("%d%%", ProgressData->TransferProgress());
      }
    }
    else
    {
      Values[0] = ProgressStr;
    }
  }

  Line = FORMAT("%1s %1s %-49s %s",
    Operation, Direction, Values[0], Values[1]);

  return true;
}

bool TQueueDialog::QueueItemNeedsFrequentRefresh(
  TQueueItemProxy * QueueItem)
{
  return (QueueItem &&
      (TQueueItem::IsUserActionStatus(QueueItem->GetStatus()) ||
        (QueueItem->GetStatus() == TQueueItem::qsPaused)));
}

bool TQueueDialog::Execute(TTerminalQueueStatus * Status)
{
  FStatus = Status;
  bool Result = false;
  try__finally
  {
    UpdateQueue();
    LoadQueue();

    Result = (ShowModal() != brCancel);
  }
  __finally
  {
    FStatus = nullptr;
  } end_try__finally
  return Result;
}

bool TWinSCPFileSystem::QueueDialog(
  TTerminalQueueStatus * Status, bool ClosingPlugin)
{
  std::unique_ptr<TQueueDialog> Dialog(std::make_unique<TQueueDialog>(FPlugin, this, ClosingPlugin));
  const bool Result = Dialog->Execute(Status);
  return Result;
}

bool TWinSCPFileSystem::CreateDirectoryDialog(UnicodeString & Directory,
  TRemoteProperties * Properties, bool & SaveSettings)
{
  std::unique_ptr<TWinSCPDialog> Dialog(std::make_unique<TWinSCPDialog>(FPlugin));

  Dialog->SetCaption(GetMsg(NB_CREATE_FOLDER_TITLE));
  Dialog->SetSize(TPoint(66, 15));

  TFarText * Text = new TFarText(Dialog.get());
  Text->SetCaption(GetMsg(NB_CREATE_FOLDER_PROMPT));

  TFarEdit * DirectoryEdit = new TFarEdit(Dialog.get());
  DirectoryEdit->SetHistory(L"NewFolder");

  TFarSeparator * Separator = new TFarSeparator(Dialog.get());
  Separator->SetCaption(GetMsg(NB_CREATE_FOLDER_ATTRIBUTES));

  TFarCheckBox * SetRightsCheck = new TFarCheckBox(Dialog.get());
  SetRightsCheck->SetCaption(GetMsg(NB_CREATE_FOLDER_SET_RIGHTS));

  TRightsContainer * RightsContainer = new TRightsContainer(Dialog.get(), false, true,
    true, SetRightsCheck);

  TFarCheckBox * SaveSettingsCheck = new TFarCheckBox(Dialog.get());
  SaveSettingsCheck->SetCaption(GetMsg(NB_CREATE_FOLDER_REUSE_SETTINGS));
  SaveSettingsCheck->Move(0, 6);

  Dialog->AddStandardButtons();

  DirectoryEdit->SetText(Directory);
  SaveSettingsCheck->SetChecked(SaveSettings);
  DebugAssert(Properties != nullptr);
  if (Properties)
  {
    SetRightsCheck->SetChecked(Properties->Valid.Contains(vpRights));
    // expect sensible value even if rights are not set valid
    RightsContainer->SetRights(Properties->Rights);
  }

  const bool Result = (Dialog->ShowModal() == brOK);

  if (Result)
  {
    Directory = DirectoryEdit->GetText();
    SaveSettings = SaveSettingsCheck->GetChecked();
    if (Properties)
    {
      if (SetRightsCheck->GetChecked())
      {
        Properties->Valid = Properties->Valid << vpRights;
        Properties->Rights = RightsContainer->GetRights();
      }
      else
      {
        Properties->Valid = Properties->Valid >> vpRights;
      }
    }
  }
  return Result;
}

