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

void WINAPI SetStartupInfoW(const PluginStartupInfo *psi)
{
    TFarPluginGuard Guard;
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
}

void WINAPI ExitFARW()
{
    TFarPluginGuard Guard;
    for (std::vector<CPanel *>::const_iterator it = m_PanelInstances.begin(); it != m_PanelInstances.end(); ++it)
    {
        delete *it;
    }
    m_PanelInstances.clear();

    curl_global_cleanup();
    WSACleanup();
}

void WINAPI GetPluginInfoW(PluginInfo *pi)
{
    TFarPluginGuard Guard;
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
}

int WINAPI ConfigureW(int /*itemNumber*/)
{
    TFarPluginGuard Guard;
    // DEBUG_PRINTF(L"ConfigureW: begin");
    m_Settings.Configure();
    // DEBUG_PRINTF(L"ConfigureW: end");
    return FALSE;
}

HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
{
    TFarPluginGuard Guard;
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
}

void WINAPI ClosePluginW(HANDLE plugin)
{
    TFarPluginGuard Guard;
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    assert(panelInstance);
    delete panelInstance;

    std::vector<CPanel *>::iterator it = std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin);
    assert(it != m_PanelInstances.end());
    m_PanelInstances.erase(it);
}

void WINAPI GetOpenPluginInfoW(HANDLE plugin, OpenPluginInfo *pluginInfo)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->GetOpenPluginInfo(pluginInfo);
}

int WINAPI GetFindDataW(HANDLE plugin, PluginPanelItem **panelItem, int *itemsNumber, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"GetFindDataW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetItemList(panelItem, itemsNumber, opMode);
    // DEBUG_PRINTF(L"GetFindDataW: itemsNumber = %u", *itemsNumber);
    return result;
}

void WINAPI FreeFindDataW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->FreeItemList(panelItem, itemsNumber);
}

int WINAPI ProcessHostFileW(HANDLE Plugin,
  struct PluginPanelItem * PanelItem, int ItemsNumber, int OpMode)
{
  // FIXME
  // assert(FarPlugin);
  // TFarPluginGuard Guard;
  // return FarPlugin->ProcessHostFile(Plugin, PanelItem, ItemsNumber, OpMode);
  return 0;
}

int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->ProcessKey(key, controlState);
}

int WINAPI ProcessEventW(HANDLE Plugin, int Event, void * Param)
{
  // FIXME
  // assert(FarPlugin);
  // TFarPluginGuard Guard;
  // return FarPlugin->ProcessEvent(Plugin, Event, Param);
  return 0;
}

int WINAPI SetDirectoryW(HANDLE plugin, const wchar_t *dir, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"SetDirectoryW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->ChangeDirectory(dir, opMode);
    // DEBUG_PRINTF(L"SetDirectoryW: result = %u", result);
    return result;
}

int WINAPI MakeDirectoryW(HANDLE plugin, const wchar_t **name, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->MakeDirectory(name, opMode);
}

int WINAPI DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->DeleteFiles(panelItem, itemsNumber, opMode);
}

int WINAPI GetFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber,
    int move, const wchar_t **destPath, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"GetFiles: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetFiles(panelItem, itemsNumber, destPath, move != 0, opMode);
    // DEBUG_PRINTF(L"GetFilesW: result = %u", result);
    return result;
}

int WINAPI PutFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int move, const wchar_t *srcPath, int opMode)
{
    TFarPluginGuard Guard;
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->PutFiles(srcPath, panelItem, itemsNumber, move != 0, opMode);
}

int WINAPI ProcessEditorEventW(int Event, void * Param)
{
  // FIXME
  // assert(FarPlugin);
  // TFarPluginGuard Guard;
  // return FarPlugin->ProcessEditorEvent(Event, Param);
  return 0;
}

int WINAPI ProcessEditorInputW(const INPUT_RECORD * Rec)
{
  // FIXME
  // assert(FarPlugin);
  // TFarPluginGuard Guard;
  // return FarPlugin->ProcessEditorInput(Rec);
  return 0;
}

HANDLE WINAPI OpenFilePluginW(const wchar_t *fileName, const unsigned char *fileHeader, int fileHeaderSize, int /*opMode*/)
{
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
  DEBUG_PRINTF(L"DllProcessAttach: start");
  FarPlugin = CreateFarPlugin(HInst);

  assert(!Processes);
  Processes++;
  DEBUG_PRINTF(L"DllProcessAttach: end");
}

//---------------------------------------------------------------------------
void DllProcessDetach()
{
  DEBUG_PRINTF(L"DllProcessDetach: start");
  assert(Processes);
  Processes--;
  if (!Processes)
  {
    assert(FarPlugin);
    SAFE_DESTROY(FarPlugin);
  }
  DEBUG_PRINTF(L"DllProcessDetach: end");
}

//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE HInst, DWORD Reason, LPVOID /*ptr*/ )
{
  DEBUG_PRINTF(L"DllEntryPoint: start");
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      DllProcessAttach(HInst);
      break;

    case DLL_PROCESS_DETACH:
      DllProcessDetach();
      break;
  }
  DEBUG_PRINTF(L"DllEntryPoint: end");
  return true;
}

}
