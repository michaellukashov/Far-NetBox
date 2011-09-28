/**************************************************************************
 *  NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)         *
 *  Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>             *
 *  Copyright (C) 2011 by Michael Lukashov <michael.lukashov@gmail.com>   *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#include "stdafx.h"
#include "SessionManager.h"
#include "Panel.h"
#include "WebDAV.h"
#include "FTP.h"
#include "SFTP.h"
#include "FTPS.h"
#include "SCP.h"
#include "Settings.h"
#include "Logging.h"
#include "Strings.h"
#include "resource.h"
#include "Common.h"

std::vector<CPanel *> m_PanelInstances;   ///< Array of active panels instances

//---------------------------------------------------------------------------
TCustomFarPlugin *CreateFarPlugin(HINSTANCE HInst);

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

int WINAPI GetMinFarVersionW()
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->GetMinFarVersion();
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *psi)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    CFarPlugin::Initialize(psi);

    m_Settings.Load();
    CLogger::Initialize(m_Settings.EnableLogging(), m_Settings.LoggingLevel(),
        m_Settings.LogToFile(), m_Settings.LogFileName());
    std::wstring ver = PLUGIN_VERSION_WTXT;
    Log1(L"NetBox plugin version %s started.", ver.c_str());

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    curl_global_init(CURL_GLOBAL_ALL);

    //Register protocol clients
    CSession::RegisterProtocolClient(0, L"FTP", CSession::SessionCreator<CSessionFTP>, L"ftp");
    CSession::RegisterProtocolClient(1, L"SFTP", CSession::SessionCreator<CSessionSFTP>, L"sftp");
    CSession::RegisterProtocolClient(2, L"WebDAV", CSession::SessionCreator<CSessionWebDAV>, L"http", L"https");
    CSession::RegisterProtocolClient(3, L"FTPS", CSession::SessionCreator<CSessionFTPS>, L"ftps");
    CSession::RegisterProtocolClient(4, L"SCP", CSession::SessionCreator<CSessionSCP>, L"scp");
#endif
    FarPlugin->SetStartupInfo(psi);
}

void WINAPI ExitFARW()
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    for (std::vector<CPanel *>::const_iterator it = m_PanelInstances.begin(); it != m_PanelInstances.end(); ++it)
    {
        delete *it;
    }
    m_PanelInstances.clear();

    curl_global_cleanup();
    WSACleanup();
#endif
    FarPlugin->ExitFAR();
}

void WINAPI GetPluginInfoW(PluginInfo *pi)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    pi->StructSize = sizeof(PluginInfo);
    pi->Flags = PF_FULLCMDLINE;
    if (!m_Settings.AddToPanelMenu())
    {
        pi->Flags |= PF_DISABLEPANELS;
    }

    static const wchar_t *menuStrings[1] = { CFarPlugin::GetString(StringTitle) };

    pi->PluginConfigStrings = menuStrings;
    pi->PluginConfigStringsNumber = LENOF(menuStrings);

    if (m_Settings.AddToPanelMenu())
    {
        pi->PluginMenuStrings = menuStrings;
        pi->PluginMenuStringsNumber = LENOF(menuStrings);
    }
    if (m_Settings.AddToDiskMenu())
    {
        pi->DiskMenuStrings = menuStrings;
        pi->DiskMenuStringsNumber = LENOF(menuStrings);
    }

    static std::wstring cmdPreffix;
    cmdPreffix = m_Settings.CmdPrefix();
    if (m_Settings.AltPrefix())
    {
        if (!cmdPreffix.empty())
        {
            cmdPreffix += L':';
        }
        cmdPreffix += CSession::GetSupportedPrefixes();
    }
    if (!cmdPreffix.empty())
    {
        pi->CommandPrefix = cmdPreffix.c_str();
    }
#endif
    FarPlugin->GetPluginInfo(pi);
}

int WINAPI ConfigureW(int item)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    // DEBUG_PRINTF(L"ConfigureW: begin");
    m_Settings.Configure();
    // DEBUG_PRINTF(L"ConfigureW: end");
#endif
    return FarPlugin->Configure(item);
}

HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    //Export sessions from registry to local folder
    // !!!! this functionality will be removed in next release !!!!
    CSession::ExportFromRegistry();

    if (openFrom != OPEN_COMMANDLINE)
    {
        CPanel *panelInstance = new CPanel(true);
        if (panelInstance->OpenConnection(new CSessionManager))
        {
            m_PanelInstances.push_back(panelInstance);
            return panelInstance;
        }
        delete panelInstance;
        return INVALID_HANDLE_VALUE;
    }

    //Command line processing
    wchar_t *cmdString = reinterpret_cast<wchar_t *>(item);
    CFarPlugin::GetPSI()->FSF->Trim(cmdString);
    PSession session = CSession::Create(cmdString);
    if (!session.get())
    {
        return INVALID_HANDLE_VALUE;
    }
    PProtocol proto = session->CreateClient();
    if (!proto.get())
    {
        CFarPlugin::MessageBox(CFarPlugin::GetString(StringTitle), CFarPlugin::GetString(StringEdErrURLInvalid), FMSG_MB_OK | FMSG_WARNING);
        return INVALID_HANDLE_VALUE;
    }
    CPanel *panelInstance = new CPanel(false);
    if (panelInstance->OpenConnection(proto.get()))
    {
        proto.release();
        m_PanelInstances.push_back(panelInstance);
        return panelInstance;
    }
    delete panelInstance;
    return INVALID_HANDLE_VALUE;
#endif
  return FarPlugin->OpenPlugin(openFrom, item);
}

void WINAPI ClosePluginW(HANDLE plugin)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    assert(panelInstance);
    delete panelInstance;

    std::vector<CPanel *>::iterator it = std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin);
    assert(it != m_PanelInstances.end());
    m_PanelInstances.erase(it);
#endif
    FarPlugin->ClosePlugin(plugin);
}

void WINAPI GetOpenPluginInfoW(HANDLE plugin, OpenPluginInfo *pluginInfo)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->GetOpenPluginInfo(pluginInfo);
#endif
    FarPlugin->GetOpenPluginInfo(plugin, pluginInfo);
}

int WINAPI GetFindDataW(HANDLE plugin, PluginPanelItem **panelItem, int *itemsNumber, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"GetFindDataW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetItemList(panelItem, itemsNumber, opMode);
    // DEBUG_PRINTF(L"GetFindDataW: itemsNumber = %u", *itemsNumber);
    return result;
#endif
    return FarPlugin->GetFindData(plugin, panelItem, itemsNumber, opMode);
}

void WINAPI FreeFindDataW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->FreeItemList(panelItem, itemsNumber);
    FarPlugin->FreeFindData(plugin, panelItem, itemsNumber);
#endif
}

int WINAPI ProcessHostFileW(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  assert(FarPlugin);
  TFarPluginGuard Guard;
  return FarPlugin->ProcessHostFile(Plugin, PanelItem, ItemsNumber, OpMode);
}

int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->ProcessKey(key, controlState);
#endif
  return FarPlugin->ProcessKey(plugin, key, controlState);
}

int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEvent(Plugin, Event, Param);
}

int WINAPI SetDirectoryW(HANDLE plugin, const wchar_t *dir, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"SetDirectoryW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->ChangeDirectory(dir, opMode);
    // DEBUG_PRINTF(L"SetDirectoryW: result = %u", result);
    return result;
#endif
    return FarPlugin->SetDirectory(plugin, dir, opMode);
}

int WINAPI MakeDirectoryW(HANDLE plugin, const wchar_t **name, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->MakeDirectory(name, opMode);
#endif
    return FarPlugin->MakeDirectory(plugin, *name, opMode);
}

int WINAPI DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->DeleteFiles(panelItem, itemsNumber, opMode);
#endif
    return FarPlugin->DeleteFiles(plugin, panelItem, itemsNumber, opMode);
}

int WINAPI GetFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber,
    int move, const wchar_t **destPath, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"GetFiles: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetFiles(panelItem, itemsNumber, destPath, move != 0, opMode);
    // DEBUG_PRINTF(L"GetFilesW: result = %u", result);
    return result;
#endif
    return FarPlugin->GetFiles(plugin, panelItem, itemsNumber,
        move, *destPath, opMode);
}

int WINAPI PutFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int move, const wchar_t *srcPath, int opMode)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
#ifndef WINSCP
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->PutFiles(srcPath, panelItem, itemsNumber, move != 0, opMode);
#endif
    return FarPlugin->PutFiles(plugin, panelItem, itemsNumber,
        move, opMode);
}

int WINAPI ProcessEditorEventW(int Event, void * Param)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEditorEvent(Event, Param);
}

int WINAPI ProcessEditorInputW(const INPUT_RECORD *Rec)
{
    assert(FarPlugin);
    TFarPluginGuard Guard;
    return FarPlugin->ProcessEditorInput(Rec);
}

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
