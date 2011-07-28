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

CURLcode CFTPS::CURLPrepare(const char *ftpPath, const bool handleTimeout /*= true*/)
{
    CURLcode urlCode = CFTP::CURLPrepare(ftpPath, handleTimeout);
    if (urlCode != CURLE_OK)
    {
        return urlCode;
    }
    DEBUG_PRINTF(L"NetBox: CFTPS::CURLPrepare: path = %s", ftpPath);
    CURL *curl = m_CURL;
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT));
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE));
    CHECK_CUCALL(urlCode, curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1));
    return urlCode;
}

