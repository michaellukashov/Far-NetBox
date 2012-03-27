#include "FarTexts.h"

#include "afxdll.h"

#include "stdafx.h"
#include "FarUtil.h"
#include "EasyURL.h"
#include "resource.h"
#include "Common.h"
#include "version.h"

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
        if (Info->StructSize < sizeof(GlobalInfo))
            return;
        Info->StructSize = sizeof(*Info);
        Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, FARMANAGERVERSION_STAGE);
        Info->Version = MAKEFARVERSION(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_PATCH, PLUGIN_VERSION_BUILD, VS_RELEASE);
        Info->Guid = MainGuid;
        Info->Title = L"NetBox";
        Info->Description = PLUGIN_DESCR;
        Info->Author = PLUGIN_AUTHOR;
    }

    void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
    {
        if (Info->StructSize < sizeof(PluginStartupInfo))
            return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->SetStartupInfo(Info);
    }

    void WINAPI ExitFARW(const struct ExitInfo *Info)
    {
        if (Info->StructSize < sizeof(ExitInfo))
            return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->ExitFAR();
    }

    void WINAPI GetPluginInfoW(PluginInfo *Info)
    {
        if (Info->StructSize < sizeof(PluginInfo))
            return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->GetPluginInfo(Info);
    }

    int WINAPI ConfigureW(const struct ConfigureInfo *Info)
    {
        if (Info->StructSize < sizeof(ConfigureInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->Configure(Info);
    }

    void WINAPI ClosePanelW(const struct ClosePanelInfo *Info)
    {
        if (Info->StructSize < sizeof(ClosePanelInfo))
           return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->ClosePanel(Info->hPanel);
    }

    void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info)
    {
        if (Info->StructSize < sizeof(OpenPanelInfo))
            return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->GetOpenPanelInfo(Info);
    }

    int WINAPI GetFindDataW(struct GetFindDataInfo *Info)
    {
        if (Info->StructSize < sizeof(GetFindDataInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->GetFindData(Info);
    }

    void WINAPI FreeFindDataW(const struct FreeFindDataInfo *Info)
    {
        if (Info->StructSize < sizeof(FreeFindDataInfo))
            return;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        FarPlugin->FreeFindData(Info);
    }

    int WINAPI ProcessHostFileW(const struct ProcessHostFileInfo *Info)
    {
        if (Info->StructSize < sizeof(ProcessHostFileInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->ProcessHostFile(Info);
    }

    int WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info)
    {
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->ProcessPanelInput(Info);
    }

    int WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo *Info)
    {
        if (Info->StructSize < sizeof(ProcessPanelEventInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->ProcessPanelEvent(Info);
    }

    int WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info)
    {
        DEBUG_PRINTF(L"begin, dir = %s", Info->Dir);
        if (Info->StructSize < sizeof(SetDirectoryInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        int result = FarPlugin->SetDirectory(Info);
        DEBUG_PRINTF(L"end, result = %d", result);
        return result;
    }

    int WINAPI MakeDirectoryW(struct MakeDirectoryInfo *Info)
    {
        DEBUG_PRINTF(L"begin, name = %s", Info->Name);
        if (Info->StructSize < sizeof(MakeDirectoryInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        int result = FarPlugin->MakeDirectory(Info);
        DEBUG_PRINTF(L"end, result = %d", result);
        return result;
    }

    int WINAPI DeleteFilesW(const struct DeleteFilesInfo *Info)
    {
        if (Info->StructSize < sizeof(DeleteFilesInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->DeleteFiles(Info);
    }

    int WINAPI GetFilesW(struct GetFilesInfo *Info)
    {
        if (Info->StructSize < sizeof(GetFilesInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->GetFiles(Info);
    }

    int WINAPI PutFilesW(const struct PutFilesInfo *Info)
    {
        DEBUG_PRINTF(L"begin, srcPath = %s", Info->SrcPath);
        if (Info->StructSize < sizeof(PutFilesInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        int result = FarPlugin->PutFiles(Info);
        DEBUG_PRINTF(L"end, result = %d", result);
        return result;
    }

    int WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info)
    {
        if (Info->StructSize < sizeof(ProcessEditorEventInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->ProcessEditorEvent(Info);
    }

    int WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info)
    {
        if (Info->StructSize < sizeof(ProcessEditorInputInfo))
            return FALSE;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        return FarPlugin->ProcessEditorInput(Info);
    }

    HANDLE WINAPI AnalyseW(const struct AnalyseInfo *Info)
    {
        if (Info->StructSize < sizeof(AnalyseInfo))
            return NULL;
        assert(FarPlugin);
        TFarPluginGuard Guard;
        if (!Info->FileName)
        {
            return NULL;
        }

        const size_t fileNameLen = wcslen(Info->FileName);
        if (fileNameLen < 8 || _wcsicmp(Info->FileName + fileNameLen - 7, L".netbox") != 0)
        {
            return NULL;
        }
        if (Info->BufferSize > 4 && strncmp(reinterpret_cast<const char *>(Info->Buffer), "<?xml", 5) != 0)
        {
            return NULL;
        }
        return HANDLE(1);
    }

    HANDLE WINAPI OpenW(const struct OpenInfo *Info)
    {
        if (Info->StructSize < sizeof(OpenInfo))
            return NULL;
        assert(FarPlugin);
        DEBUG_PRINTF(L"NetBox: OpenW: begin: OpenFrom = %d", Info->OpenFrom);
        TFarPluginGuard Guard;
        HANDLE handle = static_cast<HANDLE>(FarPlugin->OpenPlugin(Info));
        DEBUG_PRINTF(L"NetBox: end, handle = %u", handle);
        return handle;
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
        InitExtensionModule(HInst);
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        curl_global_init(CURL_GLOBAL_ALL);
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
            TermExtensionModule();
            curl_global_cleanup();
            WSACleanup();
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
