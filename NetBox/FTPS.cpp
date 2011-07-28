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

PSessionEditor CSessionFTPS::CreateEditorInstance()
{
    return PSessionEditor(new CSessionEditorFTPS(this));
}

PProtocol CSessionFTPS::CreateClientInstance() const
{
    return PProtocol(new CFTPS(this));
}


CFTPS::CFTPS(const CSession *session)
    : CFTP(session)
{
}

bool CFTPS::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo)
{
    assert(path && path[0] == L'/');

    string ftpPath = LocalToFtpCP(path);
    DEBUG_PRINTF(L"NetBox: CFTPS::CheckExisting: ftpPath = %s", CFarPlugin::MB2W(ftpPath.c_str()).c_str());
    if (type == ItemDirectory && ftpPath[ftpPath.length() - 1] != '/')
    {
        ftpPath += '/';
    }

    isExist = true;

    CURLcode urlCode = m_CURL.Prepare(ftpPath.c_str());

    DEBUG_PRINTF(L"NetBox: CFTPS::CheckExisting: path = %s", path);
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

