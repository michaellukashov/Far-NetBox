#include "stdafx.h"
#include "EasyURL.h"
#include "SessionData.h"
#include "Terminal.h"

CEasyURL::CEasyURL(TTerminal *Terminal) :
    FTerminal(Terminal),
    m_CURL(NULL),
    m_Prepared(false),
    m_regex(INVALID_HANDLE_VALUE),
    m_match(NULL),
    m_brackets(0),
    FDebugLevel(LOG_STATUS)
{
}

void CEasyURL::Init()
{
    m_Input.AbortEvent = m_Output.AbortEvent = m_Progress.AbortEvent = NULL;
    m_Input.Type = InputReader::None;
    m_Output.Type = OutputWriter::None;
    m_Progress.ProgressPtr = NULL;
    m_Progress.Aborted = false;
    // init regex
    if (FarPlugin->GetStartupInfo()->RegExpControl(0, RECTL_CREATE, reinterpret_cast<LONG_PTR>(&m_regex)))
    {
        if (FarPlugin->GetStartupInfo()->RegExpControl(m_regex, RECTL_COMPILE, reinterpret_cast<LONG_PTR>(L"/PASS(.*)/")))
        {
            m_brackets = FarPlugin->GetStartupInfo()->RegExpControl(m_regex, RECTL_BRACKETSCOUNT, 0);
            if (m_brackets == 2)
            {
                m_match = reinterpret_cast<RegExpMatch *>(malloc(m_brackets * sizeof(RegExpMatch)));
            }
        }
    }
}

CEasyURL::~CEasyURL()
{
    Close();
    free(m_match);
    if (m_regex != INVALID_HANDLE_VALUE)
    {
        FarPlugin->GetStartupInfo()->RegExpControl(m_regex, RECTL_FREE, 0);
    }
}


bool CEasyURL::Initialize(const wchar_t *url, const wchar_t *userName,
    const wchar_t *password)
{
    assert(m_CURL == NULL);
    assert(url && *url);

    m_CURL = curl_easy_init();
    if (!m_CURL)
    {
        return false;
    }

    std::wstring scheme;
    std::wstring hostName;
    unsigned short port = 0;
    ParseURL(url, &scheme, &hostName, &port, NULL, NULL, NULL, NULL);

    m_TopURL   = ::W2MB(scheme.c_str());
    m_TopURL  += "://";
    m_TopURL  += ::W2MB(hostName.c_str());
    if (port)
    {
        m_TopURL += ':';
        m_TopURL += NumberToText(port);
    }

    if (userName)
    {
        m_UserName = ::W2MB(userName);
    }
    if (password)
    {
        m_Password = ::W2MB(password);
    }
    return true;
}


bool CEasyURL::Close()
{
    if (m_CURL)
    {
        curl_easy_cleanup(m_CURL);
    }
    m_CURL = NULL;
    return true;
}


CURLcode CEasyURL::Prepare(const char *path,
    const TSessionData *Data, int LogLevel,
    const bool handleTimeout /*= true*/)
{
    assert(m_CURL);
    assert(!m_Prepared);
    assert(!path || path[0] == L'/');
    // DEBUG_PRINTF(L"CEasyURL::Prepare: m_TopURL = %s, path = %s", ::MB2W(m_TopURL.c_str()).c_str(), ::MB2W(path).c_str());
    curl_easy_reset(m_CURL);
    m_Output.Type = OutputWriter::None;
    m_Input.Type = InputReader::None;
    m_Progress.ProgressPtr = NULL;

    CURLcode urlCode = CURLE_OK;

    const std::string url = m_TopURL + (path ? path : "");
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_URL, url.c_str()));
    if (handleTimeout)
    {
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_TIMEOUT, Data->GetTimeout()));
    }
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_WRITEFUNCTION, CEasyURL::InternalWriter));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_WRITEDATA, &m_Output));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_NOPROGRESS, 0L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROGRESSFUNCTION, CEasyURL::InternalProgress));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROGRESSDATA, &m_Progress));

    if (LogLevel >= 1)
    {
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_DEBUGFUNCTION, CEasyURL::InternalDebug));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_DEBUGDATA, this));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_VERBOSE, TRUE));
    }

    if (!m_UserName.empty() || !m_Password.empty())
    {
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_USERNAME, m_UserName.c_str()));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PASSWORD, m_Password.c_str()));
    }
    TProxyMethod ProxyMethod = Data->GetProxyMethod();
    if (ProxyMethod != pmNone)
    {
        int proxy_type = CURLPROXY_HTTP;
        switch (ProxyMethod)
        {
            case pmHTTP:
            {
                proxy_type = CURLPROXY_HTTP;
                break;
            }
            case pmSocks4:
            {
                proxy_type = CURLPROXY_SOCKS4;
                break;
            }
            case pmSocks5:
            {
                proxy_type = CURLPROXY_SOCKS5;
                break;
            }
            default:
                return CURLE_UNSUPPORTED_PROTOCOL;
        }
        std::string proxy = ::W2MB(Data->GetProxyHost().c_str());
        unsigned long port = Data->GetProxyPort();
        if (port)
        {
            proxy += ":";
            proxy += NumberToText(port);
        }
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROXY, proxy.c_str()));
        // CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROXYPORT, port));
        CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROXYTYPE, proxy_type));

        std::string login = ::W2MB(Data->GetProxyUsername().c_str());
        std::string password = ::W2MB(Data->GetProxyPassword().c_str());
        if (!login.empty())
        {
            CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROXYUSERNAME, login.c_str()));
            if (!password.empty())
            {
                CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_PROXYPASSWORD, password.c_str()));
            }
        }
    }

    m_Prepared = (urlCode == CURLE_OK);
    // DEBUG_PRINTF(L"Prepare: m_Prepared = %u", m_Prepared);
    return urlCode;
}


CURLcode CEasyURL::SetSlist(CSlistURL &slist)
{
    CURLcode urlCode = CURLE_OK;
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_POSTQUOTE, static_cast<curl_slist *>(slist)));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_HTTPHEADER, static_cast<curl_slist *>(slist)));
    return urlCode;
}


CURLcode CEasyURL::SetOutput(std::string &out, int *progress)
{
    assert(m_Prepared);
    assert(progress);

    m_Output.Type = OutputWriter::TypeString;
    m_Output.String = &out;
    m_Progress.ProgressPtr = progress;

    return CURLE_OK;
}


CURLcode CEasyURL::SetOutput(CNBFile *out, int *progress)
{
    assert(m_Prepared);
    assert(out);
    assert(progress);

    m_Output.Type = OutputWriter::TypeFile;
    m_Output.File = out;
    m_Progress.ProgressPtr = progress;

    return CURLE_OK;
}


CURLcode CEasyURL::SetInput(CNBFile *in, int *progress)
{
    assert(m_Prepared);
    assert(in);
    assert(progress);

    m_Input.Type = InputReader::TypeFile;
    m_Input.File = in;
    m_Input.Progress = progress;
    m_Input.Current = 0;
    m_Input.Total = in->GetFileSize();

    CURLcode urlCode = CURLE_OK;
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_UPLOAD, 1L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_READFUNCTION, CEasyURL::InternalReader));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_READDATA, &m_Input));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(in->GetFileSize())));
    return urlCode;
}


void CEasyURL::SetAbortEvent(HANDLE event)
{
    m_Input.AbortEvent = m_Output.AbortEvent = m_Progress.AbortEvent = event;
    m_Progress.Aborted = false;
}


CURLcode CEasyURL::Perform()
{
    assert(m_CURL);
    assert(m_Prepared);

    m_Prepared = false;
    return curl_easy_perform(m_CURL);
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

    ::CheckAbortEvent(&prg->AbortEvent);

    if (prg->AbortEvent && WaitForSingleObject(prg->AbortEvent, 0) == WAIT_OBJECT_0)
    {
        prg->Aborted = true;
        return CURLE_ABORTED_BY_CALLBACK;
    }

    if (prg->ProgressPtr && dltotal > 0)
    {
        const double percent = dlnow * 100.0 / dltotal;
        *prg->ProgressPtr = static_cast<int>(percent);
    }
    return CURLE_OK;
}

int CEasyURL::DebugOutput(TLogLineType type, const char *data, size_t size)
{
    // PASS *****
    if (m_regex != INVALID_HANDLE_VALUE && m_match != NULL)
    {
        // DEBUG_PRINTF(L"data = %s", ::MB2W(data).c_str());
        std::wstring dataw = ::MB2W(data);
        RegExpSearch search = {
            dataw.c_str(),
            0,
            dataw.size(),
            m_match,
            m_brackets,
            0
        };
        if (FarPlugin->GetStartupInfo()->RegExpControl(m_regex, RECTL_SEARCHEX, reinterpret_cast<LONG_PTR>(&search)))
        {
            // DEBUG_PRINTF(L"PASS ****");
            FTerminal->GetLog()->Add(type, L"PASS ****");
            return 0;
        }
    }
    DEBUG_PRINTF(L"::MB2W(data) = %s", ::MB2W(std::string(data, size).c_str()).c_str());
    FTerminal->GetLog()->Add(type, ::MB2W(std::string(data, size).c_str()));
    return 0;
}

int CEasyURL::InternalDebug(CURL *handle, curl_infotype type,
                 char *data, size_t size,
                 void *userp)
{
    CEasyURL *instance = reinterpret_cast<CEasyURL *>(userp);
    assert(instance != NULL);
    (void)handle;
    (void)type;
    TLogLineType llType = llOutput;
    switch (type)
    {
        case CURLINFO_TEXT:
        case CURLINFO_HEADER_IN:
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
            llType = llInput;
            break;
        case CURLINFO_HEADER_OUT:
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
            llType = llOutput;
            break;
    }
    return instance->DebugOutput(llType, data, size);
}
