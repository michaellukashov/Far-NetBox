#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "FarPlugin.h"
#include "FarUtils.h"
#include "WinSCPPlugin.h"
#include "FarPluginStrings.h"
#include "FarDialog.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "puttyexp.h"
#include "plugin_version.hpp"

TCustomFarPlugin * FarPlugin = nullptr;

#define FAR_TITLE_SUFFIX L" - Far"

TFarMessageParams::TFarMessageParams() :
  MoreMessages(nullptr),
  CheckBox(false),
  Timer(0),
  TimerAnswer(0),
  TimerEvent(nullptr),
  Timeout(0),
  TimeoutButton(0),
  DefaultButton(0),
  ClickEvent(nullptr),
  Token(nullptr)
{
}

TCustomFarPlugin::TCustomFarPlugin(TObjectClassId Kind, HINSTANCE HInst) :
  TObject(Kind),
  FOpenedPlugins(new TList()),
  FTopDialog(nullptr),
  FSavedTitles(new TStringList())
{
  // ::SetGlobals(new TGlobalFunctions());
  FFarThreadId = GetCurrentThreadId();
  FHandle = HInst;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FCurrentProgress = -1;
  FValidFarSystemSettings = false;
  FFarSystemSettings = 0;

  ClearStruct(FPluginInfo);
  ClearPluginInfo(FPluginInfo);
  ClearStruct(FStartupInfo);
  ClearStruct(FFarStandardFunctions);

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

TCustomFarPlugin::~TCustomFarPlugin()
{
  DebugAssert(FTopDialog == nullptr);

  ResetCachedInfo();
  SAFE_CLOSE_HANDLE(FConsoleInput);
  FConsoleInput = INVALID_HANDLE_VALUE;
  SAFE_CLOSE_HANDLE(FConsoleOutput);
  FConsoleOutput = INVALID_HANDLE_VALUE;

  ClearPluginInfo(FPluginInfo);
  DebugAssert(FOpenedPlugins->GetCount() == 0);
  SAFE_DESTROY(FOpenedPlugins);
  for (intptr_t Index = 0; Index < FSavedTitles->GetCount(); ++Index)
  {
    TObject * Object = FSavedTitles->GetObj(Index);
    SAFE_DESTROY(Object);
  }
  SAFE_DESTROY(FSavedTitles);
//  TGlobalsIntf * Intf = GetGlobals();
//  SAFE_DESTROY_EX(TGlobalsIntf, Intf);
//  ::SetGlobals(nullptr);
}

bool TCustomFarPlugin::HandlesFunction(THandlesFunction /*Function*/) const
{
  return false;
}

VersionInfo TCustomFarPlugin::GetMinFarVersion() const
{
  return MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
}

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
    DebugAssert(FStartupInfo.GetMsg != nullptr);
    DebugAssert(FStartupInfo.Message != nullptr);

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
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
}

void TCustomFarPlugin::ExitFAR()
{
}

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
          wchar_t ** StringArray = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + NAME.GetCount())); \
          GUID *Guids = static_cast<GUID *>(nb_malloc(sizeof(GUID) * NAME.GetCount())); \
          FPluginInfo.NAME.Guids = Guids; \
          FPluginInfo.NAME.Strings = StringArray; \
          FPluginInfo.NAME.Count = NAME.GetCount(); \
          for (intptr_t Index = 0; Index < NAME.GetCount(); Index++) \
          { \
            StringArray[Index] = DuplicateStr(NAME.GetString(Index)); \
            Guids[Index] = *reinterpret_cast<const GUID *>(NAME.GetObj(Index)); \
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
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
}

UnicodeString TCustomFarPlugin::GetModuleName() const
{
  return FStartupInfo.ModuleName;
}

void TCustomFarPlugin::ClearPluginInfo(PluginInfo & Info) const
{
  if (Info.StructSize)
  {
    #define FREESTRINGARRAY(NAME) \
      for (size_t Index = 0; Index < Info.NAME.Count; ++Index) \
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

wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString & Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return nullptr;
  }
  else
  {
    const size_t sz = Str.Length() + 1;
    wchar_t * Result = nb::wchcalloc(sizeof(wchar_t) * (1 + sz));
    wcscpy_s(Result, sz, Str.c_str());
    return Result;
  }
}

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

TCustomFarFileSystem * TCustomFarPlugin::GetPanelFileSystem(bool Another,
    HANDLE /*Plugin*/)
{
  TCustomFarFileSystem * Result = nullptr;
  RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  TCustomFarFileSystem * FarFileSystem = nullptr;
  intptr_t Index = 0;
  while (!Result && (Index < FOpenedPlugins->GetCount()))
  {
    FarFileSystem = FOpenedPlugins->GetAs<TCustomFarFileSystem>(Index);
    DebugAssert(FarFileSystem);
    RECT Bounds = GetPanelBounds(FarFileSystem);
    if (Another && CompareRects(Bounds, PassivePanelBounds))
    {
      Result = FarFileSystem;
    }
    else if (!Another && CompareRects(Bounds, ActivePanelBounds))
    {
      Result = FarFileSystem;
    }
    ++Index;
  }
  return Result;
}

void TCustomFarPlugin::InvalidateOpenPanelInfo()
{
  for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
  {
    TCustomFarFileSystem * FarFileSystem =
      FOpenedPlugins->GetAs<TCustomFarFileSystem>(Index);
    FarFileSystem->InvalidateOpenPanelInfo();
  }
}

intptr_t TCustomFarPlugin::Configure(const struct ConfigureInfo * Info)
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
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return 0;
  }
}

void * TCustomFarPlugin::OpenPlugin(const struct OpenInfo * Info)
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
      Result = nullptr;
    }

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return nullptr;
  }
}

void TCustomFarPlugin::ClosePanel(void * Plugin)
{
  try
  {
    ResetCachedInfo();
    TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);
    {
      SCOPE_EXIT
      {
        FOpenedPlugins->Remove(FarFileSystem);
      };
      TGuard Guard(FarFileSystem->GetCriticalSection());
      FarFileSystem->Close();
    }
    SAFE_DESTROY(FarFileSystem);
#ifdef USE_DLMALLOC
    // dlmalloc_trim(0); // 64 * 1024);
#endif
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
}

void TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FarFileSystem, Exception * E, OPERATION_MODES OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API functions that must update
  // panel contents on themselves (like ProcessPanelInput), the instance of filesystem
  // may not exist anymore.
  // Check against object pointer is stupid, but no other idea so far.
  if (FOpenedPlugins->IndexOf(FarFileSystem) != NPOS)
  {
    DEBUG_PRINTF("before FileSystem->HandleException");
    FarFileSystem->HandleException(E, OpMode);
  }
  else
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(E, OpMode);
  }
}

void TCustomFarPlugin::GetOpenPanelInfo(struct OpenPanelInfo * Info)
{
  if (!Info)
    return;
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  if (!FOpenedPlugins || !FarFileSystem)
    return;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);
    TGuard Guard(FarFileSystem->GetCriticalSection());
    FarFileSystem->GetOpenPanelInfo(Info);
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

intptr_t TCustomFarPlugin::GetFindData(struct GetFindDataInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFindData(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

void TCustomFarPlugin::FreeFindData(const struct FreeFindDataInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      FarFileSystem->FreeFindData(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

intptr_t TCustomFarPlugin::ProcessHostFile(const struct ProcessHostFileInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessHostFile))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessHostFile(Info);
      }
    }
    else
    {
      return 0;
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessPanelInput(const struct ProcessPanelInputInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessKey))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessPanelInput(Info);
      }
    }
    else
    {
      return 0;
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
    // when error occurs, assume that key can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}

intptr_t TCustomFarPlugin::ProcessPanelEvent(const struct ProcessPanelEventInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessPanelEvent))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      UnicodeString Buf;
      void *Param = Info->Param;
      if ((Info->Event == FE_CHANGEVIEWMODE) || (Info->Event == FE_COMMAND))
      {
        Buf = static_cast<wchar_t *>(Info->Param);
        Param = const_cast<void *>(reinterpret_cast<const void *>(Buf.c_str()));
      }

      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->ProcessPanelEvent(Info->Event, Param);
    }
    else
    {
      return 0;
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
    return Info->Event == FE_COMMAND ? true : false;
  }
}

intptr_t TCustomFarPlugin::SetDirectory(const struct SetDirectoryInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  DebugAssert(FarFileSystem);
  if (!FarFileSystem)
  {
    return 0;
  }
  UnicodeString PrevCurrentDirectory = FarFileSystem->GetCurrDirectory();
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);
    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->SetDirectory(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    if (FarFileSystem->GetOpenPanelInfoValid() && !PrevCurrentDirectory.IsEmpty())
    {
      SetDirectoryInfo Info2;
      Info2.StructSize = sizeof(Info2);
      Info2.hPanel = Info->hPanel;
      Info2.Dir = PrevCurrentDirectory.c_str();
      Info2.UserData = Info->UserData;
      Info2.OpMode = Info->OpMode;
      try
      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->SetDirectory(&Info2);
      }
      catch (Exception &)
      {
        return 0;
      }
    }
    return 0;
  }
}

intptr_t TCustomFarPlugin::MakeDirectory(struct MakeDirectoryInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->MakeDirectory(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::DeleteFiles(const struct DeleteFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->DeleteFiles(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::GetFiles(struct GetFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFiles(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    // display error even for OPM_FIND
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode & ~OPM_FIND);
    return 0;
  }
}

intptr_t TCustomFarPlugin::PutFiles(const struct PutFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Info->hPanel);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->PutFiles(Info);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessEditorEvent(const struct ProcessEditorEventInfo * Info)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorEventEx(Info);
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessEditorInput(const struct ProcessEditorInputInfo * Info)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorInputEx(&Info->Rec);
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    // when error occurs, assume that input event can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}

intptr_t TCustomFarPlugin::MaxMessageLines() const
{
  return static_cast<intptr_t>(TerminalInfo().y - 5);
}

intptr_t TCustomFarPlugin::MaxMenuItemLength() const
{
  // got from maximal length of path in FAR's folders history
  return static_cast<intptr_t>(TerminalInfo().x - 13);
}

intptr_t TCustomFarPlugin::MaxLength(TStrings * Strings) const
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

TFarMessageDialog::TFarMessageDialog(TCustomFarPlugin * Plugin,
  TFarMessageParams * Params) :
  TFarDialog(Plugin),
  FCheckBoxChecked(false),
  FParams(Params),
  FTimeoutButton(nullptr),
  FCheckBox(nullptr)
{
  DebugAssert(FParams != nullptr);
}

void TFarMessageDialog::Init(uintptr_t AFlags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons)
{
  DebugAssert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  DebugAssert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME DebugAssert(FLAGCLEAR(AFlags, FMSG_DOWN));
  DebugAssert(FLAGCLEAR(AFlags, FMSG_ALLINONE));
  std::unique_ptr<TStrings> MessageLines(new TStringList());
  FarWrapText(Message, MessageLines.get(), MaxMessageWidth);
  intptr_t MaxLen = GetFarPlugin()->MaxLength(MessageLines.get());
  TStrings * MoreMessageLines = nullptr;
  std::unique_ptr<TStrings> MoreMessageLinesPtr;
  if (FParams->MoreMessages != nullptr)
  {
    MoreMessageLines = new TStringList();
    MoreMessageLinesPtr.reset(MoreMessageLines);
    UnicodeString MoreMessages = FParams->MoreMessages->GetText();
    while ((MoreMessages.Length() > 0) && (MoreMessages[MoreMessages.Length()] == L'\n' ||
           MoreMessages[MoreMessages.Length()] == L'\r'))
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
  for (uintptr_t Index = 0; Index < (uintptr_t)Buttons->GetCount(); ++Index)
  {
    TFarButton * PrevButton = Button;
    Button = new TFarButton(this);
    Button->SetDefault(FParams->DefaultButton == Index);
    Button->SetBrackets(brNone);
    Button->SetOnClick(nb::bind(&TFarMessageDialog::ButtonClick, this));
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
    Button->SetTag(reinterpret_cast<intptr_t>(Buttons->GetObj(Index)));
    if (PrevButton != nullptr)
    {
      Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
    }

    if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
    {
      for (intptr_t PIndex = 0; PIndex < GetItemCount(); ++PIndex)
      {
        TFarButton * PrevButton2 = dyn_cast<TFarButton>(GetItem(PIndex));
        if ((PrevButton2 != nullptr) && (PrevButton2 != Button))
        {
          PrevButton2->Move(0, -1);
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
    DebugAssert(MoreMessagesLister != nullptr);
    if (MoreMessageHeight > MoreMessagesLister->GetItems()->GetCount())
    {
      MoreMessageHeight = MoreMessagesLister->GetItems()->GetCount();
    }
    MoreMessagesLister->SetHeight(MoreMessageHeight);
    MoreMessagesLister->SetRight(
      GetBorderBox()->GetRight() - (MoreMessagesLister->GetScrollBar() ? 0 : 1));
    MoreMessagesLister->SetTabStop(MoreMessagesLister->GetScrollBar());
    DebugAssert(MoreMessagesSeparator != nullptr);
    MoreMessagesSeparator->SetPosition(
      MoreMessagesLister->GetTop() + MoreMessagesLister->GetHeight());
    S.y += static_cast<int>(MoreMessagesLister->GetHeight()) + 1;
  }
  SetSize(S);
}

void TFarMessageDialog::Idle()
{
  TFarDialog::Idle();

  if (FParams->Timer > 0)
  {
    uintptr_t SinceLastTimer = static_cast<uintptr_t>((Now() - FLastTimerTime).GetValue() * MSecsPerDay);
    if (SinceLastTimer >= FParams->Timeout)
    {
      DebugAssert(FParams->TimerEvent);
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
    size_t Running = static_cast<size_t>((Now() - FStartTime).GetValue() * MSecsPerDay);
    if (Running >= FParams->Timeout)
    {
      DebugAssert(FTimeoutButton != nullptr);
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

void TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != nullptr)
  {
    if ((FCheckBox != nullptr) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (intptr_t Index = 0; Index < GetItemCount(); ++Index)
      {
        TFarButton * Button = dyn_cast<TFarButton>(GetItem(Index));
        if ((Button != nullptr) && (Button->GetTag() == 0))
        {
          Button->SetEnabled(!FCheckBox->GetChecked());
        }
      }
      FCheckBoxChecked = FCheckBox->GetChecked();
    }
  }
}

intptr_t TFarMessageDialog::Execute(bool & ACheckBox)
{
  if (GetDefaultButton()->CanFocus())
    GetDefaultButton()->UpdateFocused(true);
  FStartTime = Now();
  FLastTimerTime = FStartTime;
  FCheckBoxChecked = !ACheckBox;
  if (FCheckBox != nullptr)
  {
    FCheckBox->SetChecked(ACheckBox);
  }

  intptr_t Result = ShowModal();
  DebugAssert(Result != 0);
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

void TFarMessageDialog::ButtonClick(TFarButton * Sender, bool & Close)
{
  if (FParams->ClickEvent)
  {
    FParams->ClickEvent(FParams->Token, Sender->GetResult() - 1, Close);
  }
}

intptr_t TCustomFarPlugin::DialogMessage(uintptr_t Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  std::unique_ptr<TFarMessageDialog> Dialog(new TFarMessageDialog(this, Params));
  Dialog->Init(Flags, Title, Message, Buttons);
  intptr_t Result = Dialog->Execute(Params->CheckBox);
  return Result;
}

intptr_t TCustomFarPlugin::FarMessage(uintptr_t Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  DebugAssert(Params != nullptr);

  wchar_t ** Items = nullptr;
  SCOPE_EXIT
  {
    nb_free(Items);
  };
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

  TStringList * MessageLines = new TStringList();
  std::unique_ptr<TStrings> MessageLinesPtr(MessageLines);
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

  Items = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + MessageLines->GetCount()));
  for (intptr_t Index = 0; Index < MessageLines->GetCount(); ++Index)
  {
    UnicodeString S = MessageLines->GetString(Index);
    MessageLines->SetString(Index, UnicodeString(S));
    Items[Index] = const_cast<wchar_t *>(MessageLines->GetString(Index).c_str());
  }

  TFarEnvGuard Guard;
  intptr_t Result = static_cast<intptr_t>(FStartupInfo.Message(&MainGuid, &MainGuid,
    Flags | FMSG_LEFTALIGN, nullptr, Items, static_cast<int>(MessageLines->GetCount()),
    static_cast<int>(Buttons->GetCount())));

  return Result;
}

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

  intptr_t Result = -1;
  if (Buttons != nullptr)
  {
    TFarMessageParams DefaultParams;
    TFarMessageParams * AParams = (Params == nullptr ? &DefaultParams : Params);
    Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
  }
  else
  {
    DebugAssert(Params == nullptr);
    UnicodeString Items = Title + L"\n" + Message;
    TFarEnvGuard Guard;
    Result = static_cast<intptr_t>(FStartupInfo.Message(&MainGuid, &MainGuid,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      nullptr,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0));
  }
  return Result;
}

intptr_t TCustomFarPlugin::Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, const FarMenuItem * Items, intptr_t Count,
  const FarKey * BreakKeys, intptr_t & BreakCode)
{
  DebugAssert(Items);

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

intptr_t TCustomFarPlugin::Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items, const FarKey * BreakKeys,
  intptr_t & BreakCode)
{
  DebugAssert(Items && Items->GetCount());
  intptr_t Result = 0;
  FarMenuItem * MenuItems = nb::calloc<FarMenuItem*>(sizeof(FarMenuItem) * (1 + Items->GetCount()));
  SCOPE_EXIT
  {
    nb_free(MenuItems);
  };
  intptr_t Selected = NPOS;
  intptr_t Count = 0;
  for (intptr_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    uintptr_t Flags2 = reinterpret_cast<uintptr_t>(Items->GetObj(Index));
    if (FLAGCLEAR(Flags2, MIF_HIDDEN))
    {
      ClearStruct(MenuItems[Count]);
      MenuItems[Count].Flags = static_cast<DWORD>(Flags2);
      if (MenuItems[Count].Flags & MIF_SELECTED)
      {
        DebugAssert(Selected == NPOS);
        Selected = Index;
      }
      MenuItems[Count].Text = Items->GetString(Index).c_str();
      MenuItems[Count].UserData = Index;
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
      Items->SetObj(Selected, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->GetObj(Selected)) & ~MIF_SELECTED));
    }
    Items->SetObj(Result, reinterpret_cast<TObject *>(reinterpret_cast<size_t>(Items->GetObj(Result)) | MIF_SELECTED));
  }
  else
  {
    Result = ResultItem;
  }
  return Result;
}

intptr_t TCustomFarPlugin::Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, TStrings * Items)
{
  intptr_t BreakCode;
  return Menu(Flags, Title, Bottom, Items, nullptr, BreakCode);
}

bool TCustomFarPlugin::InputBox(const UnicodeString & Title,
  const UnicodeString & Prompt, UnicodeString & Text, PLUGINPANELITEMFLAGS Flags,
  const UnicodeString & HistoryName, intptr_t MaxLen, TFarInputBoxValidateEvent OnValidate)
{
  bool Repeat = false;
  intptr_t Result = 0;
  do
  {
    UnicodeString DestText;
    DestText.SetLength(MaxLen + 1);
    HANDLE ScreenHandle = nullptr;
    SaveScreen(ScreenHandle);
    {
      TFarEnvGuard Guard;
      Result = FStartupInfo.InputBox(
        &MainGuid,
        &MainGuid,
        Title.c_str(),
        Prompt.c_str(),
        HistoryName.c_str(),
        Text.c_str(),
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
          DEBUG_PRINTF("before HandleException");
          HandleException(&E);
          Repeat = true;
        }
      }
    }
  }
  while (Repeat);

  return (Result != 0);
}

void TCustomFarPlugin::Text(int X, int Y, int Color, const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FarColor color = {};
  color.Flags = FCF_FG_4BIT | FCF_BG_4BIT;
  color.ForegroundColor = Color; // LIGHTGRAY;
  color.BackgroundColor = 0;
  FStartupInfo.Text(X, Y, &color, Str.c_str());
}

void TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, nullptr);
}

void TCustomFarPlugin::FarWriteConsole(const UnicodeString & Str)
{
  unsigned long Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, nullptr);
}

void TCustomFarPlugin::FarCopyToClipboard(const UnicodeString & Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(FCT_STREAM, Str.c_str());
}

void TCustomFarPlugin::FarCopyToClipboard(const TStrings * Strings)
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

TPoint TCustomFarPlugin::TerminalInfo(TPoint * Size, TPoint * Cursor) const
{
  CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
  ClearStruct(BufferInfo);
  ::GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);
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
    Cursor->y = BufferInfo.dwCursorPosition.Y; // - BufferInfo.srWindow.Top;
  }
  return Result;
}

HWND TCustomFarPlugin::GetConsoleWindow() const
{
  wchar_t Title[1024];
  ::GetConsoleTitle(Title, _countof(Title));
  HWND Result = ::FindWindow(nullptr, Title);
  return Result;
}

uintptr_t TCustomFarPlugin::ConsoleWindowState() const
{
  uintptr_t Result = SW_SHOWNORMAL;
  HWND Window = GetConsoleWindow();
  if (Window != nullptr)
  {
    WINDOWPLACEMENT WindowPlacement;
    ClearStruct(WindowPlacement);
    WindowPlacement.length = sizeof(WindowPlacement);
    ::Win32Check(::GetWindowPlacement(Window, &WindowPlacement) > 0);
    Result = WindowPlacement.showCmd;
  }
  return Result;
}

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

        ::Win32Check(::ShowWindow(Window, SW_RESTORE) > 0);

        SMALL_RECT WindowSize;
        WindowSize.Left = 0;
        WindowSize.Top = 0;
        WindowSize.Right = static_cast<short>(Size.X - 1);
        WindowSize.Bottom = static_cast<short>(Size.Y - 1);
        ::Win32Check(::SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);

        ::Win32Check(::SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);
      }
    }
    else
    {
      COORD Size = ::GetLargestConsoleWindowSize(FConsoleOutput);
      ::Win32Check((Size.X != 0) || (Size.Y != 0));

      FNormalConsoleSize = TerminalInfo();

      ::Win32Check(::ShowWindow(Window, SW_MAXIMIZE) > 0);

      ::Win32Check(::SetConsoleScreenBufferSize(FConsoleOutput, Size) > 0);

      CONSOLE_SCREEN_BUFFER_INFO BufferInfo;
      ::Win32Check(::GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo) > 0);

      SMALL_RECT WindowSize;
      WindowSize.Left = 0;
      WindowSize.Top = 0;
      WindowSize.Right = static_cast<short>(BufferInfo.dwMaximumWindowSize.X - 1);
      WindowSize.Bottom = static_cast<short>(BufferInfo.dwMaximumWindowSize.Y - 1);
      ::Win32Check(::SetConsoleWindowInfo(FConsoleOutput, true, &WindowSize) > 0);
    }
  }
}

void TCustomFarPlugin::ScrollTerminalScreen(int Rows)
{
  TPoint Size = TerminalInfo();

  SMALL_RECT Source;
  COORD Dest;
  CHAR_INFO Fill;
  Source.Left = 0;
  Source.Top = static_cast<SHORT>(Rows);
  Source.Right = static_cast<SHORT>(Size.x);
  Source.Bottom = static_cast<SHORT>(Size.y);
  Dest.X = 0;
  Dest.Y = 0;
  Fill.Char.UnicodeChar = L' ';
  Fill.Attributes = 7;
  ::ScrollConsoleScreenBuffer(FConsoleOutput, &Source, nullptr, Dest, &Fill);
}

void TCustomFarPlugin::ShowTerminalScreen()
{
  DebugAssert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  if (Size.y >= 2)
  {
    // clean menu keybar area before command output
    UnicodeString Blank = ::StringOfChar(L' ', static_cast<intptr_t>(Size.x));
    for (int Y = Size.y - 2; Y < Size.y; Y++)
    {
      Text(0, Y, 7 /*LIGHTGRAY*/, Blank);
    }
  }
  FlushText();

  COORD Coord;
  Coord.X = 0;
  Coord.Y = static_cast<SHORT>(Cursor.y);
  ::SetConsoleCursorPosition(FConsoleOutput, Coord);
  FTerminalScreenShowing = true;
}

void TCustomFarPlugin::SaveTerminalScreen()
{
  FTerminalScreenShowing = false;
  FarControl(FCTL_SETUSERSCREEN, 0, 0);
}

class TConsoleTitleParam : public TObject
{
public:
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TConsoleTitleParam;
  }

public:
  explicit TConsoleTitleParam() :
    TObject(OBJECT_CLASS_TConsoleTitleParam),
    Progress(0),
    Own(0)
  {
  }

  short Progress;
  short Own;
};

void TCustomFarPlugin::ShowConsoleTitle(const UnicodeString & Title)
{
  wchar_t SaveTitle[1024];
  ::GetConsoleTitle(SaveTitle, _countof(SaveTitle));
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
  UpdateCurrentConsoleTitle();
}

void TCustomFarPlugin::ClearConsoleTitle()
{
  DebugAssert(FSavedTitles->GetCount() > 0);
  UnicodeString Title = FSavedTitles->GetString(FSavedTitles->GetCount() - 1);
  TObject * Object = FSavedTitles->GetObj(FSavedTitles->GetCount() - 1);
  TConsoleTitleParam * Param = dyn_cast<TConsoleTitleParam>(Object);
  if (Param->Own)
  {
    FCurrentTitle = Title;
    FCurrentProgress = Param->Progress;
    UpdateCurrentConsoleTitle();
  }
  else
  {
    FCurrentTitle.Clear();
    FCurrentProgress = -1;
    ::SetConsoleTitle(Title.c_str());
    UpdateProgress(TBPS_NOPROGRESS, 0);
  }
  {
    TObject * Obj = FSavedTitles->GetObj(FSavedTitles->GetCount() - 1);
    SAFE_DESTROY(Obj);
  }
  FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
}

void TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString & Title)
{
  FCurrentTitle = Title;
  UpdateCurrentConsoleTitle();
}

void TCustomFarPlugin::UpdateConsoleTitleProgress(short Progress)
{
  FCurrentProgress = Progress;
  UpdateCurrentConsoleTitle();
}

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

void TCustomFarPlugin::UpdateProgress(intptr_t State, intptr_t Progress)
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, State, nullptr);
  if (State == TBPS_NORMAL)
  {
    ProgressValue pv;
    pv.StructSize = sizeof(ProgressValue);
    pv.Completed = Progress < 0 ? 0 : Progress > 100 ? 100 : Progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, 0, &pv);
  }
}

void TCustomFarPlugin::UpdateCurrentConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  ::SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? TBPS_NORMAL : TBPS_NOPROGRESS, progress);
}

void TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  DebugAssert(!Screen);
  TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  DebugAssert(Screen);
}

void TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  DebugAssert(Screen);
  TFarEnvGuard Guard;
  FStartupInfo.RestoreScreen(Screen);
  Screen = nullptr;
}

void TCustomFarPlugin::HandleException(Exception * E, OPERATION_MODES /*OpMode*/)
{
  DebugAssert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", E ? E->Message : L"");
}

UnicodeString TCustomFarPlugin::GetMsg(intptr_t MsgId) const
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(&MainGuid, (int)MsgId);
  return Result;
}

bool TCustomFarPlugin::CheckForEsc()
{
  static uint32_t LastTicks;
  uint32_t Ticks = ::GetTickCount();
  if ((LastTicks == 0) || (Ticks - LastTicks > 500))
  {
    LastTicks = Ticks;

    INPUT_RECORD Rec;
    DWORD ReadCount;
    while (::PeekConsoleInput(FConsoleInput, &Rec, 1, &ReadCount) && ReadCount)
    {
      ::ReadConsoleInput(FConsoleInput, &Rec, 1, &ReadCount);
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

bool TCustomFarPlugin::Viewer(const UnicodeString & AFileName,
  const UnicodeString & Title, VIEWER_FLAGS Flags)
{
  TFarEnvGuard Guard;
  intptr_t Result = FStartupInfo.Viewer(
    AFileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_DEFAULT);
  return Result > 0;
}

bool TCustomFarPlugin::Editor(const UnicodeString & AFileName,
  const UnicodeString & Title, EDITOR_FLAGS Flags)
{
  TFarEnvGuard Guard;
  intptr_t Result = FStartupInfo.Editor(
    AFileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
    CP_DEFAULT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}

void TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}

__int64 TCustomFarPlugin::GetSystemSetting(HANDLE & Settings, const wchar_t * Name) const
{
  FarSettingsItem item = {sizeof(FarSettingsItem), FSSF_SYSTEM, Name, FST_UNKNOWN, {0} };
  if (FStartupInfo.SettingsControl(Settings, SCTL_GET, 0, &item) && FST_QWORD == item.Type)
  {
    return item.Number;
  }
  return 0;
}

__int64 TCustomFarPlugin::GetFarSystemSettings() const
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
    DebugAssert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.PanelControl(Plugin, Command, Param1, Param2);
}

__int64 TCustomFarPlugin::FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(&MainGuid, Command, Param1, Param2);
}

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
    DebugAssert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.EditorControl(-1, Command, Param1, Param2);
}

TFarEditorInfo * TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result = nullptr;
  ::EditorInfo * Info = nb::calloc<::EditorInfo *>(sizeof(::EditorInfo));
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
    }
  }
  catch (...)
  {
    nb_free(Info);
    throw;
  }
  return Result;
}

intptr_t TCustomFarPlugin::GetFarVersion() const
{
  if (FFarVersion == 0)
  {
    FFarVersion = FarAdvControl(ACTL_GETFARMANAGERVERSION, 0);
  }
  return FFarVersion;
}

UnicodeString TCustomFarPlugin::FormatFarVersion(VersionInfo &Info) const
{
  return FORMAT(L"%d.%d.%d", Info.Major, Info.Minor, Info.Build);
}

UnicodeString TCustomFarPlugin::GetTemporaryDir() const
{
  UnicodeString Result(NB_MAX_PATH, 0);
  TFarEnvGuard Guard;
  FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), (DWORD)Result.Length(), nullptr);
  PackStr(Result);
  return Result;
}

int TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * /*Rec*/)
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

#ifdef NETBOX_DEBUG
void TCustomFarPlugin::RunTests()
{
  {
    TFileMasks m(L"*.txt;*.log");
    bool res = m.Matches(L"test.exe");
    DebugAssert(!res);
  }
  {
    random_ref();
    random_unref();
  }
}
#endif

uintptr_t TCustomFarFileSystem::FInstances = 0;

TCustomFarFileSystem::TCustomFarFileSystem(TObjectClassId Kind, TCustomFarPlugin * APlugin) :
  TObject(Kind),
  FPlugin(APlugin),
  FClosed(false),
  FOpenPanelInfoValid(false)
{
  ::ZeroMemory(FPanelInfo, sizeof(FPanelInfo));
  ClearStruct(FOpenPanelInfo);
}

void TCustomFarFileSystem::Init()
{
  FPanelInfo[0] = nullptr;
  FPanelInfo[1] = nullptr;
  FClosed = false;

  ClearStruct(FOpenPanelInfo);
  ClearOpenPanelInfo(FOpenPanelInfo);
  FInstances++;
}

TCustomFarFileSystem::~TCustomFarFileSystem()
{
  FInstances--;
  ResetCachedInfo();
  ClearOpenPanelInfo(FOpenPanelInfo);
}

void TCustomFarFileSystem::HandleException(Exception * E, OPERATION_MODES OpMode)
{
  DEBUG_PRINTF("before FPlugin->HandleException");
  FPlugin->HandleException(E, OpMode);
}

void TCustomFarFileSystem::Close()
{
  FClosed = true;
}

void TCustomFarFileSystem::InvalidateOpenPanelInfo()
{
  FOpenPanelInfoValid = false;
}

void TCustomFarFileSystem::ClearOpenPanelInfo(OpenPanelInfo & Info)
{
  if (Info.StructSize)
  {
    nb_free((void*)Info.HostFile);
    nb_free((void*)Info.CurDir);
    nb_free((void*)Info.Format);
    nb_free((void*)Info.PanelTitle);
    DebugAssert(!Info.InfoLines);
    DebugAssert(!Info.InfoLinesNumber);
    DebugAssert(!Info.DescrFiles);
    DebugAssert(!Info.DescrFilesNumber);
    DebugAssert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
    for (size_t Index = 0; Index < Info.PanelModesNumber; ++Index)
    {
      DebugAssert(Info.PanelModesArray);
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
      FOpenPanelInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplaceAll(CurDir, L"\\", L"/"));

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

intptr_t TCustomFarFileSystem::GetFindData(struct GetFindDataInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(new TObjectList());
  bool Result = !FClosed && GetFindDataEx(PanelItems.get(), Info->OpMode);
  if (Result && PanelItems->GetCount())
  {
    Info->PanelItem = nb::calloc<PluginPanelItem *>(sizeof(PluginPanelItem) * PanelItems->GetCount());
    Info->ItemsNumber = PanelItems->GetCount();
    for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      PanelItems->GetAs<TCustomFarPanelItem>(Index)->FillPanelItem(
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

void TCustomFarFileSystem::FreeFindData(const struct FreeFindDataInfo * Info)
{
  ResetCachedInfo();
  if (Info->PanelItem)
  {
    DebugAssert(Info->ItemsNumber > 0);
    for (size_t Index = 0; Index < Info->ItemsNumber; ++Index)
    {
      //delete[] Info->PanelItem[Index].FileName;
      //delete[] Info->PanelItem[Index].AlternateFileName;
      nb_free((void*)Info->PanelItem[Index].FileName);
      nb_free((void*)Info->PanelItem[Index].AlternateFileName);
      nb_free((void*)Info->PanelItem[Index].Description);
      nb_free((void*)Info->PanelItem[Index].Owner);
      for (size_t CustomIndex = 0; CustomIndex < Info->PanelItem[Index].CustomColumnNumber; ++CustomIndex)
      {
        nb_free((void*)Info->PanelItem[Index].CustomColumnData[CustomIndex]);
      }
      nb_free((void*)Info->PanelItem[Index].CustomColumnData);
    }
    nb_free(Info->PanelItem);
  }
}

intptr_t TCustomFarFileSystem::ProcessHostFile(const struct ProcessHostFileInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  bool Result = ProcessHostFileEx(PanelItems.get(), Info->OpMode);
  return Result;
}

intptr_t TCustomFarFileSystem::ProcessPanelInput(const struct ProcessPanelInputInfo * Info)
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

intptr_t TCustomFarFileSystem::ProcessPanelEvent(intptr_t Event, void * Param)
{
  ResetCachedInfo();
  return ProcessPanelEventEx(Event, Param);
}

intptr_t TCustomFarFileSystem::SetDirectory(const struct SetDirectoryInfo * Info)
{
  ResetCachedInfo();
  InvalidateOpenPanelInfo();
  intptr_t Result = SetDirectoryEx(Info->Dir, Info->OpMode);
  InvalidateOpenPanelInfo();
  return Result;
}

intptr_t TCustomFarFileSystem::MakeDirectory(struct MakeDirectoryInfo * Info)
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
  Result = MakeDirectoryEx(FNameStr, Info->OpMode);

  return Result;
}

intptr_t TCustomFarFileSystem::DeleteFiles(const struct DeleteFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  bool Result = DeleteFilesEx(PanelItems.get(), Info->OpMode);
  return Result;
}

intptr_t TCustomFarFileSystem::GetFiles(struct GetFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  intptr_t Result = 0;
  FDestPathStr = Info->DestPath;
  {
    SCOPE_EXIT
    {
      if (FDestPathStr != Info->DestPath)
      {
        Info->DestPath = FDestPathStr.c_str();
      }
    };
    Result = GetFilesEx(PanelItems.get(), Info->Move > 0, FDestPathStr, Info->OpMode);
  }

  return Result;
}

intptr_t TCustomFarFileSystem::PutFiles(const struct PutFilesInfo * Info)
{
  ResetCachedInfo();
  intptr_t Result = 0;
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, Info->ItemsNumber));
  Result = PutFilesEx(PanelItems.get(), Info->Move > 0, Info->OpMode);
  return Result;
}

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

TFarPanelInfo * const * TCustomFarFileSystem::GetPanelInfo(int Another) const
{
  return const_cast<TCustomFarFileSystem *>(this)->GetPanelInfo(Another);
}

TFarPanelInfo ** TCustomFarFileSystem::GetPanelInfo(int Another)
{
  bool bAnother = Another != 0;
  if (FPanelInfo[bAnother] == nullptr)
  {
    PanelInfo * Info = nb::calloc<PanelInfo*>(sizeof(PanelInfo));
    Info->StructSize = sizeof(PanelInfo);
    bool Res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, reinterpret_cast<void *>(Info),
      !bAnother ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!Res)
    {
      DebugAssert(false);
    }
    FPanelInfo[bAnother] = new TFarPanelInfo(Info, !bAnother ? this : nullptr);
  }
  return &FPanelInfo[bAnother];
}

intptr_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}

intptr_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}

bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  uintptr_t PrevInstances = FInstances;
  InvalidateOpenPanelInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, 0, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}

void TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, nullptr, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}

void TCustomFarFileSystem::ClosePanel()
{
  FClosed = true;
  FarControl(FCTL_CLOSEPANEL, 0, 0);
}

UnicodeString TCustomFarFileSystem::GetMsg(intptr_t MsgId) const
{
  return FPlugin->GetMsg(MsgId);
}

TCustomFarFileSystem * TCustomFarFileSystem::GetOppositeFileSystem()
{
  return FPlugin->GetPanelFileSystem(true, this);
}

bool TCustomFarFileSystem::IsActiveFileSystem() const
{
  // Cannot use PanelInfo::Focus as it occasionally does not work from editor;
  return (this == FPlugin->GetPanelFileSystem());
}

bool TCustomFarFileSystem::IsLeft() const
{
  return ((*GetPanelInfo(0))->GetBounds().Left <= 0);
}

bool TCustomFarFileSystem::IsRight() const
{
  return !IsLeft();
}

bool TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /*PanelItems*/, OPERATION_MODES /*OpMode*/)
{
  return false;
}

bool TCustomFarFileSystem::ProcessKeyEx(intptr_t /*Key*/, uintptr_t /*ControlState*/)
{
  return false;
}

bool TCustomFarFileSystem::ProcessPanelEventEx(intptr_t /*Event*/, void * /*Param*/)
{
  return false;
}

bool TCustomFarFileSystem::SetDirectoryEx(const UnicodeString & /*Dir*/, OPERATION_MODES /*OpMode*/)
{
  return false;
}

intptr_t TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /*Name*/, OPERATION_MODES /*OpMode*/)
{
  return -1;
}

bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /*PanelItems*/, OPERATION_MODES /*OpMode*/)
{
  return false;
}

intptr_t TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
  UnicodeString & /*DestPath*/, OPERATION_MODES /*OpMode*/)
{
  return 0;
}

intptr_t TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
  bool /*Move*/, OPERATION_MODES /*OpMode*/)
{
  return 0;
}

TObjectList * TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, size_t ItemsNumber)
{
  std::unique_ptr<TObjectList> PanelItems(new TObjectList());
  PanelItems->SetOwnsObjects(true);
  for (size_t Index = 0; Index < ItemsNumber; ++Index)
  {
    PanelItems->Add(new TFarPanelItem(&PanelItem[Index], false));
  }
  return PanelItems.release();
}

TFarPanelModes::TFarPanelModes() : TObject(),
  FReferenced(false)
{
  ::ZeroMemory(&FPanelModes, sizeof(FPanelModes));
}

TFarPanelModes::~TFarPanelModes()
{
  if (!FReferenced)
  {
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(_countof(FPanelModes)); ++Index)
    {
      ClearPanelMode(FPanelModes[Index]);
    }
  }
}

void TFarPanelModes::SetPanelMode(size_t Mode, const UnicodeString & ColumnTypes,
  const UnicodeString & ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, const UnicodeString & StatusColumnTypes,
  const UnicodeString & StatusColumnWidths)
{
  intptr_t ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  DebugAssert(Mode != NPOS && Mode < _countof(FPanelModes));
  DebugAssert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  wchar_t ** Titles = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + ColumnTypesCount));
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

void TFarPanelModes::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  DebugAssert(Info);
  Info->PanelModesNumber = _countof(FPanelModes);
  PanelMode * PanelModesArray = nb::calloc<PanelMode*>(sizeof(PanelMode) * _countof(FPanelModes));
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}

intptr_t TFarPanelModes::CommaCount(const UnicodeString & ColumnTypes)
{
  intptr_t Count = 0;
  for (intptr_t Index = 1; Index <= ColumnTypes.Length(); ++Index)
  {
    if (ColumnTypes[Index] == L',')
    {
      Count++;
    }
  }
  return Count;
}

TFarKeyBarTitles::TFarKeyBarTitles() :
  FReferenced(false)
{
  ClearStruct(FKeyBarTitles);
  FKeyBarTitles.CountLabels = 7 * 12;
  FKeyBarTitles.Labels = static_cast<KeyBarLabel *>(
    nb_malloc(sizeof(KeyBarLabel) * 7 * 12));
  memset(FKeyBarTitles.Labels, 0, sizeof(KeyBarLabel) * 7 * 12);
}

TFarKeyBarTitles::~TFarKeyBarTitles()
{
  if (!FReferenced)
  {
    ClearKeyBarTitles(FKeyBarTitles);
  }
}

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

void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  intptr_t FunctionKey, const UnicodeString & Title)
{
  DebugAssert(FunctionKey >= 1 && FunctionKey <= 12);
  int shift = static_cast<int>(ShiftStatus);
  DebugAssert(shift >= 0 && shift < 7);
  KeyBarLabel *Labels = &FKeyBarTitles.Labels[shift * 12];
  if (Labels[FunctionKey-1].Key.VirtualKeyCode)
  if (Labels[FunctionKey - 1].Key.VirtualKeyCode)
  {
    nb_free((void*)Labels[FunctionKey - 1].Text);
    nb_free((void*)Labels[FunctionKey - 1].LongText);
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
  Labels[FunctionKey - 1].Key.VirtualKeyCode = VK_F1 + (WORD)FunctionKey - 1;
  Labels[FunctionKey - 1].Key.ControlKeyState = FKeys[shift];
  Labels[FunctionKey - 1].Text = TCustomFarPlugin::DuplicateStr(Title, /*AllowEmpty=*/true);
  Labels[FunctionKey - 1].LongText = nullptr;
}

void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (size_t Index = 0; Index < Titles.CountLabels; ++Index)
  {
    nb_free((void*)Titles.Labels[Index].Text);
    nb_free((void*)Titles.Labels[Index].LongText);
  }
  nb_free(Titles.Labels);
  Titles.Labels = nullptr;
  Titles.CountLabels = 0;
}

void TFarKeyBarTitles::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  DebugAssert(Info);
  KeyBarTitles * KeyBar = nb::calloc<KeyBarTitles *>(sizeof(KeyBarTitles));
  Info->KeyBar = KeyBar;
  memmove(KeyBar, &FKeyBarTitles, sizeof(FKeyBarTitles));
  FReferenced = true;
}

UnicodeString TCustomFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  DebugAssert(false);
  return L"";
}

void TCustomFarPanelItem::FillPanelItem(struct PluginPanelItem * PanelItem)
{
  DebugAssert(PanelItem);

  UnicodeString FileName;
  int64_t Size = 0;
  TDateTime LastWriteTime;
  TDateTime LastAccess;
  UnicodeString Description;
  UnicodeString Owner;

  void * UserData = PanelItem->UserData.Data;
  GetData(PanelItem->Flags, FileName, Size, PanelItem->FileAttributes,
    LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
    UserData, PanelItem->CustomColumnNumber);
  PanelItem->UserData.Data = UserData;
  FILETIME FileTime = ::DateTimeToFileTime(LastWriteTime, dstmWin);
  FILETIME FileTimeA = ::DateTimeToFileTime(LastAccess, dstmWin);
  PanelItem->CreationTime = FileTime;
  PanelItem->LastAccessTime = FileTimeA;
  PanelItem->LastWriteTime = FileTime;
  PanelItem->FileSize = Size;

  // PanelItem->FileName = wcscpy(new wchar_t[FileName.Length() + 1], FileName.c_str());
  // PanelItem->AlternateFileName = wcscpy(new wchar_t[FileName.Length() + 1], FileName.c_str());
  PanelItem->FileName = TCustomFarPlugin::DuplicateStr(FileName);
  PanelItem->AlternateFileName = TCustomFarPlugin::DuplicateStr(FileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  wchar_t ** CustomColumnData = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + PanelItem->CustomColumnNumber));
  for (size_t Index = 0; Index < PanelItem->CustomColumnNumber; ++Index)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
}

TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem) :
  TCustomFarPanelItem(OBJECT_CLASS_TFarPanelItem),
  FPanelItem(APanelItem),
  FOwnsItem(OwnsItem)
{
  DebugAssert(FPanelItem);
}

TFarPanelItem::~TFarPanelItem()
{
  if (FOwnsItem)
    nb_free(FPanelItem);
  FPanelItem = nullptr;
}

void TFarPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & /*FileName*/, int64_t & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  DebugAssert(false);
}

UnicodeString TFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  DebugAssert(false);
  return L"";
}

PLUGINPANELITEMFLAGS TFarPanelItem::GetFlags() const
{
  return static_cast<uintptr_t>(FPanelItem->Flags);
}

UnicodeString TFarPanelItem::GetFileName() const
{
  UnicodeString Result = FPanelItem->FileName;
  return Result;
}

void * TFarPanelItem::GetUserData() const
{
  return FPanelItem->UserData.Data;
}

bool TFarPanelItem::GetSelected() const
{
  return (FPanelItem->Flags & PPIF_SELECTED) != 0;
}

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

uintptr_t TFarPanelItem::GetFileAttrs() const
{
  return static_cast<uintptr_t>(FPanelItem->FileAttributes);
}

bool TFarPanelItem::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}

bool TFarPanelItem::GetIsFile() const
{
  return (GetFileAttrs() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

THintPanelItem::THintPanelItem(const UnicodeString & AHint) :
  TCustomFarPanelItem(OBJECT_CLASS_THintPanelItem),
  FHint(AHint)
{
}

void THintPanelItem::GetData(
  PLUGINPANELITEMFLAGS & /*Flags*/, UnicodeString & AFileName, int64_t & /*Size*/,
  uintptr_t & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  uintptr_t & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, size_t & /*CustomColumnNumber*/)
{
  AFileName = FHint;
}

TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner) :
  TObject(),
  FPanelInfo(APanelInfo),
  FItems(nullptr),
  FOwner(AOwner)
{
  // if (!FPanelInfo) throw ExtException(L"");
  DebugAssert(FPanelInfo);
}

TFarPanelInfo::~TFarPanelInfo()
{
  nb_free(FPanelInfo);
  SAFE_DESTROY(FItems);
}

intptr_t TFarPanelInfo::GetItemCount() const
{
  return static_cast<intptr_t>(FPanelInfo->ItemsNumber);
}

TRect TFarPanelInfo::GetBounds() const
{
  RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}

intptr_t TFarPanelInfo::GetSelectedCount(bool CountCurrentItem) const
{
  intptr_t Count = static_cast<intptr_t>(FPanelInfo->SelectedItemsNumber);

  if ((Count == 1) && FOwner && !CountCurrentItem)
  {
    intptr_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, 0);
    PluginPanelItem * ppi = nb::calloc<PluginPanelItem *>(size);
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, reinterpret_cast<void *>(ppi));
    if ((ppi->Flags & PPIF_SELECTED) == 0)
    {
      Count = 0;
    }
    nb_free(ppi);
  }

  return Count;
}

TObjectList * TFarPanelInfo::GetItems()
{
  if (!FItems)
  {
    FItems = new TObjectList();
  }
  if (FOwner)
  {
    // DebugAssert(FItems->GetCount() == 0);
    if (!FItems->GetCount())
      FItems->Clear();
    for (size_t Index = 0; Index < FPanelInfo->ItemsNumber; ++Index)
    {
      TODO("move to common function");
      intptr_t Size = FOwner->FarControl(FCTL_GETPANELITEM, Index, 0);
      PluginPanelItem * ppi = nb::calloc<PluginPanelItem *>(Size);
      FarGetPluginPanelItem gppi;
      ClearStruct(gppi);
      gppi.StructSize = sizeof(FarGetPluginPanelItem);
      gppi.Size = Size;
      gppi.Item = ppi;
      FOwner->FarControl(FCTL_GETPANELITEM, Index, static_cast<void *>(&gppi));
      FItems->Add(new TFarPanelItem(ppi, /*OwnsItem=*/true));
    }
  }
  return FItems;
}

TFarPanelItem * TFarPanelInfo::FindFileName(const UnicodeString & AFileName) const
{
  const TObjectList * Items = FItems;
  if (!Items)
    Items = GetItems();
  for (intptr_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = Items->GetAs<TFarPanelItem>(Index);
    if (PanelItem->GetFileName() == AFileName)
    {
      return PanelItem;
    }
  }
  return nullptr;
}

const TFarPanelItem * TFarPanelInfo::FindUserData(const void * UserData) const
{
  return const_cast<TFarPanelInfo *>(this)->FindUserData(UserData);
}

TFarPanelItem * TFarPanelInfo::FindUserData(const void * UserData)
{
  TObjectList * Items = GetItems();
  for (intptr_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = Items->GetAs<TFarPanelItem>(Index);
    if (PanelItem->GetUserData() == UserData)
    {
      return PanelItem;
    }
  }
  return nullptr;
}

void TFarPanelInfo::ApplySelection()
{
  // for "another panel info", there's no owner
  DebugAssert(FOwner != nullptr);
  FOwner->FarControl(FCTL_SETSELECTION, 0, reinterpret_cast<void *>(FPanelInfo));
}

TFarPanelItem * TFarPanelInfo::GetFocusedItem() const
{
  const TObjectList * Items = FItems;
  if (!Items)
    Items = GetItems();
  intptr_t Index = GetFocusedIndex();
  if (Items->GetCount() > 0)
  {
    DebugAssert(Index < Items->GetCount());
    return Items->GetAs<TFarPanelItem>(Index);
  }
  else
  {
    return nullptr;
  }
}

void TFarPanelInfo::SetFocusedItem(const TFarPanelItem * Value)
{
  if (FItems && FItems->GetCount())
  {
    intptr_t Index = FItems->IndexOf(Value);
    DebugAssert(Index != NPOS);
    SetFocusedIndex(Index);
  }
}

intptr_t TFarPanelInfo::GetFocusedIndex() const
{
  return static_cast<intptr_t>(FPanelInfo->CurrentItem);
}

void TFarPanelInfo::SetFocusedIndex(intptr_t Value)
{
  // for "another panel info", there's no owner
  DebugAssert(FOwner != nullptr);
  if (GetFocusedIndex() != Value)
  {
    DebugAssert(Value != NPOS && Value < (intptr_t)FPanelInfo->ItemsNumber);
    FPanelInfo->CurrentItem = static_cast<int>(Value);
    PanelRedrawInfo PanelInfo;
    ClearStruct(PanelInfo);
    PanelInfo.StructSize = sizeof(PanelRedrawInfo);
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, reinterpret_cast<void *>(&PanelInfo));
  }
}

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
    DebugAssert(false);
    return ptFile;
  }
}

bool TFarPanelInfo::GetIsPlugin() const
{
  return ((FPanelInfo->PluginHandle != INVALID_HANDLE_VALUE) && (FPanelInfo->PluginHandle != 0));
}

UnicodeString TFarPanelInfo::GetCurrDirectory() const
{
  UnicodeString Result;
  intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
    0,
    0,
    FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    FarPanelDirectory * pfpd = nb::calloc<FarPanelDirectory *>(Size);
    pfpd->StructSize = sizeof(FarPanelDirectory);

    FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
      Size,
      pfpd,
      FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
    Result = pfpd->Name;
    nb_free(pfpd);
  }
  return Result;
}

TFarMenuItems::TFarMenuItems() :
  TStringList(OBJECT_CLASS_TFarMenuItems),
  FItemFocused(NPOS)
{
}

void TFarMenuItems::Clear()
{
  FItemFocused = NPOS;
  TStringList::Clear();
}

void TFarMenuItems::Delete(intptr_t Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = NPOS;
  }
  TStringList::Delete(Index);
}

void TFarMenuItems::SetObj(intptr_t Index, TObject * AObject)
{
  TStringList::SetObj(Index, AObject);
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

intptr_t TFarMenuItems::Add(const UnicodeString & Text, bool Visible)
{
  intptr_t Result = TStringList::Add(Text);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
  return Result;
}

void TFarMenuItems::AddSeparator(bool Visible)
{
  Add(L"");
  SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
}

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

void TFarMenuItems::SetFlag(intptr_t Index, uintptr_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    uintptr_t F = reinterpret_cast<uintptr_t>(GetObj(Index));
    if (Value)
    {
      F |= Flag;
    }
    else
    {
      F &= ~Flag;
    }
    SetObj(Index, reinterpret_cast<TObject *>(F));
  }
}

bool TFarMenuItems::GetFlag(intptr_t Index, uintptr_t Flag) const
{
  return (reinterpret_cast<uintptr_t>(GetObj(Index)) & Flag) > 0;
}

TFarEditorInfo::TFarEditorInfo(EditorInfo * Info) :
  FEditorInfo(Info)
{
}

TFarEditorInfo::~TFarEditorInfo()
{
  nb_free(FEditorInfo);
}

intptr_t TFarEditorInfo::GetEditorID() const
{
  return static_cast<intptr_t>(FEditorInfo->EditorID);
}

UnicodeString TFarEditorInfo::GetFileName()
{
  UnicodeString Result;
  intptr_t BuffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, 0, 0);
  if (BuffLen)
  {
    wchar_t * Buffer = Result.SetLength(BuffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, BuffLen, Buffer);
  }
  return Result.c_str();
}

TFarEnvGuard::TFarEnvGuard()
{
  DebugAssert(FarPlugin != nullptr);
}

TFarEnvGuard::~TFarEnvGuard()
{
  DebugAssert(FarPlugin != nullptr);
  /*
  if (!FarPlugin->GetANSIApis())
  {
      DebugAssert(!AreFileApisANSI());
      SetFileApisToANSI();
  }
  else
  {
      DebugAssert(AreFileApisANSI());
  }
  */
}

TFarPluginEnvGuard::TFarPluginEnvGuard()
{
  DebugAssert(FarPlugin != nullptr);
}

TFarPluginEnvGuard::~TFarPluginEnvGuard()
{
  DebugAssert(FarPlugin != nullptr);
}

HINSTANCE TGlobalFunctions::GetInstanceHandle() const
{
  HINSTANCE Result = nullptr;
  if (FarPlugin)
  {
    Result = FarPlugin->GetHandle();
  }
  return Result;
}

UnicodeString TGlobalFunctions::GetMsg(intptr_t Id) const
{
//  HINSTANCE hInstance = GetGlobalFunctions()->GetInstanceHandle();
//  intptr_t Length = ::LoadString(hInstance, static_cast<UINT>(Id),
//    const_cast<wchar_t *>(Fmt.c_str()), static_cast<int>(Fmt.GetLength()));
//  if (!Length)
//  {
//    DEBUG_PRINTF(L"Unknown resource string id: %d\n", Id);
//  }
  // map Id to PluginString value
  intptr_t PluginStringId = Id;
  const TFarPluginStrings * CurFarPluginStrings = &FarPluginStrings[0];
  while (CurFarPluginStrings && CurFarPluginStrings->Id)
  {
    if (CurFarPluginStrings->Id == Id)
    {
      PluginStringId = CurFarPluginStrings->FarPluginStringId;
      break;
    }
    ++CurFarPluginStrings;
  }
  DebugAssert(FarPlugin != nullptr);
  return FarPlugin->GetMsg(PluginStringId);
}

UnicodeString TGlobalFunctions::GetCurrDirectory() const
{
  UnicodeString Result;
  UnicodeString Path(NB_MAX_PATH, 0);
  int Length = 0;
  if (FarPlugin)
  {
    Length = (int)FarPlugin->GetFarStandardFunctions().GetCurrentDirectory((DWORD)Path.Length(), (wchar_t *)Path.c_str()) - 1;
  }
  else
  {
    Length = ::GetCurrentDirectory((DWORD)Path.Length(), (wchar_t *)Path.c_str());
  }
  Result = UnicodeString(Path.c_str(), Length);
  return Result;
}

UnicodeString TGlobalFunctions::GetStrVersionNumber() const
{
  return NETBOX_VERSION_NUMBER.c_str();
}

//bool InputBox(const UnicodeString & Title, const UnicodeString & Prompt,
//  UnicodeString & Text, DWORD Flags, const UnicodeString & HistoryName = UnicodeString(),
//  intptr_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = nullptr);
bool TGlobalFunctions::InputDialog(const UnicodeString & ACaption, const UnicodeString & APrompt,
                                   UnicodeString & Value, const UnicodeString & HelpKeyword,
                                   TStrings * History, bool PathInput,
                                   TInputDialogInitializeEvent OnInitialize, bool Echo)
{
  DebugUsedParam(HelpKeyword);
  DebugUsedParam(History);
  DebugUsedParam(PathInput);
  DebugUsedParam(OnInitialize);
  DebugUsedParam(Echo);

  TWinSCPPlugin * WinSCPPlugin = dyn_cast<TWinSCPPlugin>(FarPlugin);
  return WinSCPPlugin->InputBox(ACaption, APrompt, Value, 0);
}

uintptr_t TGlobalFunctions::MoreMessageDialog(const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type, uintptr_t Answers, const TMessageParams * Params)
{
  TWinSCPPlugin * WinSCPPlugin = dyn_cast<TWinSCPPlugin>(FarPlugin);
  return WinSCPPlugin->MoreMessageDialog(Message, MoreMessages, Type, Answers, Params);
}

