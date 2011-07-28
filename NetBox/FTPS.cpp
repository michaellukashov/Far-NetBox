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
#include "FTPS.h"
#include "Logging.h"
#include "Strings.h"

static const char *ParamCodePage = "CodePage";
static const char *ParamKeyFile =  "KeyFile";


UINT CSessionFTPS::GetCodePage() const
{
    return static_cast<UINT>(GetPropertyNumeric(ParamCodePage, CP_UTF8));
}


void CSessionFTPS::SetCodePage(const UINT cp)
{
    SetProperty(ParamCodePage, static_cast<__int64>(cp));
}


const wchar_t *CSessionFTPS::GetKeyFile() const
{
    return GetProperty(ParamKeyFile, L"");
}


void CSessionFTPS::SetKeyFile(const wchar_t *fileName)
{
    assert(fileName);
    SetProperty(ParamKeyFile, fileName);
}


PSessionEditor CSessionFTPS::CreateEditorInstance()
{
    return PSessionEditor(new CSessionEditorFTPS(this));
}


PProtocol CSessionFTPS::CreateClientInstance() const
{
    return PProtocol(new CFTPS(this));
}


CSessionEditorFTPS::CSessionEditorFTPS(CSession *session)
    : CSessionEditor(session, 54, 24), _IdKeyFile(0)
{
}


void CSessionEditorFTPS::OnPrepareDialog()
{
    _IdText = CreateText(GetLeft(), GetHeight() - 7, CFarPlugin::GetString(StringEdAuthCert));
    _IdKeyFile = CreateEdit(GetLeft(), GetHeight() - 6, MAX_SIZE, static_cast<CSessionFTPS *>(m_Session)->GetKeyFile());

    _IdSeparator = CreateSeparator(GetHeight() - 5);
    CreateCodePageControl(GetHeight() - 4, static_cast<CSessionFTPS *>(m_Session)->GetCodePage(),
        _IdCPText, _IdCP);
}


void CSessionEditorFTPS::OnSave()
{
    static_cast<CSessionFTPS *>(m_Session)->SetKeyFile(GetText(_IdKeyFile).c_str());
    static_cast<CSessionFTPS *>(m_Session)->SetCodePage(static_cast<UINT>(_wtoi(GetText(_IdCP).c_str())));
}

void CSessionEditorFTPS::ShowSessionDlgItems(bool visible)
{
    CSessionEditor::ShowSessionDlgItems(visible);
    ShowDlgItem(_IdText, visible);
    ShowDlgItem(_IdKeyFile, visible);
    ShowDlgItem(_IdSeparator, visible);
    ShowDlgItem(_IdCPText, visible);
    ShowDlgItem(_IdCP, visible);
}

CFTPS::CFTPS(const CSession *session)
    : CProtocolBase(session), m_AbortEvent(NULL)
{
}


CFTPS::~CFTPS()
{
    Close();
}


bool CFTPS::Connect(HANDLE abortEvent, wstring &errorInfo)
{
    assert(abortEvent);

    m_AbortEvent = abortEvent;

    const wchar_t *url = m_Session.GetURL();
    DEBUG_PRINTF(L"NetBox: FTPS::Connect: connecting to %s", url);
    //Initialize curl
    m_CURL.Initialize(url, m_Session.GetUserName(), m_Session.GetPassword(),
        m_Session.GetProxySettings());
    m_CURL.SetAbortEvent(abortEvent);

    wstring path;
    ParseURL(url, NULL, NULL, NULL, &path, NULL, NULL, NULL);
    bool dirExist = false;
    DEBUG_PRINTF(L"NetBox: FTPS::Connect: path = %s", path.c_str());
    if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        Log2(L"WebDAV: FTPS::Connect: error: path %s does not exist.", path.c_str());
        return false;
    }
    m_CurrentDirectory = path;
    DEBUG_PRINTF(L"NetBox: FTPS::Connect: _CurrentDirectory1 = %s", m_CurrentDirectory.c_str());
    while(m_CurrentDirectory.size() > 1 && m_CurrentDirectory[m_CurrentDirectory.length() - 1] == L'/')
    {
        m_CurrentDirectory.erase(m_CurrentDirectory.length() - 1);
    }
    DEBUG_PRINTF(L"NetBox: FTPS::Connect: _CurrentDirectory2 = %s", m_CurrentDirectory.c_str());
    DEBUG_PRINTF(L"NetBox: Connect: end");
    return true;
}


void CFTPS::Close()
{
    m_CURL.Close();
}


bool CFTPS::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo)
{
    assert(path && path[0] == L'/');

    string ftpPath = LocalToFtpCP(path);
    DEBUG_PRINTF(L"NetBox: CheckExisting: ftpPath = %s", CFarPlugin::MB2W(ftpPath.c_str()).c_str());
    if (type == ItemDirectory && ftpPath[ftpPath.length() - 1] != '/')
    {
        ftpPath += '/';
    }

    isExist = true;

    CURLcode urlCode = m_CURL.Prepare(ftpPath.c_str());

    CURL *curl = m_CURL;
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT));
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE));
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1));

    CHECK_CUCALL(urlCode, m_CURL.Perform());
    if (urlCode != CURLE_OK)
    {
        errorInfo = CFarPlugin::MB2W(curl_easy_strerror(urlCode));
        isExist = false;
    }

    return true;
}

bool CFTPS::MakeDirectory(const wchar_t *path, wstring &errorInfo)
{
    bool retStatus = false;
    return retStatus;
}


bool CFTPS::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);
    return true;
}


bool CFTPS::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
    return true;
}


bool CFTPS::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
    return true;
}


bool CFTPS::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, wstring &errorInfo)
{
    bool retStatus = false;
    return retStatus;
}


bool CFTPS::Delete(const wchar_t *path, const ItemType type, wstring &errorInfo)
{
    bool retStatus = false;
    return retStatus;
}

