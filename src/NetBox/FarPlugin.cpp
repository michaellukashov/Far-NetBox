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

intptr_t TCustomFarPlugin::GetMinFarVersion() const
{
  return 0;
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
          wchar_t ** StringArray = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + NAME.GetCount())); \
          FPluginInfo.NAME = StringArray; \
          FPluginInfo.NAME ## Number = static_cast<int>(NAME.GetCount()); \
          for (intptr_t Index = 0; Index < NAME.GetCount(); ++Index) \
          { \
            StringArray[Index] = DuplicateStr(NAME.GetString(Index)); \
          } \
        }

    COMPOSESTRINGARRAY(DiskMenuStrings);
    COMPOSESTRINGARRAY(PluginMenuStrings);
    COMPOSESTRINGARRAY(PluginConfigStrings);

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
      for (intptr_t Index = 0; Index < Info.NAME ## Number; ++Index) \
      { \
        nb_free((void *)Info.NAME[Index]); \
      } \
      nb_free((void *)Info.NAME); \
      Info.NAME = nullptr;

    FREESTRINGARRAY(DiskMenuStrings);
    FREESTRINGARRAY(PluginMenuStrings);
    FREESTRINGARRAY(PluginConfigStrings);

#undef FREESTRINGARRAY

    nb_free((void *)Info.CommandPrefix);
  }
  ClearStruct(Info);
  Info.StructSize = sizeof(Info);
}

wchar_t * TCustomFarPlugin::DuplicateStr(UnicodeString Str, bool AllowEmpty)
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
  FarControl(FCTL_GETPANELINFO, 0, ToInt(&Info), PanelHandle);
  RECT Bounds;
  ClearStruct(Bounds);
  if (Info.Plugin)
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

void TCustomFarPlugin::InvalidateOpenPluginInfo()
{
  for (intptr_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
  {
    TCustomFarFileSystem * FarFileSystem =
      FOpenedPlugins->GetAs<TCustomFarFileSystem>(Index);
    FarFileSystem->InvalidateOpenPluginInfo();
  }
}

intptr_t TCustomFarPlugin::Configure(intptr_t Item)
{
  try
  {
    ResetCachedInfo();
    intptr_t Result = ConfigureEx(Item);
    InvalidateOpenPluginInfo();

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return 0;
  }
}

void * TCustomFarPlugin::OpenPlugin(int OpenFrom, intptr_t Item)
{
#ifdef USE_DLMALLOC
  // dlmallopt(M_GRANULARITY, 128 * 1024);
#endif

  try
  {
    ResetCachedInfo();

    UnicodeString Buf;
    if ((OpenFrom == OPEN_SHORTCUT) || (OpenFrom == OPEN_COMMANDLINE))
    {
      Buf = reinterpret_cast<wchar_t *>(Item);
      Item = ToInt(Buf.c_str());
    }

    TCustomFarFileSystem * Result = OpenPluginEx(OpenFrom, Item);

    if (Result)
    {
      FOpenedPlugins->Add(Result);
    }
    else
    {
      Result = reinterpret_cast<TCustomFarFileSystem *>(INVALID_HANDLE_VALUE);
    }

    return Result;
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return INVALID_HANDLE_VALUE;
  }
}

void TCustomFarPlugin::ClosePlugin(void * Plugin)
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
  TCustomFarFileSystem * FarFileSystem, Exception * E, int OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API functions that must update
  // panel contents on themselves (like ProcessKey), the instance of filesystem
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

void TCustomFarPlugin::GetOpenPluginInfo(HANDLE Plugin,
  struct OpenPluginInfo * Info)
{
  if (!Info)
    return;
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  if (!FOpenedPlugins || !FarFileSystem)
    return;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);
    TGuard Guard(FarFileSystem->GetCriticalSection());
    FarFileSystem->GetOpenPluginInfo(Info);
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

intptr_t TCustomFarPlugin::GetFindData(HANDLE Plugin,
  struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFindData(PanelItem, ItemsNumber, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    return 0;
  }
}

void TCustomFarPlugin::FreeFindData(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      FarFileSystem->FreeFindData(PanelItem, ItemsNumber);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

intptr_t TCustomFarPlugin::ProcessHostFile(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessHostFile))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessHostFile(PanelItem, ItemsNumber, OpMode);
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
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessKey(HANDLE Plugin, int Key,
  DWORD ControlState)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessKey))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessKey(Key, ControlState);
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

intptr_t TCustomFarPlugin::ProcessEvent(HANDLE Plugin, int Event, void * Param)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    //ResetCachedInfo();
    if (HandlesFunction(hfProcessEvent))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

      UnicodeString Buf;
      if ((Event == FE_CHANGEVIEWMODE) || (Event == FE_COMMAND))
      {
        Buf = static_cast<wchar_t *>(Param);
        Param = const_cast<void *>(reinterpret_cast<const void *>(Buf.c_str()));
      }
      else if ((Event == FE_GOTFOCUS) || (Event == FE_KILLFOCUS))
      {
      }

      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->ProcessEvent(Event, Param);
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
    return Event == FE_COMMAND ? 1 : 0;
  }
}

intptr_t TCustomFarPlugin::SetDirectory(HANDLE Plugin, const wchar_t * Dir, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
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
      return FarFileSystem->SetDirectory(Dir, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    if (FarFileSystem->GetOpenPluginInfoValid() && !PrevCurrentDirectory.IsEmpty())
    {
      try
      {
        TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->SetDirectory(PrevCurrentDirectory.c_str(), OpMode);
      }
      catch (Exception &)
      {
        return 0;
      }
    }
    return 0;
  }
}

intptr_t TCustomFarPlugin::MakeDirectory(HANDLE Plugin, const wchar_t ** Name, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->MakeDirectory(Name, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::DeleteFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->DeleteFiles(PanelItem, ItemsNumber, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::GetFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int Move,
  const wchar_t ** DestPath, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFiles(PanelItem, ItemsNumber, Move, DestPath, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    // display error even for OPM_FIND
    HandleFileSystemException(FarFileSystem, &E, OpMode & ~OPM_FIND);
    return 0;
  }
}

intptr_t TCustomFarPlugin::PutFiles(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode)
{
  TCustomFarFileSystem * FarFileSystem = get_as<TCustomFarFileSystem>(Plugin);
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != NPOS);

    {
      TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->PutFiles(PanelItem, ItemsNumber, Move, srcPath, OpMode);
    }
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessEditorEvent(int Event, void * Param)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorEventEx(Event, Param);
  }
  catch (Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessEditorInput(const INPUT_RECORD * Rec)
{
  try
  {
    ResetCachedInfo();

    return ProcessEditorInputEx(Rec);
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
    UnicodeString Title, UnicodeString Message, TStrings * Buttons);

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
  UnicodeString Title, UnicodeString Message, TStrings * Buttons)
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
    Button->SetTag(ToInt(Buttons->GetObj(Index)));
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

intptr_t TCustomFarPlugin::DialogMessage(DWORD Flags,
  UnicodeString Title, UnicodeString Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  std::unique_ptr<TFarMessageDialog> Dialog(new TFarMessageDialog(this, Params));
  Dialog->Init(Flags, Title, Message, Buttons);
  intptr_t Result = Dialog->Execute(Params->CheckBox);
  return Result;
}

intptr_t TCustomFarPlugin::FarMessage(DWORD Flags,
  UnicodeString Title, UnicodeString Message, TStrings * Buttons,
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
    Items[Index] = const_cast<wchar_t *>(MessageLines->GetStringRef(Index).c_str());
  }

  TFarEnvGuard Guard;
  intptr_t Result = static_cast<intptr_t>(FStartupInfo.Message(FStartupInfo.ModuleNumber,
    Flags | FMSG_LEFTALIGN, nullptr, Items, static_cast<int>(MessageLines->GetCount()),
    static_cast<int>(Buttons->GetCount())));

  return Result;
}

intptr_t TCustomFarPlugin::Message(DWORD Flags,
  UnicodeString Title, UnicodeString Message, TStrings * Buttons,
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
    Result = static_cast<intptr_t>(FStartupInfo.Message(FStartupInfo.ModuleNumber,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      nullptr,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0));
  }
  return Result;
}

intptr_t TCustomFarPlugin::Menu(DWORD Flags, UnicodeString Title,
  UnicodeString Bottom, const FarMenuItem * Items, intptr_t Count,
  const int * BreakKeys, int & BreakCode)
{
  DebugAssert(Items);

  TFarEnvGuard Guard;
  return static_cast<intptr_t>(FStartupInfo.Menu(FStartupInfo.ModuleNumber, -1, -1, 0,
    Flags, Title.c_str(), Bottom.c_str(), nullptr, BreakKeys,
    &BreakCode, Items, static_cast<int>(Count)));
}

intptr_t TCustomFarPlugin::Menu(DWORD Flags, UnicodeString Title,
  UnicodeString Bottom, TStrings * Items, const int * BreakKeys,
  int & BreakCode)
{
  DebugAssert(Items && Items->GetCount());
  intptr_t Result = 0;
  FarMenuItemEx * MenuItems = nb::calloc<FarMenuItemEx*>(sizeof(FarMenuItemEx) * (1 + Items->GetCount()));
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
      MenuItems[Count].Text = Items->GetStringRef(Index).c_str();
      MenuItems[Count].UserData = Index;
      Count++;
    }
  }

  intptr_t ResultItem = Menu(Flags | FMENU_USEEXT, Title, Bottom,
    reinterpret_cast<const FarMenuItem *>(MenuItems), Count, BreakKeys, BreakCode);

  if (ResultItem >= 0)
  {
    Result = MenuItems[ResultItem].UserData;
    if (Selected >= 0)
    {
      Items->SetObj(Selected, ToObj(ToInt(Items->GetObj(Selected)) & ~MIF_SELECTED));
    }
    Items->SetObj(Result, ToObj(ToInt(Items->GetObj(Result)) | MIF_SELECTED));
  }
  else
  {
    Result = ResultItem;
  }
  return Result;
}

intptr_t TCustomFarPlugin::Menu(DWORD Flags, UnicodeString Title,
  UnicodeString Bottom, TStrings * Items)
{
  int BreakCode;
  return Menu(Flags, Title, Bottom, Items, nullptr, BreakCode);
}

bool TCustomFarPlugin::InputBox(UnicodeString Title,
  UnicodeString Prompt, UnicodeString & Text, DWORD Flags,
  UnicodeString HistoryName, intptr_t MaxLen, TFarInputBoxValidateEvent OnValidate)
{
  bool Repeat = false;
  int Result = 0;
  do
  {
    UnicodeString DestText;
    DestText.SetLength(MaxLen + 1);
    HANDLE ScreenHandle = nullptr;
    SaveScreen(ScreenHandle);
    {
      TFarEnvGuard Guard;
      Result = FStartupInfo.InputBox(
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

void TCustomFarPlugin::Text(int X, int Y, int Color, UnicodeString Str)
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(X, Y, Color, Str.c_str());
}

void TCustomFarPlugin::FlushText()
{
  TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, 0, nullptr);
}

void TCustomFarPlugin::FarWriteConsole(UnicodeString Str)
{
  DWORD Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), static_cast<DWORD>(Str.Length()), &Written, nullptr);
}

void TCustomFarPlugin::FarCopyToClipboard(UnicodeString Str)
{
  TFarEnvGuard Guard;
  FFarStandardFunctions.CopyToClipboard(Str.c_str());
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
    FarAdvControl(ACTL_GETFARRECT, &BufferInfo.srWindow);

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
        COORD Size = {static_cast<short>(FNormalConsoleSize.x), static_cast<short>(FNormalConsoleSize.y)};

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

void TCustomFarPlugin::ShowConsoleTitle(UnicodeString Title)
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
  if (FSavedTitles->GetCount() > 0)
  {
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
      UpdateProgress(PS_NOPROGRESS, 0);
    }
    {
      TObject * Obj = FSavedTitles->GetObj(FSavedTitles->GetCount() - 1);
      SAFE_DESTROY(Obj);
    }
    FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
  }
}

void TCustomFarPlugin::UpdateConsoleTitle(UnicodeString Title)
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

void TCustomFarPlugin::UpdateProgress(intptr_t State, intptr_t Progress) const
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, ToPtr(State));
  if (State == PS_NORMAL)
  {
    PROGRESSVALUE pv;
    pv.Completed = Progress < 0 ? 0 : Progress > 100 ? 100 : Progress;
    pv.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, static_cast<void *>(&pv));
  }
}

void TCustomFarPlugin::UpdateCurrentConsoleTitle()
{
  UnicodeString Title = FormatConsoleTitle();
  ::SetConsoleTitle(Title.c_str());
  short progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(progress != 0 ? PS_NORMAL : PS_NOPROGRESS, progress);
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

void TCustomFarPlugin::HandleException(Exception * E, int /*OpMode*/)
{
  DebugAssert(E);
  Message(FMSG_WARNING | FMSG_MB_OK, L"", E ? E->Message : L"");
}

UnicodeString TCustomFarPlugin::GetMsg(intptr_t MsgId) const
{
  TFarEnvGuard Guard;
  UnicodeString Result = FStartupInfo.GetMsg(FStartupInfo.ModuleNumber, (int)MsgId);
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

bool TCustomFarPlugin::Viewer(UnicodeString AFileName,
  UnicodeString Title, DWORD Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Viewer(
    AFileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags,
    CP_AUTODETECT);
  return Result > 0;
}

bool TCustomFarPlugin::Editor(UnicodeString AFileName,
  UnicodeString Title, DWORD Flags)
{
  TFarEnvGuard Guard;
  int Result = FStartupInfo.Editor(
    AFileName.c_str(),
    Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
    CP_AUTODETECT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}

void TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}

intptr_t TCustomFarPlugin::GetFarSystemSettings() const
{
  if (!FValidFarSystemSettings)
  {
    FFarSystemSettings = FarAdvControl(ACTL_GETSYSTEMSETTINGS);
    FValidFarSystemSettings = true;
  }
  return FFarSystemSettings;
}

intptr_t TCustomFarPlugin::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin)
{
  switch (Command)
  {
  case FCTL_CLOSEPLUGIN:
  case FCTL_SETPANELDIR:
  case FCTL_SETCMDLINE:
  case FCTL_INSERTCMDLINE:
    break;

  case FCTL_GETCMDLINE:
  case FCTL_GETCMDLINESELECTEDTEXT:
    // ANSI/OEM translation not implemented yet
    DebugAssert(false);
    break;
  }

  TFarEnvGuard Guard;
  return FStartupInfo.Control(Plugin, static_cast<int>(Command), static_cast<int>(Param1), Param2);
}

intptr_t TCustomFarPlugin::FarAdvControl(uintptr_t Command, void * Param) const
{
  TFarEnvGuard Guard;
  return FStartupInfo.AdvControl(FStartupInfo.ModuleNumber, static_cast<int>(Command), Param);
}

intptr_t TCustomFarPlugin::FarEditorControl(uintptr_t Command, void * Param)
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
  return static_cast<intptr_t>(FStartupInfo.EditorControl(static_cast<int>(Command), Param));
}

TFarEditorInfo * TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result = nullptr;
  ::EditorInfo * Info = nb::calloc<::EditorInfo *>(sizeof(::EditorInfo));
  try
  {
    if (FarEditorControl(ECTL_GETINFO, Info))
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
    FFarVersion = FarAdvControl(ACTL_GETFARVERSION);
  }
  return FFarVersion;
}

UnicodeString TCustomFarPlugin::FormatFarVersion(intptr_t Version) const
{
  return FORMAT(L"%d.%d.%d", (Version >> 8) & 0xFF, Version & 0xFF, Version >> 16);
}

UnicodeString TCustomFarPlugin::GetTemporaryDir() const
{
  UnicodeString Result(NB_MAX_PATH, 0);
  TFarEnvGuard Guard;
  FFarStandardFunctions.MkTemp(const_cast<wchar_t *>(Result.c_str()), (DWORD)Result.Length(), nullptr);
  PackStr(Result);
  return Result;
}

intptr_t TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * Rec)
{
  int Result;
  if (FFarStandardFunctions.FarInputRecordToKey != nullptr)
  {
    TFarEnvGuard Guard;
    Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
  }
  else
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
  FOpenPluginInfoValid(false)
{
  ClearArray(FPanelInfo);
  ClearStruct(FOpenPluginInfo);
}

void TCustomFarFileSystem::Init()
{
  FPanelInfo[0] = nullptr;
  FPanelInfo[1] = nullptr;
  FClosed = false;

  ClearStruct(FOpenPluginInfo);
  ClearOpenPluginInfo(FOpenPluginInfo);
  FInstances++;
}

TCustomFarFileSystem::~TCustomFarFileSystem()
{
  FInstances--;
  ResetCachedInfo();
  ClearOpenPluginInfo(FOpenPluginInfo);
}

void TCustomFarFileSystem::HandleException(Exception * E, int OpMode)
{
  DEBUG_PRINTF("before FPlugin->HandleException");
  FPlugin->HandleException(E, OpMode);
}

void TCustomFarFileSystem::Close()
{
  FClosed = true;
}

void TCustomFarFileSystem::InvalidateOpenPluginInfo()
{
  FOpenPluginInfoValid = false;
}

void TCustomFarFileSystem::ClearOpenPluginInfo(OpenPluginInfo & Info)
{
  if (Info.StructSize)
  {
    nb_free((void *)Info.HostFile);
    nb_free((void *)Info.CurDir);
    nb_free((void *)Info.Format);
    nb_free((void *)Info.PanelTitle);
    DebugAssert(!Info.InfoLines);
    DebugAssert(!Info.InfoLinesNumber);
    DebugAssert(!Info.DescrFiles);
    DebugAssert(!Info.DescrFilesNumber);
    DebugAssert(Info.PanelModesNumber == 0 || Info.PanelModesNumber == PANEL_MODES_COUNT);
    for (intptr_t Index = 0; Index < Info.PanelModesNumber; ++Index)
    {
      DebugAssert(Info.PanelModesArray);
      TFarPanelModes::ClearPanelMode(
        const_cast<PanelMode &>(Info.PanelModesArray[Index]));
    }
    nb_free((void *)Info.PanelModesArray);
    if (Info.KeyBar)
    {
      TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
      nb_free((void *)Info.KeyBar);
    }
    nb_free((void *)Info.ShortcutData);
  }
  ClearStruct(Info);
  Info.StructSize = sizeof(Info);
  InvalidateOpenPluginInfo();
}

void TCustomFarFileSystem::GetOpenPluginInfo(struct OpenPluginInfo * Info)
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
      UnicodeString HostFile, CurDir, Format, PanelTitle, ShortcutData;
      std::unique_ptr<TFarPanelModes> PanelModes(new TFarPanelModes());
      std::unique_ptr<TFarKeyBarTitles> KeyBarTitles(new TFarKeyBarTitles());
      bool StartSortOrder = false;

      GetOpenPluginInfoEx(FOpenPluginInfo.Flags, HostFile, CurDir, Format,
        PanelTitle, PanelModes.get(), FOpenPluginInfo.StartPanelMode,
        FOpenPluginInfo.StartSortMode, StartSortOrder, KeyBarTitles.get(), ShortcutData);

      FOpenPluginInfo.HostFile = TCustomFarPlugin::DuplicateStr(HostFile);
      FOpenPluginInfo.CurDir = TCustomFarPlugin::DuplicateStr(::StringReplaceAll(CurDir, L"\\", L"/"));
      FOpenPluginInfo.Format = TCustomFarPlugin::DuplicateStr(Format);
      FOpenPluginInfo.PanelTitle = TCustomFarPlugin::DuplicateStr(PanelTitle);
      PanelModes->FillOpenPluginInfo(&FOpenPluginInfo);
      FOpenPluginInfo.StartSortOrder = StartSortOrder;
      KeyBarTitles->FillOpenPluginInfo(&FOpenPluginInfo);
      FOpenPluginInfo.ShortcutData = TCustomFarPlugin::DuplicateStr(ShortcutData);

      FOpenPluginInfoValid = true;
    }

    memmove(Info, &FOpenPluginInfo, sizeof(FOpenPluginInfo));
  }
}

intptr_t TCustomFarFileSystem::GetFindData(
  struct PluginPanelItem ** PanelItem, int * ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(new TObjectList());
  bool Result = !FClosed && GetFindDataEx(PanelItems.get(), OpMode);
  if (Result && PanelItems->GetCount())
  {
    *PanelItem = nb::calloc<PluginPanelItem*>(sizeof(PluginPanelItem) * PanelItems->GetCount());
    *ItemsNumber = static_cast<int>(PanelItems->GetCount());
    for (intptr_t Index = 0; Index < PanelItems->GetCount(); ++Index)
    {
      PanelItems->GetAs<TCustomFarPanelItem>(Index)->FillPanelItem(
        &((*PanelItem)[Index]));
    }
  }
  else
  {
    *PanelItem = nullptr;
    *ItemsNumber = 0;
  }

  return Result;
}

void TCustomFarFileSystem::FreeFindData(
  struct PluginPanelItem * PanelItem, int ItemsNumber)
{
  ResetCachedInfo();
  if (PanelItem)
  {
    DebugAssert(ItemsNumber > 0);
    for (intptr_t Index = 0; Index < ItemsNumber; ++Index)
    {
      nb_free((void *)PanelItem[Index].FindData.lpwszFileName);
      nb_free((void *)PanelItem[Index].Description);
      nb_free((void *)PanelItem[Index].Owner);
      for (intptr_t CustomIndex = 0; CustomIndex < PanelItem[Index].CustomColumnNumber; ++CustomIndex)
      {
        nb_free((void *)PanelItem[Index].CustomColumnData[CustomIndex]);
      }
      nb_free((void *)PanelItem[Index].CustomColumnData);
    }
    nb_free(PanelItem);
  }
}

intptr_t TCustomFarFileSystem::ProcessHostFile(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(PanelItem, ItemsNumber));
  bool Result = ProcessHostFileEx(PanelItems.get(), OpMode);
  return Result;
}

intptr_t TCustomFarFileSystem::ProcessKey(intptr_t Key, uintptr_t ControlState)
{
  ResetCachedInfo();
  return ProcessKeyEx(Key, ControlState);
}

intptr_t TCustomFarFileSystem::ProcessEvent(intptr_t Event, void * Param)
{
  ResetCachedInfo();
  return ProcessEventEx(Event, Param);
}

intptr_t TCustomFarFileSystem::SetDirectory(const wchar_t * Dir, int OpMode)
{
  ResetCachedInfo();
  InvalidateOpenPluginInfo();
  intptr_t Result = SetDirectoryEx(Dir, OpMode);
  InvalidateOpenPluginInfo();
  return Result;
}

intptr_t TCustomFarFileSystem::MakeDirectory(const wchar_t ** Name, int OpMode)
{
  ResetCachedInfo();
  FNameStr = *Name;
  intptr_t Result = 0;
  SCOPE_EXIT
  {
    if (FNameStr != *Name)
    {
      *Name = FNameStr.c_str();
    }
  };
  Result = MakeDirectoryEx(FNameStr, OpMode);
  return Result;
}

intptr_t TCustomFarFileSystem::DeleteFiles(struct PluginPanelItem * PanelItem,
    int ItemsNumber, int OpMode)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(PanelItem, ItemsNumber));
  bool Result = DeleteFilesEx(PanelItems.get(), OpMode);
  return Result;
}

intptr_t TCustomFarFileSystem::GetFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, const wchar_t ** DestPath, int OpMode)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(PanelItem, ItemsNumber));
  intptr_t Result = 0;
  FDestPathStr = *DestPath;
  {
    SCOPE_EXIT
    {
      if (FDestPathStr != *DestPath)
      {
        *DestPath = FDestPathStr.c_str();
      }
    };
    Result = GetFilesEx(PanelItems.get(), Move > 0, FDestPathStr, OpMode);
  }

  return Result;
}

intptr_t TCustomFarFileSystem::PutFiles(struct PluginPanelItem * PanelItem,
  int ItemsNumber, int Move, const wchar_t * srcPath, int OpMode)
{
  (void)srcPath;
  ResetCachedInfo();
  intptr_t Result = 0;
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(PanelItem, ItemsNumber));
  Result = PutFilesEx(PanelItems.get(), Move > 0, OpMode);
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
    bool Res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, ToInt(Info),
      !bAnother ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!Res)
    {
      DebugAssert(false);
    }
    FPanelInfo[bAnother] = new TFarPanelInfo(Info, !bAnother ? this : nullptr);
  }
  return &FPanelInfo[bAnother];
}

intptr_t TCustomFarFileSystem::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2)
{
  return FPlugin->FarControl(Command, Param1, Param2, this);
}

intptr_t TCustomFarFileSystem::FarControl(uintptr_t Command, intptr_t Param1, intptr_t Param2, HANDLE Plugin)
{
  return FPlugin->FarControl(Command, Param1, Param2, Plugin);
}

bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  uintptr_t PrevInstances = FInstances;
  InvalidateOpenPluginInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, 0, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}

void TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, ToInt(static_cast<void *>(nullptr)), Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}

void TCustomFarFileSystem::ClosePlugin()
{
  FClosed = true;
  FarControl(FCTL_CLOSEPLUGIN, 0, 0);
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

bool TCustomFarFileSystem::ProcessHostFileEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
  return false;
}

bool TCustomFarFileSystem::ProcessKeyEx(intptr_t /*Key*/, uintptr_t /*ControlState*/)
{
  return false;
}

bool TCustomFarFileSystem::ProcessEventEx(intptr_t /*Event*/, void * /*Param*/)
{
  return false;
}

bool TCustomFarFileSystem::SetDirectoryEx(UnicodeString /*Dir*/, int /*OpMode*/)
{
  return false;
}

intptr_t TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /*Name*/, int /*OpMode*/)
{
  return -1;
}

bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /*PanelItems*/, int /*OpMode*/)
{
  return false;
}

intptr_t TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
  UnicodeString & /*DestPath*/, int /*OpMode*/)
{
  return 0;
}

intptr_t TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
  bool /*Move*/, int /*OpMode*/)
{
  return 0;
}

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

TFarPanelModes::TFarPanelModes() : TObject(),
  FReferenced(false)
{
  ClearArray(FPanelModes);
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

void TFarPanelModes::SetPanelMode(size_t Mode, UnicodeString ColumnTypes,
  UnicodeString ColumnWidths, TStrings * ColumnTitles,
  bool FullScreen, bool DetailedStatus, bool AlignExtensions,
  bool CaseConversion, UnicodeString StatusColumnTypes,
  UnicodeString StatusColumnWidths)
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
  FPanelModes[Mode].FullScreen = FullScreen;
  FPanelModes[Mode].DetailedStatus = DetailedStatus;
  FPanelModes[Mode].AlignExtensions = AlignExtensions;
  FPanelModes[Mode].CaseConversion = CaseConversion;

  FPanelModes[Mode].StatusColumnTypes = TCustomFarPlugin::DuplicateStr(StatusColumnTypes);
  FPanelModes[Mode].StatusColumnWidths = TCustomFarPlugin::DuplicateStr(StatusColumnWidths);
}

void TFarPanelModes::ClearPanelMode(PanelMode & Mode)
{
  if (Mode.ColumnTypes)
  {
    intptr_t ColumnTypesCount = Mode.ColumnTypes ?
      CommaCount(UnicodeString(Mode.ColumnTypes)) + 1 : 0;

    nb_free((void *)Mode.ColumnTypes);
    nb_free((void *)Mode.ColumnWidths);
    if (Mode.ColumnTitles)
    {
      for (intptr_t Index = 0; Index < ColumnTypesCount; ++Index)
      {
        nb_free((void *)Mode.ColumnTitles[Index]);
      }
      nb_free((void *)Mode.ColumnTitles);
    }
    nb_free((void *)Mode.StatusColumnTypes);
    nb_free((void *)Mode.StatusColumnWidths);
    ClearStruct(Mode);
  }
}

void TFarPanelModes::FillOpenPluginInfo(struct OpenPluginInfo * Info)
{
  DebugAssert(Info);
  Info->PanelModesNumber = _countof(FPanelModes);
  PanelMode * PanelModesArray = nb::calloc<PanelMode*>(sizeof(PanelMode) * _countof(FPanelModes));
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}

intptr_t TFarPanelModes::CommaCount(UnicodeString ColumnTypes)
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
  intptr_t FunctionKey, UnicodeString Title)
{
  DebugAssert(FunctionKey >= 1 && FunctionKey <= static_cast<intptr_t>(_countof(FKeyBarTitles.Titles)));
  wchar_t ** Titles = nullptr;
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
    DebugAssert(false);
  }
  if (Titles)
  {
    if (Titles[FunctionKey - 1])
    {
      nb_free(Titles[FunctionKey - 1]);
    }
    Titles[FunctionKey - 1] = TCustomFarPlugin::DuplicateStr(Title, /*AllowEmpty=*/true);
  }
}

void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles & Titles)
{
  for (intptr_t Index = 0; Index < static_cast<intptr_t>(_countof(Titles.Titles)); ++Index)
  {
    nb_free(Titles.Titles[Index]);
    nb_free(Titles.CtrlTitles[Index]);
    nb_free(Titles.AltTitles[Index]);
    nb_free(Titles.ShiftTitles[Index]);
    nb_free(Titles.CtrlShiftTitles[Index]);
    nb_free(Titles.AltShiftTitles[Index]);
    nb_free(Titles.CtrlAltTitles[Index]);
  }
}

void TFarKeyBarTitles::FillOpenPluginInfo(struct OpenPluginInfo * Info)
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

  void * UserData = ToPtr(PanelItem->UserData);
  GetData(PanelItem->Flags, FileName, Size, PanelItem->FindData.dwFileAttributes,
    LastWriteTime, LastAccess, PanelItem->NumberOfLinks, Description, Owner,
    UserData, PanelItem->CustomColumnNumber);
  PanelItem->UserData = reinterpret_cast<uintptr_t>(UserData);
  FILETIME FileTime = ::DateTimeToFileTime(LastWriteTime, dstmWin);
  FILETIME FileTimeA = ::DateTimeToFileTime(LastAccess, dstmWin);
  PanelItem->FindData.ftCreationTime = FileTime;
  PanelItem->FindData.ftLastAccessTime = FileTimeA;
  PanelItem->FindData.ftLastWriteTime = FileTime;
  PanelItem->FindData.nFileSize = Size;

  PanelItem->FindData.lpwszFileName = TCustomFarPlugin::DuplicateStr(FileName);
  PanelItem->Description = TCustomFarPlugin::DuplicateStr(Description);
  PanelItem->Owner = TCustomFarPlugin::DuplicateStr(Owner);
  wchar_t ** CustomColumnData = nb::calloc<wchar_t **>(sizeof(wchar_t *) * (1 + PanelItem->CustomColumnNumber));
  for (intptr_t Index = 0; Index < PanelItem->CustomColumnNumber; ++Index)
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
  DWORD & /*Flags*/, UnicodeString & /*FileName*/, int64_t & /*Size*/,
  DWORD & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  DWORD & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  DebugAssert(false);
}

UnicodeString TFarPanelItem::GetCustomColumnData(size_t /*Column*/)
{
  DebugAssert(false);
  return L"";
}

uintptr_t TFarPanelItem::GetFlags() const
{
  return static_cast<uintptr_t>(FPanelItem->Flags);
}

UnicodeString TFarPanelItem::GetFileName() const
{
  UnicodeString Result = FPanelItem->FindData.lpwszFileName;
  return Result;
}

void * TFarPanelItem::GetUserData() const
{
  return ToPtr(FPanelItem->UserData);
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
  return static_cast<uintptr_t>(FPanelItem->FindData.dwFileAttributes);
}

bool TFarPanelItem::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}

bool TFarPanelItem::GetIsFile() const
{
  return (GetFileAttrs() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

THintPanelItem::THintPanelItem(UnicodeString AHint) :
  TCustomFarPanelItem(OBJECT_CLASS_THintPanelItem),
  FHint(AHint)
{
}

void THintPanelItem::GetData(
  DWORD & /*Flags*/, UnicodeString & AFileName, int64_t & /*Size*/,
  DWORD & /*FileAttributes*/,
  TDateTime & /*LastWriteTime*/, TDateTime & /*LastAccess*/,
  DWORD & /*NumberOfLinks*/, UnicodeString & /*Description*/,
  UnicodeString & /*Owner*/, void *& /*UserData*/, int & /*CustomColumnNumber*/)
{
  AFileName = FHint;
}

TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner) :
  TObject(),
  FPanelInfo(APanelInfo),
  FItems(nullptr),
  FOwner(AOwner)
{
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
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, ToInt(ppi));
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
    for (intptr_t Index = 0; Index < FPanelInfo->ItemsNumber; ++Index)
    {
      TODO("move to common function");
      intptr_t size = FOwner->FarControl(FCTL_GETPANELITEM, Index, 0);
      PluginPanelItem * ppi = nb::calloc<PluginPanelItem *>(size);
      FOwner->FarControl(FCTL_GETPANELITEM, Index, ToInt(ppi));
      FItems->Add(new TFarPanelItem(ppi, /*OwnsItem=*/true));
    }
  }
  return FItems;
}

TFarPanelItem * TFarPanelInfo::FindFileName(UnicodeString AFileName) const
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
  FOwner->FarControl(FCTL_SETSELECTION, 0, ToInt(FPanelInfo));
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
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, ToInt(&PanelInfo));
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
  return (FPanelInfo->Plugin != 0);
}

UnicodeString TFarPanelInfo::GetCurrDirectory() const
{
  UnicodeString Result;
  intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIR,
    0,
    0,
    FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    Result.SetLength(Size);
    FarPlugin->FarControl(FCTL_GETPANELDIR,
      Size,
      ToInt(Result.c_str()),
      FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
  }
  PackStr(Result);
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

intptr_t TFarMenuItems::Add(UnicodeString Text, bool Visible)
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
    SetObj(Index, ToObj(F));
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
  intptr_t BuffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, nullptr);
  if (BuffLen)
  {
    wchar_t * Buffer = Result.SetLength(BuffLen + 1);
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, Buffer);
  }
  PackStr(Result);
  return Result;
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
  UnicodeString Path(NB_MAX_PATH, 0);
  int Length = 0;
  if (FarPlugin)
  {
    Length = FarPlugin->GetFarStandardFunctions().GetCurrentDirectory((DWORD)Path.Length(), (wchar_t *)Path.c_str()) - 1;
  }
  else
  {
    Length = ::GetCurrentDirectory((DWORD)Path.Length(), (wchar_t *)Path.c_str());
  }
  UnicodeString Result = UnicodeString(Path.c_str(), Length);
  return Result;
}

UnicodeString TGlobalFunctions::GetStrVersionNumber() const
{
  return NETBOX_VERSION_NUMBER.c_str();
}

//bool InputBox(UnicodeString Title, UnicodeString Prompt,
//  UnicodeString & Text, DWORD Flags, UnicodeString HistoryName = UnicodeString(),
//  intptr_t MaxLen = 255, TFarInputBoxValidateEvent OnValidate = nullptr);
bool TGlobalFunctions::InputDialog(UnicodeString ACaption, UnicodeString APrompt,
                                   UnicodeString & Value, UnicodeString HelpKeyword,
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

uintptr_t TGlobalFunctions::MoreMessageDialog(UnicodeString Message, TStrings * MoreMessages, TQueryType Type, uintptr_t Answers, const TMessageParams * Params)
{
  TWinSCPPlugin * WinSCPPlugin = dyn_cast<TWinSCPPlugin>(FarPlugin);
  return WinSCPPlugin->MoreMessageDialog(Message, MoreMessages, Type, Answers, Params);
}

