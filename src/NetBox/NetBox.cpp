#include "FarTexts.h"

#ifdef _AFXDLL
#include "afxdll.h"
#endif

#include "stdafx.h"
#include "EasyURL.h"
#include "resource.h"
#include "Common.h"

//---------------------------------------------------------------------------
#ifdef NETBOX_DEBUG
// static _CrtMemState s1, s2, s3;
// static HANDLE hLogFile;
#endif

//---------------------------------------------------------------------------
extern TCustomFarPlugin *CreateFarPlugin(HINSTANCE HInst);

//---------------------------------------------------------------------------
class TFarPluginGuard : public TFarPluginEnvGuard, public TGuard
{
public:
  inline TFarPluginGuard() :
    TGuard(FarPlugin->GetCriticalSection())
  {
  }
};

//---------------------------------------------------------------------------
extern "C"
{

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
    Info->StructSize = sizeof(*Info);
    Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
    Info->Version = MAKEFARVERSION(2, 0, 4, 0, VS_RELEASE);
    Info->Guid = MainGuid;
    Info->Title = L"";
    Info->Description = L"";
    Info->Author = L"";
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *psi)
{
    assert(FarPlugin);
#ifdef NETBOX_DEBUG
    // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    // _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    // hLogFile = CreateFile(L"C:\\NetBoxDebug.txt", FILE_APPEND_DATA, // GENERIC_WRITE,
      // FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
      // FILE_ATTRIBUTE_NORMAL, NULL);
    // _CrtSetReportFile(_CRT_WARN, hLogFile);
#endif
    TFarPluginGuard Guard;
    CFarPlugin::Initialize(psi);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    curl_global_init(CURL_GLOBAL_ALL);
    FarPlugin->SetStartupInfo(psi);
}

void WINAPI ExitFARW(const struct ExitInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    curl_global_cleanup();
    WSACleanup();

    FarPlugin->ExitFAR();
}

void WINAPI GetPluginInfoW(PluginInfo *pi)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    FarPlugin->GetPluginInfo(pi);
}

int WINAPI ConfigureW(const struct ConfigureInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->Configure(Info);
}

HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
{
  // GC_find_leak = 1;
#ifdef NETBOX_DEBUG
  // _CrtMemCheckpoint(&s1);
#endif
  assert(FarPlugin);
  TFarPluginGuard Guard;
  return FarPlugin->OpenPlugin(openFrom, item);
}

// TODO: void WINAPI ClosePanelW(const struct ClosePanelInfo *Info);
void WINAPI ClosePluginW(HANDLE plugin)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    FarPlugin->ClosePlugin(plugin);
#ifdef NETBOX_DEBUG
    // _CrtMemCheckpoint(&s2);
    // if (_CrtMemDifference(&s3, &s1, &s2)) 
        // _CrtMemDumpStatistics(&s3);
    // _CrtDumpMemoryLeaks();
    // CloseHandle(hLogFile);
#endif
    // GC_gcollect();
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    FarPlugin->GetOpenPanelInfo(Info);
}

int WINAPI GetFindDataW(struct GetFindDataInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->GetFindData(Info);
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    FarPlugin->FreeFindData(Info);
}

int WINAPI ProcessHostFileW(const struct ProcessHostFileInfo *Info)
{
  assert(FarPlugin);
  TFarPluginGuard Guard;
  return FarPlugin->ProcessHostFile(Info);
}

int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessKey(plugin, key, controlState);
}

// TODO: int WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info);
// TODO: int WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info);

int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEvent(Plugin, Event, Param);
}

int WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info)
{
    DEBUG_PRINTF(L"begin, dir = %s", Info->Dir);
    assert(FarPlugin);
    TFarPluginGuard Guard;
    int result = FarPlugin->SetDirectory(Info);
    DEBUG_PRINTF(L"end, result = %d", result);
    return result;
}

int WINAPI MakeDirectoryW(struct MakeDirectoryInfo *Info)
{
    DEBUG_PRINTF(L"begin, name = %s", Info->Name);
    assert(FarPlugin);
    TFarPluginGuard Guard;
    int result = FarPlugin->MakeDirectory(Info);
    DEBUG_PRINTF(L"end, result = %d", result);
    return result;
}

int WINAPI DeleteFilesW(const struct DeleteFilesInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->DeleteFiles(Info);
}

int WINAPI GetFilesW(struct GetFilesInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->GetFiles(Info);
}

int WINAPI PutFilesW(const struct PutFilesInfo *Info)
{
    DEBUG_PRINTF(L"begin, srcPath = %s", Info->SrcPath);
    assert(FarPlugin);
    TFarPluginGuard Guard;
    int result = FarPlugin->PutFiles(Info);
    DEBUG_PRINTF(L"end, result = %d", result);
    return result;
}

int WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEditorEvent(Info);
}

int WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEditorInput(Info);
}

// TODO: HANDLE WINAPI OpenW(const struct OpenInfo *Info);
// TODO: int WINAPI AnalyseW(const struct AnalyseInfo *Info)

HANDLE WINAPI OpenFilePluginW(const wchar_t *fileName, const unsigned char *fileHeader, int fileHeaderSize, int /*opMode*/)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    if (!fileName)
    {
        return INVALID_HANDLE_VALUE;
    }

    const size_t fileNameLen = wcslen(fileName);
    if (fileNameLen < 8 || _wcsicmp(fileName + fileNameLen - 7, L".netbox") != 0)
    {
        return INVALID_HANDLE_VALUE;
    }
    if (fileHeaderSize > 4 && strncmp(reinterpret_cast<const char *>(fileHeader), "<?xml", 5) != 0)
    {
        return INVALID_HANDLE_VALUE;
    }
    /*
    PSession session = CSession::Load(fileName);
    if (!session.get())
    {
        return INVALID_HANDLE_VALUE;
    }
    PProtocol proto = session->CreateClient();
    if (!proto.get())
    {
        return reinterpret_cast<HANDLE>(-2);
    }
    CPanel *panelInstance = new CPanel(false);
    if (panelInstance->OpenConnection(proto.get()))
    {
        proto.release();
        m_PanelInstances.push_back(panelInstance);
        return panelInstance;
    }
    */
    return reinterpret_cast<HANDLE>(-2);
}

//---------------------------------------------------------------------------
static int Processes = 0;
//---------------------------------------------------------------------------
void DllProcessAttach(HINSTANCE HInst)
{
    // DEBUG_PRINTF(L"HInst = %u", HInst);
    FarPlugin = CreateFarPlugin(HInst);

    assert(!Processes);
    Processes++;
#ifdef _AFXDLL
    InitExtensionModule(HInst);
#endif
 // DEBUG_PRINTF(L"DllProcessAttach: end");
}

//---------------------------------------------------------------------------
void DllProcessDetach()
{
  // DEBUG_PRINTF(L"DllProcessDetach: start");
  assert(Processes);
  Processes--;
  if (!Processes)
  {
    assert(FarPlugin);
    SAFE_DESTROY(FarPlugin);
#ifdef _AFXDLL
    TermExtensionModule();
#endif
  }
  // DEBUG_PRINTF(L"DllProcessDetach: end");
}

//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE HInst, DWORD Reason, LPVOID /*ptr*/ )
{
  // DEBUG_PRINTF(L"DllEntryPoint: start");
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      DllProcessAttach(HInst);
      break;

    case DLL_PROCESS_DETACH:
      DllProcessDetach();
      break;
  }
  // DEBUG_PRINTF(L"DllEntryPoint: end");
  return true;
}

}
