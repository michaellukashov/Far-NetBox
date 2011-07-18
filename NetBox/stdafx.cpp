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
#include "Strings.h"


wstring GetSystemErrorMessage(const DWORD errCode)
{
    assert(errCode);

    wstring errorMsg;

    wchar_t codeNum[16];
    swprintf_s(codeNum, L"[0x%08X]", errCode);
    errorMsg = codeNum;

    wchar_t errInfoBuff[256];
    ZeroMemory(errInfoBuff, sizeof(errInfoBuff));
    if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL))
        if (!FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(L"Wininet.dll"), errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL))
        {
            FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(L"Winhttp.dll"), errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL);
        }
    //Remove '\r\n' from the end
    while (*errInfoBuff && errInfoBuff[wcslen(errInfoBuff) - 1] == L'\n' || errInfoBuff[wcslen(errInfoBuff) - 1] == L'\r')
    {
        errInfoBuff[wcslen(errInfoBuff) - 1] = 0;
    }

    if (*errInfoBuff)
    {
        errorMsg += L": ";
        errorMsg += errInfoBuff;
    }

    return errorMsg;
}


void ParseURL(const wchar_t *url, wstring *scheme, wstring *hostName, unsigned short *port, wstring *path, wstring *query, wstring *userName, wstring *password)
{
    assert(url);

    wstring urlParse(url);

    //Parse scheme name
    const size_t delimScheme = urlParse.find(L"://");
    if (delimScheme != string::npos)
    {
        if (scheme)
        {
            *scheme = urlParse.substr(0, delimScheme);
            transform(scheme->begin(), scheme->end(), scheme->begin(), tolower);
        }
        urlParse.erase(0, delimScheme + sizeof(L"://") / sizeof(wchar_t) - 1);
    }

    //Parse path
    const size_t delimPath = urlParse.find(L'/');
    if (delimPath != string::npos)
    {
        wstring parsePath = urlParse.substr(delimPath);
        urlParse.erase(delimPath);
        //Parse query
        const size_t delimQuery = parsePath.rfind(L'?');
        if (delimQuery != string::npos)
        {
            if (query)
            {
                *query = parsePath.substr(delimQuery);
            }
            parsePath.erase(delimQuery);
        }
        if (path)
        {
            *path = parsePath;
        }
    }
    if (path && path->empty())
    {
        *path = L'/';
    }

    //Parse user name/password
    const size_t delimLogin = urlParse.rfind(L'@');
    if (delimLogin != string::npos)
    {
        wstring parseLogin = urlParse.substr(0, delimLogin);
        const size_t delimPwd = parseLogin.rfind(L':');
        if (delimPwd != string::npos)
        {
            if (password)
            {
                *password = parseLogin.substr(delimPwd + 1);
            }
            parseLogin.erase(delimPwd);
        }
        if (userName)
        {
            *userName = parseLogin;
        }
        urlParse.erase(0, delimLogin + 1);
    }

    //Parse port
    if (port)
    {
        *port = 0;
    }

    const size_t delimPort = urlParse.rfind(L':');
    if (delimPort != string::npos)
    {
        if (port)
        {
            const wstring portNum = urlParse.substr(delimPort + 1);
            *port = static_cast<unsigned short>(_wtoi(portNum.c_str()));
        }
        urlParse.erase(delimPort);
    }

    if (hostName)
    {
        *hostName = urlParse;
    }
}


FILETIME UnixTimeToFileTime(const time_t t)
{
    FILETIME ft;
    const LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
    ft.dwLowDateTime = static_cast<DWORD>(ll);
    ft.dwHighDateTime = ll >> 32;
    return ft;
}

string NumberToText(int number)
{
    char codeText[16];
    _itoa_s(number, codeText, 10);
    return string(codeText);
}

wstring NumberToWString(unsigned long number)
{
    wchar_t toText[16];
    _itow_s(number, toText, 10);
    return wstring(toText);
}

void InitProxySettingsDialog(CFarDialog &dlg, int &topPos,
    int &proxyType,
    wstring &proxyHost,
    unsigned long &proxyPort,
    wstring &proxyLogin,
    wstring &proxyPassword,
    ProxySettingsDialogParams &params)
{
    // Тип прокси
    dlg.CreateText(dlg.GetLeft(), topPos, CFarPlugin::GetString(StringProxySettingsProxyType));
    DEBUG_PRINTF(L"NetBox: 1");
    // FarDialogItem *proxyTypeComboBox;
    int left = dlg.GetLeft() +
        static_cast<int>(wcslen(CFarPlugin::GetString(StringProxySettingsProxyType))) + 1;
    params.idProxyTypeComboBox = dlg.CreateDlgItem(DI_COMBOBOX, left,
        left + 12, topPos, topPos, NULL, DIF_LISTWRAPMODE, &params.proxyTypeComboBox);
    DEBUG_PRINTF(L"NetBox: 1");
    FarList proxyTypeList;
    vector<FarListItem> proxyTypeListItems;
    int proxyTypeCount = 4;
    proxyTypeListItems.resize(proxyTypeCount);
    ZeroMemory(&proxyTypeListItems[0], proxyTypeCount * sizeof(FarListItem));
    DEBUG_PRINTF(L"NetBox: 3");
    for (int i = 0; i < proxyTypeCount; ++i)
    {
        if (proxyType == i)
        {
            proxyTypeListItems[i].Flags = LIF_SELECTED;
        }
        proxyTypeListItems[i].Text = CFarPlugin::GetString(proxyTypeItem1 + i);
    }
    DEBUG_PRINTF(L"NetBox: 4");
    proxyTypeList.Items = &proxyTypeListItems.front();
    proxyTypeList.ItemsNumber = static_cast<int>(proxyTypeListItems.size());
    params.proxyTypeComboBox->ListItems = &proxyTypeList;

    dlg.CreateSeparator(++topPos);
    DEBUG_PRINTF(L"NetBox: 5");
    //
    // Прокси адрес, порт, логин/пароль
    //
    // Адрес прокси сервера
    dlg.CreateText(dlg.GetLeft(), ++topPos, CFarPlugin::GetString(StringProxySettingsProxyHost));
    left = dlg.GetLeft();
    params.idProxyHost = dlg.CreateEdit(left, topPos + 1, 30, proxyHost.c_str());
    // Порт
    wstring proxyPortStr = NumberToWString(proxyPort);
    left = dlg.GetWidth() - 10;
    dlg.CreateText(left, topPos, CFarPlugin::GetString(StringProxySettingsProxyPort));
    FarDialogItem *itemPortEdit;
    params.idProxyPort = dlg.CreateDlgItem(DI_FIXEDIT, left, left + 10,
        topPos + 1, topPos + 1, proxyPortStr.c_str(), DIF_MASKEDIT, &itemPortEdit);
    itemPortEdit->Mask = L"99999999";
    DEBUG_PRINTF(L"NetBox: 6");

    topPos += 2;
    left = dlg.GetLeft();
    dlg.CreateText(left, topPos, CFarPlugin::GetString(StringProxySettingsProxyLogin));
    params.idProxyLogin = dlg.CreateEdit(left, topPos + 1, 20,
        proxyLogin.c_str());
    left = dlg.GetWidth() - 20;
    dlg.CreateText(left, topPos, CFarPlugin::GetString(StringProxySettingsProxyPassword));
    params.idProxyPassword = dlg.CreateDlgItem(DI_PSWEDIT, left, left + 20,
        topPos + 1, topPos + 1, proxyPassword.c_str());
    DEBUG_PRINTF(L"NetBox: 7");
}
