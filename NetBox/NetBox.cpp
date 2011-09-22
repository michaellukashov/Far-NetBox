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

#define MIN_FAR_VERMAJOR 2
#define MIN_FAR_VERMINOR 0
#define MIN_FAR_BUILD    0

std::vector<CPanel *> m_PanelInstances;   ///< Array of active panels instances

extern "C"
{

int WINAPI GetMinFarVersionW()
{
    return MAKEFARVERSION(MIN_FAR_VERMAJOR, MIN_FAR_VERMINOR, MIN_FAR_BUILD);
}

void WINAPI SetStartupInfoW(const PluginStartupInfo *psi)
{
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

void WINAPI GetPluginInfoW(PluginInfo *pi)
{
    pi->StructSize = sizeof(PluginInfo);
    pi->Flags = PF_FULLCMDLINE;
    if (!m_Settings.AddToPanelMenu())
    {
        pi->Flags |= PF_DISABLEPANELS;
    }

    static const wchar_t *menuStrings[1] = { CFarPlugin::GetString(StringTitle) };

    pi->PluginConfigStrings = menuStrings;
    pi->PluginConfigStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);

    if (m_Settings.AddToPanelMenu())
    {
        pi->PluginMenuStrings = menuStrings;
        pi->PluginMenuStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);
    }
    if (m_Settings.AddToDiskMenu())
    {
        pi->DiskMenuStrings = menuStrings;
        pi->DiskMenuStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);
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

void WINAPI ExitFARW()
{
    for (std::vector<CPanel *>::const_iterator it = m_PanelInstances.begin(); it != m_PanelInstances.end(); ++it)
    {
        delete *it;
    }
    m_PanelInstances.clear();

    curl_global_cleanup();
    WSACleanup();
}

void WINAPI ClosePluginW(HANDLE plugin)
{
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    assert(panelInstance);
    delete panelInstance;

    std::vector<CPanel *>::iterator it = std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin);
    assert(it != m_PanelInstances.end());
    m_PanelInstances.erase(it);
}

HANDLE WINAPI OpenPluginW(int openFrom, INT_PTR item)
{
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

HANDLE WINAPI OpenFilePluginW(const wchar_t *fileName, const unsigned char *fileHeader, int fileHeaderSize, int /*opMode*/)
{
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

void WINAPI GetOpenPluginInfoW(HANDLE plugin, OpenPluginInfo *pluginInfo)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->GetOpenPluginInfo(pluginInfo);
}

int WINAPI SetDirectoryW(HANDLE plugin, const wchar_t *dir, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"NetBox: SetDirectoryW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->ChangeDirectory(dir, opMode);
    // DEBUG_PRINTF(L"NetBox: SetDirectoryW: result = %u", result);
    return result;
}

int WINAPI GetFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber,
    int move, const wchar_t **destPath, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"NetBox: GetFiles: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetFiles(panelItem, itemsNumber, destPath, move != 0, opMode);
    // DEBUG_PRINTF(L"NetBox: GetFilesW: result = %u", result);
    return result;
}

int WINAPI GetFindDataW(HANDLE plugin, PluginPanelItem **panelItem, int *itemsNumber, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    // DEBUG_PRINTF(L"NetBox: GetFindDataW: begin");
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    int result = panelInstance->GetItemList(panelItem, itemsNumber, opMode);
    // DEBUG_PRINTF(L"NetBox: GetFindDataW: itemsNumber = %u", *itemsNumber);
    return result;
}

void WINAPI FreeFindDataW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->FreeItemList(panelItem, itemsNumber);
}

int WINAPI MakeDirectoryW(HANDLE plugin, const wchar_t **name, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->MakeDirectory(name, opMode);
}

int WINAPI DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->DeleteFiles(panelItem, itemsNumber, opMode);
}

int WINAPI PutFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int move, const wchar_t *srcPath, int opMode)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->PutFiles(srcPath, panelItem, itemsNumber, move != 0, opMode);
}

int WINAPI ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
{
    assert(std::find(m_PanelInstances.begin(), m_PanelInstances.end(), plugin) != m_PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->ProcessKey(key, controlState);
}

int WINAPI ConfigureW(int /*itemNumber*/)
{
    // DEBUG_PRINTF(L"NetBox: ConfigureW: begin");
    m_Settings.Configure();
    // DEBUG_PRINTF(L"NetBox: ConfigureW: end");
    return FALSE;
}

//---------------------------------------------------------------------------
TCustomFarPlugin * CreateFarPlugin(HINSTANCE HInst);
//---------------------------------------------------------------------------
static int Processes = 0;
//---------------------------------------------------------------------------
void DllProcessAttach(HINSTANCE HInst)
{
  FarPlugin = CreateFarPlugin(HInst);

  assert(!Processes);
  Processes++;
}

//---------------------------------------------------------------------------
void DllProcessDetach()
{
  assert(Processes);
  Processes--;
  if (!Processes)
  {
    assert(FarPlugin);
    SAFE_DESTROY(FarPlugin);
  }
}

//---------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE HInst, DWORD Reason, LPVOID /*ptr*/ )
{
  switch (Reason)
  {
    case DLL_PROCESS_ATTACH:
      DllProcessAttach(HInst);
      break;

    case DLL_PROCESS_DETACH:
      DllProcessDetach();
      break;
  }
  return true;
}

}
