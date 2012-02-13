//---------------------------------------------------------------------------
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "FarPlugin.h"
#include "FarDialog.h"
#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "puttyexp.h"

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
TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst) :
    nb::TObject()
{
    // DEBUG_PRINTF(L"TCustomFarPlugin: begin");
    InitPlatformId();
    Self = this;
    FFarThread = GetCurrentThreadId();
    FCriticalSection = new TCriticalSection;
    FHandle = HInst;
    FANSIApis = AreFileApisANSI() > 0;
    FFarVersion = 0;
    FTerminalScreenShowing = false;

    FOpenedPlugins = new nb::TObjectList();
    FOpenedPlugins->SetOwnsObjects(false);
    FSavedTitles = new nb::TStringList();
    FTopDialog = NULL;
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
        FNormalConsoleSize = nb::TPoint(-1, -1);
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
VersionInfo TCustomFarPlugin::GetMinFarVersion()
{
    return MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo *Info)
{
    try
    {
        ResetCachedInfo();
        // Info->StructSize = 336 for FAR 1.65
        memset(&FStartupInfo, 0, sizeof(FStartupInfo));
        memmove(&FStartupInfo, Info,
               Info->StructSize >= sizeof(FStartupInfo) ?
               sizeof(FStartupInfo) : static_cast<size_t>(Info->StructSize));
        StrFromFar(FStartupInfo.ModuleName);
        // the minimum we really need
        assert(FStartupInfo.GetMsg != NULL);
        assert(FStartupInfo.Message != NULL);

        memset(&FFarStandardFunctions, 0, sizeof(FFarStandardFunctions));
        size_t FSFOffset = (static_cast<const char *>(reinterpret_cast<const void *>(&Info->FSF)) -
                         static_cast<const char *>(reinterpret_cast<const void *>(Info)));
        if (static_cast<size_t>(Info->StructSize) > FSFOffset)
        {
            memmove(&FFarStandardFunctions, Info->FSF,
                   static_cast<size_t>(Info->FSF->StructSize) >= sizeof(FFarStandardFunctions) ?
                   sizeof(FFarStandardFunctions) : Info->FSF->StructSize);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
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

        nb::TStringList DiskMenu;
        nb::TStringList PluginMenu;
        nb::TStringList PluginConfig;
        nb::TStringList CommandPrefixes;

        ClearPluginInfo(FPluginInfo);

        GetPluginInfoEx(FPluginInfo.Flags, &DiskMenu, &PluginMenu,
                        &PluginConfig, &CommandPrefixes);

#define COMPOSESTRINGARRAY(NAME) \
        if (NAME.GetCount()) \
        { \
          wchar_t **StringArray = new wchar_t *[NAME.GetCount()]; \
          GUID *Guids = new GUID[NAME.GetCount()]; \
          FPluginInfo.NAME.Guids = Guids; \
          FPluginInfo.NAME.Strings = StringArray; \
          FPluginInfo.NAME.Count = NAME.GetCount(); \
          for (size_t Index = 0; Index < static_cast<int>(NAME.GetCount()); Index++) \
          { \
            StringArray[Index] = StrToFar(DuplicateStr(NAME.GetString(Index))); \
            Guids[Index] = *reinterpret_cast<const GUID *>(NAME.GetObject(Index)); \
          } \
        }

        COMPOSESTRINGARRAY(DiskMenu);
        COMPOSESTRINGARRAY(PluginMenu);
        COMPOSESTRINGARRAY(PluginConfig);

#undef COMPOSESTRINGARRAY
        // FIXME
        /*
        if (DiskMenu.GetCount())
        {
            wchar_t *NumberArray = new wchar_t[DiskMenu.GetCount()];
            FPluginInfo.DiskMenuNumbers = &NumberArray;
            for (int Index = 0; Index < DiskMenu.GetCount(); Index++)
            {
                NumberArray[Index] = (int)DiskMenu.GetObject(Index);
            }
        }
        */

        std::wstring CommandPrefix;
        for (size_t Index = 0; Index < CommandPrefixes.GetCount(); Index++)
        {
            CommandPrefix = CommandPrefix + (CommandPrefix.empty() ? L"" : L":") +
                            CommandPrefixes.GetString(Index);
        }
        // DEBUG_PRINTF(L"CommandPrefix = %s", CommandPrefix.c_str());
        FPluginInfo.CommandPrefix = StrToFar(DuplicateStr(CommandPrefix));

        memmove(Info, &FPluginInfo, sizeof(FPluginInfo));
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
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
      for (int Index = 0; Index < Info.NAME.Count; Index++) \
      { \
        delete[] Info.NAME.Strings[Index]; \
      } \
      delete[] Info.NAME.Strings; \
      delete[] Info.NAME.Guids; \
      Info.NAME.Strings = NULL;

        FREESTRINGARRAY(DiskMenu);
        FREESTRINGARRAY(PluginMenu);
        FREESTRINGARRAY(PluginConfig);

#undef FREESTRINGARRAY

        // FIXME delete[] Info.DiskMenuNumbers;
        delete[] Info.CommandPrefix;
    }
    memset(&Info, 0, sizeof(Info));
    Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
wchar_t *TCustomFarPlugin::DuplicateStr(const std::wstring Str, bool AllowEmpty)
{
    if (Str.empty() && !AllowEmpty)
    {
        return NULL;
    }
    else
    {
        // DEBUG_PRINTF(L"Str = %s", Str.c_str());
        const size_t sz = Str.size() + 1;
        wchar_t *Result = new wchar_t[sz];
        wcscpy_s(Result, sz, Str.c_str());
        return Result;
    }
}
//---------------------------------------------------------------------------
TCustomFarFileSystem *TCustomFarPlugin::GetPanelFileSystem(bool Another,
        HANDLE Plugin)
{
    // DEBUG_PRINTF(L"begin");
    TCustomFarFileSystem *Result = NULL;
    PanelInfo Info;
    FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(&Info), Another ? PANEL_PASSIVE : PANEL_ACTIVE);

    if (Info.PluginHandle)
    {
        RECT Bounds = Info.PanelRect;
        TCustomFarFileSystem *FileSystem;
        size_t Index = 0;
        while (!Result && (Index < FOpenedPlugins->GetCount()))
        {
            FileSystem = dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
            assert(FileSystem);
            nb::TRect bounds = FileSystem->GetPanelInfo()->GetBounds();
            if (bounds == Bounds)
            {
                Result = FileSystem;
            }
            Index++;
        }
    }
    // DEBUG_PRINTF(L"end");
    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::InvalidateOpenPanelInfo()
{
    for (size_t Index = 0; Index < FOpenedPlugins->GetCount(); Index++)
    {
        TCustomFarFileSystem *FileSystem =
            dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
        FileSystem->InvalidateOpenPanelInfo();
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Configure(const struct ConfigureInfo *Info)
{
    try
    {
        ResetCachedInfo();
        int Result = ConfigureEx(0);
        InvalidateOpenPanelInfo();

        return Result;
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
        HandleException(&E);
        return false;
    }
}
//---------------------------------------------------------------------------
void *TCustomFarPlugin::OpenPlugin(const struct OpenInfo *Info)
{
    try
    {
        ResetCachedInfo();
        int Item = 0;
        if (*Info->Guid == MenuCommandsGuid)
            Item = 1;
        if ((Info->OpenFrom == OPEN_SHORTCUT) || (Info->OpenFrom == OPEN_COMMANDLINE))
        {
          Item = static_cast<int>(static_cast<INT_PTR>(Info->Data));
        }
        TCustomFarFileSystem *Result = OpenPluginEx(Info->OpenFrom, Item);

        if (Result)
        {
            FOpenedPlugins->Add(Result);
        }
        else
        {
            Result = static_cast<TCustomFarFileSystem *>(INVALID_HANDLE_VALUE);
        }

        return Result;
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
        HandleException(&E);
        return static_cast<void *>(INVALID_HANDLE_VALUE);
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClosePanel(void *Plugin)
{
    try
    {
        ResetCachedInfo();
        TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);
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
        DEBUG_PRINTF(L"before HandleException");
        HandleException(&E);
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleFileSystemException(
    TCustomFarFileSystem *FileSystem, const std::exception *E, int OpMode)
{
    // This method is called as last-resort exception handler before
    // leaving plugin API. Especially for API fuctions that must update
    // panel contents on themselves (like ProcessPanelInput), the instance of filesystem
    // may not exists anymore.
    // Check against object pointer is stupid, but no other idea so far.
    if (FOpenedPlugins->IndexOf(FileSystem) != -1)
    {
        DEBUG_PRINTF(L"before FileSystem->HandleException");
        FileSystem->HandleException(E, OpMode);
    }
    else
    {
        DEBUG_PRINTF(L"before HandleException");
        HandleException(E, OpMode);
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::GetOpenPanelInfo(struct OpenPanelInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            FileSystem->GetOpenPanelInfo(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetFindData(struct GetFindDataInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->GetFindData(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FreeFindData(const struct FreeFindDataInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            FileSystem->FreeFindData(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessHostFile(const struct ProcessHostFileInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessHostFile))
        {
            assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

            {
                TGuard Guard(FileSystem->GetCriticalSection());
                return FileSystem->ProcessHostFile(Info);
            }
        }
        else
        {
            return 0;
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessKey))
        {
            assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

            {
                TGuard Guard(FileSystem->GetCriticalSection());
                return FileSystem->ProcessPanelInput(Info);
            }
        }
        else
        {
            return 0;
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E);
        // when error occurs, assume that key can be handled by plugin and
        // should not be processed by FAR
        return 1;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEvent(HANDLE Plugin, int Event, void *Param)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    try
    {
        ResetCachedInfo();
        if (HandlesFunction(hfProcessEvent))
        {
            assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

            std::wstring Buf;
            if ((Event == FE_CHANGEVIEWMODE) || (Event == FE_COMMAND))
            {
                Buf = static_cast<wchar_t *>(Param);
                StrFromFar(Buf);
                Param = const_cast<void *>(reinterpret_cast<const void *>(Buf.c_str()));
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
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E);
        return Event == FE_COMMAND ? true : false;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::SetDirectory(const struct SetDirectoryInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->SetDirectory(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MakeDirectory(struct MakeDirectoryInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->MakeDirectory(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::DeleteFiles(const struct DeleteFilesInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->DeleteFiles(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetFiles(struct GetFilesInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->GetFiles(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        // display error even for OPM_FIND
        HandleFileSystemException(FileSystem, &E, Info->OpMode & ~OPM_FIND);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::PutFiles(const struct PutFilesInfo *Info)
{
    TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
    try
    {
        ResetCachedInfo();
        assert(FOpenedPlugins->IndexOf(FileSystem) != -1);

        {
            TGuard Guard(FileSystem->GetCriticalSection());
            return FileSystem->PutFiles(Info);
        }
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleFileSystemException");
        HandleFileSystemException(FileSystem, &E, Info->OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEditorEvent(const struct ProcessEditorEventInfo *Info)
{
    try
    {
        ResetCachedInfo();

        return ProcessEditorEventEx(Info);
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
        HandleException(&E);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::ProcessEditorInput(const struct ProcessEditorInputInfo *Info)
{
    try
    {
        ResetCachedInfo();

        return ProcessEditorInputEx(&Info->Rec);
    }
    catch (const std::exception &E)
    {
        DEBUG_PRINTF(L"before HandleException");
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
size_t TCustomFarPlugin::MaxMenuItemLength()
{
    // got from maximal length of path in FAR's folders history
    return TerminalInfo().x - 13;
}
//---------------------------------------------------------------------------
size_t TCustomFarPlugin::MaxLength(nb::TStrings *Strings)
{
    size_t Result = 0;
    for (size_t Index = 0; Index < Strings->GetCount(); Index++)
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
    TFarMessageDialog(TCustomFarPlugin *Plugin,
                      TFarMessageParams *Params);

    void Init(unsigned int AFlags,
              const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons);
    int Execute(bool &ACheckBox);

protected:
    virtual void Change();
    virtual void Idle();

private:
    void ButtonClick(TFarButton *Sender, bool &Close);

private:
    bool FCheckBoxChecked;
    TFarMessageParams *FParams;
    nb::TDateTime FStartTime;
    nb::TDateTime FLastTimerTime;
    TFarButton *FTimeoutButton;
    std::wstring FTimeoutButtonCaption;
    TFarCheckBox *FCheckBox;
};
//---------------------------------------------------------------------------
TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin *Plugin,
                                     TFarMessageParams *Params) :
    TFarDialog(Plugin),
    FCheckBoxChecked(false),
    FParams(Params),
    FTimeoutButton(NULL),
    FCheckBox(NULL)
{
    assert(Params != NULL);
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Init(unsigned int AFlags,
                             const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons)
{
    assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
    assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
    // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
    assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));
    nb::TStrings *MessageLines = new nb::TStringList();
    nb::TStrings *MoreMessageLines = NULL;
    {
        BOOST_SCOPE_EXIT ( (&MessageLines) (&MoreMessageLines) )
        {
            delete MessageLines;
            delete MoreMessageLines;
        } BOOST_SCOPE_EXIT_END
        FarWrapText(Message, MessageLines, MaxMessageWidth);
        size_t MaxLen = GetFarPlugin()->MaxLength(MessageLines);
        // DEBUG_PRINTF(L"MaxLen = %d, FParams->MoreMessages = %x", MaxLen, FParams->MoreMessages);
        if (FParams->MoreMessages != NULL)
        {
            MoreMessageLines = new nb::TStringList();
            std::wstring MoreMessages = FParams->MoreMessages->GetText();
            while (MoreMessages[MoreMessages.size() - 1] == L'\n' ||
                    MoreMessages[MoreMessages.size() - 1] == L'\r')
            {
                MoreMessages.resize(MoreMessages.size() - 1);
            }
            FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
            size_t MoreMaxLen = GetFarPlugin()->MaxLength(MoreMessageLines);
            if (MaxLen < MoreMaxLen)
            {
                MaxLen = MoreMaxLen;
            }
        }

        // temporary
        // DEBUG_PRINTF(L"MaxMessageWidth = %d, Title = %s", MaxMessageWidth, Title.c_str());
        SetSize(nb::TPoint(MaxMessageWidth, 10));
        SetCaption(Title);
        SetFlags(GetFlags() |
                 FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING));

        for (size_t Index = 0; Index < MessageLines->GetCount(); Index++)
        {
            TFarText *Text = new TFarText(this);
            Text->SetCaption(MessageLines->GetString(Index));
        }

        TFarLister *MoreMessagesLister = NULL;
        TFarSeparator *MoreMessagesSeparator = NULL;

        if (FParams->MoreMessages != NULL)
        {
            new TFarSeparator(this);

            MoreMessagesLister = new TFarLister(this);
            MoreMessagesLister->GetItems()->Assign(MoreMessageLines);
            MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

            MoreMessagesSeparator = new TFarSeparator(this);
        }

        int ButtonOffset = (FParams->CheckBoxLabel.empty() ? -1 : -2);
        int ButtonLines = 1;
        TFarButton *Button = NULL;
        FTimeoutButton = NULL;
        for (size_t Index = 0; Index < Buttons->GetCount(); Index++)
        {
            TFarButton *PrevButton = Button;
            Button = new TFarButton(this);
            Button->SetDefault(Index == 0);
            Button->SetBrackets(brNone);
            Button->SetOnClick(boost::bind(&TFarMessageDialog::ButtonClick, this, _1, _2));
            std::wstring Caption = Buttons->GetString(Index);
            if ((FParams->Timeout > 0) &&
                    (FParams->TimeoutButton == Index))
            {
                FTimeoutButtonCaption = Caption;
                Caption = FORMAT(FParams->TimeoutStr.c_str(), Caption.c_str(), static_cast<int>(FParams->Timeout / 1000));
                FTimeoutButton = Button;
            }
            Button->SetCaption(FORMAT(L" %s ", Caption.c_str()));
            Button->SetTop(GetBorderBox()->GetBottom() + ButtonOffset);
            Button->SetBottom(Button->GetTop());
            Button->SetResult(Index + 1);
            Button->SetCenterGroup(true);
            Button->SetTag(reinterpret_cast<size_t>(Buttons->GetObject(Index)));
            if (PrevButton != NULL)
            {
                Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
            }

            if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
            {
                for (size_t PIndex = 0; PIndex < GetItemCount(); PIndex++)
                {
                    TFarButton *PrevButton = dynamic_cast<TFarButton *>(GetItem(PIndex));
                    if ((PrevButton != NULL) && (PrevButton != Button))
                    {
                        PrevButton->Move(0, -1);
                    }
                }
                Button->Move(-(Button->GetLeft() - GetBorderBox()->GetLeft()), 0);
                ButtonLines++;
            }

            // DEBUG_PRINTF(L"Button->GetLeft = %d, Button->GetRight = %d, GetBorderBox()->GetLeft = %d", Button->GetLeft(), Button->GetRight(), GetBorderBox()->GetLeft());
            if (MaxLen < Button->GetRight() - GetBorderBox()->GetLeft())
            {
                MaxLen = Button->GetRight() - GetBorderBox()->GetLeft();
            }
            // DEBUG_PRINTF(L"MaxLen = %d", MaxLen);

            SetNextItemPosition(ipRight);
        }

        // DEBUG_PRINTF(L"FParams->CheckBoxLabel = %s", FParams->CheckBoxLabel.c_str());
        if (!FParams->CheckBoxLabel.empty())
        {
            SetNextItemPosition(ipNewLine);
            FCheckBox = new TFarCheckBox(this);
            FCheckBox->SetCaption(FParams->CheckBoxLabel);

            if (MaxLen < FCheckBox->GetRight() - GetBorderBox()->GetLeft())
            {
                MaxLen = FCheckBox->GetRight() - GetBorderBox()->GetLeft();
            }
        }
        else
        {
            FCheckBox = NULL;
        }

        nb::TRect rect = GetClientRect();
        // DEBUG_PRINTF(L"rect.Left = %d, MaxLen = %d, rect.Right = %d", rect.Left, MaxLen, rect.Right);
        nb::TPoint S(
            // rect.Left + MaxLen + (-(rect.Right + 1)),
            rect.Left + MaxLen - rect.Right,
            rect.Top + MessageLines->GetCount() +
            (FParams->MoreMessages != NULL ? 1 : 0) + ButtonLines +
            (!FParams->CheckBoxLabel.empty() ? 1 : 0) +
            (-(rect.Bottom + 1)));

        if (FParams->MoreMessages != NULL)
        {
            size_t MoreMessageHeight = GetFarPlugin()->TerminalInfo().y - S.y - 1;
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
            S.y += static_cast<int>(MoreMessagesLister->GetHeight()) + 1;
        }
        // DEBUG_PRINTF(L"S.x = %d, S.y = %d", S.x, S.y);
        SetSize(S);
    }
}

//---------------------------------------------------------------------------
void TFarMessageDialog::Idle()
{
    TFarDialog::Idle();

    if (FParams->Timer > 0)
    {
        size_t SinceLastTimer = static_cast<size_t>((static_cast<double>(nb::Now()) - static_cast<double>(FLastTimerTime)) * 24*60*60*1000);
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
                FLastTimerTime = nb::Now();
            }
        }
    }

    if (FParams->Timeout > 0)
    {
        size_t Running = static_cast<size_t>((static_cast<double>(nb::Now()) - static_cast<double>(FStartTime)) * 24*60*60*1000);
        if (Running >= FParams->Timeout)
        {
            assert(FTimeoutButton != NULL);
            Close(FTimeoutButton);
        }
        else
        {
            std::wstring Caption =
                FORMAT(L" %s ", FORMAT(FParams->TimeoutStr.c_str(),
                                       FTimeoutButtonCaption.c_str(), static_cast<int>((FParams->Timeout - Running) / 1000)).c_str()).c_str();
            size_t sz = FTimeoutButton->GetCaption().size() > Caption.size() ? FTimeoutButton->GetCaption().size() - Caption.size() : 0;
            Caption += ::StringOfChar(L' ', sz);
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
        if ((FCheckBox != NULL) && (FCheckBoxChecked != FCheckBox->GetChecked()))
        {
            for (size_t Index = 0; Index < GetItemCount(); Index++)
            {
                TFarButton *Button = dynamic_cast<TFarButton *>(GetItem(Index));
                if ((Button != NULL) && (Button->GetTag() == 0))
                {
                    Button->SetEnabled(!FCheckBox->GetChecked());
                }
            }
            FCheckBoxChecked = FCheckBox->GetChecked();
        }
    }
}
//---------------------------------------------------------------------------
int TFarMessageDialog::Execute(bool &ACheckBox)
{
    FStartTime = nb::Now();
    FLastTimerTime = FStartTime;
    FCheckBoxChecked = !ACheckBox;
    if (FCheckBox != NULL)
    {
        FCheckBox->SetChecked(ACheckBox);
    }

    int Result = ShowModal();
    assert(Result != 0);
    if (Result > 0)
    {
        if (FCheckBox != NULL)
        {
            ACheckBox = FCheckBox->GetChecked();
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
                                    const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons,
                                    TFarMessageParams *Params)
{
    int Result;
    TFarMessageDialog *Dialog =
        new TFarMessageDialog(this, Params);
    {
        BOOST_SCOPE_EXIT ( (&Dialog) )
        {
            delete Dialog;
        } BOOST_SCOPE_EXIT_END
        Dialog->Init(Flags, Title, Message, Buttons);
        Result = Dialog->Execute(Params->CheckBox);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarMessage(unsigned int Flags,
                                 const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons,
                                 TFarMessageParams *Params)
{
    assert(Params != NULL);

    int Result;
    nb::TStringList *MessageLines = NULL;
    wchar_t **Items = NULL;
    {
        BOOST_SCOPE_EXIT ( (&MessageLines) (&Items) )
        {
            delete MessageLines;
            delete[] Items;
        } BOOST_SCOPE_EXIT_END
        std::wstring FullMessage = Message;
        if (Params->MoreMessages != NULL)
        {
            FullMessage += std::wstring(L"\n\x01\n") + Params->MoreMessages->GetText();
            while (FullMessage[FullMessage.size() - 1] == L'\n' ||
                    FullMessage[FullMessage.size() - 1] == L'\r')
            {
                FullMessage.resize(FullMessage.size() - 1);
            }
            FullMessage += L"\n\x01\n";
        }

        MessageLines = new nb::TStringList();
        MessageLines->Add(Title);
        FarWrapText(FullMessage, MessageLines, MaxMessageWidth);

        // FAR WORKAROUND
        // When there is too many lines to fit on screen, far uses not-shown
        // lines as button captions instead of real captions at the end of the list
        size_t MaxLines = MaxMessageLines();
        while (MessageLines->GetCount() > MaxLines)
        {
            MessageLines->Delete(MessageLines->GetCount() - 1);
        }

        for (size_t Index = 0; Index < Buttons->GetCount(); Index++)
        {
            MessageLines->Add(Buttons->GetString(Index));
        }

        Items = new wchar_t *[MessageLines->GetCount()];
        for (size_t Index = 0; Index < MessageLines->GetCount(); Index++)
        {
            std::wstring S = MessageLines->GetString(Index);
            MessageLines->PutString(Index, std::wstring(StrToFar(S)));
            Items[Index] = const_cast<wchar_t *>(MessageLines->GetString(Index).c_str());
        }

        TFarEnvGuard Guard;
        Result = FStartupInfo.Message(&MainGuid, &MainGuid,
                                      Flags | FMSG_LEFTALIGN, NULL, Items, static_cast<int>(MessageLines->GetCount()),
                                      static_cast<int>(Buttons->GetCount()));
    }

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Message(unsigned int Flags,
                              const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons,
                              TFarMessageParams *Params, bool Oem)
{
    // DEBUG_PRINTF(L"Message = %s", Message.c_str());
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
        Result = FStartupInfo.Message(&MainGuid, &MainGuid,
                                      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
                                      NULL,
                                      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const std::wstring Title,
                           const std::wstring Bottom, const FarMenuItem *Items, int Count,
                           const FarKey *BreakKeys, int &BreakCode)
{
    assert(Items);

    std::wstring ATitle = Title;
    std::wstring ABottom = Bottom;
    TFarEnvGuard Guard;
    return FStartupInfo.Menu(&MainGuid, &MainGuid,
        -1, -1, 0,
        Flags,
        StrToFar(ATitle),
        StrToFar(ABottom),
        NULL,
        BreakKeys,
        &BreakCode,
        Items,
        Count);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const std::wstring Title,
                           const std::wstring Bottom, nb::TStrings *Items, const FarKey *BreakKeys,
                           int &BreakCode)
{
    assert(Items && Items->GetCount());
    size_t Result = 0;
    FarMenuItem *MenuItems = new FarMenuItem[Items->GetCount()];
    {
        BOOST_SCOPE_EXIT ( (&MenuItems) )
        {
            delete[] MenuItems;
        } BOOST_SCOPE_EXIT_END
        size_t Selected = -1;
        int Count = 0;
        for (size_t i = 0; i < Items->GetCount(); i++)
        {
            size_t flags = reinterpret_cast<size_t>(Items->GetObject(i));
            if (FLAGCLEAR(Flags, MIF_HIDDEN))
            {
                memset(&MenuItems[Count], 0, sizeof(MenuItems[Count]));
                std::wstring Text = Items->GetString(i).c_str();
                MenuItems[Count].Flags = flags;
                if (MenuItems[Count].Flags & MIF_SELECTED)
                {
                    assert(Selected == -1);
                    Selected = i;
                }
                std::wstring Str = StrToFar(Text);
                MenuItems[Count].Text = TCustomFarPlugin::DuplicateStr(Str);
                MenuItems[Count].UserData = i;
                Count++;
            }
        }

        int ResultItem = Menu(Flags, Title, Bottom,
                              reinterpret_cast<const FarMenuItem *>(MenuItems), Count, BreakKeys, BreakCode);

        if (ResultItem >= 0)
        {
            Result = MenuItems[ResultItem].UserData;
            if (Selected != -1)
            {
                Items->PutObject(Selected, (nb::TObject *)(reinterpret_cast<size_t>(Items->GetObject(Selected)) & ~MIF_SELECTED));
            }
            Items->PutObject(Result, (nb::TObject *)(reinterpret_cast<size_t>(Items->GetObject(Result)) | MIF_SELECTED));
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
                           const std::wstring Bottom, nb::TStrings *Items)
{
    int BreakCode;
    return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::InputBox(const std::wstring Title,
                                const std::wstring Prompt, std::wstring &Text, unsigned long Flags,
                                const std::wstring HistoryName, size_t MaxLen, farinputboxvalidate_slot_type *OnValidate)
{
    bool Repeat = false;
    int Result = 0;
    farinputboxvalidate_signal_type sig;
    if (OnValidate)
    {
        sig.connect(*OnValidate);
    }
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
                &MainGuid,
                &MainGuid,
                         StrToFar(Title.c_str()),
                         StrToFar(Prompt.c_str()),
                         StrToFar(HistoryName.c_str()),
                         StrToFar(AText.c_str()),
                         const_cast<wchar_t *>(DestText.c_str()),
                         MaxLen,
                         NULL,
                         FIB_ENABLEEMPTY | FIB_BUTTONS | Flags);
        }
        RestoreScreen(ScreenHandle);
        Repeat = false;
        if (Result)
        {
            Text = StrFromFar(DestText.c_str());
            if (OnValidate)
            {
                try
                {
                    sig(Text);
                }
                catch (const std::exception &E)
                {
                    DEBUG_PRINTF(L"before HandleException");
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
void TCustomFarPlugin::Text(int X, int Y, int Color, const std::wstring Str)
{
    TFarEnvGuard Guard;
    FarColor color = {};
    color.Flags = FCF_FG_4BIT | FCF_BG_4BIT;
    color.ForegroundColor = Color; // LIGHTGRAY;
    color.BackgroundColor = 0;
    FStartupInfo.Text(X, Y, &color, StrToFar(Str.c_str()));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FlushText()
{
    TFarEnvGuard Guard;
    FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::WriteConsole(const std::wstring Str)
{
    unsigned long Written;
    ::WriteConsole(FConsoleOutput, StrToFar(Str.c_str()), static_cast<DWORD>(Str.size()), &Written, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(const std::wstring Str)
{
    TFarEnvGuard Guard;
    FFarStandardFunctions.CopyToClipboard(StrToFar(Str.c_str()));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(nb::TStrings *Strings)
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
nb::TPoint TCustomFarPlugin::TerminalInfo(nb::TPoint *Size, nb::TPoint *Cursor)
{
    // DEBUG_PRINTF(L"begin");
    CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
    GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);

    nb::TPoint Result(BufferInfo.dwSize.X, BufferInfo.dwSize.Y);

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
    wchar_t Title[1024];
    GetConsoleTitle(Title, sizeof(Title) - 1);
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
        Win32Check(GetWindowPlacement(Window, &WindowPlacement) > 0);
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
                COORD Size = { static_cast<short>(FNormalConsoleSize.x), static_cast<short>(FNormalConsoleSize.y) };

                Win32Check(ShowWindow(Window, SW_RESTORE) > 0);

                SMALL_RECT WindowSize;
                WindowSize.Left = 0;
                WindowSize.Top = 0;
                WindowSize.Right = static_cast<short>(Size.X - 1);
                WindowSize.Bottom = static_cast<short>(Size.Y - 1);
                Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);

                Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);
            }
        }
        else
        {
            COORD Size = GetLargestConsoleWindowSize(FConsoleOutput);
            Win32Check((Size.X != 0) || (Size.Y != 0));

            FNormalConsoleSize = TerminalInfo();

            Win32Check(ShowWindow(Window, SW_MAXIMIZE) > 0);

            Win32Check(SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);

            CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
            Win32Check(GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo) > 0);

            SMALL_RECT WindowSize;
            WindowSize.Left = 0;
            WindowSize.Top = 0;
            WindowSize.Right = static_cast<short>(BufferInfo.dwMaximumWindowSize.X - 1);
            WindowSize.Bottom = static_cast<short>(BufferInfo.dwMaximumWindowSize.Y - 1);
            Win32Check(SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);
        }
    }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ScrollTerminalScreen(int Rows)
{
    nb::TPoint Size = TerminalInfo();

    SMALL_RECT Source;
    COORD Dest;
    CHAR_INFO Fill;
    Source.Left = 0;
    Source.Top = static_cast<char>(Rows);
    Source.Right = static_cast<SHORT>(Size.x);
    Source.Bottom = static_cast<SHORT>(Size.y);
    Dest.X = 0;
    Dest.Y = 0;
    Fill.Char.AsciiChar = ' ';
    // Fill.�har.UnicodeChar = L' ';
    Fill.Attributes = 7;
    ScrollConsoleScreenBuffer(FConsoleOutput, &Source, NULL, Dest, &Fill);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowTerminalScreen()
{
    assert(!FTerminalScreenShowing);
    nb::TPoint Size, Cursor;
    TerminalInfo(&Size, &Cursor);

    std::wstring Blank = ::StringOfChar(' ', Size.x);
    Blank.resize(static_cast<size_t>(Size.x));
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
    wchar_t SaveTitle[1024];
    GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
    StrFromFar(SaveTitle);
    TConsoleTitleParam Param;
    Param.Progress = FCurrentProgress;
    Param.Own = !FCurrentTitle.empty() && (FormatConsoleTitle() == SaveTitle);
    assert(sizeof(Param) == sizeof(nb::TObject *));
    if (Param.Own)
    {
        FSavedTitles->AddObject(FCurrentTitle, *reinterpret_cast<nb::TObject **>(&Param));
    }
    else
    {
        FSavedTitles->AddObject(SaveTitle, *reinterpret_cast<nb::TObject **>(&Param));
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
    nb::TObject *Object = static_cast<nb::TObject *>(FSavedTitles->GetObject(FSavedTitles->GetCount()-1));
    TConsoleTitleParam Param = *reinterpret_cast<TConsoleTitleParam *>(&Object);
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
        SetConsoleTitle(StrToFar(Title));
    }
    FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle(const std::wstring Title)
{
    // assert(!FCurrentTitle.empty());
    FCurrentTitle = Title;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
    // assert(!FCurrentTitle.empty());
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
    SetConsoleTitle(StrToFar(Title));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveScreen(HANDLE &Screen)
{
    assert(!Screen);
    TFarEnvGuard Guard;
    Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
    assert(Screen);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::RestoreScreen(HANDLE &Screen)
{
    assert(Screen);
    TFarEnvGuard Guard;
    FStartupInfo.RestoreScreen(static_cast<HANDLE>(Screen));
    Screen = 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleException(const std::exception *E, int /*OpMode*/)
{
    assert(E);
    Message(FMSG_WARNING | FMSG_MB_OK, L"", nb::MB2W(E->what()));
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::GetMsg(int MsgId)
{
    TFarEnvGuard Guard;
    std::wstring Result = FStartupInfo.GetMsg(&MainGuid, MsgId);
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
bool TCustomFarPlugin::Viewer(const std::wstring FileName,
                              unsigned int Flags, std::wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Viewer(
                     StrToFar(FileName),
                     StrToFar(Title), 0, 0, -1, -1, Flags,
                     65001);
    return Result > 0;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Editor(const std::wstring FileName,
                              unsigned int Flags, std::wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Editor(
                     StrToFar(FileName),
                     StrToFar(Title), 0, 0, -1, -1, Flags, -1, -1,
                     65001);
    return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ResetCachedInfo()
{
    FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
INT_PTR TCustomFarPlugin::FarSystemSettings()
{
    if (!FValidFarSystemSettings)
    {
        FFarSystemSettings = FarAdvControl(ACTL_GETSYSTEMSETTINGS, 0);
        FValidFarSystemSettings = true;
    }
    return FFarSystemSettings;
}
//---------------------------------------------------------------------------
DWORD TCustomFarPlugin::FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void *Param2, HANDLE Plugin)
{
    switch (Command)
    {
    case FCTL_CLOSEPANEL:
    case FCTL_SETPANELDIRECTORY:
    case FCTL_SETCMDLINE:
    case FCTL_INSERTCMDLINE:
        break;

    case FCTL_GETCMDLINE:
    // case FCTL_GETCMDLINESELECTEDTEXT:
        // ANSI/OEM translation not implemented yet
        assert(false);
        break;
    }

    TFarEnvGuard Guard;
    return FStartupInfo.PanelControl(Plugin, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, int Param1, void *Param2)
{
    TFarEnvGuard Guard;
    return static_cast<int>(FStartupInfo.AdvControl(&MainGuid, Command, Param1, Param2));
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarEditorControl(EDITOR_CONTROL_COMMANDS Command, void *Param)
{
    std::wstring Buf;
    switch (Command)
    {
    case ECTL_GETINFO:
    case ECTL_SETPARAM:
    case ECTL_GETFILENAME:
        // noop
        break;

    case ECTL_SETTITLE:
        Buf = static_cast<wchar_t *>(Param);
        Param = StrToFar(Buf);
        break;

    default:
        // for other commands, OEM/ANSI conversion to be verified
        assert(false);
        break;
    }

    TFarEnvGuard Guard;
    return FStartupInfo.EditorControl(0, Command, 0, Param);
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
        FFarVersion = FarAdvControl(ACTL_GETFARMANAGERVERSION, 0);
    }
    return FFarVersion;
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::FormatFarVersion(VersionInfo &Info)
{
    return FORMAT(L"%d.%d.%d", Info.Major, Info.Minor, Info.Build);
}
//---------------------------------------------------------------------------
std::wstring TCustomFarPlugin::TemporaryDir()
{
    std::wstring Result;
    Result.resize(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), Result.size(), NULL);
    PackStr(Result);
    StrFromFar(Result);
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD *Rec)
{
    int Result;
    /*
    if (FFarStandardFunctions.FarInputRecordToKey != NULL)
    {
        TFarEnvGuard Guard;
        Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
    }
    else
    */
    {
        Result = 0;
    }
    return Result;
}
#ifdef NETBOX_DEBUG
void TCustomFarPlugin::RunTests()
{
    DEBUG_PRINTF(L"begin");
    {
        TFileMasks m(L"*.txt;*.log");
        bool res = m.Matches(L"test.exe");
        DEBUG_PRINTF(L"res = %d", res);
    }
    {
        random_ref();
        random_unref();
    }
    DEBUG_PRINTF(L"end");
}
#endif
//---------------------------------------------------------------------------
unsigned int TCustomFarFileSystem::FInstances = 0;
//---------------------------------------------------------------------------
TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin *APlugin) :
    nb::TObject(),
    FPlugin(APlugin)
{
};

void TCustomFarFileSystem::Init()
{
    FCriticalSection = new TCriticalSection;
    FPanelInfo[0] = NULL;
    FPanelInfo[1] = NULL;
    FClosed = false;

    memset(&FOpenPanelInfo, 0, sizeof(FOpenPanelInfo));
    ClearOpenPanelInfo(FOpenPanelInfo);
    FInstances++;
    // DEBUG_PRINTF(L"FInstances = %d", FInstances);
}

//---------------------------------------------------------------------------
TCustomFarFileSystem::~TCustomFarFileSystem()
{
    FInstances--;
    // DEBUG_PRINTF(L"FInstances = %d", FInstances);
    ResetCachedInfo();
    ClearOpenPanelInfo(FOpenPanelInfo);
    delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::HandleException(const std::exception *E, int OpMode)
{
    DEBUG_PRINTF(L"before FPlugin->HandleException");
    FPlugin->HandleException(E, OpMode);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::Close()
{
    FClosed = true;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::InvalidateOpenPanelInfo()
{
    FOpenPanelInfoValid = false;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClearOpenPanelInfo(OpenPanelInfo &Info)
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
        for (size_t Index = 0; Index < Info.PanelModesNumber; Index++)
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
    InvalidateOpenPanelInfo();
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::GetOpenPanelInfo(struct OpenPanelInfo *Info)
{
    ResetCachedInfo();
    if (FClosed)
    {
        // FAR WORKAROUND
        // if plugin is closed from ProcessEvent(FE_IDLE), is does not close,
        // so we close it here on the very next opportunity
        ClosePanel();
    }
    else
    {
        if (!FOpenPanelInfoValid)
        {
            ClearOpenPanelInfo(FOpenPanelInfo);
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

                GetOpenPanelInfoEx(FOpenPanelInfo.Flags, HostFile, CurDir, Format,
                    PanelTitle, PanelModes, FOpenPanelInfo.StartPanelMode,
                    FOpenPanelInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);

                FOpenPanelInfo.HostFile = StrToFar(TCustomFarPlugin::DuplicateStr(HostFile));
                FOpenPanelInfo.CurDir = StrToFar(TCustomFarPlugin::DuplicateStr(CurDir));
                FOpenPanelInfo.Format = StrToFar(TCustomFarPlugin::DuplicateStr(Format));
                FOpenPanelInfo.PanelTitle = StrToFar(TCustomFarPlugin::DuplicateStr(PanelTitle));
                // FOpenPanelInfo.StartPanelMode=L'4';
                // PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
                // Info->StartSortMode = SM_NAME;
                // Info->StartSortOrder = 0;

                PanelModes->FillOpenPanelInfo(&FOpenPanelInfo);
                FOpenPanelInfo.StartSortOrder = StartSortOrder;
                KeyBarTitles->FillOpenPanelInfo(&FOpenPanelInfo);
                FOpenPanelInfo.ShortcutData = StrToFar(TCustomFarPlugin::DuplicateStr(ShortcutData));
            }

            FOpenPanelInfoValid = true;
        }

        memmove(Info, &FOpenPanelInfo, sizeof(FOpenPanelInfo));
    }
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFindData(struct GetFindDataInfo *Info)
{
    // DEBUG_PRINTF(L"begin");
    ResetCachedInfo();
    nb::TObjectList *PanelItems = new nb::TObjectList();
    bool Result = false;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = !FClosed && GetFindDataEx(PanelItems, Info->OpMode);
        // DEBUG_PRINTF(L"Result = %d, PanelItems->GetCount = %d", Result, PanelItems->GetCount());
        if (Result && PanelItems->GetCount())
        {
            Info->PanelItem = new PluginPanelItem[PanelItems->GetCount()];
            memset(Info->PanelItem, 0, PanelItems->GetCount() * sizeof(PluginPanelItem));
            Info->ItemsNumber = PanelItems->GetCount();
            for (size_t Index = 0; Index < PanelItems->GetCount(); Index++)
            {
                static_cast<TCustomFarPanelItem *>(PanelItems->GetItem(Index))->FillPanelItem(
                    &(Info->PanelItem[Index]));
            }
        }
        else
        {
            Info->PanelItem = NULL;
            Info->ItemsNumber = 0;
        }
    }
    // DEBUG_PRINTF(L"end: Result = %d", Result);
    return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::FreeFindData(const struct FreeFindDataInfo *Info)
{
    ResetCachedInfo();
    if (Info->PanelItem)
    {
        assert(Info->ItemsNumber > 0);
        for (size_t Index = 0; Index < Info->ItemsNumber; Index++)
        {
            delete[] Info->PanelItem[Index].FileName;
            delete[] Info->PanelItem[Index].Description;
            delete[] Info->PanelItem[Index].Owner;
            for (int CustomIndex = 0; CustomIndex < Info->PanelItem[Index].CustomColumnNumber; CustomIndex++)
            {
                delete[] Info->PanelItem[Index].CustomColumnData[CustomIndex];
            }
            delete[] Info->PanelItem[Index].CustomColumnData;
        }
        delete[] Info->PanelItem;
    }
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessHostFile(const struct ProcessHostFileInfo *Info)
{
    ResetCachedInfo();
    nb::TObjectList *PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
    bool Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = ProcessHostFileEx(PanelItems, Info->OpMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
{
    ResetCachedInfo();
    if (Info->Rec.EventType == KEY_EVENT)
    {
        const KEY_EVENT_RECORD &Event = Info->Rec.Event.KeyEvent;
        return ProcessKeyEx(Event.wVirtualKeyCode,
            Event.dwControlKeyState);
   }
   return FALSE;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::ProcessEvent(int Event, void *Param)
{
    ResetCachedInfo();
    return ProcessEventEx(Event, Param);
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::SetDirectory(const struct SetDirectoryInfo *Info)
{
    ResetCachedInfo();
    InvalidateOpenPanelInfo();
    int Result = SetDirectoryEx(Info->Dir, Info->OpMode);
    InvalidateOpenPanelInfo();
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectory(struct MakeDirectoryInfo *Info)
{
    ResetCachedInfo();
    std::wstring NameStr = Info->Name;
    int Result = 0;
    {
        /*
        BOOST_SCOPE_EXIT ( (&NameStr) (&Info) )
        {
            StrToFar(NameStr);
            if (0 != wcscmp(NameStr.c_str(), Info->Name))
            {
                // wcscpy_s(*Name, NameStr.size(), NameStr.c_str());
                *Info->Name = TCustomFarPlugin::DuplicateStr(NameStr, true);
            }
        } BOOST_SCOPE_EXIT_END
        StrFromFar(NameStr);
        */
        Result = MakeDirectoryEx(NameStr, Info->OpMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::DeleteFiles(const struct DeleteFilesInfo *Info)
{
    ResetCachedInfo();
    nb::TObjectList *PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
    bool Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = DeleteFilesEx(PanelItems, Info->OpMode);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFiles(struct GetFilesInfo *Info)
{
    ResetCachedInfo();
    nb::TObjectList *PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
    int Result;
    std::wstring DestPathStr = Info->DestPath;
    {
        BOOST_SCOPE_EXIT ( (&DestPathStr) (&Info) (&PanelItems) )
        {
            /*
            StrToFar(DestPathStr);
            if (DestPathStr != Info->DestPath)
            {
                // wcscpy_s(*DestPath, DestPathStr.size(), DestPathStr.c_str());
                *Info->DestPath = TCustomFarPlugin::DuplicateStr(DestPathStr, true);
            }
            */
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        StrFromFar(DestPathStr);
        Result = GetFilesEx(PanelItems, Info->Move > 0, DestPathStr, Info->OpMode);
    }

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFiles(const struct PutFilesInfo *Info)
{
    ResetCachedInfo();
    nb::TObjectList *PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
    int Result;
    {
        BOOST_SCOPE_EXIT ( (&PanelItems) )
        {
            delete PanelItems;
        } BOOST_SCOPE_EXIT_END
        Result = PutFilesEx(PanelItems, Info->Move > 0, Info->OpMode);
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
    // DEBUG_PRINTF(L"Another = %d", Another);
    bool another = Another != 0;
    if (FPanelInfo[another] == NULL)
    {
        PanelInfo *Info = new PanelInfo;
        Info->StructSize = sizeof(PanelInfo);
        bool res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(Info),
                                        !another ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
        if (!res)
        {
            memset(Info, 0, sizeof(*Info));
            assert(false);
        }
        // DEBUG_PRINTF(L"Info = %x", Info);
        FPanelInfo[another] = new TFarPanelInfo(Info, !another ? this : NULL);
    }
    return FPanelInfo[another];
}
//---------------------------------------------------------------------------
DWORD TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void *Param2)
{
    return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
DWORD TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, int Param1, void *Param2, HANDLE Plugin)
{
    return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
    unsigned int PrevInstances = FInstances;
    InvalidateOpenPanelInfo();
    FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
    return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::RedrawPanel(bool Another)
{
    FPlugin->FarControl(FCTL_REDRAWPANEL, 0, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClosePanel()
{
    FClosed = true;
    FarControl(FCTL_CLOSEPANEL, 0, NULL);
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
    DEBUG_PRINTF(L"IsLeft");
    return (GetPanelInfo(0)->GetBounds().Left <= 0);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsRight()
{
    return !IsLeft();
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessHostFileEx(nb::TObjectList * /* PanelItems */, int /* OpMode */)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessKeyEx(WORD /*Key*/, DWORD /*ControlState*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessEventEx(int /*Event*/, void * /*Param*/)
{
    return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::SetDirectoryEx(const std::wstring /* Dir */, int /* OpMode */)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectoryEx(std::wstring & /* Name */, int /* OpMode */)
{
    return -1;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::DeleteFilesEx(nb::TObjectList * /*PanelItems */, int /* OpMode */)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFilesEx(nb::TObjectList * /*PanelItems*/, bool /*Move*/,
                                     std::wstring & /*DestPath*/, int /*OpMode*/)
{
    return 0;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFilesEx(nb::TObjectList * /* PanelItems */, bool /* Move */, int /* OpMode */)
{
    return 0;
}
//---------------------------------------------------------------------------
nb::TObjectList *TCustomFarFileSystem::CreatePanelItemList(
    struct PluginPanelItem *PanelItem, size_t ItemsNumber)
{
    // DEBUG_PRINTF(L"ItemsNumber = %d", ItemsNumber);
    nb::TObjectList *PanelItems = new nb::TObjectList();
    PanelItems->SetOwnsObjects(false);
    try
    {
        for (size_t Index = 0; Index < ItemsNumber; Index++)
        {
            PanelItems->Add(new TFarPanelItem(&PanelItem[Index]));
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
TFarPanelModes::TFarPanelModes() : nb::TObject()
{
    memset(&FPanelModes, 0, sizeof(FPanelModes));
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
void TFarPanelModes::SetPanelMode(size_t Mode, const std::wstring ColumnTypes,
                                  const std::wstring ColumnWidths, nb::TStrings *ColumnTitles,
                                  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
                                  bool CaseConversion, const std::wstring StatusColumnTypes,
                                  const std::wstring StatusColumnWidths)
{
    size_t ColumnTypesCount = !ColumnTypes.empty() ? CommaCount(ColumnTypes) + 1 : 0;
    assert(Mode != -1 && Mode < LENOF(FPanelModes));
    assert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

    ClearPanelMode(FPanelModes[Mode]);
    static wchar_t *Titles[PANEL_MODES_COUNT];
    FPanelModes[Mode].ColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnTypes));
    FPanelModes[Mode].ColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnWidths));
    if (ColumnTitles)
    {
        FPanelModes[Mode].ColumnTitles = new wchar_t *[ColumnTypesCount];
        for (size_t Index = 0; Index < ColumnTypesCount; Index++)
        {
            // FPanelModes[Mode].ColumnTitles[Index] = StrToFar(
            // TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index)));
            Titles[Index] = StrToFar(
                                TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index)));
        }
        FPanelModes[Mode].ColumnTitles = Titles;
    }
    else
    {
        FPanelModes[Mode].ColumnTitles = NULL;
    }
    SetFlag(FPanelModes[Mode].Flags, FullScreen, PMFLAGS_FULLSCREEN);
    SetFlag(FPanelModes[Mode].Flags, DetailedStatus, PMFLAGS_DETAILEDSTATUS);
    SetFlag(FPanelModes[Mode].Flags, AlignExtensions, PMFLAGS_ALIGNEXTENSIONS);
    SetFlag(FPanelModes[Mode].Flags, CaseConversion, PMFLAGS_CASECONVERSION);

    FPanelModes[Mode].StatusColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnTypes));
    FPanelModes[Mode].StatusColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnWidths));
}
//---------------------------------------------------------------------------
void TFarPanelModes::SetFlag(PANELMODE_FLAGS &Flags, bool value, PANELMODE_FLAGS Flag)
{
    if (value)
    {
        Flags |= Flag;
    }
    else
    {
        Flags &= ~Flag;
    }
}
//---------------------------------------------------------------------------
void TFarPanelModes::ClearPanelMode(PanelMode &Mode)
{
    if (Mode.ColumnTypes)
    {
        size_t ColumnTypesCount = Mode.ColumnTypes ?
                               CommaCount(std::wstring(Mode.ColumnTypes)) + 1 : 0;

        delete[] Mode.ColumnTypes;
        delete[] Mode.ColumnWidths;
        if (Mode.ColumnTitles)
        {
            for (size_t Index = 0; Index < ColumnTypesCount; Index++)
            {
                // delete[] Mode.ColumnTitles[Index];
            }
            // delete[] Mode.ColumnTitles;
        }
        delete[] Mode.StatusColumnTypes;
        delete[] Mode.StatusColumnWidths;
        memset(&Mode, 0, sizeof(Mode));
    }
}
//---------------------------------------------------------------------------
void TFarPanelModes::FillOpenPanelInfo(struct OpenPanelInfo *Info)
{
    assert(Info);
    Info->PanelModesNumber = LENOF(FPanelModes);
    PanelMode *PanelModesArray = new PanelMode[LENOF(FPanelModes)];
    Info->PanelModesArray = PanelModesArray;
    memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
    FReferenced = true;
}
//---------------------------------------------------------------------------
size_t TFarPanelModes::CommaCount(const std::wstring ColumnTypes)
{
    size_t Count = 0;
    for (size_t Index = 0; Index < ColumnTypes.size(); Index++)
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
    FKeyBarTitles.CountLabels = 7 * 12;
    FKeyBarTitles.Labels = new KeyBarLabel[7 * 12];
    memset(FKeyBarTitles.Labels, 0, 7 * 12 * sizeof(KeyBarLabel));
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
    // ClearKeyBarTitle(fsAltShift, 3, 4);
    // ClearKeyBarTitle(fsCtrlAlt, 3, 4);
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
    assert(FunctionKey >= 1 && FunctionKey <= 12);
    int shift = static_cast<int>(ShiftStatus);
    assert(shift >= 0 && shift < 7);
    KeyBarLabel *Labels = &FKeyBarTitles.Labels[shift * 12];
    if (Labels[FunctionKey-1].Key.VirtualKeyCode)
    {
        delete[] Labels[FunctionKey-1].Text;
        delete[] Labels[FunctionKey-1].LongText;
    }
    static WORD FKeys[] =
    {
        0, // fsNone,
        LEFT_CTRL_PRESSED, // fsCtrl
        LEFT_ALT_PRESSED, // fsAlt
        SHIFT_PRESSED, // fsShift,
        LEFT_CTRL_PRESSED | SHIFT_PRESSED, // fsCtrlShift,
        LEFT_ALT_PRESSED | SHIFT_PRESSED, // fsAltShift,
        LEFT_CTRL_PRESSED | LEFT_ALT_PRESSED, // fsCtrlAlt
    };
    Labels[FunctionKey - 1].Key.VirtualKeyCode = VK_F1 + FunctionKey - 1;
    Labels[FunctionKey - 1].Key.ControlKeyState = FKeys[shift];
    Labels[FunctionKey - 1].Text = StrToFar(TCustomFarPlugin::DuplicateStr(Title, true));
    Labels[FunctionKey-1].LongText = NULL;
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles &Titles)
{
    for (size_t Index = 0; Index < Titles.CountLabels; Index++)
    {
        delete[] Titles.Labels[Index].Text;
        delete[] Titles.Labels[Index].LongText;
    }
    delete[] Titles.Labels;
    Titles.Labels = NULL;
    Titles.CountLabels = 0;
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::FillOpenPanelInfo(struct OpenPanelInfo *Info)
{
    assert(Info);
    KeyBarTitles *KeyBar = new KeyBarTitles;
    Info->KeyBar = KeyBar;
    memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
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
    // DEBUG_PRINTF(L"begin");
    assert(PanelItem);

    std::wstring FileName;
    __int64 Size = 0;
    nb::TDateTime LastWriteTime;
    nb::TDateTime LastAccess;
    std::wstring Description;
    std::wstring Owner;

    void *UserData = reinterpret_cast<void *>(PanelItem->UserData);
    GetData(PanelItem->Flags, FileName, Size, PanelItem->FileAttributes,
            LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
            static_cast<void *>(UserData), PanelItem->CustomColumnNumber);
    PanelItem->UserData = reinterpret_cast<DWORD_PTR>(UserData);
    // DEBUG_PRINTF(L"LastWriteTime = %f, LastAccess = %f", LastWriteTime, LastAccess);
    FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
    FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
    PanelItem->CreationTime = FileTime;
    PanelItem->LastAccessTime = FileTimeA;
    PanelItem->LastWriteTime = FileTime;
    PanelItem->FileSize = Size;
    // PanelItem->PackSize = (long int)Size;

    // ASCOPY(PanelItem->FindData.lpwszFileName, FileName);
    // StrToFar(PanelItem->FindData.lpwszFileName);
    PanelItem->FileName = StrToFar(TCustomFarPlugin::DuplicateStr(FileName));
    // DEBUG_PRINTF(L"PanelItem->FindData.lpwszFileName = %s", PanelItem->FindData.lpwszFileName);
    PanelItem->Description = StrToFar(TCustomFarPlugin::DuplicateStr(Description));
    PanelItem->Owner = StrToFar(TCustomFarPlugin::DuplicateStr(Owner));
    // PanelItem->CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
    wchar_t **CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
    for (size_t Index = 0; Index < PanelItem->CustomColumnNumber; Index++)
    {
        CustomColumnData[Index] =
            StrToFar(TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index)));
    }
    PanelItem->CustomColumnData = CustomColumnData;
    // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelItem::TFarPanelItem(PluginPanelItem *APanelItem):
    TCustomFarPanelItem()
{
    assert(APanelItem);
    FPanelItem = APanelItem;
}
TFarPanelItem::~TFarPanelItem()
{
    delete FPanelItem;
    FPanelItem = NULL;
}

//---------------------------------------------------------------------------
void TFarPanelItem::GetData(
    unsigned __int64 & /*Flags*/, std::wstring & /*FileName*/, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    nb::TDateTime & /*LastWriteTime*/, nb::TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
    std::wstring & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
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
    std::wstring Result = FPanelItem->FileName;
    return StrFromFar(Result);
}
//---------------------------------------------------------------------------
void *TFarPanelItem::GetUserData()
{
    return reinterpret_cast<void *>(FPanelItem->UserData);
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
    return FPanelItem->FileAttributes;
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsParentDirectory()
{
    return (GetFileName() == PARENTDIRECTORY);
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
    unsigned __int64 & /*Flags*/, std::wstring &FileName, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    nb::TDateTime & /*LastWriteTime*/, nb::TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, std::wstring & /*Description*/,
    std::wstring & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
    FileName = FHint;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelInfo::TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner):
    nb::TObject(),
    FPanelInfo(NULL),
    FItems(NULL),
    FOwner(NULL)
{
    // if (!APanelInfo) throw ExtException(L"");
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
size_t TFarPanelInfo::GetItemCount()
{
    return FPanelInfo->ItemsNumber;
}
//---------------------------------------------------------------------------
nb::TRect TFarPanelInfo::GetBounds()
{
    RECT rect = FPanelInfo->PanelRect;
    return nb::TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
size_t TFarPanelInfo::GetSelectedCount()
{
    size_t Count = FPanelInfo->SelectedItemsNumber;

    if (Count == 1)
    {
        size_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, NULL);
        // DEBUG_PRINTF(L"size1 = %d, sizeof(PluginPanelItem) = %d", size, sizeof(PluginPanelItem));
        PluginPanelItem *ppi = (PluginPanelItem *)malloc(size);
        memset(ppi, 0, size);
        FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<void *>(ppi));
        if ((ppi->Flags & PPIF_SELECTED) == 0)
        {
            // DEBUG_PRINTF(L"ppi->Flags = %x", ppi->Flags);
            Count = 0;
        }
        free(ppi);
    }

    return Count;
}
//---------------------------------------------------------------------------
nb::TObjectList *TFarPanelInfo::GetItems()
{
    if (!FItems)
    {
        FItems = new nb::TObjectList();
    }
    // DEBUG_PRINTF(L"FPanelInfo->ItemsNumber = %d", FPanelInfo->ItemsNumber);
    for (size_t Index = 0; Index < FPanelInfo->ItemsNumber; Index++)
    {
        // DEBUG_PRINTF(L"Index = %d", Index);
        // TODO: move to common function
        size_t size = FOwner->FarControl(FCTL_GETPANELITEM, Index, NULL);
        PluginPanelItem *ppi = (PluginPanelItem *)malloc(size);
        memset(ppi, 0, size);
        FarGetPluginPanelItem gppi;
        gppi.Item = ppi;
        gppi.Size = size;
        FOwner->FarControl(FCTL_GETPANELITEM, Index, static_cast<void *>(&gppi));
        // DEBUG_PRINTF(L"ppi.FileName = %s", ppi->FileName);
        FItems->Add(static_cast<nb::TObject *>(new TFarPanelItem(ppi)));
    }
    return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::FindFileName(const std::wstring FileName)
{
    nb::TObjectList *AItems = GetItems();
    TFarPanelItem *PanelItem;
    for (size_t Index = 0; Index < AItems->GetCount(); Index++)
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
    nb::TObjectList *AItems = GetItems();
    TFarPanelItem *PanelItem;
    for (size_t Index = 0; Index < AItems->GetCount(); Index++)
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
    FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<void *>(FPanelInfo));
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::GetFocusedItem()
{
    size_t Index = GetFocusedIndex();
    nb::TObjectList *Items = GetItems();
    // DEBUG_PRINTF(L"Index = %d, Items = %x, Items->GetCount = %d", Index, Items, Items->GetCount());
    if (Items->GetCount() > 0)
    {
        assert(Index < Items->GetCount());
        return static_cast<TFarPanelItem *>(Items->GetItem(Index));
    }
    else
    {
        return NULL;
    }
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedItem(TFarPanelItem *value)
{
    nb::TObjectList *Items = GetItems();
    size_t Index = Items->IndexOf(static_cast<nb::TObject *>(value));
    assert(Index != -1);
    SetFocusedIndex(Index);
    // delete Items;
}
//---------------------------------------------------------------------------
size_t TFarPanelInfo::GetFocusedIndex()
{
    return static_cast<size_t>(FPanelInfo->CurrentItem);
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedIndex(size_t value)
{
    // for "another panel info", there's no owner
    assert(FOwner != NULL);
    // DEBUG_PRINTF(L"GetFocusedIndex = %d, value = %d", GetFocusedIndex(), value);
    if (GetFocusedIndex() != value)
    {
        assert(value != -1 && value < FPanelInfo->ItemsNumber);
        FPanelInfo->CurrentItem = value;
        PanelRedrawInfo PanelInfo;
        PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
        PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
        FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<void *>(&PanelInfo));
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
    return (FPanelInfo->PluginHandle != INVALID_HANDLE_VALUE);
}
//---------------------------------------------------------------------------
std::wstring TFarPanelInfo::GetCurrentDirectory()
{
    std::wstring Result;
    /*
    size_t Size = FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(0, NULL);
    if (Size)
    {
        Result.resize(Size);
        FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(Size,
            const_cast<wchar_t *>(Result.c_str()));
    }
    */
    // FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<LONG_PTR>(&Info), Another ? PANEL_PASSIVE : PANEL_ACTIVE);
    size_t Size = FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
                                        0,
                                        NULL,
                                        FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
    if (Size)
    {
        FarPanelDirectory* pfpd = static_cast<FarPanelDirectory *>(malloc(Size));

        FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
                              static_cast<int>(Size),
                              pfpd,
                              FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
        Result = pfpd->Name;
        free(pfpd);
    }
    return StrFromFar(Result.c_str());
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarMenuItems::TFarMenuItems() :
    nb::TStringList()
{
    FItemFocused = -1;
}
//---------------------------------------------------------------------------
void TFarMenuItems::Clear()
{
    FItemFocused = -1;
    nb::TStringList::Clear();
}
//---------------------------------------------------------------------------
void TFarMenuItems::Delete(size_t Index)
{
    if (Index == FItemFocused)
    {
        FItemFocused = -1;
    }
    nb::TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void TFarMenuItems::PutObject(size_t Index, nb::TObject *AObject)
{
    nb::TStringList::PutObject(Index, AObject);
    bool Focused = (reinterpret_cast<size_t>(AObject) & MIF_SEPARATOR) != 0;
    if ((Index == GetItemFocused()) && !Focused)
    {
        FItemFocused = -1;
    }
    if (Focused)
    {
        if (GetItemFocused() != -1)
        {
            SetFlag(GetItemFocused(), MIF_SELECTED, false);
        }
        FItemFocused = Index;
    }
}
//---------------------------------------------------------------------------
size_t TFarMenuItems::Add(const std::wstring Text, bool Visible)
{
    size_t Result = nb::TStringList::Add(Text);
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
void TFarMenuItems::SetItemFocused(size_t value)
{
    if (GetItemFocused() != value)
    {
        if (GetItemFocused() != -1)
        {
            SetFlag(GetItemFocused(), MIF_SELECTED, false);
        }
        FItemFocused = value;
        SetFlag(GetItemFocused(), MIF_SELECTED, true);
    }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetFlag(size_t Index, size_t Flag, bool Value)
{
    if (GetFlag(Index, Flag) != Value)
    {
        size_t F = reinterpret_cast<size_t>(GetObject(Index));
        if (Value)
        {
            F |= Flag;
        }
        else
        {
            F &= ~Flag;
        }
        PutObject(Index, reinterpret_cast<nb::TObject *>(F));
    }
}
//---------------------------------------------------------------------------
bool TFarMenuItems::GetFlag(size_t Index, size_t Flag)
{
    return (reinterpret_cast<size_t>(GetObject(Index)) & Flag) > 0;
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
    std::wstring Result = L"";
    size_t buffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, NULL);
    if (buffLen)
    {
        Result.resize(buffLen + 1, 0);
        FarPlugin->FarEditorControl(ECTL_GETFILENAME, &Result[0]);
    }
    return StrFromFar(Result);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
    assert(FarPlugin != NULL);
    /*
    assert(AreFileApisANSI());
    if (!FarPlugin->GetANSIApis())
    {
        SetFileApisToOEM();
    }
    */
}
//---------------------------------------------------------------------------
TFarEnvGuard::~TFarEnvGuard()
{
    assert(FarPlugin != NULL);
    /*
    if (!FarPlugin->GetANSIApis())
    {
        assert(!AreFileApisANSI());
        SetFileApisToANSI();
    }
    else
    {
        assert(AreFileApisANSI());
    }
    */
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPluginEnvGuard::TFarPluginEnvGuard()
{
    assert(FarPlugin != NULL);

    // keep the assertion, but be robust, in case we are called from incorrectly
    // programmed plugin (e.g. EMenu)
    /*
    FANSIApis = AreFileApisANSI() > 0;
    assert(FANSIApis == FarPlugin->GetANSIApis());

    if (!FANSIApis)
    {
        SetFileApisToANSI();
    }
    */
}
//---------------------------------------------------------------------------
TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
    assert(FarPlugin != NULL);
    /*
    assert(AreFileApisANSI());

    if (!FANSIApis)
    {
        SetFileApisToOEM();
    }
    */
}
//---------------------------------------------------------------------------
void FarWrapText(const std::wstring Text, nb::TStrings *Result, size_t MaxWidth)
{
    size_t TabSize = 8;
    nb::TStringList Lines;
    Lines.SetText(Text);
    nb::TStringList WrappedLines;
    for (size_t Index = 0; Index < Lines.GetCount(); Index++)
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
            for (size_t WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); WrappedIndex++)
            {
                std::wstring FullLine = WrappedLines.GetString(WrappedIndex);
                do
                {
                    // WrapText does not wrap when not possible, enforce it
                    // (it also does not wrap when the line is longer than maximum only
                    // because of trailing dot or similar)
                    std::wstring Line = FullLine.substr(0, MaxWidth);
                    FullLine.erase(0, MaxWidth);

                    size_t P;
                    while ((P = Line.find_first_of(L"\t")) != std::wstring::npos)
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
    // FIXME
    std::wstring Result = nb::MB2W(S);
    // OemToChar(Result.c_str(), Result.c_str());
    return Result;
}
