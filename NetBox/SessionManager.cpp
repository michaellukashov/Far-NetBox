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
#include "Settings.h"
#include "Strings.h"


void CSessionManager::GetOpenPluginInfo(OpenPluginInfo *pluginInfo)
{
    //Configure one panel view for all modes (show links name only)
    static PanelMode panelModes[10];
    static KeyBarTitles keyBarTitles;
    static bool init = false;
    if (!init)
    {
        ZeroMemory(&panelModes, sizeof(panelModes));
        for (int i = 0; i < sizeof(panelModes) / sizeof(panelModes[0]); ++i)
        {
            panelModes[i].ColumnTypes = L"N";
            panelModes[i].ColumnWidths = L"0";
        }
        ZeroMemory(&keyBarTitles, sizeof(keyBarTitles));
        static wchar_t *emptyTitle = L"";
        keyBarTitles.Titles[2] = keyBarTitles.Titles[4] = keyBarTitles.Titles[5] = emptyTitle;
        keyBarTitles.AltTitles[2] = keyBarTitles.AltTitles[3] = keyBarTitles.AltTitles[4] = keyBarTitles.AltTitles[5] = keyBarTitles.AltTitles[6] = emptyTitle;
        keyBarTitles.ShiftTitles[0] = keyBarTitles.ShiftTitles[1] = keyBarTitles.ShiftTitles[2] = keyBarTitles.ShiftTitles[4] = keyBarTitles.ShiftTitles[5] = keyBarTitles.ShiftTitles[6] = keyBarTitles.ShiftTitles[7] = emptyTitle;
        keyBarTitles.CtrlTitles[2] = keyBarTitles.CtrlTitles[3] = keyBarTitles.CtrlTitles[4] = keyBarTitles.CtrlTitles[5] = keyBarTitles.CtrlTitles[6] = keyBarTitles.CtrlTitles[7] = keyBarTitles.CtrlTitles[8] = keyBarTitles.CtrlTitles[9] = keyBarTitles.CtrlTitles[10] = keyBarTitles.CtrlTitles[11] = emptyTitle;
        keyBarTitles.CtrlAltTitles[2] = keyBarTitles.CtrlAltTitles[3] = emptyTitle;

        init = true;
    }
    pluginInfo->PanelModesArray = panelModes;
    pluginInfo->PanelModesNumber = sizeof(panelModes) / sizeof(panelModes[0]);
    pluginInfo->KeyBar = &keyBarTitles;
}


BOOL CSessionManager::ProcessKey(HANDLE plugin, const int key, const unsigned int controlState)
{
    if (key == VK_F3 || key == VK_F5 || key == VK_F6)   //Unsupported operations
    {
        return 1;
    }
    if (key == 'A' && controlState == PKF_CONTROL)      //Don't show attributes
    {
        return 1;
    }
    if (key == VK_F4 && (controlState & PKF_SHIFT) != 0)
    {
        PSession session = CSession::Create();
        if (session.get())
        {
            const wstring savePath = ConvertPath(_Settings.GetSessionPath().c_str(), _CurrentDirectory.c_str());
            session->Save(savePath.c_str(), false);
            CFarPlugin::GetPSI()->Control(plugin, FCTL_UPDATEPANEL, 0, NULL);
            CFarPlugin::GetPSI()->Control(plugin, FCTL_REDRAWPANEL, 0, NULL);
        }
        return 1;
    }
    if (key == VK_F4 && controlState == 0)
    {
        //Determine currently selected session name
        const size_t ppiBufferLength = CFarPlugin::GetPSI()->Control(plugin, FCTL_GETCURRENTPANELITEM, 0, static_cast<LONG_PTR>(NULL));
        if (ppiBufferLength == 0)
        {
            return 1;
        }
        vector<unsigned char> ppiBuffer(ppiBufferLength);
        PluginPanelItem *ppi = reinterpret_cast<PluginPanelItem *>(&ppiBuffer.front());
        if (!CFarPlugin::GetPSI()->Control(plugin, FCTL_GETCURRENTPANELITEM, 0, reinterpret_cast<LONG_PTR>(ppi)))
        {
            return 1;
        }
        if (ppi->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            return 1;
        }

        const wstring sessionPath = ConvertPath(_Settings.GetSessionPath().c_str(), _CurrentDirectory.c_str());
        wstring sessionFileName = ConvertPath(sessionPath.c_str(), ppi->FindData.lpwszFileName);
        sessionFileName += L".netbox";

        PSession session = CSession::Load(sessionFileName.c_str());
        if (session.get() && session->Edit())
        {
            const bool nameChanged = (_wcsicmp(ppi->FindData.lpwszFileName, session->GetSessionName()) != 0);
            if (session->Save(sessionPath.c_str(), !nameChanged) && nameChanged)
            {
                DeleteFile(sessionFileName.c_str());
            }
            CFarPlugin::GetPSI()->Control(plugin, FCTL_UPDATEPANEL, 0, NULL);
            CFarPlugin::GetPSI()->Control(plugin, FCTL_REDRAWPANEL, 0, NULL);
        }
        return 1;
    }
    return 0;
}


bool CSessionManager::CheckExisting(const wchar_t *path, const ItemType /*type*/, bool &isExist, wstring & /*errorInfo*/)
{
    isExist = (GetFileAttributes(ConvertPath(_Settings.GetSessionPath().c_str(), path).c_str()) != INVALID_FILE_ATTRIBUTES);
    return true;
}


bool CSessionManager::ChangeDirectory(const wchar_t *name, wstring &errorInfo)
{
    assert(name && *name);

    const bool moveUp = (wcscmp(L"..", name) == 0);
    const bool topDirectory = _CurrentDirectory.empty();

    assert(!moveUp || !topDirectory);   //Must be handled in CPanel (exit from session)

    wstring newPath = _CurrentDirectory;
    if (!moveUp)
    {
        newPath = ConvertPath(newPath.c_str(), name);
    }
    else
    {
        const size_t lastSlash = newPath.rfind(L'\\');
        if (lastSlash != string::npos)
        {
            newPath.erase(lastSlash);
        }
        else
        {
            newPath.clear();
        }
    }

    //Check path existing
    bool dirExist = false;
    DEBUG_PRINTF(L"NetBox: newPath = %s", newPath.c_str());
    if (!CheckExisting(newPath.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        return false;
    }
    _CurrentDirectory = newPath;
    return true;

}


bool CSessionManager::MakeDirectory(const wchar_t *path, wstring &errorInfo)
{
    const wstring dirPath = ConvertPath(_Settings.GetSessionPath().c_str(), path);
    if (!CreateDirectory(dirPath.c_str(), NULL))
    {
        const int errCode = GetLastError();
        if (errCode != ERROR_ALREADY_EXISTS)
        {
            errorInfo = GetSystemErrorMessage(errCode);
            return false;
        }
    }
    return true;
}


bool CSessionManager::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    wstring findMask = ConvertPath(_Settings.GetSessionPath().c_str(), _CurrentDirectory.c_str());
    if (findMask[findMask.length() - 1] != L'\\')
    {
        findMask += L'\\';
    }
    findMask += L'*';

    WIN32_FIND_DATA findFileData;
    HANDLE findHandle = FindFirstFile(findMask.c_str(), &findFileData);
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        errorInfo = GetSystemErrorMessage(GetLastError());
        _CurrentDirectory = ConvertPath(_Settings.GetSessionPath().c_str(), _CurrentDirectory.c_str()); //To show full error message
        return false;
    }

    //! Session item description
    struct SessionItem
    {
        SessionItem() : Attributes(0) {}
        wstring             Name;
        DWORD               Attributes;
    };
    vector<SessionItem> sessionItems;

    DWORD retCode = ERROR_SUCCESS;
    do
    {
        if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
        {
            continue;
        }

        SessionItem item;
        item.Name = findFileData.cFileName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            item.Attributes = FILE_ATTRIBUTE_DIRECTORY;
            sessionItems.push_back(item);
        }
        else
        {
            const size_t fileExtPos = item.Name.rfind(L'.');
            if (fileExtPos == string::npos)
            {
                continue;
            }
            static const wchar_t *filePostfix = L".netbox";
            if (_wcsicmp(item.Name.c_str() + fileExtPos, filePostfix) != 0)
            {
                continue;
            }
            item.Name.erase(fileExtPos);
            sessionItems.push_back(item);
        }
    }
    while (retCode == ERROR_SUCCESS && FindNextFile(findHandle, &findFileData));

    FindClose(findHandle);

    *itemsNum = static_cast<int>(sessionItems.size());
    if (*itemsNum)
    {
        *items = new PluginPanelItem[*itemsNum];
        ZeroMemory(*items, sizeof(PluginPanelItem) * (*itemsNum));
        for (int i = 0; i < *itemsNum; ++i)
        {
            PluginPanelItem &farItem = (*items)[i];
            const size_t nameSize = sessionItems[i].Name.length() + 1;
            wchar_t *name = new wchar_t[nameSize];
            wcscpy_s(name, nameSize, sessionItems[i].Name.c_str());
            farItem.FindData.lpwszFileName = name;
            farItem.FindData.dwFileAttributes = sessionItems[i].Attributes;
        }
    }

    return true;
}


void CSessionManager::FreeList(PluginPanelItem *items, int itemsNum)
{
    if (itemsNum)
    {
        assert(items);
        for (int i = 0; i < itemsNum; ++i)
        {
            delete[] items[i].FindData.lpwszFileName;
        }
        delete[] items;
    }
}


bool CSessionManager::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, wstring &errorInfo)
{
    PSession session = CSession::ImportFromFTP(localPath);
    if (!session.get())
    {
        errorInfo = L"Unsupported format";
        return false;
    }
    wstring savePath = ConvertPath(_Settings.GetSessionPath().c_str(), remotePath);
    if (savePath.rfind(L'\\') != string::npos)
    {
        savePath.erase(savePath.rfind(L'\\'));
    }
    return session->Save(savePath.c_str(), false);
}


bool CSessionManager::Delete(const wchar_t *path, const ItemType type, wstring &errorInfo)
{
    wstring delPath = ConvertPath(_Settings.GetSessionPath().c_str(), path);
    if (type == ItemFile)
    {
        delPath += L".netbox";
    }
    const BOOL deleted = (type == ItemDirectory ? RemoveDirectory(delPath.c_str()) : DeleteFile(delPath.c_str()));
    if (!deleted)
    {
        const int errCode = GetLastError();
        if (errCode != ERROR_FILE_NOT_FOUND)
        {
            errorInfo = GetSystemErrorMessage(errCode);
            return false;
        }
    }

    return true;
}


PSession CSessionManager::LoadSession(const wchar_t *name)
{
    assert(name && *name);

    wstring sessionFileName = ConvertPath(_Settings.GetSessionPath().c_str(), _CurrentDirectory.c_str());
    sessionFileName = ConvertPath(sessionFileName.c_str(), name);
    sessionFileName += L".netbox";

    return CSession::Load(sessionFileName.c_str());
}


wstring CSessionManager::ConvertPath(const wchar_t *pathBase, const wchar_t *sub /*= NULL*/) const
{
    assert(pathBase);

    wstring ret(pathBase);
    if (sub && *sub)
    {
        if (!ret.empty() && ret[ret.length() - 1] != L'\\' && ret[ret.length() - 1] != L'/')
        {
            ret += L'\\';
        }
        ret += (sub[0] == L'\\' || sub[0] == L'/' ? ++sub : sub);
    }

    size_t pos = string::npos;
    while ((pos = ret.find(L'/')) != string::npos)
    {
        ret[pos] = L'\\';
    }

    return ret;
}
