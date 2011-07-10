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
#include "SFTP.h"
#include "FTP.h"
#include "Settings.h"
#include "Logging.h"
#include "Strings.h"
#include "resource.h"

#define MIN_FAR_VERMAJOR 2
#define MIN_FAR_VERMINOR 0
#define MIN_FAR_BUILD    0

vector<CPanel *> _PanelInstances;   ///< Array of active panels instances

//FAR API Callback
int WINAPI _export GetMinFarVersionW()
{
    return MAKEFARVERSION(MIN_FAR_VERMAJOR, MIN_FAR_VERMINOR, MIN_FAR_BUILD);
}


//FAR API Callback
void WINAPI _export SetStartupInfoW(const PluginStartupInfo *psi)
{
    CFarPlugin::Initialize(psi);

    _Settings.Load();
    CLogger::Initialize(_Settings.EnableLogging(), _Settings.LoggingLevel(),
        _Settings.LogToFile(), _Settings.LogFileName());
    wstring ver = PLUGIN_VERSION_WTXT;
    Log1(L"NetBox plugin version %s started.", ver.c_str());

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    curl_global_init(CURL_GLOBAL_ALL);

    //Register protocol clients
    CSession::RegisterProtocolClient(0, L"FTP", CSession::SessionCreator<CSessionFTP>, L"ftp");
    CSession::RegisterProtocolClient(1, L"SFTP", CSession::SessionCreator<CSessionSFTP>, L"sftp");
    CSession::RegisterProtocolClient(2, L"WebDAV", CSession::SessionCreator<CSessionWebDAV>, L"http", L"https");
}


//FAR API Callback
void WINAPI _export GetPluginInfoW(PluginInfo *pi)
{
    pi->StructSize = sizeof(PluginInfo);
    pi->Flags = PF_FULLCMDLINE;
    if (!_Settings.AddToPanelMenu())
    {
        pi->Flags |= PF_DISABLEPANELS;
    }

    static const wchar_t *menuStrings[1] = { CFarPlugin::GetString(StringTitle) };

    pi->PluginConfigStrings = menuStrings;
    pi->PluginConfigStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);

    if (_Settings.AddToPanelMenu())
    {
        pi->PluginMenuStrings = menuStrings;
        pi->PluginMenuStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);
    }
    if (_Settings.AddToDiskMenu())
    {
        pi->DiskMenuStrings = menuStrings;
        pi->DiskMenuStringsNumber = sizeof(menuStrings) / sizeof(menuStrings[0]);
    }

    static wstring cmdPreffix;
    cmdPreffix = _Settings.CmdPrefix();
    if (_Settings.AltPrefix())
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


//FAR API Callback
void WINAPI _export ExitFARW()
{
    for (vector<CPanel *>::const_iterator it = _PanelInstances.begin(); it != _PanelInstances.end(); ++it)
    {
        delete *it;
    }
    _PanelInstances.clear();

    curl_global_cleanup();
    WSACleanup();
}


//FAR API Callback
void WINAPI _export ClosePluginW(HANDLE plugin)
{
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    assert(panelInstance);
    delete panelInstance;

    vector<CPanel *>::iterator it = find(_PanelInstances.begin(), _PanelInstances.end(), plugin);
    assert(it != _PanelInstances.end());
    _PanelInstances.erase(it);
}


//FAR API Callback
HANDLE WINAPI _export OpenPluginW(int openFrom, INT_PTR item)
{
    //Export sessions from registry to local folder
    // !!!! this functionality will be removed in next release !!!!
    CSession::ExportFromRegistry();

    if (openFrom != OPEN_COMMANDLINE)
    {
        CPanel *panelInstance = new CPanel(true);
        if (panelInstance->OpenConnection(new CSessionManager))
        {
            _PanelInstances.push_back(panelInstance);
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
        _PanelInstances.push_back(panelInstance);
        return panelInstance;
    }
    delete panelInstance;
    return INVALID_HANDLE_VALUE;
}


//FAR API Callback
HANDLE WINAPI _export OpenFilePluginW(const wchar_t *fileName, const unsigned char *fileHeader, int fileHeaderSize, int /*opMode*/)
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
        _PanelInstances.push_back(panelInstance);
        return panelInstance;
    }
    return reinterpret_cast<HANDLE>(-2);
}


//FAR API Callback
void WINAPI _export GetOpenPluginInfoW(HANDLE plugin, OpenPluginInfo *pluginInfo)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->GetOpenPluginInfo(pluginInfo);
}


//FAR API Callback
int WINAPI _export SetDirectoryW(HANDLE plugin, const wchar_t *dir, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->ChangeDirectory(dir, opMode);
}


//FAR API Callback
int WINAPI _export GetFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int move, const wchar_t **destPath, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->GetFiles(panelItem, itemsNumber, destPath, move != 0, opMode);
}


//FAR API Callback
int WINAPI _export GetFindDataW(HANDLE plugin, PluginPanelItem **panelItem, int *itemsNumber, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->GetItemList(panelItem, itemsNumber, opMode);
}


//FAR API Callback
void WINAPI _export FreeFindDataW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    panelInstance->FreeItemList(panelItem, itemsNumber);
}


//FAR API Callback
int WINAPI _export MakeDirectoryW(HANDLE plugin, const wchar_t **name, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->MakeDirectory(name, opMode);
}


//FAR API Callback
int WINAPI _export DeleteFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->DeleteFiles(panelItem, itemsNumber, opMode);
}


//FAR API Callback
int WINAPI _export PutFilesW(HANDLE plugin, PluginPanelItem *panelItem, int itemsNumber, int move, const wchar_t *srcPath, int opMode)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->PutFiles(srcPath, panelItem, itemsNumber, move != 0, opMode);
}


//FAR API Callback
int WINAPI _export ProcessKeyW(HANDLE plugin, int key, unsigned int controlState)
{
    assert(find(_PanelInstances.begin(), _PanelInstances.end(), plugin) != _PanelInstances.end());
    CPanel *panelInstance = static_cast<CPanel *>(plugin);
    return panelInstance->ProcessKey(key, controlState);
}


//FAR API Callback
int WINAPI _export ConfigureW(int /*itemNumber*/)
{
    _Settings.Configure();
    return FALSE;
}
