//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "FarPlugin.h"
#include "FarDialog.h"
#include <Common.h>
#include "Exceptions.h"
#include "TextsCore.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "puttyexp.h"

//---------------------------------------------------------------------------
TCustomFarPlugin * FarPlugin = NULL;
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
  Token = NULL;
}
//---------------------------------------------------------------------------
TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst) :
  TObject()
{
  // DEBUG_PRINTF(L"TCustomFarPlugin: begin");
  InitPlatformId();
  Self = this;
  FFarThread = GetCurrentThreadId();
  FCriticalSection = new TCriticalSection;
  FHandle = HInst;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FOpenedPlugins = new TObjectList();
  FOpenedPlugins->SetOwnsObjects(false);
  FSavedTitles = new TStringList();
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
  assert(FOpenedPlugins->Count == 0);
  delete FOpenedPlugins;
  for (int I = 0; I < FSavedTitles->Count; I++)
    delete FSavedTitles->Objects[I];
  delete FSavedTitles;
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::HandlesFunction(THandlesFunction /*Function*/)
{
  return false;
}
//---------------------------------------------------------------------------
VersionInfo __fastcall TCustomFarPlugin::GetMinFarVersion()
{
  return MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    ResetCachedInfo();
    // Info->StructSize = 336 for FAR 1.65
    memset(&FStartupInfo, 0, sizeof(FStartupInfo));
    memmove(&FStartupInfo, Info,
            Info->StructSize >= sizeof(FStartupInfo) ?
            sizeof(FStartupInfo) : static_cast<size_t>(Info->StructSize));
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
  catch (Exception & E)
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
void __fastcall TCustomFarPlugin::GetPluginInfo(struct PluginInfo * Info)
{
  try
  {
    ResetCachedInfo();

    TStringList DiskMenu;
    TStringList PluginMenu;
    TStringList PluginConfig;
    TStringList CommandPrefixes;

    ClearPluginInfo(FPluginInfo);

    GetPluginInfoEx(FPluginInfo.Flags, &DiskMenu, &PluginMenu,
                    &PluginConfig, &CommandPrefixes);

    #define COMPOSESTRINGARRAY(NAME) \
        if (NAME.Count) \
        { \
          wchar_t ** StringArray = new wchar_t *[NAME.Count]; \
          GUID *Guids = new GUID[NAME.Count]; \
          FPluginInfo.NAME.Guids = Guids; \
          FPluginInfo.NAME.Strings = StringArray; \
          FPluginInfo.NAME.Count = NAME.Count; \
          for (int Index = 0; Index < NAME.Count; Index++) \
          { \
            StringArray[Index] = DuplicateStr(NAME.Strings[Index]); \
            Guids[Index] = *reinterpret_cast<const GUID *>(NAME.Objects[Index]); \
          } \
        }

    COMPOSESTRINGARRAY(DiskMenu);
    COMPOSESTRINGARRAY(PluginMenu);
    COMPOSESTRINGARRAY(PluginConfig);

    #undef COMPOSESTRINGARRAY
    // FIXME
    /*
    if (DiskMenuStrings.GetCount())
    {
        wchar_t *NumberArray = new wchar_t[DiskMenuStrings.GetCount()];
        FPluginInfo.DiskMenuNumbers = &NumberArray;
        for (int Index = 0; Index < DiskMenuStrings.GetCount(); Index++)
        {
          NumberArray[Index] = (int)DiskMenu.GetObject(Index);
        }
    }
    */

    UnicodeString CommandPrefix;
    for (int Index = 0; Index < CommandPrefixes.Count; Index++)
    {
      CommandPrefix = CommandPrefix + (CommandPrefix.IsEmpty() ? L"" : L":") +
                      CommandPrefixes.Strings[Index];
    }
    // DEBUG_PRINTF(L"CommandPrefix = %s", CommandPrefix.c_str());
    FPluginInfo.CommandPrefix = DuplicateStr(CommandPrefix);

    memmove(Info, &FPluginInfo, sizeof(FPluginInfo));
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::GetModuleName()
{
  return FStartupInfo.ModuleName;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClearPluginInfo(PluginInfo & Info)
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
wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return NULL;
  }
  else
  {
    // DEBUG_PRINTF(L"Str = %s", Str.c_str());
    const size_t sz = Str.Length() + 1;
    wchar_t * Result = new wchar_t[sz];
    wcscpy_s(Result, sz, Str.c_str());
    return Result;
  }
}
//---------------------------------------------------------------------------
RECT __fastcall TCustomFarPlugin::GetPanelBounds(HANDLE PanelHandle)
{
  PanelInfo Info = {0};
  Info.StructSize = sizeof(PanelInfo);
  FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(&Info), PanelHandle);

  RECT Bounds;
  memset(&Bounds, -1, sizeof(Bounds));
  if (Info.PluginHandle)
  {
    Bounds = Info.PanelRect;
  }
  return Bounds;
}

//---------------------------------------------------------------------------
TCustomFarFileSystem * __fastcall TCustomFarPlugin::GetPanelFileSystem(bool Another,
    HANDLE /* Plugin */)
{
  // DEBUG_PRINTF(L"begin");
  TCustomFarFileSystem * Result = NULL;
  RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  TCustomFarFileSystem * FileSystem = NULL;
  int Index = 0;
  while (!Result && (Index < FOpenedPlugins->Count))
  {
    FileSystem = dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    assert(FileSystem);
    RECT Bounds = GetPanelBounds(FileSystem);
    if (Another && CompareRects(Bounds, PassivePanelBounds))
    {
      Result = FileSystem;
    }
    else if (!Another && CompareRects(Bounds, ActivePanelBounds))
    {
      Result = FileSystem;
    }
    Index++;
  }
  // DEBUG_PRINTF(L"end");
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::InvalidateOpenPanelInfo()
{
  for (int Index = 0; Index < FOpenedPlugins->Count; Index++)
  {
    TCustomFarFileSystem * FileSystem =
      dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    FileSystem->InvalidateOpenPanelInfo();
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::Configure(const struct ConfigureInfo *Info)
{
  try
  {
    ResetCachedInfo();
    intptr_t Result = ConfigureEx(Info->Guid);
    InvalidateOpenPanelInfo();

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return (int)false;
  }
}
//---------------------------------------------------------------------------
void * __fastcall TCustomFarPlugin::OpenPlugin(const struct OpenInfo *Info)
{
  try
  {
    ResetCachedInfo();
    intptr_t Item = 0;
    if (*Info->Guid == MenuCommandsGuid)
      Item = 1;
    if ((Info->OpenFrom == OPEN_SHORTCUT) || 
      (Info->OpenFrom == OPEN_COMMANDLINE) ||
      (Info->OpenFrom == OPEN_ANALYSE))
    {
      Item = Info->Data;
    }
    TCustomFarFileSystem * Result = OpenPluginEx(Info->OpenFrom, Item);

    if (Result)
    {
      FOpenedPlugins->Add(Result);
    }
    else
    {
      Result = NULL;
    }

    return Result;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return NULL;
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClosePanel(void * Plugin)
{
  try
  {
    ResetCachedInfo();
    TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    TRY_FINALLY2 (Self, FileSystem,
    {
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        FileSystem->Close();
      }
    }
    ,
    {
      Self->FOpenedPlugins->Remove(FileSystem);
    }
    );
    delete FileSystem;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FileSystem, Exception * E, int OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API fuctions that must update
  // panel contents on themselves (like ProcessPanelInput), the instance of filesystem
  // may not exists anymore.
  // Check against object pointer is stupid, but no other idea so far.
  if (FOpenedPlugins->IndexOf(FileSystem) != NPOS)
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
void __fastcall TCustomFarPlugin::GetOpenPanelInfo(struct OpenPanelInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      FileSystem->GetOpenPanelInfo(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::GetFindData(struct GetFindDataInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->GetFindData(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FreeFindData(const struct FreeFindDataInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      FileSystem->FreeFindData(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::ProcessHostFile(const struct ProcessHostFileInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessHostFile))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

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
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessKey))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

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
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
    // when error occurs, assume that key can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::ProcessPanelEvent(const struct ProcessPanelEventInfo *Info)
{
  TCustomFarFileSystem *FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessPanelEvent))
    {
      assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

      UnicodeString Buf;
      void *Param = Info->Param;
      if ((Info->Event == FE_CHANGEVIEWMODE) || (Info->Event == FE_COMMAND))
      {

        Buf = static_cast<wchar_t *>(Info->Param);
        Param = const_cast<void *>(reinterpret_cast<const void *>(Buf.c_str()));
      }

      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->ProcessPanelEvent(Info->Event, Param);
      }
    }
    else
    {
      return (int)false;
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
    return Info->Event == FE_COMMAND ? true : false;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::SetDirectory(const struct SetDirectoryInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  UnicodeString PrevCurrentDirectory = FileSystem->GetCurrentDirectory();
  // DEBUG_PRINTF(L"PrevCurrentDirectory = %s", PrevCurrentDirectory.c_str());
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->SetDirectory(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    if (!PrevCurrentDirectory.IsEmpty())
    {
      SetDirectoryInfo Info2;
      Info2.StructSize = sizeof(Info2);
      Info2.hPanel = Info->hPanel;
      Info2.Dir = PrevCurrentDirectory.c_str();
      Info2.UserData = Info->UserData;
      Info2.OpMode = Info->OpMode;
      try
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        return FileSystem->SetDirectory(&Info2);
      }
      catch(Exception & E)
      {
        return 0;
      }
    }
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::MakeDirectory(struct MakeDirectoryInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->MakeDirectory(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::DeleteFiles(const struct DeleteFilesInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->DeleteFiles(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::GetFiles(struct GetFilesInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->GetFiles(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    // display error even for OPM_FIND
    HandleFileSystemException(FileSystem, &E, Info->OpMode & ~OPM_FIND);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::PutFiles(const struct PutFilesInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);

    {
      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->PutFiles(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E, Info->OpMode);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::ProcessEditorEvent(const struct ProcessEditorEventInfo *Info)
{
  try
  {
    ResetCachedInfo();

   return ProcessEditorEventEx(Info);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::ProcessEditorInput(const struct ProcessEditorInputInfo *Info)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorInputEx(&Info->Rec);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
    // when error occurs, assume that input event can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxMessageLines()
{
  return TerminalInfo().y - 5;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxMenuItemLength()
{
  // got from maximal length of path in FAR's folders history
  return TerminalInfo().x - 13;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::MaxLength(TStrings * Strings)
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
  TFarMessageDialog(TCustomFarPlugin * Plugin,
    TFarMessageParams * Params);
  void __fastcall Init(unsigned int AFlags,
    const UnicodeString Title, const UnicodeString Message, TStrings * Buttons);

  intptr_t __fastcall Execute(bool & ACheckBox);

protected:
  virtual void __fastcall Change();
  virtual void __fastcall Idle();

private:
  void ButtonClick(TFarButton * Sender, bool & Close);

private:
  bool FCheckBoxChecked;
  TFarMessageParams * FParams;
  TDateTime FStartTime;
  TDateTime FLastTimerTime;
  TFarButton * FTimeoutButton;
  UnicodeString FTimeoutButtonCaption;
  TFarCheckBox * FCheckBox;
};
//---------------------------------------------------------------------------
TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin * Plugin,
  TFarMessageParams * Params) :
  TFarDialog(Plugin),
  FCheckBoxChecked(false),
  FParams(Params),
  FTimeoutButton(NULL),
  FCheckBox(NULL)
{
  assert(Params != NULL);
}
//---------------------------------------------------------------------------
void __fastcall TFarMessageDialog::Init(unsigned int AFlags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons)
{
  assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
  assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

  TStrings * MessageLines = new TStringList();
  std::auto_ptr<TStrings> MessageLinesPtr(MessageLines);
  {
    FarWrapText(Message, MessageLines, MaxMessageWidth);
    int MaxLen = GetFarPlugin()->MaxLength(MessageLines);
    // DEBUG_PRINTF(L"MaxLen = %d, FParams->MoreMessages = %x", MaxLen, FParams->MoreMessages);
    TStrings * MoreMessageLines = NULL;
    std::auto_ptr<TStrings> MoreMessageLinesPtr(NULL);
    if (FParams->MoreMessages != NULL)
    {
      MoreMessageLines = new TStringList();
      MoreMessageLinesPtr.reset(MoreMessageLines);
      UnicodeString MoreMessages = FParams->MoreMessages->Text;
      while (MoreMessages[MoreMessages.Length()] == L'\n' ||
             MoreMessages[MoreMessages.Length()] == L'\r')
      {
        MoreMessages.SetLength(MoreMessages.Length() - 1);
      }
      FarWrapText(MoreMessages, MoreMessageLines, MaxMessageWidth);
      intptr_t MoreMaxLen = GetFarPlugin()->MaxLength(MoreMessageLines);
      if (MaxLen < MoreMaxLen)
      {
        MaxLen = MoreMaxLen;
      }
    }

    // temporary
    // DEBUG_PRINTF(L"MaxMessageWidth = %d, Title = %s", MaxMessageWidth, Title.c_str());
    SetSize(TPoint(MaxMessageWidth, 10));
    SetCaption(Title);
    SetFlags(GetFlags() |
             FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING));

    for (int Index = 0; Index < MessageLines->Count; Index++)
    {
      TFarText * Text = new TFarText(this);
      Text->SetCaption(MessageLines->Strings[Index]);
    }

    TFarLister * MoreMessagesLister = NULL;
    TFarSeparator * MoreMessagesSeparator = NULL;

    if (FParams->MoreMessages != NULL)
    {
      new TFarSeparator(this);

      MoreMessagesLister = new TFarLister(this);
      MoreMessagesLister->GetItems()->Assign(MoreMessageLines);
      MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

      MoreMessagesSeparator = new TFarSeparator(this);
    }

    int ButtonOffset = (FParams->CheckBoxLabel.IsEmpty() ? -1 : -2);
    int ButtonLines = 1;
    TFarButton * Button = NULL;
    FTimeoutButton = NULL;
    for (int Index = 0; Index < Buttons->Count; Index++)
    {
      TFarButton * PrevButton = Button;
      Button = new TFarButton(this);
      Button->SetDefault(Index == 0);
      Button->SetBrackets(brNone);
      Button->SetOnClick(MAKE_CALLBACK2(TFarMessageDialog::ButtonClick, this));
      UnicodeString Caption = Buttons->Strings[Index];
      if ((FParams->Timeout > 0) &&
          (FParams->TimeoutButton == (size_t)Index))
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
      Button->SetTag(reinterpret_cast<int>(Buttons->Objects[Index]));
      if (PrevButton != NULL)
      {
        Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
      }

      if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
      {
        for (int PIndex = 0; PIndex < GetItemCount(); PIndex++)
        {
          TFarButton * PrevButton = dynamic_cast<TFarButton *>(GetItem(PIndex));
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
        MaxLen = Button->GetRight() - GetBorderBox()->GetLeft() + 2;
      }
      // DEBUG_PRINTF(L"MaxLen = %d", MaxLen);

      SetNextItemPosition(ipRight);
    }

    // DEBUG_PRINTF(L"FParams->CheckBoxLabel = %s", FParams->CheckBoxLabel.c_str());
    if (!FParams->CheckBoxLabel.IsEmpty())
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

    TRect rect = GetClientRect();
    // DEBUG_PRINTF(L"rect.Left = %d, MaxLen = %d, rect.Right = %d", rect.Left, MaxLen, rect.Right);
    TPoint S(
      // rect.Left + MaxLen + (-(rect.Right + 1)),
      rect.Left + MaxLen - rect.Right,
      rect.Top + MessageLines->Count +
      (FParams->MoreMessages != NULL ? 1 : 0) + ButtonLines +
      (!FParams->CheckBoxLabel.IsEmpty() ? 1 : 0) +
      (-(rect.Bottom + 1)));

    if (FParams->MoreMessages != NULL)
    {
      int MoreMessageHeight = GetFarPlugin()->TerminalInfo().y - S.y - 1;
      assert(MoreMessagesLister != NULL);
      if (MoreMessageHeight > MoreMessagesLister->GetItems()->Count)
      {
        MoreMessageHeight = MoreMessagesLister->GetItems()->Count;
      }
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
void __fastcall TFarMessageDialog::Idle()
{
  TFarDialog::Idle();

  if (FParams->Timer > 0)
  {
    size_t SinceLastTimer = static_cast<size_t>((static_cast<double>(Now()) - static_cast<double>(FLastTimerTime)) * 24*60*60*1000);
    if (SinceLastTimer >= FParams->Timeout)
    {
      assert(FParams->TimerEvent);
      if (FParams->TimerEvent)
      {
        FParams->TimerAnswer = 0;
        FParams->TimerEvent(FParams->TimerAnswer);
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
    size_t Running = static_cast<size_t>((static_cast<double>(Now()) - static_cast<double>(FStartTime)) * 24*60*60*1000);
    if (Running >= FParams->Timeout)
    {
      assert(FTimeoutButton != NULL);
      Close(FTimeoutButton);
    }
    else
    {
      UnicodeString Caption =
        FORMAT(L" %s ", FORMAT(FParams->TimeoutStr.c_str(),
                               FTimeoutButtonCaption.c_str(), static_cast<int>((FParams->Timeout - Running) / 1000)).c_str()).c_str();
      int sz = FTimeoutButton->GetCaption().Length() > Caption.Length() ? FTimeoutButton->GetCaption().Length() - Caption.Length() : 0;
      Caption += ::StringOfChar(L' ', sz);
      FTimeoutButton->SetCaption(Caption);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != NULL)
  {
    if ((FCheckBox != NULL) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (int Index = 0; Index < GetItemCount(); Index++)
      {
        TFarButton * Button = dynamic_cast<TFarButton *>(GetItem(Index));
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
intptr_t __fastcall TFarMessageDialog::Execute(bool & ACheckBox)
{
  FStartTime = Now();
  FLastTimerTime = FStartTime;
  FCheckBoxChecked = !ACheckBox;
  if (FCheckBox != NULL)
  {
    FCheckBox->SetChecked(ACheckBox);
  }

  intptr_t Result = ShowModal();
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
void TFarMessageDialog::ButtonClick(TFarButton * Sender, bool & Close)
{
  if (FParams->ClickEvent)
  {
    FParams->ClickEvent(FParams->Token, Sender->GetResult() - 1, Close);
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::DialogMessage(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  intptr_t Result;
  TFarMessageDialog * Dialog =
    new TFarMessageDialog(this, Params);
  std::auto_ptr<TFarMessageDialog> DialogPtr(Dialog);
  {
    Dialog->Init(Flags, Title, Message, Buttons);
    Result = Dialog->Execute(Params->CheckBox);
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::FarMessage(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  assert(Params != NULL);

  int Result;
  TStringList * MessageLines = NULL;
  std::auto_ptr<TStrings> MessageLinesPtr(NULL);
  wchar_t ** Items = NULL;
  TRY_FINALLY1 (Items,
  {
    UnicodeString FullMessage = Message;
    if (Params->MoreMessages != NULL)
    {
      FullMessage += UnicodeString(L"\n\x01\n") + Params->MoreMessages->Text;
      while (FullMessage[FullMessage.Length()] == L'\n' ||
             FullMessage[FullMessage.Length()] == L'\r')
      {
        FullMessage.SetLength(FullMessage.Length() - 1);
      }
      FullMessage += L"\n\x01\n";
    }

    MessageLines = new TStringList();
    MessageLinesPtr.reset(MessageLines);
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

    Items = new wchar_t *[MessageLines->Count];
    for (int Index = 0; Index < MessageLines->Count; Index++)
    {
      UnicodeString S = MessageLines->Strings[Index];
      MessageLines->Strings[Index] = UnicodeString(S);
      Items[Index] = const_cast<wchar_t *>(MessageLines->Strings[Index].c_str());
    }

    TFarEnvGuard Guard;
    Result = FStartupInfo.Message(&MainGuid, &MainGuid,
      Flags | FMSG_LEFTALIGN, NULL, Items, static_cast<int>(MessageLines->Count),
      static_cast<int>(Buttons->Count));
  }
  ,
  {
    delete[] Items;
  }
  );

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::Message(unsigned int Flags,
  const UnicodeString Title, const UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  // DEBUG_PRINTF(L"Message = %s", Message.c_str());
  // when message is shown while some "custom" output is on screen,
  // make the output actually background of FAR screen
  if (FTerminalScreenShowing)
  {
    FarControl(FCTL_SETUSERSCREEN, 0, NULL);
  }

  intptr_t Result;
  if (Buttons != NULL)
  {
    TFarMessageParams DefaultParams;
    TFarMessageParams * AParams = (Params == NULL ? &DefaultParams : Params);
    Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
  }
  else
  {
    assert(Params == NULL);
    UnicodeString Items = Title + L"\n" + Message;
    TFarEnvGuard Guard;
    Result = FStartupInfo.Message(&MainGuid, &MainGuid,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      NULL,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::Menu(unsigned int Flags, UnicodeString Title,
  UnicodeString Bottom, const FarMenuItem * Items, int Count,
  const FarKey * BreakKeys, intptr_t & BreakCode)
{
  assert(Items);

  UnicodeString ATitle = Title;
  UnicodeString ABottom = Bottom;
  TFarEnvGuard Guard;
    return FStartupInfo.Menu(&MainGuid, &MainGuid,
      -1, -1, 0,
      Flags,
      ATitle.c_str(),
      ABottom.c_str(),
      NULL,
      BreakKeys,
      &BreakCode,
      Items,
      Count);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString Title,
  const UnicodeString Bottom, TStrings * Items, const FarKey * BreakKeys,
  intptr_t & BreakCode)
{
  assert(Items && Items->Count);
  intptr_t Result = 0;
  FarMenuItem * MenuItems = new FarMenuItem[Items->Count];
  TRY_FINALLY1 (MenuItems,
  {
    int Selected = NPOS;
    int Count = 0;
    for (int i = 0; i < Items->Count; i++)
    {
      size_t flags = reinterpret_cast<size_t>(Items->Objects[i]);
      if (FLAGCLEAR(Flags, MIF_HIDDEN))
      {
        memset(&MenuItems[Count], 0, sizeof(MenuItems[Count]));
        MenuItems[Count].Flags = flags;
        if (MenuItems[Count].Flags & MIF_SELECTED)
        {
          assert(Selected == NPOS);
          Selected = i;
        }
        MenuItems[Count].Text = Items->Strings[i].c_str();
        MenuItems[Count].UserData = i;
        Count++;
      }
    }

    intptr_t ResultItem = Menu(Flags, Title, Bottom,
      reinterpret_cast<const FarMenuItem *>(MenuItems), Count, BreakKeys, BreakCode);

    if (ResultItem >= 0)
    {
      Result = MenuItems[ResultItem].UserData;
      if (Selected >= 0)
      {
        Items->Objects[Selected] = reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->Objects[Selected]) & ~MIF_SELECTED);
      }
      Items->Objects[Result] = reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->Objects[Result]) | MIF_SELECTED);
    }
    else
    {
      Result = ResultItem;
    }
  }
  ,
  {
    delete[] MenuItems;
  }
  );
  return Result;
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString Title,
  const UnicodeString Bottom, TStrings * Items)
{
  intptr_t BreakCode;
  return Menu(Flags, Title, Bottom, Items, NULL, BreakCode);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::InputBox(const UnicodeString Title,
  const UnicodeString Prompt, UnicodeString & Text, PLUGINPANELITEMFLAGS Flags,
  const UnicodeString HistoryName, size_t MaxLen, TFarInputBoxValidateEvent OnValidate)
{
  bool Repeat = false;
  int Result = 0;
  do
  {
    UnicodeString DestText;
    DestText.SetLength(MaxLen + 1);
    HANDLE ScreenHandle = 0;
    SaveScreen(ScreenHandle);
    UnicodeString AText = Text;
    {
      TFarEnvGuard Guard;
      Result = FStartupInfo.InputBox(
                         &MainGuid,
                         &MainGuid,
        Title.c_str(),
        Prompt.c_str(),
        HistoryName.c_str(),
        AText.c_str(),
        const_cast<wchar_t *>(DestText.c_str()),
        MaxLen,
        NULL,
        FIB_ENABLEEMPTY | FIB_BUTTONS | Flags);
    }
    RestoreScreen(ScreenHandle);
    Repeat = false;
    if (Result)
    {
      Text = DestText.c_str();
      if (OnValidate)
      {
        try
        {
          OnValidate(Text);
        }
        catch (Exception & E)
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
void __fastcall TCustomFarPlugin::Text(int X, int Y, int Color, const UnicodeString Str)
{
  TFarEnvGuard Guard;
  FarColor color = {};
  color.Flags = FCF_FG_4BIT | FCF_BG_4BIT;
  color.ForegroundColor = Color; // LIGHTGRAY;
  color.BackgroundColor = 0;
  FStartupInfo.Text(X, Y, &color, Str.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::WriteConsole(const UnicodeString Str)
{
  unsigned long Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, NULL);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FarCopyToClipboard(const UnicodeString Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(FCT_STREAM, Str.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::FarCopyToClipboard(TStrings * Strings)
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
TPoint __fastcall TCustomFarPlugin::TerminalInfo(TPoint * Size, TPoint * Cursor)
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
HWND __fastcall TCustomFarPlugin::GetConsoleWindow()
{
  wchar_t Title[1024];
  GetConsoleTitle(Title, sizeof(Title) - 1);
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
void __fastcall TCustomFarPlugin::ToggleVideoMode()
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
void __fastcall TCustomFarPlugin::ScrollTerminalScreen(int Rows)
{
  TPoint Size = TerminalInfo();

  SMALL_RECT Source;
  COORD Dest;
  CHAR_INFO Fill;
  Source.Left = 0;
  Source.Top = static_cast<char>(Rows);
  Source.Right = static_cast<SHORT>(Size.x);
  Source.Bottom = static_cast<SHORT>(Size.y);
  Dest.X = 0;
  Dest.Y = 0;
  Fill.Char.UnicodeChar = L' ';
  Fill.Attributes = 7;
  ScrollConsoleScreenBuffer(FConsoleOutput, &Source, NULL, Dest, &Fill);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ShowTerminalScreen()
{
  assert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  UnicodeString Blank = ::StringOfChar(L' ', Size.x);
  // Blank.SetLength(static_cast<size_t>(Size.x));
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
void __fastcall TCustomFarPlugin::SaveTerminalScreen()
{
  FTerminalScreenShowing = false;
  FarControl(FCTL_SETUSERSCREEN, 0, NULL);
}
//---------------------------------------------------------------------------
class TConsoleTitleParam : public TObject
{
public:
  TConsoleTitleParam() :
    Progress(0),
    Own(0)
  {}
  short Progress;
  short Own;
};
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ShowConsoleTitle(const UnicodeString Title)
{
  wchar_t SaveTitle[1024];
  GetConsoleTitle(SaveTitle, sizeof(SaveTitle));
  TConsoleTitleParam * Param = new TConsoleTitleParam();
  Param->Progress = FCurrentProgress;
  Param->Own = !FCurrentTitle.IsEmpty() && (FormatConsoleTitle() == SaveTitle);
  if (Param->Own)
  {
    FSavedTitles->AddObject(FCurrentTitle, Param);
  }
  else
  {
    FSavedTitles->AddObject(SaveTitle, Param);
  }
  FCurrentTitle = Title;
  FCurrentProgress = -1;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ClearConsoleTitle()
{
  assert(FSavedTitles->Count > 0);
  UnicodeString Title = FSavedTitles->Strings[FSavedTitles->Count-1];
  TObject * Object = FSavedTitles->Objects[FSavedTitles->Count-1];
  TConsoleTitleParam * Param = dynamic_cast<TConsoleTitleParam *>(Object);
  if (Param->Own)
  {
    FCurrentTitle = Title;
    FCurrentProgress = Param->Progress;
    UpdateConsoleTitle();
  }
  else
  {
    FCurrentTitle = L"";
    FCurrentProgress = -1;
    SetConsoleTitle(Title.c_str());
    UpdateProgress(TBPS_NOPROGRESS, 0);
  }
  delete FSavedTitles->Objects(FSavedTitles->Count - 1);
  FSavedTitles->Delete(FSavedTitles->Count - 1);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString Title)
{
  // assert(!FCurrentTitle.IsEmpty());
  FCurrentTitle = Title;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
  // assert(!FCurrentTitle.IsEmpty());
  FCurrentProgress = Progress;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::FormatConsoleTitle()
{
  UnicodeString Title;
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
void __fastcall TCustomFarPlugin::UpdateProgress(int state, int progress)
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, state, NULL);
  if (state == TBPS_NORMAL)
  {
    ProgressValue pv;
    pv.StructSize = sizeof(ProgressValue);
    pv.Completed = progress < 0 ? 0 : progress > 100 ? 100 : progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, 0, &pv);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::UpdateConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? TBPS_NORMAL : TBPS_NOPROGRESS, progress);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  assert(!Screen);
  TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  assert(Screen);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  assert(Screen);
  TFarEnvGuard Guard;
  FStartupInfo.RestoreScreen(static_cast<HANDLE>(Screen));
  Screen = 0;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::HandleException(Exception * E, int /*OpMode*/)
{
  assert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", MB2W(E->what()));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::GetMsg(int MsgId)
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(&MainGuid, MsgId);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::CheckForEsc()
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
bool __fastcall TCustomFarPlugin::Viewer(UnicodeString FileName,
  const UnicodeString Title, unsigned int Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Viewer(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_DEFAULT);
  return Result > 0;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarPlugin::Editor(const UnicodeString FileName,
  const UnicodeString Title, unsigned int Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Editor(
                 FileName.c_str(),
                 Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
                 CP_DEFAULT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
__int64 __fastcall TCustomFarPlugin::GetSystemSetting(HANDLE & Settings, const wchar_t * Name)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), FSSF_SYSTEM, Name, FST_UNKNOWN, {0} };
  if (FStartupInfo.SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
  {
    return item.Number;
  }
  return 0;
}
//---------------------------------------------------------------------------
__int64 __fastcall TCustomFarPlugin::FarSystemSettings()
{
  if (!FValidFarSystemSettings)
  {
    FFarSystemSettings = 0;
    FarSettingsCreate settings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
    HANDLE Settings = FStartupInfo.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &settings) ? settings.Handle : 0;
    if (Settings)
    {
      if (GetSystemSetting(Settings, L"DeleteToRecycleBin"))
        FFarSystemSettings |= NBSS_DELETETORECYCLEBIN;

       FStartupInfo.SettingsControl(Settings, SCTL_FREE, 0, 0);
       FValidFarSystemSettings = true;
    }
  }
  return FFarSystemSettings;
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
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
__int64 __fastcall TCustomFarPlugin::FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(&MainGuid, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarPlugin::FarEditorControl(EDITOR_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  UnicodeString Buf;
  switch (Command)
  {
  case ECTL_GETINFO:
  case ECTL_SETPARAM:
  case ECTL_GETFILENAME:
    // noop
    break;

  case ECTL_SETTITLE:
    // Buf = static_cast<wchar_t *>(Param);
    // Param = Buf.c_str();
    break;

  default:
    // for other commands, OEM/ANSI conversion to be verified
    assert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.EditorControl(-1, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
TFarEditorInfo * __fastcall TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result;
  ::EditorInfo * Info = new ::EditorInfo;
  memset(Info, 0, sizeof(::EditorInfo));
  Info->StructSize = sizeof(::EditorInfo);
  try
  {
    if (FarEditorControl(ECTL_GETINFO, 0, Info))
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
int __fastcall TCustomFarPlugin::FarVersion()
{
  if (FFarVersion == 0)
  {
    FFarVersion = FarAdvControl(ACTL_GETFARMANAGERVERSION, 0);
  }
  return FFarVersion;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::FormatFarVersion(VersionInfo &Info)
{
  return FORMAT(L"%d.%d.%d", Info.Major, Info.Minor, Info.Build);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPlugin::TemporaryDir()
{
  UnicodeString Result;
  if (FTemporaryDir.IsEmpty())
  {
    Result.SetLength(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), Result.Length(), NULL);
    PackStr(Result);
    FTemporaryDir = Result;
  }
  else
  {
    Result = FTemporaryDir;
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * Rec)
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
//---------------------------------------------------------------------------
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
TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin * APlugin) :
  TObject(),
  FPlugin(APlugin),
  FClosed(false),
  FCriticalSection(NULL)
{
  Self = this;
  memset(FPanelInfo, 0, sizeof(FPanelInfo));
}

void __fastcall TCustomFarFileSystem::Init()
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
void __fastcall TCustomFarFileSystem::HandleException(Exception * E, int OpMode)
{
  DEBUG_PRINTF(L"before FPlugin->HandleException");
  FPlugin->HandleException(E, OpMode);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::Close()
{
  FClosed = true;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::InvalidateOpenPanelInfo()
{
  FOpenPanelInfoValid = false;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ClearOpenPanelInfo(OpenPanelInfo & Info)
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
  InvalidateOpenPanelInfo();
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::GetOpenPanelInfo(struct OpenPanelInfo * Info)
{
  ResetCachedInfo();
  if (FClosed)
  {
    // FAR WORKAROUND
    // if plugin is closed from ProcessPanelEvent(FE_IDLE), is does not close,
    // so we close it here on the very next opportunity
    ClosePanel();
  }
  else
  {
    if (!FOpenPanelInfoValid)
    {
      ClearOpenPanelInfo(FOpenPanelInfo);
      UnicodeString HostFile, CurDir, Format, PanelTitle, ShortcutData;
      {
        TFarPanelModes * PanelModes = NULL;
        std::auto_ptr<TFarPanelModes> PanelModesPtr(NULL);
        TFarKeyBarTitles * KeyBarTitles = NULL;
        std::auto_ptr<TFarKeyBarTitles> KeyBarTitlesPtr(NULL);
        PanelModes = new TFarPanelModes();
        PanelModesPtr.reset(PanelModes);
        KeyBarTitles = new TFarKeyBarTitles();
        KeyBarTitlesPtr.reset(KeyBarTitles);
        bool StartSortOrder = false;

        GetOpenPanelInfoEx(FOpenPanelInfo.Flags, HostFile, CurDir, Format,
          PanelTitle, PanelModes, FOpenPanelInfo.StartPanelMode,
          FOpenPanelInfo.StartSortMode, StartSortOrder, KeyBarTitles, ShortcutData);
 
        FOpenPanelInfo.HostFile = TCustomFarPlugin::DuplicateStr(HostFile);
        FOpenPanelInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplace(CurDir, L"\\", L"/", TReplaceFlags() << rfReplaceAll));

        FOpenPanelInfo.Format = TCustomFarPlugin::DuplicateStr(Format);
        FOpenPanelInfo.PanelTitle = TCustomFarPlugin::DuplicateStr(PanelTitle);
        // FOpenPanelInfo.StartPanelMode=L'4';
        PanelModes->FillOpenPanelInfo(&FOpenPanelInfo);
        // Info->StartSortMode = SM_NAME;
        // Info->StartSortOrder = 0;
 
        PanelModes->FillOpenPanelInfo(&FOpenPanelInfo);
        FOpenPanelInfo.StartSortOrder = StartSortOrder;
        KeyBarTitles->FillOpenPanelInfo(&FOpenPanelInfo);
        FOpenPanelInfo.ShortcutData = TCustomFarPlugin::DuplicateStr(ShortcutData);
      }

      FOpenPanelInfoValid = true;
    }

    memmove(Info, &FOpenPanelInfo, sizeof(FOpenPanelInfo));
  }
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFindData(struct GetFindDataInfo *Info)
{
  // DEBUG_PRINTF(L"begin");
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = new TObjectList();
  std::auto_ptr<TObjectList> PanelItemsPtr(PanelItems);
  {
    Result = !FClosed && GetFindDataEx(PanelItems, Info->OpMode);
    // DEBUG_PRINTF(L"Result = %d, PanelItems->Count = %d", Result, PanelItems->Count);
    if (Result && PanelItems->Count)
    {
      Info->PanelItem = new PluginPanelItem[PanelItems->Count];
      memset(Info->PanelItem, 0, PanelItems->Count * sizeof(PluginPanelItem));
      Info->ItemsNumber = PanelItems->Count;
      for (int Index = 0; Index < PanelItems->Count; Index++)
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
void __fastcall TCustomFarFileSystem::FreeFindData(const struct FreeFindDataInfo *Info)
{
  ResetCachedInfo();
  if (Info->PanelItem)
  {
    assert(Info->ItemsNumber > 0);
    for (int Index = 0; Index < Info->ItemsNumber; Index++)
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
int __fastcall TCustomFarFileSystem::ProcessHostFile(const struct ProcessHostFileInfo *Info)
{
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
  std::auto_ptr<TObjectList> PanelItemsPtr(PanelItems);
  {
    Result = ProcessHostFileEx(PanelItems, Info->OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
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
int __fastcall TCustomFarFileSystem::ProcessPanelEvent(int Event, void * Param)
{
  ResetCachedInfo();
  return ProcessPanelEventEx(Event, Param);
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::SetDirectory(const struct SetDirectoryInfo * Info)
{
  ResetCachedInfo();
  InvalidateOpenPanelInfo();
  int Result = SetDirectoryEx(Info->Dir, Info->OpMode);
  InvalidateOpenPanelInfo();
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::MakeDirectory(struct MakeDirectoryInfo *Info)
{
  ResetCachedInfo();
  FNameStr = Info->Name;
  intptr_t Result = 0;
  TRY_FINALLY2 (Self, Info,
  {
    Result = MakeDirectoryEx(Self->FNameStr, Info->OpMode);
  }
  ,
  {
    if (0 != wcscmp(Self->FNameStr.c_str(), Info->Name))
    {
      Info->Name = Self->FNameStr.c_str();
    }
  }
  );
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::DeleteFiles(const struct DeleteFilesInfo *Info)
{
  ResetCachedInfo();
  bool Result = false;
  TObjectList * PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
  std::auto_ptr<TObjectList> PanelItemsPtr(PanelItems);
  {
    Result = DeleteFilesEx(PanelItems, Info->OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFiles(struct GetFilesInfo * Info)
{
  ResetCachedInfo();
  TObjectList * PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
  intptr_t Result = 0;
  FDestPathStr = Info->DestPath;
  TRY_FINALLY3 (Self, Info, PanelItems,
  {
    Result = GetFilesEx(PanelItems, Info->Move > 0, FDestPathStr, Info->OpMode);
  }
  ,
  {
    if (Self->FDestPathStr != Info->DestPath)
    {
      Info->DestPath = Self->FDestPathStr.c_str();
    }
    delete PanelItems;

  }
  );

  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::PutFiles(const struct PutFilesInfo *Info)
{
  ResetCachedInfo();
  intptr_t Result = 0;
  TObjectList * PanelItems = CreatePanelItemList(Info->PanelItem, Info->ItemsNumber);
  std::auto_ptr<TObjectList> PanelItemsPtr(PanelItems);
  {
    Result = PutFilesEx(PanelItems, Info->Move > 0, Info->OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ResetCachedInfo()
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
TFarPanelInfo * __fastcall TCustomFarFileSystem::GetPanelInfo(int Another)
{
  // DEBUG_PRINTF(L"Another = %d", Another);
  bool another = Another != 0;
  if (FPanelInfo[another] == NULL)
  {
    PanelInfo * Info = new PanelInfo;
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
intptr_t __fastcall TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
intptr_t __fastcall TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  unsigned int PrevInstances = FInstances;
  InvalidateOpenPanelInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, NULL, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarFileSystem::ClosePanel()
{
  FClosed = true;
  FarControl(FCTL_CLOSEPANEL, 0, NULL);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarFileSystem::GetMsg(int MsgId)
{
  return FPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
TCustomFarFileSystem * __fastcall TCustomFarFileSystem::GetOppositeFileSystem()
{
  return FPlugin->GetPanelFileSystem(true, this);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsActiveFileSystem()
{
  // Cannot use PanelInfo::Focus as it occasionally does not work from editor;
  return (this == FPlugin->GetPanelFileSystem());
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsLeft()
{
  DEBUG_PRINTF(L"IsLeft");
  return (GetPanelInfo(0)->GetBounds().Left <= 0);
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::IsRight()
{
  return !IsLeft();
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessKeyEx(intptr_t /*Key*/, uintptr_t /*ControlState*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::ProcessPanelEventEx(int /*Event*/, void * /*Param*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::SetDirectoryEx(const UnicodeString /* Dir */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /* Name */, int /* OpMode */)
{
  return -1;
}
//---------------------------------------------------------------------------
bool __fastcall TCustomFarFileSystem::DeleteFilesEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
  UnicodeString & /*DestPath*/, int /*OpMode*/)
{
  return 0;
}
//---------------------------------------------------------------------------
int __fastcall TCustomFarFileSystem::PutFilesEx(TObjectList * /* PanelItems */, bool /* Move */, int /* OpMode */)
{
  return 0;
}
//---------------------------------------------------------------------------
TObjectList * __fastcall TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  // DEBUG_PRINTF(L"ItemsNumber = %d", ItemsNumber);
  TObjectList * PanelItems = new TObjectList();
  PanelItems->SetOwnsObjects(true);
  try
  {
    for (int Index = 0; Index < ItemsNumber; Index++)
    {
      PanelItems->Add(new TFarPanelItem(&PanelItem[Index], false));
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
void __fastcall TFarPanelModes::SetPanelMode(size_t Mode, const UnicodeString ColumnTypes,
  const UnicodeString ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, const UnicodeString StatusColumnTypes,
  const UnicodeString StatusColumnWidths)
{
  intptr_t ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  assert(Mode != NPOS && Mode < LENOF(FPanelModes));
  assert(!ColumnTitles || (ColumnTitles->Count == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  wchar_t ** Titles = new wchar_t *[ColumnTypesCount];
  FPanelModes[Mode].ColumnTypes = TCustomFarPlugin::DuplicateStr(ColumnTypes);
  FPanelModes[Mode].ColumnWidths = TCustomFarPlugin::DuplicateStr(ColumnWidths);
  if (ColumnTitles)
  {
    for (intptr_t Index = 0; Index < ColumnTypesCount; Index++)
    {
      Titles[Index] = TCustomFarPlugin::DuplicateStr(ColumnTitles->Strings[Index]);
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

  FPanelModes[Mode].StatusColumnTypes = TCustomFarPlugin::DuplicateStr(StatusColumnTypes);
  FPanelModes[Mode].StatusColumnWidths = TCustomFarPlugin::DuplicateStr(StatusColumnWidths);
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelModes::SetFlag(PANELMODE_FLAGS & Flags, bool value, PANELMODE_FLAGS Flag)
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
    intptr_t ColumnTypesCount = Mode.ColumnTypes ?
      CommaCount(UnicodeString(Mode.ColumnTypes)) + 1 : 0;

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
void __fastcall TFarPanelModes::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  assert(Info);
  Info->PanelModesNumber = LENOF(FPanelModes);
  PanelMode * PanelModesArray = new PanelMode[LENOF(FPanelModes)];
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelModes::CommaCount(const UnicodeString ColumnTypes)
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
void __fastcall TFarKeyBarTitles::ClearFileKeyBarTitles()
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
void __fastcall TFarKeyBarTitles::ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
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
void __fastcall TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  int FunctionKey, const UnicodeString Title)
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
  Labels[FunctionKey - 1].Text = TCustomFarPlugin::DuplicateStr(Title, true);
  Labels[FunctionKey-1].LongText = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (int Index = 0; Index < Titles.CountLabels; Index++)
  {
    delete[] Titles.Labels[Index].Text;
    delete[] Titles.Labels[Index].LongText;
  }
    delete[] Titles.Labels;
    Titles.Labels = NULL;
    Titles.CountLabels = 0;
}
//---------------------------------------------------------------------------
void __fastcall TFarKeyBarTitles::FillOpenPanelInfo(struct OpenPanelInfo *Info)
{
  assert(Info);
  KeyBarTitles * KeyBar = new KeyBarTitles;
  Info->KeyBar = KeyBar;
  memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
  FReferenced = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
UnicodeString __fastcall TCustomFarPanelItem::GetCustomColumnData(int /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
void __fastcall TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem * PanelItem)
{
  // DEBUG_PRINTF(L"begin");
  assert(PanelItem);

  UnicodeString FileName;
  __int64 Size = 0;
  TDateTime LastWriteTime;
  TDateTime LastAccess;
  UnicodeString Description;
  UnicodeString Owner;

  void * UserData = PanelItem->UserData.Data;
  GetData(PanelItem->Flags, FileName, Size, PanelItem->FileAttributes,
    LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
    UserData, PanelItem->CustomColumnNumber);
  PanelItem->UserData.Data = UserData;
  // DEBUG_PRINTF(L"LastWriteTime = %f, LastAccess = %f", LastWriteTime, LastAccess);
  FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
  FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
  PanelItem->CreationTime = FileTime;
  PanelItem->LastAccessTime = FileTimeA;
  PanelItem->LastWriteTime = FileTime;
  PanelItem->FileSize = Size;
  // PanelItem->PackSize = (long int)Size;

  // ASCOPY(PanelItem->FindData.lpwszFileName, FileName);
  PanelItem->FileName = TCustomFarPlugin::DuplicateStr(FileName);
  // DEBUG_PRINTF(L"PanelItem->FindData.lpwszFileName = %s", PanelItem->FindData.lpwszFileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  // PanelItem->CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
  wchar_t ** CustomColumnData = new wchar_t *[PanelItem->CustomColumnNumber];
  for (int Index = 0; Index < PanelItem->CustomColumnNumber; Index++)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
  // DEBUG_PRINTF(L"end");
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem):
  TCustomFarPanelItem(),
  FPanelItem(NULL),
  FOwnsItem(false)
{
  assert(APanelItem);
  FPanelItem = APanelItem;
  FOwnsItem = OwnsItem;
}

TFarPanelItem::~TFarPanelItem()
{
  if (FOwnsItem)
    free(FPanelItem);
  FPanelItem = NULL;
}

//---------------------------------------------------------------------------
void __fastcall TFarPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & /*FileName*/, __int64 & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelItem::GetCustomColumnData(int /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
PLUGINPANELITEMFLAGS __fastcall TFarPanelItem::GetFlags()
{
  return FPanelItem->Flags;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelItem::GetFileName()
{
  UnicodeString Result = FPanelItem->FileName;
  return Result;
}
//---------------------------------------------------------------------------
void * __fastcall TFarPanelItem::GetUserData()
{
  return FPanelItem->UserData.Data;
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetSelected()
{
  return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelItem::SetSelected(bool value)
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
uintptr_t __fastcall TFarPanelItem::GetFileAttributes()
{
  return FPanelItem->FileAttributes;
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetIsParentDirectory()
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool __fastcall TFarPanelItem::GetIsFile()
{
  return (GetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
THintPanelItem::THintPanelItem(const UnicodeString AHint) :
  TCustomFarPanelItem()
{
  FHint = AHint;
}
//---------------------------------------------------------------------------
void __fastcall THintPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & FileName, __int64 & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  FileName = FHint;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner):
  TObject(),
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
int __fastcall TFarPanelInfo::GetItemCount()
{
  return FPanelInfo->ItemsNumber;
}
//---------------------------------------------------------------------------
TRect __fastcall TFarPanelInfo::GetBounds()
{
  RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelInfo::GetSelectedCount()
{
  intptr_t Count = FPanelInfo->SelectedItemsNumber;

  if ((Count == 1) && FOwner)
  {
    DWORD size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, NULL);
    // DEBUG_PRINTF(L"size1 = %d, sizeof(PluginPanelItem) = %d", size, sizeof(PluginPanelItem));
    PluginPanelItem * ppi = static_cast<PluginPanelItem *>(malloc(size));
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
TObjectList * __fastcall TFarPanelInfo::GetItems()
{
  if (!FItems)
  {
    FItems = new TObjectList();
  }
  // DEBUG_PRINTF(L"FPanelInfo->ItemsNumber = %d", FPanelInfo->ItemsNumber);
  if (FOwner)
  {
    for (int Index = 0; Index < FPanelInfo->ItemsNumber; Index++)
    {
      // DEBUG_PRINTF(L"Index = %d", Index);
      // TODO: move to common function
      intptr_t size = FOwner->FarControl(FCTL_GETPANELITEM, Index, NULL);
      PluginPanelItem * ppi = static_cast<PluginPanelItem *>(malloc(size));
      memset(ppi, 0, size);
      FarGetPluginPanelItem gppi;
      gppi.StructSize = sizeof(FarGetPluginPanelItem);
      gppi.Size = size;
      gppi.Item = ppi;
      FOwner->FarControl(FCTL_GETPANELITEM, Index, static_cast<void *>(&gppi));
      // DEBUG_PRINTF(L"ppi.FileName = %s", ppi->FileName);
      FItems->Add(new TFarPanelItem(ppi, true));
    }
  }
  return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem * __fastcall TFarPanelInfo::FindFileName(const UnicodeString FileName)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (int Index = 0; Index < AItems->Count; Index++)
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
TFarPanelItem * __fastcall TFarPanelInfo::FindUserData(void * UserData)
{
  TObjectList * AItems = GetItems();
  TFarPanelItem * PanelItem;
  for (int Index = 0; Index < AItems->Count; Index++)
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
void __fastcall TFarPanelInfo::ApplySelection()
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<void *>(FPanelInfo));
}
//---------------------------------------------------------------------------
TFarPanelItem * __fastcall TFarPanelInfo::GetFocusedItem()
{
  intptr_t Index = GetFocusedIndex();
  TObjectList * Items = GetItems();
  // DEBUG_PRINTF(L"Index = %d, Items = %x, Items->Count = %d", Index, Items, Items->Count);
  if (Items->Count > 0)
  {
    assert(Index < Items->Count);
    return static_cast<TFarPanelItem *>(Items->GetItem(Index));
  }
  else
  {
    return NULL;
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelInfo::SetFocusedItem(TFarPanelItem * value)
{
  TObjectList * Items = GetItems();
  int Index = Items->IndexOf(static_cast<TObject *>(value));
  assert(Index != NPOS);
  SetFocusedIndex(Index);
  // delete Items;
}
//---------------------------------------------------------------------------
int __fastcall TFarPanelInfo::GetFocusedIndex()
{
  return FPanelInfo->CurrentItem;
}
//---------------------------------------------------------------------------
void __fastcall TFarPanelInfo::SetFocusedIndex(int value)
{
  // for "another panel info", there's no owner
  assert(FOwner != NULL);
  // DEBUG_PRINTF(L"GetFocusedIndex = %d, value = %d", GetFocusedIndex(), value);
  if (GetFocusedIndex() != value)
  {
    assert(value != NPOS && value < FPanelInfo->ItemsNumber);
    FPanelInfo->CurrentItem = value;
    PanelRedrawInfo PanelInfo;
    PanelInfo.StructSize = sizeof(PanelRedrawInfo);
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<void *>(&PanelInfo));
  }
}
//---------------------------------------------------------------------------
TFarPanelType __fastcall TFarPanelInfo::GetType()
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
bool __fastcall TFarPanelInfo::GetIsPlugin()
{
  return ((FPanelInfo->PluginHandle != INVALID_HANDLE_VALUE) && (FPanelInfo->PluginHandle != 0));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarPanelInfo::GetCurrentDirectory()
{
  UnicodeString Result = L"";
  intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
    0,
    NULL,
    FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    FarPanelDirectory * pfpd = static_cast<FarPanelDirectory *>(malloc(Size));

    FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
      Size,      
      pfpd,
      FOwner != NULL ? PANEL_ACTIVE : PANEL_PASSIVE);
    Result = pfpd->Name;
    free(pfpd);
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarMenuItems::TFarMenuItems() :
  TStringList()
{
  FItemFocused = NPOS;
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::Clear()
{
  FItemFocused = NPOS;
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::Delete(intptr_t Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = NPOS;
  }
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::PutObject(intptr_t Index, TObject * AObject)
{
  TStringList::PutObject(Index, AObject);
  bool Focused = (reinterpret_cast<uintptr_t>(AObject) & MIF_SEPARATOR) != 0;
  if ((Index == GetItemFocused()) && !Focused)
  {
    FItemFocused = NPOS;
  }
  if (Focused)
  {
    if (GetItemFocused() != NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Index;
  }
}
//---------------------------------------------------------------------------
int __fastcall TFarMenuItems::Add(const UnicodeString Text, bool Visible)
{
  int Result = TStringList::Add(Text);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::AddSeparator(bool Visible)
{
  Add(L"");
  SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::SetItemFocused(intptr_t Value)
{
  if (GetItemFocused() != Value)
  {
    if (GetItemFocused() != NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Value;
    SetFlag(GetItemFocused(), MIF_SELECTED, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TFarMenuItems::SetFlag(intptr_t Index, uintptr_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    uintptr_t F = reinterpret_cast<uintptr_t>(Objects[Index]);
    if (Value)
    {
      F |= Flag;
    }
    else
    {
      F &= ~Flag;
    }
    PutObject(Index, reinterpret_cast<TObject *>(F));
  }
}
//---------------------------------------------------------------------------
bool __fastcall TFarMenuItems::GetFlag(intptr_t Index, uintptr_t Flag)
{
  return (reinterpret_cast<uintptr_t>(Objects[Index]) & Flag) > 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEditorInfo::TFarEditorInfo(EditorInfo * Info) :
  FEditorInfo(Info)
{
}
//---------------------------------------------------------------------------
TFarEditorInfo::~TFarEditorInfo()
{
  delete FEditorInfo;
}
//---------------------------------------------------------------------------
int __fastcall TFarEditorInfo::GetEditorID() const
{
  return FEditorInfo->EditorID;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TFarEditorInfo::GetFileName()
{
  UnicodeString Result = L"";
  intptr_t buffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, 0, NULL);
  if (buffLen)
  {
    Result.SetLength(buffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, buffLen, (wchar_t *)Result.c_str());
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
  assert(FarPlugin != NULL);
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
}
//---------------------------------------------------------------------------
TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
  assert(FarPlugin != NULL);
}
//---------------------------------------------------------------------------
void __fastcall FarWrapText(const UnicodeString Text, TStrings * Result, intptr_t MaxWidth)
{
  size_t TabSize = 8;
  TStringList Lines;
  Lines.Text = Text;
  TStringList WrappedLines;
  for (intptr_t Index = 0; Index < Lines.Count; Index++)
  {
    UnicodeString WrappedLine = Lines.Strings[Index];
    if (!WrappedLine.IsEmpty())
    {
      WrappedLine = ::ReplaceChar(WrappedLine, '\'', '\3');
      WrappedLine = ::ReplaceChar(WrappedLine, '\"', '\4');
      WrappedLine = Sysutils::WrapText(WrappedLine, MaxWidth);
      WrappedLine = ::ReplaceChar(WrappedLine, '\3', '\'');
      WrappedLine = ::ReplaceChar(WrappedLine, '\4', '\"');
      WrappedLines.Text = WrappedLine;
      for (int WrappedIndex = 0; WrappedIndex < WrappedLines.Count; WrappedIndex++)
      {
        UnicodeString FullLine = WrappedLines.Strings[WrappedIndex];
        do
        {
          // WrapText does not wrap when not possible, enforce it
          // (it also does not wrap when the line is longer than maximum only
          // because of trailing dot or similar)
          UnicodeString Line = FullLine.SubString(1, MaxWidth);
          FullLine.Delete(1, MaxWidth);

          intptr_t P = 0;
          while ((P = Line.Pos(L'\t')) > 0)
          {
            Line.Delete(P, 1);
            Line.Insert(::StringOfChar(' ',
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
      Result->Add(L"");
    }
  }
}
