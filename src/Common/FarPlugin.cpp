//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "FarUtil.h"
#include "FarPlugin.h"
#include "FarDialog.h"
#include "Common.h"
// FAR WORKAROUND
//---------------------------------------------------------------------------
TCustomFarPlugin *FarPlugin = NULL;
#define FAR_TITLE_SUFFIX L" - Far"
//---------------------------------------------------------------------------
TFarMessageParams::TFarMessageParams()
{
    MoreMessages = NULL;
    CheckBox = false;
    Timer = 0;
    TimerAnswer = 0;
    TimerEvent = NULL;
    Timeout = 0;
    TimeoutButton = 0;
    ClickEvent = NULL;
}
//---------------------------------------------------------------------------
TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst): TObject()
{
    // DEBUG_PRINTF(L"TCustomFarPlugin: begin");
    Self = this;
    FFarThread = GetCurrentThreadId();
    FCriticalSection = new TCriticalSection;
    FHandle = HInst;
    FANSIApis = AreFileApisANSI();
    FFarVersion = 0;
    FTerminalScreenShowing = false;

    FOpenedPlugins = new TObjectList();
    FOpenedPlugins->SetOwnsObjects(false);
    FSavedTitles = new TStringList();
    FTopDialog = NULL;
    FOldFar = true;
    FValidFarSystemSettings = false;

    memset(&FPluginInfo, 0, sizeof(FPluginInfo));
    ClearPluginInfo(FPluginInfo);

    // far\Examples\Compare\compare.cpp
    FConsoleInput = CreateFile(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, 0, NULL);
    FConsoleOutput = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (ConsoleWindowState() == SW_SHOWNORMAL)
    {
        FNormalConsoleSize = TerminalInfo();
    }
    else
    {
        FNormalConsoleSize = TPoint(-1, -1);
    }
    // DEBUG_PRINTF(L"TCustomFarPlugin: end");
}
//---------------------------------------------------------------------------
TCustomFarPlugin::~TCustomFarPlugin()
{
    assert(FTopDialog == NULL);

    ResetCachedInfo();
    CloseHandle(FConsoleInput);
    FConsoleInput = INVALID_HANDLE_VALUE;
    CloseHandle(FConsoleOutput);
    FConsoleOutput = INVALID_HANDLE_VALUE;

    ClearPluginInfo(FPluginInfo);
    assert(FOpenedPlugins->GetCount() == 0);
    delete FOpenedPlugins;
    delete FSavedTitles;
    delete FCriticalSection;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::HandlesFunction(THandlesFunction /*Function*/)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetMinFarVersion()
{
    return 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo *Info)
{
    try
    {
        ResetCachedInfo();
        // Info->StructSize = 336 for FAR 1.65
        FOldFar = (Info->StructSize < StartupInfoMinSize);
        memset(&FStartupInfo, 0, sizeof(FStartupInfo));
        memcpy(&FStartupInfo, Info,
               Info->StructSize >= sizeof(FStartupInfo) ?
               sizeof(FStartupInfo) : Info->StructSize);
        StrFromFar(FStartupInfo.ModuleName);
        // the minimum we really need
        assert(FStartupInfo.GetMsg != NULL);
        assert(FStartupInfo.Message != NULL);

        memset(&FFarStandardFunctions, 0, sizeof(FFarStandardFunctions));
        int FSFOffset = ((char *)&Info->FSF - (char *)Info);
        if (Info->StructSize > FSFOffset)
        {
            FOldFar = FOldFar | (Info->FSF->StructSize < StandardFunctionsMinSize);

            memcpy(&FFarStandardFunctions, Info->FSF,
                   Info->FSF->StructSize >= sizeof(FFarStandardFunctions) ?
                   sizeof(FFarStandardFunctions) : Info->FSF->StructSize);
        }
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ExitFAR()
{
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::GetPluginInfo(struct PluginInfo *Info)
{
    try
    {
        ResetCachedInfo();
        Info->StructSize = sizeof(PluginInfo);
        TStringList DiskMenuStrings;
        TStringList PluginMenuStrings;
        TStringList PluginConfigStrings;
        TStringList CommandPrefixes;

        ClearPluginInfo(FPluginInfo);

        GetPluginInfoEx(FPluginInfo.Flags, &DiskMenuStrings, &PluginMenuStrings,
                        &PluginConfigStrings, &CommandPrefixes);

#define COMPOSESTRINGARRAY(NAME) \
        if (NAME.GetCount()) \
        { \
          wchar_t ** StringArray = new wchar_t *[NAME.GetCount()]; \
          FPluginInfo.NAME = StringArray; \
          FPluginInfo.NAME ## Number = NAME.GetCount(); \
          for (int Index = 0; Index < NAME.GetCount(); Index++) \
          { \
            StringArray[Index] = (wchar_t *)StrToFar(DuplicateStr(NAME.GetString(Index))).c_str(); \
          } \
        }

        COMPOSESTRINGARRAY(DiskMenuStrings);
        COMPOSESTRINGARRAY(PluginMenuStrings);
        COMPOSESTRINGARRAY(PluginConfigStrings);

#undef COMPOSESTRINGARRAY

        if (DiskMenuStrings.GetCount())
        {
            wchar_t *NumberArray = new wchar_t[DiskMenuStrings.GetCount()];
            FPluginInfo.DiskMenuStrings = &NumberArray;
            for (int Index = 0; Index < DiskMenuStrings.GetCount(); Index++)
            {
                NumberArray[Index] = (int)DiskMenuStrings.GetObject(Index);
            }
        }

        std::wstring CommandPrefix;
        for (int Index = 0; Index < CommandPrefixes.GetCount(); Index++)
        {
            CommandPrefix = CommandPrefix + (CommandPrefix.empty() ? L"" : L":") +
                            CommandPrefixes.GetString(Index);
        }
        DEBUG_PRINTF(L"CommandPrefix = %s", CommandPrefix.c_str());
        FPluginInfo.CommandPrefix = StrToFar(DuplicateStr(CommandPrefix)).c_str();

        memcpy(Info, &FPluginInfo, sizeof(FPluginInfo));
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
    }
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::GetModuleName()
{
    return FStartupInfo.ModuleName;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClearPluginInfo(PluginInfo &Info)
{
    if (Info.StructSize)
    {
#define FREESTRINGARRAY(NAME) \
      for (int Index = 0; Index < Info.NAME ## Number; Index++) \
      { \
        delete[] Info.NAME[Index]; \
      } \
      delete[] Info.NAME;

        FREESTRINGARRAY(DiskMenuStrings);
        FREESTRINGARRAY(PluginMenuStrings);
        FREESTRINGARRAY(PluginConfigStrings);

#undef FREESTRINGARRAY

        delete[] Info.DiskMenuStrings;
        delete[] Info.CommandPrefix;
    }
    memset(&Info, 0, sizeof(Info));
    Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::DuplicateStr(const std::wstring Str, bool AllowEmpty)
{
    /*
    if (Str.empty() && !AllowEmpty)
    {
        return NULL;
    }
    else
    {
        wchar_t *Result = new wchar_t[Str.size() + 1];
        // strcpy(Result, Str.c_str());
        wcscpy_s(Result, Str.size(), Str.c_str());
        return Result;
    }
    */
    std::wstring result = Str;
    return result;
}
//---------------------------------------------------------------------------
TCustomFarFileSystem *TCustomFarPlugin::GetPanelFileSystem(bool Another,
        HANDLE Plugin)
{
    TCustomFarFileSystem *Result = NULL;
    PanelInfo Info;
    if (FarVersion() >= FAR170BETA5)
    {
        // FarControl(Another ? FCTL_GETANOTHERPANELSHORTINFO : FCTL_GETPANELSHORTINFO, &Info, Plugin);
    }
    else
    {
        if (Another)
            FarControl(FCTL_GETPANELINFO, 0, (LONG_PTR)&Info, PANEL_PASSIVE);
        else
            FarControl(FCTL_GETPANELINFO, 0, (LONG_PTR)&Info, PANEL_ACTIVE);
    }

    if (Info.Plugin)
    {
        RECT Bounds = Info.PanelRect;
        TCustomFarFileSystem *FileSystem;
        int Index = 0;
        while (!Result && Index < FOpenedPlugins->GetCount())
        {
            FileSystem = dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
            assert(FileSystem);
            TRect bounds = FileSystem->GetPanelInfo()->GetBounds();
            if (bounds == Bounds)
            {
                Result = FileSystem;
            }
            Index++;
        }
    }

    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::InvalidateOpenPluginInfo()
{
    for (int Index = 0; Index < FOpenedPlugins->GetCount(); Index++)
    {
        TCustomFarFileSystem *FileSystem =
            dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
        FileSystem->InvalidateOpenPluginInfo();
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Configure(int Item)
{
    try
    {
        ResetCachedInfo();
        if (IsOldFar())
        {
            OldFar();
        }

        int Result = ConfigureEx(Item);

        InvalidateOpenPluginInfo();

        return Result;
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
        return false;
    }
}
//---------------------------------------------------------------------------
void *TCustomFarPlugin::OpenPlugin(int OpenFrom, int Item)
{
    try
    {
        ResetCachedInfo();
        if (IsOldFar())
        {
            OldFar();
        }

        std::wstring Buf;
        if ((OpenFrom == OPEN_SHORTCUT) || (OpenFrom == OPEN_COMMANDLINE))
        {
            Buf = (wchar_t *)Item;
            StrFromFar(Buf);
            Item = int(Buf.c_str());
        }

        TCustomFarFileSystem *Result = OpenPluginEx(OpenFrom, Item);

        if (Result)
        {
            FOpenedPlugins->Add(Result);
        }
        else
        {
            Result = (TCustomFarFileSystem *)INVALID_HANDLE_VALUE;
        }

        return Result;
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
        return INVALID_HANDLE_VALUE;
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClosePlugin(void *Plugin)
{
    try
    {
        ResetCachedInfo();
        TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);
        {
            BOOST_SCOPE_EXIT ( (&Self) (&FileSystem) )
            {
                Self->FOpenedPlugins->Remove(FileSystem);
            } BOOST_SCOPE_EXIT_END
            {
                TGuard Guard(FileSystem->GetCriticalSection());
                FileSystem->Close();
            }
            delete FileSystem;
        }
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
    }
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::IsOldFar()
{
    return FOldFar;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::OldFar()
{
    throw std::exception("");
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleFileSystemException(
    TCustomFarFileSystem *FileSystem, const std::exception *E, int OpMode)
{
    // This method is called as last-resort exception handler before
    // leaving plugin API. Especially for API fuctions that must update
    // panel contents on themselves (like ProcessKey), the instance of filesystem
    // may not exists anymore.
    // Check against object pointer is stupid, but no other idea so far.
    if (FOpenedPlugins->IndexOf(FileSystem) >= 0)
    {
        FileSystem->HandleException(E, OpMode);
    }
    else
    {
        HandleException(E, OpMode);
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::GetOpenPluginInfo(HANDLE Plugin,
        struct OpenPluginInfo *Info)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            FileSystem->GetOpenPluginInfo(Info);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetFindData(HANDLE Plugin,
        struct PluginPanelItem **PanelItem, int *ItemsNumber, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->GetFindData(PanelItem, ItemsNumber, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FreeFindData(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            FileSystem->FreeFindData(PanelItem, ItemsNumber);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessHostFile(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessHostFile))
        {
            assert(!FOldFar);
            assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

            {
                TGuard Guard(FileSystem->GetCriticalSection());
                return FileSystem->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
            }
        }
        else
        {
            return 0;
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessKey(HANDLE Plugin, int Key,
        unsigned int ControlState)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessKey))
        {
            assert(!FOldFar);
            assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

            {
                TGuard Guard(FileSystem->GetCriticalSection());
                return FileSystem->ProcessKey(Key, ControlState);
            }
        }
        else
        {
            return 0;
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E);
        // when error occurs, assume that key can be handled by plugin and
        // should not be processed by FAR
        return 1;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEvent(HANDLE Plugin, int Event, void *Param)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessEvent))
        {
            assert(!FOldFar);
            assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

            std::wstring Buf;
            if ((Event == FE_CHANGEVIEWMODE) || (Event == FE_COMMAND))
            {
                Buf = (wchar_t *)Param;
                StrFromFar(Buf);
                Param = (void *)Buf.c_str();
            }

            {
                TGuard Guard(FileSystem->GetCriticalSection());
                return FileSystem->ProcessEvent(Event, Param);
            }
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E);
        return Event == FE_COMMAND ? true : false;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::SetDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->SetDirectory(Dir, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MakeDirectory(HANDLE Plugin, wchar_t *Name, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->MakeDirectory(Name, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::DeleteFiles(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->DeleteFiles(PanelItem, ItemsNumber, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetFiles(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber, int Move,
        wchar_t *DestPath, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        // display error even for OPM_FIND
        HandleFileSystemException(FileSystem, &E, OpMode & ~OPM_FIND);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::PutFiles(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber, int Move, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
        }
    }
    catch (const std::exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEditorEvent(int Event, void *Param)
{
    try
    {
        ResetCachedInfo();

        return ProcessEditorEventEx(Event, Param);
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEditorInput(const INPUT_RECORD *Rec)
{
    try
    {
        ResetCachedInfo();

        return ProcessEditorInputEx(Rec);
    }
    catch (const std::exception &E)
    {
        HandleException(&E);
        // when error occurs, assume that input event can be handled by plugin and
        // should not be processed by FAR
        return 1;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MaxMessageLines()
{
    return TerminalInfo().y - 5;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MaxMenuItemLength()
{
    // got from maximal length of path in FAR's folders history
    return TerminalInfo().x - 13;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MaxLength(TStrings *Strings)
{
    int Result = 0;
    for (int Index = 0; Index < Strings->GetCount(); Index++)
    {
        if (Result < Strings->GetString(Index).size())
        {
            Result = Strings->GetString(Index).size();
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
class TFarMessageDialog : public TFarDialog
{
public:
    TFarMessageDialog(TCustomFarPlugin *Plugin, unsigned int AFlags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params);

    int Execute(bool &ACheckBox);

protected:
    virtual void Change();
    virtual void Idle();

private:
    void ButtonClick(TFarButton *Sender, bool &Close);

private:
    bool FCheckBoxChecked;
    TFarMessageParams *FParams;
    TDateTime FStartTime;
    TDateTime FLastTimerTime;
    TFarButton *FTimeoutButton;
    std::wstring FTimeoutButtonCaption;
    TFarCheckBox *CheckBox;
};
//---------------------------------------------------------------------------
TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin *Plugin, unsigned int AFlags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params) :
    TFarDialog(Plugin), FParams(Params)
{
    assert(Params != NULL);
    assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
    assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
    // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
    assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

    TStrings *MessageLines = new TStringList();
    TStrings *MoreMessageLines = NULL;
    {
        BOOST_SCOPE_EXIT ( (&MessageLines) (&MoreMessageLines) )
        {
            delete MessageLines;
            delete MoreMessageLines;
        } BOOST_SCOPE_EXIT_END
        FarWrapText(Message, MessageLines, MaxMessageWidth);
        int MaxLen = Plugin->MaxLength(MessageLines);

        if (Params->MoreMessages != NULL)
        {
            MoreMessageLines = new TStringList();
            std::wstring MoreMessages = Params->MoreMessages->GetText();
            while (MoreMessages[MoreMessages.size()] == L'\n' ||
                    MoreMessages[MoreMessages.size()] == L'\r')
            {
                MoreMessages.resize(MoreMessages.size() - 1);
            }
            FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
            int MoreMaxLen = Plugin->MaxLength(MoreMessageLines);
            if (MaxLen < MoreMaxLen)
            {
                MaxLen = MoreMaxLen;
            }
        }

        // temporary
        SetSize(TPoint(MaxMessageWidth, 10));
        SetCaption(Title);
        SetFlags(GetFlags() |
                FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING));

        for (int Index = 0; Index < MessageLines->GetCount(); Index++)
        {
            TFarText *Text = new TFarText(this);
            Text->SetCaption(MessageLines->GetString(Index));
        }

        TFarLister *MoreMessagesLister = NULL;
        TFarSeparator *MoreMessagesSeparator = NULL;

        if (Params->MoreMessages != NULL)
        {
            new TFarSeparator(this);

            MoreMessagesLister = new TFarLister(this);
            MoreMessagesLister->GetItems()->Assign(MoreMessageLines);
            MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

            MoreMessagesSeparator = new TFarSeparator(this);
        }

        int ButtonOffset = (Params->CheckBoxLabel.empty() ? -1 : -2);
        int ButtonLines = 1;
        TFarButton *Button = NULL;
        FTimeoutButton = NULL;
        for (int Index = 0; Index < Buttons->GetCount(); Index++)
        {
            TFarButton *PrevButton = Button;
            Button = new TFarButton(this);
            Button->SetDefault(Index == 0);
            Button->SetBrackets(brNone);
            Button->SetOnClick(boost::bind(&TFarMessageDialog::ButtonClick, this, _1, _2));
            std::wstring Caption = Buttons->GetString(Index);
            if ((Params->Timeout > 0) &&
                (Params->TimeoutButton == (unsigned int)Index))
            {
                FTimeoutButtonCaption = Caption;
                Caption = FORMAT(Params->TimeoutStr.c_str(), Caption.c_str(), int(Params->Timeout / 1000));
                std::wstring Buffer;
                Buffer.resize(512);
                GetFarPlugin()->GetFarStandardFunctions().sprintf((wchar_t *)Buffer.c_str(), Params->TimeoutStr.c_str(), Caption.c_str(), int(Params->Timeout / 1000));
                SetCaption(Buffer);
                FTimeoutButton = Button;
            }
            Button->SetCaption(FORMAT(L" %s ", (Caption)));
            std::wstring Buffer;
            Buffer.resize(512);
            GetFarPlugin()->GetFarStandardFunctions().sprintf((wchar_t *)Buffer.c_str(), L" %s ", Caption.c_str(), int(Params->Timeout / 1000));
            Button->SetCaption(Buffer);
            Button->SetTop(GetBorderBox()->GetBottom() + ButtonOffset);
            Button->SetBottom(Button->GetTop());
            Button->SetResult(Index + 1);
            Button->SetCenterGroup(true);
            Button->SetTag(reinterpret_cast<int>(Buttons->GetObject(Index)));
            if (PrevButton != NULL)
            {
                Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
            }

            if (MaxMessageWidth < Button->GetRight()- GetBorderBox()->GetLeft())
            {
                for (int PIndex = 0; PIndex < GetItemCount(); PIndex++)
                {
                    TFarButton *PrevButton = dynamic_cast<TFarButton *>(GetItem(PIndex));
                    if ((PrevButton != NULL) && (PrevButton != Button))
                    {
                        PrevButton->Move(0, -1);
                    }
                }
                Button->Move(- (Button->GetLeft() - GetBorderBox()->GetLeft()), 0);
                ButtonLines++;
            }

            if (MaxLen < Button->GetRight() - GetBorderBox()->GetLeft())
            {
                MaxLen = Button->GetRight() - GetBorderBox()->GetLeft();
            }

            SetNextItemPosition(ipRight);
        }

        if (!Params->CheckBoxLabel.empty())
        {
            SetNextItemPosition(ipNewLine);
            CheckBox = new TFarCheckBox(this);
            CheckBox->SetCaption(Params->CheckBoxLabel);

            if (MaxLen < CheckBox->GetRight() - GetBorderBox()->GetLeft())
            {
                MaxLen = CheckBox->GetRight() - GetBorderBox()->GetLeft();
            }
        }
        else
        {
            CheckBox = NULL;
        }

        TRect rect = GetClientRect();
        TPoint S(
            rect.Left + MaxLen + (- (rect.Right + 1)),
            rect.Top + MessageLines->GetCount() +
            (Params->MoreMessages != NULL ? 1 : 0) + ButtonLines +
            (!Params->CheckBoxLabel.empty() ? 1 : 0) +
            (- (rect.Bottom + 1)));

        if (Params->MoreMessages != NULL)
        {
            int MoreMessageHeight = Plugin->TerminalInfo().y - S.y - 1;
            if (MoreMessageHeight > MoreMessagesLister->GetItems()->GetCount())
            {
                MoreMessageHeight = MoreMessagesLister->GetItems()->GetCount();
            }
            assert(MoreMessagesLister != NULL);
            MoreMessagesLister->SetHeight(MoreMessageHeight);
            MoreMessagesLister->SetRight(
                GetBorderBox()->GetRight() - (MoreMessagesLister->GetScrollBar() ? 0 : 1));
            MoreMessagesLister->SetTabStop(MoreMessagesLister->GetScrollBar());
            assert(MoreMessagesSeparator != NULL);
            MoreMessagesSeparator->SetPosition(
                MoreMessagesLister->GetTop() + MoreMessagesLister->GetHeight());
            S.y += MoreMessagesLister->GetHeight() + 1;
        }

        SetSize(S);
    }
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Idle()
{
    TFarDialog::Idle();

    if (FParams->Timer > 0)
    {
        unsigned int SinceLastTimer = (double(Now()) - double(FLastTimerTime)) * 24*60*60*1000;
        if (SinceLastTimer >= FParams->Timeout)
        {
            assert(FParams->TimerEvent != NULL);
            if (FParams->TimerEvent != NULL)
            {
                FParams->TimerAnswer = 0;
                farmessagetimer_signal_type sig;
                sig.connect(*FParams->TimerEvent);
                sig(FParams->TimerAnswer);
                if (FParams->TimerAnswer != 0)
                {
                    Close(GetDefaultButton());
                }
                FLastTimerTime = Now();
            }
        }
    }

    if (FParams->Timeout > 0)
    {
        unsigned int Running = (double(Now()) - double(FStartTime)) * 24*60*60*1000;
        if (Running >= FParams->Timeout)
        {
            assert(FTimeoutButton != NULL);
            Close(FTimeoutButton);
        }
        else
        {
            std::wstring Caption =
                FORMAT(L" %s ", (FORMAT(FParams->TimeoutStr.c_str(),
                                       FTimeoutButtonCaption, int((FParams->Timeout - Running) / 1000))).c_str());
            Caption += ::StringOfChar(L' ', FTimeoutButton->GetCaption().size() - Caption.size());
            /*
            std::wstring Buffer;
            Buffer.resize(512);
            std::wstring Buffer2;
            Buffer2.resize(512);
            GetFarPlugin()->GetFarStandardFunctions().sprintf((wchar_t *)Buffer2.c_str(), FTimeoutButtonCaption.c_str(), FParams->TimeoutStr.c_str(), int((FParams->Timeout - Running) / 1000));
            GetFarPlugin()->GetFarStandardFunctions().sprintf((wchar_t *)Buffer.c_str(), L" %s ", Buffer2.c_str());
            std::wstring Caption = Buffer;
            */
            FTimeoutButton->SetCaption(Caption);
        }
    }
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Change()
{
    TFarDialog::Change();

    if (GetHandle() != NULL)
    {
        if ((CheckBox != NULL) && (FCheckBoxChecked != CheckBox->GetChecked()))
        {
            for (int Index = 0; Index < GetItemCount(); Index++)
            {
                TFarButton *Button = dynamic_cast<TFarButton *>(GetItem(Index));
                if ((Button != NULL) && (Button->GetTag() == 0))
                {
                    Button->SetEnabled(!CheckBox->GetChecked());
                }
            }
            FCheckBoxChecked = CheckBox->GetChecked();
        }
    }
}
//---------------------------------------------------------------------------
int TFarMessageDialog::Execute(bool &ACheckBox)
{
    FStartTime = Now();
    FLastTimerTime = FStartTime;
    FCheckBoxChecked = !ACheckBox;
    if (CheckBox != NULL)
    {
        CheckBox->SetChecked(ACheckBox);
    }

    int Result = ShowModal();
    assert(Result != 0);
    if (Result > 0)
    {
        if (CheckBox != NULL)
        {
            ACheckBox = CheckBox->GetChecked();
        }
        Result--;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarMessageDialog::ButtonClick(TFarButton *Sender, bool &Close)
{
    if (FParams->ClickEvent != NULL)
    {
        farmessageclick_signal_type sig;
        sig.connect(*FParams->ClickEvent);
        sig(FParams->Token, Sender->GetResult() - 1, Close);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::DialogMessage(unsigned int Flags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params)
{
    int Result;
    TFarMessageDialog *Dialog =
        new TFarMessageDialog(this, Flags, Title, Message, Buttons, Params);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Result = Dialog->Execute(Params->CheckBox);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarMessage(unsigned int Flags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params)
{
    assert(Params != NULL);

    int Result;
    TStringList *MessageLines = NULL;
    wchar_t **Items = NULL;
    {
        BOOST_SCOPE_EXIT ( (&MessageLines) (&Items) )
        {
            delete MessageLines;
            delete Items;
        } BOOST_SCOPE_EXIT_END
        std::wstring FullMessage = Message;
        if (Params->MoreMessages != NULL)
        {
            FullMessage += std::wstring(L"\n\x01\n") + Params->MoreMessages->GetText();
            while (FullMessage[FullMessage.size()] == L'\n' ||
                    FullMessage[FullMessage.size()] == L'\r')
            {
                FullMessage.resize(FullMessage.size() - 1);
            }
            FullMessage += L"\n\x01\n";
        }

        MessageLines = new TStringList();
        MessageLines->Add(Title);
        FarWrapText(FullMessage, MessageLines, MaxMessageWidth);

        // FAR WORKAROUND
        // When there is too many lines to fit on screen, far uses not-shown
        // lines as button captions instead of real captions at the end of the list
        int MaxLines = MaxMessageLines();
        while (MessageLines->GetCount() > MaxLines)
        {
            MessageLines->Delete(MessageLines->GetCount() - 1);
        }

        for (int Index = 0; Index < Buttons->GetCount(); Index++)
        {
            MessageLines->Add(Buttons->GetString(Index));
        }

        Items = new wchar_t *[MessageLines->GetCount()];
        for (int Index = 0; Index < MessageLines->GetCount(); Index++)
        {
            std::wstring S = MessageLines->GetString(Index);
            MessageLines->PutString(Index, StrToFar(S));
            Items[Index] = (wchar_t *)MessageLines->GetString(Index).c_str();
        }

        TFarEnvGuard Guard;
        Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
            Flags | FMSG_LEFTALIGN, NULL, Items, MessageLines->GetCount(),
            Buttons->GetCount());
    }

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Message(unsigned int Flags,
    const std::wstring Title, const std::wstring Message, TStrings *Buttons,
    TFarMessageParams *Params, bool Oem)
{
    // when message is shown while some "custom" output is on screen,
    // make the output actually background of FAR screen
    if (FTerminalScreenShowing)
    {
        FarControl(FCTL_SETUSERSCREEN, 0, NULL);
    }

    int Result;
    if (Buttons != NULL)
    {
        TFarMessageParams DefaultParams;
        TFarMessageParams *AParams = (Params == NULL ? &DefaultParams : Params);
        Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
    }
    else
    {
        assert(Params == NULL);
        std::wstring Items = Title + L"\n" + Message;
        if (!Oem)
        {
            StrToFar(Items);
        }
        TFarEnvGuard Guard;
        Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
                                      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN, NULL, (wchar_t **)Items.c_str(), 0, 0);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, std::wstring Title,
        std::wstring Bottom, const FarMenuItem *Items, int Count,
        const int *BreakKeys, int &BreakCode)
{
    assert(Items);

    std::wstring ATitle = Title;
    std::wstring ABottom = Bottom;
    TFarEnvGuard Guard;
    return FStartupInfo.Menu(FStartupInfo.ModuleNumber, -1, -1, 0,
        Flags, (wchar_t *)StrToFar(ATitle).c_str(), (wchar_t *)StrToFar(ABottom).c_str(), NULL, BreakKeys,
        &BreakCode, Items, Count);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const std::wstring Title,
        const std::wstring Bottom, TStrings *Items, const int *BreakKeys,
        int &BreakCode)
{
    assert(Items && Items->GetCount());
    int Result;
    FarMenuItemEx *MenuItems = new FarMenuItemEx[Items->GetCount()];
    {
        BOOST_SCOPE_EXIT ( (&MenuItems) )
        {
            delete[] MenuItems;
        } BOOST_SCOPE_EXIT_END
        int Selected = -1;
        int Count = 0;
        for (int i = 0; i < Items->GetCount(); i++)
        {
            int Flags = int(Items->GetObject(i));
            if (FLAGCLEAR(Flags, MIF_HIDDEN))
            {
                memset(&MenuItems[Count], 0, sizeof(MenuItems[Count]));
                std::wstring Text = Items->GetString(i).substr(1, sizeof(MenuItems[i].Text)-1);
                MenuItems[Count].Flags = Flags;
                if (MenuItems[Count].Flags & MIF_SELECTED)
                {
                    assert(Selected < 0);
                    Selected = i;
                }
                // strcpy(MenuItems[Count].Text.Text, StrToFar(Text));
                std::wstring Str = StrToFar(Text);
                wcscpy_s((wchar_t *)MenuItems[Count].Text, Str.size(), Str.c_str());
                MenuItems[Count].UserData = i;
                Count++;
            }
        }

        int ResultItem = Menu(Flags | FMENU_USEEXT, Title, Bottom,
                              (const FarMenuItem *)MenuItems, Count, BreakKeys, BreakCode);

        if (ResultItem >= 0)
        {
            Result = MenuItems[ResultItem].UserData;
            if (Selected >= 0)
            {
                Items->PutObject(Selected, (TObject *)(int(Items->GetObject(Selected)) & ~MIF_SELECTED));
            }
            Items->PutObject(Result, (TObject *)(int(Items->GetObject(Result)) | MIF_SELECTED));
        }
        else
        {
            Result = ResultItem;
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const std::wstring Title,
    const std::wstring Bottom, TStrings *Items)
{
    int BreakCode;
    return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::InputBox(const std::wstring Title,
    const std::wstring Prompt, std::wstring &Text, unsigned long Flags,
    const std::wstring HistoryName, int MaxLen, farinputboxvalidate_slot_type *OnValidate)
{
    bool Repeat;
    int Result;
    do
    {
        std::wstring DestText;
        DestText.resize(MaxLen + 1);
        HANDLE ScreenHandle = 0;
        SaveScreen(ScreenHandle);
        std::wstring AText = Text;
        {
            TFarEnvGuard Guard;
            Result = FStartupInfo.InputBox(
                StrToFar((wchar_t *)Title.c_str()).c_str(),
                StrToFar((wchar_t *)Prompt.c_str()).c_str(),
                StrToFar((wchar_t *)HistoryName.c_str()).c_str(),
                StrToFar((wchar_t *)AText.c_str()).c_str(),
                (wchar_t *)DestText.c_str(), MaxLen, NULL,
                FIB_ENABLEEMPTY | FIB_BUTTONS | Flags);
        }
        RestoreScreen(ScreenHandle);
        Repeat = false;
        if (Result)
        {
            Text = StrFromFar(DestText);
            if (OnValidate)
            {
                try
                {
                    farinputboxvalidate_signal_type sig;
                    sig.connect(*OnValidate);
                    sig(Text);
                }
                catch (const std::exception &E)
                {
                    HandleException(&E);
                    Repeat = true;
                }
            }
        }
    }
    while (Repeat);

    return (Result != 0);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::Text(int X, int Y, int Color, std::wstring Str)
{
    TFarEnvGuard Guard;
    FStartupInfo.Text(X, Y, Color, StrToFar(Str).c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FlushText()
{
    TFarEnvGuard Guard;
    FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::WriteConsole(std::wstring Str)
{
    unsigned long Written;
    ::WriteConsole(FConsoleOutput, StrToFar(Str).c_str(), Str.size(), &Written, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(std::wstring Str)
{
    TFarEnvGuard Guard;
    FFarStandardFunctions.CopyToClipboard(StrToFar(Str).c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(TStrings *Strings)
{
    if (Strings->GetCount() > 0)
    {
        if (Strings->GetCount() == 1)
        {
            FarCopyToClipboard(Strings->GetString(0));
        }
        else
        {
            FarCopyToClipboard(Strings->GetText());
        }
    }
}
//---------------------------------------------------------------------------
TPoint TCustomFarPlugin::TerminalInfo(TPoint *Size, TPoint *Cursor)
{
    // DEBUG_PRINTF(L"begin");
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);

    TPoint Result(BufferInfo.dwSize.X, BufferInfo.dwSize.Y);

    if (Size != NULL)
    {
        *Size = Result;
    }

    if (Cursor != NULL)
    {
        Cursor->x = BufferInfo.dwCursorPosition.X;
        Cursor->y = BufferInfo.dwCursorPosition.Y;
    }
    // DEBUG_PRINTF(L"end");
    return Result;
}
//---------------------------------------------------------------------------
HWND TCustomFarPlugin::GetConsoleWindow()
{
    wchar_t Title[512];
    GetConsoleTitle(Title, sizeof(Title) - 1);
    // DEBUG_PRINTF(L"Title = %s", Title);
    StrFromFar(Title);
    HWND Result = FindWindow(NULL, Title);
    return Result;
}
//---------------------------------------------------------------------------
unsigned int TCustomFarPlugin::ConsoleWindowState()
{
    unsigned int Result;
    HWND Window = GetConsoleWindow();
    if (Window != NULL)
    {
        WINDOWPLACEMENT WindowPlacement;
        WindowPlacement.length = sizeof(WindowPlacement);
        Win32Check(GetWindowPlacement(Window, &WindowPlacement));
        Result = WindowPlacement.showCmd;
    }
    else
    {
        Result = SW_SHOWNORMAL;
    }
    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ToggleVideoMode()
{
    HWND Window = GetConsoleWindow();
    if (Window != NULL)
    {
        if (ConsoleWindowState() == SW_SHOWMAXIMIZED)
        {
            if (FNormalConsoleSize.x >= 0)
            {
                COORD Size = { (short)FNormalConsoleSize.x, (short)FNormalConsoleSize.y };

                Win32Check(ShowWindow(Window, SW_RESTORE));

                SMALL_RECT WindowSize;
                WindowSize.Left = 0;
                WindowSize.Top = 0;
                WindowSize.Right = (short)(Size.X - 1);
                WindowSize.Bottom = (short)(Size.Y - 1);
                Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize));

                Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size));
            }
        }
        else
        {
            COORD Size = GetLargestConsoleWindowSize(FConsoleOutput);
            Win32Check((Size.X != 0) || (Size.Y != 0));

            FNormalConsoleSize = TerminalInfo();

            Win32Check(ShowWindow(Window, SW_MAXIMIZE));

            Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size));

            CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
            Win32Check(GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo));

            SMALL_RECT WindowSize;
            WindowSize.Left = 0;
            WindowSize.Top = 0;
            WindowSize.Right = (short)(BufferInfo.dwMaximumWindowSize.X - 1);
            WindowSize.Bottom = (short)(BufferInfo.dwMaximumWindowSize.Y - 1);
            Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize));
        }
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ScrollTerminalScreen(int Rows)
{
    TPoint Size = TerminalInfo();

    SMALL_RECT Source;
    COORD Dest;
    CHAR_INFO Fill;
    Source.Left = 0;
    Source.Top = (char)Rows;
    Source.Right = static_cast<SHORT>(Size.x);
    Source.Bottom = static_cast<SHORT>(Size.y);
    Dest.X = 0;
    Dest.Y = 0;
    Fill.Char.AsciiChar = ' ';
    // Fill.Ñhar.UnicodeChar = L' ';
    Fill.Attributes = 7;
    ScrollConsoleScreenBuffer(FConsoleOutput, &Source, NULL, Dest, &Fill);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowTerminalScreen()
{
    assert(!FTerminalScreenShowing);
    TPoint Size, Cursor;
    TerminalInfo(&Size, &Cursor);

    std::wstring Blank; // = std::wstring::StringOfChar(' ', Size.x);
    Blank.resize(Size.x);
    for (int Y = 0; Y < Size.y; Y++)
    {
        Text(0, Y, 7/* LIGHTGRAY */, Blank);
    }
    FlushText();

    COORD Coord;
    Coord.X = 0;
    Coord.Y = static_cast<SHORT>(Cursor.y);
    SetConsoleCursorPosition(FConsoleOutput, Coord);
    FTerminalScreenShowing = true;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveTerminalScreen()
{
    FTerminalScreenShowing = false;
    FarControl(FCTL_SETUSERSCREEN, 0, NULL);
}
//---------------------------------------------------------------------------
struct TConsoleTitleParam
{
    short Progress;
    short Own;
};
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowConsoleTitle(const std::wstring Title)
{
    wchar_t SaveTitle[512];
    GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
    StrFromFar(SaveTitle);
    TConsoleTitleParam Param;
    Param.Progress = FCurrentProgress;
    Param.Own = !FCurrentTitle.empty() && (FormatConsoleTitle() == SaveTitle);
    assert(sizeof(Param) == sizeof(TObject *));
    if (Param.Own)
    {
        FSavedTitles->AddObject(FCurrentTitle, *(TObject **)&Param);
    }
    else
    {
        FSavedTitles->AddObject(SaveTitle, *(TObject **)&Param);
    }
    FCurrentTitle = Title;
    FCurrentProgress = -1;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClearConsoleTitle()
{
    assert(FSavedTitles->GetCount() > 0);
    std::wstring Title = FSavedTitles->GetString(FSavedTitles->GetCount()-1);
    TObject *Object = (TObject *)FSavedTitles->GetObject(FSavedTitles->GetCount()-1);
    TConsoleTitleParam Param = *(TConsoleTitleParam *)&Object;
    if (Param.Own)
    {
        FCurrentTitle = Title;
        FCurrentProgress = Param.Progress;
        UpdateConsoleTitle();
    }
    else
    {
        FCurrentTitle = L"";
        FCurrentProgress = -1;
        SetConsoleTitle(StrToFar(Title).c_str());
    }
    FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle(const std::wstring Title)
{
    assert(!FCurrentTitle.empty());
    FCurrentTitle = Title;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
    assert(!FCurrentTitle.empty());
    FCurrentProgress = Progress;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::FormatConsoleTitle()
{
    std::wstring Title;
    if (FCurrentProgress >= 0)
    {
        Title = FORMAT(L"{%d%%} %s", FCurrentProgress, FCurrentTitle.c_str());
        std::wstring Buffer;
        Buffer.resize(512);
        GetFarStandardFunctions().sprintf((wchar_t *)Buffer.c_str(), L"{%d%%} %s", FCurrentProgress, FCurrentTitle);
        Title = Buffer;
    }
    else
    {
        Title = FCurrentTitle;
    }
    Title += FAR_TITLE_SUFFIX;
    return Title;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle()
{
    std::wstring Title = FormatConsoleTitle();
    SetConsoleTitle(StrToFar(Title).c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveScreen(HANDLE &Screen)
{
    assert(!Screen);
    TFarEnvGuard Guard;
    Screen = (HANDLE)FStartupInfo.SaveScreen(0, 0, -1, -1);
    assert(Screen);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::RestoreScreen(HANDLE &Screen)
{
    assert(Screen);
    TFarEnvGuard Guard;
    FStartupInfo.RestoreScreen((HANDLE)Screen);
    Screen = 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleException(const std::exception *E, int /*OpMode*/)
{
    assert(E);
    Message(FMSG_WARNING | FMSG_MB_OK, L"", StrToFar(E->what()).c_str());
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::GetMsg(int MsgId)
{
    TFarEnvGuard Guard;
    std::wstring Result = FStartupInfo.GetMsg(FStartupInfo.ModuleNumber, MsgId);
    StrFromFar(Result);
    return Result;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::CheckForEsc()
{
    INPUT_RECORD Rec;
    unsigned long ReadCount;
    while (PeekConsoleInput(FConsoleInput, &Rec, 1, &ReadCount) && ReadCount)
    {
        ReadConsoleInput(FConsoleInput, &Rec, 1, &ReadCount);
        if (Rec.EventType == KEY_EVENT &&
                Rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
                Rec.Event.KeyEvent.bKeyDown)
        {
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Viewer(std::wstring FileName,
        unsigned int Flags, std::wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Viewer(
        StrToFar(FileName).c_str(),
        StrToFar(Title).c_str(), 0, 0, -1, -1, Flags,
        65001);
    return Result;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Editor(std::wstring FileName,
        unsigned int Flags, std::wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Editor(
        StrToFar(FileName).c_str(),
        StrToFar(Title).c_str(), 0, 0, -1, -1, Flags, -1, -1,
        65001);
    return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ResetCachedInfo()
{
    FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
unsigned int TCustomFarPlugin::FarSystemSettings()
{
    if (!FValidFarSystemSettings)
    {
        FFarSystemSettings = FarAdvControl(ACTL_GETSYSTEMSETTINGS);
        FValidFarSystemSettings = true;
    }
    return FFarSystemSettings;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin)
{
    std::wstring Buf;
    int Param = 0;
    switch (Command)
    {
    case FCTL_CLOSEPLUGIN:
    case FCTL_SETPANELDIR:
    // case FCTL_SETANOTHERPANELDIR:
    case FCTL_SETCMDLINE:
    case FCTL_INSERTCMDLINE:
        Buf = (wchar_t *)Param2;
        // Param = StrToFar(Buf);
        break;

    case FCTL_GETCMDLINE:
    case FCTL_GETCMDLINESELECTEDTEXT:
        // ANSI/OEM translation not implemented yet
        assert(false);
        break;
    }

    TFarEnvGuard Guard;
    return FStartupInfo.Control(Plugin, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarAdvControl(int Command, void *Param)
{
    TFarEnvGuard Guard;
    return FStartupInfo.AdvControl(FStartupInfo.ModuleNumber, Command, Param);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarAdvControl(int Command, int Param)
{
    TFarEnvGuard Guard;
    return FStartupInfo.AdvControl(FStartupInfo.ModuleNumber, Command, (void *)Param);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarEditorControl(int Command, void *Param)
{
    std::wstring Buf;
    switch (Command)
    {
    case ECTL_GETINFO:
    case ECTL_SETPARAM:
        // noop
        break;

    case ECTL_SETTITLE:
        Buf = (wchar_t *)Param;
        Param = (wchar_t *)StrToFar(Buf).c_str();
        break;

    default:
        // for other commands, OEM/ANSI conversion to be verified
        assert(false);
        break;
    }

    TFarEnvGuard Guard;
    return FStartupInfo.EditorControl(Command, Param);
}
//---------------------------------------------------------------------------
TFarEditorInfo *TCustomFarPlugin::EditorInfo()
{
    TFarEditorInfo *Result;
    ::EditorInfo *Info = new ::EditorInfo;
    try
    {
        if (FarEditorControl(ECTL_GETINFO, Info))
        {
            Result = new TFarEditorInfo(Info);
        }
        else
        {
            delete Info;
            Result = NULL;
        }
    }
    catch (...)
    {
        delete Info;
        throw;
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarVersion()
{
    if (FFarVersion == 0)
    {
        FFarVersion = FarAdvControl(ACTL_GETFARVERSION);
    }
    return FFarVersion;
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::FormatFarVersion(int Version)
{
    return FORMAT(L"%d.%d.%d", (Version >> 8) & 0xFF, Version & 0xFF, Version >> 16);
    // std::wstring Buffer;
    // Buffer.resize(512);
    // GetFarStandardFunctions().sprintf((wchar_t *)Buffer.c_str(), L"%d.%d.%d", ((Version >> 8) & 0xFF, Version & 0xFF, Version >> 16));
    // return Buffer;
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::TemporaryDir()
{
    std::wstring Result;
    Result.resize(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp((wchar_t *)Result.c_str(), Result.size(), NULL);
    PackStr(Result);
    StrFromFar(Result);
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD *Rec)
{
    int Result;
    if (FFarStandardFunctions.FarInputRecordToKey != NULL)
    {
        TFarEnvGuard Guard;
        Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
    }
    else
    {
        Result = 0;
    }
    return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
unsigned int TCustomFarFileSystem::FInstances = 0;
//---------------------------------------------------------------------------
TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin *APlugin):
    TObject()
{
    FCriticalSection = new TCriticalSection;
    FPlugin = APlugin;
    FPanelInfo[0] = NULL;
    FPanelInfo[1] = NULL;
    FClosed = false;

    memset(&FOpenPluginInfo, 0, sizeof(FOpenPluginInfo));
    ClearOpenPluginInfo(FOpenPluginInfo);
    FInstances++;
};
//---------------------------------------------------------------------------
TCustomFarFileSystem::~TCustomFarFileSystem()
{
    FInstances--;
    ResetCachedInfo();
    ClearOpenPluginInfo(FOpenPluginInfo);
    delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::HandleException(const std::exception *E, int OpMode)
{
    FPlugin->HandleException(E, OpMode);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::Close()
{
    FClosed = true;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::InvalidateOpenPluginInfo()
{
    FOpenPluginInfoValid = false;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClearOpenPluginInfo(OpenPluginInfo &Info)
{
    if (Info.StructSize)
    {
        delete[] Info.HostFile;
        delete[] Info.CurDir;
        delete[] Info.Format;
        delete[] Info.PanelTitle;
        assert(!Info.InfoLines);
        assert(!Info.InfoLinesNumber);
        assert(!Info.DescrFiles);
        assert(!Info.DescrFilesNumber);
        assert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
        for (int Index = 0; Index < Info.PanelModesNumber; Index++)
        {
            assert(Info.PanelModesArray);
            TFarPanelModes::ClearPanelMode(
                const_cast<PanelMode &>(Info.PanelModesArray[Index]));
        }
        delete[] Info.PanelModesArray;
        if (Info.KeyBar)
        {
            TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
            delete Info.KeyBar;
        }
        delete[] Info.ShortcutData;
    }
    memset(&Info, 0, sizeof(Info));
    Info.StructSize = sizeof(Info);
    InvalidateOpenPluginInfo();
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
    ResetCachedInfo();
    if (FClosed)
    {
        // FAR WORKAROUND
        // if plugin is closed from ProcessEvent(FE_IDLE), is does not close,
        // so we close it here on the very next opportunity
        ClosePlugin();
    }
    else
    {
        if (!FOpenPluginInfoValid)
        {
            ClearOpenPluginInfo(FOpenPluginInfo);
            std::wstring HostFile, CurDir, Format, PanelTitle, ShortcutData;
            bool StartSortOrder;
            TFarPanelModes *PanelModes = NULL;
            TFarKeyBarTitles *KeyBarTitles = NULL;
            {
                BOOST_SCOPE_EXIT ( (&PanelModes) (&KeyBarTitles) )
                {
                    delete PanelModes;
                    delete KeyBarTitles;
                } BOOST_SCOPE_EXIT_END
                PanelModes = new TFarPanelModes();
                KeyBarTitles = new TFarKeyBarTitles();
                StartSortOrder = false;

                GetOpenPluginInfoEx(FOpenPluginInfo.Flags, HostFile, CurDir, Format,
                                    PanelTitle, PanelModes, FOpenPluginInfo.StartPanelMode,
                                    FOpenPluginInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);

                FOpenPluginInfo.HostFile = StrToFar(TCustomFarPlugin::DuplicateStr(HostFile)).c_str();
                FOpenPluginInfo.CurDir = StrToFar(TCustomFarPlugin::DuplicateStr(CurDir)).c_str();
                FOpenPluginInfo.Format = StrToFar(TCustomFarPlugin::DuplicateStr(Format)).c_str();
                FOpenPluginInfo.PanelTitle = StrToFar(TCustomFarPlugin::DuplicateStr(PanelTitle)).c_str();
                PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
                FOpenPluginInfo.StartSortOrder = StartSortOrder;
                KeyBarTitles->FillOpenPluginInfo(&FOpenPluginInfo);
                FOpenPluginInfo.ShortcutData = StrToFar(TCustomFarPlugin::DuplicateStr(ShortcutData)).c_str();
            }

            FOpenPluginInfoValid = true;
        }

        memcpy(Info, &FOpenPluginInfo, sizeof(FOpenPluginInfo));
    }
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFindData(
    struct PluginPanelItem **PanelItem, int *ItemsNumber, int OpMode)
{
    ResetCachedInfo();
    TObjectList *PanelItems = new TObjectList();
    bool Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = !FClosed && GetFindDataEx(PanelItems, OpMode);
        if (Result && PanelItems->GetCount())
        {
            *PanelItem = new PluginPanelItem[PanelItems->GetCount()];
            memset(*PanelItem, 0, PanelItems->GetCount() * sizeof(PluginPanelItem));
            *ItemsNumber = PanelItems->GetCount();
            for (int Index = 0; Index < PanelItems->GetCount(); Index++)
            {
                ((TCustomFarPanelItem *)PanelItems->GetItem(Index))->FillPanelItem(
                    &((*PanelItem)[Index]));
            }
        }
        else
        {
            *PanelItem = NULL;
            *ItemsNumber = 0;
        }
    }
    delete PanelItems;
    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::FreeFindData(
    struct PluginPanelItem *PanelItem, int ItemsNumber)
{
    ResetCachedInfo();
    if (PanelItem)
    {
        assert(ItemsNumber > 0);
        for (int Index = 0; Index < ItemsNumber; Index++)
        {
            delete[] PanelItem[Index].Description;
            delete[] PanelItem[Index].Owner;
            for (int CustomIndex = 0; CustomIndex < PanelItem[Index].CustomColumnNumber; CustomIndex++)
            {
                delete[] PanelItem[Index].CustomColumnData[CustomIndex];
            }
            delete[] PanelItem[Index].CustomColumnData;
        }
        delete[] PanelItem;
    }
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessHostFile(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int OpMode)
{
    ResetCachedInfo();
    TObjectList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    bool Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = ProcessHostFileEx(PanelItems, OpMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessKey(int Key, unsigned int ControlState)
{
    ResetCachedInfo();
    return ProcessKeyEx(Key, ControlState);
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessEvent(int Event, void *Param)
{
    ResetCachedInfo();
    return ProcessEventEx(Event, Param);
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::SetDirectory(const wchar_t *Dir, int OpMode)
{
    ResetCachedInfo();
    InvalidateOpenPluginInfo();
    int Result = SetDirectoryEx(StrFromFar(Dir), OpMode);
    InvalidateOpenPluginInfo();
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectory(wchar_t *Name, int OpMode)
{
    ResetCachedInfo();
    std::wstring NameStr = Name;
    int Result;
    {
        BOOST_SCOPE_EXIT ( (&NameStr) (&Name) )
        {
            StrToFar(NameStr);
            if (NameStr != Name)
            {
                // strcpy(Name, NameStr.c_str());
                wcscpy_s(Name, NameStr.size(), NameStr.c_str());
            }
        } BOOST_SCOPE_EXIT_END
        StrFromFar(NameStr);
        Result = MakeDirectoryEx(NameStr, OpMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::DeleteFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int OpMode)
{
    ResetCachedInfo();
    TObjectList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    bool Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = DeleteFilesEx(PanelItems, OpMode);
    }

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int Move, wchar_t *DestPath, int OpMode)
{
    ResetCachedInfo();
    TObjectList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    int Result;
    std::wstring DestPathStr = DestPath;
    {
        BOOST_SCOPE_EXIT ( (&DestPathStr) (&DestPath) (&PanelItems) )
        {
            StrToFar(DestPathStr);
            if (DestPathStr != DestPath)
            {
                wcscpy_s(DestPath, DestPathStr.size(), DestPathStr.c_str());
            }
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        StrFromFar(DestPathStr);
        Result = GetFilesEx(PanelItems, Move, DestPathStr, OpMode);
    }

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int Move, int OpMode)
{
    ResetCachedInfo();
    TObjectList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    int Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = PutFilesEx(PanelItems, Move, OpMode);
    }

    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ResetCachedInfo()
{
    if (FPanelInfo[0])
    {
        SAFE_DESTROY(FPanelInfo[false]);
    }
    if (FPanelInfo[1])
    {
        SAFE_DESTROY(FPanelInfo[true]);
    }
}
//---------------------------------------------------------------------------
TFarPanelInfo *TCustomFarFileSystem::GetPanelInfo(int Another)
{
    if (FPanelInfo[Another] == NULL)
    {
        ::PanelInfo *Info = new ::PanelInfo;
        bool res = false;
        if (Another)
            res = FPlugin->FarControl(FCTL_GETPANELINFO, 0, (LONG_PTR)&Info, PANEL_PASSIVE);
        else
            res = FPlugin->FarControl(FCTL_GETPANELINFO, 0, (LONG_PTR)&Info, PANEL_ACTIVE);
        if (!res) // FarControl(Another == 0 ? FCTL_GETPANELINFO : FCTL_GETANOTHERPANELINFO, Info))
        {
            memset(Info, 0, sizeof(*Info));
            assert(false);
        }
        FPanelInfo[Another] = new TFarPanelInfo(Info, (Another == 0 ? this : NULL));
    }
    return FPanelInfo[Another];
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::FarControl(int Command, int Param1, LONG_PTR Param2)
{
    return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
    unsigned int PrevInstances = FInstances;
    InvalidateOpenPluginInfo();
    // FarControl(Another ? FCTL_UPDATEANOTHERPANEL : FCTL_UPDATEPANEL,
               // (void *)(!ClearSelection));
    if (Another)
        FPlugin->FarControl(FCTL_UPDATEPANEL, 0, (LONG_PTR)(!ClearSelection), PANEL_PASSIVE);
    else
        FPlugin->FarControl(FCTL_UPDATEPANEL, 0, (LONG_PTR)(!ClearSelection), PANEL_ACTIVE);
    return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::RedrawPanel(bool Another)
{
    // FarControl(Another ? FCTL_REDRAWANOTHERPANEL : FCTL_REDRAWPANEL, NULL);
    if (Another)
        FPlugin->FarControl(FCTL_REDRAWPANEL, 0, (LONG_PTR)0, PANEL_PASSIVE);
    else
        FPlugin->FarControl(FCTL_REDRAWPANEL, 0, (LONG_PTR)0, PANEL_ACTIVE);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClosePlugin()
{
    FClosed = true;
    FarControl(FCTL_CLOSEPLUGIN, 0, (LONG_PTR)L"C:\\");
    // FAR WORKAROUND
    // Calling UpdatePanel() is necessary, otherwise plugin remains in panel,
    // but it causes FAR to fail
    // UpdatePanel();
}
//---------------------------------------------------------------------------
std::wstring TCustomFarFileSystem::GetMsg(int MsgId)
{
    return FPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
TCustomFarFileSystem *TCustomFarFileSystem::GetOppositeFileSystem()
{
    return FPlugin->GetPanelFileSystem(true, this);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsActiveFileSystem()
{
    // Cannot use PanelInfo::Focus as it occasionally does not work from editor;
    return (this == FPlugin->GetPanelFileSystem());
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsLeft()
{
    return (GetPanelInfo(0)->GetBounds().Left <= 0);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsRight()
{
    return !IsLeft();
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessKeyEx(int /*Key*/, unsigned int /*ControlState*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessEventEx(int /*Event*/, void * /*Param*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::SetDirectoryEx(const std::wstring /*Dir*/, int /*OpMode*/)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectoryEx(std::wstring & /*Name*/, int /*OpMode*/)
{
    return -1;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
        std::wstring & /*DestPath*/, int /*OpMode*/)
{
    return 0;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
        bool /*Move*/, int /*OpMode*/)
{
    return 0;
}
//---------------------------------------------------------------------------
TObjectList *TCustomFarFileSystem::CreatePanelItemList(
    struct PluginPanelItem *PanelItem, int ItemsNumber)
{
    TObjectList *PanelItems = new TObjectList();
    try
    {
        for (int Index = 0; Index < ItemsNumber; Index++)
        {
            PanelItems->Add((TObject *)new TFarPanelItem(&PanelItem[Index]));
        }
    }
    catch (...)
    {
        delete PanelItems;
        throw;
    }
    return PanelItems;
}
//---------------------------------------------------------------------------
TFarPanelModes::TFarPanelModes() : TObject()
{
    memset(FPanelModes, 0, sizeof(FPanelModes));
    FReferenced = false;
}
//---------------------------------------------------------------------------
TFarPanelModes::~TFarPanelModes()
{
    if (!FReferenced)
    {
        for (int Index = 0; Index < LENOF(FPanelModes); Index++)
        {
            ClearPanelMode(FPanelModes[Index]);
        }
    }
}
//---------------------------------------------------------------------------
void TFarPanelModes::SetPanelMode(int Mode, const std::wstring ColumnTypes,
        const std::wstring ColumnWidths, TStrings *ColumnTitles,
        bool FullScreen, bool DetailedStatus, bool AlignExtensions,
        bool CaseConversion, const std::wstring StatusColumnTypes,
        const std::wstring StatusColumnWidths)
{
    int ColumnTypesCount = !ColumnTypes.empty() ? CommaCount(ColumnTypes) + 1 : 0;
    assert(Mode >= 0 && Mode < LENOF(FPanelModes));
    assert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

    ClearPanelMode(FPanelModes[Mode]);
    wchar_t *Titles[PANEL_MODES_COUNT];
    FPanelModes[Mode].ColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnTypes)).c_str();
    FPanelModes[Mode].ColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnWidths)).c_str();
    if (ColumnTitles)
    {
        FPanelModes[Mode].ColumnTitles = new wchar_t *[ColumnTypesCount];
        for (int Index = 0; Index < ColumnTypesCount; Index++)
        {
            // FPanelModes[Mode].ColumnTitles[Index] = StrToFar(
                // TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index)));
            Titles[Index] = (wchar_t *)StrToFar(
                TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index))).c_str();
        }
        FPanelModes[Mode].ColumnTitles = Titles;
    }
    FPanelModes[Mode].FullScreen = FullScreen;
    FPanelModes[Mode].DetailedStatus = DetailedStatus;
    FPanelModes[Mode].AlignExtensions = AlignExtensions;
    FPanelModes[Mode].CaseConversion = CaseConversion;

    FPanelModes[Mode].StatusColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnTypes)).c_str();
    FPanelModes[Mode].StatusColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnWidths)).c_str();
}
//---------------------------------------------------------------------------
void TFarPanelModes::ClearPanelMode(PanelMode &Mode)
{
    if (Mode.ColumnTypes)
    {
        int ColumnTypesCount = Mode.ColumnTypes ?
                               CommaCount(std::wstring(Mode.ColumnTypes)) + 1 : 0;

        delete[] Mode.ColumnTypes;
        delete[] Mode.ColumnWidths;
        if (Mode.ColumnTitles)
        {
            for (int Index = 0; Index < ColumnTypesCount; Index++)
            {
                delete[] Mode.ColumnTitles[Index];
            }
            delete[] Mode.ColumnTitles;
        }
        delete[] Mode.StatusColumnTypes;
        delete[] Mode.StatusColumnWidths;
        memset(&Mode, 0, sizeof(Mode));
    }
}
//---------------------------------------------------------------------------
void TFarPanelModes::FillOpenPluginInfo(struct OpenPluginInfo *Info)
{
    assert(Info);
    Info->PanelModesNumber = LENOF(FPanelModes);
    PanelMode *PanelModesArray = new PanelMode[LENOF(FPanelModes)];
    Info->PanelModesArray = PanelModesArray;
    memcpy(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
    FReferenced = true;
}
//---------------------------------------------------------------------------
int TFarPanelModes::CommaCount(const std::wstring ColumnTypes)
{
    int Count = 0;
    for (int Index = 1; Index <= ColumnTypes.size(); Index++)
    {
        if (ColumnTypes[Index] == ',')
        {
            Count++;
        }
    }
    return Count;
}
//---------------------------------------------------------------------------
TFarKeyBarTitles::TFarKeyBarTitles()
{
    memset(&FKeyBarTitles, 0, sizeof(FKeyBarTitles));
    FReferenced = false;
}
//---------------------------------------------------------------------------
TFarKeyBarTitles::~TFarKeyBarTitles()
{
    if (!FReferenced)
    {
        ClearKeyBarTitles(FKeyBarTitles);
    }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearFileKeyBarTitles()
{
    ClearKeyBarTitle(fsNone, 3, 8);
    ClearKeyBarTitle(fsCtrl, 4, 11);
    ClearKeyBarTitle(fsAlt, 3, 7);
    ClearKeyBarTitle(fsShift, 1, 8);
    ClearKeyBarTitle(fsCtrlShift, 3, 4);
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
        int FunctionKeyStart, int FunctionKeyEnd)
{
    if (!FunctionKeyEnd)
    {
        FunctionKeyEnd = FunctionKeyStart;
    }
    for (int Index = FunctionKeyStart; Index <= FunctionKeyEnd; Index++)
    {
        SetKeyBarTitle(ShiftStatus, Index, L"");
    }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
        int FunctionKey, const std::wstring Title)
{
    assert(FunctionKey >= 1 && FunctionKey <= LENOF(FKeyBarTitles.Titles));
    wchar_t **Titles;
    switch (ShiftStatus)
    {
    case fsNone:
        Titles = FKeyBarTitles.Titles;
        break;
    case fsCtrl:
        Titles = FKeyBarTitles.CtrlTitles;
        break;
    case fsAlt:
        Titles = FKeyBarTitles.AltTitles;
        break;
    case fsShift:
        Titles = FKeyBarTitles.ShiftTitles;
        break;
    case fsCtrlShift:
        Titles = FKeyBarTitles.CtrlShiftTitles;
        break;
    case fsAltShift:
        Titles = FKeyBarTitles.AltShiftTitles;
        break;
    case fsCtrlAlt:
        Titles = FKeyBarTitles.CtrlAltTitles;
        break;
    default:
        assert(false);
    }
    if (Titles[FunctionKey-1])
    {
        delete[] Titles[FunctionKey-1];
    }
    Titles[FunctionKey-1] = (wchar_t *)StrToFar(TCustomFarPlugin::DuplicateStr(Title, true)).c_str();
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles &Titles)
{
    for (int Index = 0; Index < LENOF(Titles.Titles); Index++)
    {
        delete[] Titles.Titles[Index];
        delete[] Titles.CtrlTitles[Index];
        delete[] Titles.AltTitles[Index];
        delete[] Titles.ShiftTitles[Index];
        delete[] Titles.CtrlShiftTitles[Index];
        delete[] Titles.AltShiftTitles[Index];
        delete[] Titles.CtrlAltTitles[Index];
    }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::FillOpenPluginInfo(struct OpenPluginInfo *Info)
{
    assert(Info);
    KeyBarTitles *KeyBar = new KeyBarTitles;
    Info->KeyBar = KeyBar;
    memcpy(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
    FReferenced = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
std::wstring TCustomFarPanelItem::GetCustomColumnData(int /*Column*/)
{
    assert(false);
    return L"";
}
//---------------------------------------------------------------------------
void TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem *PanelItem)
{
    assert(PanelItem);

    std::wstring FileName;
    __int64 Size = 0;
    TDateTime LastWriteTime;
    TDateTime LastAccess;
    std::wstring Description;
    std::wstring Owner;

    void *UserData = (void *)PanelItem->UserData;
    GetData(PanelItem->Flags, FileName, Size, PanelItem->FindData.dwFileAttributes,
            LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
            UserData, PanelItem->CustomColumnNumber);

    FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
    FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
    PanelItem->FindData.ftCreationTime = FileTime;
    PanelItem->FindData.ftLastAccessTime = FileTimeA;
    PanelItem->FindData.ftLastWriteTime = FileTime;
    PanelItem->FindData.nFileSize = Size;
    // PanelItem->FindData.FileSizeHigh = (long int)(Size >> 32);
    // PanelItem->PackSize = (long int)Size;
    // ASCOPY(PanelItem->FindData.lpwszFileName, FileName);
    wcscpy_s((wchar_t *)PanelItem->FindData.lpwszFileName, FileName.size(), FileName.c_str());
    // StrToFar(PanelItem->FindData.lpwszFileName);
    PanelItem->Description = StrToFar(TCustomFarPlugin::DuplicateStr(Description)).c_str();
    PanelItem->Owner = StrToFar(TCustomFarPlugin::DuplicateStr(Owner)).c_str();

    // PanelItem->CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
    wchar_t **CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
    for (int Index = 0; Index < PanelItem->CustomColumnNumber; Index++)
    {
        CustomColumnData[Index] =
            (wchar_t *)StrToFar(TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index))).c_str();
    }
    PanelItem->CustomColumnData = CustomColumnData;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelItem::TFarPanelItem(PluginPanelItem *APanelItem):
    TCustomFarPanelItem()
{
    assert(APanelItem);
    FPanelItem = APanelItem;
}
//---------------------------------------------------------------------------
void TFarPanelItem::GetData(
    unsigned long & /*Flags*/, std::wstring & /*FileName*/, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
    std::wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
std::wstring TFarPanelItem::GetCustomColumnData(int /*Column*/)
{
    assert(false);
    return L"";
}
//---------------------------------------------------------------------------
unsigned long TFarPanelItem::GetFlags()
{
    return FPanelItem->Flags;
}
//---------------------------------------------------------------------------
std::wstring TFarPanelItem::GetFileName()
{
    std::wstring Result = FPanelItem->FindData.lpwszFileName;
    return StrFromFar(Result);
}
//---------------------------------------------------------------------------
void *TFarPanelItem::GetUserData()
{
    return (void *)FPanelItem->UserData;
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetSelected()
{
    return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}
//---------------------------------------------------------------------------
void TFarPanelItem::SetSelected(bool value)
{
    if (value)
    {
        FPanelItem->Flags |= PPIF_SELECTED;
    }
    else
    {
        FPanelItem->Flags &= ~PPIF_SELECTED;
    }
}
//---------------------------------------------------------------------------
unsigned long TFarPanelItem::GetFileAttributes()
{
    return FPanelItem->FindData.dwFileAttributes;
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsParentDirectory()
{
    return (GetFileName() == L"..");
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsFile()
{
    return (GetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
THintPanelItem::THintPanelItem(const std::wstring AHint) :
    TCustomFarPanelItem()
{
    FHint = AHint;
}
//---------------------------------------------------------------------------
void THintPanelItem::GetData(
    unsigned long & /*Flags*/, std::wstring &FileName, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
    std::wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
    FileName = FHint;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelInfo::TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner):
    TObject()
{
    assert(APanelInfo);
    FPanelInfo = APanelInfo;
    FOwner = AOwner;
    FItems = NULL;
}
//---------------------------------------------------------------------------
TFarPanelInfo::~TFarPanelInfo()
{
    delete FPanelInfo;
    delete FItems;
}
//---------------------------------------------------------------------------
int TFarPanelInfo::GetItemCount()
{
    return FPanelInfo->ItemsNumber;
}
//---------------------------------------------------------------------------
TRect TFarPanelInfo::GetBounds()
{
    RECT rect = FPanelInfo->PanelRect;
    return TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
int TFarPanelInfo::GetSelectedCount()
{
    int Count = FPanelInfo->SelectedItemsNumber;

    if (Count == 1)
    {
        // (FPanelInfo->SelectedItems(0).Flags & PPIF_SELECTED) == 0)
        Count = 0;
    }

    return Count;
}
//---------------------------------------------------------------------------
TObjectList *TFarPanelInfo::GetItems()
{
    if (!FItems)
    {
        FItems = new TObjectList();
        for (int Index = 0; Index < FPanelInfo->ItemsNumber; Index++)
        {
            // FItems->Add((TObject *)new TFarPanelItem(&FPanelInfo->PanelItems[Index]));
        }
    }
    return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::FindFileName(const std::wstring FileName)
{
    TObjectList *AItems = GetItems();
    TFarPanelItem *PanelItem;
    for (int Index = 0; Index < AItems->GetCount(); Index++)
    {
        PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
        if (PanelItem->GetFileName() == FileName)
        {
            return PanelItem;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::FindUserData(void *UserData)
{
    TObjectList *AItems = GetItems();
    TFarPanelItem *PanelItem;
    for (int Index = 0; Index < AItems->GetCount(); Index++)
    {
        PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
        if (PanelItem->GetUserData() == UserData)
        {
            return PanelItem;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
void TFarPanelInfo::ApplySelection()
{
    // for "another panel info", there's no owner
    assert(FOwner != NULL);
    FOwner->FarControl(FCTL_SETSELECTION, 0, (LONG_PTR)FPanelInfo);
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::GetFocusedItem()
{
    int Index = GetFocusedIndex();
    if ((Index >= 0) && (GetItems()->GetCount() > 0))
    {
        assert(Index < GetItems()->GetCount());
        return (TFarPanelItem *)GetItems()->GetItem(Index);
    }
    else
    {
        return NULL;
    }
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedItem(TFarPanelItem *value)
{
    int Index = GetItems()->IndexOf((TObject *)value);
    assert(Index >= 0);
    SetFocusedIndex(Index);
}
//---------------------------------------------------------------------------
int TFarPanelInfo::GetFocusedIndex()
{
    return FPanelInfo->CurrentItem;
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedIndex(int value)
{
    // for "another panel info", there's no owner
    assert(FOwner != NULL);
    if (GetFocusedIndex() != value)
    {
        assert(value >= 0 && value < FPanelInfo->ItemsNumber);
        FPanelInfo->CurrentItem = value;
        PanelRedrawInfo PanelInfo;
        PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
        PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
        FOwner->FarControl(FCTL_REDRAWPANEL, 0, (LONG_PTR)&PanelInfo);
    }
}
//---------------------------------------------------------------------------
TFarPanelType TFarPanelInfo::GetType()
{
    switch (FPanelInfo->PanelType)
    {
    case PTYPE_FILEPANEL:
        return ptFile;

    case PTYPE_TREEPANEL:
        return ptTree;

    case PTYPE_QVIEWPANEL:
        return ptQuickView;

    case PTYPE_INFOPANEL:
        return ptInfo;

    default:
        assert(false);
        return ptFile;
    }
}
//---------------------------------------------------------------------------
bool TFarPanelInfo::GetIsPlugin()
{
    return (FPanelInfo->Plugin != 0);
}
//---------------------------------------------------------------------------
std::wstring TFarPanelInfo::GetCurrentDirectory()
{
    std::wstring Result = L""; //FPanelInfo->CurDir;
    return StrFromFar(Result);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarMenuItems::TFarMenuItems() :
    TStringList()
{
    FItemFocused = -1;
}
//---------------------------------------------------------------------------
void TFarMenuItems::Clear()
{
    FItemFocused = -1;
    TStringList::Clear();
}
//---------------------------------------------------------------------------
void TFarMenuItems::Delete(int Index)
{
    if (Index == FItemFocused)
    {
        FItemFocused = -1;
    }
    TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void TFarMenuItems::PutObject(int Index, TObject *AObject)
{
    TStringList::PutObject(Index, AObject);
    bool Focused = (reinterpret_cast<int>(AObject) & MIF_SEPARATOR) != 0;
    if ((Index == GetItemFocused()) && !Focused)
    {
        FItemFocused = -1;
    }
    if (Focused)
    {
        if (GetItemFocused() >= 0)
        {
            SetFlag(GetItemFocused(), MIF_SELECTED, false);
        }
        FItemFocused = Index;
    }
}
//---------------------------------------------------------------------------
int TFarMenuItems::Add(std::wstring Text, bool Visible)
{
    int Result = TStringList::Add(Text);
    if (!Visible)
    {
        SetFlag(GetCount() - 1, MIF_HIDDEN, true);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarMenuItems::AddSeparator(bool Visible)
{
    Add(L"");
    SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
    if (!Visible)
    {
        SetFlag(GetCount() - 1, MIF_HIDDEN, true);
    }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetItemFocused(int value)
{
    if (GetItemFocused() != value)
    {
        if (GetItemFocused() >= 0)
        {
            SetFlag(GetItemFocused(), MIF_SELECTED, false);
        }
        FItemFocused = value;
        SetFlag(GetItemFocused(), MIF_SELECTED, true);
    }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetFlag(int Index, int Flag, bool Value)
{
    if (GetFlag(Index, Flag) != Value)
    {
        int F = int(GetObject(Index));
        if (Value)
        {
            F |= Flag;
        }
        else
        {
            F &= ~Flag;
        }
        PutObject(Index, (TObject *)F);
    }
}
//---------------------------------------------------------------------------
bool TFarMenuItems::GetFlag(int Index, int Flag)
{
    return int(GetObject(Index)) & Flag;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEditorInfo::TFarEditorInfo(EditorInfo *Info) :
    FEditorInfo(Info)
{
}
//---------------------------------------------------------------------------
TFarEditorInfo::~TFarEditorInfo()
{
    delete FEditorInfo;
}
//---------------------------------------------------------------------------
int TFarEditorInfo::GetEditorID()
{
    return FEditorInfo->EditorID;
}
//---------------------------------------------------------------------------
std::wstring TFarEditorInfo::GetFileName()
{
    std::wstring Result = L""; // FEditorInfo->GetFileName();
    return StrFromFar(Result);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
    assert(AreFileApisANSI());
    assert(FarPlugin != NULL);
    if (!FarPlugin->GetANSIApis())
    {
        SetFileApisToOEM();
    }
}
//---------------------------------------------------------------------------
TFarEnvGuard::~TFarEnvGuard()
{
    assert(FarPlugin != NULL);
    if (!FarPlugin->GetANSIApis())
    {
        assert(!AreFileApisANSI());
        SetFileApisToANSI();
    }
    else
    {
        assert(AreFileApisANSI());
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPluginEnvGuard::TFarPluginEnvGuard()
{
    assert(FarPlugin != NULL);

    // keep the assertion, but be robust, in case we are called from incorrectly
    // programmed plugin (e.g. EMenu)
    FANSIApis = AreFileApisANSI();
    assert(FANSIApis == FarPlugin->GetANSIApis());

    if (!FANSIApis)
    {
        SetFileApisToANSI();
    }
}
//---------------------------------------------------------------------------
TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
    assert(FarPlugin != NULL);
    assert(AreFileApisANSI());

    if (!FANSIApis)
    {
        SetFileApisToOEM();
    }
}
//---------------------------------------------------------------------------
void FarWrapText(std::wstring Text, TStrings *Result, int MaxWidth)
{
    int TabSize = 8;
    TStringList Lines;
    Lines.SetText(Text);
    TStringList WrappedLines;
    for (int Index = 0; Index < Lines.GetCount(); Index++)
    {
        std::wstring WrappedLine = Lines.GetString(Index);
        if (!WrappedLine.empty())
        {
            WrappedLine = ::ReplaceChar(WrappedLine, '\'', '\3');
            WrappedLine = ::ReplaceChar(WrappedLine, '\"', '\4');
            // FIXME WrappedLine = ::WrapText(WrappedLine, MaxWidth);
            WrappedLine = ::ReplaceChar(WrappedLine, '\3', '\'');
            WrappedLine = ::ReplaceChar(WrappedLine, '\4', '\"');
            WrappedLines.SetText(WrappedLine);
            for (int WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); WrappedIndex++)
            {
                std::wstring FullLine = WrappedLines.GetString(WrappedIndex);
                do
                {
                    // WrapText does not wrap when not possible, enforce it
                    // (it also does not wrap when the line is longer than maximum only
                    // because of trailing dot or similar)
                    std::wstring Line = FullLine.substr(0, MaxWidth);
                    FullLine.erase(0, MaxWidth);

                    int P;
                    while ((P = Line.find_first_of(L"\t")) >= 0)
                    {
                        Line.erase(P, 1);
                        Line.insert(P, ::StringOfChar(' ',
                            ((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1));
                        std::wstring s;
                        s.resize(((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1);
                        Line.append(s.c_str(), P);
                    }
                    // DEBUG_PRINTF(L"Line = %s", Line.c_str());
                    Result->Add(Line);
                }
                while (!FullLine.empty());
            }
        }
        else
        {
            Result->Add(L"");
        }
    }
}
//---------------------------------------------------------------------------
std::wstring StrFromFar(const char *S)
{
    std::wstring Result; // = S;
    // OemToChar(Result.c_str(), Result.c_str());
    return Result;
}
