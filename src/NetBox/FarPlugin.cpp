#include <vcl.h>
#pragma hdrstop

#include <plugin.hpp>
#include <Common.h>
#include <Queue.h> // TODO: move TSimpleThread to Sysutils
#include "FarPlugin.h"
#include "FarUtils.h"
#include "WinSCPPlugin.h"
#include "FarPluginStrings.h"
#include "FarDialog.h"
extern "C" {
#include <puttyexp.h>
}
#include "plugin_version.hpp"

#undef GetCurrentDirectory

TCustomFarPlugin * FarPlugin = nullptr;

constexpr const wchar_t * FAR_TITLE_SUFFIX = L" - Far";

class TPluginIdleThread : public TSimpleThread
{
  TPluginIdleThread() = delete;
public:
  explicit TPluginIdleThread(gsl::not_null<TCustomFarPlugin *> Plugin, DWORD Millisecs) noexcept :
    TSimpleThread(OBJECT_CLASS_TPluginIdleThread),
    FPlugin(Plugin),
    FMillisecs(Millisecs)
  {}

  virtual ~TPluginIdleThread() noexcept override
  {
    SAFE_CLOSE_HANDLE(FEvent);
    TPluginIdleThread::Terminate();
    WaitFor();
  }

  virtual void Execute() override
  {
    while (!IsFinished())
    {
      if (::WaitForSingleObject(FEvent, FMillisecs) != WAIT_FAILED)
      {
        if (!IsFinished() && FPlugin && FPlugin->GetPluginHandle())
          FPlugin->FarAdvControl(ACTL_SYNCHRO, 0, nullptr);
      }
    }
    if (!IsFinished())
      SAFE_CLOSE_HANDLE(FEvent);
  }

  virtual void Terminate() override
  {
    // TCompThread::Terminate();
    FFinished = true;
    if (FEvent)
      ::SetEvent(FEvent);
  }

  void InitIdleThread(const UnicodeString & Name)
  {
    TSimpleThread::InitSimpleThread(Name);
    FEvent = ::CreateEvent(nullptr, false, false, nullptr);
    Start();
  }

private:
  gsl::not_null<TCustomFarPlugin *> FPlugin;
  HANDLE FEvent{INVALID_HANDLE_VALUE};
  DWORD FMillisecs{0};
};

TCustomFarPlugin::TCustomFarPlugin(TObjectClassId Kind, HINSTANCE HInst) noexcept :
  TObject(Kind),
  FOpenedPlugins(std::make_unique<TList>()),
  FSavedTitles(std::make_unique<TStringList>())
{
  // DEBUG_PRINTF("begin");
  FFarThreadId = GetCurrentThreadId();
  FPluginHandle = HInst;
  FFarVersion = 0;
  FTerminalScreenShowing = false;

  FCurrentProgress = -1;
  FValidFarSystemSettings = false;
  FFarSystemSettings = 0;

  nb::ClearStruct(FPluginInfo);
  FPluginInfo.StructSize = sizeof(FPluginInfo);
  ClearPluginInfo(FPluginInfo);
  nb::ClearStruct(FStartupInfo);
  FStartupInfo.StructSize = sizeof(FStartupInfo);
  nb::ClearStruct(FFarStandardFunctions);
  FFarStandardFunctions.StructSize = sizeof(FFarStandardFunctions);

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
  // DEBUG_PRINTF("begin");
}

TCustomFarPlugin::~TCustomFarPlugin() noexcept
{
  // DEBUG_PRINTF("end");
  DebugAssert(FTopDialog == nullptr);

  ResetCachedInfo();
  SAFE_CLOSE_HANDLE(FConsoleInput);
  FConsoleInput = INVALID_HANDLE_VALUE;
  SAFE_CLOSE_HANDLE(FConsoleOutput);
  FConsoleOutput = INVALID_HANDLE_VALUE;

  ClearPluginInfo(FPluginInfo);
  DebugAssert(FOpenedPlugins->GetCount() == 0);
  // SAFE_DESTROY(FOpenedPlugins);
  for (int32_t Index = 0; Index < FSavedTitles->GetCount(); ++Index)
  {
    TObject * Object = FSavedTitles->Get(Index);
    SAFE_DESTROY(Object);
  }
  // SAFE_DESTROY(FSavedTitles);
  // TODO: CloseFileSystem(FarFileSystem);
  // DEBUG_PRINTF("end");
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
    nb::ClearStruct(FStartupInfo);
    memmove(&FStartupInfo, Info,
      Info->StructSize >= nb::ToSizeT(sizeof(FStartupInfo)) ?
      sizeof(FStartupInfo) : nb::ToSizeT(Info->StructSize));
    // the minimum we really need
    DebugAssert(FStartupInfo.GetMsg != nullptr);
    DebugAssert(FStartupInfo.Message != nullptr);

    nb::ClearStruct(FFarStandardFunctions);
    const size_t FSFOffset = (static_cast<const char *>(nb::ToPtr(&Info->FSF)) -
        static_cast<const char *>(nb::ToPtr(Info)));
    if (nb::ToSizeT(Info->StructSize) > FSFOffset)
    {
      memmove(&FFarStandardFunctions, Info->FSF,
        nb::ToSizeT(Info->FSF->StructSize) >= sizeof(FFarStandardFunctions) ?
        sizeof(FFarStandardFunctions) : Info->FSF->StructSize);
    }
  }
  catch(Exception & E)
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
    do { if (NAME.GetCount()) \
    { \
      wchar_t ** StringArray = nb::calloc<wchar_t **>(1 + NAME.GetCount(), sizeof(wchar_t *)); \
      GUID * Guids = static_cast<GUID *>(nb_malloc(sizeof(GUID) * NAME.GetCount())); \
      FPluginInfo.NAME.Guids = Guids; \
      FPluginInfo.NAME.Strings = StringArray; \
      FPluginInfo.NAME.Count = NAME.GetCount(); \
      for (int32_t Index = 0; Index < NAME.GetCount(); Index++) \
      { \
        StringArray[Index] = DuplicateStr(NAME.GetString(Index)); \
        Guids[Index] = *reinterpret_cast<const GUID *>(NAME.Objects[Index]); \
      } \
    } } while(0)

    COMPOSESTRINGARRAY(DiskMenu);
    COMPOSESTRINGARRAY(PluginMenu);
    COMPOSESTRINGARRAY(PluginConfig);

#undef COMPOSESTRINGARRAY
    UnicodeString CommandPrefix;
    for (int32_t Index = 0; Index < CommandPrefixes.GetCount(); ++Index)
    {
      CommandPrefix = CommandPrefix + (CommandPrefix.IsEmpty() ? L"" : L":") +
        CommandPrefixes.GetString(Index);
    }
    FPluginInfo.CommandPrefix = DuplicateStr(CommandPrefix);

    memmove(Info, &FPluginInfo, sizeof(FPluginInfo));
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
}

intptr_t TCustomFarPlugin::ProcessSynchroEvent(const ProcessSynchroEventInfo * Info)
{
  Expects(Info);
  try
  {
    const TSynchroParams * SynchroParams = static_cast<TSynchroParams *>(Info->Param);
    if (SynchroParams)
    {
      if (SynchroParams->Sender && SynchroParams->SynchroEvent)
      {
        SynchroParams->SynchroEvent(this, nullptr);
      }
    }
    else
    {
      TCustomFarFileSystem * FarFileSystem1 = GetPanelFileSystem(false);
      if (FarFileSystem1)
        FarFileSystem1->ProcessPanelEventEx(FE_IDLE, nullptr);
      TCustomFarFileSystem * FarFileSystem2 = GetPanelFileSystem(true);
      if (FarFileSystem2)
        FarFileSystem2->ProcessPanelEventEx(FE_IDLE, nullptr);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
  return 0;
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
    do { for (size_t Index = 0; Index < Info.NAME.Count; ++Index) \
    { \
      nb_free(Info.NAME.Strings[Index]); \
    } \
    nb_free(Info.NAME.Strings); \
    nb_free(Info.NAME.Guids); \
    Info.NAME.Strings = nullptr; } while(0)

    FREESTRINGARRAY(DiskMenu);
    FREESTRINGARRAY(PluginMenu);
    FREESTRINGARRAY(PluginConfig);

#undef FREESTRINGARRAY

    nb_free(Info.CommandPrefix);
  }
  nb::ClearStruct(Info);
  Info.StructSize = sizeof(Info);
}

wchar_t * TCustomFarPlugin::DuplicateStr(const UnicodeString & Str, bool AllowEmpty)
{
  if (Str.IsEmpty() && !AllowEmpty)
  {
    return nullptr;
  }
  const size_t sz = Str.Length() + 1;
  wchar_t * Result = nb::wchcalloc(1 + sz);
  wcscpy_s(Result, sz, Str.c_str());
  return Result;
}

RECT TCustomFarPlugin::GetPanelBounds(HANDLE PanelHandle)
{
  PanelInfo Info{};
  nb::ClearStruct(Info);
  Info.StructSize = sizeof(PanelInfo);
  FarControl(FCTL_GETPANELINFO, 0, nb::ToPtr(&Info), PanelHandle);

  RECT Bounds;
  nb::ClearStruct(Bounds);
  if (Info.PluginHandle)
  {
    Bounds = Info.PanelRect;
  }
  return Bounds;
}

TCustomFarFileSystem * TCustomFarPlugin::GetPanelFileSystem(bool Another,
  HANDLE /*Plugin*/)
{
  TCustomFarFileSystem * Result{nullptr};
  const RECT ActivePanelBounds = GetPanelBounds(PANEL_ACTIVE);
  const RECT PassivePanelBounds = GetPanelBounds(PANEL_PASSIVE);

  int32_t Index{0};
  while (!Result && (Index < FOpenedPlugins->GetCount()))
  {
    TCustomFarFileSystem * FarFileSystem = FOpenedPlugins->GetAs<TCustomFarFileSystem>(Index);
    DebugAssert(FarFileSystem);
    const RECT Bounds = GetPanelBounds(FarFileSystem);
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
  for (int32_t Index = 0; Index < FOpenedPlugins->GetCount(); ++Index)
  {
    TCustomFarFileSystem * FarFileSystem = FOpenedPlugins->GetAs<TCustomFarFileSystem>(Index);
    DebugAssert(FarFileSystem);
    FarFileSystem->InvalidateOpenPanelInfo();
  }
}

int32_t TCustomFarPlugin::Configure(const struct ConfigureInfo * Info)
{
  try
  {
    ResetCachedInfo();
    const int32_t Result = ConfigureEx(Info->Guid);
    InvalidateOpenPanelInfo();

    return Result;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
  }
  return 0;
}

void * TCustomFarPlugin::OpenPlugin(const struct OpenInfo * Info)
{
#ifdef USE_DLMALLOC
  // dlmallopt(M_GRANULARITY, 128 * 1024);
#endif

  try
  {
    if (Info->OpenFrom == OPEN_FROMMACRO)
      return nullptr;
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
      if (Info->OpenFrom == OPEN_ANALYSE)
      {
        Result->SetOwnerFileSystem(GetPanelFileSystem());
      }
      FOpenedPlugins->Add(Result);
    }
    else
    {
      Result = nullptr;
    }

    return Result;
  }
  catch(Exception & E)
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
    TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Plugin);
    if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
      return;
    CloseFileSystem(FarFileSystem);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(& E);
  }
}

void TCustomFarPlugin::CloseFileSystem(TCustomFarFileSystem * FileSystem)
{
  if (!FileSystem)
    return;
  DebugAssert(FOpenedPlugins->IndexOf(FileSystem) != nb::NPOS);
  try__finally
  {
    const TGuard Guard(FileSystem->GetCriticalSection());
    FileSystem->Close();
  }
  __finally
  {
    FOpenedPlugins->Remove(FileSystem);
    CloseFileSystem(FileSystem->GetOwnerFileSystem());
    SAFE_DESTROY(FileSystem);
  } end_try__finally
#ifdef USE_DLMALLOC
  // dlmalloc_trim(0); // 64 * 1024);
#endif
}

void TCustomFarPlugin::HandleFileSystemException(
  TCustomFarFileSystem * FarFileSystem, Exception * E, OPERATION_MODES OpMode)
{
  // This method is called as last-resort exception handler before
  // leaving plugin API. Especially for API functions that must update
  // panel contents on themselves (like ProcessPanelInput), the instance of filesystem
  // may not exist anymore.
  // Check against object pointer is stupid, but no other idea so far.
  if (FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS)
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
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);
    const TGuard Guard(FarFileSystem->GetCriticalSection());
    FarFileSystem->GetOpenPanelInfo(Info);
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

int32_t TCustomFarPlugin::GetFindData(struct GetFindDataInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFindData(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

void TCustomFarPlugin::FreeFindData(const struct FreeFindDataInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);
    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      FarFileSystem->FreeFindData(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
  }
}

intptr_t TCustomFarPlugin::ProcessHostFile(const struct ProcessHostFileInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessHostFile))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);
      {
        const TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessHostFile(Info);
      }
    }
    return 0;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::ProcessPanelInput(const struct ProcessPanelInputInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessKey))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);
      {
        const TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->ProcessPanelInput(Info);
      }
    }
    return 0;
  }
  catch(Exception & E)
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
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    if (HandlesFunction(hfProcessPanelEvent))
    {
      DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

      UnicodeString Buf;
      void * Param = Info->Param;
      if ((Info->Event == FE_CHANGEVIEWMODE) || (Info->Event == FE_COMMAND))
      {
        Buf = static_cast<wchar_t *>(Param);
        Param = nb::ToPtr(Buf.c_str());
      }

      bool Result{false};
      { 
        const TGuard Guard(FarFileSystem->GetCriticalSection());
        Result = FarFileSystem->ProcessPanelEvent(Info->Event, Param);
      }
      return Result;
    }
    return 0;
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E);
    return Info->Event == FE_COMMAND ? true : false;
  }
}

intptr_t TCustomFarPlugin::SetDirectory(const struct SetDirectoryInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  DebugAssert(FarFileSystem);
  const UnicodeString PrevCurrentDirectory = FarFileSystem->GetCurrentDirectory();
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);
    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->SetDirectory(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    if (FarFileSystem->GetOpenPanelInfoValid() && !PrevCurrentDirectory.IsEmpty())
    {
      SetDirectoryInfo Info2{};
      Info2.StructSize = sizeof(Info2);
      Info2.hPanel = Info->hPanel;
      Info2.Dir = PrevCurrentDirectory.c_str();
      Info2.UserData = Info->UserData;
      Info2.OpMode = Info->OpMode;
      try
      {
        const TGuard Guard(FarFileSystem->GetCriticalSection());
        return FarFileSystem->SetDirectory(&Info2);
      }
      catch(Exception &)
      {
        return 0;
      }
    }
    return 0;
  }
}

intptr_t TCustomFarPlugin::MakeDirectory(struct MakeDirectoryInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->MakeDirectory(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::DeleteFiles(const struct DeleteFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->DeleteFiles(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode);
    return 0;
  }
}

intptr_t TCustomFarPlugin::GetFiles(struct GetFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->GetFiles(Info);
    }
  }
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleFileSystemException");
    // display error even for OPM_FIND
    HandleFileSystemException(FarFileSystem, &E, Info->OpMode & ~OPM_FIND);
  }
  return 0;
}

intptr_t TCustomFarPlugin::PutFiles(const struct PutFilesInfo * Info)
{
  TCustomFarFileSystem * FarFileSystem = static_cast<TCustomFarFileSystem *>(Info->hPanel);
  if (!FarFileSystem || !FOpenedPlugins || (FOpenedPlugins->IndexOf(FarFileSystem) == nb::NPOS))
    return 0;
  try
  {
    ResetCachedInfo();
    DebugAssert(FOpenedPlugins->IndexOf(FarFileSystem) != nb::NPOS);

    {
      const TGuard Guard(FarFileSystem->GetCriticalSection());
      return FarFileSystem->PutFiles(Info);
    }
  }
  catch(Exception & E)
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
  catch(Exception & E)
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
  catch(Exception & E)
  {
    DEBUG_PRINTF("before HandleException");
    HandleException(&E);
    // when error occurs, assume that input event can be handled by plugin and
    // should not be processed by FAR
    return 1;
  }
}

int32_t TCustomFarPlugin::MaxMessageLines() const
{
  return std::min<int32_t>(1, TerminalInfo().y - 5);
}

int32_t TCustomFarPlugin::MaxMenuItemLength() const
{
  // got from maximal length of path in FAR's folders history
  return std::min<int32_t>(10, TerminalInfo().x - 13);
}

int32_t TCustomFarPlugin::MaxLength(TStrings * Strings) const
{
  int32_t Result = 0;
  for (int32_t Index = 0; Index < Strings->GetCount(); ++Index)
  {
    if (Result < Strings->GetString(Index).Length())
    {
      Result = Strings->GetString(Index).Length();
    }
  }
  return Result;
}

class TFarMessageDialog final : public TFarDialog
{
  TFarMessageDialog() = delete;
public:
  explicit TFarMessageDialog(gsl::not_null<TCustomFarPlugin *> Plugin,
    gsl::not_null<TFarMessageParams *> Params);
  void Init(uint32_t AFlags, const UnicodeString & Title, const UnicodeString & Message,
    TStrings * Buttons);
  virtual ~TFarMessageDialog() override;

  int32_t Execute(bool & ACheckBox);

protected:
  virtual const UUID * GetDialogGuid() const override { return &FarMessageDialogGuid; }
  virtual void Change() override;
  virtual void Idle() override;

private:
  void ButtonClick(TFarButton * Sender, bool & Close);
  void OnUpdateTimeoutButton(TObject * Sender, void * Data);

private:
  bool FCheckBoxChecked{false};
  gsl::not_null<TFarMessageParams *> FParams;
  TDateTime FStartTime;
  TDateTime FLastTimerTime;
  TFarButton * FTimeoutButton{nullptr};
  UnicodeString FTimeoutButtonCaption;
  TFarCheckBox * FCheckBox{nullptr};
};

TFarMessageDialog::TFarMessageDialog(gsl::not_null<TCustomFarPlugin *> Plugin,
  gsl::not_null<TFarMessageParams *> Params) :
  TFarDialog(Plugin),
  FParams(Params)
{
  DebugAssert(FParams != nullptr);
}

void TFarMessageDialog::Init(uint32_t AFlags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons)
{
  TFarDialog::InitDialog();
  DebugAssert(FLAGCLEAR(AFlags, FMSG_ERRORTYPE));
  DebugAssert(FLAGCLEAR(AFlags, FMSG_KEEPBACKGROUND));
  // FIXME DebugAssert(FLAGCLEAR(AFlags, FMSG_DOWN));
  DebugAssert(FLAGCLEAR(AFlags, FMSG_ALLINONE));
  std::unique_ptr<TStrings> MessageLines(std::make_unique<TStringList>());
  FarWrapText(Message, MessageLines.get(), MaxMessageWidth);
  int32_t MaxLen = GetFarPlugin()->MaxLength(MessageLines.get());
  std::unique_ptr<TStrings> MoreMessageLines;
  if (FParams->MoreMessages != nullptr)
  {
    MoreMessageLines = std::make_unique<TStringList>();
    UnicodeString MoreMessages = FParams->MoreMessages->GetText();
    while ((MoreMessages.Length() > 0) && (MoreMessages[MoreMessages.Length()] == L'\n' ||
        MoreMessages[MoreMessages.Length()] == L'\r'))
    {
      MoreMessages.SetLength(MoreMessages.Length() - 1);
    }
    FarWrapText(MoreMessages, MoreMessageLines.get(), MaxMessageWidth);
    const int32_t MoreMaxLen = GetFarPlugin()->MaxLength(MoreMessageLines.get());
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

  for (int32_t Index = 0; Index < MessageLines->GetCount(); ++Index)
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
    MoreMessagesLister->GetItems()->Assign(MoreMessageLines.get());
    MoreMessagesLister->SetLeft(GetBorderBox()->GetLeft() + 1);

    MoreMessagesSeparator = new TFarSeparator(this);
  }

  const int32_t ButtonOffset = FParams->CheckBoxLabel.IsEmpty() ? -1 : -2;
  int32_t ButtonLines = 1;
  TFarButton * Button = nullptr;
  FTimeoutButton = nullptr;
  for (int32_t Index = 0; Index < Buttons->GetCount(); ++Index)
  {
    const TFarButton * PrevButton = Button;
    Button = new TFarButton(this);
    Button->SetDefault(FParams->DefaultButton == Index);
    Button->SetBrackets(brNone);
    Button->SetOnClick(nb::bind(&TFarMessageDialog::ButtonClick, this));
    UnicodeString Caption = Buttons->GetString(Index);
    if ((FParams->Timeout > 0) &&
      (FParams->TimeoutButton == nb::ToUIntPtr(Index)))
    {
      FTimeoutButtonCaption = Caption;
      Caption = FORMAT(FParams->TimeoutStr, Caption, nb::ToInt32(FParams->Timeout / 1000));
      FTimeoutButton = Button;
    }
    Button->SetCaption(FORMAT(" %s ", Caption));
    Button->SetTop(GetBorderBox()->GetBottom() + ButtonOffset);
    Button->SetBottom(Button->GetTop());
    Button->SetResult(Index + 1);
    Button->SetCenterGroup(true);
    Button->SetTag(nb::ToInt32(nb::ToIntPtr(Buttons->Objects[Index])));
    if (PrevButton != nullptr)
    {
      Button->Move(PrevButton->GetRight() - Button->GetLeft() + 1, 0);
    }

    if (MaxMessageWidth < Button->GetRight() - GetBorderBox()->GetLeft())
    {
      for (int32_t PIndex = 0; PIndex < GetItemCount(); ++PIndex)
      {
        TFarButton * PrevButton2 = rtti::dyn_cast_or_null<TFarButton>(GetItem(PIndex));
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
      MaxLen = Button->GetRight() - GetBorderBox()->GetLeft() + 2;
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
      MaxLen = FCheckBox->GetRight() - GetBorderBox()->GetLeft();
    }
  }
  else
  {
    FCheckBox = nullptr;
  }

  const TRect rect = GetClientRect();
  TPoint S(
    nb::ToInt32(rect.Left + MaxLen - rect.Right),
    nb::ToInt32(rect.Top + MessageLines->GetCount() +
      (FParams->MoreMessages != nullptr ? 1 : 0) + ButtonLines +
      (!FParams->CheckBoxLabel.IsEmpty() ? 1 : 0) +
      (-(rect.Bottom + 1))));

  if (FParams->MoreMessages != nullptr)
  {
    int32_t MoreMessageHeight = nb::ToInt32(GetFarPlugin()->TerminalInfo().y - S.y - 1);
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
    S.y += nb::ToInt32(MoreMessagesLister->GetHeight()) + 1;
  }
  SetSize(S);
}

TFarMessageDialog::~TFarMessageDialog()
{
  if (GetFarPlugin())
  {
    TSynchroParams & SynchroParams = GetFarPlugin()->FSynchroParams;
    SynchroParams.Sender = nullptr;
  }
}

void TFarMessageDialog::Idle()
{
  TFarDialog::Idle();
  if (GetFarPlugin())
  {
    TSynchroParams & SynchroParams = GetFarPlugin()->FSynchroParams;
    SynchroParams.SynchroEvent = nb::bind(&TFarMessageDialog::OnUpdateTimeoutButton, this);
    SynchroParams.Sender = this;
    GetFarPlugin()->FarAdvControl(ACTL_SYNCHRO, 0, &SynchroParams);
  }
}

void TFarMessageDialog::Change()
{
  TFarDialog::Change();

  if (GetHandle() != nullptr)
  {
    if ((FCheckBox != nullptr) && (FCheckBoxChecked != FCheckBox->GetChecked()))
    {
      for (int32_t Index = 0; Index < GetItemCount(); ++Index)
      {
        TFarButton * Button = rtti::dyn_cast_or_null<TFarButton>(GetItem(Index));
        if ((Button != nullptr) && (Button->GetTag() == 0))
        {
          Button->SetEnabled(!FCheckBox->GetChecked());
        }
      }
      FCheckBoxChecked = FCheckBox->GetChecked();
    }
  }
}

int32_t TFarMessageDialog::Execute(bool & ACheckBox)
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

  int32_t Result = ShowModal();
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

void TFarMessageDialog::OnUpdateTimeoutButton(TObject * /*Sender*/, void * /*Data*/)
{
  // DEBUG_PRINTF("Sender: %p, Data: %p", (void *)Sender, (void *)Data);
  if (FParams && (FParams->Timer > 0))
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

  if (FParams && (FParams->Timeout > 0))
  {
    const uint32_t Running = nb::ToUInt32((Now() - FStartTime).GetValue() * MSecsPerDay);
    if (Running >= FParams->Timeout)
    {
      DebugAssert(FTimeoutButton != nullptr);
      Close(FTimeoutButton);
    }
    else
    {
      UnicodeString Caption =
        FORMAT(" %s ", FORMAT(FParams->TimeoutStr,
            FTimeoutButtonCaption, nb::ToInt32((FParams->Timeout - Running) / 1000)));
      const int32_t Sz = FTimeoutButton->GetCaption().Length() > Caption.Length() ? FTimeoutButton->GetCaption().Length() - Caption.Length() : 0;
      Caption += ::StringOfChar(L' ', Sz);
      FTimeoutButton->SetCaption(Caption);
    }
  }
}

int32_t TCustomFarPlugin::DialogMessage(uint32_t Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  gsl::not_null<TFarMessageParams *> Params)
{
  std::unique_ptr<TFarMessageDialog> Dialog(std::make_unique<TFarMessageDialog>(this, Params));
  Dialog->Init(Flags, Title, Message, Buttons);
  const int32_t Result = Dialog->Execute(Params->CheckBox);
  return Result;
}

int32_t TCustomFarPlugin::FarMessage(uint32_t Flags,
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
  if (Params && (Params->MoreMessages != nullptr))
  {
    FullMessage += UnicodeString(L"\n\x01\n") + Params->MoreMessages->GetText();
    while (FullMessage[FullMessage.Length()] == L'\n' ||
      FullMessage[FullMessage.Length()] == L'\r')
    {
      FullMessage.SetLength(FullMessage.Length() - 1);
    }
    FullMessage += L"\n\x01\n";
  }

  std::unique_ptr<TStringList> MessageLines(std::make_unique<TStringList>());
  MessageLines->Add(Title);
  FarWrapText(FullMessage, MessageLines.get(), MaxMessageWidth);

  // FAR WORKAROUND
  // When there is too many lines to fit on screen, far uses not-shown
  // lines as button captions instead of real captions at the end of the list
  const int32_t MaxLines = MaxMessageLines();
  while (MessageLines->GetCount() > MaxLines)
  {
    MessageLines->Delete(MessageLines->GetCount() - 1);
  }

  for (int32_t Index = 0; Index < Buttons->GetCount(); ++Index)
  {
    MessageLines->Add(Buttons->GetString(Index));
  }

  Items = nb::calloc<wchar_t **>(1 + MessageLines->GetCount(), sizeof(wchar_t *));
  for (int32_t Index = 0; Index < MessageLines->GetCount(); ++Index)
  {
    const UnicodeString S = MessageLines->GetString(Index);
    MessageLines->SetString(Index, UnicodeString(S));
    Items[Index] = ToWCharPtr(MessageLines->GetStringRef(Index));
  }

  const TFarEnvGuard Guard;
  const int32_t Result = nb::ToInt32(FStartupInfo.Message(
    &NetBoxPluginGuid, &MessageGuid,
    Flags | FMSG_LEFTALIGN, nullptr, Items, nb::ToInt32(MessageLines->GetCount()),
    nb::ToInt32(Buttons->GetCount())));

  return Result;
}

int32_t TCustomFarPlugin::Message(uint32_t Flags,
  const UnicodeString & Title, const UnicodeString & Message, TStrings * Buttons,
  TFarMessageParams * Params)
{
  // when message is shown while some "custom" output is on screen,
  // make the output actually background of FAR screen
  if (FTerminalScreenShowing)
  {
    FarControl(FCTL_SETUSERSCREEN, 0, nullptr);
  }

  int32_t Result;
  if (Buttons != nullptr)
  {
    TFarMessageParams DefaultParams;
    TFarMessageParams * AParams = (Params == nullptr ? &DefaultParams : Params);
    Result = DialogMessage(Flags, Title, Message, Buttons, AParams);
  }
  else
  {
    DebugAssert(Params == nullptr);
    const UnicodeString Items = Title + L"\n" + Message;
    const TFarEnvGuard Guard;
    Result = nb::ToInt32(FStartupInfo.Message(
      &NetBoxPluginGuid, &MessageGuid,
      Flags | FMSG_ALLINONE | FMSG_LEFTALIGN,
      nullptr,
      static_cast<const wchar_t * const *>(static_cast<const void *>(Items.c_str())), 0, 0));
  }
  return Result;
}

intptr_t TCustomFarPlugin::Menu(FARMENUFLAGS Flags, const UnicodeString & Title,
  const UnicodeString & Bottom, const FarMenuItem * Items, size_t Count,
  const FarKey * BreakKeys, intptr_t & BreakCode)
{
  DebugAssert(Items);

  const TFarEnvGuard Guard;
  return FStartupInfo.Menu(&NetBoxPluginGuid, &MenuGuid,
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
  const UnicodeString & Bottom, TStrings * Items,
  const FarKey * BreakKeys, intptr_t & BreakCode)
{
  DebugAssert(Items && Items->GetCount());
  intptr_t Result{0};
  FarMenuItem * MenuItems = nb::calloc<FarMenuItem *>(1 + Items->GetCount(), sizeof(FarMenuItem));
  SCOPE_EXIT
  {
    nb_free(MenuItems);
  };
  int32_t Selected = nb::NPOS;
  int32_t Count = 0;
  for (int32_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    const uint32_t Flags2 = nb::ToUInt32(nb::ToUIntPtr(Items->Objects[Index]));
    if (FLAGCLEAR(Flags2, MIF_HIDDEN))
    {
      nb::ClearStruct(MenuItems[Count]);
      MenuItems[Count].Flags = nb::ToDWord(Flags2);
      if (MenuItems[Count].Flags & MIF_SELECTED)
      {
        DebugAssert(Selected == nb::NPOS);
        Selected = Index;
      }
      MenuItems[Count].Text = Items->GetStringRef(Index).c_str();
      MenuItems[Count].UserData = Index;
      Count++;
    }
  }

  const intptr_t ResultItem = Menu(Flags, Title, Bottom,
    MenuItems, Count, BreakKeys, BreakCode);

  if (ResultItem >= 0)
  {
    Result = MenuItems[ResultItem].UserData;
    if (Selected >= 0)
    {
      Items->SetObject(Selected, ToObj(nb::ToIntPtr(Items->Objects[Selected]) & ~MIF_SELECTED));
    }
    Items->SetObject(nb::ToInt32(Result), ToObj(nb::ToIntPtr(Items->Objects[nb::ToInt32(Result)]) | MIF_SELECTED));
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
  const UnicodeString & HistoryName, int32_t MaxLen, TFarInputBoxValidateEvent && OnValidate)
{
  bool Repeat;
  intptr_t Result;
  do
  {
    UnicodeString DestText;
    DestText.SetLength(MaxLen + 1);
    HANDLE ScreenHandle = nullptr;
    SaveScreen(ScreenHandle);
    {
      const TFarEnvGuard Guard;
      Result = FStartupInfo.InputBox(
          &NetBoxPluginGuid,
          &InputBoxGuid,
          Title.c_str(),
          Prompt.c_str(),
          HistoryName.c_str(),
          Text.c_str(),
          ToWCharPtr(DestText),
          nb::ToInt32(MaxLen),
          nullptr,
          FIB_ENABLEEMPTY | FIB_BUTTONS | Flags);
    }
    RestoreScreen(ScreenHandle);
    Repeat = false;
    if (Result)
    {
      Text = DestText.c_str();
      if (!OnValidate.empty())
      {
        try
        {
          OnValidate(Text);
        }
        catch(Exception & E)
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

void TCustomFarPlugin::Text(int32_t X, int32_t Y, int32_t Color, const UnicodeString & Str)
{
  const TFarEnvGuard Guard;
  FarColor color = {};
  color.Flags = FCF_FG_4BIT | FCF_BG_4BIT;
  color.ForegroundColor = Color; // LIGHTGRAY;
  color.BackgroundColor = 0;
  FStartupInfo.Text(X, Y, &color, Str.c_str());
}

void TCustomFarPlugin::FlushText()
{
  const TFarEnvGuard Guard;
  FStartupInfo.Text(0, 0, nullptr, nullptr);
}

void TCustomFarPlugin::FarWriteConsole(const UnicodeString & Str)
{
  unsigned long Written;
  ::WriteConsole(FConsoleOutput, Str.c_str(), nb::ToDWord(Str.Length()), &Written, nullptr);
}

void TCustomFarPlugin::FarCopyToClipboard(const UnicodeString & Str)
{
  const TFarEnvGuard Guard;
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
  nb::ClearStruct(BufferInfo);
  ::GetConsoleScreenBufferInfo(FConsoleOutput, &BufferInfo);
  if (FarPlugin)
    FarAdvControl(ACTL_GETFARRECT, 0, &BufferInfo.srWindow);

  const TPoint Result(
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
  const HWND Result = ::FindWindow(nullptr, Title);
  return Result;
}

uint32_t TCustomFarPlugin::ConsoleWindowState() const
{
  uint32_t Result = SW_SHOWNORMAL;
  const HWND Window = GetConsoleWindow();
  if (Window != nullptr)
  {
    WINDOWPLACEMENT WindowPlacement;
    nb::ClearStruct(WindowPlacement);
    WindowPlacement.length = sizeof(WindowPlacement);
    ::Win32Check(::GetWindowPlacement(Window, &WindowPlacement) > 0);
    Result = WindowPlacement.showCmd;
  }
  return Result;
}

void TCustomFarPlugin::ToggleVideoMode()
{
  const HWND Window = GetConsoleWindow();
  if (Window != nullptr)
  {
    if (ConsoleWindowState() == SW_SHOWMAXIMIZED)
    {
      if (FNormalConsoleSize.x >= 0)
      {
        const COORD Size = {static_cast<short>(FNormalConsoleSize.x), static_cast<short>(FNormalConsoleSize.y)};

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
      const COORD Size = ::GetLargestConsoleWindowSize(FConsoleOutput);
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

void TCustomFarPlugin::ScrollTerminalScreen(int32_t Rows)
{
  const TPoint Size = TerminalInfo();

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

void TCustomFarPlugin::ShowTerminalScreen(const UnicodeString & Command)
{
  DebugAssert(!FTerminalScreenShowing);
  TPoint Size, Cursor;
  TerminalInfo(&Size, &Cursor);

  if (Size.y >= 2)
  {
    // clean menu keybar area before command output
    int32_t Y = Size.y - 2;
    // if any panel is visible -- clear all screen (don't scroll panel)
    {
      PanelInfo Info{};
      nb::ClearStruct(Info);
      Info.StructSize = sizeof(Info);
      FarControl(FCTL_GETPANELINFO, 0, &Info, PANEL_ACTIVE);
      if(Info.Flags & PFLAGS_VISIBLE)
        goto clearall;
      nb::ClearStruct(Info);
      Info.StructSize = sizeof(Info);
      FarControl(FCTL_GETPANELINFO, 0, &Info, PANEL_PASSIVE);
      if (Info.Flags&PFLAGS_VISIBLE)
      {
clearall:
        Y = 0;
      }
    }
    UnicodeString Blank = ::StringOfChar(L' ', Size.x);
    do
    {
      Text(0, Y, 7 /*LIGHTGRAY*/, Blank);
    } while (++Y < Size.y);
    if(Command.Length() && Size.x > 2)
    {
      Blank = Command;
      Blank.Insert(0, L"$ ", 2);
      if(Blank.Length() > Size.x) Blank.SetLength(Size.x);
      if(Cursor.y == Y-1) --Cursor.y; // !'Show key bar'
      Text(0, Cursor.y, 7 /*LIGHTGRAY*/, Blank);
      ++Cursor.y;
    }
  }
  FlushText();

  COORD Coord{};
  Coord.X = 0;
  Coord.Y = static_cast<SHORT>(Cursor.y);
  ::SetConsoleCursorPosition(FConsoleOutput, Coord);
  FTerminalScreenShowing = true;
}

void TCustomFarPlugin::SaveTerminalScreen()
{
  FTerminalScreenShowing = false;
  FarControl(FCTL_SETUSERSCREEN, 0, nullptr);
}

const TObjectClassId OBJECT_CLASS_TConsoleTitleParam = static_cast<TObjectClassId>(nb::counter_id());
class TConsoleTitleParam : public TObject
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TConsoleTitleParam); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TConsoleTitleParam) || TObject::is(Kind); }
public:
  explicit TConsoleTitleParam() : TObject(OBJECT_CLASS_TConsoleTitleParam) {}

  int16_t Progress{0};
  uint16_t Own{0};
};

void TCustomFarPlugin::ShowConsoleTitle(const UnicodeString & Title)
{
  UnicodeString SaveTitle(1024, 0);
  ::GetConsoleTitle(const_cast<wchar_t *>(SaveTitle.c_str()), SaveTitle.GetLength());
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
    const UnicodeString Title = FSavedTitles->GetString(FSavedTitles->GetCount() - 1);
    TObject * Object = FSavedTitles->Get(FSavedTitles->GetCount() - 1);
    const TConsoleTitleParam * Param = rtti::dyn_cast_or_null<TConsoleTitleParam>(Object);
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
      TObject * Obj = FSavedTitles->Get(FSavedTitles->GetCount() - 1);
      SAFE_DESTROY(Obj);
    }
    FSavedTitles->Delete(FSavedTitles->GetCount() - 1);
  }
}

void TCustomFarPlugin::UpdateConsoleTitle(const UnicodeString & Title)
{
  FCurrentTitle = Title;
  UpdateCurrentConsoleTitle();
}

void TCustomFarPlugin::UpdateConsoleTitleProgress(int16_t Progress)
{
  FCurrentProgress = Progress;
  UpdateCurrentConsoleTitle();
}

UnicodeString TCustomFarPlugin::FormatConsoleTitle() const
{
  UnicodeString Title;
  if (FCurrentProgress >= 0)
  {
    Title = FORMAT("{%d%%} %s", FCurrentProgress, FCurrentTitle);
  }
  else
  {
    Title = FCurrentTitle;
  }
  Title += FAR_TITLE_SUFFIX;
  return Title;
}

void TCustomFarPlugin::UpdateProgress(int32_t State, int32_t Progress) const
{
  FarAdvControl(ACTL_SETPROGRESSSTATE, State, nullptr);
  if (State == TBPS_NORMAL)
  {
    ProgressValue PV{};
    PV.StructSize = sizeof(ProgressValue);
    PV.Completed = (Progress < 0) ? 0 : (Progress > 100) ? 100 : Progress;
    PV.Total = 100;
    FarAdvControl(ACTL_SETPROGRESSVALUE, 0, &PV);
  }
}

void TCustomFarPlugin::UpdateCurrentConsoleTitle()
{
  const UnicodeString Title = FormatConsoleTitle();
  ::SetConsoleTitle(Title.c_str());
  const int32_t Progress = FCurrentProgress != -1 ? FCurrentProgress : 0;
  UpdateProgress(Progress != 0 ? TBPS_NORMAL : TBPS_NOPROGRESS, Progress);
}

void TCustomFarPlugin::SaveScreen(HANDLE & Screen)
{
  DebugAssert(!Screen);
  const TFarEnvGuard Guard;
  Screen = static_cast<HANDLE>(FStartupInfo.SaveScreen(0, 0, -1, -1));
  DebugAssert(Screen);
}

void TCustomFarPlugin::RestoreScreen(HANDLE & Screen)
{
  DebugAssert(Screen);
  const TFarEnvGuard Guard;
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
  const TFarEnvGuard Guard;
  UnicodeString Result;
  if (FStartupInfo.GetMsg)
  try
  {
    Result = FStartupInfo.GetMsg(&NetBoxPluginGuid, MsgId);
  }
  catch(...)
  {
    // TODO: report error
  }

  return Result;
}

bool TCustomFarPlugin::CheckForEsc() const
{
  static uint32_t LastTicks;
  const uint32_t Ticks = ::GetTickCount();
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
  const TFarEnvGuard Guard;
  const intptr_t Result = FStartupInfo.Viewer(
      AFileName.c_str(),
      Title.c_str(), 0, 0, -1, -1, Flags,
      CP_DEFAULT);
  return Result > 0;
}

bool TCustomFarPlugin::Editor(const UnicodeString & AFileName,
  const UnicodeString & Title, EDITOR_FLAGS Flags)
{
  const TFarEnvGuard Guard;
  const intptr_t Result = FStartupInfo.Editor(
      AFileName.c_str(),
      Title.c_str(), 0, 0, -1, -1, Flags, -1, -1,
      CP_DEFAULT);
  return (Result == EEC_MODIFIED) || (Result == EEC_NOT_MODIFIED);
}

void TCustomFarPlugin::ResetCachedInfo()
{
  FValidFarSystemSettings = false;
}

int64_t TCustomFarPlugin::GetSystemSetting(HANDLE & Settings, const wchar_t * Name) const
{
  FarSettingsItem Item = {sizeof(FarSettingsItem), FSSF_SYSTEM, Name, FST_UNKNOWN, {0} };
  if (FStartupInfo.SettingsControl(Settings, SCTL_GET, 0, &Item) && FST_QWORD == Item.Type)
  {
    return nb::ToInt64(Item.Number);
  }
  return 0;
}

intptr_t TCustomFarPlugin::GetFarSystemSettings() const
{
  if (!FValidFarSystemSettings)
  {
    FFarSystemSettings = 0;
    FarSettingsCreate FarSettings = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
    HANDLE Settings = FStartupInfo.SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &Settings) ? FarSettings.Handle : nullptr;
    if (Settings)
    {
      if (GetSystemSetting(Settings, L"DeleteToRecycleBin"))
        FFarSystemSettings |= NBSS_DELETETORECYCLEBIN;

      FStartupInfo.SettingsControl(Settings, SCTL_FREE, 0, nullptr);
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

  const TFarEnvGuard Guard;
  return FStartupInfo.PanelControl(Plugin, Command, Param1, Param2);
}

intptr_t TCustomFarPlugin::FarAdvControl(ADVANCED_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2) const
{
  const TFarEnvGuard Guard;
  return FStartupInfo.AdvControl ?
    FStartupInfo.AdvControl(&NetBoxPluginGuid, Command, Param1, Param2) : 0;
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

  const TFarEnvGuard Guard;
  return FStartupInfo.EditorControl(-1, Command, Param1, Param2);
}

TFarEditorInfo * TCustomFarPlugin::EditorInfo()
{
  TFarEditorInfo * Result = nullptr;
  ::EditorInfo * Info = nb::calloc<::EditorInfo *>(1, sizeof(::EditorInfo));
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
  catch(...)
  {
    nb_free(Info);
    throw;
  }
  return Result;
}

int32_t TCustomFarPlugin::GetFarVersion() const
{
  if (FFarVersion == 0)
  {
    FFarVersion = nb::ToInt32(FarAdvControl(ACTL_GETFARMANAGERVERSION, 0));
  }
  return FFarVersion;
}

UnicodeString TCustomFarPlugin::FormatFarVersion(const VersionInfo & Info) const
{
  return FORMAT("%d.%d.%d", Info.Major, Info.Minor, Info.Build);
}

UnicodeString TCustomFarPlugin::GetTemporaryDir() const
{
  UnicodeString Result(nb::NB_MAX_PATH, 0);
  const TFarEnvGuard Guard;
  FFarStandardFunctions.MkTemp(ToWCharPtr(Result), nb::ToSizeT(Result.Length()), nullptr);
  PackStr(Result);
  return Result;
}

intptr_t TCustomFarPlugin::InputRecordToKey(const INPUT_RECORD * /*Rec*/)
{
  intptr_t  Result;
  /*
  {
    const TFarEnvGuard Guard;
    Result = FFarStandardFunctions.FarInputRecordToKey(Rec);
  }
  else
  */
  {
    Result = 0;
  }
  return nb::ToIntPtr(Result);
}

void TCustomFarPlugin::Initialize()
{
//  ::SetGlobals(new TGlobalFunctions());
  FTIdleThread = std::make_unique<TPluginIdleThread>(this, 400);
  FTIdleThread->InitIdleThread("NetBox IdleThread");
}

void TCustomFarPlugin::Finalize()
{
//  TGlobalsIntf * Intf = GetGlobals();
//  delete Intf;
//  ::SetGlobals(nullptr);
}

#ifdef NETBOX_DEBUG
void TCustomFarPlugin::RunTests()
{
}
#endif

uint32_t TCustomFarFileSystem::FInstances = 0;

TCustomFarFileSystem::TCustomFarFileSystem(TObjectClassId Kind, gsl::not_null<TCustomFarPlugin *> APlugin) noexcept :
  TObject(Kind),
  FPlugin(APlugin)
{
  nb::ClearArray(FPanelInfo);
  nb::ClearStruct(FOpenPanelInfo);
}

void TCustomFarFileSystem::Init()
{
  FPanelInfo[0] = nullptr;
  FPanelInfo[1] = nullptr;
  FClosed = false;

  nb::ClearStruct(FOpenPanelInfo);
  ClearOpenPanelInfo(FOpenPanelInfo);
  FInstances++;
}

TCustomFarFileSystem::~TCustomFarFileSystem() noexcept
{
  FInstances--;
  TINYLOG_TRACE(g_tinylog) << repr("C %d", FInstances);
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
    nb_free(Info.HostFile);
    nb_free(Info.CurDir);
    nb_free(Info.Format);
    nb_free(Info.PanelTitle);
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
    nb_free(Info.PanelModesArray);
    if (Info.KeyBar)
    {
      TFarKeyBarTitles::ClearKeyBarTitles(const_cast<KeyBarTitles &>(*Info.KeyBar));
      nb_free(Info.KeyBar);
    }
    nb_free(Info.ShortcutData);
  }
  nb::ClearStruct(Info);
  Info.StructSize = sizeof(Info);
  InvalidateOpenPanelInfo();
}

void TCustomFarFileSystem::GetOpenPanelInfo(struct OpenPanelInfo * Info)
{
  ResetCachedInfo();
  if (FClosed)
  {
    ClosePanel();
  }
  else
  {
    if (!FOpenPanelInfoValid)
    {
      ClearOpenPanelInfo(FOpenPanelInfo);
      UnicodeString HostFile, CurDir, Format, PanelTitle, ShortcutData;
      std::unique_ptr<TFarPanelModes> PanelModes(std::make_unique<TFarPanelModes>());
      std::unique_ptr<TFarKeyBarTitles> KeyBarTitles(std::make_unique<TFarKeyBarTitles>());
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

int32_t TCustomFarFileSystem::GetFindData(struct GetFindDataInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(std::make_unique<TObjectList>());
  const bool Result = !FClosed && GetFindDataEx(PanelItems.get(), Info->OpMode);
  if (Result && PanelItems->GetCount())
  {
    Info->PanelItem = nb::calloc<PluginPanelItem *>(PanelItems->GetCount(), sizeof(PluginPanelItem));
    Info->ItemsNumber = PanelItems->GetCount();
    for (int32_t Index = 0; Index < PanelItems->GetCount(); ++Index)
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
      nb_free(Info->PanelItem[Index].FileName);
      nb_free(Info->PanelItem[Index].AlternateFileName);
      nb_free(Info->PanelItem[Index].Description);
      nb_free(Info->PanelItem[Index].Owner);
      for (size_t CustomIndex = 0; CustomIndex < Info->PanelItem[Index].CustomColumnNumber; ++CustomIndex)
      {
        nb_free(Info->PanelItem[Index].CustomColumnData[CustomIndex]);
      }
      nb_free(Info->PanelItem[Index].CustomColumnData);
    }
    nb_free(Info->PanelItem);
  }
}

int32_t TCustomFarFileSystem::ProcessHostFile(const struct ProcessHostFileInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, nb::ToInt32(Info->ItemsNumber)));
  const bool Result = ProcessHostFileEx(PanelItems.get(), Info->OpMode);
  return Result;
}

int32_t TCustomFarFileSystem::ProcessPanelInput(const struct ProcessPanelInputInfo * Info)
{
  ResetCachedInfo();
  if (Info->Rec.EventType == KEY_EVENT)
  {
    const KEY_EVENT_RECORD & Event = Info->Rec.Event.KeyEvent;
    return ProcessKeyEx(Event.wVirtualKeyCode,
        Event.dwControlKeyState);
  }
  return FALSE;
}

bool TCustomFarFileSystem::ProcessPanelEvent(intptr_t Event, void * Param)
{
  ResetCachedInfo();
  return ProcessPanelEventEx(Event, Param);
}

int32_t TCustomFarFileSystem::SetDirectory(const struct SetDirectoryInfo * Info)
{
  ResetCachedInfo();
  InvalidateOpenPanelInfo();
  const int32_t Result = SetDirectoryEx(Info->Dir, Info->OpMode);
  InvalidateOpenPanelInfo();
  return Result;
}

int32_t TCustomFarFileSystem::MakeDirectory(struct MakeDirectoryInfo * Info)
{
  ResetCachedInfo();
  FNameStr = Info->Name;
  int32_t Result = 0;
  try__finally
  {
    Result = MakeDirectoryEx(FNameStr, Info->OpMode);
  }
  __finally
  {
    if (0 != wcscmp(FNameStr.c_str(), Info->Name))
    {
      Info->Name = FNameStr.c_str();
    }
  } end_try__finally
  return Result;
}

int32_t TCustomFarFileSystem::DeleteFiles(const struct DeleteFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, nb::ToInt32(Info->ItemsNumber)));
  const bool Result = DeleteFilesEx(PanelItems.get(), Info->OpMode);
  return Result;
}

int32_t TCustomFarFileSystem::GetFiles(struct GetFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, nb::ToInt32(Info->ItemsNumber)));
  int32_t Result;
  FDestPathStr = Info->DestPath;
  try__finally
  {
    Result = GetFilesEx(PanelItems.get(), Info->Move > 0, FDestPathStr, Info->OpMode);
  }
  __finally
  {
    if (FDestPathStr != Info->DestPath)
    {
      Info->DestPath = FDestPathStr.c_str();
    }
  } end_try__finally

  return Result;
}

int32_t TCustomFarFileSystem::PutFiles(const struct PutFilesInfo * Info)
{
  ResetCachedInfo();
  std::unique_ptr<TObjectList> PanelItems(CreatePanelItemList(Info->PanelItem, nb::ToInt32(Info->ItemsNumber)));
  const int32_t Result = PutFilesEx(PanelItems.get(), Info->Move > 0, Info->OpMode);
  return Result;
}

void TCustomFarFileSystem::ResetCachedInfo()
{
  if (FPanelInfo[0])
  {
    SAFE_DESTROY(FPanelInfo[0]);
  }
  if (FPanelInfo[1])
  {
    SAFE_DESTROY(FPanelInfo[1]);
  }
}

TFarPanelInfo * const * TCustomFarFileSystem::GetPanelInfo(int32_t Another) const
{
  return const_cast<TCustomFarFileSystem *>(this)->GetPanelInfo(Another);
}

TFarPanelInfo ** TCustomFarFileSystem::GetPanelInfo(int32_t Another)
{
  const bool bAnother = Another != 0;
  if (FPanelInfo[bAnother] == nullptr)
  {
    PanelInfo * Info = nb::calloc<PanelInfo *>(1, sizeof(PanelInfo));
    Info->StructSize = sizeof(PanelInfo);
    const bool Res = (FPlugin->FarControl(FCTL_GETPANELINFO, 0, nb::ToPtr(Info),
      !bAnother ? PANEL_ACTIVE : PANEL_PASSIVE) > 0);
    if (!Res)
    {
      DebugAssert(false);
    }
    FPanelInfo[bAnother] = new TFarPanelInfo(Info, !bAnother ? this : nullptr);
  }
  return &FPanelInfo[bAnother];
}

int32_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2)
{
  return nb::ToInt32(FPlugin->FarControl(Command, Param1, Param2, this));
}

int32_t TCustomFarFileSystem::FarControl(FILE_CONTROL_COMMANDS Command, intptr_t Param1, void * Param2, HANDLE Plugin)
{
  return nb::ToInt32(FPlugin->FarControl(Command, Param1, Param2, Plugin));
}

bool TCustomFarFileSystem::UpdatePanel(bool ClearSelection, bool Another)
{
  const uint32_t PrevInstances = FInstances;
  InvalidateOpenPanelInfo();
  FPlugin->FarControl(FCTL_UPDATEPANEL, !ClearSelection, nullptr, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
  return (FInstances >= PrevInstances);
}

void TCustomFarFileSystem::RedrawPanel(bool Another)
{
  FPlugin->FarControl(FCTL_REDRAWPANEL, 0, nullptr, Another ? PANEL_PASSIVE : PANEL_ACTIVE);
}

void TCustomFarFileSystem::ClosePanel()
{
  static bool InsideClose = false;
  if (!InsideClose)
  {
    InsideClose = true;
    FClosed = true;
    FarControl(FCTL_CLOSEPANEL, 0, nullptr);
    InsideClose = false;
  }
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

bool TCustomFarFileSystem::ProcessKeyEx(int32_t /*Key*/, uint32_t /*ControlState*/)
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

int32_t TCustomFarFileSystem::MakeDirectoryEx(UnicodeString & /*Name*/, OPERATION_MODES /*OpMode*/)
{
  return -1;
}

bool TCustomFarFileSystem::DeleteFilesEx(TObjectList * /*PanelItems*/, OPERATION_MODES /*OpMode*/)
{
  return false;
}

int32_t TCustomFarFileSystem::GetFilesEx(TObjectList * /*PanelItems*/, bool /*Move*/,
  UnicodeString & /*DestPath*/, OPERATION_MODES /*OpMode*/)
{
  return 0;
}

int32_t TCustomFarFileSystem::PutFilesEx(TObjectList * /*PanelItems*/,
  bool /*Move*/, OPERATION_MODES /*OpMode*/)
{
  return 0;
}

TObjectList * TCustomFarFileSystem::CreatePanelItemList(
  struct PluginPanelItem * PanelItem, int32_t ItemsNumber)
{
  std::unique_ptr<TObjectList> PanelItems(std::make_unique<TObjectList>());
  PanelItems->SetOwnsObjects(true);
  for (int32_t Index = 0; Index < ItemsNumber; ++Index)
  {
    PanelItems->Add(new TFarPanelItem(&PanelItem[Index], false));
  }
  return PanelItems.release();
}

TFarPanelModes::TFarPanelModes() noexcept : TObject(),
  FReferenced(false)
{
  nb::ClearArray(FPanelModes);
}

TFarPanelModes::~TFarPanelModes() noexcept
{
  if (!FReferenced)
  {
    for (int32_t Index = 0; Index < nb::ToInt32(_countof(FPanelModes)); ++Index)
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
  const int32_t ColumnTypesCount = !ColumnTypes.IsEmpty() ? CommaCount(ColumnTypes) + 1 : 0;
  DebugAssert(Mode != nb::NPOS && Mode < _countof(FPanelModes));
  DebugAssert(!ColumnTitles || (ColumnTitles->GetCount() == ColumnTypesCount));

  ClearPanelMode(FPanelModes[Mode]);
  wchar_t ** Titles = nb::calloc<wchar_t **>(1 + ColumnTypesCount, sizeof(wchar_t *));
  FPanelModes[Mode].ColumnTypes = TCustomFarPlugin::DuplicateStr(ColumnTypes);
  FPanelModes[Mode].ColumnWidths = TCustomFarPlugin::DuplicateStr(ColumnWidths);
  if (ColumnTitles)
  {
    for (int32_t Index = 0; Index < ColumnTypesCount; ++Index)
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
    const int32_t ColumnTypesCount = Mode.ColumnTypes ?
      CommaCount(UnicodeString(Mode.ColumnTypes)) + 1 : 0;

    nb_free(Mode.ColumnTypes);
    nb_free(Mode.ColumnWidths);
    if (Mode.ColumnTitles)
    {
      for (int32_t Index = 0; Index < ColumnTypesCount; ++Index)
      {
        nb_free(Mode.ColumnTitles[Index]);
      }
      nb_free(Mode.ColumnTitles);
    }
    nb_free(Mode.StatusColumnTypes);
    nb_free(Mode.StatusColumnWidths);
    nb::ClearStruct(Mode);
  }
}

void TFarPanelModes::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  DebugAssert(Info);
  Info->PanelModesNumber = _countof(FPanelModes);
  PanelMode * PanelModesArray = nb::calloc<PanelMode *>(_countof(FPanelModes), sizeof(PanelMode));
  memmove(PanelModesArray, &FPanelModes, sizeof(FPanelModes));
  Info->PanelModesArray = PanelModesArray;
  FReferenced = true;
}

int32_t TFarPanelModes::CommaCount(const UnicodeString & ColumnTypes)
{
  int32_t Count = 0;
  for (int32_t Index = 1; Index <= ColumnTypes.Length(); ++Index)
  {
    if (ColumnTypes[Index] == L',')
    {
      Count++;
    }
  }
  return Count;
}

TFarKeyBarTitles::TFarKeyBarTitles() noexcept
{
  nb::ClearStruct(FKeyBarTitles);
  constexpr size_t CountLabels = 7 * 12;
  FKeyBarTitles.CountLabels = CountLabels;
  FKeyBarTitles.Labels = static_cast<KeyBarLabel *>(
      nb_malloc(sizeof(KeyBarLabel) * CountLabels));
  memset(FKeyBarTitles.Labels, 0, sizeof(KeyBarLabel) * CountLabels);
}

TFarKeyBarTitles::~TFarKeyBarTitles() noexcept
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
  int32_t FunctionKeyStart, int32_t FunctionKeyEnd)
{
  if (!FunctionKeyEnd)
  {
    FunctionKeyEnd = FunctionKeyStart;
  }
  for (int32_t Index = FunctionKeyStart; Index <= FunctionKeyEnd; ++Index)
  {
    SetKeyBarTitle(ShiftStatus, Index, L"");
  }
}

void TFarKeyBarTitles::SetKeyBarTitle(TFarShiftStatus ShiftStatus,
  int32_t FunctionKey, const UnicodeString & Title)
{
  DebugAssert(FunctionKey >= 1 && FunctionKey <= 12);
  const int32_t shift = nb::ToInt32(ShiftStatus);
  DebugAssert(shift >= 0 && shift < 7);
  KeyBarLabel * Labels = &FKeyBarTitles.Labels[shift * 12];
  if (Labels[FunctionKey - 1].Key.VirtualKeyCode)
  {
    nb_free(Labels[FunctionKey - 1].Text);
    nb_free(Labels[FunctionKey - 1].LongText);
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
  Labels[FunctionKey - 1].Key.VirtualKeyCode = VK_F1 + static_cast<WORD>(FunctionKey) - 1;
  Labels[FunctionKey - 1].Key.ControlKeyState = FKeys[shift];
  Labels[FunctionKey - 1].Text = TCustomFarPlugin::DuplicateStr(Title, /*AllowEmpty=*/true);
  Labels[FunctionKey - 1].LongText = nullptr;
}

void TFarKeyBarTitles::ClearKeyBarTitles(KeyBarTitles &Titles)
{
  for (size_t Index = 0; Index < Titles.CountLabels; ++Index)
  {
    nb_free(Titles.Labels[Index].Text);
    nb_free(Titles.Labels[Index].LongText);
  }
  nb_free(Titles.Labels);
  Titles.Labels = nullptr;
  Titles.CountLabels = 0;
}

void TFarKeyBarTitles::FillOpenPanelInfo(struct OpenPanelInfo * Info)
{
  DebugAssert(Info);
  KeyBarTitles * KeyBar = nb::calloc<KeyBarTitles *>(1, sizeof(KeyBarTitles));
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
  const FILETIME FileTime = ::DateTimeToFileTime(LastWriteTime, dstmWin);
  const FILETIME FileTimeA = ::DateTimeToFileTime(LastAccess, dstmWin);
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
  wchar_t ** CustomColumnData = nb::calloc<wchar_t **>(1 + PanelItem->CustomColumnNumber, sizeof(wchar_t *));
  for (size_t Index = 0; Index < PanelItem->CustomColumnNumber; ++Index)
  {
    CustomColumnData[Index] =
      TCustomFarPlugin::DuplicateStr(GetCustomColumnData(Index));
  }
  PanelItem->CustomColumnData = CustomColumnData;
}

TFarPanelItem::TFarPanelItem(PluginPanelItem * APanelItem, bool OwnsItem) noexcept :
  TCustomFarPanelItem(OBJECT_CLASS_TFarPanelItem),
  FPanelItem(APanelItem),
  FOwnsItem(OwnsItem)
{
  DebugAssert(FPanelItem);
}

TFarPanelItem::~TFarPanelItem() noexcept
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
  return nb::ToUIntPtr(FPanelItem->Flags);
}

UnicodeString TFarPanelItem::GetFileName() const
{
  // UnicodeString Result = FPanelItem->AlternateFileName ? FPanelItem->AlternateFileName : FPanelItem->FileName;
  const UnicodeString Result = FPanelItem->FileName;
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

uint32_t TFarPanelItem::GetFileAttrs() const
{
  return nb::ToUInt32(FPanelItem->FileAttributes);
}

bool TFarPanelItem::GetIsParentDirectory() const
{
  return (GetFileName() == PARENTDIRECTORY);
}

bool TFarPanelItem::GetIsFile() const
{
  return (GetFileAttrs() & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

THintPanelItem::THintPanelItem(const UnicodeString & AHint) noexcept :
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

TFarPanelInfo::TFarPanelInfo(PanelInfo * APanelInfo, TCustomFarFileSystem * AOwner) noexcept :
  TObject(),
  FPanelInfo(APanelInfo),
  FOwner(AOwner)
{
  DebugAssert(FPanelInfo);
}

TFarPanelInfo::~TFarPanelInfo() noexcept
{
  nb_free(FPanelInfo);
  SAFE_DESTROY(FItems);
}

int32_t TFarPanelInfo::GetItemCount() const
{
  return nb::ToInt32(FPanelInfo->ItemsNumber);
}

TRect TFarPanelInfo::GetBounds() const
{
  const RECT rect = FPanelInfo->PanelRect;
  return TRect(rect.left, rect.top, rect.right, rect.bottom);
}

int32_t TFarPanelInfo::GetSelectedCount(bool CountCurrentItem) const
{
  int32_t Count = nb::ToInt32(FPanelInfo->SelectedItemsNumber);

  if ((Count == 1) && FOwner && !CountCurrentItem)
  {
    const int32_t size = FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, nullptr);
    PluginPanelItem * ppi = nb::calloc<PluginPanelItem *>(1, size);
    FOwner->FarControl(FCTL_GETSELECTEDPANELITEM, 0, nb::ToPtr(ppi));
    if ((ppi->Flags & PPIF_SELECTED) == 0)
    {
      Count = 0;
    }
    nb_free(ppi);
  }

  return Count;
}

TObjectList * TFarPanelInfo::GetItems() const
{
  if (FItems != nullptr)
  {
    return FItems;
  }
  FItems = new TObjectList();
  if (FOwner)
  {
    for (size_t Index = 0; Index < FPanelInfo->ItemsNumber; ++Index)
    {
      TODO("move to common function");
      const int32_t Size = FOwner->FarControl(FCTL_GETPANELITEM, Index, nullptr);
      PluginPanelItem * ppi = nb::calloc<PluginPanelItem *>(1, Size);
      FarGetPluginPanelItem gppi{};
      nb::ClearStruct(gppi);
      gppi.StructSize = sizeof(FarGetPluginPanelItem);
      gppi.Size = Size;
      gppi.Item = ppi;
      FOwner->FarControl(FCTL_GETPANELITEM, Index, nb::ToPtr(&gppi));
      FItems->Add(new TFarPanelItem(ppi, /*OwnsItem=*/true));
    }
  }
  return FItems;
}

const TFarPanelItem * TFarPanelInfo::FindFileName(const UnicodeString & AFileName) const
{
  const TObjectList * Items = FItems;
  if (!Items)
    Items = GetItems();
  for (int32_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    const TFarPanelItem * PanelItem = Items->As<TFarPanelItem>(Index);
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
  for (int32_t Index = 0; Index < Items->GetCount(); ++Index)
  {
    TFarPanelItem * PanelItem = Items->GetAs<TFarPanelItem>(Index);
    Ensures(PanelItem);
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
  FOwner->FarControl(FCTL_SETSELECTION, 0, nb::ToPtr(FPanelInfo));
}

const TFarPanelItem * TFarPanelInfo::GetFocusedItem() const
{
  const TObjectList * Items = FItems;
  if (!Items)
    Items = GetItems();
  const int32_t Index = GetFocusedIndex();
  if (Items->GetCount() > 0)
  {
    DebugAssert(Index < Items->GetCount());
    return Items->As<TFarPanelItem>(Index);
  }
  return nullptr;
}

void TFarPanelInfo::SetFocusedItem(const TFarPanelItem * Value)
{
  if (FItems && FItems->GetCount())
  {
    const int32_t Index = FItems->IndexOf(Value);
    DebugAssert(Index != nb::NPOS);
    SetFocusedIndex(Index);
  }
}

int32_t TFarPanelInfo::GetFocusedIndex() const
{
  return nb::ToInt32(FPanelInfo->CurrentItem);
}

void TFarPanelInfo::SetFocusedIndex(int32_t Value)
{
  // for "another panel info", there's no owner
  DebugAssert(FOwner != nullptr);
  if (GetFocusedIndex() != Value)
  {
    DebugAssert(Value != nb::NPOS && Value < nb::ToInt32(FPanelInfo->ItemsNumber));
    FPanelInfo->CurrentItem = nb::ToInt32(Value);
    PanelRedrawInfo PanelInfo{};
    nb::ClearStruct(PanelInfo);
    PanelInfo.StructSize = sizeof(PanelRedrawInfo);
    PanelInfo.CurrentItem = FPanelInfo->CurrentItem;
    PanelInfo.TopPanelItem = FPanelInfo->TopPanelItem;
    FOwner->FarControl(FCTL_REDRAWPANEL, 0, nb::ToPtr(&PanelInfo));
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
  return CheckHandle(FPanelInfo->PluginHandle);
}

UnicodeString TFarPanelInfo::GetCurrentDirectory() const
{
  UnicodeString Result;
  const intptr_t Size = FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
    0,
    nullptr,
    FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
  if (Size)
  {
    FarPanelDirectory * pfpd = nb::calloc<FarPanelDirectory *>(1, Size);
    pfpd->StructSize = sizeof(FarPanelDirectory);

    FarPlugin->FarControl(FCTL_GETPANELDIRECTORY,
      Size,
      pfpd,
      FOwner != nullptr ? PANEL_ACTIVE : PANEL_PASSIVE);
    Result = pfpd->Name;
    nb_free(pfpd);
  }
  PackStr(Result);
  return Result;
}

TFarMenuItems::TFarMenuItems() noexcept :
  TStringList(OBJECT_CLASS_TFarMenuItems)
{
}

void TFarMenuItems::Clear()
{
  FItemFocused = nb::NPOS;
  TStringList::Clear();
}

void TFarMenuItems::Delete(int32_t Index)
{
  if (Index == FItemFocused)
  {
    FItemFocused = nb::NPOS;
  }
  TStringList::Delete(Index);
}

void TFarMenuItems::SetObject(int32_t Index, TObject * AObject)
{
  TStringList::SetObject(Index, AObject);
  const bool Focused = (nb::ToUIntPtr(AObject) & MIF_SEPARATOR) != 0;
  if ((Index == GetItemFocused()) && !Focused)
  {
    FItemFocused = nb::NPOS;
  }
  if (Focused)
  {
    if (GetItemFocused() != nb::NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Index;
  }
}

int32_t TFarMenuItems::Add(const UnicodeString & Text, bool Visible)
{
  const int32_t Result = TStringList::Add(Text);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
  return Result;
}

void TFarMenuItems::AddSeparator(bool Visible)
{
  Add("");
  SetFlag(GetCount() - 1, MIF_SEPARATOR, true);
  if (!Visible)
  {
    SetFlag(GetCount() - 1, MIF_HIDDEN, true);
  }
}

void TFarMenuItems::SetItemFocused(int32_t Value)
{
  if (GetItemFocused() != Value)
  {
    if (GetItemFocused() != nb::NPOS)
    {
      SetFlag(GetItemFocused(), MIF_SELECTED, false);
    }
    FItemFocused = Value;
    SetFlag(GetItemFocused(), MIF_SELECTED, true);
  }
}

void TFarMenuItems::SetFlag(int32_t Index, uint32_t Flag, bool Value)
{
  if (GetFlag(Index, Flag) != Value)
  {
    uint32_t F = nb::ToUInt32(nb::ToUIntPtr(Objects[Index]));
    if (Value)
    {
      F |= Flag;
    }
    else
    {
      F &= ~Flag;
    }
    SetObject(Index, ToObj(F));
  }
}

bool TFarMenuItems::GetFlag(int32_t Index, uint32_t Flag) const
{
  return (nb::ToUIntPtr(Objects[Index]) & Flag) != 0;
}

TFarEditorInfo::TFarEditorInfo(EditorInfo * Info) noexcept :
  FEditorInfo(Info)
{
}

TFarEditorInfo::~TFarEditorInfo() noexcept
{
  nb_free(FEditorInfo);
}

int32_t TFarEditorInfo::GetEditorID() const
{
  return nb::ToInt32(FEditorInfo->EditorID);
}

UnicodeString TFarEditorInfo::GetFileName()
{
  UnicodeString Result;
  const intptr_t BuffLen = FarPlugin->FarEditorControl(ECTL_GETFILENAME, 0, nullptr);
  if (BuffLen)
  {
    wchar_t * Buffer = Result.SetLength(nb::ToInt32(BuffLen + 1));
    FarPlugin->FarEditorControl(ECTL_GETFILENAME, BuffLen, Buffer);
  }
  PackStr(Result);
  return Result;
}

TFarEnvGuard::TFarEnvGuard() noexcept
{
  DebugAssert(FarPlugin != nullptr);
}

TFarEnvGuard::~TFarEnvGuard() noexcept
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

TFarPluginEnvGuard::TFarPluginEnvGuard() noexcept
{
  DebugAssert(FarPlugin != nullptr);
}

TFarPluginEnvGuard::~TFarPluginEnvGuard() noexcept
{
  DebugAssert(FarPlugin != nullptr);
}

HINSTANCE TGlobalFunctions::GetInstanceHandle() const
{
  HINSTANCE Result = nullptr;
  if (FarPlugin)
  {
    Result = FarPlugin->GetPluginHandle();
  }
  return Result;
}

UnicodeString TGlobalFunctions::GetMsg(int32_t Id) const
{
  UnicodeString Result;
  if (FarPlugin != nullptr)
  {
    // map Id to PluginString value
    int32_t PluginStringId = -1;
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
  
    if (FarPlugin && (PluginStringId != -1)) 
      Result = FarPlugin->GetMsg(PluginStringId);
  }

  if (Result.IsEmpty())
  {
    const HINSTANCE Instance = GetInstanceHandle();
    Result = ::LoadStrFrom(Instance, Id);
  }
  return Result;
}

UnicodeString TGlobalFunctions::GetCurrentDirectory() const
{
  UnicodeString Path(nb::NB_MAX_PATH, 0);
  int32_t Length;
  if (FarPlugin)
  {
    Length = nb::ToInt32(FarPlugin->GetFarStandardFunctions().GetCurrentDirectory(nb::ToDWord(Path.Length()), ToWCharPtr(Path)) - 1);
  }
  else
  {
    Length = nb::ToInt32(::GetCurrentDirectoryW(nb::ToDWord(Path.Length()), ToWCharPtr(Path)));
  }
  UnicodeString Result = UnicodeString(Path.c_str(), Length);
  return Result;
}

UnicodeString TGlobalFunctions::GetStrVersionNumber() const
{
  return FORMAT("%d.%d.%d.%d", NETBOX_VERSION_MAJOR, NETBOX_VERSION_MINOR, NETBOX_VERSION_PATCH, NETBOX_VERSION_BUILD);
}

bool TGlobalFunctions::InputDialog(const UnicodeString & ACaption, const UnicodeString & APrompt,
  UnicodeString & Value, const UnicodeString & HelpKeyword,
  TStrings * History, bool PathInput,
  TInputDialogInitializeEvent && OnInitialize, bool Echo)
{
  DebugUsedParam(HelpKeyword);
  DebugUsedParam(History);
  DebugUsedParam(PathInput);
  DebugUsedParam(OnInitialize);
  DebugUsedParam(Echo);

  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  Ensures(WinSCPPlugin);
  return WinSCPPlugin->InputBox(ACaption, APrompt, Value, 0);
}

uint32_t TGlobalFunctions::MoreMessageDialog(const UnicodeString & AMessage, TStrings * MoreMessages, TQueryType Type, uint32_t Answers, const TMessageParams * Params)
{
  TWinSCPPlugin * WinSCPPlugin = rtti::dyn_cast_or_null<TWinSCPPlugin>(FarPlugin);
  Ensures(WinSCPPlugin);
  return WinSCPPlugin->MoreMessageDialog(AMessage, MoreMessages, Type, Answers, Params);
}
