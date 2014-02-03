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
#include "plugin_version.hpp"

//---------------------------------------------------------------------------
TCustomFarPlugin * FarPlugin = nullptr;
#define FAR_TITLE_SUFFIX L" - Far"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TFarMessageParams::TFarMessageParams()
{
  MoreMessages = nullptr;
  CheckBox = false;
  Timer = 0;
  TimerAnswer = 0;
  TimerEvent = nullptr;
  Timeout = 0;
  TimeoutButton = 0;
  ClickEvent = nullptr;
  Token = nullptr;
}
//---------------------------------------------------------------------------
TCustomFarPlugin::TCustomFarPlugin(HINSTANCE HInst) :
  TObject(),
  FCriticalSection(new TCriticalSection()),
  FOpenedPlugins(new TObjectList()),
  FSavedTitles(new TStringList())
{
  InitPlatformId();
  FFarThread = GetCurrentThreadId();
  FHandle = HInst;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FOpenedPlugins->SetOwnsObjects(false);
  FCurrentProgress = -1;
  FTopDialog = nullptr;
  FValidFarSystemSettings = false;

  ClearStruct(FPluginInfo);
  ClearPluginInfo(FPluginInfo);

  // far\Examples\Compare\compare.cpp
  FConsoleInput = ::CreateFile(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr,
    OPEN_EXISTING, 0, nullptr);
  FConsoleOutput = ::CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
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
  assert(FTopDialog == nullptr);

  ResetCachedInfo();
  ::CloseHandle(FConsoleInput);
  FConsoleInput = INVALID_HANDLE_VALUE;
  ::CloseHandle(FConsoleOutput);
  FConsoleOutput = INVALID_HANDLE_VALUE;

  ClearPluginInfo(FPluginInfo);
  assert(FOpenedPlugins->GetCount() == 0);
  delete FOpenedPlugins;
  for (intptr_t I = 0; I < FSavedTitles->GetCount(); I++)
    delete FSavedTitles->GetObject(I);
  delete FSavedTitles;
  delete FCriticalSection;
  delete GetGlobalFunctions();
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
void TCustomFarPlugin::SetStartupInfo(const struct PluginStartupInfo * Info)
{
  try
  {
    ResetCachedInfo();
    ClearStruct(FStartupInfo);
    memmove(&FStartupInfo, Info,
            Info->StructSize >= static_cast<intptr_t>(sizeof(FStartupInfo)) ?
            sizeof(FStartupInfo) : static_cast<size_t>(Info->StructSize));
    // the minimum we really need
    assert(FStartupInfo.GetMsg != nullptr);
    assert(FStartupInfo.Message != nullptr);

    ClearStruct(FFarStandardFunctions);
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
void TCustomFarPlugin::GetPluginInfo(struct PluginInfo * Info)
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
        if (NAME.GetCount()) \
        { \
          wchar_t ** StringArray = static_cast<wchar_t **>(nb_malloc(sizeof(wchar_t *) * NAME.GetCount())); \
          GUID *Guids = static_cast<GUID *>(nb_malloc(sizeof(GUID) * NAME.GetCount())); \
          FPluginInfo.NAME.Guids = Guids; \
          FPluginInfo.NAME.Strings = StringArray; \
          FPluginInfo.NAME.Count = NAME.GetCount(); \
          for (intptr_t Index = 0; Index < NAME.GetCount(); Index++) \
          { \
            StringArray[Index] = DuplicateStr(NAME.GetString(Index)); \
            Guids[Index] = *reinterpret_cast<const GUID *>(NAME.GetObject(Index)); \
          } \
        }

    COMPOSESTRINGARRAY(DiskMenu);
    COMPOSESTRINGARRAY(PluginMenu);
    COMPOSESTRINGARRAY(PluginConfig);

    #undef COMPOSESTRINGARRAY
    UnicodeString CommandPrefix;
    for (intptr_t Index = 0; Index < CommandPrefixes.GetCount(); ++Index)
    {
      CommandPrefix = CommandPrefix + (CommandPrefix.IsEmpty() ? L"" : L":") +
                      CommandPrefixes.GetString(Index);
    }
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
UnicodeString TCustomFarPlugin::GetModuleName() const
{
  return FStartupInfo.ModuleName;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ClearPluginInfo(PluginInfo & Info)
{
  if (Info.StructSize)
  {
    #define FREESTRINGARRAY(NAME) \
      for (intptr_t Index = 0; Index < Info.NAME.Count; ++Index) \
      { \
        nb_free((void*)Info.NAME.Strings[Index]); \
      } \
      nb_free((void*)Info.NAME.Strings); \
      nb_free((void*)Info.NAME.Guids); \
      Info.NAME.Strings = nullptr;

      FREESTRINGARRAY(DiskMenu);
      FREESTRINGARRAY(PluginMenu);
      FREESTRINGARRAY(PluginConfig);

      #undef FREESTRINGARRAY

      nb_free((void*)Info.CommandPrefix);
  }
  ClearStruct(Info);
  Info.StructSize = sizeof(Info);
}
//---------------------------------------------------------------------------
wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString & Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return nullptr;
  }
  else
  {
    const size_t sz = Str.Length() + 1;
    wchar_t * Result = static_cast<wchar_t *>(
      nb_malloc(sizeof(wchar_t) * sz));
    wcscpy_s(Result, sz, Str.c_str());
    return Result;
  }
}
//---------------------------------------------------------------------------
RECT TCustomFarPlugin::GetPanelBounds(HANDLE PanelHandle)
{
  PanelInfo Info;
  ClearStruct(Info);
  Info.StructSize = sizeof(PanelInfo);
  FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(&Info), PanelHandle);

  RECT Bounds;
  ClearStruct(Bounds);
  if (Info.PluginHandle)
  {
    Bounds = Info.PanelRect;
  }
  return Bounds;
}

//---------------------------------------------------------------------------
TCustomFarFileSystem * TCustomFarPlugin::GetPanelFileSystem(bool Another,
    HANDLE /* Plugin */)
{
  TCustomFarFileSystem * Result = nullptr;
  RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  TCustomFarFileSystem * FileSystem = nullptr;
  intptr_t Index = 0;
  while (!Result && (Index < FOpenedPlugins->GetCount()))
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
    ++Index;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::InvalidateOpenPanelInfo()
{
  for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
  {
    TCustomFarFileSystem * FileSystem =
      dynamic_cast<TCustomFarFileSystem *>(FOpenedPlugins->GetItem(Index));
    FileSystem->InvalidateOpenPanelInfo();
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Configure(const struct ConfigureInfo *Info)
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
    return 0;
  }
}
//---------------------------------------------------------------------------
void * TCustomFarPlugin::OpenPlugin(const struct OpenInfo *Info)
{
#ifdef USE_DLMALLOC
  // dlmallopt(M_GRANULARITY, 128 * 1024);
#endif

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
void TCustomFarPlugin::ClosePanel(void * Plugin)
{
  try
  {
    ResetCachedInfo();
    TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    SCOPE_EXIT
    {
      FOpenedPlugins->Remove(FileSystem);
    };
    {
      {
        TGuard Guard(FileSystem->GetCriticalSection());
        FileSystem->Close();
      }
    }
    delete FileSystem;
#ifdef USE_DLMALLOC
    // dlmalloc_trim(0); // 64 * 1024);
#endif
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleException");
    HandleException(&E);
  }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FileSystem, Exception * E, int OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API functions that must update
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
void TCustomFarPlugin::GetOpenPanelInfo(struct OpenPanelInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    assert(FOpenedPlugins->IndexOf(FileSystem) != NPOS);
    TGuard Guard(FileSystem->GetCriticalSection());
    FileSystem->GetOpenPanelInfo(Info);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF(L"before HandleFileSystemException");
    HandleFileSystemException(FileSystem, &E);
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::GetFindData(struct GetFindDataInfo *Info)
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
void TCustomFarPlugin::FreeFindData(const struct FreeFindDataInfo *Info)
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
intptr_t TCustomFarPlugin::ProcessHostFile(const struct ProcessHostFileInfo *Info)
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
intptr_t TCustomFarPlugin::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
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
intptr_t TCustomFarPlugin::ProcessPanelEvent(const struct ProcessPanelEventInfo *Info)
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

      TGuard Guard(FileSystem->GetCriticalSection());
      return FileSystem->ProcessPanelEvent(Info->Event, Param);
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
    return Info->Event == FE_COMMAND ? true : false;
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::SetDirectory(const struct SetDirectoryInfo *Info)
{
  TCustomFarFileSystem * FileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  UnicodeString PrevCurrentDirectory = FileSystem->GetCurrentDirectory();
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
    if (FileSystem->FOpenPanelInfoValid){
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
        catch (Exception & E)
        {
          (void)E;
          return 0;
        }
      }
    }
    return 0;
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MakeDirectory(struct MakeDirectoryInfo *Info)
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
intptr_t TCustomFarPlugin::DeleteFiles(const struct DeleteFilesInfo *Info)
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
intptr_t TCustomFarPlugin::GetFiles(struct GetFilesInfo *Info)
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
intptr_t TCustomFarPlugin::PutFiles(const struct PutFilesInfo *Info)
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
intptr_t TCustomFarPlugin::ProcessEditorEvent(const struct ProcessEditorEventInfo *Info)
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
intptr_t TCustomFarPlugin::ProcessEditorInput(const struct ProcessEditorInputInfo *Info)
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
intptr_t TCustomFarPlugin::MaxMessageLines()
{
  return static_cast<intptr_t>(TerminalInfo().y - 5);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MaxMenuItemLength()
{
  // got from maximal length of path in FAR's folders history
  return static_cast<intptr_t>(TerminalInfo().x - 13);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::MaxLength(TStrings * Strings)
{
  intptr_t Result = 0;
  for (intptr_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    if (Result < Strings->GetString(Index).Length())
    {
      Result = Strings->GetString(Index).Length();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TFarMessageDialog : public TFarDialog
{
public:
  explicit TFarMessageDialog(TCustomFarPlugin * Plugin,
    TFarMessageParams * Params);
  void Init(uintptr_t AFlags,
    const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons);

  intptr_t Execute(bool & ACheckBox);

protected:
  virtual void Change();
  virtual void Idle();

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
  FTimeoutButton(nullptr),
  FCheckBox(nullptr)
{
  assert(Params != nullptr);
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Init(uintptr_t AFlags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons)
{
  assert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  assert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME assert(FLAGCLEAR(AFlags, FMSG_DOWN));
  assert(FLAGCLEAR(AFlags, FMSG_ALLINONE));

  std::unique_ptr<TStrings> MessageLines(new TStringList());
  FarWrapText(Message, MessageLines.get(), MaxMessageWidth);
  intptr_t MaxLen = GetFarPlugin()->MaxLength(MessageLines.get());
  TStrings * MoreMessageLines = nullptr;
  std::unique_ptr<TStrings> MoreMessageLinesPtr(nullptr);
  if (FParams->MoreMessages != nullptr)
  {
    MoreMessageLines = new TStringList();
    MoreMessageLinesPtr.reset(MoreMessageLines);
    UnicodeString MoreMessages = FParams->MoreMessages->GetText();
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
  SetSize(TPoint(MaxMessageWidth, 10));
  SetCaption(Title);
  SetFlags(GetFlags() |
           FLAGMASK(FLAGSET(AFlags, FMSG_WARNING), FDLG_WARNING));

  for (intptr_t Index = 0; Index < MessageLines->GetCount(); ++Index)
  {
    TFarText * Text = new TFarText(this);
    Text->SetCaption(MessageLines->GetString(Index));
  }

  TFarLister * MoreMessagesLister = nullptr;
  TFarSeparator * MoreMessagesSeparator = nullptr;

  if (FParams->MoreMessages != nullptr)
  {
    new TFarSeparator(this);

    MoreMessagesLister = new TFarLister(this);
    MoreMessagesLister->GetItems()->Assign(MoreMessageLines);
    MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

    MoreMessagesSeparator = new TFarSeparator(this);
  }

  int ButtonOffset = (FParams->CheckBoxLabel.IsEmpty() ? -1 : -2);
  int ButtonLines = 1;
  TFarButton * Button = nullptr;
  FTimeoutButton = nullptr;
  for (intptr_t Index = 0; Index < Buttons->GetCount(); ++Index)
  {
    TFarButton * PrevButton = Button;
    Button = new TFarButton(this);
    Button->SetDefault(Index == 0);
    Button->SetBrackets(brNone);
    Button->SetOnClick(MAKE_CALLBACK(TFarMessageDialog::ButtonClick, this));
    UnicodeString Caption = Buttons->GetString(Index);
    if ((FParams->Timeout > 0) &&
        (FParams->TimeoutButton == static_cast<size_t>(Index)))
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
    Button->SetTag(reinterpret_cast<intptr_t>(Buttons->GetObject(Index)));
    if (PrevButton != nullptr)
    {
      Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
    }

    if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
    {
      for (intptr_t PIndex = 0; PIndex < GetItemCount(); ++PIndex)
      {
        TFarButton * PrevButton = dynamic_cast<TFarButton *>(GetItem(PIndex));
        if ((PrevButton != nullptr) && (PrevButton != Button))
        {
          PrevButton->Move(0, -1);
        }
      }
      Button->Move(-(Button->GetLeft() - GetBorderBox()->GetLeft()), 0);
      ButtonLines++;
    }

    if (MaxLen < Button->GetRight() - GetBorderBox()->GetLeft())
    {
      MaxLen = static_cast<intptr_t>(Button->GetRight() - GetBorderBox()->GetLeft() + 2);
    }

    SetNextItemPosition(ipRight);
  }

  if (!FParams->CheckBoxLabel.IsEmpty())
  {
    SetNextItemPosition(ipNewLine);
    FCheckBox = new TFarCheckBox(this);
    FCheckBox->SetCaption(FParams->CheckBoxLabel);

    if (MaxLen < FCheckBox->GetRight() - GetBorderBox()->GetLeft())
    {
      MaxLen = static_cast<intptr_t>(FCheckBox->GetRight() - GetBorderBox()->GetLeft());
    }
  }
  else
  {
    FCheckBox = nullptr;
  }

  TRect rect = GetClientRect();
  TPoint S(
    static_cast<int>(rect.Left + MaxLen - rect.Right),
    static_cast<int>(rect.Top + MessageLines->GetCount() +
    (FParams->MoreMessages != nullptr ? 1 : 0) + ButtonLines +
    (!FParams->CheckBoxLabel.IsEmpty() ? 1 : 0) +
    (-(rect.Bottom + 1))));

  if (FParams->MoreMessages != nullptr)
  {
    intptr_t MoreMessageHeight = static_cast<intptr_t>(GetFarPlugin()->TerminalInfo().y - S.y - 1);
    assert(MoreMessagesLister != nullptr);
    if (MoreMessageHeight > MoreMessagesLister->GetItems()->GetCount())
    {
      MoreMessageHeight = MoreMessagesLister->GetItems()->GetCount();
    }
    MoreMessagesLister->SetHeight(MoreMessageHeight);
    MoreMessagesLister->SetRight(
      GetBorderBox()->GetRight() - (MoreMessagesLister->GetScrollBar() ? 0 : 1));
    MoreMessagesLister->SetTabStop(MoreMessagesLister->GetScrollBar());
    assert(MoreMessagesSeparator != nullptr);
    MoreMessagesSeparator->SetPosition(
      MoreMessagesLister->GetTop() + MoreMessagesLister->GetHeight());
    S.y += static_cast<int>(MoreMessagesLister->GetHeight()) + 1;
  }
  SetSize(S);
}

//---------------------------------------------------------------------------
void TFarMessageDialog::Idle()
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
      assert(FTimeoutButton != nullptr);
      Close(FTimeoutButton);
    }
    else
    {
      UnicodeString Caption =
        FORMAT(L" %s ", ::Format(FParams->TimeoutStr.c_str(),
                               FTimeoutButtonCaption.c_str(), static_cast<int>((FParams->Timeout - Running) / 1000)).c_str()).c_str();
      intptr_t sz = FTimeoutButton->GetCaption().Length() > Caption.Length() ? FTimeoutButton->GetCaption().Length() - Caption.Length() : 0;
      Caption += ::StringOfChar(L' ', sz);
      FTimeoutButton->SetCaption(Caption);
    }
  }
}
//---------------------------------------------------------------------------
void TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != nullptr)
  {
    if ((FCheckBox != nullptr) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
      {
        TFarButton * Button = dynamic_cast<TFarButton *>(GetItem(Index));
        if ((Button != nullptr) && (Button->GetTag() == 0))
        {
          Button->SetEnabled(!FCheckBox->GetChecked());
        }
      }
      FCheckBoxChecked = FCheckBox->GetChecked();
    }
  }
}
//---------------------------------------------------------------------------
intptr_t TFarMessageDialog::Execute(bool & ACheckBox)
{
  FStartTime = Now();
  FLastTimerTime = FStartTime;
  FCheckBoxChecked = !ACheckBox;
  if (FCheckBox != nullptr)
  {
    FCheckBox->SetChecked(ACheckBox);
  }

  intptr_t Result = ShowModal();
  assert(Result != 0);
  if (Result > 0)
  {
    if (FCheckBox != nullptr)
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
intptr_t TCustomFarPlugin::DialogMessage(unsigned int Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  intptr_t Result;
  std::unique_ptr<TFarMessageDialog> Dialog(new TFarMessageDialog(this, Params));
  Dialog->Init(Flags, Title, Message, Buttons);
  Result = Dialog->Execute(Params->CheckBox);
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarMessage(unsigned int Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  assert(Params != nullptr);

  intptr_t Result;
  TStringList * MessageLines = nullptr;
  std::unique_ptr<TStrings> MessageLinesPtr(nullptr);
  wchar_t ** Items = nullptr;
  SCOPE_EXIT
  {
    nb_free(Items);
  };
  {
    UnicodeString FullMessage = Message;
    if (Params->MoreMessages != nullptr)
    {
      FullMessage += UnicodeString(L"\n\x01\n") + Params->MoreMessages->GetText();
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
    intptr_t MaxLines = MaxMessageLines();
    while (MessageLines->GetCount() > MaxLines)
    {
      MessageLines->Delete(MessageLines->GetCount() - 1);
    }

    for (intptr_t Index = 0; Index < Buttons->GetCount(); ++Index)
    {
      MessageLines->Add(Buttons->GetString(Index));
    }

    Items = static_cast<wchar_t **>(
      nb_malloc(sizeof(wchar_t *) * MessageLines->GetCount()));
    for (intptr_t Index = 0; Index < MessageLines->GetCount(); ++Index)
    {
      UnicodeString S = MessageLines->GetString(Index);
      MessageLines->SetString(Index, UnicodeString(S));
      Items[Index] = const_cast<wchar_t *>(MessageLines->GetString(Index).c_str());
    }

    TFarEnvGuard Guard;
    Result = static_cast<intptr_t>(FStartupInfo.Message(&MainGuid, &MainGuid,
      Flags | FMSG_LEFTALIGN, nullptr, Items, static_cast<int>(MessageLines->GetCount()),
      static_cast<int>(Buttons->GetCount())));
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Message(uintptr_t Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  // when message is shown while some "custom" output is on screen,
  // make the output actually background of FAR screen
  if (FTerminalScreenShowing)
  {
    FarControl(FCTL_SETUSERSCREEN, 0, 0);
  }

  intptr_t Result;
  if (Buttons != nullptr)
  {
    TFarMessageParams DefaultParams;
    TFarMessageParams * AParams = (Params == nullptr ? &DefaultParams : Params);
    Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
  }
  else
  {
    assert(Params == nullptr);
    UnicodeString Items = Title + L"\n" + Message;
    TFarEnvGuard Guard;
    Result = static_cast<intptr_t>(FStartupInfo.Message(&MainGuid, &MainGuid,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      nullptr,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0));
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, const FarMenuItem * Items, intptr_t Count,
  const FarKey * BreakKeys, intptr_t & BreakCode)
{
  assert(Items);

  TFarEnvGuard Guard;
    return FStartupInfo.Menu(&MainGuid, &MainGuid,
      -1, -1, 0,
      Flags,
      Title.c_str(),
      Bottom.c_str(),
      nullptr,
      BreakKeys,
      &BreakCode,
      Items,
      Count);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items, const FarKey * BreakKeys,
  intptr_t & BreakCode)
{
  assert(Items && Items->GetCount());
  intptr_t Result = 0;
  FarMenuItem * MenuItems = static_cast<FarMenuItem *>(
    nb_malloc(sizeof(FarMenuItem) * Items->GetCount()));
  SCOPE_EXIT
  {
    nb_free(MenuItems);
  };
  {
    intptr_t Selected = NPOS;
    intptr_t Count = 0;
    for (intptr_t I = 0; I < Items->GetCount(); ++I)
    {
      uintptr_t Flags = reinterpret_cast<uintptr_t>(Items->GetObject(I));
      if (FLAGCLEAR(Flags, MIF_HIDDEN))
      {
        ClearStruct(MenuItems[Count]);
        MenuItems[Count].Flags = static_cast<DWORD>(Flags);
        if (MenuItems[Count].Flags & MIF_SELECTED)
        {
          assert(Selected == NPOS);
          Selected = I;
        }
        MenuItems[Count].Text = Items->GetString(I).c_str();
        MenuItems[Count].UserData = I;
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
        Items->SetObject(Selected, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->GetObject(Selected)) & ~MIF_SELECTED));
      }
      Items->SetObject(Result, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->GetObject(Result)) | MIF_SELECTED));
    }
    else
    {
      Result = ResultItem;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::Menu(unsigned int Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items)
{
  intptr_t BreakCode;
  return Menu(Flags, Title, Bottom, Items, nullptr, BreakCode);
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::InputBox(const UnicodeString & Title,
  const UnicodeString & Prompt, UnicodeString & Text, PLUGINPANELITEMFLAGS Flags,
  const UnicodeString & HistoryName, intptr_t MaxLen, TFarInputBoxValidateEvent OnValidate)
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
        static_cast<int>(MaxLen),
        nullptr,
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
void TCustomFarPlugin::Text(int X, int Y, int Color, const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FarColor color = {};
  color.Flags = FCF_FG_4BIT | FCF_BG_4BIT;
  color.ForegroundColor = Color; // LIGHTGRAY;
  color.BackgroundColor = 0;
  FStartupInfo.Text(X, Y, &color, Str.c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, nullptr);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::WriteConsole(const UnicodeString & Str)
{
  unsigned long Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, nullptr);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(FCT_STREAM, Str.c_str());
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::FarCopyToClipboard(TStrings * Strings)
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
TPoint TCustomFarPlugin::TerminalInfo(TPoint * Size, TPoint * Cursor) const
{
  CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
  ClearStruct(BufferInfo);
  GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);
  if (FarPlugin)
    FarAdvControl(ACTL_GETFARRECT, 0, &BufferInfo.srWindow);

  TPoint Result(
    BufferInfo.srWindow.Right - BufferInfo.srWindow.Left + 1,
    BufferInfo.srWindow.Bottom - BufferInfo.srWindow.Top + 1);

  if (Size != nullptr)
  {
    *Size = Result;
  }

  if (Cursor != nullptr)
  {
    Cursor->x = BufferInfo.dwCursorPosition.X - BufferInfo.srWindow.Left;
    Cursor->y = BufferInfo.dwCursorPosition.Y - BufferInfo.srWindow.Top;
  }
  return Result;
}
//---------------------------------------------------------------------------
HWND TCustomFarPlugin::GetConsoleWindow() const
{
  wchar_t Title[1024];
  ::GetConsoleTitle(Title, LENOF(Title));
  HWND Result = ::FindWindow(nullptr, Title);
  return Result;
}
//---------------------------------------------------------------------------
uintptr_t TCustomFarPlugin::ConsoleWindowState() const
{
  uintptr_t Result;
  HWND Window = GetConsoleWindow();
  if (Window != nullptr)
  {
    WINDOWPLACEMENT WindowPlacement;
    ClearStruct(WindowPlacement);
    WindowPlacement.length = sizeof(WindowPlacement);
    Win32Check(::GetWindowPlacement(Window, &WindowPlacement) > 0);
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
  if (Window != nullptr)
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
      COORD Size = ::GetLargestConsoleWindowSize(FConsoleOutput);
      Win32Check((Size.X != 0) || (Size.Y != 0));

      FNormalConsoleSize = TerminalInfo();

      Win32Check(::ShowWindow(Window, SW_MAXIMIZE) > 0);

      Win32Check(::SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);

      CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
      Win32Check(::GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo) > 0);

      SMALL_RECT WindowSize;
      WindowSize.Left = 0;
      WindowSize.Top = 0;
      WindowSize.Right = static_cast<short>(BufferInfo.dwMaximumWindowSize.X - 1);
      WindowSize.Bottom = static_cast<short>(BufferInfo.dwMaximumWindowSize.Y - 1);
      Win32Check(::SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);
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
  Source.Top = static_cast<char>(Rows);
  Source.Right = static_cast<SHORT>(Size.x);
  Source.Bottom = static_cast<SHORT>(Size.y);
  Dest.X = 0;
  Dest.Y = 0;
  Fill.Char.UnicodeChar = L' ';
  Fill.Attributes = 7;
  ::ScrollConsoleScreenBuffer(FConsoleOutput, &Source, nullptr, Dest, &Fill);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowTerminalScreen()
{
  assert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  UnicodeString Blank = ::StringOfChar(L' ', static_cast<intptr_t>(Size.x));
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
  FarControl(FCTL_SETUSERSCREEN, 0, 0);
}
//---------------------------------------------------------------------------
class TConsoleTitleParam : public TObject
{
public:
  explicit TConsoleTitleParam() :
    Progress(0),
    Own(0)
  {}
  short Progress;
  short Own;
};
//---------------------------------------------------------------------------
void TCustomFarPlugin::ShowConsoleTitle(const UnicodeString & Title)
{
  wchar_t SaveTitle[1024];
  ::GetConsoleTitle(SaveTitle, LENOF(SaveTitle));
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
void TCustomFarPlugin::ClearConsoleTitle()
{
  assert(FSavedTitles->GetCount() > 0);
  UnicodeString Title = FSavedTitles->GetString(FSavedTitles->GetCount() - 1);
  TObject * Object = FSavedTitles->GetObject(FSavedTitles->GetCount()-1);
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
    ::SetConsoleTitle(Title.c_str());
    UpdateProgress(TBPS_NOPROGRESS, 0);
  }
  delete FSavedTitles->GetObject(FSavedTitles->GetCount() - 1);
  FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString & Title)
{
  FCurrentTitle = Title;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
  FCurrentProgress = Progress;
  UpdateConsoleTitle();
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::FormatConsoleTitle()
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
void TCustomFarPlugin::UpdateProgress(intptr_t State, intptr_t Progress)
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, State, NULL);
  if (State == TBPS_NORMAL)
  {
    ProgressValue pv;
    pv.StructSize = sizeof(ProgressValue);
    pv.Completed = Progress < 0 ? 0 : Progress > 100 ? 100 : Progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, 0, &pv);
  }
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::UpdateConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? TBPS_NORMAL : TBPS_NOPROGRESS, progress);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  assert(!Screen);
  TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  assert(Screen);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  assert(Screen);
  TFarEnvGuard Guard;
  FStartupInfo.RestoreScreen(static_cast<HANDLE>(Screen));
  Screen = 0;
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::HandleException(Exception * E, int /*OpMode*/)
{
  assert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", MB2W(E->what()));
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::GetMsg(intptr_t MsgId)
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(&MainGuid, (int)MsgId);
  return Result;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::CheckForEsc()
{
  static unsigned long LastTicks;
  unsigned long Ticks = GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    INPUT_RECORD Rec;
    DWORD ReadCount;
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
  }
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Viewer(const UnicodeString & FileName,
  const UnicodeString & Title, unsigned int Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Viewer(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_DEFAULT);
  return Result > 0;
}
//---------------------------------------------------------------------------
bool TCustomFarPlugin::Editor(const UnicodeString & FileName,
  const UnicodeString & Title, unsigned int Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Editor(
    FileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
    CP_DEFAULT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}
//---------------------------------------------------------------------------
void TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}
//---------------------------------------------------------------------------
__int64 TCustomFarPlugin::GetSystemSetting(HANDLE & Settings, const wchar_t * Name)
{
  FarSettingsItem item = {sizeof(FarSettingsItem), FSSF_SYSTEM, Name, FST_UNKNOWN, {0} };
  if (FStartupInfo.SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
  {
    return item.Number;
  }
  return 0;
}
//---------------------------------------------------------------------------
__int64 TCustomFarPlugin::FarSystemSettings()
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
intptr_t TCustomFarPlugin::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
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
__int64 TCustomFarPlugin::FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(&MainGuid, Command, Param1, Param2);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarEditorControl(EDITOR_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const
{
  switch (Command)
  {
  case ECTL_GETINFO:
  case ECTL_SETPARAM:
  case ECTL_GETFILENAME:
    // noop
    break;

  case ECTL_SETTITLE:
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
TFarEditorInfo * TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result;
  ::EditorInfo * Info = static_cast< ::EditorInfo *>(
    nb_malloc(sizeof(::EditorInfo)));
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
      nb_free(Info);
      Result = nullptr;
    }
  }
  catch (...)
  {
    nb_free(Info);
    throw;
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarPlugin::FarVersion()
{
  if (FFarVersion == 0)
  {
    FFarVersion = FarAdvControl(ACTL_GETFARMANAGERVERSION, 0);
  }
  return FFarVersion;
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::FormatFarVersion(VersionInfo &Info) const
{
  return FORMAT(L"%d.%d.%d", Info.Major, Info.Minor, Info.Build);
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarPlugin::TemporaryDir() const
{
  UnicodeString Result;
  if (FTemporaryDir.IsEmpty())
  {
    Result.SetLength(MAX_PATH);
    TFarEnvGuard Guard;
    FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), (DWORD)Result.Length(), nullptr);
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
int TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * Rec)
{
  int Result;
  /*
  {
    TFarEnvGuard Guard;
    Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
  }
  else
  */
  {
    Result = 0;
  }
  return static_cast<intptr_t>(Result);
}
//---------------------------------------------------------------------------
#ifdef NETBOX_DEBUG
void TCustomFarPlugin::RunTests()
{
  {
    TFileMasks m(L"*.txt;*.log");
    bool res = m.Matches(L"test.exe");
  }
  {
    random_ref();
    random_unref();
  }
}
#endif
//---------------------------------------------------------------------------
uintptr_t TCustomFarFileSystem::FInstances = 0;
//---------------------------------------------------------------------------
TCustomFarFileSystem::TCustomFarFileSystem(TCustomFarPlugin * APlugin) :
  TObject(),
  FPlugin(APlugin),
  FClosed(false),
  FCriticalSection(nullptr),
  FOpenPanelInfoValid(false)
{
  memset(FPanelInfo, 0, sizeof(FPanelInfo));
}

void TCustomFarFileSystem::Init()
{
  FCriticalSection = new TCriticalSection();
  FPanelInfo[0] = nullptr;
  FPanelInfo[1] = nullptr;
  FClosed = false;

  ClearStruct(FOpenPanelInfo);
  ClearOpenPanelInfo(FOpenPanelInfo);
  FInstances++;
}

//---------------------------------------------------------------------------
TCustomFarFileSystem::~TCustomFarFileSystem()
{
  FInstances--;
  ResetCachedInfo();
  ClearOpenPanelInfo(FOpenPanelInfo);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::HandleException(Exception * E, int OpMode)
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
void TCustomFarFileSystem::ClearOpenPanelInfo(OpenPanelInfo & Info)
{
  if (Info.StructSize)
  {
    nb_free((void*)Info.HostFile);
    nb_free((void*)Info.CurDir);
    nb_free((void*)Info.Format);
    nb_free((void*)Info.PanelTitle);
    assert(!Info.InfoLines);
    assert(!Info.InfoLinesNumber);
    assert(!Info.DescrFiles);
    assert(!Info.DescrFilesNumber);
    assert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
    for (intptr_t Index = 0; Index < Info.PanelModesNumber; ++Index)
    {
      assert(Info.PanelModesArray);
      TFarPanelModes::ClearPanelMode(
        const_cast<PanelMode &>(Info.PanelModesArray[Index]));
    }
    nb_free((void*)Info.PanelModesArray);
    if (Info.KeyBar)
    {
      TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
      nb_free((void*)Info.KeyBar);
    }
    nb_free((void*)Info.ShortcutData);
  }
  ClearStruct(Info);
  Info.StructSize = sizeof(Info);
  InvalidateOpenPanelInfo();
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::GetOpenPanelInfo(struct OpenPanelInfo * Info)
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
      std::unique_ptr<TFarPanelModes> PanelModes(new TFarPanelModes());
      std::unique_ptr<TFarKeyBarTitles> KeyBarTitles(new TFarKeyBarTitles());
      bool StartSortOrder = false;

      GetOpenPanelInfoEx(FOpenPanelInfo.Flags, HostFile, CurDir, Format,
        PanelTitle, PanelModes.get(), FOpenPanelInfo.StartPanelMode,
        FOpenPanelInfo.StartSortMode, StartSortOrder, KeyBarTitles.get(), ShortcutData);

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

      FOpenPanelInfoValid = true;
    }

    memmove(Info, &FOpenPanelInfo, sizeof(FOpenPanelInfo));
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFindData(struct GetFindDataInfo *Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(new TObjectList());
  bool Result = !FClosed && GetFindDataEx(PanelItems.get(), Info->OpMode);
  if (Result && PanelItems->GetCount())
  {
    Info->PanelItem = static_cast<PluginPanelItem *>(
    nb_calloc(1, sizeof(PluginPanelItem) * PanelItems->GetCount()));
    memset(Info->PanelItem, 0, sizeof(PluginPanelItem) * PanelItems->GetCount());
    Info->ItemsNumber = PanelItems->GetCount();
    for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      static_cast<TCustomFarPanelItem *>(PanelItems->GetItem(Index))->FillPanelItem(
        &(Info->PanelItem[Index]));
    }
  }
  else
  {
    Info->PanelItem = nullptr;
    Info->ItemsNumber = 0;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TCustomFarFileSystem::FreeFindData(const struct FreeFindDataInfo *Info)
{
  ResetCachedInfo();
  if (Info->PanelItem)
  {
    assert(Info->ItemsNumber > 0);
    for (intptr_t Index = 0; Index < Info->ItemsNumber; ++Index)
    {
      nb_free((void*)Info->PanelItem[Index].FileName);
      nb_free((void*)Info->PanelItem[Index].Description);
      nb_free((void*)Info->PanelItem[Index].Owner);
      for (intptr_t CustomIndex = 0; CustomIndex < Info->PanelItem[Index].CustomColumnNumber; ++CustomIndex)
      {
        nb_free((void*)Info->PanelItem[Index].CustomColumnData[CustomIndex]);
      }
      nb_free((void*)Info->PanelItem[Index].CustomColumnData);
    }
    nb_free(Info->PanelItem);
  }
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::ProcessHostFile(const struct ProcessHostFileInfo *Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  bool Result = ProcessHostFileEx(PanelItems.get(), Info->OpMode);
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::ProcessPanelInput(const struct ProcessPanelInputInfo *Info)
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
intptr_t TCustomFarFileSystem::ProcessPanelEvent(intptr_t Event, void * Param)
{
  ResetCachedInfo();
  return ProcessPanelEventEx(Event, Param);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::SetDirectory(const struct SetDirectoryInfo * Info)
{
  ResetCachedInfo();
  InvalidateOpenPanelInfo();
  intptr_t Result = SetDirectoryEx(Info->Dir, Info->OpMode);
  InvalidateOpenPanelInfo();
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::MakeDirectory(struct MakeDirectoryInfo *Info)
{
  ResetCachedInfo();
  FNameStr = Info->Name;
  intptr_t Result = 0;
  SCOPE_EXIT
  {
    if (0 != wcscmp(FNameStr.c_str(), Info->Name))
    {
      Info->Name = FNameStr.c_str();
    }
  };
  {
    Result = MakeDirectoryEx(FNameStr, Info->OpMode);
  }
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::DeleteFiles(const struct DeleteFilesInfo *Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  bool Result = DeleteFilesEx(PanelItems.get(), Info->OpMode);
  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFiles(struct GetFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  intptr_t Result = 0;
  FDestPathStr = Info->DestPath;
  SCOPE_EXIT
  {
    if (FDestPathStr != Info->DestPath)
    {
      Info->DestPath = FDestPathStr.c_str();
    }
  };
  {
    Result = GetFilesEx(PanelItems.get(), Info->Move > 0, FDestPathStr, Info->OpMode);
  }

  return Result;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::PutFiles(const struct PutFilesInfo *Info)
{
  ResetCachedInfo();
  intptr_t Result = 0;
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  Result = PutFilesEx(PanelItems.get(), Info->Move > 0, Info->OpMode);
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
const TFarPanelInfo * TCustomFarFileSystem::GetPanelInfo(int Another) const
{
  return const_cast<TCustomFarFileSystem *>(this)->GetPanelInfo(Another);
}
//---------------------------------------------------------------------------
TFarPanelInfo * TCustomFarFileSystem::GetPanelInfo(int Another)
{
  bool bAnother = Another != 0;
  if (FPanelInfo[bAnother] == nullptr)
  {
    PanelInfo * Info = static_cast<PanelInfo *>(
      nb_calloc(1, sizeof(PanelInfo)));
    Info->StructSize = sizeof(PanelInfo);
    bool Res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(Info),
      !bAnother ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!Res)
    {
      memset(Info, 0, sizeof(*Info));
      assert(false);
    }
    FPanelInfo[bAnother] = new TFarPanelInfo(Info, !bAnother ? this : nullptr);
  }
  return FPanelInfo[bAnother];
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  uintptr_t PrevInstances = FInstances;
  InvalidateOpenPanelInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, 0, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
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
  FarControl(FCTL_CLOSEPANEL, 0, 0);
}
//---------------------------------------------------------------------------
UnicodeString TCustomFarFileSystem::GetMsg(int MsgId)
{
  return FPlugin->GetMsg(MsgId);
}
//---------------------------------------------------------------------------
TCustomFarFileSystem * TCustomFarFileSystem::GetOppositeFileSystem()
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
bool TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessKeyEx(intptr_t /* Key */, uintptr_t /* ControlState */)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::ProcessPanelEventEx(intptr_t /*Event*/, void * /*Param*/)
{
  return false;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::SetDirectoryEx(const UnicodeString & /* Dir */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /* Name */, int /* OpMode */)
{
  return -1;
}
//---------------------------------------------------------------------------
bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /* PanelItems */, int /* OpMode */)
{
  return false;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::GetFilesEx(TObjectList * /* PanelItems */, bool /* Move */,
  UnicodeString & /* DestPath */, int /* OpMode */)
{
  return 0;
}
//---------------------------------------------------------------------------
intptr_t TCustomFarFileSystem::PutFilesEx(TObjectList * /* PanelItems */, bool /* Move */, int /* OpMode */)
{
  return 0;
}
//---------------------------------------------------------------------------
TObjectList * TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  std::unique_ptr<TObjectList> PanelItems(new TObjectList());
  PanelItems->SetOwnsObjects(true);
  for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
  {
    PanelItems->Add(new TFarPanelItem(&PanelItem[Index], false));
  }
  return PanelItems.release();
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
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(FPanelModes)); ++Index)
    {
      ClearPanelMode(FPanelModes[Index]);
    }
  }
}
//---------------------------------------------------------------------------
void TFarPanelModes::SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes,
  const UnicodeString & ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, const UnicodeString & StatusColumnTypes,
  const UnicodeString & StatusColumnWidths)
{
  intptr_t ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  assert(Mode != NPOS && Mode < LENOF(FPanelModes));
  assert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  wchar_t ** Titles = static_cast<wchar_t **>(
    nb_malloc(sizeof(wchar_t *) * ColumnTypesCount));
  FPanelModes[Mode].ColumnTypes = TCustomFarPlugin::DuplicateStr(ColumnTypes);
  FPanelModes[Mode].ColumnWidths = TCustomFarPlugin::DuplicateStr(ColumnWidths);
  if (ColumnTitles)
  {
    for (intptr_t Index = 0; Index < ColumnTypesCount; ++Index)
    {
      Titles[Index] = TCustomFarPlugin::DuplicateStr(ColumnTitles->GetString(Index));
    }
    FPanelModes[Mode].ColumnTitles = Titles;
  }
  else
  {
    FPanelModes[Mode].ColumnTitles = nullptr;
  }
  SetFlag(FPanelModes[Mode].Flags, FullScreen, PMFLAGS_FULLSCREEN);
  SetFlag(FPanelModes[Mode].Flags, DetailedStatus, PMFLAGS_DETAILEDSTATUS);
  SetFlag(FPanelModes[Mode].Flags, AlignExtensions, PMFLAGS_ALIGNEXTENSIONS);
  SetFlag(FPanelModes[Mode].Flags, CaseConversion, PMFLAGS_CASECONVERSION);

  FPanelModes[Mode].StatusColumnTypes = TCustomFarPlugin::DuplicateStr(StatusColumnTypes);
  FPanelModes[Mode].StatusColumnWidths = TCustomFarPlugin::DuplicateStr(StatusColumnWidths);
}
//---------------------------------------------------------------------------
void TFarPanelModes::SetFlag(PANELMODE_FLAGS & Flags, bool Value, PANELMODE_FLAGS Flag)
{
  if (Value)
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

    nb_free((void*)Mode.ColumnTypes);
    nb_free((void*)Mode.ColumnWidths);
    if (Mode.ColumnTitles)
    {
      for (intptr_t Index = 0; Index < ColumnTypesCount; ++Index)
      {
        nb_free((void*)Mode.ColumnTitles[Index]);
      }
      nb_free((void*)Mode.ColumnTitles);
    }
    nb_free((void*)Mode.StatusColumnTypes);
    nb_free((void*)Mode.StatusColumnWidths);
    ClearStruct(Mode);
  }
}
//---------------------------------------------------------------------------
void TFarPanelModes::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  assert(Info);
  Info->PanelModesNumber = LENOF(FPanelModes);
  PanelMode * PanelModesArray = static_cast<PanelMode *>(nb_calloc(1, sizeof(PanelMode) * LENOF(FPanelModes)));
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}
//---------------------------------------------------------------------------
intptr_t TFarPanelModes::CommaCount(const UnicodeString & ColumnTypes)
{
  intptr_t Count = 0;
  for (intptr_t Index = 1; Index <= ColumnTypes.Length(); ++Index)
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
  FKeyBarTitles.Labels = static_cast<KeyBarLabel *>(
    nb_malloc(sizeof(KeyBarLabel) * 7 * 12));
  memset(FKeyBarTitles.Labels, 0, sizeof(KeyBarLabel) * 7 * 12);
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
  intptr_t FunctionKeyStart, intptr_t FunctionKeyEnd)
{
  if (!FunctionKeyEnd)
  {
    FunctionKeyEnd = FunctionKeyStart;
  }
  for (intptr_t Index = FunctionKeyStart; Index <= FunctionKeyEnd; ++Index)
  {
    SetKeyBarTitle(ShiftStatus, Index, L"");
  }
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  intptr_t FunctionKey, const UnicodeString & Title)
{
  assert(FunctionKey >= 1 && FunctionKey <= 12);
  int shift = static_cast<int>(ShiftStatus);
  assert(shift >= 0 && shift < 7);
  KeyBarLabel *Labels = &FKeyBarTitles.Labels[shift * 12];
  if (Labels[FunctionKey-1].Key.VirtualKeyCode)
  {
    nb_free((void*)Labels[FunctionKey-1].Text);
    nb_free((void*)Labels[FunctionKey-1].LongText);
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
void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (intptr_t Index = 0; Index < Titles.CountLabels; ++Index)
  {
    nb_free((void*)Titles.Labels[Index].Text);
    nb_free((void*)Titles.Labels[Index].LongText);
  }
  nb_free(Titles.Labels);
  Titles.Labels = NULL;
  Titles.CountLabels = 0;
}
//---------------------------------------------------------------------------
void TFarKeyBarTitles::FillOpenPanelInfo(struct OpenPanelInfo *Info)
{
  assert(Info);
  KeyBarTitles * KeyBar = static_cast<KeyBarTitles *>(
    nb_malloc(sizeof(KeyBarTitles)));
  Info->KeyBar = KeyBar;
  memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
  FReferenced = true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
UnicodeString TCustomFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
void TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem * PanelItem)
{
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
  FILETIME FileTime = DateTimeToFileTime(LastWriteTime, dstmWin);
  FILETIME FileTimeA = DateTimeToFileTime(LastAccess, dstmWin);
  PanelItem->CreationTime = FileTime;
  PanelItem->LastAccessTime = FileTimeA;
  PanelItem->LastWriteTime = FileTime;
  PanelItem->FileSize = Size;

  PanelItem->FileName = TCustomFarPlugin::DuplicateStr(FileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  wchar_t ** CustomColumnData = static_cast<wchar_t **>(
    nb_malloc(sizeof(wchar_t *) * PanelItem->CustomColumnNumber));
  for (intptr_t Index = 0; Index < PanelItem->CustomColumnNumber; ++Index)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem):
  TCustomFarPanelItem(),
  FPanelItem(nullptr),
  FOwnsItem(false)
{
  assert(APanelItem);
  FPanelItem = APanelItem;
  FOwnsItem = OwnsItem;
}

TFarPanelItem::~TFarPanelItem()
{
  if (FOwnsItem)
    nb_free(FPanelItem);
  FPanelItem = nullptr;
}

//---------------------------------------------------------------------------
void TFarPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & /*FileName*/, __int64 & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  assert(false);
  return L"";
}
//---------------------------------------------------------------------------
PLUGINPANELITEMFLAGS TFarPanelItem::GetFlags() const
{
  return static_cast<uintptr_t>(FPanelItem->Flags);
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelItem::GetFileName() const
{
  UnicodeString Result = FPanelItem->FileName;
  return Result;
}
//---------------------------------------------------------------------------
void * TFarPanelItem::GetUserData()
{
  return FPanelItem->UserData.Data;
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetSelected() const
{
  return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}
//---------------------------------------------------------------------------
void TFarPanelItem::SetSelected(bool Value)
{
  if (Value)
  {
    FPanelItem->Flags |= PPIF_SELECTED;
  }
  else
  {
    FPanelItem->Flags &= ~PPIF_SELECTED;
  }
}
//---------------------------------------------------------------------------
uintptr_t TFarPanelItem::GetFileAttributes() const
{
  return static_cast<uintptr_t>(FPanelItem->FileAttributes);
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}
//---------------------------------------------------------------------------
bool TFarPanelItem::GetIsFile() const
{
  return (GetFileAttributes() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
THintPanelItem::THintPanelItem(const UnicodeString & AHint) :
  TCustomFarPanelItem()
{
  FHint = AHint;
}
//---------------------------------------------------------------------------
void THintPanelItem::GetData(
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
  FPanelInfo(APanelInfo),
  FItems(nullptr),
  FOwner(AOwner)
{
  // if (!FPanelInfo) throw ExtException(L"");
  assert(FPanelInfo);
}
//---------------------------------------------------------------------------
TFarPanelInfo::~TFarPanelInfo()
{
  nb_free(FPanelInfo);
  delete FItems;
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetItemCount() const
{
  return static_cast<intptr_t>(FPanelInfo->ItemsNumber);
}
//---------------------------------------------------------------------------
TRect TFarPanelInfo::GetBounds() const
{
  RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetSelectedCount() const
{
  intptr_t Count = static_cast<intptr_t>(FPanelInfo->SelectedItemsNumber);

  if ((Count == 1) && FOwner)
  {
    intptr_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, 0);
    PluginPanelItem * ppi = static_cast<PluginPanelItem *>(nb_calloc(1, size));
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<void *>(ppi));
    if ((ppi->Flags & PPIF_SELECTED) == 0)
    {
      Count = 0;
    }
    nb_free(ppi);
  }

  return Count;
}
//---------------------------------------------------------------------------
TObjectList * TFarPanelInfo::GetItems()
{
  if (!FItems)
  {
    FItems = new TObjectList();
  }
  if (FOwner)
  {
    for (intptr_t Index = 0; Index < FPanelInfo->ItemsNumber; ++Index)
    {
      // TODO: move to common function
      intptr_t Size = FOwner->FarControl(FCTL_GETPANELITEM, Index, 0);
      PluginPanelItem * ppi = static_cast<PluginPanelItem *>(nb_calloc(1, Size));
      FarGetPluginPanelItem gppi;
      gppi.StructSize = sizeof(FarGetPluginPanelItem);
      gppi.Size = Size;
      gppi.Item = ppi;
      FOwner->FarControl(FCTL_GETPANELITEM, Index, static_cast<void *>(&gppi));
      FItems->Add(new TFarPanelItem(ppi, true));
    }
  }
  return FItems;
}
//---------------------------------------------------------------------------
TFarPanelItem * TFarPanelInfo::FindFileName(const UnicodeString & FileName) const
{
  const TObjectList * AItems = GetItems();
  for (intptr_t Index = 0; Index < AItems->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
    if (PanelItem->GetFileName() == FileName)
    {
      return PanelItem;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
const TFarPanelItem * TFarPanelInfo::FindUserData(const void * UserData) const
{
  return const_cast<TFarPanelInfo *>(this)->FindUserData(UserData);
}
//---------------------------------------------------------------------------
TFarPanelItem * TFarPanelInfo::FindUserData(const void * UserData)
{
  TObjectList * AItems = GetItems();
  for (intptr_t Index = 0; Index < AItems->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = static_cast<TFarPanelItem *>(AItems->GetItem(Index));
    if (PanelItem->GetUserData() == UserData)
    {
      return PanelItem;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
void TFarPanelInfo::ApplySelection()
{
  // for "another panel info", there's no owner
  assert(FOwner != nullptr);
  FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<void *>(FPanelInfo));
}
//---------------------------------------------------------------------------
TFarPanelItem * TFarPanelInfo::GetFocusedItem()
{
  intptr_t Index = GetFocusedIndex();
  TObjectList * Items = GetItems();
  if (Items->GetCount() > 0)
  {
    assert(Index < Items->GetCount());
    return static_cast<TFarPanelItem *>(Items->GetItem(Index));
  }
  else
  {
    return nullptr;
  }
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedItem(const TFarPanelItem * Value)
{
  TObjectList * Items = GetItems();
  intptr_t Index = Items->IndexOf(static_cast<const TObject *>(Value));
  assert(Index != NPOS);
  SetFocusedIndex(Index);
}
//---------------------------------------------------------------------------
intptr_t TFarPanelInfo::GetFocusedIndex() const
{
  return static_cast<intptr_t>(FPanelInfo->CurrentItem);
}
//---------------------------------------------------------------------------
void TFarPanelInfo::SetFocusedIndex(intptr_t Value)
{
  // for "another panel info", there's no owner
  assert(FOwner != nullptr);
  if (GetFocusedIndex() != Value)
  {
    assert(Value != NPOS && Value < FPanelInfo->ItemsNumber);
    FPanelInfo->CurrentItem = static_cast<int>(Value);
    PanelRedrawInfo PanelInfo;
    PanelInfo.StructSize = sizeof(PanelRedrawInfo);
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<void *>(&PanelInfo));
  }
}
//---------------------------------------------------------------------------
TFarPanelType TFarPanelInfo::GetType() const
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
bool TFarPanelInfo::GetIsPlugin() const
{
  return ((FPanelInfo->PluginHandle != INVALID_HANDLE_VALUE) && (FPanelInfo->PluginHandle != 0));
}
//---------------------------------------------------------------------------
UnicodeString TFarPanelInfo::GetCurrentDirectory() const
{
  UnicodeString Result = L"";
  intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
    0,
    0,
    FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    FarPanelDirectory * pfpd = static_cast<FarPanelDirectory *>(nb_malloc(Size));

    FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
      Size,
      pfpd,
      FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
    Result = pfpd->Name;
    nb_free(pfpd);
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
void TFarMenuItems::Clear()
{
  FItemFocused = NPOS;
  TStringList::Clear();
}
//---------------------------------------------------------------------------
void TFarMenuItems::Delete(intptr_t Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = NPOS;
  }
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
void TFarMenuItems::SetObject(intptr_t Index, TObject * AObject)
{
  TStringList::SetObject(Index, AObject);
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
intptr_t TFarMenuItems::Add(const UnicodeString & Text, bool Visible)
{
  intptr_t Result = TStringList::Add(Text);
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
void TFarMenuItems::SetItemFocused(intptr_t Value)
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
void TFarMenuItems::SetFlag(intptr_t Index, uintptr_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    uintptr_t F = reinterpret_cast<uintptr_t>(GetObject(Index));
    if (Value)
    {
      F |= Flag;
    }
    else
    {
      F &= ~Flag;
    }
    SetObject(Index, reinterpret_cast<TObject *>(F));
  }
}
//---------------------------------------------------------------------------
bool TFarMenuItems::GetFlag(intptr_t Index, uintptr_t Flag) const
{
  return (reinterpret_cast<uintptr_t>(GetObject(Index)) & Flag) > 0;
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
  nb_free(FEditorInfo);
}
//---------------------------------------------------------------------------
intptr_t TFarEditorInfo::GetEditorID() const
{
  return static_cast<intptr_t>(FEditorInfo->EditorID);
}
//---------------------------------------------------------------------------
UnicodeString TFarEditorInfo::GetFileName()
{
  UnicodeString Result = L"";
  intptr_t BuffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, 0, 0);
  if (BuffLen)
  {
    Result.SetLength(BuffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, BuffLen, const_cast<wchar_t *>(Result.c_str()));
  }
  return Result.c_str();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFarEnvGuard::TFarEnvGuard()
{
  assert(FarPlugin != nullptr);
}
//---------------------------------------------------------------------------
TFarEnvGuard::~TFarEnvGuard()
{
  assert(FarPlugin != nullptr);
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
  assert(FarPlugin != nullptr);
}
//---------------------------------------------------------------------------
TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
  assert(FarPlugin != nullptr);
}
//---------------------------------------------------------------------------
void FarWrapText(const UnicodeString & Text, TStrings * Result, intptr_t MaxWidth)
{
  size_t TabSize = 8;
  TStringList Lines;
  Lines.SetText(Text);
  TStringList WrappedLines;
  for (intptr_t Index = 0; Index < Lines.GetCount(); ++Index)
  {
    UnicodeString WrappedLine = Lines.GetString(Index);
    if (!WrappedLine.IsEmpty())
    {
      WrappedLine = ::ReplaceChar(WrappedLine, '\'', '\3');
      WrappedLine = ::ReplaceChar(WrappedLine, '\"', '\4');
      WrappedLine = Sysutils::WrapText(WrappedLine, MaxWidth);
      WrappedLine = ::ReplaceChar(WrappedLine, '\3', '\'');
      WrappedLine = ::ReplaceChar(WrappedLine, '\4', '\"');
      WrappedLines.SetText(WrappedLine);
      for (intptr_t WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); ++WrappedIndex)
      {
        UnicodeString FullLine = WrappedLines.GetString(WrappedIndex);
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
//------------------------------------------------------------------------------
TGlobalFunctionsIntf * GetGlobalFunctions()
{
  static TGlobalFunctions * GlobalFunctions = nullptr;
  if (!GlobalFunctions)
  {
    GlobalFunctions = new TGlobalFunctions();
  }
  return GlobalFunctions;
}

//------------------------------------------------------------------------------
HINSTANCE TGlobalFunctions::GetInstanceHandle() const
{
  HINSTANCE Result = nullptr;
  if (FarPlugin)
  {
    Result = FarPlugin->GetHandle();
  }
  return Result;
}
//------------------------------------------------------------------------------
UnicodeString TGlobalFunctions::GetCurrentDirectory() const
{
  UnicodeString Result;
  wchar_t Path[MAX_PATH + 1];
  if (FarPlugin)
  {
    FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(sizeof(Path), Path);
  }
  else
  {
    ::GetCurrentDirectory(LENOF(Path), Path);
  }
  Result = Path;
  return Result;
}

UnicodeString TGlobalFunctions::GetStrVersionNumber() const
{
  return NETBOX_VERSION_NUMBER;
}
//------------------------------------------------------------------------------
