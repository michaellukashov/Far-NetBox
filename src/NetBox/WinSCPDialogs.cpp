//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/signals/signal1.hpp>
#include <boost/signals/signal2.hpp>
#include <boost/signals/signal3.hpp>
#include <boost/bind.hpp>

#include "WinSCPPlugin.h"
#include "WinSCPFileSystem.h"
#include "FarTexts.h"
#include "FarDialog.h"
#include "FarConfiguration.h"

#include <ShellAPI.h>
#include <PuttyTools.h>
#include <GUITools.h>
#include <CoreMain.h>
#include <Common.h>
#include <CopyParam.h>
#include <TextsCore.h>
#include <Terminal.h>
#include <Bookmarks.h>
#include <Queue.h>
#include <farcolor.hpp>
// FAR WORKAROUND
//---------------------------------------------------------------------------
enum TButtonResult { brCancel = -1, brOK = 1, brConnect };
//---------------------------------------------------------------------------
class TWinSCPDialog : public TFarDialog
{
public:
    explicit TWinSCPDialog(TCustomFarPlugin *AFarPlugin);

    void AddStandardButtons(int Shift = 0, bool ButtonsOnly = false);

    TFarSeparator *ButtonSeparator;
    TFarButton *OkButton;
    TFarButton *CancelButton;
};
//---------------------------------------------------------------------------
TWinSCPDialog::TWinSCPDialog(TCustomFarPlugin *AFarPlugin) :
    TFarDialog(AFarPlugin),
    ButtonSeparator(NULL),
    OkButton(NULL),
    CancelButton(NULL)
{
}
//---------------------------------------------------------------------------
void TWinSCPDialog::AddStandardButtons(int Shift, bool ButtonsOnly)
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
            OkButton->SetTop(Shift);
            OkButton->SetBottom(Shift);
        }
    }
    OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
    OkButton->SetDefault(true);
    OkButton->SetResult(brOK);
    OkButton->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    assert(CancelButton == NULL);
    CancelButton = new TFarButton(this);
    CancelButton->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    CancelButton->SetResult(brCancel);
    CancelButton->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTabbedDialog : public TWinSCPDialog
{
    friend class TTabButton;

public:
    explicit TTabbedDialog(TCustomFarPlugin *AFarPlugin, int TabCount);
    // __property int Tab = { read = FTab };
    int GetTab() { return FTab; }

protected:
    void HideTabs();
    virtual void SelectTab(int Tab);
    void TabButtonClick(TFarButton *Sender, bool &Close);
    virtual bool Key(TFarDialogItem *Item, long KeyCode);
    virtual std::wstring TabName(int Tab);
    TTabButton *TabButton(int Tab);

private:
    std::wstring FOrigCaption;
    int FTab;
    int FTabCount;
};
//---------------------------------------------------------------------------
class TTabButton : public TFarButton
{
public:
    explicit TTabButton(TTabbedDialog *Dialog);

    // __property int Tab = { read = FTab, write = FTab };
    int GetTab() { return FTab; }
    void SetTab(int value) { FTab = value; }
    // __property std::wstring GetTabName() = { read = FTabName, write = SetTabName };
    std::wstring GetTabName() { return FTabName; }
    void SetTabName(const std::wstring value);

private:
    std::wstring FTabName;
    int FTab;
};
//---------------------------------------------------------------------------
TTabbedDialog::TTabbedDialog(TCustomFarPlugin *AFarPlugin, int TabCount) :
    TWinSCPDialog(AFarPlugin)
{
    FTab = 0;
    FTabCount = TabCount;

    // FAR WORKAROUND
    // (to avoid first control on dialog be a button, that would be "pressed"
    // when listbox loses focus)
    TFarText *Text = new TFarText(this);
    // make next item be inserted to default position
    Text->Move(0, -1);
    // on FAR 1.70 alpha 6 and later, empty text control would overwrite the
    // dialog box caption
    Text->SetVisible(false);
}
//---------------------------------------------------------------------------
void TTabbedDialog::HideTabs()
{
    for (size_t i = 0; i < GetItemCount(); i++)
    {
        TFarDialogItem *I = GetItem(i);
        if (I->GetGroup())
        {
            I->SetVisible(false);
        }
    }
}
//---------------------------------------------------------------------------
void TTabbedDialog::SelectTab(int Tab)
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

    for (size_t i = 0; i < GetItemCount(); i++)
    {
        TFarDialogItem *I = GetItem(i);
        if ((I->GetGroup() == Tab) && I->CanFocus())
        {
            I->SetFocus();
            break;
        }
    }

    if (FOrigCaption.empty())
    {
        FOrigCaption = GetCaption();
    }
    SetCaption(FORMAT(L"%s - %s", TabName(Tab).c_str(), FOrigCaption.c_str()));
}
//---------------------------------------------------------------------------
TTabButton *TTabbedDialog::TabButton(int Tab)
{
    TTabButton *Result = NULL;
    for (size_t i = 0; i < GetItemCount(); i++)
    {
        TTabButton *T = dynamic_cast<TTabButton *>(GetItem(i));
        if ((T != NULL) && (T->GetTab() == Tab))
        {
            Result = T;
            break;
        }
    }

    assert(Result != NULL);

    return Result;
}
//---------------------------------------------------------------------------
std::wstring TTabbedDialog::TabName(int Tab)
{
    return TabButton(Tab)->GetTabName();
}
//---------------------------------------------------------------------------
void TTabbedDialog::TabButtonClick(TFarButton *Sender, bool &Close)
{
    TTabButton *Tab = dynamic_cast<TTabButton *>(Sender);
    assert(Tab != NULL);

    SelectTab(Tab->GetTab());

    Close = false;
}
//---------------------------------------------------------------------------
bool TTabbedDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
    bool Result = false;
    if (KeyCode == (VK_NEXT | (LEFT_CTRL_PRESSED << 16)) ||
        KeyCode == (VK_PRIOR | (LEFT_CTRL_PRESSED << 16)))
    {
        int NewTab = FTab;
        do
        {
            if (KeyCode == VK_NEXT)
            {
                NewTab = NewTab == FTabCount - 1 ? 1 : NewTab + 1;
            }
            else
            {
                NewTab = NewTab == 1 ? FTabCount - 1 : NewTab - 1;
            }
        }
        while (!TabButton(NewTab)->GetEnabled());
        SelectTab(NewTab);
        Result = true;
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TTabButton::TTabButton(TTabbedDialog *Dialog) :
    TFarButton(Dialog)
{
    SetCenterGroup(true);
    SetOnClick(boost::bind(&TTabbedDialog::TabButtonClick, Dialog, _1, _2));
}
//---------------------------------------------------------------------------
void TTabButton::SetTabName(const std::wstring Value)
{
    std::wstring value = Value;
    if (FTabName != value)
    {
        std::wstring C;
        size_t P = ::Pos(value, L"|");
        if (P > 0)
        {
            C = value.substr(0, P);
            value.erase(0, P + 1);
        }
        else
        {
            C = value;
        }
        SetCaption(C);
        FTabName = StripHotKey(value);
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPPlugin::ConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarText *Text;

        Dialog->SetSize(nb::TPoint(67, 23));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_INTERFACE)).c_str()));

        TFarCheckBox *DisksMenuCheck = new TFarCheckBox(Dialog);
        DisksMenuCheck->SetCaption(GetMsg(CONFIG_DISKS_MENU));

        Text = new TFarText(Dialog);
        Text->SetLeft(Text->GetLeft() + 4);
        Text->SetCaption(GetMsg(CONFIG_HOTKEY_LABEL));
        Text->SetEnabledDependency(DisksMenuCheck);

        Dialog->SetNextItemPosition(ipRight);

        TFarRadioButton *AutoHotKeyButton = new TFarRadioButton(Dialog);
        AutoHotKeyButton->SetCaption(GetMsg(CONFIG_HOTKEY_AUTOASSIGN));
        AutoHotKeyButton->SetEnabledDependency(DisksMenuCheck);
        AutoHotKeyButton->SetChecked(true);

        TFarRadioButton *ManualHotKeyButton = new TFarRadioButton(Dialog);
        ManualHotKeyButton->SetCaption(GetMsg(CONFIG_HOTKEY_MANUAL));
        // ManualHotKeyButton->SetEnabledDependency(DisksMenuCheck);
        ManualHotKeyButton->SetEnabled(false);
        AutoHotKeyButton->SetChecked(false);

        TFarEdit *HotKeyEdit = new TFarEdit(Dialog);
        HotKeyEdit->SetWidth(1);
        HotKeyEdit->SetFixed(true);
        HotKeyEdit->SetMask(L"9");
        HotKeyEdit->SetEnabledDependency(ManualHotKeyButton);

        Text = new TFarText(Dialog);
        Text->SetCaption(L"(1 - 9)");
        Text->SetEnabledDependency(ManualHotKeyButton);

        Dialog->SetNextItemPosition(ipNewLine);

        TFarCheckBox *PluginsMenuCheck = new TFarCheckBox(Dialog);
        PluginsMenuCheck->SetCaption(GetMsg(CONFIG_PLUGINS_MENU));

        TFarCheckBox *PluginsMenuCommandsCheck = new TFarCheckBox(Dialog);
        PluginsMenuCommandsCheck->SetCaption(GetMsg(CONFIG_PLUGINS_MENU_COMMANDS));

        TFarCheckBox *HostNameInTitleCheck = new TFarCheckBox(Dialog);
        HostNameInTitleCheck->SetCaption(GetMsg(CONFIG_HOST_NAME_IN_TITLE));

        new TFarSeparator(Dialog);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(CONFIG_COMAND_PREFIXES));

        TFarEdit *CommandPrefixesEdit = new TFarEdit(Dialog);

        new TFarSeparator(Dialog);

        TFarCheckBox *CustomPanelCheck = new TFarCheckBox(Dialog);
        CustomPanelCheck->SetCaption(GetMsg(CONFIG_PANEL_MODE_CHECK));
        CustomPanelCheck->SetEnabled(false);

        Text = new TFarText(Dialog);
        Text->SetLeft(Text->GetLeft() + 4);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_TYPES));

        Dialog->SetNextItemPosition(ipBelow);

        TFarEdit *CustomPanelTypesEdit = new TFarEdit(Dialog);
        CustomPanelTypesEdit->SetEnabledDependency(CustomPanelCheck);
        CustomPanelTypesEdit->SetWidth(CustomPanelTypesEdit->GetWidth() / 2 - 1);

        Dialog->SetNextItemPosition(ipRight);

        Text = new TFarText(Dialog);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->Move(0, -1);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_STATUS_TYPES));

        Dialog->SetNextItemPosition(ipBelow);

        TFarEdit *CustomPanelStatusTypesEdit = new TFarEdit(Dialog);
        CustomPanelStatusTypesEdit->SetEnabledDependency(CustomPanelCheck);

        Dialog->SetNextItemPosition(ipNewLine);

        Text = new TFarText(Dialog);
        Text->SetLeft(Text->GetLeft() + 4);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_WIDTHS));

        Dialog->SetNextItemPosition(ipBelow);

        TFarEdit *CustomPanelWidthsEdit = new TFarEdit(Dialog);
        CustomPanelWidthsEdit->SetEnabledDependency(CustomPanelCheck);
        CustomPanelWidthsEdit->SetWidth(CustomPanelTypesEdit->GetWidth());

        Dialog->SetNextItemPosition(ipRight);

        Text = new TFarText(Dialog);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->Move(0, -1);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_STATUS_WIDTHS));

        Dialog->SetNextItemPosition(ipBelow);

        TFarEdit *CustomPanelStatusWidthsEdit = new TFarEdit(Dialog);
        CustomPanelStatusWidthsEdit->SetEnabledDependency(CustomPanelCheck);

        Dialog->SetNextItemPosition(ipNewLine);

        TFarCheckBox *CustomPanelFullScreenCheck = new TFarCheckBox(Dialog);
        CustomPanelFullScreenCheck->SetLeft(CustomPanelFullScreenCheck->GetLeft() + 4);
        CustomPanelFullScreenCheck->SetEnabledDependency(CustomPanelCheck);
        CustomPanelFullScreenCheck->SetCaption(GetMsg(CONFIG_PANEL_MODE_FULL_SCREEN));

        Text = new TFarText(Dialog);
        Text->SetLeft(Text->GetLeft() + 4);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_HINT));
        Text = new TFarText(Dialog);
        Text->SetLeft(Text->GetLeft() + 4);
        Text->SetEnabledDependency(CustomPanelCheck);
        Text->SetCaption(GetMsg(CONFIG_PANEL_MODE_HINT2));

        Dialog->AddStandardButtons();

        DisksMenuCheck->SetChecked(FarConfiguration->GetDisksMenu());
        AutoHotKeyButton->SetChecked(!FarConfiguration->GetDisksMenuHotKey() != 0);
        ManualHotKeyButton->SetChecked(FarConfiguration->GetDisksMenuHotKey() != 0);
        HotKeyEdit->SetText(FarConfiguration->GetDisksMenuHotKey() ?
                            IntToStr(FarConfiguration->GetDisksMenuHotKey()) : std::wstring());
        PluginsMenuCheck->SetChecked(FarConfiguration->GetPluginsMenu());
        PluginsMenuCommandsCheck->SetChecked(FarConfiguration->GetPluginsMenuCommands());
        HostNameInTitleCheck->SetChecked(FarConfiguration->GetHostNameInTitle());
        CommandPrefixesEdit->SetText(FarConfiguration->GetCommandPrefixes());

        CustomPanelCheck->SetChecked(FarConfiguration->GetCustomPanelModeDetailed());
        CustomPanelTypesEdit->SetText(FarConfiguration->GetColumnTypesDetailed());
        CustomPanelWidthsEdit->SetText(FarConfiguration->GetColumnWidthsDetailed());
        CustomPanelStatusTypesEdit->SetText(FarConfiguration->GetStatusColumnTypesDetailed());
        CustomPanelStatusWidthsEdit->SetText(FarConfiguration->GetStatusColumnWidthsDetailed());
        CustomPanelFullScreenCheck->SetChecked(FarConfiguration->GetFullScreenDetailed());

        Result = (Dialog->ShowModal() == brOK);
        if (Result)
        {
            FarConfiguration->SetDisksMenu(DisksMenuCheck->GetChecked());
            FarConfiguration->SetDisksMenuHotKey(
                ManualHotKeyButton->GetChecked() && !HotKeyEdit->GetIsEmpty() ?
                StrToInt(HotKeyEdit->GetText()) : 0);
            FarConfiguration->SetPluginsMenu(PluginsMenuCheck->GetChecked());
            FarConfiguration->SetPluginsMenuCommands(PluginsMenuCommandsCheck->GetChecked());
            FarConfiguration->SetHostNameInTitle(HostNameInTitleCheck->GetChecked());

            FarConfiguration->SetCommandPrefixes(CommandPrefixesEdit->GetText());

            FarConfiguration->SetCustomPanelModeDetailed(CustomPanelCheck->GetChecked());
            FarConfiguration->SetColumnTypesDetailed(CustomPanelTypesEdit->GetText());
            FarConfiguration->SetColumnWidthsDetailed(CustomPanelWidthsEdit->GetText());
            FarConfiguration->SetStatusColumnTypesDetailed(CustomPanelStatusTypesEdit->GetText());
            FarConfiguration->SetStatusColumnWidthsDetailed(CustomPanelStatusWidthsEdit->GetText());
            FarConfiguration->SetFullScreenDetailed(CustomPanelFullScreenCheck->GetChecked());
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::PanelConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->SetSize(nb::TPoint(65, 7));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_PANEL)).c_str()));

        TFarCheckBox *AutoReadDirectoryAfterOpCheck = new TFarCheckBox(Dialog);
        AutoReadDirectoryAfterOpCheck->SetCaption(GetMsg(CONFIG_AUTO_READ_DIRECTORY_AFTER_OP));

        Dialog->AddStandardButtons();

        AutoReadDirectoryAfterOpCheck->SetChecked(Configuration->GetAutoReadDirectoryAfterOp());

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                Configuration->SetAutoReadDirectoryAfterOp(AutoReadDirectoryAfterOpCheck->GetChecked());
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::LoggingConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarSeparator *Separator;
        TFarText *Text;

        Dialog->SetSize(nb::TPoint(65, 15));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_LOGGING)).c_str()));

        TFarCheckBox *LoggingCheck = new TFarCheckBox(Dialog);
        LoggingCheck->SetCaption(GetMsg(LOGGING_ENABLE));

        Separator = new TFarSeparator(Dialog);
        Separator->SetCaption(GetMsg(LOGGING_OPTIONS_GROUP));

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(LOGGING_LOG_PROTOCOL));
        Text->SetEnabledDependency(LoggingCheck);

        Dialog->SetNextItemPosition(ipRight);

        TFarComboBox *LogProtocolCombo = new TFarComboBox(Dialog);
        LogProtocolCombo->SetDropDownList(true);
        LogProtocolCombo->SetWidth(10);
        for (int i = 0; i <= 2; i++)
        {
            LogProtocolCombo->GetItems()->Add(GetMsg(LOGGING_LOG_PROTOCOL_0 + i));
        }
        LogProtocolCombo->SetEnabledDependency(LoggingCheck);

        Dialog->SetNextItemPosition(ipNewLine);

        new TFarSeparator(Dialog);

        TFarCheckBox *LogToFileCheck = new TFarCheckBox(Dialog);
        LogToFileCheck->SetCaption(GetMsg(LOGGING_LOG_TO_FILE));
        LogToFileCheck->SetEnabledDependency(LoggingCheck);

        TFarEdit *LogFileNameEdit = new TFarEdit(Dialog);
        LogFileNameEdit->SetLeft(LogFileNameEdit->GetLeft() + 4);
        LogFileNameEdit->SetHistory(LOG_FILE_HISTORY);
        LogFileNameEdit->SetEnabledDependency(LogToFileCheck);

        Dialog->SetNextItemPosition(ipBelow);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(LOGGING_LOG_FILE_HINT1));
        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(LOGGING_LOG_FILE_HINT2));

        TFarRadioButton *LogFileAppendButton = new TFarRadioButton(Dialog);
        LogFileAppendButton->SetCaption(GetMsg(LOGGING_LOG_FILE_APPEND));
        LogFileAppendButton->SetEnabledDependency(LogToFileCheck);

        Dialog->SetNextItemPosition(ipRight);

        TFarRadioButton *LogFileOverwriteButton = new TFarRadioButton(Dialog);
        LogFileOverwriteButton->SetCaption(GetMsg(LOGGING_LOG_FILE_OVERWRITE));
        LogFileOverwriteButton->SetEnabledDependency(LogToFileCheck);

        Dialog->AddStandardButtons();

        LoggingCheck->SetChecked(Configuration->GetLogging());
        LogProtocolCombo->GetItems()->SetSelected(Configuration->GetLogProtocol());
        LogToFileCheck->SetChecked(Configuration->GetLogToFile());
        LogFileNameEdit->SetText(
            (!Configuration->GetLogToFile() && Configuration->GetLogFileName().empty()) ?
            IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"&s.log" :
            Configuration->GetLogFileName());
        LogFileAppendButton->SetChecked(Configuration->GetLogFileAppend());
        LogFileOverwriteButton->SetChecked(!Configuration->GetLogFileAppend());

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                Configuration->SetLogging(LoggingCheck->GetChecked());
                Configuration->SetLogProtocol(LogProtocolCombo->GetItems()->GetSelected());
                Configuration->SetLogToFile(LogToFileCheck->GetChecked());
                if (LogToFileCheck->GetChecked())
                {
                    Configuration->SetLogFileName(LogFileNameEdit->GetText());
                }
                Configuration->SetLogFileAppend(LogFileAppendButton->GetChecked());
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::TransferConfigurationDialog()
{
    std::wstring Caption = FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_TRANSFER)).c_str());

    TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();
    bool Result = CopyParamDialog(Caption, CopyParam, 0);
    if (Result)
    {
        GUIConfiguration->SetDefaultCopyParam(CopyParam);
    }

    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::EnduranceConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarSeparator *Separator;
        TFarText *Text;

        Dialog->SetSize(nb::TPoint(76, 13));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_ENDURANCE)).c_str()));

        Separator = new TFarSeparator(Dialog);
        Separator->SetCaption(GetMsg(TRANSFER_RESUME));

        TFarRadioButton *ResumeOnButton = new TFarRadioButton(Dialog);
        ResumeOnButton->SetCaption(GetMsg(TRANSFER_RESUME_ON));

        TFarRadioButton *ResumeSmartButton = new TFarRadioButton(Dialog);
        ResumeSmartButton->SetCaption(GetMsg(TRANSFER_RESUME_SMART));
        int ResumeThresholdLeft = ResumeSmartButton->GetRight();

        TFarRadioButton *ResumeOffButton = new TFarRadioButton(Dialog);
        ResumeOffButton->SetCaption(GetMsg(TRANSFER_RESUME_OFF));

        TFarEdit *ResumeThresholdEdit = new TFarEdit(Dialog);
        ResumeThresholdEdit->Move(0, -2);
        ResumeThresholdEdit->SetLeft(ResumeThresholdLeft + 3);
        ResumeThresholdEdit->SetFixed(true);
        ResumeThresholdEdit->SetMask(L"9999999");
        ResumeThresholdEdit->SetWidth(9);
        ResumeThresholdEdit->SetEnabledDependency(ResumeSmartButton);

        Dialog->SetNextItemPosition(ipRight);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(TRANSFER_RESUME_THRESHOLD_UNIT));
        Text->SetEnabledDependency(ResumeSmartButton);

        Dialog->SetNextItemPosition(ipNewLine);

        Separator = new TFarSeparator(Dialog);
        Separator->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_GROUP));
        Separator->Move(0, 1);

        TFarCheckBox *SessionReopenAutoCheck = new TFarCheckBox(Dialog);
        SessionReopenAutoCheck->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_AUTO));

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_AUTO_LABEL));
        Text->SetEnabledDependency(SessionReopenAutoCheck);
        Text->Move(4, 0);

        Dialog->SetNextItemPosition(ipRight);

        TFarEdit *SessionReopenAutoEdit = new TFarEdit(Dialog);
        SessionReopenAutoEdit->SetEnabledDependency(SessionReopenAutoCheck);
        SessionReopenAutoEdit->SetFixed(true);
        SessionReopenAutoEdit->SetMask(L"999");
        SessionReopenAutoEdit->SetWidth(5);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(TRANSFER_SESSION_REOPEN_AUTO_UNIT));
        Text->SetEnabledDependency(SessionReopenAutoCheck);

        Dialog->AddStandardButtons();

        ResumeOnButton->SetChecked(GUIConfiguration->GetDefaultCopyParam().GetResumeSupport() == rsOn);
        ResumeSmartButton->SetChecked(GUIConfiguration->GetDefaultCopyParam().GetResumeSupport() == rsSmart);
        ResumeOffButton->SetChecked(GUIConfiguration->GetDefaultCopyParam().GetResumeSupport() == rsOff);
        ResumeThresholdEdit->SetAsInteger(
            static_cast<int>(GUIConfiguration->GetDefaultCopyParam().GetResumeThreshold() / 1024));

        SessionReopenAutoCheck->SetChecked((Configuration->GetSessionReopenAuto() > 0));
        SessionReopenAutoEdit->SetAsInteger((Configuration->GetSessionReopenAuto() > 0 ?
                                             (Configuration->GetSessionReopenAuto() / 1000): 5));

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();

                if (ResumeOnButton->GetChecked()) { CopyParam.SetResumeSupport(rsOn); }
                if (ResumeSmartButton->GetChecked()) { CopyParam.SetResumeSupport(rsSmart); }
                if (ResumeOffButton->GetChecked()) { CopyParam.SetResumeSupport(rsOff); }
                CopyParam.SetResumeThreshold(ResumeThresholdEdit->GetAsInteger() * 1024);

                GUIConfiguration->SetDefaultCopyParam(CopyParam);

                Configuration->SetSessionReopenAuto(
                    (SessionReopenAutoCheck->GetChecked() ? (SessionReopenAutoEdit->GetAsInteger() * 1000) : 0));
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::QueueConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarText *Text;

        Dialog->SetSize(nb::TPoint(76, 11));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_BACKGROUND)).c_str()));

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(TRANSFER_QUEUE_LIMIT));

        Dialog->SetNextItemPosition(ipRight);

        TFarEdit *QueueTransferLimitEdit = new TFarEdit(Dialog);
        QueueTransferLimitEdit->SetFixed(true);
        QueueTransferLimitEdit->SetMask(L"9");
        QueueTransferLimitEdit->SetWidth(3);

        Dialog->SetNextItemPosition(ipNewLine);

        TFarCheckBox *QueueCheck = new TFarCheckBox(Dialog);
        QueueCheck->SetCaption(GetMsg(TRANSFER_QUEUE_DEFAULT));

        TFarCheckBox *QueueAutoPopupCheck = new TFarCheckBox(Dialog);
        QueueAutoPopupCheck->SetCaption(GetMsg(TRANSFER_AUTO_POPUP));

        TFarCheckBox *RememberPasswordCheck = new TFarCheckBox(Dialog);
        RememberPasswordCheck->SetCaption(GetMsg(TRANSFER_REMEMBER_PASSWORD));

        TFarCheckBox *QueueBeepCheck = new TFarCheckBox(Dialog);
        QueueBeepCheck->SetCaption(GetMsg(TRANSFER_QUEUE_BEEP));

        Dialog->AddStandardButtons();

        QueueTransferLimitEdit->SetAsInteger(FarConfiguration->GetQueueTransfersLimit());
        QueueCheck->SetChecked(FarConfiguration->GetDefaultCopyParam().GetQueue());
        QueueAutoPopupCheck->SetChecked(FarConfiguration->GetQueueAutoPopup());
        RememberPasswordCheck->SetChecked(GUIConfiguration->GetQueueRememberPassword());
        QueueBeepCheck->SetChecked(FarConfiguration->GetQueueBeep());

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                TGUICopyParamType &CopyParam = GUIConfiguration->GetDefaultCopyParam();

                FarConfiguration->SetQueueTransfersLimit(QueueTransferLimitEdit->GetAsInteger());
                CopyParam.SetQueue(QueueCheck->GetChecked());
                FarConfiguration->SetQueueAutoPopup(QueueAutoPopupCheck->GetChecked());
                GUIConfiguration->SetQueueRememberPassword(RememberPasswordCheck->GetChecked());
                FarConfiguration->SetQueueBeep(QueueBeepCheck->GetChecked());

                GUIConfiguration->SetDefaultCopyParam(CopyParam);
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TTransferEditorConfigurationDialog : public TWinSCPDialog
{
public:
    explicit TTransferEditorConfigurationDialog(TCustomFarPlugin *AFarPlugin);

    bool Execute();

protected:
    virtual void Change();

private:
    TFarCheckBox *EditorMultipleCheck;
    TFarCheckBox *EditorUploadOnSaveCheck;
    TFarRadioButton *EditorDownloadDefaultButton;
    TFarRadioButton *EditorDownloadOptionsButton;
    TFarRadioButton *EditorUploadSameButton;
    TFarRadioButton *EditorUploadOptionsButton;
    TTransferEditorConfigurationDialog *Self;

    virtual void UpdateControls();
};
//---------------------------------------------------------------------------
TTransferEditorConfigurationDialog::TTransferEditorConfigurationDialog(
    TCustomFarPlugin *AFarPlugin) :
    TWinSCPDialog(AFarPlugin)
{
    Self = this;
    TFarSeparator *Separator;

    SetSize(nb::TPoint(55, 14));
    SetCaption(FORMAT(L"%s - %s",
                      GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_TRANSFER_EDITOR)).c_str()));

    EditorMultipleCheck = new TFarCheckBox(this);
    EditorMultipleCheck->SetCaption(GetMsg(TRANSFER_EDITOR_MULTIPLE));

    EditorUploadOnSaveCheck = new TFarCheckBox(this);
    EditorUploadOnSaveCheck->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_ON_SAVE));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD));

    EditorDownloadDefaultButton = new TFarRadioButton(this);
    EditorDownloadDefaultButton->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD_DEFAULT));

    EditorDownloadOptionsButton = new TFarRadioButton(this);
    EditorDownloadOptionsButton->SetCaption(GetMsg(TRANSFER_EDITOR_DOWNLOAD_OPTIONS));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD));

    EditorUploadSameButton = new TFarRadioButton(this);
    EditorUploadSameButton->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_SAME));

    EditorUploadOptionsButton = new TFarRadioButton(this);
    EditorUploadOptionsButton->SetCaption(GetMsg(TRANSFER_EDITOR_UPLOAD_OPTIONS));

    AddStandardButtons();
}
//---------------------------------------------------------------------------
bool TTransferEditorConfigurationDialog::Execute()
{
    EditorDownloadDefaultButton->SetChecked(FarConfiguration->GetEditorDownloadDefaultMode());
    EditorDownloadOptionsButton->SetChecked(!FarConfiguration->GetEditorDownloadDefaultMode());
    EditorUploadSameButton->SetChecked(FarConfiguration->GetEditorUploadSameOptions());
    EditorUploadOptionsButton->SetChecked(!FarConfiguration->GetEditorUploadSameOptions());
    EditorUploadOnSaveCheck->SetChecked(FarConfiguration->GetEditorUploadOnSave());
    EditorMultipleCheck->SetChecked(FarConfiguration->GetEditorMultiple());

    bool Result = (ShowModal() == brOK);

    if (Result)
    {
        Configuration->BeginUpdate();
        {
            BOOST_SCOPE_EXIT ( (&Configuration) )
            {
                Configuration->EndUpdate();
            } BOOST_SCOPE_EXIT_END
            FarConfiguration->SetEditorDownloadDefaultMode(EditorDownloadDefaultButton->GetChecked());
            FarConfiguration->SetEditorUploadSameOptions(EditorUploadSameButton->GetChecked());
            FarConfiguration->SetEditorUploadOnSave(EditorUploadOnSaveCheck->GetChecked());
            FarConfiguration->SetEditorMultiple(EditorMultipleCheck->GetChecked());
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
void TTransferEditorConfigurationDialog::Change()
{
    TWinSCPDialog::Change();

    if (GetHandle())
    {
        LockChanges();
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->UnlockChanges();
            } BOOST_SCOPE_EXIT_END
            UpdateControls();
        }
    }
}
//---------------------------------------------------------------------------
void TTransferEditorConfigurationDialog::UpdateControls()
{
    EditorDownloadDefaultButton->SetEnabled(!EditorMultipleCheck->GetChecked());
    EditorDownloadOptionsButton->SetEnabled(EditorDownloadDefaultButton->GetEnabled());

    EditorUploadSameButton->SetEnabled(
        !EditorMultipleCheck->GetChecked() && !EditorUploadOnSaveCheck->GetChecked());
    EditorUploadOptionsButton->SetEnabled(EditorUploadSameButton->GetEnabled());
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::TransferEditorConfigurationDialog()
{
    bool Result;
    TTransferEditorConfigurationDialog *Dialog = new TTransferEditorConfigurationDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute();
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::ConfirmationsConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->SetSize(nb::TPoint(65, 10));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_CONFIRMATIONS)).c_str()));

        TFarCheckBox *ConfirmOverwritingCheck = new TFarCheckBox(Dialog);
        ConfirmOverwritingCheck->SetAllowGrayed(true);
        ConfirmOverwritingCheck->SetCaption(GetMsg(CONFIRMATIONS_CONFIRM_OVERWRITING));

        TFarCheckBox *ConfirmCommandSessionCheck = new TFarCheckBox(Dialog);
        ConfirmCommandSessionCheck->SetCaption(GetMsg(CONFIRMATIONS_OPEN_COMMAND_SESSION));

        TFarCheckBox *ConfirmResumeCheck = new TFarCheckBox(Dialog);
        ConfirmResumeCheck->SetCaption(GetMsg(CONFIRMATIONS_CONFIRM_RESUME));

        TFarCheckBox *ConfirmSynchronizedBrowsingCheck = new TFarCheckBox(Dialog);
        ConfirmSynchronizedBrowsingCheck->SetCaption(GetMsg(CONFIRMATIONS_SYNCHRONIZED_BROWSING));

        Dialog->AddStandardButtons();

        ConfirmOverwritingCheck->SetSelected(!FarConfiguration->GetConfirmOverwritingOverride() ?
                                             BSTATE_3STATE : (Configuration->GetConfirmOverwriting() ? BSTATE_CHECKED :
                                                     BSTATE_UNCHECKED));
        ConfirmCommandSessionCheck->SetChecked(GUIConfiguration->GetConfirmCommandSession());
        ConfirmResumeCheck->SetChecked(GUIConfiguration->GetConfirmResume());
        ConfirmSynchronizedBrowsingCheck->SetChecked(FarConfiguration->GetConfirmSynchronizedBrowsing());

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                FarConfiguration->SetConfirmOverwritingOverride(
                    ConfirmOverwritingCheck->GetSelected() != BSTATE_3STATE);
                GUIConfiguration->SetConfirmCommandSession(ConfirmCommandSessionCheck->GetChecked());
                GUIConfiguration->SetConfirmResume(ConfirmResumeCheck->GetChecked());
                if (FarConfiguration->GetConfirmOverwritingOverride())
                {
                    Configuration->SetConfirmOverwriting(ConfirmOverwritingCheck->GetChecked());
                }
                FarConfiguration->SetConfirmSynchronizedBrowsing(ConfirmSynchronizedBrowsingCheck->GetChecked());
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::IntegrationConfigurationDialog()
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarText *Text;

        Dialog->SetSize(nb::TPoint(65, 14));
        Dialog->SetCaption(FORMAT(L"%s - %s",
                                  GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_INTEGRATION)).c_str()));

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(INTEGRATION_PUTTY));

        TFarEdit *PuttyPathEdit = new TFarEdit(Dialog);

        TFarCheckBox *PuttyPasswordCheck = new TFarCheckBox(Dialog);
        PuttyPasswordCheck->SetCaption(GetMsg(INTEGRATION_PUTTY_PASSWORD));
        PuttyPasswordCheck->SetEnabledDependency(PuttyPathEdit);

        TFarCheckBox *TelnetForFtpInPuttyCheck = new TFarCheckBox(Dialog);
        TelnetForFtpInPuttyCheck->SetCaption(GetMsg(INTEGRATION_TELNET_FOR_FTP_IN_PUTTY));
        TelnetForFtpInPuttyCheck->SetEnabledDependency(PuttyPathEdit);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(INTEGRATION_PAGEANT));

        TFarEdit *PageantPathEdit = new TFarEdit(Dialog);

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(INTEGRATION_PUTTYGEN));

        TFarEdit *PuttygenPathEdit = new TFarEdit(Dialog);

        Dialog->AddStandardButtons();

        PuttyPathEdit->SetText(GUIConfiguration->GetPuttyPath());
        PuttyPasswordCheck->SetChecked(GUIConfiguration->GetPuttyPassword());
        TelnetForFtpInPuttyCheck->SetChecked(GUIConfiguration->GetTelnetForFtpInPutty());
        PageantPathEdit->SetText(FarConfiguration->GetPageantPath());
        PuttygenPathEdit->SetText(FarConfiguration->GetPuttygenPath());

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Configuration->BeginUpdate();
            {
                BOOST_SCOPE_EXIT ( (&Configuration) )
                {
                    Configuration->EndUpdate();
                } BOOST_SCOPE_EXIT_END
                GUIConfiguration->SetPuttyPath(PuttyPathEdit->GetText());
                GUIConfiguration->SetPuttyPassword(PuttyPasswordCheck->GetChecked());
                GUIConfiguration->SetTelnetForFtpInPutty(TelnetForFtpInPuttyCheck->GetChecked());
                FarConfiguration->SetPageantPath(PageantPathEdit->GetText());
                FarConfiguration->SetPuttygenPath(PuttygenPathEdit->GetText());
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
class TAboutDialog : public TFarDialog
{
public:
    explicit TAboutDialog(TCustomFarPlugin *AFarPlugin);

private:
    void UrlButtonClick(TFarButton *Sender, bool &Close);
    void UrlTextClick(TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);
};
//---------------------------------------------------------------------------
std::wstring ReplaceCopyright(const std::wstring S)
{
    return ::StringReplace(S, L"�", L"(c)");
}
//---------------------------------------------------------------------------
TAboutDialog::TAboutDialog(TCustomFarPlugin *AFarPlugin) :
    TFarDialog(AFarPlugin)
{
    TFarText *Text;
    TFarButton *Button;

    // std::wstring ProductName = Configuration->GetFileInfoString(L"ProductName");
    std::wstring ProductName = LoadStr(WINSCPFAR_NAME);
    std::wstring Comments;
    try
    {
        Comments = Configuration->GetFileInfoString(L"Comments");
    }
    catch (...)
    {
        Comments = L"";
    }
    std::wstring LegalCopyright; // = Configuration->GetFileInfoString(L"LegalCopyright");

    int Height = 15;
#ifndef NO_FILEZILLA
    Height += 2;
#endif
    if (!ProductName.empty())
    {
        Height++;
    }
    if (!Comments.empty())
    {
        Height++;
    }
    if (!LegalCopyright.empty())
    {
        Height++;
    }
    SetSize(nb::TPoint(55, Height));

    SetCaption(FORMAT(L"%s - %s",
                      GetMsg(PLUGIN_TITLE).c_str(), StripHotKey(GetMsg(CONFIG_ABOUT)).c_str()));
    Text = new TFarText(this);
    Text->SetCaption(Configuration->GetFileInfoString(L"FileDescription"));
    Text->SetCenterGroup(true);

    Text = new TFarText(this);
    // DEBUG_PRINTF(L"Configuration->GetVersion = %s", Configuration->GetVersion().c_str());
    Text->SetCaption(FORMAT(GetMsg(ABOUT_VERSION).c_str(), Configuration->GetVersion().c_str()));
    Text->SetCenterGroup(true);

    Text = new TFarText(this);
    Text->Move(0, 1);
    Text->SetCaption(LoadStr(WINSCPFAR_BASED_ON));
    Text->SetCenterGroup(true);

    Text = new TFarText(this);
    Text->SetCaption(FMTLOAD(WINSCPFAR_BASED_VERSION, LoadStr(WINSCPFAR_VERSION).c_str()));
    Text->SetCenterGroup(true);

    if (!ProductName.empty())
    {
        Text = new TFarText(this);
        Text->SetCaption(FORMAT(GetMsg(ABOUT_PRODUCT_VERSION).c_str(),
                                ProductName.c_str(),
                                LoadStr(WINSCP_VERSION).c_str()));
        Text->SetCenterGroup(true);
    }

    Text = new TFarText(this);
    Text->SetCaption(LoadStr(WINSCPFAR_BASED_COPYRIGHT));
    Text->SetCenterGroup(true);

    if (!Comments.empty())
    {
        Text = new TFarText(this);
        if (ProductName.empty())
        {
            Text->Move(0, 1);
        }
        Text->SetCaption(Comments);
        Text->SetCenterGroup(true);
    }
#if 0
    if (!LegalCopyright.empty())
    {
        Text = new TFarText(this);
        Text->Move(0, 1);
        Text->SetCaption(Configuration->GetFileInfoString(L"LegalCopyright"));
        Text->SetCenterGroup(true);
    }
    Text = new TFarText(this);
    if (LegalCopyright.empty())
    {
        Text->Move(0, 1);
    }
    Text->SetCaption(GetMsg(ABOUT_URL));
    // FIXME Text->SetColor(static_cast<int>((GetSystemColor(COL_DIALOGTEXT) & 0xF0) | 0x09));
    Text->SetCenterGroup(true);
    Text->SetOnMouseClick(boost::bind(&TAboutDialog::UrlTextClick, this, _1, _2));

    Button = new TFarButton(this);
    Button->Move(0, 1);
    Button->SetCaption(GetMsg(ABOUT_HOMEPAGE));
    Button->SetOnClick(boost::bind(&TAboutDialog::UrlButtonClick, this, _1, _2));
    Button->SetTag(1);
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(ABOUT_FORUM));
    Button->SetOnClick(boost::bind(&TAboutDialog::UrlButtonClick, this, _1, _2));
    Button->SetTag(2);
    Button->SetCenterGroup(true);
    SetNextItemPosition(ipNewLine);
#endif

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(FMTLOAD(PUTTY_BASED_ON, LoadStr(PUTTY_VERSION).c_str()));
    Text->SetCenterGroup(true);

    Text = new TFarText(this);
    Text->SetCaption(ReplaceCopyright(LoadStr(PUTTY_COPYRIGHT)));
    Text->SetCenterGroup(true);

#ifndef NO_FILEZILLA
    Text = new TFarText(this);
    Text->SetCaption(FMTLOAD(FILEZILLA_BASED_ON, LoadStr(FILEZILLA_VERSION).c_str()));
    Text->SetCenterGroup(true);

    Text = new TFarText(this);
    Text->SetCaption(ReplaceCopyright(LoadStr(FILEZILLA_COPYRIGHT)));
    Text->SetCenterGroup(true);
#endif

    new TFarSeparator(this);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
    Button->SetDefault(true);
    Button->SetResult(brOK);
    Button->SetCenterGroup(true);
    Button->SetFocus();
}
//---------------------------------------------------------------------------
void TAboutDialog::UrlTextClick(TFarDialogItem * /*Item*/,
                                MOUSE_EVENT_RECORD * /*Event*/)
{
    std::wstring Address = GetMsg(ABOUT_URL);
    ShellExecute(NULL, L"open", const_cast<wchar_t *>(Address.c_str()), NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
void TAboutDialog::UrlButtonClick(TFarButton *Sender, bool & /*Close*/)
{
    std::wstring Address;
    switch (Sender->GetTag())
    {
    case 1: Address = GetMsg(ABOUT_URL) + L"eng/docs/far"; break;
    case 2: Address = GetMsg(ABOUT_URL) + L"forum/"; break;
    }
    ShellExecute(NULL, L"open", const_cast<wchar_t *>(Address.c_str()), NULL, NULL, SW_SHOWNORMAL);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TWinSCPPlugin::AboutDialog()
{
    DEBUG_PRINTF(L"begin");
    TFarDialog *Dialog = new TAboutDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        DEBUG_PRINTF(L"before Dialog->ShowModal");
        Dialog->ShowModal();
    }
    DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TPasswordDialog : public TFarDialog
{
public:
    explicit TPasswordDialog(TCustomFarPlugin *AFarPlugin,
                             const std::wstring SessionName, TPromptKind Kind, const std::wstring Name,
                             const std::wstring Instructions, nb::TStrings *Prompts, bool StoredCredentialsTried);

    bool Execute(nb::TStrings *Results);

private:
    TSessionData *FSessionData;
    std::wstring FPrompt;
    nb::TList *FEdits;
    TFarCheckBox *SavePasswordCheck;

    void ShowPromptClick(TFarButton *Sender, bool &Close);
    void GenerateLabel(const std::wstring Caption, bool &Truncated);
    TFarEdit *GenerateEdit(bool Echo);
    void GeneratePrompt(bool ShowSavePassword,
                        std::wstring Instructions, nb::TStrings *Prompts, bool &Truncated);
};
//---------------------------------------------------------------------------
TPasswordDialog::TPasswordDialog(TCustomFarPlugin *AFarPlugin,
                                 const std::wstring SessionName, TPromptKind Kind, const std::wstring Name,
                                 const std::wstring Instructions, nb::TStrings *Prompts, bool StoredCredentialsTried) :
    TFarDialog(AFarPlugin),
    FSessionData(NULL),
    FEdits(NULL),
    SavePasswordCheck(NULL)
{
    TFarButton *Button;

    bool ShowSavePassword = false;
    FSessionData = NULL;
    if (((Kind == pkPassword) || (Kind == pkTIS) || (Kind == pkCryptoCard) ||
            (Kind == pkKeybInteractive)) &&
            (Prompts->GetCount() == 1) && !(Prompts->GetObject(0) != NULL) &&
            !SessionName.empty() &&
            StoredCredentialsTried)
    {
        FSessionData = dynamic_cast<TSessionData *>(StoredSessions->FindByName(SessionName));
        ShowSavePassword = (FSessionData != NULL) && !FSessionData->GetPassword().empty();
    }

    bool Truncated = false;
    GeneratePrompt(ShowSavePassword, Instructions, Prompts, Truncated);

    SetCaption(Name);

    if (ShowSavePassword)
    {
        SavePasswordCheck = new TFarCheckBox(this);
        SavePasswordCheck->SetCaption(GetMsg(PASSWORD_SAVE));
    }
    else
    {
        SavePasswordCheck = NULL;
    }

    new TFarSeparator(this);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_OK));
    Button->SetDefault(true);
    Button->SetResult(brOK);
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    if (Truncated)
    {
        Button = new TFarButton(this);
        Button->SetCaption(GetMsg(PASSWORD_SHOW_PROMPT));
        Button->SetOnClick(boost::bind(&TPasswordDialog::ShowPromptClick, this, _1, _2));
        Button->SetCenterGroup(true);
    }

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
void TPasswordDialog::GenerateLabel(const std::wstring Caption,
                                    bool &Truncated)
{
    std::wstring caption = Caption;
    TFarText *Result = new TFarText(this);

    if (!FPrompt.empty())
    {
        FPrompt += L"\n\n";
    }
    FPrompt += caption;

    if (GetSize().x - 10 < static_cast<int>(caption.size()))
    {
        caption.resize(GetSize().x - 10 - 4);
        caption += L" ...";
        Truncated = true;
    }

    Result->SetCaption(caption);
}
//---------------------------------------------------------------------------
TFarEdit *TPasswordDialog::GenerateEdit(bool Echo)
{
    TFarEdit *Result = new TFarEdit(this);
    Result->SetPassword(!Echo);
    return Result;
}
//---------------------------------------------------------------------------
void TPasswordDialog::GeneratePrompt(bool ShowSavePassword,
                                     std::wstring Instructions, nb::TStrings *Prompts, bool &Truncated)
{
    FEdits = new nb::TList;

    nb::TPoint S = nb::TPoint(40, ShowSavePassword ? 1 : 0);

    int x = static_cast<int>(Instructions.size());
    if (S.x < x)
    {
        S.x = x;
    }
    if (!Instructions.empty())
    {
        S.y += 2;
    }

    for (size_t Index = 0; Index < Prompts->GetCount(); Index++)
    {
        int x = static_cast<int>(Prompts->GetString(Index).size());
        if (S.x < x)
        {
            S.x = x;
        }
        S.y += 2;
    }

    if (S.x > 80 - 10)
    {
        S.x = 80 - 10;
    }

    SetSize(nb::TPoint(S.x + 10, S.y + 6));

    if (!Instructions.empty())
    {
        GenerateLabel(Instructions, Truncated);
        // dumb way to add empty line
        GenerateLabel(L"", Truncated);
    }

    for (size_t Index = 0; Index < Prompts->GetCount(); Index++)
    {
        GenerateLabel(Prompts->GetString(Index), Truncated);

        FEdits->Add(GenerateEdit((Prompts->GetObject(Index)) != NULL));
    }
}
//---------------------------------------------------------------------------
void TPasswordDialog::ShowPromptClick(TFarButton * /*Sender*/,
                                      bool & /*Close*/)
{
    TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);

    WinSCPPlugin->MoreMessageDialog(FPrompt, NULL, qtInformation, qaOK);
}
//---------------------------------------------------------------------------
bool TPasswordDialog::Execute(nb::TStrings *Results)
{
    for (size_t Index = 0; Index < FEdits->GetCount(); Index++)
    {
        reinterpret_cast<TFarEdit *>(FEdits->GetItem(Index))->SetText(Results->GetString(Index));
    }

    bool Result = (ShowModal() != brCancel);
    if (Result)
    {
        for (size_t Index = 0; Index < FEdits->GetCount(); Index++)
        {
            std::wstring Text = reinterpret_cast<TFarEdit *>(FEdits->GetItem(Index))->GetText();
            Results->PutString(Index, Text);
        }

        if ((SavePasswordCheck != NULL) && SavePasswordCheck->GetChecked())
        {
            assert(FSessionData != NULL);
            FSessionData->SetPassword(Results->GetString(0));
            // modified only, explicit
            StoredSessions->Save(false, true);
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::PasswordDialog(TSessionData *SessionData,
                                       TPromptKind Kind, const std::wstring Name, const std::wstring Instructions, nb::TStrings *Prompts,
                                       nb::TStrings *Results, bool StoredCredentialsTried)
{
    bool Result = false;
    TPasswordDialog *Dialog = new TPasswordDialog(FPlugin, SessionData->GetName(),
            Kind, Name, Instructions, Prompts, StoredCredentialsTried);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Results);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::BannerDialog(const std::wstring SessionName,
                                     const std::wstring Banner, bool &NeverShowAgain, int Options)
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(FPlugin);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->SetSize(nb::TPoint(70, 21));
        Dialog->SetCaption(FORMAT(GetMsg(BANNER_TITLE).c_str(), SessionName.c_str()));

        TFarLister *Lister = new TFarLister(Dialog);
        FarWrapText(Banner, Lister->GetItems(), Dialog->GetBorderBox()->GetWidth() - 4);
        Lister->SetHeight(15);
        Lister->SetLeft(Dialog->GetBorderBox()->GetLeft() + 1);
        Lister->SetRight(Dialog->GetBorderBox()->GetRight() - (Lister->GetScrollBar() ? 0 : 1));

        new TFarSeparator(Dialog);

        TFarCheckBox *NeverShowAgainCheck = NULL;
        if (FLAGCLEAR(Options, boDisableNeverShowAgain))
        {
            NeverShowAgainCheck = new TFarCheckBox(Dialog);
            NeverShowAgainCheck->SetCaption(GetMsg(BANNER_NEVER_SHOW_AGAIN));
            NeverShowAgainCheck->SetVisible(FLAGCLEAR(Options, boDisableNeverShowAgain));
            NeverShowAgainCheck->SetChecked(NeverShowAgain);

            Dialog->SetNextItemPosition(ipRight);
        }

        TFarButton *Button = new TFarButton(Dialog);
        Button->SetCaption(GetMsg(BANNER_CONTINUE));
        Button->SetDefault(true);
        Button->SetResult(brOK);
        if (NeverShowAgainCheck != NULL)
        {
            Button->SetLeft(Dialog->GetBorderBox()->GetRight() - Button->GetWidth() - 1);
        }
        else
        {
            Button->SetCenterGroup(true);
        }

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            if (NeverShowAgainCheck != NULL)
            {
                NeverShowAgain = NeverShowAgainCheck->GetChecked();
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSessionDialog : public TTabbedDialog
{
public:
    enum TSessionTab { tabSession = 1, tabEnvironment, tabDirectories, tabSFTP, tabSCP, tabFTP,
                       tabConnection, tabTunnel, tabProxy, tabSsh, tabKex, tabAuthentication, tabBugs, tabCount
                     };

    explicit TSessionDialog(TCustomFarPlugin *AFarPlugin, TSessionActionEnum Action);

    bool Execute(TSessionData *Data, TSessionActionEnum &Action);

protected:
    virtual void Change();
    virtual bool CloseQuery();

private:
    TSessionActionEnum FAction;
    TSessionData *FSessionData;
    size_t FTransferProtocolIndex;
    size_t FLoginTypeIndex;

    TTabButton *SshTab;
    TTabButton *AuthenticatonTab;
    TTabButton *KexTab;
    TTabButton *BugsTab;
    TTabButton *ScpTab;
    TTabButton *SftpTab;
    TTabButton *FtpTab;
    TTabButton *TunnelTab;
    TFarButton *ConnectButton;
    TFarEdit *HostNameEdit;
    TFarEdit *PortNumberEdit;
    TFarComboBox *LoginTypeCombo;
    TFarEdit *UserNameEdit;
    TFarEdit *PasswordEdit;
    TFarEdit *PrivateKeyEdit;
    TFarComboBox *TransferProtocolCombo;
    TFarCheckBox *AllowScpFallbackCheck;
    TFarText *HostNameLabel;
    TFarText *InsecureLabel;
    TFarCheckBox *UpdateDirectoriesCheck;
    TFarCheckBox *CacheDirectoriesCheck;
    TFarCheckBox *CacheDirectoryChangesCheck;
    TFarCheckBox *PreserveDirectoryChangesCheck;
    TFarCheckBox *ResolveSymlinksCheck;
    TFarEdit *RemoteDirectoryEdit;
    TFarComboBox *EOLTypeCombo;
    TFarRadioButton *DSTModeWinCheck;
    TFarRadioButton *DSTModeKeepCheck;
    TFarRadioButton *DSTModeUnixCheck;
    TFarCheckBox *CompressionCheck;
    TFarRadioButton *SshProt1onlyButton;
    TFarRadioButton *SshProt1Button;
    TFarRadioButton *SshProt2Button;
    TFarRadioButton *SshProt2onlyButton;
    TFarListBox *CipherListBox;
    TFarButton *CipherUpButton;
    TFarButton *CipherDownButton;
    TFarCheckBox *Ssh2DESCheck;
    TFarComboBox *ShellEdit;
    TFarComboBox *ReturnVarEdit;
    TFarCheckBox *LookupUserGroupsCheck;
    TFarCheckBox *ClearAliasesCheck;
    TFarCheckBox *UnsetNationalVarsCheck;
    TFarComboBox *ListingCommandEdit;
    TFarCheckBox *IgnoreLsWarningsCheck;
    TFarCheckBox *SCPLsFullTimeAutoCheck;
    TFarCheckBox *Scp1CompatibilityCheck;
    TFarEdit *PostLoginCommandsEdits[3];
    TFarEdit *TimeDifferenceEdit;
    TFarEdit *TimeDifferenceMinutesEdit;
    TFarEdit *TimeoutEdit;
    TFarRadioButton *PingOffButton;
    TFarRadioButton *PingNullPacketButton;
    TFarRadioButton *PingDummyCommandButton;
    TFarEdit *PingIntervalSecEdit;
    TFarComboBox *SshProxyMethodCombo;
    TFarComboBox *FtpProxyMethodCombo;
    TFarEdit *ProxyHostEdit;
    TFarEdit *ProxyPortEdit;
    TFarEdit *ProxyUsernameEdit;
    TFarEdit *ProxyPasswordEdit;
    TFarText *ProxyLocalCommandLabel;
    TFarEdit *ProxyLocalCommandEdit;
    TFarText *ProxyTelnetCommandLabel;
    TFarEdit *ProxyTelnetCommandEdit;
    TFarCheckBox *ProxyLocalhostCheck;
    TFarRadioButton *ProxyDNSOffButton;
    TFarRadioButton *ProxyDNSAutoButton;
    TFarRadioButton *ProxyDNSOnButton;
    TFarCheckBox *TunnelCheck;
    TFarEdit *TunnelHostNameEdit;
    TFarEdit *TunnelPortNumberEdit;
    TFarEdit *TunnelUserNameEdit;
    TFarEdit *TunnelPasswordEdit;
    TFarEdit *TunnelPrivateKeyEdit;
    TFarComboBox *TunnelLocalPortNumberEdit;
    TFarComboBox *BugIgnore1Combo;
    TFarComboBox *BugPlainPW1Combo;
    TFarComboBox *BugRSA1Combo;
    TFarComboBox *BugHMAC2Combo;
    TFarComboBox *BugDeriveKey2Combo;
    TFarComboBox *BugRSAPad2Combo;
    TFarComboBox *BugPKSessID2Combo;
    TFarComboBox *BugRekey2Combo;
    TFarCheckBox *SshNoUserAuthCheck;
    TFarCheckBox *AuthTISCheck;
    TFarCheckBox *TryAgentCheck;
    TFarCheckBox *AuthKICheck;
    TFarCheckBox *AuthKIPasswordCheck;
    TFarCheckBox *AgentFwdCheck;
    TFarCheckBox *AuthGSSAPICheck2;
    TFarEdit *GSSAPIServerRealmEdit;
    TFarCheckBox *DeleteToRecycleBinCheck;
    TFarCheckBox *OverwrittenToRecycleBinCheck;
    TFarEdit *RecycleBinPathEdit;
    TFarComboBox *SFTPMaxVersionCombo;
    TFarComboBox *SftpServerEdit;
    TFarComboBox *SFTPBugSymlinkCombo;
    TFarComboBox *SFTPBugSignedTSCombo;
    TFarComboBox *UtfCombo;
    TFarListBox *KexListBox;
    TFarButton *KexUpButton;
    TFarButton *KexDownButton;
    TFarEdit *SFTPMinPacketSizeEdit;
    TFarEdit *SFTPMaxPacketSizeEdit;
    TFarEdit *RekeyTimeEdit;
    TFarEdit *RekeyDataEdit;
    TFarRadioButton *IPAutoButton;
    TFarRadioButton *IPv4Button;
    TFarRadioButton *IPv6Button;
    TFarCheckBox *FtpPasvModeCheck;
    TFarCheckBox *FtpAllowEmptyPasswordCheck;
    TSessionDialog *Self;

    void LoadPing(TSessionData *SessionData);
    void SavePing(TSessionData *SessionData);
    size_t LoginTypeToIndex(TLoginType LoginType);
    size_t FSProtocolToIndex(TFSProtocol FSProtocol, bool &AllowScpFallback);
    TFSProtocol IndexToFSProtocol(size_t Index, bool AllowScpFallback);
    TFSProtocol GetFSProtocol();
    TLoginType IndexToLoginType(size_t Index);
    TLoginType GetLoginType();
    bool VerifyKey(std::wstring FileName, bool TypeOnly);
    void CipherButtonClick(TFarButton *Sender, bool &Close);
    void KexButtonClick(TFarButton *Sender, bool &Close);
    void AuthGSSAPICheckAllowChange(TFarDialogItem *Sender, void *NewState, bool &Allow);
    void UnixEnvironmentButtonClick(TFarButton *Sender, bool &Close);
    void WindowsEnvironmentButtonClick(TFarButton *Sender, bool &Close);
    void UpdateControls();
    void TransferProtocolComboChange();
    void LoginTypeComboChange();
};
//---------------------------------------------------------------------------
#define BUG(BUGID, MSG, PREFIX) \
  TRISTATE(PREFIX ## Bug ## BUGID ## Combo, PREFIX ## Bug(sb ## BUGID), MSG)
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
static const TFSProtocol FSOrder[] = { fsSFTPonly, fsSCPonly, fsFTP, fsFTPS, fsHTTP, fsHTTPS };
//---------------------------------------------------------------------------
TSessionDialog::TSessionDialog(TCustomFarPlugin *AFarPlugin, TSessionActionEnum Action) :
    TTabbedDialog(AFarPlugin, tabCount),
    FAction(Action)
{
    Self = this;
    nb::TPoint S = nb::TPoint(67, 23);
    bool Limited = (S.y > GetMaxSize().y);
    if (Limited)
    {
        S.y = GetMaxSize().y;
    }
    SetSize(S);

#define TRISTATE(COMBO, PROP, MSG) \
    Text = new TFarText(this); \
    Text->SetCaption(GetMsg(MSG)); \
    SetNextItemPosition(ipRight); \
    COMBO = new TFarComboBox(this); \
    COMBO->SetDropDownList(true); \
    COMBO->SetWidth(7); \
    COMBO->GetItems()->BeginUpdate(); \
    { \
        BOOST_SCOPE_EXIT ( (&COMBO) ) \
        { \
          COMBO->GetItems()->EndUpdate(); \
        } BOOST_SCOPE_EXIT_END \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_AUTO)); \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_OFF)); \
      COMBO->GetItems()->Add(GetMsg(LOGIN_BUGS_ON)); \
    } \
    Text->SetEnabledFollow(COMBO); \
    SetNextItemPosition(ipNewLine);

    nb::TRect CRect = GetClientRect();

    TFarButton *Button;
    TTabButton *Tab;
    TFarSeparator *Separator;
    TFarText *Text;
    int GroupTop;
    int Pos;

    TFarButtonBrackets TabBrackets = brNone;

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(LOGIN_TAB_SESSION));
    Tab->SetTab(tabSession);
    Tab->SetBrackets(TabBrackets);

    SetNextItemPosition(ipRight);

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(LOGIN_TAB_ENVIRONMENT));
    Tab->SetTab(tabEnvironment);
    Tab->SetBrackets(TabBrackets);

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(LOGIN_TAB_DIRECTORIES));
    Tab->SetTab(tabDirectories);
    Tab->SetBrackets(TabBrackets);

    SftpTab = new TTabButton(this);
    SftpTab->SetTabName(GetMsg(LOGIN_TAB_SFTP));
    SftpTab->SetTab(tabSFTP);
    SftpTab->SetBrackets(TabBrackets);

    ScpTab = new TTabButton(this);
    ScpTab->SetTabName(GetMsg(LOGIN_TAB_SCP));
    ScpTab->SetTab(tabSCP);
    ScpTab->SetBrackets(TabBrackets);

    FtpTab = new TTabButton(this);
    FtpTab->SetTabName(GetMsg(LOGIN_TAB_FTP));
    FtpTab->SetTab(tabFTP);
    FtpTab->SetBrackets(TabBrackets);

    SetNextItemPosition(ipNewLine);

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(LOGIN_TAB_CONNECTION));
    Tab->SetTab(tabConnection);
    Tab->SetBrackets(TabBrackets);

    SetNextItemPosition(ipRight);

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(LOGIN_TAB_PROXY));
    Tab->SetTab(tabProxy);
    Tab->SetBrackets(TabBrackets);

    TunnelTab = new TTabButton(this);
    TunnelTab->SetTabName(GetMsg(LOGIN_TAB_TUNNEL));
    TunnelTab->SetTab(tabTunnel);
    TunnelTab->SetBrackets(TabBrackets);

    SshTab = new TTabButton(this);
    SshTab->SetTabName(GetMsg(LOGIN_TAB_SSH));
    SshTab->SetTab(tabSsh);
    SshTab->SetBrackets(TabBrackets);

    KexTab = new TTabButton(this);
    KexTab->SetTabName(GetMsg(LOGIN_TAB_KEX));
    KexTab->SetTab(tabKex);
    KexTab->SetBrackets(TabBrackets);

    AuthenticatonTab = new TTabButton(this);
    AuthenticatonTab->SetTabName(GetMsg(LOGIN_TAB_AUTH));
    AuthenticatonTab->SetTab(tabAuthentication);
    AuthenticatonTab->SetBrackets(TabBrackets);

    BugsTab = new TTabButton(this);
    BugsTab->SetTabName(GetMsg(LOGIN_TAB_BUGS));
    BugsTab->SetTab(tabBugs);
    BugsTab->SetBrackets(TabBrackets);

    // Sesion tab

    SetNextItemPosition(ipNewLine);
    SetDefaultGroup(tabSession);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_GROUP_SESSION));
    GroupTop = Separator->GetTop();

    // Separator = new TFarSeparator(this);
    // Separator->SetCaption(GetMsg(LOGIN_GROUP_PROTOCOL));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TRANSFER_PROTOCOL));

    SetNextItemPosition(ipRight);

    TransferProtocolCombo = new TFarComboBox(this);
    TransferProtocolCombo->SetDropDownList(true);
    TransferProtocolCombo->SetWidth(10);
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_SFTP));
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_SCP));
#ifndef NO_FILEZILLA
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_FTP));
#ifndef MPEXT_NO_SSLDLL
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_FTPS));
#endif
#endif
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_HTTP));
    TransferProtocolCombo->GetItems()->Add(GetMsg(LOGIN_HTTPS));

    AllowScpFallbackCheck = new TFarCheckBox(this);
    AllowScpFallbackCheck->SetCaption(GetMsg(LOGIN_ALLOW_SCP_FALLBACK));

    InsecureLabel = new TFarText(this);
    InsecureLabel->SetCaption(GetMsg(LOGIN_INSECURE));
    InsecureLabel->MoveAt(AllowScpFallbackCheck->GetLeft(), AllowScpFallbackCheck->GetTop());

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    HostNameLabel = new TFarText(this);
    HostNameLabel->SetCaption(GetMsg(LOGIN_HOST_NAME));

    HostNameEdit = new TFarEdit(this);
    HostNameEdit->SetRight(CRect.Right - 12 - 2);

    SetNextItemPosition(ipRight);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PORT_NUMBER));
    Text->Move(0, -1);

    SetNextItemPosition(ipBelow);

    PortNumberEdit = new TFarEdit(this);
    PortNumberEdit->SetFixed(true);
    PortNumberEdit->SetMask(L"99999");

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_LOGIN_TYPE));
    Text->SetWidth(20);

    SetNextItemPosition(ipRight);

    LoginTypeCombo = new TFarComboBox(this);
    LoginTypeCombo->SetDropDownList(true);
    LoginTypeCombo->GetItems()->Add(GetMsg(LOGIN_LOGIN_TYPE_ANONYMOUS));
    LoginTypeCombo->GetItems()->Add(GetMsg(LOGIN_LOGIN_TYPE_NORMAL));
    LoginTypeCombo->SetWidth(20);
    LoginTypeCombo->SetRight(CRect.Right - 12 - 2);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_USER_NAME));
    Text->SetWidth(20);

    SetNextItemPosition(ipRight);

    UserNameEdit = new TFarEdit(this);
    UserNameEdit->SetWidth(20);
    UserNameEdit->SetRight(CRect.Right - 12 - 2);

    // SetNextItemPosition(ipRight);
    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PASSWORD));
    Text->SetWidth(20);
    // Text->Move(0, -1);

    // SetNextItemPosition(ipBelow);
    SetNextItemPosition(ipRight);

    PasswordEdit = new TFarEdit(this);
    PasswordEdit->SetPassword(true);
    PasswordEdit->SetWidth(20);
    PasswordEdit->SetRight(CRect.Right - 12 - 2);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PRIVATE_KEY));

    PrivateKeyEdit = new TFarEdit(this);
    Text->SetEnabledFollow(PrivateKeyEdit);

    Separator = new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetTop(CRect.Bottom - 3);
    Text->SetBottom(Text->GetTop());
    Text->SetCaption(GetMsg(LOGIN_TAB_HINT1));
    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TAB_HINT2));

    // Environment tab

    SetDefaultGroup(tabEnvironment);
    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_ENVIRONMENT_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_EOL_TYPE));

    SetNextItemPosition(ipRight);

    EOLTypeCombo = new TFarComboBox(this);
    EOLTypeCombo->SetDropDownList(true);
    EOLTypeCombo->SetWidth(7);
    EOLTypeCombo->GetItems()->Add(L"LF");
    EOLTypeCombo->GetItems()->Add(L"CR/LF");

    SetNextItemPosition(ipNewLine);

    UTF_TRISTATE();

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE));

    SetNextItemPosition(ipRight);

    TimeDifferenceEdit = new TFarEdit(this);
    TimeDifferenceEdit->SetFixed(true);
    TimeDifferenceEdit->SetMask(L"###");
    TimeDifferenceEdit->SetWidth(4);
    Text->SetEnabledFollow(TimeDifferenceEdit);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE_HOURS));
    Text->SetEnabledFollow(TimeDifferenceEdit);

    TimeDifferenceMinutesEdit = new TFarEdit(this);
    TimeDifferenceMinutesEdit->SetFixed(true);
    TimeDifferenceMinutesEdit->SetMask(L"###");
    TimeDifferenceMinutesEdit->SetWidth(4);
    TimeDifferenceMinutesEdit->SetEnabledFollow(TimeDifferenceEdit);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TIME_DIFFERENCE_MINUTES));
    Text->SetEnabledFollow(TimeDifferenceEdit);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_DST_MODE_GROUP));

    DSTModeUnixCheck = new TFarRadioButton(this);
    DSTModeUnixCheck->SetCaption(GetMsg(LOGIN_DST_MODE_UNIX));

    DSTModeWinCheck = new TFarRadioButton(this);
    DSTModeWinCheck->SetCaption(GetMsg(LOGIN_DST_MODE_WIN));
    DSTModeWinCheck->SetEnabledFollow(DSTModeUnixCheck);

    DSTModeKeepCheck = new TFarRadioButton(this);
    DSTModeKeepCheck->SetCaption(GetMsg(LOGIN_DST_MODE_KEEP));
    DSTModeKeepCheck->SetEnabledFollow(DSTModeUnixCheck);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(LOGIN_ENVIRONMENT_UNIX));
    Button->SetOnClick(boost::bind(&TSessionDialog::UnixEnvironmentButtonClick, this, _1, _2));
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(LOGIN_ENVIRONMENT_WINDOWS));
    Button->SetOnClick(boost::bind(&TSessionDialog::WindowsEnvironmentButtonClick, this, _1, _2));
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_GROUP));

    DeleteToRecycleBinCheck = new TFarCheckBox(this);
    DeleteToRecycleBinCheck->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_DELETE));

    OverwrittenToRecycleBinCheck = new TFarCheckBox(this);
    OverwrittenToRecycleBinCheck->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_OVERWRITE));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_RECYCLE_BIN_LABEL));

    RecycleBinPathEdit = new TFarEdit(this);
    Text->SetEnabledFollow(RecycleBinPathEdit);

    SetNextItemPosition(ipNewLine);

    // Directories tab

    SetDefaultGroup(tabDirectories);
    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_DIRECTORIES_GROUP));

    UpdateDirectoriesCheck = new TFarCheckBox(this);
    UpdateDirectoriesCheck->SetCaption(GetMsg(LOGIN_UPDATE_DIRECTORIES));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_REMOTE_DIRECTORY));

    RemoteDirectoryEdit = new TFarEdit(this);
    RemoteDirectoryEdit->SetHistory(REMOTE_DIR_HISTORY);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_DIRECTORY_OPTIONS_GROUP));

    CacheDirectoriesCheck = new TFarCheckBox(this);
    CacheDirectoriesCheck->SetCaption(GetMsg(LOGIN_CACHE_DIRECTORIES));

    CacheDirectoryChangesCheck = new TFarCheckBox(this);
    CacheDirectoryChangesCheck->SetCaption(GetMsg(LOGIN_CACHE_DIRECTORY_CHANGES));

    PreserveDirectoryChangesCheck = new TFarCheckBox(this);
    PreserveDirectoryChangesCheck->SetCaption(GetMsg(LOGIN_PRESERVE_DIRECTORY_CHANGES));
    PreserveDirectoryChangesCheck->SetLeft(PreserveDirectoryChangesCheck->GetLeft() + 4);

    ResolveSymlinksCheck = new TFarCheckBox(this);
    ResolveSymlinksCheck->SetCaption(GetMsg(LOGIN_RESOLVE_SYMLINKS));

    new TFarSeparator(this);

    // SCP Tab

    SetNextItemPosition(ipNewLine);

    SetDefaultGroup(tabSCP);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_SHELL_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SHELL_SHELL));

    SetNextItemPosition(ipRight);

    ShellEdit = new TFarComboBox(this);
    ShellEdit->GetItems()->Add(GetMsg(LOGIN_SHELL_SHELL_DEFAULT));
    ShellEdit->GetItems()->Add(L"/bin/bash");
    ShellEdit->GetItems()->Add(L"/bin/ksh");

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SHELL_RETURN_VAR));

    SetNextItemPosition(ipRight);

    ReturnVarEdit = new TFarComboBox(this);
    ReturnVarEdit->GetItems()->Add(GetMsg(LOGIN_SHELL_RETURN_VAR_AUTODETECT));
    ReturnVarEdit->GetItems()->Add(L"?");
    ReturnVarEdit->GetItems()->Add(L"status");

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_SCP_LS_OPTIONS_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_LISTING_COMMAND));

    SetNextItemPosition(ipRight);

    ListingCommandEdit = new TFarComboBox(this);
    ListingCommandEdit->GetItems()->Add(L"ls -la");
    ListingCommandEdit->GetItems()->Add(L"ls -gla");
    Text->SetEnabledFollow(ListingCommandEdit);

    SetNextItemPosition(ipNewLine);

    IgnoreLsWarningsCheck = new TFarCheckBox(this);
    IgnoreLsWarningsCheck->SetCaption(GetMsg(LOGIN_IGNORE_LS_WARNINGS));

    SetNextItemPosition(ipRight);

    SCPLsFullTimeAutoCheck = new TFarCheckBox(this);
    SCPLsFullTimeAutoCheck->SetCaption(GetMsg(LOGIN_SCP_LS_FULL_TIME_AUTO));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_SCP_OPTIONS));

    LookupUserGroupsCheck = new TFarCheckBox(this);
    LookupUserGroupsCheck->SetCaption(GetMsg(LOGIN_LOOKUP_USER_GROUPS));

    SetNextItemPosition(ipRight);

    UnsetNationalVarsCheck = new TFarCheckBox(this);
    UnsetNationalVarsCheck->SetCaption(GetMsg(LOGIN_CLEAR_NATIONAL_VARS));

    SetNextItemPosition(ipNewLine);

    ClearAliasesCheck = new TFarCheckBox(this);
    ClearAliasesCheck->SetCaption(GetMsg(LOGIN_CLEAR_ALIASES));

    SetNextItemPosition(ipRight);

    Scp1CompatibilityCheck = new TFarCheckBox(this);
    Scp1CompatibilityCheck->SetCaption(GetMsg(LOGIN_SCP1_COMPATIBILITY));

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    // SFTP Tab

    SetNextItemPosition(ipNewLine);

    SetDefaultGroup(tabSFTP);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_SFTP_PROTOCOL_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SFTP_SERVER));
    SetNextItemPosition(ipRight);
    SftpServerEdit = new TFarComboBox(this);
    SftpServerEdit->GetItems()->Add(GetMsg(LOGIN_SFTP_SERVER_DEFAULT));
    SftpServerEdit->GetItems()->Add(L"/bin/sftp-server");
    SftpServerEdit->GetItems()->Add(L"sudo su -c /bin/sftp-server");
    Text->SetEnabledFollow(SftpServerEdit);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SFTP_MAX_VERSION));
    SetNextItemPosition(ipRight);
    SFTPMaxVersionCombo = new TFarComboBox(this);
    SFTPMaxVersionCombo->SetDropDownList(true);
    SFTPMaxVersionCombo->SetWidth(7);
    for (int i = 0; i <= 5; i++)
    {
        SFTPMaxVersionCombo->GetItems()->Add(IntToStr(i));
    }
    Text->SetEnabledFollow(SFTPMaxVersionCombo);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_SFTP_BUGS_GROUP));

    SFTP_BUGS();

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SFTP_MIN_PACKET_SIZE));
    SetNextItemPosition(ipRight);

    SFTPMinPacketSizeEdit = new TFarEdit(this);
    SFTPMinPacketSizeEdit->SetFixed(true);
    SFTPMinPacketSizeEdit->SetMask(L"99999999");
    SFTPMinPacketSizeEdit->SetWidth(8);
    // SFTPMinPacketSizeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_SFTP_MAX_PACKET_SIZE));
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
    Separator->SetCaption(GetMsg(LOGIN_FTP_GROUP));

    FtpAllowEmptyPasswordCheck = new TFarCheckBox(this);
    FtpAllowEmptyPasswordCheck->SetCaption(GetMsg(LOGIN_FTP_ALLOW_EMPTY_PASSWORD));

    Separator = new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_FTP_POST_LOGIN_COMMANDS));

    for (size_t Index = 0; Index < LENOF(PostLoginCommandsEdits); Index++)
    {
        TFarEdit *Edit = new TFarEdit(this);
        PostLoginCommandsEdits[Index] = Edit;
    }

    new TFarSeparator(this);

    // Connection tab

    SetNextItemPosition(ipNewLine);

    SetDefaultGroup(tabConnection);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_CONNECTION_GROUP));

    FtpPasvModeCheck = new TFarCheckBox(this);
    FtpPasvModeCheck->SetCaption(GetMsg(LOGIN_FTP_PASV_MODE));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_TIMEOUTS_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TIMEOUT));

    SetNextItemPosition(ipRight);

    TimeoutEdit = new TFarEdit(this);
    TimeoutEdit->SetFixed(true);
    TimeoutEdit->SetMask(L"####");
    TimeoutEdit->SetWidth(5);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TIMEOUT_SECONDS));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_PING_GROUP));

    PingOffButton = new TFarRadioButton(this);
    PingOffButton->SetCaption(GetMsg(LOGIN_PING_OFF));

    PingNullPacketButton = new TFarRadioButton(this);
    PingNullPacketButton->SetCaption(GetMsg(LOGIN_PING_NULL_PACKET));

    PingDummyCommandButton = new TFarRadioButton(this);
    PingDummyCommandButton->SetCaption(GetMsg(LOGIN_PING_DUMMY_COMMAND));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PING_INTERVAL));
    Text->SetEnabledDependencyNegative(PingOffButton);

    SetNextItemPosition(ipRight);

    PingIntervalSecEdit = new TFarEdit(this);
    PingIntervalSecEdit->SetFixed(true);
    PingIntervalSecEdit->SetMask(L"####");
    PingIntervalSecEdit->SetWidth(6);
    PingIntervalSecEdit->SetEnabledDependencyNegative(PingOffButton);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_IP_GROUP));

    IPAutoButton = new TFarRadioButton(this);
    IPAutoButton->SetCaption(GetMsg(LOGIN_IP_AUTO));

    SetNextItemPosition(ipRight);

    IPv4Button = new TFarRadioButton(this);
    IPv4Button->SetCaption(GetMsg(LOGIN_IP_V4));

    IPv6Button = new TFarRadioButton(this);
    IPv6Button->SetCaption(GetMsg(LOGIN_IP_V6));

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    // Proxy tab

    SetDefaultGroup(tabProxy);
    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_PROXY_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_METHOD));

    SetNextItemPosition(ipRight);

    FtpProxyMethodCombo = new TFarComboBox(this);
    FtpProxyMethodCombo->SetDropDownList(true);
    FtpProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_NONE));
    FtpProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_SOCKS4));
    FtpProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_SOCKS5));
    FtpProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_HTTP));
    FtpProxyMethodCombo->SetRight(CRect.Right - 12 - 2);

    SshProxyMethodCombo = new TFarComboBox(this);
    SshProxyMethodCombo->SetLeft(FtpProxyMethodCombo->GetLeft());
    SshProxyMethodCombo->SetWidth(FtpProxyMethodCombo->GetWidth());
    FtpProxyMethodCombo->SetRight(FtpProxyMethodCombo->GetRight());
    SshProxyMethodCombo->SetDropDownList(true);
    SshProxyMethodCombo->GetItems()->AddStrings(FtpProxyMethodCombo->GetItems());
    SshProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_TELNET));
    SshProxyMethodCombo->GetItems()->Add(GetMsg(LOGIN_PROXY_LOCAL));

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_HOST));

    ProxyHostEdit = new TFarEdit(this);
    ProxyHostEdit->SetRight(FtpProxyMethodCombo->GetRight());
    Text->SetEnabledFollow(ProxyHostEdit);

    SetNextItemPosition(ipRight);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_PORT));
    Text->Move(0, -1);

    SetNextItemPosition(ipBelow);

    ProxyPortEdit = new TFarEdit(this);
    ProxyPortEdit->SetFixed(true);
    ProxyPortEdit->SetMask(L"99999");
    Text->SetEnabledFollow(ProxyPortEdit);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_USERNAME));

    ProxyUsernameEdit = new TFarEdit(this);
    ProxyUsernameEdit->SetWidth(ProxyUsernameEdit->GetWidth() / 2 - 1);
    Text->SetEnabledFollow(ProxyUsernameEdit);

    SetNextItemPosition(ipRight);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_PASSWORD));
    Text->Move(0, -1);

    SetNextItemPosition(ipBelow);

    ProxyPasswordEdit = new TFarEdit(this);
    ProxyPasswordEdit->SetPassword(true);
    Text->SetEnabledFollow(ProxyPasswordEdit);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_PROXY_SETTINGS_GROUP));

    ProxyTelnetCommandLabel = new TFarText(this);
    ProxyTelnetCommandLabel->SetCaption(GetMsg(LOGIN_PROXY_TELNET_COMMAND));

    SetNextItemPosition(ipRight);

    ProxyTelnetCommandEdit = new TFarEdit(this);
    ProxyTelnetCommandLabel->SetEnabledFollow(ProxyTelnetCommandEdit);

    SetNextItemPosition(ipNewLine);

    ProxyLocalCommandLabel = new TFarText(this);
    ProxyLocalCommandLabel->SetCaption(GetMsg(LOGIN_PROXY_LOCAL_COMMAND));
    ProxyLocalCommandLabel->Move(0, -1);

    SetNextItemPosition(ipRight);

    ProxyLocalCommandEdit = new TFarEdit(this);
    ProxyLocalCommandLabel->SetEnabledFollow(ProxyLocalCommandEdit);

    SetNextItemPosition(ipNewLine);

    ProxyLocalhostCheck = new TFarCheckBox(this);
    ProxyLocalhostCheck->SetCaption(GetMsg(LOGIN_PROXY_LOCALHOST));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PROXY_DNS));

    ProxyDNSOffButton = new TFarRadioButton(this);
    ProxyDNSOffButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_NO));
    Text->SetEnabledFollow(ProxyDNSOffButton);

    SetNextItemPosition(ipRight);

    ProxyDNSAutoButton = new TFarRadioButton(this);
    ProxyDNSAutoButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_AUTO));
    ProxyDNSAutoButton->SetEnabledFollow(ProxyDNSOffButton);

    ProxyDNSOnButton = new TFarRadioButton(this);
    ProxyDNSOnButton->SetCaption(GetMsg(LOGIN_PROXY_DNS_YES));
    ProxyDNSOnButton->SetEnabledFollow(ProxyDNSOffButton);

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    // Tunnel tab

    SetNextItemPosition(ipNewLine);

    SetDefaultGroup(tabTunnel);
    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_TUNNEL_GROUP));

    TunnelCheck = new TFarCheckBox(this);
    TunnelCheck->SetCaption(GetMsg(LOGIN_TUNNEL_TUNNEL));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_TUNNEL_SESSION_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_HOST_NAME));
    Text->SetEnabledDependency(TunnelCheck);

    TunnelHostNameEdit = new TFarEdit(this);
    TunnelHostNameEdit->SetRight(CRect.Right - 12 - 2);
    TunnelHostNameEdit->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipRight);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PORT_NUMBER));
    Text->Move(0, -1);
    Text->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipBelow);

    TunnelPortNumberEdit = new TFarEdit(this);
    TunnelPortNumberEdit->SetFixed(true);
    TunnelPortNumberEdit->SetMask(L"99999");
    TunnelPortNumberEdit->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_USER_NAME));
    Text->SetEnabledDependency(TunnelCheck);

    TunnelUserNameEdit = new TFarEdit(this);
    TunnelUserNameEdit->SetWidth(TunnelUserNameEdit->GetWidth() / 2 - 1);
    TunnelUserNameEdit->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipRight);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PASSWORD));
    Text->Move(0, -1);
    Text->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipBelow);

    TunnelPasswordEdit = new TFarEdit(this);
    TunnelPasswordEdit->SetPassword(true);
    TunnelPasswordEdit->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_PRIVATE_KEY));
    Text->SetEnabledDependency(TunnelCheck);

    TunnelPrivateKeyEdit = new TFarEdit(this);
    TunnelPrivateKeyEdit->SetEnabledDependency(TunnelCheck);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_TUNNEL_OPTIONS_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER));
    Text->SetEnabledDependency(TunnelCheck);

    SetNextItemPosition(ipRight);

    TunnelLocalPortNumberEdit = new TFarComboBox(this);
    TunnelLocalPortNumberEdit->SetLeft(TunnelPortNumberEdit->GetLeft());
    TunnelLocalPortNumberEdit->SetEnabledDependency(TunnelCheck);
    TunnelLocalPortNumberEdit->GetItems()->BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->TunnelLocalPortNumberEdit->GetItems()->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        TunnelLocalPortNumberEdit->GetItems()->Add(GetMsg(LOGIN_TUNNEL_LOCAL_PORT_NUMBER_AUTOASSIGN));
        for (size_t Index = Configuration->GetTunnelLocalPortNumberLow();
                Index <= Configuration->GetTunnelLocalPortNumberHigh(); Index++)
        {
            TunnelLocalPortNumberEdit->GetItems()->Add(IntToStr(static_cast<int>(Index)));
        }
    }

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    // SSH tab

    SetNextItemPosition(ipNewLine);

    SetDefaultGroup(tabSsh);
    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_SSH_GROUP));

    CompressionCheck = new TFarCheckBox(this);
    CompressionCheck->SetCaption(GetMsg(LOGIN_COMPRESSION));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_SSH_PROTOCOL_GROUP));

    SshProt1onlyButton = new TFarRadioButton(this);
    SshProt1onlyButton->SetCaption(GetMsg(LOGIN_SSH1_ONLY));

    SetNextItemPosition(ipRight);

    SshProt1Button = new TFarRadioButton(this);
    SshProt1Button->SetCaption(GetMsg(LOGIN_SSH1));

    SshProt2Button = new TFarRadioButton(this);
    SshProt2Button->SetCaption(GetMsg(LOGIN_SSH2));

    SshProt2onlyButton = new TFarRadioButton(this);
    SshProt2onlyButton->SetCaption(GetMsg(LOGIN_SSH2_ONLY));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_ENCRYPTION_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_CIPHER));

    CipherListBox = new TFarListBox(this);
    CipherListBox->SetRight(CipherListBox->GetRight() - 15);
    CipherListBox->SetHeight(1 + CIPHER_COUNT + 1);
    Pos = CipherListBox->GetBottom();

    SetNextItemPosition(ipRight);

    CipherUpButton = new TFarButton(this);
    CipherUpButton->SetCaption(GetMsg(LOGIN_UP));
    CipherUpButton->Move(0, 1);
    CipherUpButton->SetResult(-1);
    CipherUpButton->SetOnClick(boost::bind(&TSessionDialog::CipherButtonClick, this, _1, _2));

    SetNextItemPosition(ipBelow);

    CipherDownButton = new TFarButton(this);
    CipherDownButton->SetCaption(GetMsg(LOGIN_DOWN));
    CipherDownButton->SetResult(1);
    CipherDownButton->SetOnClick(boost::bind(&TSessionDialog::CipherButtonClick, this, _1, _2));

    SetNextItemPosition(ipNewLine);

    if (!Limited)
    {
        Ssh2DESCheck = new TFarCheckBox(this);
        Ssh2DESCheck->Move(0, Pos - Ssh2DESCheck->GetTop() + 1);
        Ssh2DESCheck->SetCaption(GetMsg(LOGIN_SSH2DES));
        Ssh2DESCheck->SetEnabledDependencyNegative(SshProt1onlyButton);
    }
    else
    {
        Ssh2DESCheck = NULL;
    }

    // KEX tab

    SetDefaultGroup(tabKex);
    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_KEX_REEXCHANGE_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_KEX_REKEY_TIME));
    Text->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipRight);

    RekeyTimeEdit = new TFarEdit(this);
    RekeyTimeEdit->SetFixed(true);
    RekeyTimeEdit->SetMask(L"####");
    RekeyTimeEdit->SetWidth(6);
    RekeyTimeEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipNewLine);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_KEX_REKEY_DATA));
    Text->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipRight);

    RekeyDataEdit = new TFarEdit(this);
    RekeyDataEdit->SetWidth(6);
    RekeyDataEdit->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_KEX_OPTIONS_GROUP));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_KEX_LIST));
    Text->SetEnabledDependencyNegative(SshProt1onlyButton);

    KexListBox = new TFarListBox(this);
    KexListBox->SetRight(KexListBox->GetRight() - 15);
    KexListBox->SetHeight(1 + KEX_COUNT + 1);
    KexListBox->SetEnabledDependencyNegative(SshProt1onlyButton);

    SetNextItemPosition(ipRight);

    KexUpButton = new TFarButton(this);
    KexUpButton->SetCaption(GetMsg(LOGIN_UP));
    KexUpButton->Move(0, 1);
    KexUpButton->SetResult(-1);
    KexUpButton->SetOnClick(boost::bind(&TSessionDialog::KexButtonClick, this, _1, _2));

    SetNextItemPosition(ipBelow);

    KexDownButton = new TFarButton(this);
    KexDownButton->SetCaption(GetMsg(LOGIN_DOWN));
    KexDownButton->SetResult(1);
    KexDownButton->SetOnClick(boost::bind(&TSessionDialog::KexButtonClick, this, _1, _2));

    SetNextItemPosition(ipNewLine);

    // Authentication tab

    SetDefaultGroup(tabAuthentication);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);

    SshNoUserAuthCheck = new TFarCheckBox(this);
    SshNoUserAuthCheck->SetCaption(GetMsg(LOGIN_AUTH_SSH_NO_USER_AUTH));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_AUTH_GROUP));

    TryAgentCheck = new TFarCheckBox(this);
    TryAgentCheck->SetCaption(GetMsg(LOGIN_AUTH_TRY_AGENT));

    AuthTISCheck = new TFarCheckBox(this);
    AuthTISCheck->SetCaption(GetMsg(LOGIN_AUTH_TIS));

    AuthKICheck = new TFarCheckBox(this);
    AuthKICheck->SetCaption(GetMsg(LOGIN_AUTH_KI));

    AuthKIPasswordCheck = new TFarCheckBox(this);
    AuthKIPasswordCheck->SetCaption(GetMsg(LOGIN_AUTH_KI_PASSWORD));
    AuthKIPasswordCheck->Move(4, 0);

    AuthGSSAPICheck2 = new TFarCheckBox(this);
    AuthGSSAPICheck2->SetCaption(GetMsg(LOGIN_AUTH_GSSAPI));
    AuthGSSAPICheck2->SetOnAllowChange(boost::bind(&TSessionDialog::AuthGSSAPICheckAllowChange, this, _1, _2, _3));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(LOGIN_AUTH_PARAMS_GROUP));

    AgentFwdCheck = new TFarCheckBox(this);
    AgentFwdCheck->SetCaption(GetMsg(LOGIN_AUTH_AGENT_FWD));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(LOGIN_AUTH_GSSAPI_SERVER_REALM));
    GSSAPIServerRealmEdit = new TFarEdit(this);
    Text->SetEnabledFollow(GSSAPIServerRealmEdit);

    new TFarSeparator(this);

    // Bugs tab

    SetDefaultGroup(tabBugs);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GroupTop);
    Separator->SetCaption(GetMsg(LOGIN_BUGS_GROUP));

    BUGS();

    BugIgnore1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
    BugPlainPW1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
    BugRSA1Combo->SetEnabledDependencyNegative(SshProt2onlyButton);
    BugHMAC2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
    BugDeriveKey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
    BugRSAPad2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
    BugPKSessID2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);
    BugRekey2Combo->SetEnabledDependencyNegative(SshProt1onlyButton);

#undef TRISTATE

    new TFarSeparator(this);

    // Buttons

    SetNextItemPosition(ipNewLine);
    SetDefaultGroup(0);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(CRect.Bottom - 1);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_OK));
    Button->SetDefault((Action != saConnect));
    Button->SetResult(brOK);
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    ConnectButton = new TFarButton(this);
    ConnectButton->SetCaption(GetMsg(LOGIN_CONNECT_BUTTON));
    ConnectButton->SetDefault((Action == saConnect));
    ConnectButton->SetResult(brConnect);
    ConnectButton->SetCenterGroup(true);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
void TSessionDialog::Change()
{
    TTabbedDialog::Change();

    if (GetHandle() && !ChangesLocked())
    {
        if (FTransferProtocolIndex != TransferProtocolCombo->GetItems()->GetSelected())
        {
            TransferProtocolComboChange();
        }
        if (FLoginTypeIndex != LoginTypeCombo->GetItems()->GetSelected())
        {
            LoginTypeComboChange();
        }

        LockChanges();
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->UnlockChanges();
            } BOOST_SCOPE_EXIT_END
            UpdateControls();
        }
    }
}
//---------------------------------------------------------------------------
void AdjustRemoteDir(TFarEdit *HostNameEdit,
                     TFarEdit *RemoteDirectoryEdit,
                     TFarCheckBox *UpdateDirectoriesCheck)
{
    std::wstring hostName = HostNameEdit->GetText();
    if (LowerCase(hostName.substr(0, 7)) == L"http://")
    {
        hostName.erase(0, 7);
    }
    else if (LowerCase(hostName.substr(0, 7)) == L"https://")
    {
        hostName.erase(0, 8);
    }
    std::wstring dir;
    size_t P = hostName.find_first_of(L'/');
    if (P != std::wstring::npos)
    {
        dir = hostName.substr(P, hostName.size() - P);
        hostName.resize(hostName.size() - dir.size());
    }
    std::wstring remotedir = RemoteDirectoryEdit->GetText();
    if (remotedir.empty() && !dir.empty())
    {
        // UpdateDirectoriesCheck->SetChecked(true);
        RemoteDirectoryEdit->SetText(dir);
        HostNameEdit->SetText(hostName);
    }
}
//---------------------------------------------------------------------------
void TSessionDialog::TransferProtocolComboChange()
{
    // note that this modifies the session for good,
    // even if user cancels the dialog
    SavePing(FSessionData);

    FTransferProtocolIndex = TransferProtocolCombo->GetItems()->GetSelected();

    LoadPing(FSessionData);
    // DEBUG_PRINTF(L"GetFSProtocol = %d, PortNumberEdit->GetAsInteger = %d", GetFSProtocol(), PortNumberEdit->GetAsInteger());
    if (GetFSProtocol() == fsSFTPonly || GetFSProtocol() == fsSCPonly)
    {
        if (PortNumberEdit->GetAsInteger() == 21)
        {
            PortNumberEdit->SetAsInteger(22);
        }
    }
    else if (GetFSProtocol() == fsFTP)
    {
        if (PortNumberEdit->GetAsInteger() == 22 || PortNumberEdit->GetAsInteger() == 990)
        {
            PortNumberEdit->SetAsInteger(21);
        }
    }
    else if (GetFSProtocol() == fsFTPS)
    {
        if (PortNumberEdit->GetAsInteger() == 21 || PortNumberEdit->GetAsInteger() == 80)
        {
            PortNumberEdit->SetAsInteger(990);
        }
    }
    else if (GetFSProtocol() == fsHTTP)
    {
        if (PortNumberEdit->GetAsInteger() == 990 || PortNumberEdit->GetAsInteger() == 443)
        {
            PortNumberEdit->SetAsInteger(80);
            ::AdjustRemoteDir(HostNameEdit, RemoteDirectoryEdit, UpdateDirectoriesCheck);
        }
    }
    else if (GetFSProtocol() == fsHTTPS)
    {
        if (PortNumberEdit->GetAsInteger() == 80)
        {
            PortNumberEdit->SetAsInteger(443);
            ::AdjustRemoteDir(HostNameEdit, RemoteDirectoryEdit, UpdateDirectoriesCheck);
        }
    }
}
//---------------------------------------------------------------------------
void TSessionDialog::LoginTypeComboChange()
{
    FLoginTypeIndex = LoginTypeCombo->GetItems()->GetSelected();
    if (GetLoginType() == ltAnonymous)
    {
        UserNameEdit->SetText(CONST_LOGIN_ANONYMOUS);
        PasswordEdit->SetText(L"");
    }
    else if (GetLoginType() == ltNormal)
    {
    }
}
//---------------------------------------------------------------------------
void TSessionDialog::UpdateControls()
{
    TFSProtocol FSProtocol = GetFSProtocol();
    bool InternalSshProtocol =
        (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP) || (FSProtocol == fsSCPonly);
    bool InternalHTTPProtocol =
        (FSProtocol == fsHTTP) || (FSProtocol == fsHTTPS);
    bool SshProtocol = InternalSshProtocol;
    bool HTTPProtocol = FSProtocol == fsHTTP;
    bool HTTPSProtocol = FSProtocol == fsHTTPS;
    bool SftpProtocol = (FSProtocol == fsSFTPonly) || (FSProtocol == fsSFTP);
    bool ScpOnlyProtocol = (FSProtocol == fsSCPonly);
    bool FtpProtocol = ((FSProtocol == fsFTP) || (FSProtocol == fsFTPS));
    bool FtpsProtocol = (FSProtocol == fsFTPS);
    bool LoginAnonymous = (GetLoginType() == ltAnonymous);

    ConnectButton->SetEnabled(!HostNameEdit->GetIsEmpty());

    // Basic tab
    AllowScpFallbackCheck->SetVisible(
        TransferProtocolCombo->GetVisible() &&
        (IndexToFSProtocol(TransferProtocolCombo->GetItems()->GetSelected(), false) == fsSFTPonly));
    InsecureLabel->SetVisible(TransferProtocolCombo->GetVisible() && !SshProtocol && !FtpsProtocol && !HTTPSProtocol);
    PrivateKeyEdit->SetEnabled(SshProtocol);
    HostNameLabel->SetCaption(GetMsg(LOGIN_HOST_NAME));

    UserNameEdit->SetEnabled(!LoginAnonymous);
    PasswordEdit->SetEnabled(!LoginAnonymous);

    // Connection sheet
    FtpPasvModeCheck->SetEnabled(FtpProtocol);
    FtpAllowEmptyPasswordCheck->SetEnabled(FtpProtocol);
    if (FtpProtocol && (FtpProxyMethodCombo->GetItems()->GetSelected() != pmNone) && !FtpPasvModeCheck->GetChecked())
    {
        FtpPasvModeCheck->SetChecked(true);
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
        WinSCPPlugin->MoreMessageDialog(GetMsg(FTP_PASV_MODE_REQUIRED),
                                        NULL, qtInformation, qaOK);
    }
    PingNullPacketButton->SetEnabled(SshProtocol);
    IPAutoButton->SetEnabled(SshProtocol);

    // SFTP tab
    SftpTab->SetEnabled(SftpProtocol);

    // FTP tab
    FtpTab->SetEnabled(FtpProtocol);

    // SSH tab
    SshTab->SetEnabled(SshProtocol);
    CipherUpButton->SetEnabled(CipherListBox->GetItems()->GetSelected() != 0);
    CipherDownButton->SetEnabled(
        CipherListBox->GetItems()->GetSelected() < CipherListBox->GetItems()->GetCount() - 1);

    // Authentication tab
    AuthenticatonTab->SetEnabled(SshProtocol);
    SshNoUserAuthCheck->SetEnabled(!SshProt1onlyButton->GetChecked());
    bool Authentication = !SshNoUserAuthCheck->GetEnabled() || !SshNoUserAuthCheck->GetChecked();
    TryAgentCheck->SetEnabled(Authentication);
    AuthTISCheck->SetEnabled(Authentication && !SshProt2onlyButton->GetChecked());
    AuthKICheck->SetEnabled(Authentication && !SshProt1onlyButton->GetChecked());
    AuthKIPasswordCheck->SetEnabled(
        Authentication &&
        ((AuthTISCheck->GetEnabled() && AuthTISCheck->GetChecked()) ||
         (AuthKICheck->GetEnabled() && AuthKICheck->GetChecked())));
    AuthGSSAPICheck2->SetEnabled(
        Authentication && !SshProt1onlyButton->GetChecked());
    GSSAPIServerRealmEdit->SetEnabled(
        AuthGSSAPICheck2->GetEnabled() && AuthGSSAPICheck2->GetChecked());

    // Directories tab
    CacheDirectoryChangesCheck->SetEnabled(
        (FSProtocol != fsSCPonly) || CacheDirectoriesCheck->GetChecked());
    PreserveDirectoryChangesCheck->SetEnabled(
        CacheDirectoryChangesCheck->GetIsEnabled() && CacheDirectoryChangesCheck->GetChecked());
    ResolveSymlinksCheck->SetEnabled(!FtpProtocol);

    // Environment tab
    DSTModeUnixCheck->SetEnabled(!FtpProtocol);
    // UtfCombo->SetEnabled(FSProtocol != fsSCPonly);
    TimeDifferenceEdit->SetEnabled((FtpProtocol || (FSProtocol == fsSCPonly)));

    // Recycle bin tab
    OverwrittenToRecycleBinCheck->SetEnabled((FSProtocol != fsSCPonly) &&
            !FtpProtocol);
    RecycleBinPathEdit->SetEnabled(
        (DeleteToRecycleBinCheck->GetIsEnabled() && DeleteToRecycleBinCheck->GetChecked()) ||
        (OverwrittenToRecycleBinCheck->GetIsEnabled() && OverwrittenToRecycleBinCheck->GetChecked()));

    // Kex tab
    KexTab->SetEnabled(SshProtocol && !SshProt1onlyButton->GetChecked() &&
                       (BugRekey2Combo->GetItems()->GetSelected() != 2));
    KexUpButton->SetEnabled((KexListBox->GetItems()->GetSelected() > 0));
    KexDownButton->SetEnabled(
        (KexListBox->GetItems()->GetSelected() < KexListBox->GetItems()->GetCount() - 1));

    // Bugs tab
    BugsTab->SetEnabled(SshProtocol);

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
    TFarComboBox *ProxyMethodCombo = (SshProtocol ? SshProxyMethodCombo : FtpProxyMethodCombo);
    ProxyMethodCombo->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()));
    TFarComboBox *OtherProxyMethodCombo = (!SshProtocol ? SshProxyMethodCombo : FtpProxyMethodCombo);
    OtherProxyMethodCombo->SetVisible(false);
    if (ProxyMethodCombo->GetItems()->GetSelected() >= OtherProxyMethodCombo->GetItems()->GetCount())
    {
        OtherProxyMethodCombo->GetItems()->SetSelected(pmNone);
    }
    else
    {
        OtherProxyMethodCombo->GetItems()->SetSelected(ProxyMethodCombo->GetItems()->GetSelected());
    }

    bool Proxy = (ProxyMethodCombo->GetItems()->GetSelected() != pmNone);
    std::wstring ProxyCommand =
        ((ProxyMethodCombo->GetItems()->GetSelected() == pmCmd) ?
         ProxyLocalCommandEdit->GetText() : ProxyTelnetCommandEdit->GetText());
    ProxyHostEdit->SetEnabled(Proxy &&
                              ((ProxyMethodCombo->GetItems()->GetSelected() != pmCmd) ||
                               AnsiContainsText(ProxyCommand, L"%proxyhost")));
    ProxyPortEdit->SetEnabled(Proxy &&
                              ((ProxyMethodCombo->GetItems()->GetSelected() != pmCmd) ||
                               AnsiContainsText(ProxyCommand, L"%proxyport")));
    ProxyUsernameEdit->SetEnabled(Proxy &&
                                  // FZAPI does not support username for SOCKS4
                                  (((ProxyMethodCombo->GetItems()->GetSelected() == pmSocks4) && SshProtocol) ||
                                   (ProxyMethodCombo->GetItems()->GetSelected() == pmSocks5) ||
                                   (ProxyMethodCombo->GetItems()->GetSelected() == pmHTTP) ||
                                   (((ProxyMethodCombo->GetItems()->GetSelected() == pmTelnet) ||
                                     (ProxyMethodCombo->GetItems()->GetSelected() == pmCmd)) &&
                                    AnsiContainsText(ProxyCommand, L"%user"))));
    ProxyPasswordEdit->SetEnabled(Proxy &&
                                  ((ProxyMethodCombo->GetItems()->GetSelected() == pmSocks5) ||
                                   (ProxyMethodCombo->GetItems()->GetSelected() == pmHTTP) ||
                                   (((ProxyMethodCombo->GetItems()->GetSelected() == pmTelnet) ||
                                     (ProxyMethodCombo->GetItems()->GetSelected() == pmCmd)) &&
                                    AnsiContainsText(ProxyCommand, L"%pass"))));
    bool ProxySettings = Proxy && SshProtocol;
    ProxyTelnetCommandEdit->SetEnabled(ProxySettings && (ProxyMethodCombo->GetItems()->GetSelected() == pmTelnet));
    ProxyLocalCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethodCombo->GetItems()->GetSelected() == pmCmd));
    ProxyLocalCommandLabel->SetVisible(ProxyLocalCommandEdit->GetVisible());
    ProxyTelnetCommandEdit->SetVisible((GetTab() == ProxyMethodCombo->GetGroup()) && (ProxyMethodCombo->GetItems()->GetSelected() != pmCmd));
    ProxyTelnetCommandLabel->SetVisible(ProxyTelnetCommandEdit->GetVisible());
    ProxyLocalhostCheck->SetEnabled(ProxySettings);
    ProxyDNSOffButton->SetEnabled(ProxySettings);

    // Tunnel tab
    TunnelTab->SetEnabled(InternalSshProtocol);
}
//---------------------------------------------------------------------------
bool TSessionDialog::Execute(TSessionData *SessionData, TSessionActionEnum &Action)
{
    int Captions[] = { LOGIN_ADD, LOGIN_EDIT, LOGIN_CONNECT };
    SetCaption(GetMsg(Captions[Action]));

    FSessionData = SessionData;
    FTransferProtocolIndex = TransferProtocolCombo->GetItems()->GetSelected();
    FLoginTypeIndex = LoginTypeCombo->GetItems()->GetSelected();

    HideTabs();
    SelectTab(tabSession);

    // load session data

    // Basic tab
    HostNameEdit->SetText(SessionData->GetHostName());
    PortNumberEdit->SetAsInteger(SessionData->GetPortNumber());

    LoginTypeCombo->GetItems()->SetSelected(
        LoginTypeToIndex(SessionData->GetLoginType()));

    UserNameEdit->SetText(SessionData->GetUserName());
    PasswordEdit->SetText(SessionData->GetPassword());
    PrivateKeyEdit->SetText(SessionData->GetPublicKeyFile());

    if ((GetLoginType() == ltAnonymous))
    {
        LoginTypeCombo->GetItems()->SetSelected(0);
        UserNameEdit->SetText(CONST_LOGIN_ANONYMOUS);
        PasswordEdit->SetText(L"");
    }
    else
    {
        LoginTypeCombo->GetItems()->SetSelected(1);
    }

    bool AllowScpFallback;
    TransferProtocolCombo->GetItems()->SetSelected(
        FSProtocolToIndex(SessionData->GetFSProtocol(), AllowScpFallback));
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
        EOLTypeCombo->GetItems()->SetSelected(0);
    }
    else
    {
        EOLTypeCombo->GetItems()->SetSelected(1);
    }
    switch (SessionData->GetNotUtf())
    {
    case asOn:
        UtfCombo->GetItems()->SetSelected(1);
        break;

    case asOff:
        UtfCombo->GetItems()->SetSelected(2);
        break;

    default:
        UtfCombo->GetItems()->SetSelected(0);
        break;
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
    LookupUserGroupsCheck->SetChecked(SessionData->GetLookupUserGroups());
    ClearAliasesCheck->SetChecked(SessionData->GetClearAliases());
    IgnoreLsWarningsCheck->SetChecked(SessionData->GetIgnoreLsWarnings());
    Scp1CompatibilityCheck->SetChecked(SessionData->GetScp1Compatibility());
    UnsetNationalVarsCheck->SetChecked(SessionData->GetUnsetNationalVars());
    ListingCommandEdit->SetText(SessionData->GetListingCommand());
    SCPLsFullTimeAutoCheck->SetChecked((SessionData->GetSCPLsFullTime() != asOff));
    int TimeDifferenceMin = DateTimeToTimeStamp(SessionData->GetTimeDifference()).Time / 60000;
    if (static_cast<double>(SessionData->GetTimeDifference()) < 0)
    {
        TimeDifferenceMin = -TimeDifferenceMin;
    }
    TimeDifferenceEdit->SetAsInteger(TimeDifferenceMin / 60);
    TimeDifferenceMinutesEdit->SetAsInteger(TimeDifferenceMin % 60);

    // SFTP tab

#define TRISTATE(COMBO, PROP, MSG) \
    COMBO->GetItems()->SetSelected(2 - SessionData->Get ## PROP)
    SFTP_BUGS();

    // DEBUG_PRINTF(L"SessionData->GetSftpServer = %s", SessionData->GetSftpServer().c_str());
    // DEBUG_PRINTF(L"SftpServerEdit->GetText = %s", SftpServerEdit->GetText().c_str());
    if (SessionData->GetSftpServer().empty())
    {
        // DEBUG_PRINTF(L"SftpServerEdit->GetItems()->GetString(0) = %s", SftpServerEdit->GetItems()->GetString(0).c_str());
        SftpServerEdit->SetText(SftpServerEdit->GetItems()->GetString(0));
    }
    else
    {
        SftpServerEdit->SetText(SessionData->GetSftpServer());
    }
    SFTPMaxVersionCombo->GetItems()->SetSelected(SessionData->GetSFTPMaxVersion());
    SFTPMinPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMinPacketSize());
    SFTPMaxPacketSizeEdit->SetAsInteger(SessionData->GetSFTPMaxPacketSize());

    // FTP tab
    FtpAllowEmptyPasswordCheck->SetChecked(SessionData->GetFtpAllowEmptyPassword());
    nb::TStrings *PostLoginCommands = new nb::TStringList();
    {
        BOOST_SCOPE_EXIT ( (&PostLoginCommands) )
        {
            delete PostLoginCommands;
        } BOOST_SCOPE_EXIT_END
        PostLoginCommands->SetText(SessionData->GetPostLoginCommands());
        for (size_t Index = 0; (Index < PostLoginCommands->GetCount()) &&
                (Index < LENOF(PostLoginCommandsEdits)); Index++)
        {
            PostLoginCommandsEdits[Index]->SetText(PostLoginCommands->GetString(Index));
        }
    }

    // Connection tab
    FtpPasvModeCheck->SetChecked(SessionData->GetFtpPasvMode());
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

    // Proxy tab
    SshProxyMethodCombo->GetItems()->SetSelected(SessionData->GetProxyMethod());
    if (SessionData->GetProxyMethod() >= static_cast<int>(FtpProxyMethodCombo->GetItems()->GetCount()))
    {
        FtpProxyMethodCombo->GetItems()->SetSelected(pmNone);
    }
    else
    {
        FtpProxyMethodCombo->GetItems()->SetSelected(SessionData->GetProxyMethod());
    }
    ProxyHostEdit->SetText(SessionData->GetProxyHost());
    ProxyPortEdit->SetAsInteger(SessionData->GetProxyPort());
    ProxyUsernameEdit->SetText(SessionData->GetProxyUsername());
    ProxyPasswordEdit->SetText(SessionData->GetProxyPassword());
    ProxyTelnetCommandEdit->SetText(SessionData->GetProxyTelnetCommand());
    ProxyLocalCommandEdit->SetText(SessionData->GetProxyLocalCommand());
    ProxyLocalhostCheck->SetChecked(SessionData->GetProxyLocalhost());
    switch (SessionData->GetProxyDNS())
    {
    case asOn: ProxyDNSOnButton->SetChecked(true); break;
    case asOff: ProxyDNSOffButton->SetChecked(true); break;
    default: ProxyDNSAutoButton->SetChecked(true); break;
    }

    // Tunnel tab
    TunnelCheck->SetChecked(SessionData->GetTunnel());
    TunnelUserNameEdit->SetText(SessionData->GetTunnelUserName());
    TunnelPortNumberEdit->SetAsInteger(static_cast<int>(SessionData->GetTunnelPortNumber()));
    TunnelHostNameEdit->SetText(SessionData->GetTunnelHostName());
    TunnelPasswordEdit->SetText(SessionData->GetTunnelPassword());
    TunnelPrivateKeyEdit->SetText(SessionData->GetTunnelPublicKeyFile());
    if (SessionData->GetTunnelAutoassignLocalPortNumber())
    {
        TunnelLocalPortNumberEdit->SetText(TunnelLocalPortNumberEdit->GetItems()->GetString(0));
    }
    else
    {
        TunnelLocalPortNumberEdit->SetText(IntToStr(static_cast<int>(SessionData->GetTunnelLocalPortNumber())));
    }

    // SSH tab
    CompressionCheck->SetChecked(SessionData->GetCompression());
    if (Ssh2DESCheck != NULL)
    {
        Ssh2DESCheck->SetChecked(SessionData->GetSsh2DES());
    }

    switch (SessionData->GetSshProt())
    {
    case ssh1only:  SshProt1onlyButton->SetChecked(true); break;
    case ssh1:      SshProt1Button->SetChecked(true); break;
    case ssh2:      SshProt2Button->SetChecked(true); break;
    case ssh2only:  SshProt2onlyButton->SetChecked(true); break;
    }

    CipherListBox->GetItems()->BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&CipherListBox) )
        {
            CipherListBox->GetItems()->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        CipherListBox->GetItems()->Clear();
        assert(CIPHER_NAME_WARN+CIPHER_COUNT-1 == CIPHER_NAME_ARCFOUR);
        for (size_t Index = 0; Index < CIPHER_COUNT; Index++)
        {
            nb::TObject *Obj = static_cast<nb::TObject *>(reinterpret_cast<void *>(SessionData->GetCipher(Index)));
            CipherListBox->GetItems()->AddObject(
                GetMsg(CIPHER_NAME_WARN + static_cast<int>(SessionData->GetCipher(Index))),
                Obj);
        }
    }

    // KEX tab

    RekeyTimeEdit->SetAsInteger(SessionData->GetRekeyTime());
    RekeyDataEdit->SetText(SessionData->GetRekeyData());

    KexListBox->GetItems()->BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&KexListBox) )
        {
            KexListBox->GetItems()->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        KexListBox->GetItems()->Clear();
        // DEBUG_PRINTF(L"KEX_NAME_WARN = %d, KEX_COUNT = %d, KEX_NAME_GSSGEX = %d", KEX_NAME_WARN, KEX_COUNT, KEX_NAME_GSSGEX);
        assert(KEX_NAME_WARN+KEX_COUNT+1 == KEX_NAME_GSSGEX);
        for (size_t Index = 0; Index < KEX_COUNT; Index++)
        {
            KexListBox->GetItems()->AddObject(
                GetMsg(KEX_NAME_WARN + static_cast<int>(SessionData->GetKex(Index))),
                static_cast<nb::TObject *>(reinterpret_cast<void *>(SessionData->GetKex(Index))));
        }
    }

    // Authentication tab
    SshNoUserAuthCheck->SetChecked(SessionData->GetSshNoUserAuth());
    TryAgentCheck->SetChecked(SessionData->GetTryAgent());
    AuthTISCheck->SetChecked(SessionData->GetAuthTIS());
    AuthKICheck->SetChecked(SessionData->GetAuthKI());
    AuthKIPasswordCheck->SetChecked(SessionData->GetAuthKIPassword());
    AuthGSSAPICheck2->SetChecked(SessionData->GetAuthGSSAPI());
    AgentFwdCheck->SetChecked(SessionData->GetAgentFwd());
    GSSAPIServerRealmEdit->SetText(SessionData->GetGSSAPIServerRealm());

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

        if ((GetFSProtocol() == fsHTTP) || (GetFSProtocol() == fsHTTPS))
        {
            ::AdjustRemoteDir(HostNameEdit, RemoteDirectoryEdit, UpdateDirectoriesCheck);
        }

        // save session data

        // Basic tab
        SessionData->SetHostName(HostNameEdit->GetText());
        SessionData->SetPortNumber(PortNumberEdit->GetAsInteger());
        SessionData->SetUserName(UserNameEdit->GetText());
        SessionData->SetPassword(PasswordEdit->GetText());
        SessionData->SetLoginType(GetLoginType());
        SessionData->SetPublicKeyFile(PrivateKeyEdit->GetText());

        SessionData->SetFSProtocol(GetFSProtocol());

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
        if (EOLTypeCombo->GetItems()->GetSelected() == 0)
        {
            SessionData->SetEOLType(eolLF);
        }
        else
        {
            SessionData->SetEOLType(eolCRLF);
        }
        switch (UtfCombo->GetItems()->GetSelected())
        {
        case 1:
            SessionData->SetNotUtf(asOn);
            break;

        case 2:
            SessionData->SetNotUtf(asOff);
            break;

        default:
            SessionData->SetNotUtf(asAuto);
            break;
        }

        SessionData->SetDeleteToRecycleBin(DeleteToRecycleBinCheck->GetChecked());
        SessionData->SetOverwrittenToRecycleBin(OverwrittenToRecycleBinCheck->GetChecked());
        SessionData->SetRecycleBinPath(RecycleBinPathEdit->GetText());

        // SCP tab
        SessionData->SetDefaultShell(ShellEdit->GetText() == ShellEdit->GetItems()->GetString(0));
        SessionData->SetShell((SessionData->GetDefaultShell() ? std::wstring() : ShellEdit->GetText()));
        SessionData->SetDetectReturnVar(ReturnVarEdit->GetText() == ReturnVarEdit->GetItems()->GetString(0));
        SessionData->SetReturnVar((SessionData->GetDetectReturnVar() ? std::wstring() : ReturnVarEdit->GetText()));
        SessionData->SetLookupUserGroups(LookupUserGroupsCheck->GetChecked());
        SessionData->SetClearAliases(ClearAliasesCheck->GetChecked());
        SessionData->SetIgnoreLsWarnings(IgnoreLsWarningsCheck->GetChecked());
        SessionData->SetScp1Compatibility(Scp1CompatibilityCheck->GetChecked());
        SessionData->SetUnsetNationalVars(UnsetNationalVarsCheck->GetChecked());
        SessionData->SetListingCommand(ListingCommandEdit->GetText());
        SessionData->SetSCPLsFullTime(SCPLsFullTimeAutoCheck->GetChecked() ? asAuto : asOff);
        SessionData->SetTimeDifference(nb::TDateTime(
                                           (static_cast<double>(TimeDifferenceEdit->GetAsInteger()) / 24) +
                                           (static_cast<double>(TimeDifferenceMinutesEdit->GetAsInteger()) / 24 / 60)));

        // SFTP tab
#define TRISTATE(COMBO, PROP, MSG) \
      SessionData->Set##PROP(sb##PROP, static_cast<TAutoSwitch>(2 - COMBO->GetItems()->GetSelected()));
        // SFTP_BUGS();
        SessionData->SetSFTPBug(sbSymlink, static_cast<TAutoSwitch>(2 - SFTPBugSymlinkCombo->GetItems()->GetSelected()));
        SessionData->SetSFTPBug(sbSignedTS, static_cast<TAutoSwitch>(2 - SFTPBugSignedTSCombo->GetItems()->GetSelected()));

        SessionData->SetSftpServer(
            (SftpServerEdit->GetText() == SftpServerEdit->GetItems()->GetString(0)) ?
            std::wstring() : SftpServerEdit->GetText());
        SessionData->SetSFTPMaxVersion(SFTPMaxVersionCombo->GetItems()->GetSelected());
        SessionData->SetSFTPMinPacketSize(SFTPMinPacketSizeEdit->GetAsInteger());
        SessionData->SetSFTPMaxPacketSize(SFTPMaxPacketSizeEdit->GetAsInteger());

        // FTP tab
        SessionData->SetFtpAllowEmptyPassword(FtpAllowEmptyPasswordCheck->GetChecked());
        nb::TStrings *PostLoginCommands = new nb::TStringList;
        {
            BOOST_SCOPE_EXIT ( (&PostLoginCommands) )
            {
                delete PostLoginCommands;
            } BOOST_SCOPE_EXIT_END
            for (size_t Index = 0; Index < LENOF(PostLoginCommandsEdits); Index++)
            {
                std::wstring Text = PostLoginCommandsEdits[Index]->GetText();
                if (!Text.empty())
                {
                    PostLoginCommands->Add(PostLoginCommandsEdits[Index]->GetText());
                }
            }

            SessionData->SetPostLoginCommands(PostLoginCommands->GetText());
        }
        // TODO: FTPS tab
        if (GetFSProtocol() == fsFTPS)
        {
            SessionData->SetFtps(ftpsImplicit);
        }
        else
        {
            SessionData->SetFtps(ftpsNone);
        }

        // Connection tab
        SessionData->SetFtpPasvMode(FtpPasvModeCheck->GetChecked());
        if (PingNullPacketButton->GetChecked())
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
        SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
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

        // Proxy tab
        SessionData->SetProxyMethod(static_cast<TProxyMethod>(SshProxyMethodCombo->GetItems()->GetSelected()));
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
            SessionData->SetTunnelLocalPortNumber(StrToIntDef(TunnelLocalPortNumberEdit->GetText(), 0));
        }

        // SSH tab
        SessionData->SetCompression(CompressionCheck->GetChecked());
        if (Ssh2DESCheck != NULL)
        {
            SessionData->SetSsh2DES(Ssh2DESCheck->GetChecked());
        }

        if (SshProt1onlyButton->GetChecked()) { SessionData->SetSshProt(ssh1only); }
        else if (SshProt1Button->GetChecked()) { SessionData->SetSshProt(ssh1); }
        else if (SshProt2Button->GetChecked()) { SessionData->SetSshProt(ssh2); }
        else { SessionData->SetSshProt(ssh2only); }

        for (size_t Index = 0; Index < CIPHER_COUNT; Index++)
        {
            nb::TObject *Obj = static_cast<nb::TObject *>(CipherListBox->GetItems()->GetObject(Index));
            SessionData->SetCipher(Index, static_cast<TCipher>(reinterpret_cast<int>(Obj)));
        }

        // KEX tab

        SessionData->SetRekeyTime(RekeyTimeEdit->GetAsInteger());
        SessionData->SetRekeyData(RekeyDataEdit->GetText());

        for (size_t Index = 0; Index < KEX_COUNT; Index++)
        {
            SessionData->SetKex(Index, (TKex)(int)KexListBox->GetItems()->GetObject(Index));
        }

        // Authentication tab
        SessionData->SetSshNoUserAuth(SshNoUserAuthCheck->GetChecked());
        SessionData->SetTryAgent(TryAgentCheck->GetChecked());
        SessionData->SetAuthTIS(AuthTISCheck->GetChecked());
        SessionData->SetAuthKI(AuthKICheck->GetChecked());
        SessionData->SetAuthKIPassword(AuthKIPasswordCheck->GetChecked());
        SessionData->SetAuthGSSAPI(AuthGSSAPICheck2->GetChecked());
        SessionData->SetAgentFwd(AgentFwdCheck->GetChecked());
        SessionData->SetGSSAPIServerRealm(GSSAPIServerRealmEdit->GetText());

        // Bugs tab
        // BUGS();
#undef TRISTATE
        SessionData->SetBug(sbIgnore1, static_cast<TAutoSwitch>(2 - BugIgnore1Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbPlainPW1, static_cast<TAutoSwitch>(2 - BugPlainPW1Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbRSA1, static_cast<TAutoSwitch>(2 - BugRSA1Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbHMAC2, static_cast<TAutoSwitch>(2 - BugHMAC2Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbDeriveKey2, static_cast<TAutoSwitch>(2 - BugDeriveKey2Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbRSAPad2, static_cast<TAutoSwitch>(2 - BugRSAPad2Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbPKSessID2, static_cast<TAutoSwitch>(2 - BugPKSessID2Combo->GetItems()->GetSelected()));
        SessionData->SetBug(sbRekey2, static_cast<TAutoSwitch>(2 - BugRekey2Combo->GetItems()->GetSelected()));
    }

    return Result;
}
//---------------------------------------------------------------------------
void TSessionDialog::LoadPing(TSessionData *SessionData)
{
    TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
                             AllowScpFallbackCheck->GetChecked());

    switch ((FSProtocol == fsFTP) || (FSProtocol == fsFTPS) ? SessionData->GetFtpPingType() : SessionData->GetPingType())
    {
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
        ((GetFSProtocol() == fsFTP) || (GetFSProtocol() == fsFTPS) ?
         SessionData->GetFtpPingInterval() : SessionData->GetPingInterval()));
}
//---------------------------------------------------------------------
void TSessionDialog::SavePing(TSessionData *SessionData)
{
    TPingType PingType;
    if (PingNullPacketButton->GetChecked())
    {
        PingType = ptNullPacket;
    }
    else if (PingDummyCommandButton->GetChecked())
    {
        PingType = ptDummyCommand;
    }
    else
    {
        PingType = ptOff;
    }
    TFSProtocol FSProtocol = IndexToFSProtocol(FTransferProtocolIndex,
                             AllowScpFallbackCheck->GetChecked());
    if ((FSProtocol == fsFTP) || (FSProtocol == fsFTPS))
    {
        SessionData->SetFtpPingType(PingType);
        SessionData->SetFtpPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
    else
    {
        SessionData->SetPingType(PingType);
        SessionData->SetPingInterval(PingIntervalSecEdit->GetAsInteger());
    }
}
//---------------------------------------------------------------------------
size_t TSessionDialog::LoginTypeToIndex(TLoginType LoginType)
{
    return static_cast<size_t>(LoginType);
}
//---------------------------------------------------------------------------
size_t TSessionDialog::FSProtocolToIndex(TFSProtocol FSProtocol,
                                      bool &AllowScpFallback)
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
        for (size_t Index = 0; Index < TransferProtocolCombo->GetItems()->GetCount(); Index++)
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
TFSProtocol TSessionDialog::GetFSProtocol()
{
    return IndexToFSProtocol(TransferProtocolCombo->GetItems()->GetSelected(),
                             AllowScpFallbackCheck->GetChecked());
}
//---------------------------------------------------------------------------
TLoginType TSessionDialog::GetLoginType()
{
    return IndexToLoginType(LoginTypeCombo->GetItems()->GetSelected());
}
//---------------------------------------------------------------------------
TFSProtocol TSessionDialog::IndexToFSProtocol(size_t Index, bool AllowScpFallback)
{
    bool InBounds = (Index != -1) && (Index < LENOF(FSOrder));
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
TLoginType TSessionDialog::IndexToLoginType(size_t Index)
{
    bool InBounds = (Index != -1) && (Index <= ltNormal);
    assert(InBounds);
    TLoginType Result = ltAnonymous;
    if (InBounds)
    {
        Result = TLoginType(Index);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TSessionDialog::VerifyKey(std::wstring FileName, bool TypeOnly)
{
    bool Result = true;

    if (!::Trim(FileName).empty())
    {
        TKeyType Type = KeyType(FileName);
        std::wstring Message;
        switch (Type)
        {
        case ktOpenSSH:
            Message = FMTLOAD(KEY_TYPE_UNSUPPORTED, FileName.c_str(), L"OpenSSH SSH-2");
            break;

        case ktSSHCom:
            Message = FMTLOAD(KEY_TYPE_UNSUPPORTED, FileName.c_str(), L"ssh.com SSH-2");
            break;

        case ktSSH1:
        case ktSSH2:
            if (!TypeOnly)
            {
                if ((Type == ktSSH1) !=
                        (SshProt1onlyButton->GetChecked() || SshProt1Button->GetChecked()))
                {
                    Message = FMTLOAD(KEY_TYPE_DIFFERENT_SSH,
                                      FileName.c_str(), (Type == ktSSH1 ? L"SSH-1" : L"PuTTY SSH-2"));
                }
            }
            break;

        default:
            assert(false);
            // fallthru
        case ktUnopenable:
        case ktUnknown:
            Message = FMTLOAD(KEY_TYPE_UNKNOWN, FileName.c_str());
            break;
        }

        if (!Message.empty())
        {
            TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
            Result = (WinSCPPlugin->MoreMessageDialog(Message, NULL, qtWarning,
                      qaIgnore | qaAbort) != qaAbort);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
bool TSessionDialog::CloseQuery()
{
    bool CanClose = TTabbedDialog::CloseQuery();

    if (CanClose && (GetResult() != brCancel))
    {
        CanClose =
            VerifyKey(PrivateKeyEdit->GetText(), false) &&
            // for tunnel key do not check SSH version as it is not configurable
            VerifyKey(TunnelPrivateKeyEdit->GetText(), true);
    }

    if (CanClose && !PasswordEdit->GetText().empty() &&
            !Configuration->GetDisablePasswordStoring() &&
            (PasswordEdit->GetText() != FSessionData->GetPassword()) &&
            (((GetResult() == brOK)) ||
             ((GetResult() == brConnect) && (FAction == saEdit))))
    {
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
        CanClose = (WinSCPPlugin->MoreMessageDialog(GetMsg(SAVE_PASSWORD), NULL,
                    qtWarning, qaOK | qaCancel) == qaOK);
    }

    return CanClose;
}
//---------------------------------------------------------------------------
void TSessionDialog::CipherButtonClick(TFarButton *Sender, bool &Close)
{
    if (Sender->GetEnabled())
    {
        size_t Source = CipherListBox->GetItems()->GetSelected();
        size_t Dest = Source + Sender->GetResult();

        CipherListBox->GetItems()->Move(Source, Dest);
        CipherListBox->GetItems()->SetSelected(Dest);
    }

    Close = false;
}
//---------------------------------------------------------------------------
void TSessionDialog::KexButtonClick(TFarButton *Sender, bool &Close)
{
    if (Sender->GetEnabled())
    {
        size_t Source = KexListBox->GetItems()->GetSelected();
        size_t Dest = Source + Sender->GetResult();

        KexListBox->GetItems()->Move(Source, Dest);
        KexListBox->GetItems()->SetSelected(Dest);
    }

    Close = false;
}
//---------------------------------------------------------------------------
void TSessionDialog::AuthGSSAPICheckAllowChange(TFarDialogItem * /*Sender*/,
        void *NewState, bool &Allow)
{
    if ((reinterpret_cast<int>(NewState) == BSTATE_CHECKED) && !Configuration->GetGSSAPIInstalled())
    {
        Allow = false;
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);

        WinSCPPlugin->MoreMessageDialog(GetMsg(GSSAPI_NOT_INSTALLED),
                                        NULL, qtError, qaOK);
    }
}
//---------------------------------------------------------------------------
void TSessionDialog::UnixEnvironmentButtonClick(
    TFarButton * /*Sender*/, bool & /*Close*/)
{
    EOLTypeCombo->GetItems()->SetSelected(0);
    DSTModeUnixCheck->SetChecked(true);
}
//---------------------------------------------------------------------------
void TSessionDialog::WindowsEnvironmentButtonClick(
    TFarButton * /*Sender*/, bool & /*Close*/)
{
    EOLTypeCombo->GetItems()->SetSelected(1);
    DSTModeWinCheck->SetChecked(true);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SessionDialog(TSessionData *SessionData,
                                      TSessionActionEnum &Action)
{
    bool Result;
    TSessionDialog *Dialog = new TSessionDialog(FPlugin, Action);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(SessionData, Action);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TRightsContainer : public TFarDialogContainer
{
public:
    explicit TRightsContainer(TFarDialog *ADialog, bool AAnyDirectories,
                              bool ShowButtons, bool ShowSpecials,
                              TFarDialogItem *EnabledDependency);
    // __property bool AddXToDirectories = { read = GetAddXToDirectories, write = SetAddXToDirectories };
    bool GetAddXToDirectories();
    void SetAddXToDirectories(bool value);
    // __property TRights Rights = { read = GetRights, write = SetRights };
    TRights GetRights();
    void SetRights(const TRights &value);
    // __property TFarCheckBox * Checks[TRights::TRight Right] = { read = GetChecks };
    TFarCheckBox *GetCheck(TRights::TRight Right);
    // __property TRights::TState States[TRights::TRight Right] = { read = GetStates, write = SetStates };
    TRights::TState GetState(TRights::TRight Right);
    void SetState(TRights::TRight Flag, TRights::TState value);
    // __property bool AllowUndef = { read = GetAllowUndef, write = SetAllowUndef };
    bool GetAllowUndef();
    void SetAllowUndef(bool value);

protected:
    bool FAnyDirectories;
    TFarCheckBox *FCheckBoxes[12];
    TRights::TState FFixedStates[12];
    TFarEdit *OctalEdit;
    TFarCheckBox *DirectoriesXCheck;
    TRightsContainer *Self;

    virtual void Change();
    void UpdateControls();

private:
    void OctalEditExit(nb::TObject *Sender);
    void RightsButtonClick(TFarButton *Sender, bool &Close);
};
//---------------------------------------------------------------------------
TRightsContainer::TRightsContainer(TFarDialog *ADialog,
                                   bool AAnyDirectories, bool ShowButtons,
                                   bool ShowSpecials, TFarDialogItem *EnabledDependency) :
    TFarDialogContainer(ADialog),
    OctalEdit(NULL),
    DirectoriesXCheck(NULL)
{
    Self = this;
    FAnyDirectories = AAnyDirectories;

    TFarButton *Button;
    TFarText *Text;
    TFarCheckBox *CheckBox;

    GetDialog()->SetNextItemPosition(ipNewLine);

    static int RowLabels[] = { PROPERTIES_OWNER_RIGHTS, PROPERTIES_GROUP_RIGHTS,
                               PROPERTIES_OTHERS_RIGHTS
                             };
    static int ColLabels[] = { PROPERTIES_READ_RIGHTS, PROPERTIES_WRITE_RIGHTS,
                               PROPERTIES_EXECUTE_RIGHTS
                             };
    static int SpecialLabels[] = { PROPERTIES_SETUID_RIGHTS, PROPERTIES_SETGID_RIGHTS,
                                   PROPERTIES_STICKY_BIT_RIGHTS
                                 };

    for (size_t RowIndex = 0; RowIndex < 3; RowIndex++)
    {
        GetDialog()->SetNextItemPosition(ipNewLine);
        Text = new TFarText(GetDialog());
        if (RowIndex == 0)
        {
            Text->SetTop(0);
        }
        Text->SetLeft(0);
        Add(Text);
        Text->SetEnabledDependency(EnabledDependency);
        Text->SetCaption(GetMsg(RowLabels[RowIndex]));

        GetDialog()->SetNextItemPosition(ipRight);

        for (size_t ColIndex = 0; ColIndex < 3; ColIndex++)
        {
            CheckBox = new TFarCheckBox(GetDialog());
            FCheckBoxes[(RowIndex + 1)* 3 + ColIndex] = CheckBox;
            Add(CheckBox);
            CheckBox->SetEnabledDependency(EnabledDependency);
            CheckBox->SetCaption(GetMsg(ColLabels[ColIndex]));
        }

        if (ShowSpecials)
        {
            CheckBox = new TFarCheckBox(GetDialog());
            Add(CheckBox);
            CheckBox->SetVisible(ShowSpecials);
            CheckBox->SetEnabledDependency(EnabledDependency);
            CheckBox->SetCaption(GetMsg(SpecialLabels[RowIndex]));
            FCheckBoxes[RowIndex] = CheckBox;
        }
        else
        {
            FCheckBoxes[RowIndex] = NULL;
            FFixedStates[RowIndex] = TRights::rsNo;
        }
    }

    GetDialog()->SetNextItemPosition(ipNewLine);

    Text = new TFarText(GetDialog());
    Add(Text);
    Text->SetEnabledDependency(EnabledDependency);
    Text->SetLeft(0);
    Text->SetCaption(GetMsg(PROPERTIES_OCTAL));

    GetDialog()->SetNextItemPosition(ipRight);

    OctalEdit = new TFarEdit(GetDialog());
    Add(OctalEdit);
    OctalEdit->SetEnabledDependency(EnabledDependency);
    OctalEdit->SetWidth(5);
    OctalEdit->SetMask(L"9999");
    OctalEdit->SetOnExit(boost::bind(&TRightsContainer::OctalEditExit, this, _1));

    if (ShowButtons)
    {
        GetDialog()->SetNextItemPosition(ipRight);

        Button = new TFarButton(GetDialog());
        Add(Button);
        Button->SetEnabledDependency(EnabledDependency);
        Button->SetCaption(GetMsg(PROPERTIES_NONE_RIGHTS));
        Button->SetTag(TRights::rfNo);
        Button->SetOnClick(boost::bind(&TRightsContainer::RightsButtonClick, this, _1, _2));

        Button = new TFarButton(GetDialog());
        Add(Button);
        Button->SetEnabledDependency(EnabledDependency);
        Button->SetCaption(GetMsg(PROPERTIES_DEFAULT_RIGHTS));
        Button->SetTag(TRights::rfDefault);
        Button->SetOnClick(boost::bind(&TRightsContainer::RightsButtonClick, this, _1, _2));

        Button = new TFarButton(GetDialog());
        Add(Button);
        Button->SetEnabledDependency(EnabledDependency);
        Button->SetCaption(GetMsg(PROPERTIES_ALL_RIGHTS));
        Button->SetTag(TRights::rfAll);
        Button->SetOnClick(boost::bind(&TRightsContainer::RightsButtonClick, this, _1, _2));
    }

    GetDialog()->SetNextItemPosition(ipNewLine);

    if (FAnyDirectories)
    {
        DirectoriesXCheck = new TFarCheckBox(GetDialog());
        Add(DirectoriesXCheck);
        DirectoriesXCheck->SetEnabledDependency(EnabledDependency);
        DirectoriesXCheck->SetLeft(0);
        DirectoriesXCheck->SetCaption(GetMsg(PROPERTIES_DIRECTORIES_X));
    }
    else
    {
        DirectoriesXCheck = NULL;
    }
}
//---------------------------------------------------------------------------
void TRightsContainer::RightsButtonClick(TFarButton *Sender,
        bool & /*Close*/)
{
    TRights R = GetRights();
    R.SetNumber(static_cast<unsigned short>(Sender->GetTag()));
    SetRights(R);
}
//---------------------------------------------------------------------------
void TRightsContainer::OctalEditExit(nb::TObject * /*Sender*/)
{
    if (!::Trim(OctalEdit->GetText()).empty())
    {
        TRights R = GetRights();
        R.SetOctal(::Trim(OctalEdit->GetText()));
        SetRights(R);
    }
}
//---------------------------------------------------------------------------
void TRightsContainer::UpdateControls()
{
    if (GetDialog()->GetHandle())
    {
        TRights R = GetRights();

        if (DirectoriesXCheck)
        {
            DirectoriesXCheck->SetEnabled(
                !((R.GetNumberSet() & TRights::rfExec) == TRights::rfExec));
        }

        if (!OctalEdit->Focused())
        {
            OctalEdit->SetText(R.GetIsUndef() ? std::wstring() : R.GetOctal());
        }
        else if (::Trim(OctalEdit->GetText()).size() >= 3)
        {
            try
            {
                OctalEditExit(NULL);
            }
            catch (...)
            {
            }
        }
    }
}
//---------------------------------------------------------------------------
void TRightsContainer::Change()
{
    TFarDialogContainer::Change();

    if (GetDialog()->GetHandle())
    {
        UpdateControls();
    }
}
//---------------------------------------------------------------------------
TFarCheckBox *TRightsContainer::GetCheck(TRights::TRight Right)
{
    assert((Right >= 0) && (Right < LENOF(FCheckBoxes)));
    return FCheckBoxes[Right];
}
//---------------------------------------------------------------------------
TRights::TState TRightsContainer::GetState(TRights::TRight Right)
{
    TFarCheckBox *CheckBox = GetCheck(Right);
    if (CheckBox != NULL)
    {
        switch (CheckBox->GetSelected())
        {
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
void TRightsContainer::SetState(TRights::TRight Right,
                                TRights::TState value)
{
    TFarCheckBox *CheckBox = GetCheck(Right);
    if (CheckBox != NULL)
    {
        switch (value)
        {
        case TRights::rsNo: CheckBox->SetSelected(BSTATE_UNCHECKED); break;
        case TRights::rsYes: CheckBox->SetSelected(BSTATE_CHECKED); break;
        case TRights::rsUndef: CheckBox->SetSelected(BSTATE_3STATE); break;
        }
    }
    else
    {
        FFixedStates[Right] = value;
    }
}
//---------------------------------------------------------------------------
TRights TRightsContainer::GetRights()
{
    TRights Result;
    Result.SetAllowUndef(GetAllowUndef());
    for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
    {
        Result.SetRightUndef(static_cast<TRights::TRight>(Right),
                             GetState(static_cast<TRights::TRight>(Right)));
    }
    return Result;
}
//---------------------------------------------------------------------------
void TRightsContainer::SetRights(const TRights &value)
{
    if (GetRights() != value)
    {
        GetDialog()->LockChanges();
        {
            BOOST_SCOPE_EXIT ( (&Self) )
            {
                Self->GetDialog()->UnlockChanges();
            } BOOST_SCOPE_EXIT_END
            SetAllowUndef(true); // temporarily
            for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
            {
                SetState(static_cast<TRights::TRight>(Right),
                         value.GetRightUndef(static_cast<TRights::TRight>(Right)));
            }
            SetAllowUndef(value.GetAllowUndef());
        }
    }
}
//---------------------------------------------------------------------------
bool TRightsContainer::GetAddXToDirectories()
{
    return DirectoriesXCheck ? DirectoriesXCheck->GetChecked() : false;
}
//---------------------------------------------------------------------------
void TRightsContainer::SetAddXToDirectories(bool value)
{
    if (DirectoriesXCheck)
    {
        DirectoriesXCheck->SetChecked(value);
    }
}
//---------------------------------------------------------------------------
bool TRightsContainer::GetAllowUndef()
{
    assert(FCheckBoxes[LENOF(FCheckBoxes) - 1] != NULL);
    return FCheckBoxes[LENOF(FCheckBoxes) - 1]->GetAllowGrayed();
}
//---------------------------------------------------------------------------
void TRightsContainer::SetAllowUndef(bool value)
{
    for (int Right = 0; Right < LENOF(FCheckBoxes); Right++)
    {
        if (FCheckBoxes[Right] != NULL)
        {
            FCheckBoxes[Right]->SetAllowGrayed(value);
        }
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TPropertiesDialog : public TFarDialog
{
public:
    explicit TPropertiesDialog(TCustomFarPlugin *AFarPlugin, nb::TStrings *FileList,
                               const std::wstring Directory,
                               // nb::TStrings * GroupList, nb::TStrings * UserList,
                               const TRemoteTokenList *GroupList, const TRemoteTokenList *UserList,
                               int AllowedChanges);

    bool Execute(TRemoteProperties *Properties);

protected:
    virtual void Change();
    void UpdateProperties(TRemoteProperties &Properties);

private:
    bool FAnyDirectories;
    int FAllowedChanges;
    TRemoteProperties FOrigProperties;
    bool FMultiple;

    TRightsContainer *RightsContainer;
    TFarComboBox *OwnerComboBox;
    TFarComboBox *GroupComboBox;
    TFarCheckBox *RecursiveCheck;
    TFarButton *OkButton;
};
//---------------------------------------------------------------------------
TPropertiesDialog::TPropertiesDialog(TCustomFarPlugin *AFarPlugin,
                                     nb::TStrings *FileList, const std::wstring Directory,
                                     const TRemoteTokenList *GroupList, const TRemoteTokenList *UserList,
                                     // nb::TStrings * GroupList, nb::TStrings * UserList,
                                     int AAllowedChanges) :
    TFarDialog(AFarPlugin),
    RightsContainer(NULL),
    OwnerComboBox(NULL),
    GroupComboBox(NULL),
    RecursiveCheck(NULL),
    OkButton(NULL)
{
    FAllowedChanges = AAllowedChanges;

    assert(FileList->GetCount() > 0);
    TRemoteFile *OnlyFile = reinterpret_cast<TRemoteFile *>(FileList->GetObject(0));
    USEDPARAM(OnlyFile);
    assert(OnlyFile);
    FMultiple = (FileList->GetCount() > 1);
    TRemoteFile *File;
    int Directories = 0;

    nb::TStringList *UsedGroupList = NULL;
    nb::TStringList *UsedUserList = NULL;

    {
        BOOST_SCOPE_EXIT ( (&UsedUserList) (&UsedGroupList) )
        {
            delete UsedUserList;
            delete UsedGroupList;
        } BOOST_SCOPE_EXIT_END
        if ((GroupList == NULL) || (GroupList->GetCount() == 0))
        {
            UsedGroupList = new nb::TStringList();
            UsedGroupList->SetDuplicates(nb::dupIgnore);
            UsedGroupList->SetSorted(true);
        }
        if ((UserList == NULL) || (UserList->GetCount() == 0))
        {
            UsedUserList = new nb::TStringList();
            UsedUserList->SetDuplicates(nb::dupIgnore);
            UsedUserList->SetSorted(true);
        }

        for (size_t Index = 0; Index < FileList->GetCount(); Index++)
        {
            File = reinterpret_cast<TRemoteFile *>(FileList->GetObject(Index));
            assert(File);
            if (UsedGroupList && !File->GetGroup().GetName().empty())
            {
                UsedGroupList->Add(File->GetGroup().GetName());
            }
            if (UsedUserList && !File->GetOwner().GetName().empty())
            {
                UsedUserList->Add(File->GetOwner().GetName());
            }
            if (File->GetIsDirectory())
            {
                Directories++;
            }
        }
        FAnyDirectories = (Directories > 0);

        SetCaption(GetMsg(PROPERTIES_CAPTION));

        SetSize(nb::TPoint(56, 19));

        TFarButton *Button;
        TFarSeparator *Separator;
        TFarText *Text;
        nb::TRect CRect = GetClientRect();

        Text = new TFarText(this);
        Text->SetCaption(GetMsg(PROPERTIES_PROMPT));
        Text->SetCenterGroup(true);

        SetNextItemPosition(ipNewLine);

        Text = new TFarText(this);
        Text->SetCenterGroup(true);
        if (FileList->GetCount() > 1)
        {
            Text->SetCaption(FORMAT(GetMsg(PROPERTIES_PROMPT_FILES).c_str(), FileList->GetCount()));
        }
        else
        {
            Text->SetCaption(MinimizeName(FileList->GetString(0), GetClientSize().x, true));
        }

        new TFarSeparator(this);

        Text = new TFarText(this);
        Text->SetCaption(GetMsg(PROPERTIES_OWNER));
        Text->SetEnabled((FAllowedChanges & cpOwner) != 0);

        SetNextItemPosition(ipRight);

        OwnerComboBox = new TFarComboBox(this);
        OwnerComboBox->SetWidth(20);
        OwnerComboBox->SetEnabled((FAllowedChanges & cpOwner) != 0);
        if (UsedUserList)
        {
            OwnerComboBox->GetItems()->Assign(UsedUserList);
        }
        else if (UserList)
        {
            DEBUG_PRINTF(L"UserList->GetCount = %d", UserList->GetCount());
            for (size_t Index = 0; Index < UserList->GetCount(); Index++)
            {
                DEBUG_PRINTF(L"user = %s", UserList->GetToken(Index)->GetName().c_str());
                GroupComboBox->GetItems()->Add(UserList->GetToken(Index)->GetName());
            }
        }

        SetNextItemPosition(ipNewLine);

        Text = new TFarText(this);
        Text->SetCaption(GetMsg(PROPERTIES_GROUP));
        Text->SetEnabled((FAllowedChanges & cpGroup) != 0);

        SetNextItemPosition(ipRight);

        GroupComboBox = new TFarComboBox(this);
        GroupComboBox->SetWidth(OwnerComboBox->GetWidth());
        GroupComboBox->SetEnabled((FAllowedChanges & cpGroup) != 0);
        if (UsedGroupList)
        {
            GroupComboBox->GetItems()->Assign(UsedGroupList);
        }
        else if (GroupList)
        {
            for (size_t Index = 0; Index < GroupList->GetCount(); Index++)
            {
                DEBUG_PRINTF(L"group = %s", GroupList->GetToken(Index)->GetName().c_str());
                GroupComboBox->GetItems()->Add(GroupList->GetToken(Index)->GetName());
            }
        }

        SetNextItemPosition(ipNewLine);

        Separator = new TFarSeparator(this);
        Separator->SetCaption(GetMsg(PROPERTIES_RIGHTS));

        RightsContainer = new TRightsContainer(this, FAnyDirectories,
                                               true, true, NULL);
        RightsContainer->SetEnabled(FAllowedChanges & cpMode);

        if (FAnyDirectories)
        {
            Separator = new TFarSeparator(this);
            Separator->SetPosition(Separator->GetPosition() + RightsContainer->GetTop());

            RecursiveCheck = new TFarCheckBox(this);
            RecursiveCheck->SetCaption(GetMsg(PROPERTIES_RECURSIVE));
        }
        else
        {
            RecursiveCheck = NULL;
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

        Button = new TFarButton(this);
        Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
        Button->SetResult(brCancel);
        Button->SetCenterGroup(true);
    }
}
//---------------------------------------------------------------------------
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
            bool AllowUndef =
                (FOrigProperties.Valid.Contains(vpRights) &&
                 FOrigProperties.Rights.GetAllowUndef()) ||
                ((RecursiveCheck != NULL) && (RecursiveCheck->GetChecked()));
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
            (!GroupComboBox->GetText().empty() ||
             (FMultiple && !FOrigProperties.Valid.Contains(vpGroup)) ||
             (FOrigProperties.Group.GetName() == GroupComboBox->GetText())) &&
            // same but with owner
            (!OwnerComboBox->GetText().empty() ||
             (FMultiple && !FOrigProperties.Valid.Contains(vpOwner)) ||
             (FOrigProperties.Owner.GetName() == OwnerComboBox->GetText())) &&
            ((FileProperties != FOrigProperties) || (RecursiveCheck && RecursiveCheck->GetChecked())));
    }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::UpdateProperties(TRemoteProperties &Properties)
{
    if (FAllowedChanges & cpMode)
    {
        Properties.Valid << vpRights;
        Properties.Rights = RightsContainer->GetRights();
        Properties.AddXToDirectories = RightsContainer->GetAddXToDirectories();
    }

#define STORE_NAME(PROPERTY) \
    if (!PROPERTY ## ComboBox->GetText().empty() && \
        FAllowedChanges & cp ## PROPERTY) \
    { \
      Properties.Valid << vp ## PROPERTY; \
      Properties.PROPERTY.SetName(::Trim(PROPERTY ## ComboBox->GetText())); \
    }
    STORE_NAME(Group);
    STORE_NAME(Owner);
#undef STORE_NAME

    Properties.Recursive = RecursiveCheck != NULL && RecursiveCheck->GetChecked();
}
//---------------------------------------------------------------------------
bool TPropertiesDialog::Execute(TRemoteProperties *Properties)
{
    TValidProperties Valid;
    if (Properties->Valid.Contains(vpRights) && FAllowedChanges & cpMode) { Valid << vpRights; }
    if (Properties->Valid.Contains(vpOwner) && FAllowedChanges & cpOwner) { Valid << vpOwner; }
    if (Properties->Valid.Contains(vpGroup) && FAllowedChanges & cpGroup) { Valid << vpGroup; }
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
                           Properties->Owner.GetName() : std::wstring());
    GroupComboBox->SetText(Properties->Valid.Contains(vpGroup) ?
                           Properties->Group.GetName() : std::wstring());
    if (RecursiveCheck)
    {
        RecursiveCheck->SetChecked(Properties->Recursive);
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
bool TWinSCPFileSystem::PropertiesDialog(nb::TStrings *FileList,
        const std::wstring Directory,
        const TRemoteTokenList *GroupList, const TRemoteTokenList *UserList,
        // nb::TStrings * GroupList, nb::TStrings * UserList,
        TRemoteProperties *Properties, int AllowedChanges)
{
    bool Result = false;
    TPropertiesDialog *Dialog = new TPropertiesDialog(FPlugin, FileList,
            Directory, GroupList, UserList, AllowedChanges);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Properties);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCopyParamsContainer : public TFarDialogContainer
{
public:
    explicit TCopyParamsContainer(TFarDialog *ADialog,
                                  int Options, int CopyParamAttrs);

    // __property TCopyParamType Params = { read = GetParams, write = SetParams };
    TCopyParamType GetParams();
    void SetParams(TCopyParamType value);
    // __property int Height = { read = GetHeight };
    int GetHeight();

protected:
    TFarRadioButton *TMTextButton;
    TFarRadioButton *TMBinaryButton;
    TFarRadioButton *TMAutomaticButton;
    TFarEdit *AsciiFileMaskEdit;
    TRightsContainer *RightsContainer;
    TFarRadioButton *CCNoChangeButton;
    TFarRadioButton *CCUpperCaseButton;
    TFarRadioButton *CCLowerCaseButton;
    TFarRadioButton *CCFirstUpperCaseButton;
    TFarRadioButton *CCLowerCaseShortButton;
    TFarCheckBox *ReplaceInvalidCharsCheck;
    TFarCheckBox *PreserveRightsCheck;
    TFarCheckBox *PreserveTimeCheck;
    TFarCheckBox *PreserveReadOnlyCheck;
    TFarCheckBox *IgnorePermErrorsCheck;
    TFarCheckBox *ClearArchiveCheck;
    TFarComboBox *NegativeExcludeCombo;
    TFarEdit *ExcludeFileMaskCombo;
    TFarCheckBox *CalculateSizeCheck;
    TFarComboBox *SpeedCombo;

    void ValidateMaskComboExit(nb::TObject *Sender);
    void ValidateSpeedComboExit(nb::TObject *Sender);
    virtual void Change();
    void UpdateControls();

private:
    int FOptions;
    int FCopyParamAttrs;
    TCopyParamType FParams;
};
//---------------------------------------------------------------------------
TCopyParamsContainer::TCopyParamsContainer(TFarDialog *ADialog,
        int Options, int CopyParamAttrs) :
    TFarDialogContainer(ADialog),
    FOptions(Options), FCopyParamAttrs(CopyParamAttrs),
    TMTextButton(NULL),
    TMBinaryButton(NULL),
    TMAutomaticButton(NULL),
    AsciiFileMaskEdit(NULL),
    RightsContainer(NULL),
    CCNoChangeButton(NULL),
    CCUpperCaseButton(NULL),
    CCLowerCaseButton(NULL),
    CCFirstUpperCaseButton(NULL),
    CCLowerCaseShortButton(NULL),
    ReplaceInvalidCharsCheck(NULL),
    PreserveRightsCheck(NULL),
    PreserveTimeCheck(NULL),
    PreserveReadOnlyCheck(NULL),
    IgnorePermErrorsCheck(NULL),
    ClearArchiveCheck(NULL),
    NegativeExcludeCombo(NULL),
    ExcludeFileMaskCombo(NULL),
    CalculateSizeCheck(NULL),
    SpeedCombo(NULL)
{
    TFarBox *Box;
    TFarSeparator *Separator;
    TFarText *Text;

    int TMWidth = 37;
    int TMTop;
    int TMBottom;

    SetLeft(GetLeft() - 1);

    Box = new TFarBox(GetDialog());
    Box->SetLeft(0);
    Box->SetTop(0);
    Box->SetHeight(1);
    Add(Box);
    Box->SetWidth(TMWidth + 2);
    Box->SetCaption(GetMsg(TRANSFER_MODE));

    GetDialog()->SetNextItemPosition(ipRight);

    Box = new TFarBox(GetDialog());
    Add(Box);
    Box->SetLeft(Box->GetLeft() - 2);
    Box->SetRight(Box->GetRight() + 1);
    Box->SetCaption(GetMsg(TRANSFER_UPLOAD_OPTIONS));

    GetDialog()->SetNextItemPosition(ipNewLine);

    TMTextButton = new TFarRadioButton(GetDialog());
    TMTextButton->SetLeft(1);
    Add(TMTextButton);
    TMTop = TMTextButton->GetTop();
    TMTextButton->SetCaption(GetMsg(TRANSFER_MODE_TEXT));
    TMTextButton->SetEnabled(
        FLAGCLEAR(CopyParamAttrs, cpaNoTransferMode) &&
        FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly));

    TMBinaryButton = new TFarRadioButton(GetDialog());
    TMBinaryButton->SetLeft(1);
    Add(TMBinaryButton);
    TMBinaryButton->SetCaption(GetMsg(TRANSFER_MODE_BINARY));
    TMBinaryButton->SetEnabled(TMTextButton->GetEnabled());

    TMAutomaticButton = new TFarRadioButton(GetDialog());
    TMAutomaticButton->SetLeft(1);
    Add(TMAutomaticButton);
    TMAutomaticButton->SetCaption(GetMsg(TRANSFER_MODE_AUTOMATIC));
    TMAutomaticButton->SetEnabled(TMTextButton->GetEnabled());

    Text = new TFarText(GetDialog());
    Text->SetLeft(1);
    Add(Text);
    Text->SetCaption(GetMsg(TRANSFER_MODE_MASK));
    Text->SetEnabledDependency(TMAutomaticButton);

    AsciiFileMaskEdit = new TFarEdit(GetDialog());
    AsciiFileMaskEdit->SetLeft(1);
    Add(AsciiFileMaskEdit);
    AsciiFileMaskEdit->SetEnabledDependency(TMAutomaticButton);
    AsciiFileMaskEdit->SetWidth(TMWidth);
    AsciiFileMaskEdit->SetHistory(ASCII_MASK_HISTORY);
    AsciiFileMaskEdit->SetOnExit(boost::bind(&TCopyParamsContainer::ValidateMaskComboExit, this, _1));

    Box = new TFarBox(GetDialog());
    Box->SetLeft(0);
    Add(Box);
    Box->SetWidth(TMWidth + 2);
    Box->SetCaption(GetMsg(TRANSFER_FILENAME_MODIFICATION));

    CCNoChangeButton = new TFarRadioButton(GetDialog());
    CCNoChangeButton->SetLeft(1);
    Add(CCNoChangeButton);
    CCNoChangeButton->SetCaption(GetMsg(TRANSFER_FILENAME_NOCHANGE));
    CCNoChangeButton->SetEnabled(FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly));

    GetDialog()->SetNextItemPosition(ipRight);

    CCUpperCaseButton = new TFarRadioButton(GetDialog());
    Add(CCUpperCaseButton);
    CCUpperCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_UPPERCASE));
    CCUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

    GetDialog()->SetNextItemPosition(ipNewLine);

    CCFirstUpperCaseButton = new TFarRadioButton(GetDialog());
    CCFirstUpperCaseButton->SetLeft(1);
    Add(CCFirstUpperCaseButton);
    CCFirstUpperCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_FIRSTUPPERCASE));
    CCFirstUpperCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

    GetDialog()->SetNextItemPosition(ipRight);

    CCLowerCaseButton = new TFarRadioButton(GetDialog());
    Add(CCLowerCaseButton);
    CCLowerCaseButton->SetCaption(GetMsg(TRANSFER_FILENAME_LOWERCASE));
    CCLowerCaseButton->SetEnabled(CCNoChangeButton->GetEnabled());

    GetDialog()->SetNextItemPosition(ipNewLine);

    CCLowerCaseShortButton = new TFarRadioButton(GetDialog());
    CCLowerCaseShortButton->SetLeft(1);
    Add(CCLowerCaseShortButton);
    CCLowerCaseShortButton->SetCaption(GetMsg(TRANSFER_FILENAME_LOWERCASESHORT));
    CCLowerCaseShortButton->SetEnabled(CCNoChangeButton->GetEnabled());

    GetDialog()->SetNextItemPosition(ipRight);

    ReplaceInvalidCharsCheck = new TFarCheckBox(GetDialog());
    Add(ReplaceInvalidCharsCheck);
    ReplaceInvalidCharsCheck->SetCaption(GetMsg(TRANSFER_FILENAME_REPLACE_INVALID));
    ReplaceInvalidCharsCheck->SetEnabled(CCNoChangeButton->GetEnabled());

    GetDialog()->SetNextItemPosition(ipNewLine);

    Box = new TFarBox(GetDialog());
    Box->SetLeft(0);
    Add(Box);
    Box->SetWidth(TMWidth + 2);
    Box->SetCaption(GetMsg(TRANSFER_DOWNLOAD_OPTIONS));

    PreserveReadOnlyCheck = new TFarCheckBox(GetDialog());
    Add(PreserveReadOnlyCheck);
    PreserveReadOnlyCheck->SetLeft(1);
    PreserveReadOnlyCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_READONLY));
    PreserveReadOnlyCheck->SetEnabled(
        FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly) &&
        FLAGCLEAR(CopyParamAttrs, cpaNoPreserveReadOnly));
    TMBottom = PreserveReadOnlyCheck->GetTop();

    PreserveRightsCheck = new TFarCheckBox(GetDialog());
    Add(PreserveRightsCheck);
    PreserveRightsCheck->SetLeft(TMWidth + 3);
    PreserveRightsCheck->SetTop(TMTop);
    PreserveRightsCheck->SetBottom(TMTop);
    PreserveRightsCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_RIGHTS));
    PreserveRightsCheck->SetEnabled(
        FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly) &&
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
    IgnorePermErrorsCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_PERM_ERRORS));

    ClearArchiveCheck = new TFarCheckBox(GetDialog());
    ClearArchiveCheck->SetLeft(IgnorePermErrorsCheck->GetLeft());
    Add(ClearArchiveCheck);
    ClearArchiveCheck->SetTop(TMTop + 7);
    ClearArchiveCheck->SetCaption(GetMsg(TRANSFER_CLEAR_ARCHIVE));
    ClearArchiveCheck->SetEnabled(
        FLAGCLEAR(FOptions, coTempTransfer) &&
        FLAGCLEAR(CopyParamAttrs, cpaNoClearArchive) &&
        FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly));

    Box = new TFarBox(GetDialog());
    Box->SetTop(TMTop + 8);
    Add(Box);
    Box->SetBottom(Box->GetTop());
    Box->SetLeft(TMWidth + 3 - 1);
    Box->SetCaption(GetMsg(TRANSFER_COMMON_OPTIONS));

    PreserveTimeCheck = new TFarCheckBox(GetDialog());
    Add(PreserveTimeCheck);
    PreserveTimeCheck->SetLeft(TMWidth + 3);
    PreserveTimeCheck->SetCaption(GetMsg(TRANSFER_PRESERVE_TIMESTAMP));
    PreserveTimeCheck->SetEnabled(
        FLAGCLEAR(CopyParamAttrs, cpaNoPreserveTime) &&
        FLAGCLEAR(CopyParamAttrs, cpaExcludeMaskOnly));

    CalculateSizeCheck = new TFarCheckBox(GetDialog());
    CalculateSizeCheck->SetCaption(GetMsg(TRANSFER_CALCULATE_SIZE));
    Add(CalculateSizeCheck);
    CalculateSizeCheck->SetLeft(TMWidth + 3);

    GetDialog()->SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(GetDialog());
    Add(Separator);
    Separator->SetPosition(TMBottom + 1);
    Separator->SetCaption(GetMsg(TRANSFER_OTHER));

    NegativeExcludeCombo = new TFarComboBox(GetDialog());
    NegativeExcludeCombo->SetLeft(1);
    Add(NegativeExcludeCombo);
    NegativeExcludeCombo->GetItems()->Add(GetMsg(TRANSFER_EXCLUDE));
    NegativeExcludeCombo->GetItems()->Add(GetMsg(TRANSFER_INCLUDE));
    NegativeExcludeCombo->SetDropDownList(true);
    NegativeExcludeCombo->ResizeToFitContent();
    NegativeExcludeCombo->SetEnabled(
        FLAGCLEAR(FOptions, coTempTransfer) &&
        (FLAGCLEAR(CopyParamAttrs, cpaNoExcludeMask) ||
         FLAGSET(CopyParamAttrs, cpaExcludeMaskOnly)));

    GetDialog()->SetNextItemPosition(ipRight);

    Text = new TFarText(GetDialog());
    Add(Text);
    Text->SetCaption(GetMsg(TRANSFER_EXCLUDE_FILE_MASK));
    Text->SetEnabled(NegativeExcludeCombo->GetEnabled());

    GetDialog()->SetNextItemPosition(ipNewLine);

    ExcludeFileMaskCombo = new TFarEdit(GetDialog());
    ExcludeFileMaskCombo->SetLeft(1);
    Add(ExcludeFileMaskCombo);
    ExcludeFileMaskCombo->SetWidth(TMWidth);
    ExcludeFileMaskCombo->SetHistory(EXCLUDE_FILE_MASK_HISTORY);
    ExcludeFileMaskCombo->SetOnExit(boost::bind(&TCopyParamsContainer::ValidateMaskComboExit, this, _1));
    ExcludeFileMaskCombo->SetEnabled(NegativeExcludeCombo->GetEnabled());

    GetDialog()->SetNextItemPosition(ipNewLine);

    Text = new TFarText(GetDialog());
    Add(Text);
    Text->SetCaption(GetMsg(TRANSFER_SPEED));
    Text->MoveAt(TMWidth + 3, NegativeExcludeCombo->GetTop());

    GetDialog()->SetNextItemPosition(ipRight);

    SpeedCombo = new TFarComboBox(GetDialog());
    Add(SpeedCombo);
    SpeedCombo->GetItems()->Add(LoadStr(SPEED_UNLIMITED));
    unsigned long Speed = 1024;
    while (Speed >= 8)
    {
        SpeedCombo->GetItems()->Add(IntToStr(Speed));
        Speed = Speed / 2;
    }
    SpeedCombo->SetOnExit(boost::bind(&TCopyParamsContainer::ValidateSpeedComboExit, this, _1));

    GetDialog()->SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(GetDialog());
    Separator->SetPosition(ExcludeFileMaskCombo->GetBottom() + 1);
    Separator->SetLeft(0);
    Add(Separator);
}
//---------------------------------------------------------------------------
void TCopyParamsContainer::UpdateControls()
{
    if (IgnorePermErrorsCheck != NULL)
    {
        IgnorePermErrorsCheck->SetEnabled(
            ((PreserveRightsCheck->GetEnabled() && PreserveRightsCheck->GetChecked()) ||
             (PreserveTimeCheck->GetEnabled() && PreserveTimeCheck->GetChecked())) &&
            FLAGCLEAR(FCopyParamAttrs, cpaNoIgnorePermErrors) &&
            FLAGCLEAR(FCopyParamAttrs, cpaExcludeMaskOnly));
    }
}
//---------------------------------------------------------------------------
void TCopyParamsContainer::Change()
{
    TFarDialogContainer::Change();

    if (GetDialog()->GetHandle())
    {
        UpdateControls();
    }
}
//---------------------------------------------------------------------------
void TCopyParamsContainer::SetParams(TCopyParamType value)
{
    if (TMBinaryButton->GetEnabled())
    {
        switch (value.GetTransferMode())
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

    AsciiFileMaskEdit->SetText(value.GetAsciiFileMask().GetMasks());

    switch (value.GetFileNameCase())
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

    RightsContainer->SetAddXToDirectories(value.GetAddXToDirectories());
    RightsContainer->SetRights(value.GetRights());
    PreserveRightsCheck->SetChecked(value.GetPreserveRights());
    IgnorePermErrorsCheck->SetChecked(value.GetIgnorePermErrors());

    PreserveReadOnlyCheck->SetChecked(value.GetPreserveReadOnly());
    ReplaceInvalidCharsCheck->SetChecked(
        value.GetInvalidCharsReplacement() != TCopyParamType::NoReplacement);

    ClearArchiveCheck->SetChecked(value.GetClearArchive());

    NegativeExcludeCombo->GetItems()->SetSelected((value.GetNegativeExclude() ? 1 : 0));
    ExcludeFileMaskCombo->SetText(value.GetExcludeFileMask().GetMasks());

    PreserveTimeCheck->SetChecked(value.GetPreserveTime());
    CalculateSizeCheck->SetChecked(value.GetCalculateSize());

    SpeedCombo->SetText(SetSpeedLimit(value.GetCPSLimit()));

    FParams = value;
}
//---------------------------------------------------------------------------
TCopyParamType TCopyParamsContainer::GetParams()
{
    TCopyParamType Result = FParams;

    assert(TMTextButton->GetChecked() || TMBinaryButton->GetChecked() || TMAutomaticButton->GetChecked());
    if (TMTextButton->GetChecked()) { Result.SetTransferMode(tmAscii); }
    else if (TMAutomaticButton->GetChecked()) { Result.SetTransferMode(tmAutomatic); }
    else { Result.SetTransferMode(tmBinary); }

    if (Result.GetTransferMode() == tmAutomatic)
    {
        Result.GetAsciiFileMask().SetMasks(AsciiFileMaskEdit->GetText());
        size_t Start, Length;
        assert(Result.GetAsciiFileMask().GetIsValid(Start, Length));
    }

    if (CCLowerCaseButton->GetChecked()) { Result.SetFileNameCase(ncLowerCase); }
    else if (CCUpperCaseButton->GetChecked()) { Result.SetFileNameCase(ncUpperCase); }
    else if (CCFirstUpperCaseButton->GetChecked()) { Result.SetFileNameCase(ncFirstUpperCase); }
    else if (CCLowerCaseShortButton->GetChecked()) { Result.SetFileNameCase(ncLowerCaseShort); }
    else { Result.SetFileNameCase(ncNoChange); }

    Result.SetAddXToDirectories(RightsContainer->GetAddXToDirectories());
    Result.SetRights(RightsContainer->GetRights());
    Result.SetPreserveRights(PreserveRightsCheck->GetChecked());
    Result.SetIgnorePermErrors(IgnorePermErrorsCheck->GetChecked());

    Result.SetReplaceInvalidChars(ReplaceInvalidCharsCheck->GetChecked());
    Result.SetPreserveReadOnly(PreserveReadOnlyCheck->GetChecked());

    Result.SetClearArchive(ClearArchiveCheck->GetChecked());

    Result.SetNegativeExclude((NegativeExcludeCombo->GetItems()->GetSelected() == 1));
    Result.GetExcludeFileMask().SetMasks(ExcludeFileMaskCombo->GetText());

    Result.SetPreserveTime(PreserveTimeCheck->GetChecked());
    Result.SetCalculateSize(CalculateSizeCheck->GetChecked());

    Result.SetCPSLimit(GetSpeedLimit(SpeedCombo->GetText()));

    return Result;
}
//---------------------------------------------------------------------------
void TCopyParamsContainer::ValidateMaskComboExit(nb::TObject *Sender)
{
    TFarEdit *Edit = dynamic_cast<TFarEdit *>(Sender);
    assert(Edit != NULL);
    TFileMasks Masks(Edit->GetText());
    size_t Start = 0, Length = 0;
    if (!Masks.GetIsValid(Start, Length))
    {
        Edit->SetFocus();
        throw ExtException(FORMAT(GetMsg(EDIT_MASK_ERROR).c_str(), Masks.GetMasks().c_str()));
    }
}
//---------------------------------------------------------------------------
void TCopyParamsContainer::ValidateSpeedComboExit(nb::TObject * /*Sender*/)
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
//---------------------------------------------------------------------------
int TCopyParamsContainer::GetHeight()
{
    return 16;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TCopyDialog : TFarDialog
{
public:
    explicit TCopyDialog(TCustomFarPlugin *AFarPlugin,
                         bool ToRemote, bool Move, nb::TStrings *FileList, int Options, int CopyParamAttrs);

    bool Execute(std::wstring &TargetDirectory, TGUICopyParamType *Params);

protected:
    virtual bool CloseQuery();
    virtual void Change();
    void CustomCopyParam();

    void CopyParamListerClick(TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);
    void TransferSettingsButtonClick(TFarButton *Sender, bool &Close);

private:
    TFarEdit *DirectoryEdit;
    TFarLister *CopyParamLister;
    TFarCheckBox *NewerOnlyCheck;
    TFarCheckBox *SaveSettingsCheck;
    TFarCheckBox *QueueCheck;
    TFarCheckBox *QueueNoConfirmationCheck;

    bool FToRemote;
    int FOptions;
    int FCopyParamAttrs;
    TGUICopyParamType FCopyParams;
};
//---------------------------------------------------------------------------
TCopyDialog::TCopyDialog(TCustomFarPlugin *AFarPlugin,
                         bool ToRemote, bool Move, nb::TStrings *FileList,
                         int Options, int CopyParamAttrs) :
    TFarDialog(AFarPlugin)
{
    FToRemote = ToRemote;
    FOptions = Options;
    FCopyParamAttrs = CopyParamAttrs;

    TFarButton *Button;
    TFarSeparator *Separator;
    TFarText *Text;

    SetSize(nb::TPoint(78, 12 + (FLAGCLEAR(FOptions, coTempTransfer) ? 4 : 0)));
    nb::TRect CRect = GetClientRect();

    SetCaption(GetMsg(Move ? MOVE_TITLE : COPY_TITLE));

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
        std::wstring Prompt;
        if (FileList->GetCount() > 1)
        {
            Prompt = FORMAT(GetMsg(Move ? MOVE_FILES_PROMPT : COPY_FILES_PROMPT).c_str(), FileList->GetCount());
        }
        else
        {
            Prompt = FORMAT(GetMsg(Move ? MOVE_FILE_PROMPT : COPY_FILE_PROMPT).c_str(),
                            ToRemote ? ExtractFileName(FileList->GetString(0), true).c_str() :
                            UnixExtractFileName(FileList->GetString(0)).c_str());
        }

        Text = new TFarText(this);
        Text->SetCaption(Prompt);

        DirectoryEdit = new TFarEdit(this);
        DirectoryEdit->SetHistory(ToRemote ? REMOTE_DIR_HISTORY : L"Copy");
    }

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

    CopyParamLister = new TFarLister(this);
    CopyParamLister->SetHeight(3);
    CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
    CopyParamLister->SetTabStop(false);
    CopyParamLister->SetOnMouseClick(boost::bind(&TCopyDialog::CopyParamListerClick, this, _1, _2));

    new TFarSeparator(this);

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
        NewerOnlyCheck = new TFarCheckBox(this);
        NewerOnlyCheck->SetCaption(GetMsg(TRANSFER_NEWER_ONLY));
        NewerOnlyCheck->SetEnabled(FLAGCLEAR(FOptions, coDisableNewerOnly));

        QueueCheck = new TFarCheckBox(this);
        QueueCheck->SetCaption(GetMsg(TRANSFER_QUEUE));

        SetNextItemPosition(ipRight);

        QueueNoConfirmationCheck = new TFarCheckBox(this);
        QueueNoConfirmationCheck->SetCaption(GetMsg(TRANSFER_QUEUE_NO_CONFIRMATION));
        QueueNoConfirmationCheck->SetEnabledDependency(QueueCheck);

        SetNextItemPosition(ipNewLine);
    }
    else
    {
        assert(FLAGSET(FOptions, coDisableNewerOnly));
    }

    SaveSettingsCheck = new TFarCheckBox(this);
    SaveSettingsCheck->SetCaption(GetMsg(TRANSFER_REUSE_SETTINGS));

    new TFarSeparator(this);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
    Button->SetResult(-1);
    Button->SetCenterGroup(true);
    Button->SetOnClick(boost::bind(&TCopyDialog::TransferSettingsButtonClick, this, _1, _2));

    SetNextItemPosition(ipRight);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_OK));
    Button->SetDefault(true);
    Button->SetResult(brOK);
    Button->SetCenterGroup(true);
    Button->SetEnabledDependency(
        ((Options & coTempTransfer) == 0) ? DirectoryEdit : NULL);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
bool TCopyDialog::Execute(std::wstring &TargetDirectory,
                          TGUICopyParamType *Params)
{
    FCopyParams = *Params;

    if (FLAGCLEAR(FOptions, coTempTransfer))
    {
        NewerOnlyCheck->SetChecked(FLAGCLEAR(FOptions, coDisableNewerOnly) && Params->GetNewerOnly());

        DirectoryEdit->SetText(
            (FToRemote ? UnixIncludeTrailingBackslash(TargetDirectory) :
             ::IncludeTrailingBackslash(TargetDirectory)) + Params->GetFileMask());
        QueueCheck->SetChecked(Params->GetQueue());
        QueueNoConfirmationCheck->SetChecked(Params->GetQueueNoConfirmation());
    }

    bool Result = ShowModal() != brCancel;

    if (Result)
    {
        *Params = FCopyParams;

        if (FLAGCLEAR(FOptions, coTempTransfer))
        {
            if (FToRemote)
            {
                Params->SetFileMask(UnixExtractFileName(DirectoryEdit->GetText()));
                TargetDirectory = UnixExtractFilePath(DirectoryEdit->GetText());
            }
            else
            {
                Params->SetFileMask(ExtractFileName(DirectoryEdit->GetText(), false));
                TargetDirectory = ExtractFilePath(DirectoryEdit->GetText());
            }

            Params->SetNewerOnly(FLAGCLEAR(FOptions, coDisableNewerOnly) && NewerOnlyCheck->GetChecked());

            Params->SetQueue(QueueCheck->GetChecked());
            Params->SetQueueNoConfirmation(QueueNoConfirmationCheck->GetChecked());
        }

        Configuration->BeginUpdate();
        {
            BOOST_SCOPE_EXIT ( (&Configuration) )
            {
                Configuration->EndUpdate();
            } BOOST_SCOPE_EXIT_END
            if (SaveSettingsCheck->GetChecked())
            {
                GUIConfiguration->SetDefaultCopyParam(*Params);
            }
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TCopyDialog::CloseQuery()
{
    bool CanClose = TFarDialog::CloseQuery();

    if (CanClose && GetResult() >= 0)
    {
        if (!FToRemote && ((FOptions & coTempTransfer) == 0))
        {
            std::wstring Directory = ExtractFilePath(DirectoryEdit->GetText());
            if (!DirectoryExists(Directory))
            {
                TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);

                if (WinSCPPlugin->MoreMessageDialog(FORMAT(GetMsg(CREATE_LOCAL_DIRECTORY).c_str(), Directory.c_str()),
                                                    NULL, qtConfirmation, qaOK | qaCancel) != qaCancel)
                {
                    if (!ForceDirectories(Directory))
                    {
                        DirectoryEdit->SetFocus();
                        throw ExtException(FORMAT(GetMsg(CREATE_LOCAL_DIR_ERROR).c_str(), Directory.c_str()));
                    }
                }
                else
                {
                    DirectoryEdit->SetFocus();
                    nb::Abort();
                }
            }
        }
    }
    return CanClose;
}
//---------------------------------------------------------------------------
void TCopyDialog::Change()
{
    TFarDialog::Change();

    if (GetHandle())
    {
        std::wstring InfoStr = FCopyParams.GetInfoStr(L"; ", FCopyParamAttrs);
        nb::TStringList *InfoStrLines = new nb::TStringList();
        {
            BOOST_SCOPE_EXIT ( (&InfoStrLines) )
            {
                delete InfoStrLines;
            } BOOST_SCOPE_EXIT_END
            FarWrapText(InfoStr, InfoStrLines, GetBorderBox()->GetWidth() - 4);
            CopyParamLister->SetItems(InfoStrLines);
            CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
        }
    }
}
//---------------------------------------------------------------------------
void TCopyDialog::TransferSettingsButtonClick(
    TFarButton * /*Sender*/, bool &Close)
{
    CustomCopyParam();
    Close = false;
}
//---------------------------------------------------------------------------
void TCopyDialog::CopyParamListerClick(
    TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD *Event)
{
    if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
    {
        CustomCopyParam();
    }
}
//---------------------------------------------------------------------------
void TCopyDialog::CustomCopyParam()
{
    TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
    if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, FCopyParamAttrs))
    {
        Change();
    }
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::CopyDialog(bool ToRemote,
                                   bool Move, nb::TStrings *FileList,
                                   std::wstring &TargetDirectory,
                                   TGUICopyParamType *Params,
                                   int Options,
                                   int CopyParamAttrs)
{
    bool Result;
    TCopyDialog *Dialog = new TCopyDialog(FPlugin, ToRemote,
                                          Move, FileList, Options, CopyParamAttrs);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(TargetDirectory, Params);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::CopyParamDialog(const std::wstring Caption,
                                    TCopyParamType &CopyParam, int CopyParamAttrs)
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(this);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->SetCaption(Caption);

        // temporary
        Dialog->SetSize(nb::TPoint(78, 10));

        TCopyParamsContainer *CopyParamsContainer = new TCopyParamsContainer(
            Dialog, 0, CopyParamAttrs);

        Dialog->SetSize(nb::TPoint(78, 2 + CopyParamsContainer->GetHeight() + 3));

        Dialog->SetNextItemPosition(ipNewLine);

        Dialog->AddStandardButtons(2, true);

        CopyParamsContainer->SetParams(CopyParam);

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            CopyParam = CopyParamsContainer->GetParams();
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPPlugin::CopyParamCustomDialog(TCopyParamType &CopyParam,
        int CopyParamAttrs)
{
    return CopyParamDialog(GetMsg(COPY_PARAM_CUSTOM_TITLE), CopyParam, CopyParamAttrs);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TLinkDialog : TFarDialog
{
public:
    explicit TLinkDialog(TCustomFarPlugin *AFarPlugin,
                         bool Edit, bool AllowSymbolic);

    bool Execute(std::wstring &FileName, std::wstring &PointTo,
                 bool &Symbolic);

protected:
    virtual void Change();

private:
    TFarEdit *FileNameEdit;
    TFarEdit *PointToEdit;
    TFarCheckBox *SymbolicCheck;
    TFarButton *OkButton;
};
//---------------------------------------------------------------------------
TLinkDialog::TLinkDialog(TCustomFarPlugin *AFarPlugin,
                         bool Edit, bool AllowSymbolic) : TFarDialog(AFarPlugin)
{
    TFarButton *Button;
    TFarSeparator *Separator;
    TFarText *Text;

    SetSize(nb::TPoint(76, 12));
    nb::TRect CRect = GetClientRect();

    SetCaption(GetMsg(Edit ? STRING_LINK_EDIT_CAPTION : STRING_LINK_ADD_CAPTION));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(STRING_LINK_FILE));
    Text->SetEnabled(!Edit);

    FileNameEdit = new TFarEdit(this);
    FileNameEdit->SetEnabled(!Edit);
    FileNameEdit->SetHistory(LINK_FILENAME_HISTORY);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(STRING_LINK_POINT_TO));

    PointToEdit = new TFarEdit(this);
    PointToEdit->SetHistory(LINK_POINT_TO_HISTORY);

    new TFarSeparator(this);

    SymbolicCheck = new TFarCheckBox(this);
    SymbolicCheck->SetCaption(GetMsg(STRING_LINK_SYMLINK));
    SymbolicCheck->SetEnabled(AllowSymbolic && !Edit);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(CRect.Bottom - 1);

    OkButton = new TFarButton(this);
    OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
    OkButton->SetDefault(true);
    OkButton->SetResult(brOK);
    OkButton->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(MSG_BUTTON_Cancel));
    Button->SetResult(brCancel);
    Button->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
void TLinkDialog::Change()
{
    TFarDialog::Change();

    if (GetHandle())
    {
        OkButton->SetEnabled(!FileNameEdit->GetText().empty() &&
                             !PointToEdit->GetText().empty());
    }
}
//---------------------------------------------------------------------------
bool TLinkDialog::Execute(std::wstring &FileName, std::wstring &PointTo,
                          bool &Symbolic)
{
    FileNameEdit->SetText(FileName);
    PointToEdit->SetText(PointTo);
    SymbolicCheck->SetChecked(Symbolic);

    bool Result = ShowModal() != brCancel;
    if (Result)
    {
        FileName = FileNameEdit->GetText();
        PointTo = PointToEdit->GetText();
        Symbolic = SymbolicCheck->GetChecked();
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::LinkDialog(std::wstring &FileName,
                                   std::wstring &PointTo, bool &Symbolic, bool Edit, bool AllowSymbolic)
{
    bool Result;
    TLinkDialog *Dialog = new TLinkDialog(FPlugin, Edit, AllowSymbolic);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(FileName, PointTo, Symbolic);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// typedef void (nb::TObject::*TFeedFileSystemData)
// (nb::TObject * Control, int Label, std::wstring Value);
typedef boost::signal3<void, nb::TObject *, int, std::wstring> feedfilesystemdata_signal_type;
typedef feedfilesystemdata_signal_type::slot_type feedfilesystemdata_slot_type;
//---------------------------------------------------------------------------
class TLabelList;
class TFileSystemInfoDialog : TTabbedDialog
{
public:
    enum { tabProtocol = 1, tabCapabilities, tabSpaceAvailable, tabCount };

    explicit TFileSystemInfoDialog(TCustomFarPlugin *AFarPlugin,
                                   const getspaceavailable_slot_type &OnGetSpaceAvailable);

    void Execute(const TSessionInfo &SessionInfo,
                 const TFileSystemInfo &FileSystemInfo, std::wstring SpaceAvailablePath);

protected:
    void Feed(const feedfilesystemdata_slot_type &AddItem);
    std::wstring CapabilityStr(TFSCapability Capability);
    std::wstring CapabilityStr(TFSCapability Capability1,
                               TFSCapability Capability2);
    std::wstring SpaceStr(__int64 Bytes);
    void ControlsAddItem(nb::TObject *Control, int Label, std::wstring Value);
    void CalculateMaxLenAddItem(nb::TObject *Control, int Label, std::wstring Value);
    void ClipboardAddItem(nb::TObject *Control, int Label, std::wstring Value);
    void FeedControls();
    void UpdateControls();
    TLabelList *CreateLabelArray(size_t Count);
    virtual void SelectTab(int Tab);
    virtual void Change();
    void SpaceAvailableButtonClick(TFarButton *Sender, bool &Close);
    void ClipboardButtonClick(TFarButton *Sender, bool &Close);
    void CheckSpaceAvailable();
    void NeedSpaceAvailable();
    bool SpaceAvailableSupported();
    virtual bool Key(TFarDialogItem *Item, long KeyCode);

private:
    getspaceavailable_signal_type FOnGetSpaceAvailable;
    TFileSystemInfo FFileSystemInfo;
    TSessionInfo FSessionInfo;
    bool FSpaceAvailableLoaded;
    TSpaceAvailable FSpaceAvailable;
    nb::TObject *FLastFeededControl;
    int FLastListItem;
    std::wstring FClipboard;

    TLabelList *ServerLabels;
    TLabelList *ProtocolLabels;
    TLabelList *SpaceAvailableLabels;
    TTabButton *SpaceAvailableTab;
    TFarText *HostKeyFingerprintLabel;
    TFarEdit *HostKeyFingerprintEdit;
    TFarText *InfoLabel;
    TFarSeparator *InfoSeparator;
    TFarLister *InfoLister;
    TFarEdit *SpaceAvailablePathEdit;
    TFarButton *OkButton;
};
//---------------------------------------------------------------------------
class TLabelList : public nb::TList
{
public:
    TLabelList() :
        nb::TList(), MaxLen(0)
    {
    }

    size_t MaxLen;
};
//---------------------------------------------------------------------------
TFileSystemInfoDialog::TFileSystemInfoDialog(TCustomFarPlugin *AFarPlugin,
        const getspaceavailable_slot_type &OnGetSpaceAvailable) : TTabbedDialog(AFarPlugin, tabCount),
    FSpaceAvailableLoaded(false)
{
    FOnGetSpaceAvailable.connect(OnGetSpaceAvailable);
    TFarText *Text;
    TFarSeparator *Separator;
    TFarButton *Button;
    TTabButton *Tab;
    int GroupTop;

    SetSize(nb::TPoint(73, 22));
    SetCaption(GetMsg(SERVER_PROTOCOL_INFORMATION));

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_PROTOCOL));
    Tab->SetTab(tabProtocol);

    SetNextItemPosition(ipRight);

    Tab = new TTabButton(this);
    Tab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_CAPABILITIES));
    Tab->SetTab(tabCapabilities);

    SpaceAvailableTab = new TTabButton(this);
    SpaceAvailableTab->SetTabName(GetMsg(SERVER_PROTOCOL_TAB_SPACE_AVAILABLE));
    SpaceAvailableTab->SetTab(tabSpaceAvailable);

    // Server tab

    SetNextItemPosition(ipNewLine);
    SetDefaultGroup(tabProtocol);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(SERVER_INFORMATION_GROUP));
    GroupTop = Separator->GetTop();

    ServerLabels = CreateLabelArray(5);

    new TFarSeparator(this);

    HostKeyFingerprintLabel = new TFarText(this);
    HostKeyFingerprintLabel->SetCaption(GetMsg(SERVER_HOST_KEY));
    HostKeyFingerprintEdit = new TFarEdit(this);
    HostKeyFingerprintEdit->SetReadOnly(true);

    // Protocol tab

    SetDefaultGroup(tabCapabilities);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(PROTOCOL_INFORMATION_GROUP));
    Separator->SetPosition(GroupTop);

    ProtocolLabels = CreateLabelArray(9);

    InfoSeparator = new TFarSeparator(this);
    InfoSeparator->SetCaption(GetMsg(PROTOCOL_INFO_GROUP));

    InfoLister = new TFarLister(this);
    InfoLister->SetHeight(4);
    InfoLister->SetLeft(GetBorderBox()->GetLeft() + 1);
    // Right edge is adjusted in FeedControls

    // Space available tab

    SetDefaultGroup(tabSpaceAvailable);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(SPACE_AVAILABLE_GROUP));
    Separator->SetPosition(GroupTop);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(SPACE_AVAILABLE_PATH));

    SetNextItemPosition(ipRight);

    SpaceAvailablePathEdit = new TFarEdit(this);
    SpaceAvailablePathEdit->SetRight(
        - (static_cast<int>(GetMsg(SPACE_AVAILABLE_CHECK_SPACE).size() + 11)));

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(SPACE_AVAILABLE_CHECK_SPACE));
    Button->SetEnabledDependency(SpaceAvailablePathEdit);
    Button->SetOnClick(boost::bind(&TFileSystemInfoDialog::SpaceAvailableButtonClick, this, _1, _2));

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    SpaceAvailableLabels = CreateLabelArray(5);

    // Buttons

    SetDefaultGroup(0);

    Separator = new TFarSeparator(this);
    Separator->SetPosition(GetClientRect().Bottom - 1);

    Button = new TFarButton(this);
    Button->SetCaption(GetMsg(SERVER_PROTOCOL_COPY_CLIPBOARD));
    Button->SetOnClick(boost::bind(&TFileSystemInfoDialog::ClipboardButtonClick, this, _1, _2));
    Button->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    OkButton = new TFarButton(this);
    OkButton->SetCaption(GetMsg(MSG_BUTTON_OK));
    OkButton->SetDefault(true);
    OkButton->SetResult(brOK);
    OkButton->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
TLabelList *TFileSystemInfoDialog::CreateLabelArray(size_t Count)
{
    TLabelList *List = new TLabelList();
    try
    {
        for (size_t Index = 0; Index < Count; Index++)
        {
            List->Add(new TFarText(this));
        }
    }
    catch (...)
    {
        delete List;
        throw;
    }
    return List;
}
//---------------------------------------------------------------------
std::wstring TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability)
{
    return BooleanToStr(FFileSystemInfo.IsCapable[Capability]);
}
//---------------------------------------------------------------------
std::wstring TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability1,
        TFSCapability Capability2)
{
    return FORMAT(L"%s/%s", CapabilityStr(Capability1).c_str(), CapabilityStr(Capability2).c_str());
}
//---------------------------------------------------------------------
std::wstring TFileSystemInfoDialog::SpaceStr(__int64 Bytes)
{
    std::wstring Result;
    if (Bytes == 0)
    {
        Result = GetMsg(SPACE_AVAILABLE_BYTES_UNKNOWN);
    }
    else
    {
        Result = FormatBytes(Bytes);
        std::wstring SizeUnorderedStr = FormatBytes(Bytes, false);
        if (Result != SizeUnorderedStr)
        {
            Result = FORMAT(L"%s (%s)", Result.c_str(), SizeUnorderedStr.c_str());
        }
    }
    return Result;
}
//---------------------------------------------------------------------
void TFileSystemInfoDialog::Feed(const feedfilesystemdata_slot_type &AddItem)
{
    feedfilesystemdata_signal_type sig;
    sig.connect(AddItem);
    sig(ServerLabels, SERVER_REMOTE_SYSTEM, FFileSystemInfo.RemoteSystem);
    sig(ServerLabels, SERVER_SESSION_PROTOCOL, FSessionInfo.ProtocolName);
    sig(ServerLabels, SERVER_SSH_IMPLEMENTATION, FSessionInfo.SshImplementation);

    std::wstring Str = FSessionInfo.CSCipher;
    if (FSessionInfo.CSCipher != FSessionInfo.SCCipher)
    {
        Str += FORMAT(L"/%s", FSessionInfo.SCCipher.c_str());
    }
    sig(ServerLabels, SERVER_CIPHER, Str);

    Str = DefaultStr(FSessionInfo.CSCompression, LoadStr(NO_STR));
    if (FSessionInfo.CSCompression != FSessionInfo.SCCompression)
    {
        Str += FORMAT(L"/%s", DefaultStr(FSessionInfo.SCCompression, LoadStr(NO_STR)).c_str());
    }
    sig(ServerLabels, SERVER_COMPRESSION, Str);
    if (FSessionInfo.ProtocolName != FFileSystemInfo.ProtocolName)
    {
        sig(ServerLabels, SERVER_FS_PROTOCOL, FFileSystemInfo.ProtocolName);
    }

    sig(HostKeyFingerprintEdit, 0, FSessionInfo.HostKeyFingerprint);

    sig(ProtocolLabels, PROTOCOL_MODE_CHANGING, CapabilityStr(fcModeChanging));
    sig(ProtocolLabels, PROTOCOL_OWNER_GROUP_CHANGING, CapabilityStr(fcGroupChanging));
    std::wstring AnyCommand;
    if (!FFileSystemInfo.IsCapable[fcShellAnyCommand] &&
            FFileSystemInfo.IsCapable[fcAnyCommand])
    {
        AnyCommand = GetMsg(PROTOCOL_PROTOCOL_ANY_COMMAND);
    }
    else
    {
        AnyCommand = CapabilityStr(fcAnyCommand);
    }
    sig(ProtocolLabels, PROTOCOL_ANY_COMMAND, AnyCommand);
    sig(ProtocolLabels, PROTOCOL_SYMBOLIC_HARD_LINK, CapabilityStr(fcSymbolicLink, fcHardLink));
    sig(ProtocolLabels, PROTOCOL_USER_GROUP_LISTING, CapabilityStr(fcUserGroupListing));
    sig(ProtocolLabels, PROTOCOL_REMOTE_COPY, CapabilityStr(fcRemoteCopy));
    sig(ProtocolLabels, PROTOCOL_CHECKING_SPACE_AVAILABLE, CapabilityStr(fcCheckingSpaceAvailable));
    sig(ProtocolLabels, PROTOCOL_CALCULATING_CHECKSUM, CapabilityStr(fcCalculatingChecksum));
    sig(ProtocolLabels, PROTOCOL_NATIVE_TEXT_MODE, CapabilityStr(fcNativeTextMode));

    sig(InfoLister, 0, FFileSystemInfo.AdditionalInfo);

    sig(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.BytesOnDevice));
    sig(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.UnusedBytesOnDevice));
    sig(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.BytesAvailableToUser));
    sig(SpaceAvailableLabels, SPACE_AVAILABLE_UNUSED_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.UnusedBytesAvailableToUser));
    sig(SpaceAvailableLabels, SPACE_AVAILABLE_BYTES_PER_ALLOCATION_UNIT, SpaceStr(FSpaceAvailable.BytesPerAllocationUnit));
}
//---------------------------------------------------------------------
void TFileSystemInfoDialog::ControlsAddItem(nb::TObject *Control,
        int Label, std::wstring Value)
{
    if (FLastFeededControl != Control)
    {
        FLastFeededControl = Control;
        FLastListItem = 0;
    }

    if (Control == HostKeyFingerprintEdit)
    {
        HostKeyFingerprintEdit->SetText(Value);
        HostKeyFingerprintEdit->SetEnabled(!Value.empty());
        if (!HostKeyFingerprintEdit->GetEnabled())
        {
            HostKeyFingerprintEdit->SetVisible(false);
            HostKeyFingerprintEdit->SetGroup(0);
            HostKeyFingerprintLabel->SetVisible(false);
            HostKeyFingerprintLabel->SetGroup(0);
        }
    }
    else if (Control == InfoLister)
    {
        InfoLister->GetItems()->SetText(Value);
        InfoLister->SetEnabled(!Value.empty());
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
        TLabelList *List = dynamic_cast<TLabelList *>(Control);
        assert(List != NULL);
        if (!Value.empty())
        {
            TFarText *Text = reinterpret_cast<TFarText *>(List->GetItem(FLastListItem));
            FLastListItem++;

            Text->SetCaption(FORMAT(L"%d-%s  %s", List->MaxLen, GetMsg(Label).c_str(), Value.c_str()));
        }
    }
}
//---------------------------------------------------------------------
void TFileSystemInfoDialog::CalculateMaxLenAddItem(nb::TObject *Control,
        int Label, std::wstring Value)
{
    TLabelList *List = dynamic_cast<TLabelList *>(Control);
    if (List != NULL)
    {
        std::wstring S = GetMsg(Label);
        if (List->MaxLen < S.size())
        {
            List->MaxLen = S.size();
        }
    }
}
//---------------------------------------------------------------------
void TFileSystemInfoDialog::ClipboardAddItem(nb::TObject *AControl,
        int Label, std::wstring Value)
{
    TFarDialogItem *Control = dynamic_cast<TFarDialogItem *>(AControl);
    // check for Enabled instead of Visible, as Visible is false
    // when control is on non-active tab
    if (!Value.empty() &&
            ((Control == NULL) || Control->GetEnabled()) &&
            (AControl != SpaceAvailableLabels) ||
            SpaceAvailableSupported())
    {
        if (FLastFeededControl != AControl)
        {
            if (FLastFeededControl != NULL)
            {
                FClipboard += StringOfChar('-', 60) + L"\r\n";
            }
            FLastFeededControl = AControl;
        }

        if (dynamic_cast<TLabelList *>(AControl) == NULL)
        {
            std::wstring LabelStr;
            if (Control == HostKeyFingerprintEdit)
            {
                LabelStr = GetMsg(SERVER_HOST_KEY);
            }
            else if (Control == InfoLister)
            {
                LabelStr = ::Trim(GetMsg(PROTOCOL_INFO_GROUP));
            }
            else
            {
                assert(false);
            }

            if (!LabelStr.empty() && (LabelStr[LabelStr.size() - 1] == ':'))
            {
                LabelStr.resize(LabelStr.size() - 1);
            }

            if ((Value.size() >= 2) && (Value.substr(Value.size() - 1, 2) == L"\r\n"))
            {
                Value.resize(Value.size() - 2);
            }

            FClipboard += FORMAT(L"%s\r\n%s\r\n", LabelStr.c_str(), Value.c_str());
        }
        else
        {
            assert(dynamic_cast<TLabelList *>(AControl) != NULL);
            std::wstring LabelStr = GetMsg(Label);
            if (!LabelStr.empty() && (LabelStr[LabelStr.size() - 1] == ':'))
            {
                LabelStr.resize(LabelStr.size() - 1);
            }
            FClipboard += FORMAT(L"%s = %s\r\n", LabelStr.c_str(), Value.c_str());
        }
    }
}
//---------------------------------------------------------------------
void TFileSystemInfoDialog::FeedControls()
{
    Feed(boost::bind(&TFileSystemInfoDialog::ControlsAddItem, this, _1, _2, _3));
    InfoLister->SetRight(GetBorderBox()->GetRight() - (InfoLister->GetScrollBar() ? 0 : 1));
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::SelectTab(int Tab)
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
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::Execute(
    const TSessionInfo &SessionInfo, const TFileSystemInfo &FileSystemInfo,
    std::wstring SpaceAvailablePath)
{
    FFileSystemInfo = FileSystemInfo;
    FSessionInfo = SessionInfo;
    SpaceAvailablePathEdit->SetText(SpaceAvailablePath);
    UpdateControls();

    Feed(boost::bind(&TFileSystemInfoDialog::CalculateMaxLenAddItem, this, _1, _2, _3));
    FeedControls();
    HideTabs();
    SelectTab(tabProtocol);

    ShowModal();
}
//---------------------------------------------------------------------------
bool TFileSystemInfoDialog::Key(TFarDialogItem *Item, long KeyCode)
{
    bool Result;
    if ((Item == SpaceAvailablePathEdit) && (KeyCode == VK_RETURN))
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
void TFileSystemInfoDialog::Change()
{
    TTabbedDialog::Change();

    if (GetHandle())
    {
        UpdateControls();
    }
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::UpdateControls()
{
    SpaceAvailableTab->SetEnabled(SpaceAvailableSupported());
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::ClipboardButtonClick(TFarButton * /*Sender*/,
        bool &Close)
{
    NeedSpaceAvailable();
    FLastFeededControl = NULL;
    FClipboard = L"";
    Feed(boost::bind(&TFileSystemInfoDialog::ClipboardAddItem, this, _1, _2, _3));
    FarPlugin->FarCopyToClipboard(FClipboard);
    Close = false;
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::SpaceAvailableButtonClick(
    TFarButton * /*Sender*/, bool &Close)
{
    CheckSpaceAvailable();
    Close = false;
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::CheckSpaceAvailable()
{
    assert(!FOnGetSpaceAvailable.empty());
    assert(!SpaceAvailablePathEdit->GetText().empty());

    FSpaceAvailableLoaded = true;

    bool DoClose = false;

    FOnGetSpaceAvailable(SpaceAvailablePathEdit->GetText(), FSpaceAvailable, DoClose);

    FeedControls();
    if (DoClose)
    {
        Close(OkButton);
    }
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::NeedSpaceAvailable()
{
    if (!FSpaceAvailableLoaded && SpaceAvailableSupported())
    {
        CheckSpaceAvailable();
    }
}
//---------------------------------------------------------------------------
bool TFileSystemInfoDialog::SpaceAvailableSupported()
{
    return (!FOnGetSpaceAvailable.empty());
}
//---------------------------------------------------------------------------
void TWinSCPFileSystem::FileSystemInfoDialog(
    const TSessionInfo &SessionInfo, const TFileSystemInfo &FileSystemInfo,
    std::wstring SpaceAvailablePath, const getspaceavailable_slot_type &OnGetSpaceAvailable)
{
    TFileSystemInfoDialog *Dialog = new TFileSystemInfoDialog(FPlugin, OnGetSpaceAvailable);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->Execute(SessionInfo, FileSystemInfo, SpaceAvailablePath);
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::OpenDirectoryDialog(
    bool Add, std::wstring &Directory, TBookmarkList *BookmarkList)
{
    bool Result;
    bool Repeat;

    size_t ItemFocused = -1;

    do
    {
        nb::TStrings *BookmarkPaths = new nb::TStringList();
        TFarMenuItems *BookmarkItems = new TFarMenuItems();
        nb::TList *Bookmarks = new nb::TList();
        {
            BOOST_SCOPE_EXIT ( (&BookmarkItems) (&Bookmarks) (&BookmarkPaths) )
            {
                delete BookmarkItems;
                delete Bookmarks;
                delete BookmarkPaths;
            } BOOST_SCOPE_EXIT_END
            size_t BookmarksOffset = -1;

            size_t MaxLength = FPlugin->MaxMenuItemLength();
            size_t MaxHistory = 40;
            size_t FirstHistory = 0;

            if (FPathHistory->GetCount() > MaxHistory)
            {
                FirstHistory = FPathHistory->GetCount() - MaxHistory + 1;
            }

            for (size_t i = FirstHistory; i < FPathHistory->GetCount(); i++)
            {
                std::wstring Path = FPathHistory->GetString(i);
                BookmarkPaths->Add(Path);
                BookmarkItems->Add(MinimizeName(Path, MaxLength, true));
            }

            size_t FirstItemFocused = -1;
            nb::TStringList *BookmarkDirectories = new nb::TStringList();
            {
                BOOST_SCOPE_EXIT ( (&BookmarkDirectories) )
                {
                    delete BookmarkDirectories;
                } BOOST_SCOPE_EXIT_END
                BookmarkDirectories->SetSorted(true);
                for (size_t i = 0; i < BookmarkList->GetCount(); i++)
                {
                    TBookmark *Bookmark = BookmarkList->GetBookmark(i);
                    std::wstring RemoteDirectory = Bookmark->GetRemote();
                    if (!RemoteDirectory.empty() && (BookmarkDirectories->IndexOf(RemoteDirectory.c_str()) == -1))
                    {
                        size_t Pos;
                        Pos = BookmarkDirectories->Add(RemoteDirectory);
                        if (RemoteDirectory == Directory)
                        {
                            FirstItemFocused = Pos;
                        }
                        else if ((FirstItemFocused != -1) && (FirstItemFocused >= Pos))
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

                    if (FirstItemFocused != -1)
                    {
                        FirstItemFocused += BookmarkItems->GetCount();
                    }
                    else
                    {
                        FirstItemFocused = BookmarkItems->GetCount();
                    }

                    for (size_t ii = 0; ii < BookmarkDirectories->GetCount(); ii++)
                    {
                        std::wstring Path = BookmarkDirectories->GetString(ii);
                        BookmarkItems->Add(Path);
                        BookmarkPaths->Add(MinimizeName(Path, MaxLength, true));
                    }
                }
            }

            if (ItemFocused == -1)
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

            int BreakCode;

            Repeat = false;
            std::wstring Caption = GetMsg(Add ? OPEN_DIRECTORY_ADD_BOOMARK_ACTION :
                                          OPEN_DIRECTORY_BROWSE_CAPTION);
            const FarKey BreakKeys[] = {
              { VK_DELETE, 0 },
              { VK_F8, 0},
              { VK_RETURN + (LEFT_CTRL_PRESSED << 16), 0},
              { 'C' + (LEFT_CTRL_PRESSED << 16), 0},
              { VK_INSERT + (LEFT_CTRL_PRESSED << 16), 0},
              { 0 }
            };

            ItemFocused = FPlugin->Menu(FMENU_REVERSEAUTOHIGHLIGHT | FMENU_SHOWAMPERSAND | FMENU_WRAPMODE,
                                        Caption, GetMsg(OPEN_DIRECTORY_HELP), BookmarkItems, BreakKeys, BreakCode);
            if (BreakCode >= 0)
            {
                assert(BreakCode >= 0 && BreakCode <= 4);
                if ((BreakCode == 0) || (BreakCode == 1))
                {
                    assert(ItemFocused != -1);
                    if (ItemFocused >= BookmarksOffset)
                    {
                        TBookmark *Bookmark = static_cast<TBookmark *>(Bookmarks->GetItem(ItemFocused - BookmarksOffset));
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
                    FarControl(FCTL_INSERTCMDLINE, 0, reinterpret_cast<void *>(
                        const_cast<wchar_t *>(BookmarkPaths->GetString(ItemFocused).c_str())));
                }
                else if (BreakCode == 3 || BreakCode == 4)
                {
                    FPlugin->FarCopyToClipboard(BookmarkPaths->GetString(ItemFocused));
                    Repeat = true;
                }
            }
            else if (ItemFocused != -1)
            {
                Directory = BookmarkPaths->GetString(ItemFocused);
                if (Directory.empty())
                {
                    // empty trailing line in no-bookmark mode selected
                    ItemFocused = -1;
                }
            }

            Result = (BreakCode < 0) && (ItemFocused != -1);
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
    TApplyCommandDialog(TCustomFarPlugin *AFarPlugin);

    bool Execute(std::wstring &Command, int &Params);

protected:
    virtual void Change();

private:
    int FParams;

    TFarEdit *CommandEdit;
    TFarText *LocalHintText;
    TFarRadioButton *RemoteCommandButton;
    TFarRadioButton *LocalCommandButton;
    TFarCheckBox *ApplyToDirectoriesCheck;
    TFarCheckBox *RecursiveCheck;
    TFarCheckBox *ShowResultsCheck;
    TFarCheckBox *CopyResultsCheck;

    std::wstring FPrompt;
    TFarEdit *PasswordEdit;
    TFarEdit *NormalEdit;
    TFarCheckBox *HideTypingCheck;
};
//---------------------------------------------------------------------------
TApplyCommandDialog::TApplyCommandDialog(TCustomFarPlugin *AFarPlugin) :
    TWinSCPDialog(AFarPlugin)
{
    TFarText *Text;

    SetSize(nb::TPoint(76, 18));
    SetCaption(GetMsg(APPLY_COMMAND_TITLE));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_PROMPT));

    CommandEdit = new TFarEdit(this);
    CommandEdit->SetHistory(APPLY_COMMAND_HISTORY);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_HINT1));
    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_HINT2));
    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_HINT3));
    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_HINT4));
    Text = new TFarText(this);
    Text->SetCaption(GetMsg(APPLY_COMMAND_HINT5));
    LocalHintText = new TFarText(this);
    LocalHintText->SetCaption(GetMsg(APPLY_COMMAND_HINT_LOCAL));

    new TFarSeparator(this);

    RemoteCommandButton = new TFarRadioButton(this);
    RemoteCommandButton->SetCaption(GetMsg(APPLY_COMMAND_REMOTE_COMMAND));

    SetNextItemPosition(ipRight);

    LocalCommandButton = new TFarRadioButton(this);
    LocalCommandButton->SetCaption(GetMsg(APPLY_COMMAND_LOCAL_COMMAND));

    LocalHintText->SetEnabledDependency(LocalCommandButton);

    SetNextItemPosition(ipNewLine);

    ApplyToDirectoriesCheck = new TFarCheckBox(this);
    ApplyToDirectoriesCheck->SetCaption(
        GetMsg(APPLY_COMMAND_APPLY_TO_DIRECTORIES));

    SetNextItemPosition(ipRight);

    RecursiveCheck = new TFarCheckBox(this);
    RecursiveCheck->SetCaption(GetMsg(APPLY_COMMAND_RECURSIVE));

    SetNextItemPosition(ipNewLine);

    ShowResultsCheck = new TFarCheckBox(this);
    ShowResultsCheck->SetCaption(GetMsg(APPLY_COMMAND_SHOW_RESULTS));
    ShowResultsCheck->SetEnabledDependency(RemoteCommandButton);

    SetNextItemPosition(ipRight);

    CopyResultsCheck = new TFarCheckBox(this);
    CopyResultsCheck->SetCaption(GetMsg(APPLY_COMMAND_COPY_RESULTS));
    CopyResultsCheck->SetEnabledDependency(RemoteCommandButton);

    AddStandardButtons();

    OkButton->SetEnabledDependency(CommandEdit);
}
//---------------------------------------------------------------------------
void TApplyCommandDialog::Change()
{
    TWinSCPDialog::Change();

    if (GetHandle())
    {
        bool RemoteCommand = RemoteCommandButton->GetChecked();
        bool AllowRecursive = true;
        bool AllowApplyToDirectories = true;
        try
        {
            TRemoteCustomCommand RemoteCustomCommand;
            TLocalCustomCommand LocalCustomCommand;
            TFileCustomCommand *FileCustomCommand =
                (RemoteCommand ? &RemoteCustomCommand : &LocalCustomCommand);

            TInteractiveCustomCommand InteractiveCustomCommand(FileCustomCommand);
            std::wstring Cmd = InteractiveCustomCommand.Complete(CommandEdit->GetText(), false);
            bool FileCommand = FileCustomCommand->IsFileCommand(Cmd);
            AllowRecursive = FileCommand && !FileCustomCommand->IsFileListCommand(Cmd);
            if (AllowRecursive && !RemoteCommand)
            {
                AllowRecursive = !LocalCustomCommand.HasLocalFileName(Cmd);
            }
            AllowApplyToDirectories = FileCommand;
        }
        catch (...)
        {
        }

        RecursiveCheck->SetEnabled(AllowRecursive);
        ApplyToDirectoriesCheck->SetEnabled(AllowApplyToDirectories);
    }
}
//---------------------------------------------------------------------------
bool TApplyCommandDialog::Execute(std::wstring &Command, int &Params)
{
    CommandEdit->SetText(Command);
    FParams = Params;
    RemoteCommandButton->SetChecked(FLAGCLEAR(Params, ccLocal));
    LocalCommandButton->SetChecked(FLAGSET(Params, ccLocal));
    ApplyToDirectoriesCheck->SetChecked(FLAGSET(Params, ccApplyToDirectories));
    RecursiveCheck->SetChecked(FLAGSET(Params, ccRecursive));
    ShowResultsCheck->SetChecked(FLAGSET(Params, ccShowResults));
    CopyResultsCheck->SetChecked(FLAGSET(Params, ccCopyResults));

    bool Result = (ShowModal() != brCancel);
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
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::ApplyCommandDialog(std::wstring &Command,
        int &Params)
{
    bool Result;
    TApplyCommandDialog *Dialog = new TApplyCommandDialog(FPlugin);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Command, Params);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TFullSynchronizeDialog : public TWinSCPDialog
{
public:
    TFullSynchronizeDialog(TCustomFarPlugin *AFarPlugin, int Options,
                           const TUsableCopyParamAttrs &CopyParamAttrs);

    bool Execute(TTerminal::TSynchronizeMode &Mode,
                 int &Params, std::wstring &LocalDirectory, std::wstring &RemoteDirectory,
                 TCopyParamType *CopyParams, bool &SaveSettings, bool &SaveMode);

protected:
    virtual bool CloseQuery();
    virtual void Change();
    virtual long DialogProc(int Msg, int Param1, void *Param2);

    void TransferSettingsButtonClick(TFarButton *Sender, bool &Close);
    void CopyParamListerClick(TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);

    int ActualCopyParamAttrs();
    void CustomCopyParam();
    void AdaptSize();

private:
    TFarEdit *LocalDirectoryEdit;
    TFarEdit *RemoteDirectoryEdit;
    TFarRadioButton *SynchronizeBothButton;
    TFarRadioButton *SynchronizeRemoteButton;
    TFarRadioButton *SynchronizeLocalButton;
    TFarRadioButton *SynchronizeFilesButton;
    TFarRadioButton *MirrorFilesButton;
    TFarRadioButton *SynchronizeTimestampsButton;
    TFarCheckBox *SynchronizeDeleteCheck;
    TFarCheckBox *SynchronizeExistingOnlyCheck;
    TFarCheckBox *SynchronizeSelectedOnlyCheck;
    TFarCheckBox *SynchronizePreviewChangesCheck;
    TFarCheckBox *SynchronizeByTimeCheck;
    TFarCheckBox *SynchronizeBySizeCheck;
    TFarCheckBox *SaveSettingsCheck;
    TFarLister *CopyParamLister;

    bool FSaveMode;
    int FOptions;
    int FFullHeight;
    TTerminal::TSynchronizeMode FOrigMode;
    TUsableCopyParamAttrs FCopyParamAttrs;
    TCopyParamType FCopyParams;

    TTerminal::TSynchronizeMode GetMode();
};
//---------------------------------------------------------------------------
TFullSynchronizeDialog::TFullSynchronizeDialog(
    TCustomFarPlugin *AFarPlugin, int Options,
    const TUsableCopyParamAttrs &CopyParamAttrs) :
    TWinSCPDialog(AFarPlugin)
{
    FOptions = Options;
    FCopyParamAttrs = CopyParamAttrs;

    TFarText *Text;
    TFarSeparator *Separator;

    SetSize(nb::TPoint(78, 25));
    SetCaption(GetMsg(FULL_SYNCHRONIZE_TITLE));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(FULL_SYNCHRONIZE_LOCAL_LABEL));

    LocalDirectoryEdit = new TFarEdit(this);
    LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(FULL_SYNCHRONIZE_REMOTE_LABEL));

    RemoteDirectoryEdit = new TFarEdit(this);
    RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_DIRECTION_GROUP));

    SynchronizeBothButton = new TFarRadioButton(this);
    SynchronizeBothButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_BOTH));

    SetNextItemPosition(ipRight);

    SynchronizeRemoteButton = new TFarRadioButton(this);
    SynchronizeRemoteButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_REMOTE));

    SynchronizeLocalButton = new TFarRadioButton(this);
    SynchronizeLocalButton->SetCaption(GetMsg(FULL_SYNCHRONIZE_LOCAL));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_MODE_GROUP));

    SynchronizeFilesButton = new TFarRadioButton(this);
    SynchronizeFilesButton->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE_FILES));

    SetNextItemPosition(ipRight);

    MirrorFilesButton = new TFarRadioButton(this);
    MirrorFilesButton->SetCaption(GetMsg(SYNCHRONIZE_MIRROR_FILES));

    SynchronizeTimestampsButton = new TFarRadioButton(this);
    SynchronizeTimestampsButton->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE_TIMESTAMPS));
    SynchronizeTimestampsButton->SetEnabled(FLAGCLEAR(Options, fsoDisableTimestamp));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_GROUP));

    SynchronizeDeleteCheck = new TFarCheckBox(this);
    SynchronizeDeleteCheck->SetCaption(GetMsg(SYNCHRONIZE_DELETE));

    SetNextItemPosition(ipRight);

    SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
    SynchronizeExistingOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_EXISTING_ONLY));
    SynchronizeExistingOnlyCheck->SetEnabledDependencyNegative(SynchronizeTimestampsButton);

    SetNextItemPosition(ipNewLine);

    SynchronizePreviewChangesCheck = new TFarCheckBox(this);
    SynchronizePreviewChangesCheck->SetCaption(GetMsg(SYNCHRONIZE_PREVIEW_CHANGES));

    SetNextItemPosition(ipRight);

    SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
    SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_SELECTED_ONLY));
    SynchronizeSelectedOnlyCheck->SetEnabled(FLAGSET(FOptions, fsoAllowSelectedOnly));

    SetNextItemPosition(ipNewLine);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(FULL_SYNCHRONIZE_CRITERIONS_GROUP));

    SynchronizeByTimeCheck = new TFarCheckBox(this);
    SynchronizeByTimeCheck->SetCaption(GetMsg(SYNCHRONIZE_BY_TIME));

    SetNextItemPosition(ipRight);

    SynchronizeBySizeCheck = new TFarCheckBox(this);
    SynchronizeBySizeCheck->SetCaption(GetMsg(SYNCHRONIZE_BY_SIZE));
    SynchronizeBySizeCheck->SetEnabledDependencyNegative(SynchronizeBothButton);

    SetNextItemPosition(ipNewLine);

    new TFarSeparator(this);

    SaveSettingsCheck = new TFarCheckBox(this);
    SaveSettingsCheck->SetCaption(GetMsg(SYNCHRONIZE_REUSE_SETTINGS));

    Separator = new TFarSeparator(this);
    Separator->SetGroup(1);
    Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

    CopyParamLister = new TFarLister(this);
    CopyParamLister->SetHeight(3);
    CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
    CopyParamLister->SetTabStop(false);
    CopyParamLister->SetOnMouseClick(boost::bind(&TFullSynchronizeDialog::CopyParamListerClick, this, _1, _2));
    CopyParamLister->SetGroup(1);
    // Right edge is adjusted in Change

    // align buttons with bottom of the window
    Separator = new TFarSeparator(this);
    Separator->SetPosition(-4);

    TFarButton *Button = new TFarButton(this);
    Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
    Button->SetResult(-1);
    Button->SetCenterGroup(true);
    Button->SetOnClick(boost::bind(&TFullSynchronizeDialog::TransferSettingsButtonClick, this, _1, _2));

    SetNextItemPosition(ipRight);

    AddStandardButtons(0, true);

    FFullHeight = GetSize().y;
    AdaptSize();
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::AdaptSize()
{
    bool ShowCopyParam = (FFullHeight <= GetMaxSize().y);
    if (ShowCopyParam != CopyParamLister->GetVisible())
    {
        ShowGroup(1, ShowCopyParam);
        SetHeight(FFullHeight - (ShowCopyParam ? 0 : CopyParamLister->GetHeight() + 1));
    }
}
//---------------------------------------------------------------------------
TTerminal::TSynchronizeMode TFullSynchronizeDialog::GetMode()
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
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::TransferSettingsButtonClick(
    TFarButton * /*Sender*/, bool &Close)
{
    CustomCopyParam();
    Close = false;
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::CopyParamListerClick(
    TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD *Event)
{
    if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
    {
        CustomCopyParam();
    }
}
//---------------------------------------------------------------------------
void TFullSynchronizeDialog::CustomCopyParam()
{
    TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
    if (WinSCPPlugin->CopyParamCustomDialog(FCopyParams, ActualCopyParamAttrs()))
    {
        Change();
    }
}
//---------------------------------------------------------------------------
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
                                           GetMsg(SYNCHRONIZE_SAME_SIZE) : GetMsg(SYNCHRONIZE_BY_SIZE));

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

        std::wstring InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
        nb::TStringList *InfoStrLines = new nb::TStringList();
        {
            BOOST_SCOPE_EXIT ( (&InfoStrLines) )
            {
                delete InfoStrLines;
            } BOOST_SCOPE_EXIT_END
            FarWrapText(InfoStr, InfoStrLines, GetBorderBox()->GetWidth() - 4);
            CopyParamLister->SetItems(InfoStrLines);
            CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
        }
    }
}
//---------------------------------------------------------------------------
int TFullSynchronizeDialog::ActualCopyParamAttrs()
{
    int Result;
    if (SynchronizeTimestampsButton->GetChecked())
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
bool TFullSynchronizeDialog::CloseQuery()
{
    bool CanClose = TWinSCPDialog::CloseQuery();

    if (CanClose && (GetResult() == brOK) &&
            SaveSettingsCheck->GetChecked() && (FOrigMode != GetMode()) && !FSaveMode)
    {
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);

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
long TFullSynchronizeDialog::DialogProc(int Msg, int Param1, void *Param2)
{
    if (Msg == DN_RESIZECONSOLE)
    {
        AdaptSize();
    }

    return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
bool TFullSynchronizeDialog::Execute(TTerminal::TSynchronizeMode &Mode,
                                     int &Params, std::wstring &LocalDirectory, std::wstring &RemoteDirectory,
                                     TCopyParamType *CopyParams, bool &SaveSettings, bool &SaveMode)
{
    LocalDirectoryEdit->SetText(LocalDirectory);
    RemoteDirectoryEdit->SetText(RemoteDirectory);
    SynchronizeRemoteButton->SetChecked((Mode == TTerminal::smRemote));
    SynchronizeLocalButton->SetChecked((Mode == TTerminal::smLocal));
    SynchronizeBothButton->SetChecked((Mode == TTerminal::smBoth));
    SynchronizeDeleteCheck->SetChecked(FLAGSET(Params, TTerminal::spDelete));
    SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params, TTerminal::spExistingOnly));
    SynchronizePreviewChangesCheck->SetChecked(FLAGSET(Params, TTerminal::spPreviewChanges));
    SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params, spSelectedOnly));
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

    bool Result = (ShowModal() == brOK);

    if (Result)
    {
        RemoteDirectory = RemoteDirectoryEdit->GetText();
        LocalDirectory = LocalDirectoryEdit->GetText();

        Mode = GetMode();

        Params &= ~(TTerminal::spDelete | TTerminal::spNoConfirmation |
                    TTerminal::spExistingOnly | TTerminal::spPreviewChanges |
                    TTerminal::spTimestamp | TTerminal::spNotByTime | TTerminal::spBySize |
                    spSelectedOnly | TTerminal::spMirror);
        Params |=
            FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
            FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
            FLAGMASK(SynchronizePreviewChangesCheck->GetChecked(), TTerminal::spPreviewChanges) |
            FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), spSelectedOnly) |
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
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::FullSynchronizeDialog(TTerminal::TSynchronizeMode &Mode,
        int &Params, std::wstring &LocalDirectory, std::wstring &RemoteDirectory,
        TCopyParamType *CopyParams, bool &SaveSettings, bool &SaveMode, int Options,
        const TUsableCopyParamAttrs &CopyParamAttrs)
{
    bool Result;
    TFullSynchronizeDialog *Dialog = new TFullSynchronizeDialog(
        FPlugin, Options, CopyParamAttrs);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Mode, Params, LocalDirectory, RemoteDirectory,
                                 CopyParams, SaveSettings, SaveMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSynchronizeChecklistDialog : public TWinSCPDialog
{
public:
    TSynchronizeChecklistDialog(
        TCustomFarPlugin *AFarPlugin, TTerminal::TSynchronizeMode Mode, int Params,
        const std::wstring LocalDirectory, const std::wstring RemoteDirectory);

    bool Execute(TSynchronizeChecklist *Checklist);

protected:
    virtual long DialogProc(int Msg, int Param1, void *Param2);
    virtual bool Key(TFarDialogItem *Item, long KeyCode);
    void CheckAllButtonClick(TFarButton *Sender, bool &Close);
    void VideoModeButtonClick(TFarButton *Sender, bool &Close);
    void ListBoxClick(TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);

private:
    TFarText *Header;
    TFarListBox *ListBox;
    TFarButton *CheckAllButton;
    TFarButton *UncheckAllButton;
    TFarButton *VideoModeButton;

    TSynchronizeChecklist *FChecklist;
    std::wstring FLocalDirectory;
    std::wstring FRemoteDirectory;
    static const size_t FColumns = 8;
    int FWidths[FColumns];
    std::wstring FActions[TSynchronizeChecklist::ActionCount];
    int FScroll;
    bool FCanScrollRight;
    size_t FChecked;

    void AdaptSize();
    int ColumnWidth(size_t Index);
    void LoadChecklist();
    void RefreshChecklist(bool Scroll);
    void UpdateControls();
    void CheckAll(bool Check);
    std::wstring ItemLine(const TSynchronizeChecklist::TItem *ChecklistItem);
    void AddColumn(std::wstring &List, std::wstring Value, size_t Column,
                   bool Header = false);
    std::wstring FormatSize(__int64 Size, int Column);
};
//---------------------------------------------------------------------------
TSynchronizeChecklistDialog::TSynchronizeChecklistDialog(
    TCustomFarPlugin *AFarPlugin, TTerminal::TSynchronizeMode /*Mode*/, int /*Params*/,
    const std::wstring LocalDirectory, const std::wstring RemoteDirectory) :
    TWinSCPDialog(AFarPlugin),
    FChecklist(NULL),
    FLocalDirectory(LocalDirectory),
    FRemoteDirectory(RemoteDirectory),
    FScroll(0),
    FCanScrollRight(false)
{
    SetCaption(GetMsg(CHECKLIST_TITLE));

    Header = new TFarText(this);
    Header->SetOem(true);

    ListBox = new TFarListBox(this);
    ListBox->SetNoBox(true);
    // align list with bottom of the window
    ListBox->SetBottom(-5);
    ListBox->SetOnMouseClick(boost::bind(&TSynchronizeChecklistDialog::ListBoxClick, this, _1, _2));
    ListBox->SetOem(true);

    std::wstring Actions = GetMsg(CHECKLIST_ACTIONS);
    int Action = 0;
    while (!Actions.empty() && (Action < LENOF(FActions)))
    {
        FActions[Action] = CutToChar(Actions, '|', false);
        Action++;
    }

    // align buttons with bottom of the window
    ButtonSeparator = new TFarSeparator(this);
    ButtonSeparator->SetTop(-4);
    ButtonSeparator->SetBottom(ButtonSeparator->GetTop());

    CheckAllButton = new TFarButton(this);
    CheckAllButton->SetCaption(GetMsg(CHECKLIST_CHECK_ALL));
    CheckAllButton->SetCenterGroup(true);
    CheckAllButton->SetOnClick(boost::bind(&TSynchronizeChecklistDialog::CheckAllButtonClick, this, _1, _2));

    SetNextItemPosition(ipRight);

    UncheckAllButton = new TFarButton(this);
    UncheckAllButton->SetCaption(GetMsg(CHECKLIST_UNCHECK_ALL));
    UncheckAllButton->SetCenterGroup(true);
    UncheckAllButton->SetOnClick(boost::bind(&TSynchronizeChecklistDialog::CheckAllButtonClick, this, _1, _2));

    VideoModeButton = new TFarButton(this);
    VideoModeButton->SetCenterGroup(true);
    VideoModeButton->SetOnClick(boost::bind(&TSynchronizeChecklistDialog::VideoModeButtonClick, this, _1, _2));

    AddStandardButtons(0, true);

    AdaptSize();
    UpdateControls();
    ListBox->SetFocus();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::AddColumn(std::wstring &List,
        std::wstring Value, size_t Column, bool Header)
{
    // OEM character set (Ansi does not have the ascii art we need)
    char Separator = '\xB3';
    StrToFar(Value);
    size_t Len = Value.size();
    int Width = FWidths[Column];
    bool Right = (Column == 2) || (Column == 3) || (Column == 6) || (Column == 7);
    bool LastCol = (Column == FColumns - 1);
    if (Len <= Width)
    {
        size_t Added = 0;
        if (Header && (Len < Width))
        {
            Added += (Width - Len) / 2;
        }
        else if (Right && (Len < Width))
        {
            Added += Width - Len;
        }
        List += ::StringOfChar(' ', Added) + Value;
        Added += Value.size();
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
        int Scroll = FScroll;
        if ((Scroll > 0) && !Header)
        {
            if (List.empty())
            {
                List += '{';
                Width--;
                Scroll++;
            }
            else
            {
                List[List.size() - 1] = '{';
            }
        }
        if (Scroll > Len - Width)
        {
            Scroll = static_cast<int>(Len - Width);
        }
        else if (!Header && LastCol && (Scroll < Len - Width))
        {
            Width--;
        }
        List += Value.substr(Scroll + 1, Width);
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
void TSynchronizeChecklistDialog::AdaptSize()
{
    FScroll = 0;
    SetSize(GetMaxSize());

    VideoModeButton->SetCaption(GetMsg(
                                    FarPlugin->ConsoleWindowState() == SW_SHOWMAXIMIZED ?
                                    CHECKLIST_RESTORE : CHECKLIST_MAXIMIZE));

    static const int Ratio[FColumns] = { 140, 100, 80, 150, -2, 100, 80, 150 };

    int Width = ListBox->GetWidth() - 2 /*checkbox*/ - 1 /*scrollbar*/ - FColumns;
    double Temp[FColumns];

    int TotalRatio = 0;
    int FixedRatio = 0;
    for (size_t Index = 0; Index < FColumns; Index++)
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
    for (size_t Index = 0; Index < FColumns; Index++)
    {
        if (Ratio[Index] >= 0)
        {
            double W = static_cast<double>(Ratio[Index]) * (Width - FixedRatio) / TotalRatio;
            FWidths[Index] = static_cast<int>(floor(W));
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
        for (size_t Index = 0; Index < FColumns; Index++)
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
std::wstring TSynchronizeChecklistDialog::FormatSize(
    __int64 Size, int Column)
{
    size_t Width = FWidths[Column];
    std::wstring Result = FORMAT(L"%lu", Size);

    if (Result.size() > Width)
    {
        Result = FORMAT(L"%.2f 'K'", Size / 1024.0);
        if (Result.size() > Width)
        {
            Result = FORMAT(L"%.2f 'M'", Size / (1024.0 * 1024));
            if (Result.size() > Width)
            {
                Result = FORMAT(L"%.2f 'G'", Size / (1024.0 * 1024 * 1024));
                if (Result.size() > Width)
                {
                    // back to default
                    Result = FORMAT(L"%lu", Size);
                }
            }
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
std::wstring TSynchronizeChecklistDialog::ItemLine(
    const TSynchronizeChecklist::TItem *ChecklistItem)
{
    std::wstring Line;
    std::wstring S;

    S = ChecklistItem->GetFileName();
    if (ChecklistItem->IsDirectory)
    {
        S = IncludeTrailingBackslash(S);
    }
    AddColumn(Line, S, 0);

    if (ChecklistItem->Action == TSynchronizeChecklist::saDeleteRemote)
    {
        AddColumn(Line, L"", 1);
        AddColumn(Line, L"", 2);
        AddColumn(Line, L"", 3);
    }
    else
    {
        S = ChecklistItem->Local.Directory;
        if (AnsiSameText(FLocalDirectory, S.substr(0, FLocalDirectory.size())))
        {
            S[0] = '.';
            S.erase(1, FLocalDirectory.size() - 1);
        }
        else
        {
            assert(false);
        }
        AddColumn(Line, S, 1);
        if (ChecklistItem->Action == TSynchronizeChecklist::saDownloadNew)
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
            AddColumn(Line, UserModificationStr(ChecklistItem->Local.Modification,
                                                ChecklistItem->Local.ModificationFmt), 3);
        }
    }

    size_t Action = ChecklistItem->Action - 1;
    assert((Action != -1) && (Action < LENOF(FActions)));
    AddColumn(Line, FActions[Action], 4);

    if (ChecklistItem->Action == TSynchronizeChecklist::saDeleteLocal)
    {
        AddColumn(Line, L"", 5);
        AddColumn(Line, L"", 6);
        AddColumn(Line, L"", 7);
    }
    else
    {
        S = ChecklistItem->Remote.Directory;
        if (AnsiSameText(FRemoteDirectory, S.substr(0, FRemoteDirectory.size())))
        {
            S[0] = '.';
            S.erase(1, FRemoteDirectory.size() - 1);
        }
        else
        {
            assert(false);
        }
        AddColumn(Line, S, 5);
        if (ChecklistItem->Action == TSynchronizeChecklist::saUploadNew)
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
            AddColumn(Line, UserModificationStr(ChecklistItem->Remote.Modification,
                                                ChecklistItem->Remote.ModificationFmt), 7);
        }
    }

    return Line;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::LoadChecklist()
{
    FChecked = 0;
    TFarList *List = new TFarList();
    {
        BOOST_SCOPE_EXIT ( (&List) )
        {
            delete List;
        } BOOST_SCOPE_EXIT_END
        List->BeginUpdate();
        for (size_t Index = 0; Index < FChecklist->GetCount(); Index++)
        {
            const TSynchronizeChecklist::TItem *ChecklistItem = FChecklist->GetItem(Index);

            List->AddObject(ItemLine(ChecklistItem),
                            const_cast<nb::TObject *>(reinterpret_cast<const nb::TObject *>(ChecklistItem)));
        }
        List->EndUpdate();

        // items must be checked in second pass once the internal array is allocated
        for (size_t Index = 0; Index < FChecklist->GetCount(); Index++)
        {
            const TSynchronizeChecklist::TItem *ChecklistItem = FChecklist->GetItem(Index);

            List->SetChecked(Index, ChecklistItem->Checked);
            if (ChecklistItem->Checked)
            {
                FChecked++;
            }
        }

        ListBox->SetItems(List);
    }

    UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::RefreshChecklist(bool Scroll)
{
    std::wstring HeaderStr = GetMsg(CHECKLIST_HEADER);
    std::wstring HeaderCaption(::StringOfChar(' ', 2));

    for (size_t Index = 0; Index < FColumns; Index++)
    {
        AddColumn(HeaderCaption, CutToChar(HeaderStr, '|', false), Index, true);
    }
    Header->SetCaption(HeaderCaption);

    FCanScrollRight = false;
    TFarList *List = ListBox->GetItems();
    List->BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&List) )
        {
            List->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        for (size_t Index = 0; Index < List->GetCount(); Index++)
        {
            if (!Scroll || (::LastDelimiter(List->GetString(Index), L"{}") != std::wstring::npos))
            {
                const TSynchronizeChecklist::TItem *ChecklistItem =
                    reinterpret_cast<TSynchronizeChecklist::TItem *>(List->GetObject(Index));

                List->PutString(Index, ItemLine(ChecklistItem));
            }
        }
    }
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::UpdateControls()
{
    ButtonSeparator->SetCaption(
        FORMAT(GetMsg(CHECKLIST_CHECKED).c_str(), FChecked, ListBox->GetItems()->GetCount()));
    CheckAllButton->SetEnabled((FChecked < ListBox->GetItems()->GetCount()));
    UncheckAllButton->SetEnabled((FChecked > 0));
}
//---------------------------------------------------------------------------
long TSynchronizeChecklistDialog::DialogProc(int Msg, int Param1, void *Param2)
{
    if (Msg == DN_RESIZECONSOLE)
    {
        AdaptSize();
    }

    return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CheckAll(bool Check)
{
    TFarList *List = ListBox->GetItems();
    List->BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&List) )
        {
            List->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        size_t Count = List->GetCount();
        for (size_t Index = 0; Index < Count; Index++)
        {
            List->SetChecked(Index, Check);
        }

        FChecked = (Check ? Count : 0);
    }

    UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::CheckAllButtonClick(
    TFarButton *Sender, bool &Close)
{
    CheckAll(Sender == CheckAllButton);
    ListBox->SetFocus();

    Close = false;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::VideoModeButtonClick(
    TFarButton * /*Sender*/, bool &Close)
{
    FarPlugin->ToggleVideoMode();

    Close = false;
}
//---------------------------------------------------------------------------
void TSynchronizeChecklistDialog::ListBoxClick(
    TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD * /*Event*/)
{
    size_t Index = ListBox->GetItems()->GetSelected();
    if (Index != -1)
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
//---------------------------------------------------------------------------
bool TSynchronizeChecklistDialog::Key(TFarDialogItem *Item, long KeyCode)
{
    bool Result = false;
    if (ListBox->Focused())
    {
        if ((KeyCode == (VK_ADD | (SHIFT_PRESSED << 16))) || 
            (KeyCode == (VK_SUBTRACT | (SHIFT_PRESSED << 16))))
        {
            CheckAll(KeyCode == (VK_ADD | (SHIFT_PRESSED << 16)));
            Result = true;
        }
        else if ((KeyCode == VK_SPACE) || (KeyCode == VK_INSERT) ||
                 (KeyCode == VK_ADD) || (KeyCode == VK_SUBTRACT))
        {
            size_t Index = ListBox->GetItems()->GetSelected();
            if (Index != -1)
            {
                if (ListBox->GetItems()->GetChecked(Index) && (KeyCode != VK_ADD))
                {
                    ListBox->GetItems()->SetChecked(Index, false);
                    FChecked--;
                }
                else if (!ListBox->GetItems()->GetChecked(Index) && (KeyCode != VK_SUBTRACT))
                {
                    ListBox->GetItems()->SetChecked(Index, true);
                    FChecked++;
                }

                // FAR WORKAROUND
                // Changing "checked" state is not always drawn.
                Redraw();
                UpdateControls();
                if ((KeyCode == VK_INSERT) &&
                    (Index < ListBox->GetItems()->GetCount() - 1))
                {
                    ListBox->GetItems()->SetSelected(Index + 1);
                }
            }
            Result = true;
        }
        else if (KeyCode == (VK_LEFT | (LEFT_ALT_PRESSED << 16)))
        {
            if (FScroll > 0)
            {
                FScroll--;
                RefreshChecklist(true);
            }
            Result = true;
        }
        else if (KeyCode == VK_RIGHT)
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
bool TSynchronizeChecklistDialog::Execute(TSynchronizeChecklist *Checklist)
{
    FChecklist = Checklist;
    LoadChecklist();
    bool Result = (ShowModal() == brOK);

    if (Result)
    {
        TFarList *List = ListBox->GetItems();
        size_t Count = List->GetCount();
        for (size_t Index = 0; Index < Count; Index++)
        {
            TSynchronizeChecklist::TItem *ChecklistItem =
                reinterpret_cast<TSynchronizeChecklist::TItem *>(List->GetObject(Index));
            ChecklistItem->Checked = List->GetChecked(Index);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeChecklistDialog(
    TSynchronizeChecklist *Checklist, TTerminal::TSynchronizeMode Mode, int Params,
    const std::wstring LocalDirectory, const std::wstring RemoteDirectory)
{
    bool Result;
    TSynchronizeChecklistDialog *Dialog = new TSynchronizeChecklistDialog(
        FPlugin, Mode, Params, LocalDirectory, RemoteDirectory);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Checklist);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TSynchronizeDialog : TFarDialog
{
public:
    TSynchronizeDialog(TCustomFarPlugin *AFarPlugin,
                       const synchronizestartstop_slot_type &OnStartStop,
                       int Options, int CopyParamAttrs, const getsynchronizeoptions_slot_type &OnGetOptions);
    virtual ~TSynchronizeDialog();

    bool Execute(TSynchronizeParamType &Params,
                 const TCopyParamType *CopyParams, bool &SaveSettings);

protected:
    virtual void Change();
    void UpdateControls();
    void StartButtonClick(TFarButton *Sender, bool &Close);
    void StopButtonClick(TFarButton *Sender, bool &Close);
    void TransferSettingsButtonClick(TFarButton *Sender, bool &Close);
    void CopyParamListerClick(TFarDialogItem *Item, MOUSE_EVENT_RECORD *Event);
    void Stop();
    void DoStartStop(bool Start, bool Synchronize);
    TSynchronizeParamType GetParams();
    void DoAbort(nb::TObject *Sender, bool Close);
    void DoLog(TSynchronizeController *Controller,
               TSynchronizeLogEntry Entry, const std::wstring Message);
    void DoSynchronizeThreads(nb::TObject *Sender, const nb::threadmethod_slot_type &slot);
    virtual long DialogProc(int Msg, int Param1, void *Param2);
    virtual bool CloseQuery();
    virtual bool Key(TFarDialogItem *Item, long KeyCode);
    TCopyParamType GetCopyParams();
    int ActualCopyParamAttrs();
    void CustomCopyParam();

private:
    bool FSynchronizing;
    bool FStarted;
    bool FAbort;
    bool FClose;
    TSynchronizeParamType FParams;
    synchronizestartstop_signal_type FOnStartStop;
    int FOptions;
    TSynchronizeOptions *FSynchronizeOptions;
    TCopyParamType FCopyParams;
    getsynchronizeoptions_signal_type FOnGetOptions;
    int FCopyParamAttrs;

    TFarEdit *LocalDirectoryEdit;
    TFarEdit *RemoteDirectoryEdit;
    TFarCheckBox *SynchronizeDeleteCheck;
    TFarCheckBox *SynchronizeExistingOnlyCheck;
    TFarCheckBox *SynchronizeSelectedOnlyCheck;
    TFarCheckBox *SynchronizeRecursiveCheck;
    TFarCheckBox *SynchronizeSynchronizeCheck;
    TFarCheckBox *SaveSettingsCheck;
    TFarButton *StartButton;
    TFarButton *StopButton;
    TFarButton *CloseButton;
    TFarLister *CopyParamLister;
};
//---------------------------------------------------------------------------
TSynchronizeDialog::TSynchronizeDialog(TCustomFarPlugin *AFarPlugin,
                                       const synchronizestartstop_slot_type &OnStartStop,
                                       int Options, int CopyParamAttrs, const getsynchronizeoptions_slot_type &OnGetOptions) :
    TFarDialog(AFarPlugin)
{
    TFarText *Text;
    TFarSeparator *Separator;

    FSynchronizing = false;
    FStarted = false;
    FOnStartStop.connect(OnStartStop);
    FAbort = false;
    FClose = false;
    FOptions = Options;
    FOnGetOptions.connect(OnGetOptions);
    FSynchronizeOptions = NULL;
    FCopyParamAttrs = CopyParamAttrs;

    SetSize(nb::TPoint(76, 20));

    SetDefaultGroup(1);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(SYNCHRONIZE_LOCAL_LABEL));

    LocalDirectoryEdit = new TFarEdit(this);
    LocalDirectoryEdit->SetHistory(LOCAL_SYNC_HISTORY);

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(SYNCHRONIZE_REMOTE_LABEL));

    RemoteDirectoryEdit = new TFarEdit(this);
    RemoteDirectoryEdit->SetHistory(REMOTE_SYNC_HISTORY);

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(SYNCHRONIZE_GROUP));
    Separator->SetGroup(0);

    SynchronizeDeleteCheck = new TFarCheckBox(this);
    SynchronizeDeleteCheck->SetCaption(GetMsg(SYNCHRONIZE_DELETE));

    SetNextItemPosition(ipRight);

    SynchronizeExistingOnlyCheck = new TFarCheckBox(this);
    SynchronizeExistingOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_EXISTING_ONLY));

    SetNextItemPosition(ipNewLine);

    SynchronizeRecursiveCheck = new TFarCheckBox(this);
    SynchronizeRecursiveCheck->SetCaption(GetMsg(SYNCHRONIZE_RECURSIVE));

    SetNextItemPosition(ipRight);

    SynchronizeSelectedOnlyCheck = new TFarCheckBox(this);
    SynchronizeSelectedOnlyCheck->SetCaption(GetMsg(SYNCHRONIZE_SELECTED_ONLY));
    // have more complex enable rules
    SynchronizeSelectedOnlyCheck->SetGroup(0);

    SetNextItemPosition(ipNewLine);

    SynchronizeSynchronizeCheck = new TFarCheckBox(this);
    SynchronizeSynchronizeCheck->SetCaption(GetMsg(SYNCHRONIZE_SYNCHRONIZE));
    SynchronizeSynchronizeCheck->SetAllowGrayed(true);

    Separator = new TFarSeparator(this);
    Separator->SetGroup(0);

    SaveSettingsCheck = new TFarCheckBox(this);
    SaveSettingsCheck->SetCaption(GetMsg(SYNCHRONIZE_REUSE_SETTINGS));

    Separator = new TFarSeparator(this);
    Separator->SetCaption(GetMsg(COPY_PARAM_GROUP));

    CopyParamLister = new TFarLister(this);
    CopyParamLister->SetHeight(3);
    CopyParamLister->SetLeft(GetBorderBox()->GetLeft() + 1);
    CopyParamLister->SetTabStop(false);
    CopyParamLister->SetOnMouseClick(boost::bind(&TSynchronizeDialog::CopyParamListerClick, this, _1, _2));
    // Right edge is adjusted in Change

    SetDefaultGroup(0);

    // align buttons with bottom of the window
    Separator = new TFarSeparator(this);
    Separator->SetPosition(-4);

    TFarButton *Button = new TFarButton(this);
    Button->SetCaption(GetMsg(TRANSFER_SETTINGS_BUTTON));
    Button->SetResult(-1);
    Button->SetCenterGroup(true);
    Button->SetOnClick(boost::bind(&TSynchronizeDialog::TransferSettingsButtonClick, this, _1, _2));

    SetNextItemPosition(ipRight);

    StartButton = new TFarButton(this);
    StartButton->SetCaption(GetMsg(SYNCHRONIZE_START_BUTTON));
    StartButton->SetDefault(true);
    StartButton->SetCenterGroup(true);
    StartButton->SetOnClick(boost::bind(&TSynchronizeDialog::StartButtonClick, this, _1, _2));

    StopButton = new TFarButton(this);
    StopButton->SetCaption(GetMsg(SYNCHRONIZE_STOP_BUTTON));
    StopButton->SetCenterGroup(true);
    StopButton->SetOnClick(boost::bind(&TSynchronizeDialog::StopButtonClick, this, _1, _2));

    SetNextItemPosition(ipRight);

    CloseButton = new TFarButton(this);
    CloseButton->SetCaption(GetMsg(MSG_BUTTON_CLOSE));
    CloseButton->SetResult(brCancel);
    CloseButton->SetCenterGroup(true);
}
//---------------------------------------------------------------------------
TSynchronizeDialog::~TSynchronizeDialog()
{
    delete FSynchronizeOptions;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::TransferSettingsButtonClick(
    TFarButton * /*Sender*/, bool &Close)
{
    CustomCopyParam();
    Close = false;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::CopyParamListerClick(
    TFarDialogItem * /*Item*/, MOUSE_EVENT_RECORD *Event)
{
    if (FLAGSET(Event->dwEventFlags, DOUBLE_CLICK))
    {
        CustomCopyParam();
    }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::CustomCopyParam()
{
    TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
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
bool TSynchronizeDialog::Execute(TSynchronizeParamType &Params,
                                 const TCopyParamType *CopyParams, bool &SaveSettings)
{
    RemoteDirectoryEdit->SetText(Params.RemoteDirectory);
    LocalDirectoryEdit->SetText(Params.LocalDirectory);
    SynchronizeDeleteCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spDelete));
    SynchronizeExistingOnlyCheck->SetChecked(FLAGSET(Params.Params, TTerminal::spExistingOnly));
    SynchronizeSelectedOnlyCheck->SetChecked(FLAGSET(Params.Params, spSelectedOnly));
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
//---------------------------------------------------------------------------
TSynchronizeParamType TSynchronizeDialog::GetParams()
{
    TSynchronizeParamType Result = FParams;
    Result.RemoteDirectory = RemoteDirectoryEdit->GetText();
    Result.LocalDirectory = LocalDirectoryEdit->GetText();
    Result.Params =
        (Result.Params & ~(TTerminal::spDelete | TTerminal::spExistingOnly |
                           spSelectedOnly | TTerminal::spTimestamp)) |
        FLAGMASK(SynchronizeDeleteCheck->GetChecked(), TTerminal::spDelete) |
        FLAGMASK(SynchronizeExistingOnlyCheck->GetChecked(), TTerminal::spExistingOnly) |
        FLAGMASK(SynchronizeSelectedOnlyCheck->GetChecked(), spSelectedOnly);
    Result.Options =
        (Result.Options & ~(soRecurse | soSynchronize | soSynchronizeAsk)) |
        FLAGMASK(SynchronizeRecursiveCheck->GetChecked(), soRecurse) |
        FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_CHECKED, soSynchronize) |
        FLAGMASK(SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE, soSynchronizeAsk);
    return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoStartStop(bool Start, bool Synchronize)
{
    if (!FOnStartStop.empty())
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
        FOnStartStop(this, Start, SParams, GetCopyParams(), FSynchronizeOptions,
                     boost::bind(&TSynchronizeDialog::DoAbort, this, _1, _2),
                     boost::bind(&TSynchronizeDialog::DoSynchronizeThreads, this, _1, _2),
                     boost::bind(&TSynchronizeDialog::DoLog, this, _1, _2, _3));
    }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoSynchronizeThreads(nb::TObject * /*Sender*/,
        const nb::threadmethod_slot_type &slot)
{
    if (FStarted)
    {
        Synchronize(slot);
    }
}
//---------------------------------------------------------------------------
long TSynchronizeDialog::DialogProc(int Msg, int Param1, void *Param2)
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
            assert(CloseButton->GetEnabled());
            Close(CloseButton);
        }
    }

    return TFarDialog::DialogProc(Msg, Param1, Param2);
}
//---------------------------------------------------------------------------
bool TSynchronizeDialog::CloseQuery()
{
    return TFarDialog::CloseQuery() && !FSynchronizing;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoAbort(nb::TObject * /*Sender*/, bool Close)
{
    FAbort = true;
    FClose = Close;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::DoLog(TSynchronizeController * /*Controller*/,
                               TSynchronizeLogEntry /*Entry*/, const std::wstring /*Message*/)
{
    // void
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::StartButtonClick(TFarButton * /*Sender*/,
        bool & /*Close*/)
{
    bool Synchronize = false;
    bool Continue = true;
    if (SynchronizeSynchronizeCheck->GetSelected() == BSTATE_3STATE)
    {
        TMessageParams Params;
        Params.Params = qpNeverAskAgainCheck;
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
        switch (WinSCPPlugin->MoreMessageDialog(GetMsg(SYNCHRONISE_BEFORE_KEEPUPTODATE),
                                                NULL, qtConfirmation, qaYes | qaNo | qaCancel, &Params))
        {
        case qaNeverAskAgain:
            SynchronizeSynchronizeCheck->SetSelected(BSTATE_CHECKED);
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
        Synchronize = SynchronizeSynchronizeCheck->GetChecked();
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
        catch (ExtException &E)
        {
            FSynchronizing = false;
            UpdateControls();

            DEBUG_PRINTF(L"before FarPlugin->HandleException");
            FarPlugin->HandleException(&E);
        }
    }
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::StopButtonClick(TFarButton * /*Sender*/,
        bool & /*Close*/)
{
    Stop();
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Stop()
{
    FSynchronizing = false;
    FStarted = false;
    BreakSynchronize();
    DoStartStop(false, false);
    UpdateControls();
    StartButton->SetFocus();
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::Change()
{
    TFarDialog::Change();

    if (GetHandle() && !ChangesLocked())
    {
        UpdateControls();

        std::wstring InfoStr = FCopyParams.GetInfoStr(L"; ", ActualCopyParamAttrs());
        nb::TStringList *InfoStrLines = new nb::TStringList();
        {
            BOOST_SCOPE_EXIT ( (&InfoStrLines) )
            {
                delete InfoStrLines;
            } BOOST_SCOPE_EXIT_END
            FarWrapText(InfoStr, InfoStrLines, GetBorderBox()->GetWidth() - 4);
            CopyParamLister->SetItems(InfoStrLines);
            CopyParamLister->SetRight(GetBorderBox()->GetRight() - (CopyParamLister->GetScrollBar() ? 0 : 1));
        }
    }
}
//---------------------------------------------------------------------------
bool TSynchronizeDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
    bool Result = false;
    if ((KeyCode == VK_ESCAPE) && FSynchronizing)
    {
        Stop();
        Result = true;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TSynchronizeDialog::UpdateControls()
{
    SetCaption(GetMsg(FSynchronizing ? SYNCHRONIZE_SYCHRONIZING : SYNCHRONIZE_TITLE));
    StartButton->SetEnabled(!FSynchronizing);
    StopButton->SetEnabled(FSynchronizing);
    CloseButton->SetEnabled(!FSynchronizing);
    EnableGroup(1, !FSynchronizing);
    SynchronizeSelectedOnlyCheck->SetEnabled(
        !FSynchronizing && FLAGSET(FOptions, soAllowSelectedOnly));
}
//---------------------------------------------------------------------------
TCopyParamType TSynchronizeDialog::GetCopyParams()
{
    TCopyParamType Result = FCopyParams;
    Result.SetPreserveTime(true);
    return Result;
}
//---------------------------------------------------------------------------
int TSynchronizeDialog::ActualCopyParamAttrs()
{
    return FCopyParamAttrs | cpaNoPreserveTime;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::SynchronizeDialog(TSynchronizeParamType &Params,
        const TCopyParamType *CopyParams, const synchronizestartstop_slot_type &OnStartStop,
        bool &SaveSettings, int Options, int CopyParamAttrs, const getsynchronizeoptions_slot_type &OnGetOptions)
{
    bool Result;
    TSynchronizeDialog *Dialog = new TSynchronizeDialog(FPlugin, OnStartStop,
            Options, CopyParamAttrs, OnGetOptions);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Params, CopyParams, SaveSettings);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::RemoteTransferDialog(nb::TStrings *FileList,
        std::wstring &Target, std::wstring &FileMask, bool Move)
{
    std::wstring Prompt = FileNameFormatString(
                              GetMsg(Move ? REMOTE_MOVE_FILE : REMOTE_COPY_FILE),
                              GetMsg(Move ? REMOTE_MOVE_FILES : REMOTE_COPY_FILES), FileList, true);

    std::wstring Value = UnixIncludeTrailingBackslash(Target) + FileMask;
    bool Result = FPlugin->InputBox(
                      GetMsg(Move ? REMOTE_MOVE_TITLE : REMOTE_COPY_TITLE), Prompt,
                      Value, 0, MOVE_TO_HISTORY) && !Value.empty();
    if (Result)
    {
        Target = UnixExtractFilePath(Value);
        FileMask = UnixExtractFileName(Value);
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::RenameFileDialog(TRemoteFile *File,
        std::wstring &NewName)
{
    return FPlugin->InputBox(GetMsg(RENAME_FILE_TITLE).c_str(),
                             FORMAT(GetMsg(RENAME_FILE).c_str(), File->GetFileName().c_str()), NewName, 0) &&
           !NewName.empty();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TQueueDialog : TFarDialog
{
public:
    explicit TQueueDialog(TCustomFarPlugin *AFarPlugin,
                          TWinSCPFileSystem *AFileSystem, bool ClosingPlugin);

    bool Execute(TTerminalQueueStatus *Status);

protected:
    virtual void Change();
    virtual void Idle();
    bool UpdateQueue();
    void LoadQueue();
    void RefreshQueue();
    bool FillQueueItemLine(std::wstring &Line,
                           TQueueItemProxy *QueueItem, size_t Index);
    bool QueueItemNeedsFrequentRefresh(TQueueItemProxy *QueueItem);
    void UpdateControls();
    virtual bool Key(TFarDialogItem *Item, long KeyCode);
    virtual bool CloseQuery();

private:
    TTerminalQueueStatus *FStatus;
    TWinSCPFileSystem *FFileSystem;
    bool FClosingPlugin;

    TFarListBox *QueueListBox;
    TFarButton *ShowButton;
    TFarButton *ExecuteButton;
    TFarButton *DeleteButton;
    TFarButton *MoveUpButton;
    TFarButton *MoveDownButton;
    TFarButton *CloseButton;

    void OperationButtonClick(TFarButton *Sender, bool &Close);
};
//---------------------------------------------------------------------------
TQueueDialog::TQueueDialog(TCustomFarPlugin *AFarPlugin,
                           TWinSCPFileSystem *AFileSystem, bool ClosingPlugin) :
    TFarDialog(AFarPlugin),
    FStatus(NULL),
    QueueListBox(NULL),
    ShowButton(NULL),
    ExecuteButton(NULL),
    DeleteButton(NULL),
    MoveUpButton(NULL),
    MoveDownButton(NULL),
    CloseButton(NULL),
    FFileSystem(AFileSystem),
    FClosingPlugin(ClosingPlugin)
{
    TFarSeparator *Separator = NULL;
    TFarText *Text = NULL;

    SetSize(nb::TPoint(80, 23));
    nb::TRect CRect = GetClientRect();
    int ListTop;
    int ListHeight = GetClientSize().y - 4;

    SetCaption(GetMsg(QUEUE_TITLE));

    Text = new TFarText(this);
    Text->SetCaption(GetMsg(QUEUE_HEADER));

    Separator = new TFarSeparator(this);
    ListTop = Separator->GetBottom();

    Separator = new TFarSeparator(this);
    Separator->Move(0, ListHeight);

    ExecuteButton = new TFarButton(this);
    ExecuteButton->SetCaption(GetMsg(QUEUE_EXECUTE));
    ExecuteButton->SetOnClick(boost::bind(&TQueueDialog::OperationButtonClick, this, _1, _2));
    ExecuteButton->SetCenterGroup(true);

    SetNextItemPosition(ipRight);

    DeleteButton = new TFarButton(this);
    DeleteButton->SetCaption(GetMsg(QUEUE_DELETE));
    DeleteButton->SetOnClick(boost::bind(&TQueueDialog::OperationButtonClick, this, _1, _2));
    DeleteButton->SetCenterGroup(true);

    MoveUpButton = new TFarButton(this);
    MoveUpButton->SetCaption(GetMsg(QUEUE_MOVE_UP));
    MoveUpButton->SetOnClick(boost::bind(&TQueueDialog::OperationButtonClick, this, _1, _2));
    MoveUpButton->SetCenterGroup(true);

    MoveDownButton = new TFarButton(this);
    MoveDownButton->SetCaption(GetMsg(QUEUE_MOVE_DOWN));
    MoveDownButton->SetOnClick(boost::bind(&TQueueDialog::OperationButtonClick, this, _1, _2));
    MoveDownButton->SetCenterGroup(true);

    CloseButton = new TFarButton(this);
    CloseButton->SetCaption(GetMsg(QUEUE_CLOSE));
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
//---------------------------------------------------------------------------
void TQueueDialog::OperationButtonClick(TFarButton *Sender,
                                        bool & /*Close*/)
{
    TQueueItemProxy *QueueItem = NULL;
    if (QueueListBox->GetItems()->GetSelected() != -1)
    {
        QueueItem = reinterpret_cast<TQueueItemProxy *>(
                        QueueListBox->GetItems()->GetObject(QueueListBox->GetItems()->GetSelected()));

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
bool TQueueDialog::Key(TFarDialogItem * /*Item*/, long KeyCode)
{
    bool Result = false;
    if (QueueListBox->Focused())
    {
        TFarButton *DoButton = NULL;
        if (KeyCode == VK_RETURN)
        {
            if (ExecuteButton->GetEnabled())
            {
                DoButton = ExecuteButton;
            }
            Result = true;
        }
        else if (KeyCode == VK_DELETE)
        {
            if (DeleteButton->GetEnabled())
            {
                DoButton = DeleteButton;
            }
            Result = true;
        }
        else if (KeyCode == (VK_UP | (LEFT_CTRL_PRESSED << 16)))
        {
            if (MoveUpButton->GetEnabled())
            {
                DoButton = MoveUpButton;
            }
            Result = true;
        }
        else if (KeyCode == (VK_DOWN | (LEFT_CTRL_PRESSED << 16)))
        {
            if (MoveDownButton->GetEnabled())
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
void TQueueDialog::UpdateControls()
{
    TQueueItemProxy *QueueItem = NULL;
    if (QueueListBox->GetItems()->GetSelected() != -1)
    {
        QueueItem = reinterpret_cast<TQueueItemProxy *>(
                        QueueListBox->GetItems()->GetObject(QueueListBox->GetItems()->GetSelected()));
    }

    if ((QueueItem != NULL) && (QueueItem->GetStatus() == TQueueItem::qsProcessing))
    {
        ExecuteButton->SetCaption(GetMsg(QUEUE_PAUSE));
        ExecuteButton->SetEnabled(true);
    }
    else if ((QueueItem != NULL) && (QueueItem->GetStatus() == TQueueItem::qsPaused))
    {
        ExecuteButton->SetCaption(GetMsg(QUEUE_RESUME));
        ExecuteButton->SetEnabled(true);
    }
    else if ((QueueItem != NULL) && TQueueItem::IsUserActionStatus(QueueItem->GetStatus()))
    {
        ExecuteButton->SetCaption(GetMsg(QUEUE_SHOW));
        ExecuteButton->SetEnabled(true);
    }
    else
    {
        ExecuteButton->SetCaption(GetMsg(QUEUE_EXECUTE));
        ExecuteButton->SetEnabled(
            (QueueItem != NULL) && (QueueItem->GetStatus() == TQueueItem::qsPending));
    }
    DeleteButton->SetEnabled((QueueItem != NULL) &&
                             (QueueItem->GetStatus() != TQueueItem::qsDone));
    MoveUpButton->SetEnabled((QueueItem != NULL) &&
                             (QueueItem->GetStatus() == TQueueItem::qsPending) &&
                             (QueueItem->GetIndex() > FStatus->GetActiveCount()));
    MoveDownButton->SetEnabled((QueueItem != NULL) &&
                               (QueueItem->GetStatus() == TQueueItem::qsPending) &&
                               (QueueItem->GetIndex() < FStatus->GetCount() - 1));
}
//---------------------------------------------------------------------------
void TQueueDialog::Idle()
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
bool TQueueDialog::CloseQuery()
{
    bool Result = TFarDialog::CloseQuery();
    if (Result)
    {
        TWinSCPPlugin *WinSCPPlugin = dynamic_cast<TWinSCPPlugin *>(FarPlugin);
        Result = !FClosingPlugin || (FStatus->GetCount() == 0) ||
                 (WinSCPPlugin->MoreMessageDialog(GetMsg(QUEUE_PENDING_ITEMS), NULL,
                         qtWarning, qaOK | qaCancel) == qaCancel);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TQueueDialog::UpdateQueue()
{
    assert(FFileSystem != NULL);
    TTerminalQueueStatus *Status = FFileSystem->ProcessQueue(false);
    bool Result = (Status != NULL);
    if (Result)
    {
        FStatus = Status;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TQueueDialog::Change()
{
    TFarDialog::Change();

    if (GetHandle())
    {
        UpdateControls();
    }
}
//---------------------------------------------------------------------------
void TQueueDialog::RefreshQueue()
{
    if (QueueListBox->GetItems()->GetCount() > 0)
    {
        bool Change = false;
        size_t TopIndex = QueueListBox->GetItems()->GetTopIndex();
        size_t Index = TopIndex;

        size_t ILine = 0;
        while ((Index > ILine) &&
                (QueueListBox->GetItems()->GetObject(Index) ==
                 QueueListBox->GetItems()->GetObject(Index - ILine - 1)))
        {
            ILine++;
        }

        TQueueItemProxy *PrevQueueItem = NULL;
        TQueueItemProxy *QueueItem = NULL;
        std::wstring Line;
        while ((Index < QueueListBox->GetItems()->GetCount()) &&
                (Index < TopIndex + QueueListBox->GetHeight()))
        {
            QueueItem = reinterpret_cast<TQueueItemProxy *>(
                            QueueListBox->GetItems()->GetObject(Index));
            assert(QueueItem != NULL);
            if ((PrevQueueItem != NULL) && (QueueItem != PrevQueueItem))
            {
                ILine = 0;
            }

            if (QueueItemNeedsFrequentRefresh(QueueItem) &&
                    !QueueItem->GetProcessingUserAction())
            {
                FillQueueItemLine(Line, QueueItem, ILine);
                if (QueueListBox->GetItems()->GetString(Index) != Line)
                {
                    Change = true;
                    QueueListBox->GetItems()->PutString(Index, Line);
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
void TQueueDialog::LoadQueue()
{
    TFarList *List = new TFarList();
    {
        BOOST_SCOPE_EXIT ( (&List) )
        {
            delete List;
        } BOOST_SCOPE_EXIT_END
        std::wstring Line;
        TQueueItemProxy *QueueItem = NULL;
        for (size_t Index = 0; Index < FStatus->GetCount(); Index++)
        {
            QueueItem = FStatus->GetItem(Index);
            size_t ILine = 0;
            while (FillQueueItemLine(Line, QueueItem, ILine))
            {
                List->AddObject(Line, reinterpret_cast<nb::TObject *>(QueueItem));
                List->SetDisabled(List->GetCount() - 1, (ILine > 0));
                ILine++;
            }
        }
        QueueListBox->SetItems(List);
    }
}
//---------------------------------------------------------------------------
bool TQueueDialog::FillQueueItemLine(std::wstring &Line,
                                     TQueueItemProxy *QueueItem, size_t Index)
{
    size_t PathMaxLen = 49;

    if ((Index > 2) ||
            ((Index == 2) && (QueueItem->GetStatus() == TQueueItem::qsPending)))
    {
        return false;
    }

    std::wstring ProgressStr;

    switch (QueueItem->GetStatus())
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
                     !QueueItem->GetProcessingUserAction() &&
                     ((GetTickCount() % 2000) >= 1000);

    std::wstring Operation;
    std::wstring Direction;
    std::wstring Values[2];
    TFileOperationProgressType *ProgressData = QueueItem->GetProgressData();
    TQueueItem::TInfo *Info = QueueItem->GetInfo();

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

        if (ProgressStr.empty())
        {
            if (ProgressData != NULL)
            {
                if (ProgressData->Operation == Info->Operation)
                {
                    Values[1] = FORMAT(L"%d%%", ProgressData->OverallProgress());
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
                Values[1] = FORMAT(L"%d%%", ProgressData->TransferProgress());
            }
        }
        else
        {
            Values[0] = ProgressStr;
        }
    }

    Line = FORMAT(L"%s %s  %s %s",
                  // Operation.c_str(), Direction.c_str(), PathMaxLen, PathMaxLen, Values[0].c_str(), Values[1].c_str());
                  Operation.c_str(), Direction.c_str(), Values[0].c_str(), Values[1].c_str());

    return true;
}
//---------------------------------------------------------------------------
bool TQueueDialog::QueueItemNeedsFrequentRefresh(
    TQueueItemProxy *QueueItem)
{
    return
        (TQueueItem::IsUserActionStatus(QueueItem->GetStatus()) ||
         (QueueItem->GetStatus() == TQueueItem::qsPaused));
}
//---------------------------------------------------------------------------
bool TQueueDialog::Execute(TTerminalQueueStatus *Status)
{
    FStatus = Status;

    UpdateQueue();
    LoadQueue();

    bool Result = (ShowModal() != brCancel);

    FStatus = NULL;

    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::QueueDialog(
    TTerminalQueueStatus *Status, bool ClosingPlugin)
{
    bool Result;
    TQueueDialog *Dialog = new TQueueDialog(FPlugin, this, ClosingPlugin);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Status);
    }
    return Result;
}
//---------------------------------------------------------------------------
bool TWinSCPFileSystem::CreateDirectoryDialog(std::wstring &Directory,
        TRemoteProperties *Properties, bool &SaveSettings)
{
    bool Result;
    TWinSCPDialog *Dialog = new TWinSCPDialog(FPlugin);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        TFarText *Text;
        TFarSeparator *Separator;

        Dialog->SetCaption(GetMsg(CREATE_FOLDER_TITLE));
        Dialog->SetSize(nb::TPoint(66, 15));

        Text = new TFarText(Dialog);
        Text->SetCaption(GetMsg(CREATE_FOLDER_PROMPT));

        TFarEdit *DirectoryEdit = new TFarEdit(Dialog);
        DirectoryEdit->SetHistory(L"NewFolder");

        Separator = new TFarSeparator(Dialog);
        Separator->SetCaption(GetMsg(CREATE_FOLDER_ATTRIBUTES));

        TFarCheckBox *SetRightsCheck = new TFarCheckBox(Dialog);
        SetRightsCheck->SetCaption(GetMsg(CREATE_FOLDER_SET_RIGHTS));

        TRightsContainer *RightsContainer = new TRightsContainer(Dialog, false, true,
                true, SetRightsCheck);

        TFarCheckBox *SaveSettingsCheck = new TFarCheckBox(Dialog);
        SaveSettingsCheck->SetCaption(GetMsg(CREATE_FOLDER_REUSE_SETTINGS));
        SaveSettingsCheck->Move(0, 6);

        Dialog->AddStandardButtons();

        DirectoryEdit->SetText(Directory);
        SaveSettingsCheck->SetChecked(SaveSettings);
        assert(Properties != NULL);
        SetRightsCheck->SetChecked(Properties->Valid.Contains(vpRights));
        // expect sensible value even if rights are not set valid
        RightsContainer->SetRights(Properties->Rights);

        Result = (Dialog->ShowModal() == brOK);

        if (Result)
        {
            Directory = DirectoryEdit->GetText();
            SaveSettings = SaveSettingsCheck->GetChecked();
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
//---------------------------------------------------------------------------
