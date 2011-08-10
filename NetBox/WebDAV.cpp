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

#include "tinyXML\tinyxml.h"
#include <Winhttp.h>

#include "Strings.h"
#include "Settings.h"
#include "Logging.h"
#include "WebDAV.h"


PSessionEditor CSessionWebDAV::CreateEditorInstance()
{
    return PSessionEditor(new CSessionEditorWebDAV(this));
}


PProtocol CSessionWebDAV::CreateClientInstance() const
{
    return PProtocol(new CWebDAV(this));
}


CWebDAV::CWebDAV(const CSession *session)
    : CProtocolBase(session)
{
}


CWebDAV::~CWebDAV()
{
    Close();
}


bool CWebDAV::Connect(HANDLE abortEvent, wstring &errorInfo)
{
    assert(abortEvent);

    const wchar_t *url = m_Session.GetURL();
    DEBUG_PRINTF(L"NetBox: WebDAV: connecting to %s", url);
    //Initialize curl
    m_CURL.Initialize(url, m_Session.GetUserName(), m_Session.GetPassword(),
        m_Session.GetProxySettings());
    m_CURL.SetAbortEvent(abortEvent);

    //Check initial path existing
    wstring path;
    // wstring query;
    ParseURL(url, NULL, NULL, NULL, &path, NULL, NULL, NULL);
    bool dirExist = false;
    // DEBUG_PRINTF(L"NetBox: path = %s, query = %s", path.c_str(), query.c_str());
    // if (!query.empty())
    // {
        // path += query;
    // }
    if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        Log2(L"WebDAV: error: path %s does not exist.", path.c_str());
        return false;
    }
    m_CurrentDirectory = path;
    while(m_CurrentDirectory.size() > 1 && m_CurrentDirectory[m_CurrentDirectory.length() - 1] == L'/')
    {
        m_CurrentDirectory.erase(m_CurrentDirectory.length() - 1);
    }
    return true;
}


void CWebDAV::Close()
{
    m_CURL.Close();
}


bool CWebDAV::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo)
{
    assert(type == ItemDirectory);

    string responseDummy;
    isExist = SendPropFindRequest(path, responseDummy, errorInfo);
    return true;
}


bool CWebDAV::MakeDirectory(const wchar_t *path, wstring &errorInfo)
{
    const string webDavPath = EscapeUTF8URL(path);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_CUSTOMREQUEST, "MKCOL"));

    CHECK_CUCALL(urlCode, m_CURL.Perform());
    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_CREATED, errorInfo);
}


bool CWebDAV::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);

    string response;
    if (!SendPropFindRequest(m_CurrentDirectory.c_str(), response, errorInfo))
    {
        return false;
    }

    //Erase slashes (to compare in xml parse)
    wstring currentPath(m_CurrentDirectory);
    while (!currentPath.empty() && currentPath[currentPath.length() - 1] == L'/')
    {
        currentPath.erase(currentPath.length() - 1);
    }
    while (!currentPath.empty() && currentPath[0] == L'/')
    {
        currentPath.erase(0, 1);
    }

    const string decodedResp = DecodeHex(response);

#ifdef _DEBUG
    //////////////////////////////////////////////////////////////////////////
    //CFile::SaveFile(L"d:\\webdav_response_raw.xml", response.c_str());
    //CFile::SaveFile(L"d:\\webdav_response_decoded.xml", decodedResp.c_str());
    //////////////////////////////////////////////////////////////////////////
#endif  //_DEBUG

    //! WebDAV item description
    struct WebDAVItem
    {
        WebDAVItem() : Attributes(0), Size(0)
        {
            LastAccess.dwLowDateTime = LastAccess.dwHighDateTime = Created.dwLowDateTime = Created.dwHighDateTime = Modified.dwLowDateTime = Modified.dwHighDateTime = 0;
        }
        wstring             Name;
        DWORD               Attributes;
        FILETIME            Created;
        FILETIME            Modified;
        FILETIME            LastAccess;
        unsigned __int64    Size;
    };
    vector<WebDAVItem> wdavItems;

    TiXmlDocument xmlDoc;
    xmlDoc.Parse(decodedResp.c_str());
    if (xmlDoc.Error())
    {
        errorInfo = L"Error parsing response xml:\n[";
        errorInfo += ::NumberToWString(xmlDoc.ErrorId());
        errorInfo += L"]: ";
        errorInfo += ::MB2W(xmlDoc.ErrorDesc());
        return false;
    }

    const TiXmlElement *xmlRoot = xmlDoc.RootElement();

    //Determine global namespace
    const string glDavNs = GetNamespace(xmlRoot, "DAV:", "D:");
    const string glMsNs = GetNamespace(xmlRoot, "urn:schemas-microsoft-com:", "Z:");

    const TiXmlNode *xmlRespNode = NULL;
    while ((xmlRespNode = xmlRoot->IterateChildren((glDavNs + "response").c_str(), xmlRespNode)) != NULL)
    {
        WebDAVItem item;

        const TiXmlElement *xmlRespElem = xmlRespNode->ToElement();
        const string davNamespace = GetNamespace(xmlRespElem, "DAV:", glDavNs.c_str());
        const string msNamespace = GetNamespace(xmlRespElem, "urn:schemas-microsoft-com:", glMsNs.c_str());
        const TiXmlElement *xmlHref = xmlRespNode->FirstChildElement((glDavNs + "href").c_str());
        if (!xmlHref || !xmlHref->GetText())
        {
            continue;
        }

        const wstring href = ::MB2W(xmlHref->GetText(), CP_UTF8);
        wstring path;
        ParseURL(href.c_str(), NULL, NULL, NULL, &path, NULL, NULL, NULL);
        if (path.empty())
        {
            path = href;
        }
        while (!path.empty() && path[path.length() - 1] == L'/')
        {
            path.erase(path.length() - 1);
        }
        while (!path.empty() && path[0] == L'/')
        {
            path.erase(0, 1);
        }

        //Check for self-link (compare paths)
        if (_wcsicmp(path.c_str(), currentPath.c_str()) == 0)
        {
            continue;
        }

        //name
        item.Name = path;
        const size_t nameDelim = item.Name.rfind(L'/'); //Save only name without full path
        if (nameDelim != string::npos)
        {
            item.Name.erase(0, nameDelim + 1);
        }

        //Find correct 'propstat' node (with HTTP 200 OK status)
        const TiXmlElement *xmlProps = NULL;
        const TiXmlNode *xmlPropsNode = NULL;
        while (xmlProps == NULL && (xmlPropsNode = xmlRespNode->IterateChildren((glDavNs + "propstat").c_str(), xmlPropsNode)) != NULL)
        {
            const TiXmlElement *xmlStatus = xmlPropsNode->FirstChildElement((glDavNs + "status").c_str());
            if (xmlStatus && strstr(xmlStatus->GetText(), "200"))
            {
                xmlProps = xmlPropsNode->FirstChildElement((glDavNs + "prop").c_str());
            }
        }
        if (xmlProps)
        {
            /************************************************************************/
            /* WebDAV [D:] (DAV:)
            /************************************************************************/
            //attributes
            const TiXmlElement *xmlResType = xmlProps->FirstChildElement((davNamespace + "resourcetype").c_str());
            if (xmlResType && xmlResType->FirstChildElement((glDavNs + "collection").c_str()))
            {
                item.Attributes = FILE_ATTRIBUTE_DIRECTORY;
            }

            //size
            const TiXmlElement *xmlSize = xmlProps->FirstChildElement((davNamespace + "getcontentlength").c_str());
            if (xmlSize && xmlSize->GetText())
            {
                item.Size = _atoi64(xmlSize->GetText());
            }

            //creation datetime
            const TiXmlElement *xmlCrDate = xmlProps->FirstChildElement((davNamespace + "creationdate").c_str());
            if (xmlCrDate && xmlCrDate->GetText())
            {
                item.Created = ParseDateTime(xmlCrDate->GetText());
            }

            //last modified datetime
            const TiXmlElement *xmlLmDate = xmlProps->FirstChildElement((davNamespace + "getlastmodified").c_str());
            if (xmlLmDate && xmlLmDate->GetText())
            {
                item.Modified = ParseDateTime(xmlLmDate->GetText());
            }

            /************************************************************************/
            /* Win32 [Z:] (urn:schemas-microsoft-com)
            /************************************************************************/
            //last access datetime
            // TODO: process D:creationdate D:getlastmodified
            const TiXmlElement *xmlLaDate = xmlProps->FirstChildElement((msNamespace + "Win32LastAccessTime").c_str());
            if (xmlLaDate && xmlLaDate->GetText())
            {
                item.LastAccess = ParseDateTime(xmlLaDate->GetText());
            }

            //attributes
            const TiXmlElement *xmlAttr = xmlProps->FirstChildElement((msNamespace + "Win32FileAttributes").c_str());
            if (xmlAttr && xmlAttr->GetText())
            {
                DWORD attr = 0;
                sscanf_s(xmlAttr->GetText(), "%x", &attr);
                item.Attributes |= attr;
            }
        }
        wdavItems.push_back(item);
    }

    *itemsNum = static_cast<int>(wdavItems.size());
    if (*itemsNum)
    {
        *items = new PluginPanelItem[*itemsNum];
        ZeroMemory(*items, sizeof(PluginPanelItem) * (*itemsNum));
        for (int i = 0; i < *itemsNum; ++i)
        {
            PluginPanelItem &farItem = (*items)[i];
            const size_t nameSize = wdavItems[i].Name.length() + 1;
            wchar_t *name = new wchar_t[nameSize];
            wcscpy_s(name, nameSize, wdavItems[i].Name.c_str());
            farItem.FindData.lpwszFileName = name;
            farItem.FindData.dwFileAttributes = wdavItems[i].Attributes;
            if ((farItem.FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                farItem.FindData.nFileSize = wdavItems[i].Size;
            }
            farItem.FindData.ftCreationTime = wdavItems[i].Created;
            farItem.FindData.ftLastWriteTime = wdavItems[i].Modified;
            farItem.FindData.ftLastAccessTime = wdavItems[i].LastAccess;
        }
    }

    return true;
}


bool CWebDAV::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, wstring &errorInfo)
{
    assert(localPath && *localPath);

    CFile outFile;
    if (!outFile.OpenWrite(localPath))
    {
        errorInfo = FormatErrorDescription(outFile.LastError());
        return false;
    }

    const string webDavPath = EscapeUTF8URL(remotePath);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str(), false);
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, m_CURL.SetOutput(&outFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    outFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_OK, errorInfo);
}


bool CWebDAV::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, wstring &errorInfo)
{
    assert(localPath && *localPath);

    CFile inFile;
    if (!inFile.OpenRead(localPath))
    {
        errorInfo = FormatErrorDescription(inFile.LastError());
        return false;
    }

    const string webDavPath = EscapeUTF8URL(remotePath);
    CURLcode urlCode = CURLPrepare(webDavPath.c_str(), false);
    CSlistURL slist;
    slist.Append("Expect:");    //Expect: 100-continue is not wanted
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, m_CURL.SetInput(&inFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    inFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_CREATED, HTTP_STATUS_NO_CONTENT, errorInfo);
}


bool CWebDAV::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, wstring &errorInfo)
{
    const string srcWebDavPath = EscapeUTF8URL(srcPath);
    const string dstWebDavPath = EscapeUTF8URL(dstPath);

    CURLcode urlCode = CURLPrepare(srcWebDavPath.c_str());
    CSlistURL slist;
    slist.Append("Depth: infinity");
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    string dstParam = "Destination: ";
    dstParam += m_CURL.GetTopURL();
    dstParam += dstWebDavPath;
    slist.Append(dstParam.c_str());
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_CUSTOMREQUEST, "MOVE"));

    CHECK_CUCALL(urlCode, m_CURL.Perform());

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_CREATED, HTTP_STATUS_NO_CONTENT, errorInfo);
}


bool CWebDAV::Delete(const wchar_t *path, const ItemType /*type*/, wstring &errorInfo)
{
    const string webDavPath = EscapeUTF8URL(path);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    CSlistURL slist;
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    slist.Append("Content-Length: 0");
    slist.Append("Connection: Keep-Alive");
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_CUSTOMREQUEST, "DELETE"));

    CHECK_CUCALL(urlCode, m_CURL.Perform());

    if (urlCode != CURLE_OK)
    {
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return CheckResponseCode(HTTP_STATUS_NO_CONTENT, errorInfo);
}


bool CWebDAV::SendPropFindRequest(const wchar_t *dir, string &response, wstring &errInfo)
{
    const string webDavPath = EscapeUTF8URL(dir);
    DEBUG_PRINTF(L"NetBox: webDavPath = %s", ::MB2W(webDavPath.c_str()).c_str());

    response.clear();

    static const char *requestData =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<D:propfind xmlns:D=\"DAV:\">"
        "<D:prop xmlns:Z=\"urn:schemas-microsoft-com:\">"
        "<D:resourcetype/>"
        "<D:getcontentlength/>"
        "<D:creationdate/>"
        "<D:getlastmodified/>"
        //"<Z:Win32LastAccessTime/>"
        //"<Z:Win32FileAttributes/>"
        "</D:prop>"
        "</D:propfind>";

    static const size_t requestDataLen = strlen(requestData);

    CURLcode urlCode = CURLPrepare(webDavPath.c_str());
    CHECK_CUCALL(urlCode, m_CURL.SetOutput(response, &m_ProgressPercent));

    CSlistURL slist;
    slist.Append("Depth: 1");
    slist.Append("Content-Type: text/xml; charset=\"utf-8\"");
    char contentLength[64];
    sprintf_s(contentLength, "Content-Length: %d", requestDataLen);
    slist.Append(contentLength);
    slist.Append("Connection: Keep-Alive");

    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_CUSTOMREQUEST, "PROPFIND"));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_MAXREDIRS, 5));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDS, requestData));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_POSTFIELDSIZE, requestDataLen));

    CHECK_CUCALL(urlCode, m_CURL.Perform());
    // DEBUG_PRINTF(L"NetBox: urlCode = %d", urlCode);
    if (urlCode != CURLE_OK)
    {
        errInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    if (!CheckResponseCode(HTTP_STATUS_WEBDAV_MULTI_STATUS, errInfo))
    {
        // DEBUG_PRINTF(L"NetBox: errInfo = %s", errInfo.c_str());
        return false;
    }

    if (response.empty())
    {
        errInfo = L"Server return empty response";
        return false;
    }

    return true;
}


bool CWebDAV::CheckResponseCode(const long expect, wstring &errInfo)
{
    long responseCode = 0;
    if (curl_easy_getinfo(m_CURL, CURLINFO_RESPONSE_CODE, &responseCode) == CURLE_OK)
    {
        if (responseCode != expect)
        {
            errInfo = GetBadResponseInfo(responseCode);
            DEBUG_PRINTF(L"NetBox: errInfo = %s", errInfo.c_str());
            return false;
        }
    }
    return true;
}


bool CWebDAV::CheckResponseCode(const long expect1, const long expect2, wstring &errInfo)
{
    long responseCode = 0;
    if (curl_easy_getinfo(m_CURL, CURLINFO_RESPONSE_CODE, &responseCode) == CURLE_OK)
    {
        if (responseCode != expect1 && responseCode != expect2)
        {
            errInfo = GetBadResponseInfo(responseCode);
            return false;
        }
    }
    return true;
}


wstring CWebDAV::GetBadResponseInfo(const int code) const
{
    const wchar_t *descr = NULL;
    switch (code)
    {
    case HTTP_STATUS_CONTINUE           :
        descr = L"OK to continue with request";
        break;
    case HTTP_STATUS_SWITCH_PROTOCOLS   :
        descr = L"Server has switched protocols in upgrade header";
        break;
    case HTTP_STATUS_OK                 :
        descr = L"Request completed";
        break;
    case HTTP_STATUS_CREATED            :
        descr = L"Object created, reason = new URI";
        break;
    case HTTP_STATUS_ACCEPTED           :
        descr = L"Async completion (TBS)";
        break;
    case HTTP_STATUS_PARTIAL            :
        descr = L"Partial completion";
        break;
    case HTTP_STATUS_NO_CONTENT         :
        descr = L"No info to return";
        break;
    case HTTP_STATUS_RESET_CONTENT      :
        descr = L"Request completed, but clear form";
        break;
    case HTTP_STATUS_PARTIAL_CONTENT    :
        descr = L"Partial GET furfilled";
        break;
    case HTTP_STATUS_WEBDAV_MULTI_STATUS:
        descr = L"WebDAV Multi-Status";
        break;
    case HTTP_STATUS_AMBIGUOUS          :
        descr = L"Server couldn't decide what to return";
        break;
    case HTTP_STATUS_MOVED              :
        descr = L"Object permanently moved";
        break;
    case HTTP_STATUS_REDIRECT           :
        descr = L"Object temporarily moved";
        break;
    case HTTP_STATUS_REDIRECT_METHOD    :
        descr = L"Redirection w/ new access method";
        break;
    case HTTP_STATUS_NOT_MODIFIED       :
        descr = L"If-modified-since was not modified";
        break;
    case HTTP_STATUS_USE_PROXY          :
        descr = L"Redirection to proxy, location header specifies proxy to use";
        break;
    case HTTP_STATUS_REDIRECT_KEEP_VERB :
        descr = L"HTTP/1.1: keep same verb";
        break;
    case HTTP_STATUS_BAD_REQUEST        :
        descr = L"Invalid syntax";
        break;
    case HTTP_STATUS_DENIED             :
        descr = L"Access denied";
        break;
    case HTTP_STATUS_PAYMENT_REQ        :
        descr = L"Payment required";
        break;
    case HTTP_STATUS_FORBIDDEN          :
        descr = L"Request forbidden";
        break;
    case HTTP_STATUS_NOT_FOUND          :
        descr = L"Object not found";
        break;
    case HTTP_STATUS_BAD_METHOD         :
        descr = L"Method is not allowed";
        break;
    case HTTP_STATUS_NONE_ACCEPTABLE    :
        descr = L"No response acceptable to client found";
        break;
    case HTTP_STATUS_PROXY_AUTH_REQ     :
        descr = L"Proxy authentication required";
        break;
    case HTTP_STATUS_REQUEST_TIMEOUT    :
        descr = L"Server timed out waiting for request";
        break;
    case HTTP_STATUS_CONFLICT           :
        descr = L"User should resubmit with more info";
        break;
    case HTTP_STATUS_GONE               :
        descr = L"The resource is no longer available";
        break;
    case HTTP_STATUS_LENGTH_REQUIRED    :
        descr = L"The server refused to accept request w/o a length";
        break;
    case HTTP_STATUS_PRECOND_FAILED     :
        descr = L"Precondition given in request failed";
        break;
    case HTTP_STATUS_REQUEST_TOO_LARGE  :
        descr = L"Request entity was too large";
        break;
    case HTTP_STATUS_URI_TOO_LONG       :
        descr = L"Request URI too long";
        break;
    case HTTP_STATUS_UNSUPPORTED_MEDIA  :
        descr = L"Unsupported media type";
        break;
    case 416                            :
        descr = L"Requested Range Not Satisfiable";
        break;
    case 417                            :
        descr = L"Expectation Failed";
        break;
    case HTTP_STATUS_RETRY_WITH         :
        descr = L"Retry after doing the appropriate action";
        break;
    case HTTP_STATUS_SERVER_ERROR       :
        descr = L"Internal server error";
        break;
    case HTTP_STATUS_NOT_SUPPORTED      :
        descr = L"Required not supported";
        break;
    case HTTP_STATUS_BAD_GATEWAY        :
        descr = L"Error response received from gateway";
        break;
    case HTTP_STATUS_SERVICE_UNAVAIL    :
        descr = L"Temporarily overloaded";
        break;
    case HTTP_STATUS_GATEWAY_TIMEOUT    :
        descr = L"Timed out waiting for gateway";
        break;
    case HTTP_STATUS_VERSION_NOT_SUP    :
        descr = L"HTTP version not supported";
        break;
    }

    wstring errInfo = L"Incorrect response code: ";

    errInfo += ::NumberToWString(code);

    if (descr)
    {
        errInfo += L' ';
        errInfo += descr;
    }

    return errInfo;
}


string CWebDAV::GetNamespace(const TiXmlElement *element, const char *name, const char *defaultVal) const
{
    assert(element);
    assert(name);
    assert(defaultVal);

    string ns = defaultVal;
    const TiXmlAttribute *attr = element->FirstAttribute();
    while (attr)
    {
        if (strncmp(attr->Name(), "xmlns:", 6) == 0 && strcmp(attr->Value(), name) == 0)
        {
            ns = attr->Name();
            ns.erase(0, ns.find(':') + 1);
            ns += ':';
            break;
        }
        attr = attr->Next();
    }
    return ns;
}


FILETIME CWebDAV::ParseDateTime(const char *dt) const
{
    assert(dt);

    FILETIME ft;
    ZeroMemory(&ft, sizeof(ft));
    SYSTEMTIME st;
    ZeroMemory(&st, sizeof(st));

    if (WinHttpTimeToSystemTime(::MB2W(dt).c_str(), &st))
    {
        SystemTimeToFileTime(&st, &ft);
    }
    else if (strlen(dt) > 18)
    {
        //rfc 3339 date-time
        st.wYear =   static_cast<WORD>(atoi(dt +  0));
        st.wMonth =  static_cast<WORD>(atoi(dt +  5));
        st.wDay =    static_cast<WORD>(atoi(dt +  8));
        st.wHour =   static_cast<WORD>(atoi(dt + 11));
        st.wMinute = static_cast<WORD>(atoi(dt + 14));
        st.wSecond = static_cast<WORD>(atoi(dt + 17));
        SystemTimeToFileTime(&st, &ft);
    }

    return ft;
}


string CWebDAV::DecodeHex(const string &src) const
{
    const size_t cntLength = src.length();
    string result;
    result.reserve(cntLength);

    for (size_t i = 0; i < cntLength; ++i)
    {
        const char chkChar = src[i];
        if (chkChar != L'%' || (i + 2 >= cntLength) || !IsHexadecimal(src[i + 1]) || !IsHexadecimal(src[i + 2]))
        {
            result += chkChar;
        }
        else
        {
            const char ch1 = src[i + 1];
            const char ch2 = src[i + 2];
            const char encChar = (((ch1 & 0xf) + ((ch1 >= 'A') ? 9 : 0)) << 4) | ((ch2 & 0xf) + ((ch2 >= 'A') ? 9 : 0));
            result += encChar;
            i += 2;
        }
    }

    return result;
}


string CWebDAV::EscapeUTF8URL(const wchar_t *src) const
{
    assert(src && src[0] == L'/');

    string plainText = ::W2MB(src, CP_UTF8);
    const size_t cntLength = plainText.length();

    string result;
    result.reserve(cntLength);

    static const char permitSymbols[] = "/;@&=+$,-_.?!~'()#%{}^[]`";

    for (size_t i = 0; i < cntLength; ++i)
    {
        const char chkChar = plainText[i];
        if (*find(permitSymbols, permitSymbols + sizeof(permitSymbols), chkChar) ||
                (chkChar >= 'a' && chkChar <= 'z') ||
                (chkChar >= 'A' && chkChar <= 'Z') ||
                (chkChar >= '0' && chkChar <= '9'))
        {
            result += chkChar;
        }
        else
        {
            char encChar[4];
            sprintf_s(encChar, "%%%02X", static_cast<unsigned char>(chkChar));
            result += encChar;
        }
    }
    return result;
}

CURLcode CWebDAV::CURLPrepare(const char *webDavPath, const bool handleTimeout /*= true*/)
{
    CURLcode urlCode = m_CURL.Prepare(webDavPath, handleTimeout);
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_HTTPAUTH, CURLAUTH_ANY));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_FOLLOWLOCATION, 1));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_POST301, 1));

    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYPEER, 0L));
    CHECK_CUCALL(urlCode, curl_easy_setopt(m_CURL, CURLOPT_SSL_VERIFYHOST, 0L));
    return urlCode;
}
