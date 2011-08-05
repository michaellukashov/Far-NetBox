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


CFTPS::CFTPS(const CSession *session) :
    CFTP(session),
    m_SSL_VERIFYPEER(TRUE)
{
}

CFTPS::~CFTPS()
{
}

CURLcode CFTPS::CURLPrepare(const char *ftpPath, const bool handleTimeout /*= true*/)
{
    CURLcode urlCode = CFTP::CURLPrepare(ftpPath, handleTimeout);
    if (urlCode == CURLE_OK)
    {
        // DEBUG_PRINTF(L"NetBox: CFTPS::CURLPrepare: path = %s", ftpPath);
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYPEER, m_SSL_VERIFYPEER));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYHOST, 1));

        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_USE_SSL, CURLUSESSL_CONTROL));
    }
    return urlCode;
}

bool CFTPS::TryToResolveConnectionProblem()
{
    if (m_lastErrorCurlCode == CURLE_SSL_CACERT)
    {
        // Показываем предупреждение о сертификате
        // DEBUG_PRINTF(L"NetBox: TryToResolveConnectionProblem: errorCode: %u", m_lastErrorCurlCode);
        wstring errorInfo = ::MB2W(curl_easy_strerror(m_lastErrorCurlCode));
        wstring msg = CFarPlugin::GetFormattedString(StringSSLErrorContinue, errorInfo.c_str());
        const int retCode = CFarPlugin::MessageBox(CFarPlugin::GetString(StringTitle), msg.c_str(), FMSG_MB_YESNO | FMSG_WARNING);
        if (retCode == 0) // Yes
        {
            m_SSL_VERIFYPEER = FALSE;
            m_lastErrorCurlCode = CURLE_OK;
            return true;
        }
        m_SSL_VERIFYPEER = TRUE;
    }
    return false;
}
