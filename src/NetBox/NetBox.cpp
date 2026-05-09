#include "afxdll.h"
#include <vcl.h>

#include <plugin.hpp>
#include <Sysutils.hpp>
#include <nbutils.h>

#include "resource.h"
#include "FarDialog.h"
#include "plugin_version.hpp"

static HINSTANCE HInstanceDLL;

// extern void InitExtensionModule(HINSTANCE HInst);
// extern void TermExtensionModule();
extern TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst);
extern void DestroyFarPlugin(TCustomFarPlugin *& Plugin);
extern void CleanupVCLCommon();
// TFarPluginGuard acquires the global plugin lock for the entire export call.
// CRITICAL: any modal dialog or message loop run while this lock is held
// will deadlock if Far dispatches a keyboard/mouse event to another plugin
// export that tries to acquire the same lock. Use TUnguard to temporarily
// release the lock before showing modal UI. See docs/far3-plugin-init-deadlock.md
class TFarPluginGuard final : public TFarPluginEnvGuard, public TGuard
{
public:
  TFarPluginGuard() :
    TGuard(FarPlugin->GetCriticalSection())
  {
  }
};

void CreatePlugin()
{
  FarPlugin = CreateFarPlugin(HInstanceDLL);
#if !defined(NO_FILEZILLA)
  InitExtensionModule(HInstanceDLL);
#endif //if !defined(NO_FILEZILLA)
}

void DestroyPlugin()
{
  DestroyFarPlugin(FarPlugin);
  // CleanupVCLCommon();
  TermExtensionModule();
}

[[noreturn]]
void InvalidParameterHandler(const wchar_t * expression, const wchar_t * function,
  const wchar_t * file, unsigned int line, uintptr_t pReserved)
{
  (void)expression;
  (void)function;
  (void)file;
  (void)line;
  (void)pReserved;
  RaiseException(STATUS_INVALID_PARAMETER, 0, 0, nullptr);
}


// Helper to log current console mode for debugging console state changes.
static void LogConsoleMode(const char * Label)
{
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  DWORD Mode = 0;
  if (GetConsoleMode(hInput, &Mode))
  {
    DEBUG_PRINTFA("ConsoleMode %s: 0x%08X (LINE=%s ECHO=%s PROCESSED=%s WINDOW=%s MOUSE=%s EXTENDED=%s)",
      Label, Mode,
      (Mode & ENABLE_LINE_INPUT) ? "yes" : "no",
      (Mode & ENABLE_ECHO_INPUT) ? "yes" : "no",
      (Mode & ENABLE_PROCESSED_INPUT) ? "yes" : "no",
      (Mode & ENABLE_WINDOW_INPUT) ? "yes" : "no",
      (Mode & ENABLE_MOUSE_INPUT) ? "yes" : "no",
      (Mode & ENABLE_EXTENDED_FLAGS) ? "yes" : "no");
  }
  else
  {
    DEBUG_PRINTFA("ConsoleMode %s: FAILED", Label);
  }
}

extern "C" {
// Export tracer for deadlock debugging. Logs ENTER/LEAVE to DebugView.
struct TExportTracer {
  const char * FName;
  explicit TExportTracer(const char * Name) noexcept : FName(Name) {
    DEBUG_PRINTFA("ENTER %s", Name);
  }
  ~TExportTracer() noexcept {
    DEBUG_PRINTFA("LEAVE %s", FName);
  }
  NB_DISABLE_COPY(TExportTracer)
};

void WINAPI GetGlobalInfoW(struct GlobalInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(GlobalInfo)))
    return;
  CreatePlugin();
  Info->StructSize = sizeof(*Info);
  Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
  Info->Version = MAKEFARVERSION(NETBOX_VERSION_MAJOR, NETBOX_VERSION_MINOR, NETBOX_VERSION_PATCH, NETBOX_VERSION_BUILD, VS_RELEASE);
  Info->Guid = NetBoxPluginGuid;
  Info->Title = L"NetBox";
  Info->Description = PLUGIN_DESCR;
  Info->Author = PLUGIN_AUTHOR;
  Info->Instance = FarPlugin;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo * Info)
{
  LogConsoleMode("BEFORE-SetStartupInfoW");
  const TExportTracer Tracer("SetStartupInfoW");
  if (!Info || (Info->StructSize < sizeof(PluginStartupInfo)))
    return;
  const TFarPluginGuard Guard;
  FarPlugin->SetStartupInfo(Info);
  LogConsoleMode("AFTER-SetStartupInfoW");
}

void WINAPI ExitFARW(const struct ExitInfo * Info)
{
  const TExportTracer Tracer("ExitFARW");
  if (!Info || (Info->StructSize < sizeof(ExitInfo)))
    return;
  {
    const TFarPluginGuard Guard;
    FarPlugin->ExitFAR();
  }
  // Now Guard is released
  DestroyPlugin();
}

void WINAPI GetPluginInfoW(PluginInfo * Info)
{
  LogConsoleMode("BEFORE-GetPluginInfoW");
  const TExportTracer Tracer("GetPluginInfoW");
  if (!Info || (Info->StructSize < sizeof(PluginInfo)))
    return;
  DebugAssert(FarPlugin);
  const TFarPluginGuard Guard;
  FarPlugin->GetPluginInfo(Info);
  LogConsoleMode("AFTER-GetPluginInfoW");
}

intptr_t WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo * Info)
{
  const TExportTracer Tracer("ProcessSynchroEventW");
  // DEBUG_PRINTF("Info: %p", Info->Param);
  if (!Info || (Info->StructSize < sizeof(ProcessSynchroEventInfo)))
    return 0;
  DebugAssert(FarPlugin);
  const TFarPluginGuard Guard;
  return FarPlugin->ProcessSynchroEvent(Info);
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo * Info)
{
  const TExportTracer Tracer("ConfigureW");
  if (!Info || (Info->StructSize < sizeof(ConfigureInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->Configure(Info);
}

void WINAPI ClosePanelW(const struct ClosePanelInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(ClosePanelInfo)))
    return;
  const TFarPluginGuard Guard;
  FarPlugin->ClosePanel(Info->hPanel);
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(OpenPanelInfo)))
    return;
  const TFarPluginGuard Guard;
  FarPlugin->GetOpenPanelInfo(Info);
}

intptr_t WINAPI GetFindDataW(struct GetFindDataInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(GetFindDataInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->GetFindData(Info);
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(FreeFindDataInfo)))
    return;
  const TFarPluginGuard Guard;
  FarPlugin->FreeFindData(Info);
}

intptr_t WINAPI ProcessHostFileW(const struct ProcessHostFileInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(ProcessHostFileInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->ProcessHostFile(Info);
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo * Info)
{
  LogConsoleMode("BEFORE-ProcessPanelInputW");
  const TExportTracer Tracer("ProcessPanelInputW");
  DEBUG_PRINTFA("ProcessPanelInputW: about to acquire lock");
  DebugAssert(Info && FarPlugin);
  const TFarPluginGuard Guard;
  DEBUG_PRINTFA("ProcessPanelInputW: lock acquired, calling ProcessPanelInput");
  const intptr_t Result = FarPlugin->ProcessPanelInput(Info);
  DEBUG_PRINTFA("ProcessPanelInputW: returning");
  LogConsoleMode("AFTER-ProcessPanelInputW");
  return Result;
}

intptr_t WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo * Info)
{
  LogConsoleMode("BEFORE-ProcessPanelEventW");
  const TExportTracer Tracer("ProcessPanelEventW");
  if (!Info || (Info->StructSize < sizeof(ProcessPanelEventInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  const intptr_t Result = FarPlugin->ProcessPanelEvent(Info);
  LogConsoleMode("AFTER-ProcessPanelEventW");
  return Result;
}

intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(SetDirectoryInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  const intptr_t Result = FarPlugin->SetDirectory(Info);
  return Result;
}

intptr_t WINAPI MakeDirectoryW(struct MakeDirectoryInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(MakeDirectoryInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  const intptr_t Result = FarPlugin->MakeDirectory(Info);
  return Result;
}

intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(DeleteFilesInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->DeleteFiles(Info);
}

intptr_t WINAPI GetFilesW(struct GetFilesInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(GetFilesInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->GetFiles(Info);
}

intptr_t WINAPI PutFilesW(const struct PutFilesInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(PutFilesInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  const intptr_t Result = FarPlugin->PutFiles(Info);
  return Result;
}

intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(ProcessEditorEventInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->ProcessEditorEvent(Info);
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(ProcessEditorInputInfo)))
    return FALSE;
  const TFarPluginGuard Guard;
  return FarPlugin->ProcessEditorInput(Info);
}

HANDLE WINAPI AnalyseW(const struct AnalyseInfo * Info)
{
  if (!Info || (Info->StructSize < sizeof(AnalyseInfo)))
    return nullptr;
  const TFarPluginGuard Guard;
  if (Info->OpMode & (OPM_FIND | OPM_COMMANDS))
  {
    return nullptr;
  }
  if (!Info->FileName)
  {
    return nullptr;
  }

  const size_t fileNameLen = nb::StrLength(Info->FileName);
  if (fileNameLen < 8 || _wcsicmp(Info->FileName + fileNameLen - 7, L".netbox") != 0)
  {
    return nullptr;
  }
  if (Info->BufferSize > 4 && strncmp(static_cast<const char *>(Info->Buffer), "<?xml", 5) != 0)
  {
    return nullptr;
  }
  return reinterpret_cast<HANDLE>(1);
}

HANDLE WINAPI OpenW(const struct OpenInfo * Info)
{
  const TExportTracer Tracer("OpenW");
  if (!Info || (Info->StructSize < sizeof(OpenInfo)))
    return nullptr;
  DebugAssert(FarPlugin);
  const TFarPluginGuard Guard;
  const HANDLE Handle = static_cast<HANDLE>(FarPlugin->OpenPlugin(Info));
  if (!Handle && Info->OpenFrom == OPEN_ANALYSE)
  {
    return PANEL_STOP;
  }
  return Handle;
}

BOOL WINAPI DllMain(HINSTANCE HInstDLL, DWORD Reason, LPVOID /*ptr*/ )
{
  if (Reason == DLL_PROCESS_ATTACH)
  {
    HInstanceDLL = HInstDLL;
    (void) _set_invalid_parameter_handler(InvalidParameterHandler);
  }
  return TRUE;
}

} // extern "C"
