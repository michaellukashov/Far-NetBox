#include "afxdll.h"
#include <vcl.h>

#include <Common.h>
#include <Sysutils.hpp>

#include "FarTexts.h"
#include "FarUtil.h"
#include "resource.h"
#include "plugin_version.hpp"

extern TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst);

class TFarPluginGuard : public TFarPluginEnvGuard, public TGuard
{
public:
  inline TFarPluginGuard() :
    TGuard(FarPlugin->GetCriticalSection())
  {
  }
};

extern "C"
{

void WINAPI GetGlobalInfoW(struct GlobalInfo * Info)
{
  if (Info->StructSize < sizeof(GlobalInfo))
    return;
  Info->StructSize = sizeof(*Info);
  Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
  Info->Version = MAKEFARVERSION(NETBOX_VERSION_MAJOR, NETBOX_VERSION_MINOR, NETBOX_VERSION_SUBMINOR, NETBOX_VERSION_BUILD, VS_RELEASE);
  Info->Guid = MainGuid;
  Info->Title = L"NetBox";
  Info->Description = PLUGIN_DESCR;
  Info->Author = PLUGIN_AUTHOR;
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo * Info)
{
  if (Info->StructSize < sizeof(PluginStartupInfo))
    return;
  TFarPluginGuard Guard;
  FarPlugin->SetStartupInfo(Info);
}

void WINAPI ExitFARW(const struct ExitInfo * Info)
{
  if (Info->StructSize < sizeof(ExitInfo))
    return;
  TFarPluginGuard Guard;
  FarPlugin->ExitFAR();
}

void WINAPI GetPluginInfoW(PluginInfo * Info)
{
  if (Info->StructSize < sizeof(PluginInfo))
    return;
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  FarPlugin->GetPluginInfo(Info);
}

intptr_t WINAPI ConfigureW(const struct ConfigureInfo * Info)
{
  if (Info->StructSize < sizeof(ConfigureInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->Configure(Info);
}

void WINAPI ClosePanelW(const struct ClosePanelInfo * Info)
{
  if (Info->StructSize < sizeof(ClosePanelInfo))
    return;
  TFarPluginGuard Guard;
  FarPlugin->ClosePanel(Info->hPanel);
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo * Info)
{
  if (Info->StructSize < sizeof(OpenPanelInfo))
    return;
  TFarPluginGuard Guard;
  FarPlugin->GetOpenPanelInfo(Info);
}

intptr_t WINAPI GetFindDataW(struct GetFindDataInfo * Info)
{
  if (Info->StructSize < sizeof(GetFindDataInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->GetFindData(Info);
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo * Info)
{
  if (Info->StructSize < sizeof(FreeFindDataInfo))
    return;
  TFarPluginGuard Guard;
  FarPlugin->FreeFindData(Info);
}

intptr_t WINAPI ProcessHostFileW(const struct ProcessHostFileInfo * Info)
{
  if (Info->StructSize < sizeof(ProcessHostFileInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->ProcessHostFile(Info);
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo * Info)
{
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  return FarPlugin->ProcessPanelInput(Info);
}

intptr_t WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo * Info)
{
  if (Info->StructSize < sizeof(ProcessPanelEventInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->ProcessPanelEvent(Info);
}

intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo * Info)
{
  if (Info->StructSize < sizeof(SetDirectoryInfo))
    return FALSE;
  TFarPluginGuard Guard;
  intptr_t Result = FarPlugin->SetDirectory(Info);
  return Result;
}

intptr_t WINAPI MakeDirectoryW(struct MakeDirectoryInfo * Info)
{
  if (Info->StructSize < sizeof(MakeDirectoryInfo))
    return FALSE;
  TFarPluginGuard Guard;
  intptr_t Result = FarPlugin->MakeDirectory(Info);
  return Result;
}

intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo * Info)
{
  if (Info->StructSize < sizeof(DeleteFilesInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->DeleteFiles(Info);
}

intptr_t WINAPI GetFilesW(struct GetFilesInfo * Info)
{
  if (Info->StructSize < sizeof(GetFilesInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->GetFiles(Info);
}

intptr_t WINAPI PutFilesW(const struct PutFilesInfo * Info)
{
  if (Info->StructSize < sizeof(PutFilesInfo))
    return FALSE;
  TFarPluginGuard Guard;
  intptr_t Result = FarPlugin->PutFiles(Info);
  return Result;
}

intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo * Info)
{
  if (Info->StructSize < sizeof(ProcessEditorEventInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->ProcessEditorEvent(Info);
}

intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo * Info)
{
  if (Info->StructSize < sizeof(ProcessEditorInputInfo))
    return FALSE;
  TFarPluginGuard Guard;
  return FarPlugin->ProcessEditorInput(Info);
}

HANDLE WINAPI AnalyseW(const struct AnalyseInfo * Info)
{
  if (Info->StructSize < sizeof(AnalyseInfo))
    return nullptr;
  TFarPluginGuard Guard;
  if (!Info->FileName)
  {
    return nullptr;
  }

  const size_t fileNameLen = wcslen(Info->FileName);
  if (fileNameLen < 8 || _wcsicmp(Info->FileName + fileNameLen - 7, L".netbox") != 0)
  {
    return nullptr;
  }
  if (Info->BufferSize > 4 && strncmp(reinterpret_cast<const char *>(Info->Buffer), "<?xml", 5) != 0)
  {
    return nullptr;
  }
  return HANDLE(1);
}

HANDLE WINAPI OpenW(const struct OpenInfo * Info)
{
  if (Info->StructSize < sizeof(OpenInfo))
    return nullptr;
  DebugAssert(FarPlugin);
  TFarPluginGuard Guard;
  HANDLE Handle = static_cast<HANDLE>(FarPlugin->OpenPlugin(Info));
  return Handle;
}

static int Processes = 0;

BOOL DllProcessAttach(HINSTANCE HInstance)
{
  FarPlugin = CreateFarPlugin(HInstance);

  DebugAssert(!Processes);
  Processes++;
  InitExtensionModule(HInstance);
  return TRUE;
}

BOOL DllProcessDetach()
{
  DebugAssert(Processes);
  Processes--;
  if (!Processes)
  {
    DebugAssert(FarPlugin);
    SAFE_DESTROY(FarPlugin);
    TermExtensionModule();
  }
  return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE HInstance, DWORD Reason, LPVOID /*ptr*/ )
{
  BOOL Result = TRUE;
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      Result = DllProcessAttach(HInstance);
      break;

    case DLL_PROCESS_DETACH:
      Result = DllProcessDetach();
      break;
  }
  return Result;
}

} // extern "C"
