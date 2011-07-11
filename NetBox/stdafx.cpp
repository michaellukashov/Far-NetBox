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
