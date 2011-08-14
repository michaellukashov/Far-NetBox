//---------------------------------------------------------------------------
#include "stdafx.h"

#include "FarPlugin.h"
#include "FarDialog.h"
#include "Common.h"
// FAR WORKAROUND
//---------------------------------------------------------------------------
TCustomFarPlugin *FarPlugin = NULL;
#define FAR_TITLE_SUFFIX L" - Far"
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
TCustomFarPlugin::TCustomFarPlugin(HWND AHandle): TObject()
{
    FFarThread = GetCurrentThreadId();
    FCriticalSection = new TCriticalSection;
    FHandle = AHandle;
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
    assert(FOpenedPlugins->Count == 0);
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
    catch(exception &E)
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
        TStrings *DiskMenuStrings = NULL;
        TStrings *PluginMenuStrings = NULL;
        TStrings *PluginConfigStrings = NULL;
        TStrings *CommandPrefixes = NULL;
        try
        {
            DiskMenuStrings = new TStringList();
            PluginMenuStrings = new TStringList();
            PluginConfigStrings = new TStringList();
            CommandPrefixes = new TStringList();

            ClearPluginInfo(FPluginInfo);

            GetPluginInfoEx(FPluginInfo.Flags, DiskMenuStrings, PluginMenuStrings,
                            PluginConfigStrings, CommandPrefixes);

#define COMPOSESTRINGARRAY(NAME) \
        if (NAME->GetCount()) \
        { \
          wchar_t ** StringArray = new wchar_t *[NAME->GetCount()]; \
          FPluginInfo.NAME = StringArray; \
          FPluginInfo.NAME ## Number = NAME->GetCount(); \
          for (int Index = 0; Index < NAME->GetCount(); Index++) \
          { \
            StringArray[Index] = StrToFar(DuplicateStr(NAME->GetString(Index))); \
          } \
        }

            COMPOSESTRINGARRAY(DiskMenuStrings);
            COMPOSESTRINGARRAY(PluginMenuStrings);
            COMPOSESTRINGARRAY(PluginConfigStrings);

#undef COMPOSESTRINGARRAY(NAME)

            if (DiskMenuStrings->GetCount())
            {
                wchar_t *NumberArray = new wchar_t[DiskMenuStrings->GetCount()];
                FPluginInfo.DiskMenuStrings = &NumberArray;
                for (int Index = 0; Index < DiskMenuStrings->GetCount(); Index++)
                {
                    NumberArray[Index] = (int)DiskMenuStrings->GetObjects(Index);
                }
            }

            wstring CommandPrefix;
            for (int Index = 0; Index < CommandPrefixes->GetCount(); Index++)
            {
                CommandPrefix = CommandPrefix + (CommandPrefix.empty() ? L"" : L":") +
                                CommandPrefixes->GetString(Index);
            }
            FPluginInfo.CommandPrefix = StrToFar(DuplicateStr(CommandPrefix));
        }
        catch (...)
        {
        }
        delete DiskMenuStrings;
        delete PluginMenuStrings;
        delete PluginConfigStrings;
        delete CommandPrefixes;
        memcpy(Info, &FPluginInfo, sizeof(FPluginInfo));
    }
    catch(exception &E)
    {
        HandleException(&E);
    }
}
//---------------------------------------------------------------------------
wstring TCustomFarPlugin::GetModuleName()
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

        delete[] Info.DiskMenuNumbers;
        delete[] Info.CommandPrefix;
    }
    memset(&Info, 0, sizeof(Info));
    Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
wchar_t *TCustomFarPlugin::DuplicateStr(const wstring Str, bool AllowEmpty)
{
    if (Str.IsEmpty() && !AllowEmpty)
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
}
//---------------------------------------------------------------------------
TCustomFarFileSystem *TCustomFarPlugin::GetPanelFileSystem(bool Another,
        HANDLE Plugin)
{
    TCustomFarFileSystem *Result = NULL;
    PanelInfo Info;
    if (FarVersion() >= FAR170BETA5)
    {
        FarControl(Another ? FCTL_GETANOTHERPANELSHORTINFO : FCTL_GETPANELSHORTINFO, &Info, Plugin);
    }
    else
    {
        FarControl(Another ? FCTL_GETANOTHERPANELINFO : FCTL_GETPANELINFO, &Info, Plugin);
    }

    if (Info.Plugin)
    {
        TRect Bounds = Info.PanelRect;
        TCustomFarFileSystem *FileSystem;
        int Index = 0;
        while (!Result && Index < FOpenedPlugins->Count)
        {
            FileSystem = dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->Items[Index]);
            assert(FileSystem);
            if (FileSystem->PanelInfo->Bounds == Bounds)
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
    for (int Index = 0; Index < FOpenedPlugins->Count; Index++)
    {
        TCustomFarFileSystem *FileSystem =
            dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->Items[Index]);
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
    catch(exception &E)
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

        wstring Buf;
        if ((OpenFrom == OPEN_SHORTCUT) || (OpenFrom == OPEN_COMMANDLINE))
        {
            Buf = (char *)Item;
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
    catch(exception &E)
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
        try
        {
            {
                TGuard Guard(FileSystem->CriticalSection);
                FileSystem->Close();
            }
            delete FileSystem;
        }
        catch (...)
        {
        }
        FOpenedPlugins->Remove(FileSystem);
    }
    catch(exception &E)
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
    throw exception("");
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleFileSystemException(
    TCustomFarFileSystem *FileSystem, exception *E, int OpMode)
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
            TGuard Guard(FileSystem->CriticalSection);
            FileSystem->GetOpenPluginInfo(Info);
        }
    }
    catch(exception &E)
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
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->GetFindData(PanelItem, ItemsNumber, OpMode);
        }
    }
    catch(exception &E)
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
            TGuard Guard(FileSystem->CriticalSection);
            FileSystem->FreeFindData(PanelItem, ItemsNumber);
        }
    }
    catch(exception &E)
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
                TGuard Guard(FileSystem->CriticalSection);
                return FileSystem->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
            }
        }
        else
        {
            return 0;
        }
    }
    catch(exception &E)
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
                TGuard Guard(FileSystem->CriticalSection);
                return FileSystem->ProcessKey(Key, ControlState);
            }
        }
        else
        {
            return 0;
        }
    }
    catch(exception &E)
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

            wstring Buf;
            if ((Event == FE_CHANGEVIEWMODE) || (Event == FE_COMMAND))
            {
                Buf = (char *)Param;
                StrFromFar(Buf);
                Param = Buf.c_str();
            }

            {
                TGuard Guard(FileSystem->CriticalSection);
                return FileSystem->ProcessEvent(Event, Param);
            }
        }
        else
        {
            return false;
        }
    }
    catch(exception &E)
    {
        HandleFileSystemException(FileSystem, &E);
        return Event == FE_COMMAND ? true : false;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::SetDirectory(HANDLE Plugin, const char *Dir, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->SetDirectory(Dir, OpMode);
        }
    }
    catch(exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::MakeDirectory(HANDLE Plugin, char *Name, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->MakeDirectory(Name, OpMode);
        }
    }
    catch(exception &E)
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
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->DeleteFiles(PanelItem, ItemsNumber, OpMode);
        }
    }
    catch(exception &E)
    {
        HandleFileSystemException(FileSystem, &E, OpMode);
        return 0;
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::GetFiles(HANDLE Plugin,
        struct PluginPanelItem *PanelItem, int ItemsNumber, int Move,
        char *DestPath, int OpMode)
{
    TCustomFarFileSystem *FileSystem = (TCustomFarFileSystem *)Plugin;
    try
    {
        ResetCachedInfo();
        assert(!FOldFar);
        assert(FOpenedPlugins->IndexOf(FileSystem) >= 0);

        {
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
        }
    }
    catch(exception &E)
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
            TGuard Guard(FileSystem->CriticalSection);
            return FileSystem->PutFiles(PanelItem, ItemsNumber, Move, OpMode);
        }
    }
    catch(exception &E)
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
    catch(exception &E)
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
    catch(exception &E)
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
    for (int Index = 0; Index < Strings->Count; Index++)
    {
        if (Result < Strings->Strings[Index].Length())
        {
            Result = Strings->Strings[Index].Length();
        }
    }
    return Result;
}
//---------------------------------------------------------------------------
class TFarMessageDialog : public TFarDialog
{
public:
    TFarMessageDialog(TCustomFarPlugin *Plugin, unsigned int AFlags,
                                 const wstring Title, const wstring Message, TStrings *Buttons,
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
    wstring FTimeoutButtonCaption;
    TFarCheckBox *CheckBox;
};
//---------------------------------------------------------------------------
TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin *Plugin, unsigned int AFlags,
        const wstring Title, const wstring Message, TStrings *Buttons,
        TFarMessageParams *Params) :
    TFarDialog(Plugin), FParams(Params)
{
    assert(Params != NULL);
    assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
    assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
    assert(FLAGCLEAR(AFlags, FMSG_DOWN));
    assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

    TStrings *MessageLines = new TStringList();
    TStrings *MoreMessageLines = NULL;
    try
    {
        FarWrapText(Message, MessageLines, MaxMessageWidth);
        int MaxLen = Plugin->MaxLength(MessageLines);

        if (Params->MoreMessages != NULL)
        {
            MoreMessageLines = new TStringList();
            wstring MoreMessages = Params->MoreMessages->Text;
            while (MoreMessages[MoreMessages.Length()] == '\n' ||
                    MoreMessages[MoreMessages.Length()] == '\r')
            {
                MoreMessages.SetLength(MoreMessages.Length() - 1);
            }
            FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
            int MoreMaxLen = Plugin->MaxLength(MoreMessageLines);
            if (MaxLen < MoreMaxLen)
            {
                MaxLen = MoreMaxLen;
            }
        }

        // temporary
        Size = TPoint(MaxMessageWidth, 10);
        Caption = Title;
        Flags = Flags |
                FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING);

        for (int Index = 0; Index < MessageLines->Count; Index++)
        {
            TFarText *Text = new TFarText(this);
            Text->Caption = MessageLines->Strings[Index];
        }

        TFarLister *MoreMessagesLister = NULL;
        TFarSeparator *MoreMessagesSeparator = NULL;

        if (Params->MoreMessages != NULL)
        {
            new TFarSeparator(this);

            MoreMessagesLister = new TFarLister(this);
            MoreMessagesLister->Items->Assign(MoreMessageLines);
            MoreMessagesLister->Left = BorderBox->Left + 1;

            MoreMessagesSeparator = new TFarSeparator(this);
        }

        int ButtonOffset = (Params->CheckBoxLabel.IsEmpty() ? -1 : -2);
        int ButtonLines = 1;
        TFarButton *Button = NULL;
        FTimeoutButton = NULL;
        for (int Index = 0; Index < Buttons->Count; Index++)
        {
            TFarButton *PrevButton = Button;
            Button = new TFarButton(this);
            Button->Default = (Index == 0);
            Button->Brackets = brNone;
            Button->OnClick = ButtonClick;
            wstring Caption = Buttons->Strings[Index];
            if ((Params->Timeout > 0) &&
                    (Params->TimeoutButton == (unsigned int)Index))
            {
                FTimeoutButtonCaption = Caption;
                Caption = FORMAT(Params->TimeoutStr, (Caption, int(Params->Timeout / 1000)));
                FTimeoutButton = Button;
            }
            Button->Caption = FORMAT(" %s ", (Caption));
            Button->Top = BorderBox->Bottom + ButtonOffset;
            Button->Bottom = Button->Top;
            Button->Result = Index + 1;
            Button->CenterGroup = true;
            Button->Tag = reinterpret_cast<int>(Buttons->Objects[Index]);
            if (PrevButton != NULL)
            {
                Button->Move(PrevButton->Right - Button->Left + 1, 0);
            }

            if (MaxMessageWidth < Button->Right - BorderBox->Left)
            {
                for (int PIndex = 0; PIndex < ItemCount; PIndex++)
                {
                    TFarButton *PrevButton = dynamic_cast<TFarButton *>(Item[PIndex]);
                    if ((PrevButton != NULL) && (PrevButton != Button))
                    {
                        PrevButton->Move(0, -1);
                    }
                }
                Button->Move(- (Button->Left - BorderBox->Left), 0);
                ButtonLines++;
            }

            if (MaxLen < Button->Right - BorderBox->Left)
            {
                MaxLen = Button->Right - BorderBox->Left;
            }

            NextItemPosition = ipRight;
        }

        if (!Params->CheckBoxLabel.IsEmpty())
        {
            NextItemPosition = ipNewLine;
            CheckBox = new TFarCheckBox(this);
            CheckBox->Caption = Params->CheckBoxLabel;

            if (MaxLen < CheckBox->Right - BorderBox->Left)
            {
                MaxLen = CheckBox->Right - BorderBox->Left;
            }
        }
        else
        {
            CheckBox = NULL;
        }

        TPoint S(
            ClientRect.Left + MaxLen + (- (ClientRect.Right + 1)),
            ClientRect.Top + MessageLines->Count +
            (Params->MoreMessages != NULL ? 1 : 0) + ButtonLines +
            (!Params->CheckBoxLabel.IsEmpty() ? 1 : 0) +
            (- (ClientRect.Bottom + 1)));

        if (Params->MoreMessages != NULL)
        {
            int MoreMessageHeight = Plugin->TerminalInfo().y - S.y - 1;
            if (MoreMessageHeight > MoreMessagesLister->Items->Count)
            {
                MoreMessageHeight = MoreMessagesLister->Items->Count;
            }
            assert(MoreMessagesLister != NULL);
            MoreMessagesLister->Height = MoreMessageHeight;
            MoreMessagesLister->Right =
                BorderBox->Right - (MoreMessagesLister->ScrollBar ? 0 : 1);
            MoreMessagesLister->TabStop = MoreMessagesLister->ScrollBar;
            assert(MoreMessagesSeparator != NULL);
            MoreMessagesSeparator->Position =
                MoreMessagesLister->Top + MoreMessagesLister->Height;
            S.y += MoreMessagesLister->Height + 1;
        }

        Size = S;
    }
    catch (...)
    {
    }
    delete MessageLines;
    delete MoreMessageLines;
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
                FParams->TimerEvent(FParams->TimerAnswer);
                if (FParams->TimerAnswer != 0)
                {
                    Close(DefaultButton);
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
            wstring Caption =
                FORMAT(" %s ", (FORMAT(FParams->TimeoutStr,
                                       (FTimeoutButtonCaption, int((FParams->Timeout - Running) / 1000)))));
            Caption += wstring::StringOfChar(' ',
                                                FTimeoutButton->Caption.Length() - Caption.Length());
            FTimeoutButton->Caption = Caption;
        }
    }
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Change()
{
    TFarDialog::Change();

    if (Handle != NULL)
    {
        if ((CheckBox != NULL) && (FCheckBoxChecked != CheckBox->Checked))
        {
            for (int Index = 0; Index < ItemCount; Index++)
            {
                TFarButton *Button = dynamic_cast<TFarButton *>(Item[Index]);
                if ((Button != NULL) && (Button->Tag == 0))
                {
                    Button->Enabled = !CheckBox->Checked;
                }
            }
            FCheckBoxChecked = CheckBox->Checked;
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
        CheckBox->Checked = ACheckBox;
    }

    int Result = ShowModal();
    assert(Result != 0);
    if (Result > 0)
    {
        if (CheckBox != NULL)
        {
            ACheckBox = CheckBox->Checked;
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
        FParams->ClickEvent(FParams->Token, Sender->Result - 1, Close);
    }
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::DialogMessage(unsigned int Flags,
        const wstring Title, const wstring Message, TStrings *Buttons,
        TFarMessageParams *Params)
{
    int Result;
    TFarMessageDialog *Dialog =
        new TFarMessageDialog(this, Flags, Title, Message, Buttons, Params);
    try
    {
        Result = Dialog->Execute(Params->CheckBox);
    }
    catch (...)
    {
    }
    delete Dialog;
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::FarMessage(unsigned int Flags,
        const wstring Title, const wstring Message, TStrings *Buttons,
        TFarMessageParams *Params)
{
    assert(Params != NULL);

    int Result;
    TStrings *MessageLines = NULL;
    char **Items = NULL;
    try
    {
        wstring FullMessage = Message;
        if (Params->MoreMessages != NULL)
        {
            FullMessage += wstring("\n\x01\n") + Params->MoreMessages->Text;
            while (FullMessage[FullMessage.Length()] == '\n' ||
                    FullMessage[FullMessage.Length()] == '\r')
            {
                FullMessage.SetLength(FullMessage.Length() - 1);
            }
            FullMessage += "\n\x01\n";
        }

        MessageLines = new TStringList();
        MessageLines->Add(Title);
        FarWrapText(FullMessage, MessageLines, MaxMessageWidth);

        // FAR WORKAROUND
        // When there is too many lines to fit on screen, far uses not-shown
        // lines as button captions instead of real captions at the end of the list
        int MaxLines = MaxMessageLines();
        while (MessageLines->Count > MaxLines)
        {
            MessageLines->Delete(MessageLines->Count - 1);
        }

        for (int Index = 0; Index < Buttons->Count; Index++)
        {
            MessageLines->Add(Buttons->Strings[Index]);
        }

        Items = new char *[MessageLines->Count];
        for (int Index = 0; Index < MessageLines->Count; Index++)
        {
            wstring S = MessageLines->Strings[Index];
            MessageLines->Strings[Index] = StrToFar(S);
            Items[Index] = MessageLines->Strings[Index].c_str();
        }

        TFarEnvGuard Guard;
        Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
                                      Flags | FMSG_LEFTALIGN, NULL, Items, MessageLines->Count,
                                      Buttons->Count);
    }
    catch (...)
    {
    }
    delete Items;
    delete MessageLines;

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Message(unsigned int Flags,
        const wstring Title, const wstring Message, TStrings *Buttons,
        TFarMessageParams *Params, bool Oem)
{
    // when message is shown while some "custom" output is on screen,
    // make the output actually background of FAR screen
    if (FTerminalScreenShowing)
    {
        FarControl(FCTL_SETUSERSCREEN, NULL);
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
        wstring Items = Title + "\n" + Message;
        if (!Oem)
        {
            StrToFar(Items);
        }
        TFarEnvGuard Guard;
        Result = FStartupInfo.Message(FStartupInfo.ModuleNumber,
                                      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN, NULL, (char **)Items.c_str(), 0, 0);
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, wstring Title,
                                      wstring Bottom, const FarMenuItem *Items, int Count,
                                      const int *BreakKeys, int &BreakCode)
{
    assert(Items);

    wstring ATitle = Title;
    wstring ABottom = Bottom;
    TFarEnvGuard Guard;
    return FStartupInfo.Menu(FStartupInfo.ModuleNumber, -1, -1, 0,
                             Flags, StrToFar(ATitle), (char *)StrToFar(ABottom), NULL, BreakKeys,
                             &BreakCode, Items, Count);
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const wstring Title,
                                      const wstring Bottom, TStrings *Items, const int *BreakKeys,
                                      int &BreakCode)
{
    assert(Items && Items->Count);
    int Result;
    FarMenuItemEx *MenuItems = new FarMenuItemEx[Items->Count];
    try
    {
        int Selected = -1;
        int Count = 0;
        for (int i = 0; i < Items->Count; i++)
        {
            int Flags = int(Items->Objects[i]);
            if (FLAGCLEAR(Flags, MIF_HIDDEN))
            {
                memset(&MenuItems[Count], 0, sizeof(MenuItems[Count]));
                wstring Text = Items->Strings[i].SubString(1, sizeof(MenuItems[i].Text)-1);
                MenuItems[Count].Flags = Flags;
                if (MenuItems[Count].Flags & MIF_SELECTED)
                {
                    assert(Selected < 0);
                    Selected = i;
                }
                strcpy(MenuItems[Count].Text.Text, StrToFar(Text));
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
                Items->Objects[Selected] = (TObject *)(int(Items->Objects[Selected]) & ~MIF_SELECTED);
            }
            Items->Objects[Result] = (TObject *)(int(Items->Objects[Result]) | MIF_SELECTED);
        }
        else
        {
            Result = ResultItem;
        }
    }
    catch (...)
    {
    }
    delete[] MenuItems;
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarPlugin::Menu(unsigned int Flags, const wstring Title,
                                      const wstring Bottom, TStrings *Items)
{
    int BreakCode;
    return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::InputBox(const wstring Title,
        const wstring Prompt, wstring &Text, unsigned long Flags,
        const wstring HistoryName, int MaxLen, TFarInputBoxValidateEvent OnValidate)
{
    bool Repeat;
    int Result;
    do
    {
        wstring DestText;
        DestText.SetLength(MaxLen + 1);
        THandle ScreenHandle = 0;
        SaveScreen(ScreenHandle);
        wstring AText = Text;
        {
            TFarEnvGuard Guard;
            Result = FStartupInfo.InputBox(StrToFar(Title), StrToFar(Prompt),
                                           StrToFar(HistoryName), StrToFar(AText), DestText.c_str(), MaxLen, NULL,
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
                    OnValidate(Text);
                }
                catch(exception &E)
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
void TCustomFarPlugin::Text(int X, int Y, int Color, wstring Str)
{
    TFarEnvGuard Guard;
    FStartupInfo.Text(X, Y, Color, StrToFar(Str));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FlushText()
{
    TFarEnvGuard Guard;
    FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::WriteConsole(wstring Str)
{
    unsigned long Written;
    ::WriteConsole(FConsoleOutput, StrToFar(Str), Str.Length(), &Written, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(wstring Str)
{
    TFarEnvGuard Guard;
    FFarStandardFunctions.CopyToClipboard(StrToFar(Str));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(TStrings *Strings)
{
    if (Strings->Count > 0)
    {
        if (Strings->Count == 1)
        {
            FarCopyToClipboard(Strings->Strings[0]);
        }
        else
        {
            FarCopyToClipboard(Strings->Text);
        }
    }
}
//---------------------------------------------------------------------------
TPoint TCustomFarPlugin::TerminalInfo(TPoint *Size, TPoint *Cursor)
{
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

    return Result;
}
//---------------------------------------------------------------------------
HWND TCustomFarPlugin::GetConsoleWindow()
{
    char Title[512];
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
    Fill.char.AsciiChar = ' ';
    Fill.Attributes = 7;
    ScrollConsoleScreenBuffer(FConsoleOutput, &Source, NULL, Dest, &Fill);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowTerminalScreen()
{
    assert(!FTerminalScreenShowing);
    TPoint Size, Cursor;
    TerminalInfo(&Size, &Cursor);

    wstring Blank = wstring::StringOfChar(' ', Size.x);
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
    FarControl(FCTL_SETUSERSCREEN, NULL);
}
//---------------------------------------------------------------------------
struct TConsoleTitleParam
{
    short Progress;
    short Own;
};
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowConsoleTitle(const wstring Title)
{
    char SaveTitle[512];
    GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
    StrFromFar(SaveTitle);
    TConsoleTitleParam Param;
    Param.Progress = FCurrentProgress;
    Param.Own = !FCurrentTitle.IsEmpty() && (FormatConsoleTitle() == SaveTitle);
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
    assert(FSavedTitles->Count > 0);
    wstring Title = FSavedTitles->Strings[FSavedTitles->Count-1];
    TObject *Object = FSavedTitles->Objects[FSavedTitles->Count-1];
    TConsoleTitleParam Param = *(TConsoleTitleParam *)&Object;
    if (Param.Own)
    {
        FCurrentTitle = Title;
        FCurrentProgress = Param.Progress;
        UpdateConsoleTitle();
    }
    else
    {
        FCurrentTitle = "";
        FCurrentProgress = -1;
        SetConsoleTitle(StrToFar(Title));
    }
    FSavedTitles->Delete(FSavedTitles->Count-1);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle(const wstring Title)
{
    assert(!FCurrentTitle.IsEmpty());
    FCurrentTitle = Title;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
    assert(!FCurrentTitle.IsEmpty());
    FCurrentProgress = Progress;
    UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
wstring TCustomFarPlugin::FormatConsoleTitle()
{
    wstring Title;
    if (FCurrentProgress >= 0)
    {
        Title = FORMAT("{%d%%} %s", (FCurrentProgress, FCurrentTitle));
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
    wstring Title = FormatConsoleTitle();
    SetConsoleTitle(StrToFar(Title));
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveScreen(THandle &Screen)
{
    assert(!Screen);
    TFarEnvGuard Guard;
    Screen = (THandle)FStartupInfo.SaveScreen(0, 0, -1, -1);
    assert(Screen);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::RestoreScreen(THandle &Screen)
{
    assert(Screen);
    TFarEnvGuard Guard;
    FStartupInfo.RestoreScreen((HANDLE)Screen);
    Screen = 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleException(exception *E, int /*OpMode*/)
{
    assert(E);
    Message(FMSG_WARNING | FMSG_MB_OK, "", E->Message);
}
//---------------------------------------------------------------------------
wstring TCustomFarPlugin::GetMsg(int MsgId)
{
    TFarEnvGuard Guard;
    wstring Result = FStartupInfo.GetMsg(FStartupInfo.ModuleNumber, MsgId);
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
bool TCustomFarPlugin::Viewer(wstring FileName,
        unsigned int Flags, wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Viewer(StrToFar(FileName),
                                     StrToFar(Title), 0, 0, -1, -1, Flags);
    return Result;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Editor(wstring FileName,
        unsigned int Flags, wstring Title)
{
    TFarEnvGuard Guard;
    int Result = FStartupInfo.Editor(StrToFar(FileName),
                                     StrToFar(Title), 0, 0, -1, -1, Flags, -1, -1);
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
bool TCustomFarPlugin::FarControl(int Command, void *Param, HANDLE Plugin)
{
    wstring Buf;
    switch (Command)
    {
    case FCTL_CLOSEPLUGIN:
    case FCTL_SETPANELDIR:
    case FCTL_SETANOTHERPANELDIR:
    case FCTL_SETCMDLINE:
    case FCTL_INSERTCMDLINE:
        Buf = (char *)Param;
        Param = StrToFar(Buf);
        break;

    case FCTL_GETCMDLINE:
    case FCTL_GETCMDLINESELECTEDTEXT:
        // ANSI/OEM translation not implemented yet
        assert(false);
        break;
    }

    TFarEnvGuard Guard;
    return FStartupInfo.Control(Plugin, Command, Param);
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
    wstring Buf;
    switch (Command)
    {
    case ECTL_GETINFO:
    case ECTL_SETPARAM:
        // noop
        break;

    case ECTL_SETTITLE:
        Buf = (char *)Param;
        Param = StrToFar(Buf);
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
wstring TCustomFarPlugin::FormatFarVersion(int Version)
{
    return FORMAT("%d.%d.%d", ((Version >> 8) & 0xFF, Version & 0xFF, Version >> 16));
}
//---------------------------------------------------------------------------
wstring TCustomFarPlugin::TemporaryDir()
{
    wstring Result;
    Result.SetLength(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp(Result.c_str(), NULL);
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
void TCustomFarFileSystem::HandleException(exception *E, int OpMode)
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
            wstring HostFile, CurDir, Format, PanelTitle, ShortcutData;
            bool StartSortOrder;
            TFarPanelModes *PanelModes = NULL;
            TFarKeyBarTitles *KeyBarTitles = NULL;
            try
            {
                PanelModes = new TFarPanelModes();
                KeyBarTitles = new TFarKeyBarTitles();
                StartSortOrder = false;

                GetOpenPluginInfoEx(FOpenPluginInfo.Flags, HostFile, CurDir, Format,
                                    PanelTitle, PanelModes, FOpenPluginInfo.StartPanelMode,
                                    FOpenPluginInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);

                FOpenPluginInfo.HostFile = StrToFar(TCustomFarPlugin::DuplicateStr(HostFile));
                FOpenPluginInfo.CurDir = StrToFar(TCustomFarPlugin::DuplicateStr(CurDir));
                FOpenPluginInfo.Format = StrToFar(TCustomFarPlugin::DuplicateStr(Format));
                FOpenPluginInfo.PanelTitle = StrToFar(TCustomFarPlugin::DuplicateStr(PanelTitle));
                PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
                FOpenPluginInfo.StartSortOrder = StartSortOrder;
                KeyBarTitles->FillOpenPluginInfo(&FOpenPluginInfo);
                FOpenPluginInfo.ShortcutData = StrToFar(TCustomFarPlugin::DuplicateStr(ShortcutData));
            }
            catch (...)
            {
            }
            delete PanelModes;
            delete KeyBarTitles;

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
    try
    {
        Result = !FClosed && GetFindDataEx(PanelItems, OpMode);
        if (Result && PanelItems->Count)
        {
            *PanelItem = new PluginPanelItem[PanelItems->Count];
            memset(*PanelItem, 0, PanelItems->Count * sizeof(PluginPanelItem));
            *ItemsNumber = PanelItems->Count;
            for (int Index = 0; Index < PanelItems->Count; Index++)
            {
                ((TCustomFarPanelItem *)PanelItems->Items[Index])->FillPanelItem(
                    &((*PanelItem)[Index]));
            }
        }
        else
        {
            *PanelItem = NULL;
            *ItemsNumber = 0;
        }
    }
    catch (...)
    {
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
    TList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    bool Result;
    try
    {
        Result = ProcessHostFileEx(PanelItems, OpMode);
    }
    catch (...)
    {
    }
    delete PanelItems;

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
int TCustomFarFileSystem::SetDirectory(const char *Dir, int OpMode)
{
    ResetCachedInfo();
    InvalidateOpenPluginInfo();
    int Result = SetDirectoryEx(StrFromFar(Dir), OpMode);
    InvalidateOpenPluginInfo();
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectory(char *Name, int OpMode)
{
    ResetCachedInfo();
    wstring NameStr = Name;
    int Result;
    try
    {
        StrFromFar(NameStr);
        Result = MakeDirectoryEx(NameStr, OpMode);
    }
    catch (...)
    {
    }
    StrToFar(NameStr);
    if (NameStr != Name)
    {
        strcpy(Name, NameStr.c_str());
    }
    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::DeleteFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int OpMode)
{
    ResetCachedInfo();
    TList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    bool Result;
    try
    {
        Result = DeleteFilesEx(PanelItems, OpMode);
    }
    catch (...)
    {
    }
    delete PanelItems;

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int Move, char *DestPath, int OpMode)
{
    ResetCachedInfo();
    TList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    int Result;
    wstring DestPathStr = DestPath;
    try
    {
        StrFromFar(DestPathStr);
        Result = GetFilesEx(PanelItems, Move, DestPathStr, OpMode);
    }
    catch (...)
    {
    }
    StrToFar(DestPathStr);
    if (DestPathStr != DestPath)
    {
        strcpy(DestPath, DestPathStr.c_str());
    }
    delete PanelItems;

    return Result;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFiles(struct PluginPanelItem *PanelItem,
        int ItemsNumber, int Move, int OpMode)
{
    ResetCachedInfo();
    TList *PanelItems = CreatePanelItemList(PanelItem, ItemsNumber);
    int Result;
    try
    {
        Result = PutFilesEx(PanelItems, Move, OpMode);
    }
    catch (...)
    {
    }
    delete PanelItems;

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
        if (!FarControl(Another == 0 ? FCTL_GETPANELINFO : FCTL_GETANOTHERPANELINFO, Info))
        {
            memset(Info, 0, sizeof(*Info));
            assert(false);
        }
        FPanelInfo[Another] = new TFarPanelInfo(Info, (Another == 0 ? this : NULL));
    }
    return FPanelInfo[Another];
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::FarControl(int Command, void *Param)
{
    return FPlugin->FarControl(Command, Param, this);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
    unsigned int PrevInstances = FInstances;
    InvalidateOpenPluginInfo();
    FarControl(Another ? FCTL_UPDATEANOTHERPANEL : FCTL_UPDATEPANEL,
               (void *)(!ClearSelection));
    return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::RedrawPanel(bool Another)
{
    FarControl(Another ? FCTL_REDRAWANOTHERPANEL : FCTL_REDRAWPANEL, NULL);
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::ClosePlugin()
{
    FClosed = true;
    FarControl(FCTL_CLOSEPLUGIN, "C:\\");
    // FAR WORKAROUND
    // Calling UpdatePanel() is necessary, otherwise plugin remains in panel,
    // but it causes FAR to fail
    // UpdatePanel();
}
//---------------------------------------------------------------------------
wstring TCustomFarFileSystem::GetMsg(int MsgId)
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
    return (PanelInfo->Bounds.Left <= 0);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::IsRight()
{
    return !IsLeft();
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessHostFileEx(TList * /*PanelItems*/, int /*OpMode*/)
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
bool TCustomFarFileSystem::SetDirectoryEx(const wstring /*Dir*/, int /*OpMode*/)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::MakeDirectoryEx(wstring & /*Name*/, int /*OpMode*/)
{
    return -1;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::DeleteFilesEx(TList * /*PanelItems*/, int /*OpMode*/)
{
    return false;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::GetFilesEx(TList * /*PanelItems*/, bool /*Move*/,
        wstring & /*DestPath*/, int /*OpMode*/)
{
    return 0;
}
//---------------------------------------------------------------------------
int TCustomFarFileSystem::PutFilesEx(TList * /*PanelItems*/,
        bool /*Move*/, int /*OpMode*/)
{
    return 0;
}
//---------------------------------------------------------------------------
TList *TCustomFarFileSystem::CreatePanelItemList(
    struct PluginPanelItem *PanelItem, int ItemsNumber)
{
    TList *PanelItems = new TObjectList();
    try
    {
        for (int Index = 0; Index < ItemsNumber; Index++)
        {
            PanelItems->Add(new TFarPanelItem(&PanelItem[Index]));
        }
    }
    catch(...)
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
void TFarPanelModes::SetPanelMode(int Mode, const wstring ColumnTypes,
        const wstring ColumnWidths, TStrings *ColumnTitles,
        bool FullScreen, bool DetailedStatus, bool AlignExtensions,
        bool CaseConversion, const wstring StatusColumnTypes,
        const wstring StatusColumnWidths)
{
    int ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes)+1 : 0;
    assert(Mode >= 0 && Mode < LENOF(FPanelModes));
    assert(!ColumnTitles || (ColumnTitles->Count == ColumnTypesCount));

    ClearPanelMode(FPanelModes[Mode]);
    FPanelModes[Mode].ColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnTypes));
    FPanelModes[Mode].ColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(ColumnWidths));
    if (ColumnTitles)
    {
        FPanelModes[Mode].ColumnTitles = new char *[ColumnTypesCount];
        for (int Index = 0; Index < ColumnTypesCount; Index++)
        {
            FPanelModes[Mode].ColumnTitles[Index] = StrToFar(
                    TCustomFarPlugin::DuplicateStr(ColumnTitles->Strings[Index]));
        }
    }
    FPanelModes[Mode].FullScreen = FullScreen;
    FPanelModes[Mode].DetailedStatus = DetailedStatus;
    FPanelModes[Mode].AlignExtensions = AlignExtensions;
    FPanelModes[Mode].CaseConversion = CaseConversion;

    FPanelModes[Mode].StatusColumnTypes = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnTypes));
    FPanelModes[Mode].StatusColumnWidths = StrToFar(TCustomFarPlugin::DuplicateStr(StatusColumnWidths));
}
//---------------------------------------------------------------------------
void TFarPanelModes::ClearPanelMode(PanelMode &Mode)
{
    if (Mode.ColumnTypes)
    {
        int ColumnTypesCount = Mode.ColumnTypes ?
                               CommaCount(wstring(Mode.ColumnTypes)) + 1 : 0;

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
int TFarPanelModes::CommaCount(const wstring ColumnTypes)
{
    int Count = 0;
    for (int Index = 1; Index <= ColumnTypes.Length(); Index++)
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
        SetKeyBarTitle(ShiftStatus, Index, "");
    }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
        int FunctionKey, const wstring Title)
{
    assert(FunctionKey >= 1 && FunctionKey <= LENOF(FKeyBarTitles.Titles));
    char **Titles;
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
    Titles[FunctionKey-1] = StrToFar(TCustomFarPlugin::DuplicateStr(Title, true));
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
wstring TCustomFarPanelItem::CustomColumnData(int /*Column*/)
{
    assert(false);
    return "";
}
//---------------------------------------------------------------------------
void TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem *PanelItem)
{
    assert(PanelItem);

    wstring FileName;
    __int64 Size = 0;
    TDateTime LastWriteTime;
    TDateTime LastAccess;
    wstring Description;
    wstring Owner;

    GetData(PanelItem->Flags, FileName, Size, PanelItem->FindData.dwFileAttributes,
            LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
            (void *)PanelItem->UserData, PanelItem->CustomColumnNumber);

    FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
    FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
    PanelItem->FindData.ftCreationTime = FileTime;
    PanelItem->FindData.ftLastAccessTime = FileTimeA;
    PanelItem->FindData.ftLastWriteTime = FileTime;
    PanelItem->FindData.nFileSizeLow = (long int)Size;
    PanelItem->FindData.nFileSizeHigh = (long int)(Size >> 32);
    PanelItem->PackSize = (long int)Size;
    ASCOPY(PanelItem->FindData.cFileName, FileName);
    StrToFar(PanelItem->FindData.cFileName);
    PanelItem->Description = StrToFar(TCustomFarPlugin::DuplicateStr(Description));
    PanelItem->Owner = StrToFar(TCustomFarPlugin::DuplicateStr(Owner));

    PanelItem->CustomColumnData = new char *[PanelItem->CustomColumnNumber];
    for (int Index = 0; Index < PanelItem->CustomColumnNumber; Index++)
    {
        PanelItem->CustomColumnData[Index] =
            StrToFar(TCustomFarPlugin::DuplicateStr(CustomColumnData(Index)));
    }
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
    unsigned long & /*Flags*/, wstring & /*FileName*/, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, wstring & /*Description*/,
    wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
    assert(false);
}
//---------------------------------------------------------------------------
wstring TFarPanelItem::CustomColumnData(int /*Column*/)
{
    assert(false);
    return "";
}
//---------------------------------------------------------------------------
unsigned long TFarPanelItem::GetFlags()
{
    return FPanelItem->Flags;
}
//---------------------------------------------------------------------------
wstring TFarPanelItem::GetFileName()
{
    wstring Result = FPanelItem->FindData.cFileName;
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
    return (FileName == "..");
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsFile()
{
    return (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
THintPanelItem::THintPanelItem(const wstring AHint) :
    TCustomFarPanelItem()
{
    FHint = AHint;
}
//---------------------------------------------------------------------------
void THintPanelItem::GetData(
    unsigned long & /*Flags*/, wstring &FileName, __int64 & /*Size*/,
    unsigned long & /*FileAttributes*/,
    TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
    unsigned long & /*NumberOfLinks*/, wstring & /*Description*/,
    wstring & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
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
    return FPanelInfo->PanelRect;
}
//---------------------------------------------------------------------------
int TFarPanelInfo::GetSelectedCount()
{
    int Count = FPanelInfo->SelectedItemsNumber;

    if (Count == 1 &&
            (FPanelInfo->SelectedItems[0].Flags & PPIF_SELECTED) == 0)
    {
        Count = 0;
    }

    return Count;
}
//---------------------------------------------------------------------------
TList *TFarPanelInfo::GetItems()
{
    if (!FItems)
    {
        FItems = new TObjectList();
        for (int Index = 0; Index < FPanelInfo->ItemsNumber; Index++)
        {
            FItems->Add(new TFarPanelItem(&FPanelInfo->PanelItems[Index]));
        }
    }
    return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::FindFileName(const wstring FileName)
{
    TList *AItems = Items;
    TFarPanelItem *PanelItem;
    for (int Index = 0; Index < AItems->Count; Index++)
    {
        PanelItem = static_cast<TFarPanelItem *>(AItems->Items[Index]);
        if (PanelItem->FileName == FileName)
        {
            return PanelItem;
        }
    }
    return NULL;
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::FindUserData(void *UserData)
{
    TList *AItems = Items;
    TFarPanelItem *PanelItem;
    for (int Index = 0; Index < AItems->Count; Index++)
    {
        PanelItem = static_cast<TFarPanelItem *>(AItems->Items[Index]);
        if (PanelItem->UserData == UserData)
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
    FOwner->FarControl(FCTL_SETSELECTION, FPanelInfo);
}
//---------------------------------------------------------------------------
TFarPanelItem *TFarPanelInfo::GetFocusedItem()
{
    int Index = FocusedIndex;
    if ((Index >= 0) && (Items->Count > 0))
    {
        assert(Index < Items->Count);
        return (TFarPanelItem *)Items->Items[Index];
    }
    else
    {
        return NULL;
    }
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedItem(TFarPanelItem *value)
{
    int Index = Items->IndexOf(value);
    assert(Index >= 0);
    FocusedIndex = Index;
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
    if (FocusedIndex != value)
    {
        assert(value >= 0 && value < FPanelInfo->ItemsNumber);
        FPanelInfo->CurrentItem = value;
        PanelRedrawInfo PanelInfo;
        PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
        PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
        FOwner->FarControl(FCTL_REDRAWPANEL, &PanelInfo);
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
wstring TFarPanelInfo::GetCurrentDirectory()
{
    wstring Result = FPanelInfo->CurDir;
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
    if ((Index == ItemFocused) && !Focused)
    {
        FItemFocused = -1;
    }
    if (Focused)
    {
        if (ItemFocused >= 0)
        {
            SetFlag(ItemFocused, MIF_SELECTED, false);
        }
        FItemFocused = Index;
    }
}
//---------------------------------------------------------------------------
int TFarMenuItems::Add(wstring Text, bool Visible)
{
    int Result = TStringList::Add(Text);
    if (!Visible)
    {
        SetFlag(Count - 1, MIF_HIDDEN, true);
    }
    return Result;
}
//---------------------------------------------------------------------------
void TFarMenuItems::AddSeparator(bool Visible)
{
    Add("");
    SetFlag(Count - 1, MIF_SEPARATOR, true);
    if (!Visible)
    {
        SetFlag(Count - 1, MIF_HIDDEN, true);
    }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetItemFocused(int value)
{
    if (ItemFocused != value)
    {
        if (ItemFocused >= 0)
        {
            SetFlag(ItemFocused, MIF_SELECTED, false);
        }
        FItemFocused = value;
        SetFlag(ItemFocused, MIF_SELECTED, true);
    }
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetFlag(int Index, int Flag, bool Value)
{
    if (GetFlag(Index, Flag) != Value)
    {
        int F = int(Objects[Index]);
        if (Value)
        {
            F |= Flag;
        }
        else
        {
            F &= ~Flag;
        }
        Objects[Index] = (TObject *)F;
    }
}
//---------------------------------------------------------------------------
bool TFarMenuItems::GetFlag(int Index, int Flag)
{
    return int(Objects[Index]) & Flag;
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
wstring TFarEditorInfo::GetFileName()
{
    wstring Result = FEditorInfo->FileName;
    return StrFromFar(Result);
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
    assert(AreFileApisANSI());
    assert(FarPlugin != NULL);
    if (!FarPlugin->ANSIApis)
    {
        SetFileApisToOEM();
    }
}
//---------------------------------------------------------------------------
TFarEnvGuard::~TFarEnvGuard()
{
    assert(FarPlugin != NULL);
    if (!FarPlugin->ANSIApis)
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
    assert(FANSIApis == FarPlugin->ANSIApis);

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
//---------------------------------------------------------------------------
void FarWrapText(wstring Text, TStrings *Result, int MaxWidth)
{
    int TabSize = 8;
    TStringList *Lines = NULL;
    TStringList *WrappedLines = NULL;
    try
    {
        Lines = new TStringList();
        Lines->Text = Text;
        WrappedLines = new TStringList();
        for (int Index = 0; Index < Lines->Count; Index++)
        {
            wstring WrappedLine = Lines->Strings[Index];
            if (!WrappedLine.IsEmpty())
            {
                WrappedLine = StringReplace(WrappedLine, "'", "\3", TReplaceFlags() << rfReplaceAll);
                WrappedLine = StringReplace(WrappedLine, "\"", "\4", TReplaceFlags() << rfReplaceAll);
                WrappedLine = WrapText(WrappedLine, MaxWidth);
                WrappedLine = StringReplace(WrappedLine, "\3", "'", TReplaceFlags() << rfReplaceAll);
                WrappedLine = StringReplace(WrappedLine, "\4", "\"", TReplaceFlags() << rfReplaceAll);
                WrappedLines->Text = WrappedLine;
                for (int WrappedIndex = 0; WrappedIndex < WrappedLines->Count; WrappedIndex++)
                {
                    wstring FullLine = WrappedLines->Strings[WrappedIndex];
                    do
                    {
                        // WrapText does not wrap when not possible, enforce it
                        // (it also does not wrap when the line is longer than maximum only
                        // because of trailing dot or similar)
                        wstring Line = FullLine.SubString(1, MaxWidth);
                        FullLine.Delete(1, MaxWidth);

                        int P;
                        while ((P = Line.Pos("\t")) > 0)
                        {
                            Line.Delete(P, 1);
                            Line.Insert(wstring::StringOfChar(' ',
                                                                 ((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1),
                                        P);
                        }
                        Result->Add(Line);
                    }
                    while (!FullLine.IsEmpty());
                }
            }
            else
            {
                Result->Add("");
            }
        }
    }
    catch (...)
    {
    }
    delete Lines;
    delete WrappedLines;
}
//---------------------------------------------------------------------------
wstring StrFromFar(const char *S)
{
    wstring Result = S;
    OemToChar(Result.c_str(), Result.c_str());
    return Result;
}
