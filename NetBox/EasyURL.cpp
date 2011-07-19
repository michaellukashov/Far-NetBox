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
#include "EasyURL.h"
#include "Settings.h"


CEasyURL::CEasyURL()
    : _CURL(NULL), _Prepared(false)
{
    _Input.AbortEvent = _Output.AbortEvent = _Progress.AbortEvent = NULL;
    _Input.Type = InputReader::None;
    _Output.Type = OutputWriter::None;
    _Progress.ProgressPtr = NULL;
}


CEasyURL::~CEasyURL()
{
    Close();
}


bool CEasyURL::Initialize(const wchar_t *url, const wchar_t *userName, const wchar_t *password,
    const struct ProxySettings &proxySettings)
{
    assert(_CURL == NULL);
    assert(url && *url);

    _CURL = curl_easy_init();
    if (!_CURL)
    {
        return false;
    }

    wstring scheme;
    wstring hostName;
    unsigned short port = 0;
    ParseURL(url, &scheme, &hostName, &port, NULL, NULL, NULL, NULL);

    _TopURL   = CFarPlugin::W2MB(scheme.c_str());
    _TopURL  += "://";
    _TopURL  += CFarPlugin::W2MB(hostName.c_str());
    if (port)
    {
        _TopURL += ':';
        _TopURL += NumberToText(port);
    }

    if (userName)
    {
        _UserName = CFarPlugin::W2MB(userName);
    }
    if (password)
    {
        _Password = CFarPlugin::W2MB(password);
    }
    _proxySettings = proxySettings;
    DEBUG_PRINTF(L"NetBox: CEasyURL::Initialize: proxy type = %u, adress = %s", _proxySettings.proxyType, _proxySettings.proxyHost.c_str());
    return true;
}


void CEasyURL::Close()
{
    if (_CURL)
    {
        curl_easy_cleanup(_CURL);
    }
    _CURL = NULL;

}


CURLcode CEasyURL::Prepare(const char *path, const bool handleTimeout /*= true*/)
{
    assert(_CURL);
    assert(!_Prepared);
    assert(!path || path[0] == L'/');
    // DEBUG_PRINTF(L"NetBox: CEasyURL::Prepare: path = %s", CFarPlugin::MB2W(path).c_str());
    curl_easy_reset(_CURL);
    _Output.Type = OutputWriter::None;
    _Input.Type = InputReader::None;
    _Progress.ProgressPtr = NULL;

    CURLcode urlCode = CURLE_OK;

    const string url = _TopURL + (path ? path : "");
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_URL, url.c_str()));
    if (handleTimeout)
    {
        CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_TIMEOUT, _Settings.Timeout()));
    }
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_WRITEFUNCTION, CEasyURL::InternalWriter));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_WRITEDATA, &_Output));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_NOPROGRESS, 0L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROGRESSFUNCTION, CEasyURL::InternalProgress));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROGRESSDATA, &_Progress));

    if (!_UserName.empty() || !_Password.empty())
    {
        CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_USERNAME, _UserName.c_str()));
        CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PASSWORD, _Password.c_str()));
    }
    // DEBUG_PRINTF(L"NetBox: CEasyURL::Prepare: proxy type = %u, host = %s", _proxySettings.proxyType, _proxySettings.proxyHost.c_str());
    if (_proxySettings.proxyType != PROXY_NONE)
    {
        int proxy_type = CURLPROXY_HTTP;
        switch (_proxySettings.proxyType)
        {
            case PROXY_HTTP:
            {
                proxy_type = CURLPROXY_HTTP;
                break;
            }
            case PROXY_SOCKS4:
            {
                proxy_type = CURLPROXY_SOCKS4;
                break;
            }
            case PROXY_SOCKS5:
            {
                proxy_type = CURLPROXY_SOCKS5;
                break;
            }
            default:
                return CURLE_UNSUPPORTED_PROTOCOL;
        }
        string proxy = CFarPlugin::W2MB(_proxySettings.proxyHost.c_str());
        unsigned long port = _proxySettings.proxyPort;
        if (port)
        {
            proxy += ":";
            proxy += NumberToText(port);
        }
        CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROXY, proxy.c_str()));
        // CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROXYPORT, port));
        CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROXYTYPE, proxy_type));

        // CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_VERBOSE, 1));

        string login = CFarPlugin::W2MB(_proxySettings.proxyLogin.c_str());
        string password = CFarPlugin::W2MB(_proxySettings.proxyPassword.c_str());
        if (!login.empty())
        {
            CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROXYUSERNAME, login.c_str()));
            if (!password.empty())
            {
                CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_PROXYPASSWORD, password.c_str()));
            }
        }
    }

    _Prepared = (urlCode == CURLE_OK);
    return urlCode;
}


CURLcode CEasyURL::SetSlist(CSlistURL &slist)
{
    CURLcode urlCode = CURLE_OK;
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_POSTQUOTE, static_cast<curl_slist *>(slist)));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_HTTPHEADER, static_cast<curl_slist *>(slist)));
    return urlCode;
}


CURLcode CEasyURL::SetOutput(string *out, int *progress)
{
    assert(_Prepared);
    assert(out);
    assert(progress);

    _Output.Type = OutputWriter::TypeString;
    _Output.String = out;
    _Progress.ProgressPtr = progress;

    return CURLE_OK;
}


CURLcode CEasyURL::SetOutput(CFile *out, int *progress)
{
    assert(_Prepared);
    assert(out);
    assert(progress);

    _Output.Type = OutputWriter::TypeFile;
    _Output.File = out;
    _Progress.ProgressPtr = progress;

    return CURLE_OK;
}


CURLcode CEasyURL::SetInput(CFile *in, int *progress)
{
    assert(_Prepared);
    assert(in);
    assert(progress);

    _Input.Type = InputReader::TypeFile;
    _Input.File = in;
    _Input.Progress = progress;
    _Input.Current = 0;
    _Input.Total = in->GetFileSize();

    CURLcode urlCode = CURLE_OK;
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_UPLOAD, 1L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_READFUNCTION, CEasyURL::InternalReader));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_READDATA, &_Input));
    CHECK_CUCALL(urlCode, curl_easy_setopt(_CURL, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(in->GetFileSize())));
    return urlCode;
}


void CEasyURL::SetAbortEvent(HANDLE event)
{
    _Input.AbortEvent = _Output.AbortEvent = _Progress.AbortEvent = event;
}


CURLcode CEasyURL::Perform()
{
    assert(_CURL);
    assert(_Prepared);

    _Prepared = false;
    return curl_easy_perform(_CURL);
}


CURLcode CEasyURL::ExecuteFtpCommand(const char *cmd)
{
    assert(cmd);
    DEBUG_PRINTF(L"NetBox: ExecuteFtpCommand: cmd = %s", CFarPlugin::MB2W(cmd).c_str());

    CSlistURL slist;
    slist.Append(cmd);
    CURLcode errCode = SetSlist(slist);

    if (errCode == CURLE_OK)
    {
        errCode = Prepare(NULL);
    }
    if (errCode == CURLE_OK)
    {
        errCode = curl_easy_setopt(_CURL, CURLOPT_QUOTE, slist);
    }
    if (errCode == CURLE_OK)
    {
        errCode = Perform();
    }

    return errCode;
}


size_t CEasyURL::InternalWriter(void *buffer, size_t size, size_t nmemb, void *userData)
{
    OutputWriter *writer = static_cast<OutputWriter *>(userData);
    assert(writer);
    if (writer->AbortEvent && WaitForSingleObject(writer->AbortEvent, 0) == WAIT_OBJECT_0)
    {
        return 0;
    }

#ifdef _DEBUG
    const char *dbgOut = static_cast<const char *>(buffer);
    UNREFERENCED_PARAMETER(dbgOut);
#endif

    const size_t buffLen = size * nmemb;

    if (writer->Type == OutputWriter::TypeString)
    {
        writer->String->append(static_cast<const char *>(buffer), static_cast<const char *>(buffer) + buffLen);
    }
    else if (writer->Type == OutputWriter::TypeFile)
    {
        return writer->File->Write(buffer, buffLen) ? buffLen : 0;
    }

    return buffLen;
}


size_t CEasyURL::InternalReader(void *buffer, size_t size, size_t nmemb, void *userData)
{
    InputReader *reader = static_cast<InputReader *>(userData);
    assert(reader);
    if (reader->AbortEvent && WaitForSingleObject(reader->AbortEvent, 0) == WAIT_OBJECT_0)
    {
        return CURL_READFUNC_ABORT;
    }

    size_t buffLen = size * nmemb;

    if (reader->Type == InputReader::TypeFile)
    {
        if (!reader->File->Read(buffer, buffLen))
        {
            buffLen = 0;
        }
        else
        {
            reader->Current += buffLen;
            if (reader->Total != 0)
            {
                *reader->Progress = static_cast<int>(reader->Current * 100 / reader->Total);
            }
            else
            {
                *reader->Progress = 100;
            }
        }
    }

    return buffLen;
}


int CEasyURL::InternalProgress(void *userData, double dltotal, double dlnow, double /*ultotal*/, double /*ulnow*/)
{
    Progress *prg = static_cast<Progress *>(userData);
    assert(prg);

    CFarPlugin::CheckAbortEvent(&prg->AbortEvent);

    if (prg->AbortEvent && WaitForSingleObject(prg->AbortEvent, 0) == WAIT_OBJECT_0)
    {
        return CURLE_ABORTED_BY_CALLBACK;
    }

    if (prg->ProgressPtr && dltotal > 0)
    {
        const double percent = dlnow * 100.0 / dltotal;
        *prg->ProgressPtr = static_cast<int>(percent);
    }
    return CURLE_OK;
}
