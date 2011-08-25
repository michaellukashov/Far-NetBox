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

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <time.h>

#include "..\Common\FarPlugin.h"
#include "..\Common\FarSettings.h"
#include "..\Common\FarDialog.h"
#include "..\Common\FarUtil.h"

/**
 * Get system error message by error code
 * \param errCode system error code
 * \return error message
 */
std::wstring GetSystemErrorMessage(const DWORD errCode);

/**
 * Parse URL
 * \param url source url std::string
 * \param scheme scheme name
 * \param hostName host name
 * \param port port number
 * \param path path
 * \param query additional query std::string
 * \param userName user name
 * \param password password
 */
void ParseURL(const wchar_t *url, std::wstring *scheme, std::wstring *hostName, unsigned short *port, std::wstring *path, std::wstring *query, std::wstring *userName, std::wstring *password);

/**
 * Convert unix time to file time
 * \param t unix time
 * \return file time
 */
FILETIME UnixTimeToFileTime(const time_t t);

unsigned long TextToNumber(const std::wstring &text);

/**
 * Convert int to std::string
 * \param number number
 * \return std::string result
 */
std::string NumberToText(int number);

std::wstring NumberToWString(unsigned long number);

struct ProxySettings
{
    int proxyType;
    std::wstring proxyHost;
    unsigned long proxyPort;
    std::wstring proxyLogin;
    std::wstring proxyPassword;
};

struct ProxySettingsDialogParams
{
    FarDialogItem *proxyTypeTextItem;
    FarDialogItem *proxyTypeComboBoxItem;
    FarList proxyTypeList;
    std::vector<FarListItem> proxyTypeListItems;
    FarDialogItem *separatorItem;
    FarDialogItem *proxyHostTextItem;
    FarDialogItem *proxyHostItem;
    FarDialogItem *proxyPortTextItem;
    FarDialogItem *proxyPortItem;
    FarDialogItem *proxyLoginTextItem;
    FarDialogItem *proxyLoginItem;
    FarDialogItem *proxyPasswordTextItem;
    FarDialogItem *proxyPasswordItem;
    std::wstring proxyPortStr;

    int idProxyTypeText;
    int idProxyTypeComboBox;
    int idSeparatorItem;
    int idProxyHostText;
    int idProxyHost;
    int idProxyPortText;
    int idProxyPort;
    int idProxyLoginText;
    int idProxyLogin;
    int idProxyPasswordText;
    int idProxyPassword;
};

void InitProxySettingsDialog(CFarDialog &dlg, int &topPos,
    ProxySettings &ps,
    ProxySettingsDialogParams &params,
    bool visible
);

void GetProxySettings(const CFarDialog &dlg, const struct ProxySettingsDialogParams &params,
    struct ProxySettings &proxySettings);

void AppendWChar(std::wstring &str, const wchar_t ch);
void AppendChar(std::string &str, const char ch);

void AppendPathDelimiterW(std::wstring &str);
void AppendPathDelimiterA(std::string &str);

std::wstring MB2W(const char *src, const UINT cp = CP_ACP);
std::string W2MB(const wchar_t *src, const UINT cp = CP_ACP);

void CheckAbortEvent(HANDLE *AbortEvent);

std::wstring ExpandEnvVars(const std::wstring& str);

std::wstring Trim(const std::wstring str);
std::wstring TrimRight(const std::wstring str);
std::wstring LowerCase(const std::wstring str);
std::wstring AnsiReplaceStr(const std::wstring str, const std::wstring from, const std::wstring to);
int AnsiPos(const std::wstring str, wchar_t ñ);
std::wstring StringReplace(const std::wstring str, const std::wstring from, const std::wstring to);
bool AnsiSameText(const std::wstring str1, const std::wstring str2);
bool IsDelimiter(const std::wstring str1, const std::wstring delim, int size);
int LastDelimiter(const std::wstring str1, const wchar_t delim);

bool ForceDirectories(const std::wstring Dir);
bool DeleteFile(const std::wstring File);
bool RemoveDir(const std::wstring Dir);

std::wstring IncludeTrailingBackslash(const std::wstring Str);
std::wstring ExcludeTrailingBackslash(const std::wstring Str);
std::wstring ExtractFileDir(const std::wstring Str);
std::wstring ExtractFilePath(const std::wstring Str);
std::wstring GetCurrentDir();

std::wstring StringOfChar(const wchar_t c, size_t len);

void RaiseLastOSError();
