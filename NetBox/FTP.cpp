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
#include "FTP.h"
#include "Strings.h"
#include <Wininet.h>

static const char *ParamCodePage = "CodePage";


UINT CSessionFTP::GetCodePage() const
{
    return static_cast<UINT>(GetPropertyNumeric(ParamCodePage, CP_UTF8));
}


void CSessionFTP::SetCodePage(const UINT cp)
{
    SetProperty(ParamCodePage, static_cast<__int64>(cp));
}


PSessionEditor CSessionFTP::CreateEditorInstance()
{
    return PSessionEditor(new CSessionEditorFTP(this));
}


PProtocol CSessionFTP::CreateClientInstance() const
{
    return PProtocol(new CFTP(this));
}


CSessionEditorFTP::CSessionEditorFTP(CSession *session) :
    CSessionEditor(session, 54, 22), m_IdCP(0)
{

}


void CSessionEditorFTP::OnPrepareDialog()
{
    m_IdSeparator = CreateSeparator(GetHeight() - 5);
    CreateCodePageControl(GetHeight() - 4, static_cast<CSessionFTP *>(m_Session)->GetCodePage(),
        m_IdCPText, m_IdCP);
}


void CSessionEditorFTP::OnSave()
{
    static_cast<CSessionFTP *>(m_Session)->SetCodePage(static_cast<UINT>(_wtoi(GetText(m_IdCP).c_str())));
}

void CSessionEditorFTP::ShowSessionDlgItems(bool visible)
{
    CSessionEditor::ShowSessionDlgItems(visible);
    ShowDlgItem(m_IdSeparator, visible);
    ShowDlgItem(m_IdCPText, visible);
    ShowDlgItem(m_IdCP, visible);
}

CFTP::CFTP(const CSession *session) :
    CProtocolBase(session),
    m_lastErrorCurlCode(CURLE_OK)
{
}


CFTP::~CFTP()
{
    Close();
}


bool CFTP::Connect(HANDLE abortEvent, wstring &errorInfo)
{
    assert(abortEvent);

    const wchar_t *url = m_Session.GetURL();
    // DEBUG_PRINTF(L"NetBox: CFTP::Connect: connecting to %s", url);
    m_CURL.Initialize(url, m_Session.GetUserName(), m_Session.GetPassword(),
        m_Session.GetProxySettings());
    m_CURL.SetAbortEvent(abortEvent);

    //Check initial path existing
    wstring path;
    ParseURL(url, NULL, NULL, NULL, &path, NULL, NULL, NULL);
    bool dirExist = false;
    if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
    {
        return false;
    }
    m_CurrentDirectory = path;
    while(m_CurrentDirectory.size() > 1 && m_CurrentDirectory[m_CurrentDirectory.length() - 1] == L'/')
    {
        m_CurrentDirectory.erase(m_CurrentDirectory.length() - 1);
    }
    // DEBUG_PRINTF(L"NetBox: CFTP::Connect: end");
    return true;
}


void CFTP::Close()
{
    m_CURL.Close();
}


CURLcode CFTP::CURLPrepare(const char *ftpPath, const bool handleTimeout /*= true*/)
{
    CURLcode urlCode = m_CURL.Prepare(ftpPath, handleTimeout);
    return urlCode;
}

bool CFTP::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo)
{
    assert(path && path[0] == L'/');
    isExist = false;

    string ftpPath = LocalToFtpCP(path);
    if (type == ItemDirectory && ftpPath[ftpPath.length() - 1] != '/')
    {
        ftpPath += '/';
    }
    // DEBUG_PRINTF(L"NetBox: CheckExisting: ftpPath = %s", ::MB2W(ftpPath.c_str()).c_str());

    isExist = true;

    CURLcode urlCode = CURLPrepare(ftpPath.c_str());
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    // DEBUG_PRINTF(L"NetBox: CheckExisting: urlCode = %d", urlCode);
    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        // DEBUG_PRINTF(L"NetBox: CheckExisting: 1: urlCode = %u, errorInfo = %s", urlCode, errorInfo.c_str());

        isExist = false;
    }

    return true;
}


bool CFTP::MakeDirectory(const wchar_t *path, wstring &errorInfo)
{
    assert(path && path[0] == L'/');

    const string ftpCommand = "MKD " + LocalToFtpCP(path);
    const CURLcode urlCode = m_CURL.ExecuteFtpCommand(ftpCommand.c_str());
    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }
    return true;
}


bool CFTP::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);
    // DEBUG_PRINTF(L"NetBox: GetList: m_CurrentDirectory = %s", m_CurrentDirectory.c_str());

    string ftpPath = LocalToFtpCP(m_CurrentDirectory.c_str(), true);
    ::AppendChar(ftpPath, '/');
    // DEBUG_PRINTF(L"NetBox: GetList: ftpPath = %s", ::MB2W(ftpPath.c_str()).c_str());

    CURLcode urlCode = CURLPrepare(ftpPath.c_str());
    string response;
    CHECK_CUCALL(urlCode, m_CURL.SetOutput(response, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, m_CURL.Perform());
    if (urlCode != CURLE_OK && urlCode != CURLE_REMOTE_FILE_NOT_FOUND)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    //Divide string to vector by EOL
    vector<FTPItem> ftpList;
    ftpList.reserve(100);
    static const char *delimiters = "\r\n";
    size_t lastPos = response.find_first_not_of(delimiters, 0);
    size_t nextPos = response.find_first_of(delimiters, lastPos);
    while (string::npos != nextPos || string::npos != lastPos)
    {
        const string singleString = response.substr(lastPos, nextPos - lastPos);
        if (_strnicmp(singleString.c_str(), "total ", 6))
        {
            FTPItem ftpItem;
            ParseFtpList(singleString.c_str(), ftpItem);
            // DEBUG_PRINTF(L"NetBox: ftpItem.Name = %s, FullText = %s", ftpItem.Name.c_str(), ftpItem.FullText.c_str());
            if (ftpItem.Name.empty())   //Parse error?
            {
                ftpItem.Name = L"***->" + ftpItem.FullText;
                ftpItem.Type = FTPItem::Undefined;
            }
            if (ftpItem.Type == FTPItem::Link)
            {
                bool dirExist = true;
                bool fileExist = true;
                wstring errDummy;
                wstring chkPath = ftpItem.LinkPath;
                if (CheckExisting(chkPath.c_str(), ItemDirectory, dirExist, errDummy) && dirExist)
                {
                    ftpItem.Type = FTPItem::Directory;
                }
                else if (CheckExisting(chkPath.c_str(), ItemFile, fileExist, errDummy) && fileExist)
                {
                    ftpItem.Type = FTPItem::File;
                }
                else
                {
                    ftpItem.Type = FTPItem::Undefined;
                }
            }
            if ((ftpItem.Type != FTPItem::Undefined) && (ftpItem.Name.compare(L".") != 0) &&
                (ftpItem.Name.compare(L"..") != 0))
            {
                ftpList.push_back(ftpItem);
            }
        }
        lastPos = response.find_first_not_of(delimiters, nextPos);
        nextPos = response.find_first_of(delimiters, lastPos);
    }

    *itemsNum = static_cast<int>(ftpList.size());
    if (*itemsNum)
    {
        *items = new PluginPanelItem[*itemsNum];
        ZeroMemory(*items, sizeof(PluginPanelItem) * (*itemsNum));
        for (int i = 0; i < *itemsNum; ++i)
        {
            PluginPanelItem &farItem = (*items)[i];
            const size_t nameSize = ftpList[i].Name.length() + 1;
            wchar_t *name = new wchar_t[nameSize];
            wcscpy_s(name, nameSize, ftpList[i].Name.c_str());
            farItem.FindData.lpwszFileName = name;
            switch (ftpList[i].Type)
            {
            case FTPItem::File:
                farItem.FindData.nFileSize = ftpList[i].Size;
                break;
            case FTPItem::Directory:
                farItem.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                break;
            case FTPItem::Link:
            {
                //Check link for file/dir
                bool dirExist = true;
                wstring errDummy;
                wstring chkPath = ftpList[i].LinkPath;
                if (CheckExisting(chkPath.c_str(), ItemDirectory, dirExist, errDummy) && dirExist)
                {
                    farItem.FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                }
                /* not supported yet - only for attribute view
                assert(!ftpList[i].LinkPath.empty());
                const size_t linkSize = ftpList[i].LinkPath.length() + 1;
                wchar_t* linkName = new wchar_t[linkSize];
                wcscpy_s(linkName, linkSize, ftpList[i].LinkPath.c_str());
                farItem.UserData = reinterpret_cast<DWORD_PTR>(linkName);
                farItem.FindData.nFileSize = 0;
                */
            }
            break;
            }
            farItem.FindData.ftLastWriteTime = ftpList[i].Modified;
        }
    }

    return true;
}


bool CFTP::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, wstring &errorInfo)
{
    assert(remotePath && remotePath[0] == L'/');
    assert(localPath && *localPath);
    // DEBUG_PRINTF(L"NetBox: GetFile: remotePath = %s, localPath = %s", remotePath, localPath);

    CFile outFile;
    if (!outFile.OpenWrite(localPath))
    {
        errorInfo = FormatErrorDescription(outFile.LastError());
        return false;
    }

    const string ftpFileName = LocalToFtpCP(remotePath);
    CURLcode urlCode = CURLPrepare(ftpFileName.c_str(), false);
    CHECK_CUCALL(urlCode, m_CURL.SetOutput(&outFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    outFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return true;
}


bool CFTP::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 /*fileSize*/, wstring &errorInfo)
{
    assert(remotePath && remotePath[0] == L'/');
    assert(localPath && *localPath);
    // DEBUG_PRINTF(L"NetBox: PutFile: remotePath = %s, localPath = %s", remotePath, localPath);

    CFile inFile;
    if (!inFile.OpenRead(localPath))
    {
        errorInfo = FormatErrorDescription(inFile.LastError());
        return false;
    }

    const string ftpFileName = LocalToFtpCP(remotePath, true);
    CURLcode urlCode = CURLPrepare(ftpFileName.c_str(), false);
    CHECK_CUCALL(urlCode, m_CURL.SetInput(&inFile, &m_ProgressPercent));
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    inFile.Close();
    m_ProgressPercent = -1;

    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return true;
}


bool CFTP::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, wstring &errorInfo)
{
    assert(srcPath && srcPath[0] == L'/');
    assert(dstPath && dstPath[0] == L'/');

    const string cmd1 = "RNFR " + LocalToFtpCP(srcPath);
    const string cmd2 = "RNTO " + LocalToFtpCP(dstPath);
    DEBUG_PRINTF(L"NetBox: Rename: srcPath = %s, dstPath = %s, type = %u", srcPath, dstPath, type);
    CSlistURL slist;
    slist.Append(cmd1.c_str());
    slist.Append(cmd2.c_str());

    CURLcode urlCode = CURLPrepare(NULL);
    CHECK_CUCALL(urlCode, m_CURL.SetSlist(slist));
    CHECK_CUCALL(urlCode, m_CURL.Perform());

    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }

    return true;
}


bool CFTP::Delete(const wchar_t *path, const ItemType type, wstring &errorInfo)
{
    assert(path && path[0] == L'/');

    const string ftpCommand = (type == ItemDirectory ? "RMD " : "DELE ") + LocalToFtpCP(path);
    // DEBUG_PRINTF(L"NetBox: Delete: ftpCommand = %s", ::MB2W(ftpCommand.c_str()).c_str());
    const CURLcode urlCode = m_CURL.ExecuteFtpCommand(ftpCommand.c_str());
    if (urlCode != CURLE_OK)
    {
        m_lastErrorCurlCode = urlCode;
        errorInfo = ::MB2W(curl_easy_strerror(urlCode));
        return false;
    }
    return true;
}

WORD CFTP::GetMonth(const char *name) const
{
    static const char *months[] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
    for (WORD i = 0; i < sizeof(months) / sizeof(months[0]); ++i)
    {
        if (_strnicmp(months[i], name, 3) == 0)
        {
            return ++i;
        }
    }
    return 0;
}


unsigned short CFTP::ParsePermission(const char *perm) const
{
    assert(perm);

    //TODO:
//  r (read permission)
//  w (write permission)
//  x (execute permision)
//  X (conditional execute permision)
//  u (current permissions for user)
//  g (current permissions for group)
//  o (current permissions for others)
//  s (set uid or set gid)
//  t (sticky bit)

    unsigned short ret = 0;
    if (perm[0] == 'r' || perm[0] == 'R')
    {
        ret |= (1 << 10);
    }
    if (perm[1] == 'w' || perm[1] == 'W')
    {
        ret |= (1 << 9);
    }
    if (perm[2] == 'x' || perm[2] == 'X')
    {
        ret |= (1 << 8);
    }
    if (perm[3] == 'r' || perm[3] == 'R')
    {
        ret |= (1 << 6);
    }
    if (perm[4] == 'w' || perm[4] == 'W')
    {
        ret |= (1 << 5);
    }
    if (perm[5] == 'x' || perm[5] == 'X')
    {
        ret |= (1 << 4);
    }
    if (perm[6] == 'r' || perm[6] == 'R')
    {
        ret |= (1 << 2);
    }
    if (perm[7] == 'w' || perm[7] == 'W')
    {
        ret |= (1 << 1);
    }
    if (perm[8] == 'x' || perm[8] == 'X')
    {
        ret |= (1 << 0);
    }
    return ret;
}


bool CFTP::ParseFtpList(const char *text, FTPItem &item) const
{
    assert(text);

    SYSTEMTIME st;
    ZeroMemory(&st, sizeof(st));
    char typeSymb, permission[32];
    char msdosStr[16];
    char owner[32], group[32];
    unsigned __int64 size = 0;
    char name[128];
    char monthName[4], dayPart[3];
    int minutes, hours, day, month, year;

    bool parseSuccess = true;

    item.FullText = FtpToLocalCP(text);

    //*************************** UNIX style *****************************
    //-rw-r--r--   1 root     other        531 Jan 29 03:26 README
    //dr-xr-xr-x   2 root     other        512 Apr  8  1994 etc
    //lrwxrwxrwx   1 root     other          7 Jan 25 00:17 bin -> usr/bin
    //************** Microsoft's FTP servers for Windows *****************
    //----------   1 owner    group         1803128 Jul 10 10:18 ls-lR.Z
    //d---------   1 owner    group               0 May  9 19:45 Softlib
    if (sscanf_s(text, "%c%s %*d %s %s %I64d %s %d %d:%d %[^\n]", &typeSymb, sizeof(typeSymb), permission, sizeof(permission), owner, sizeof(owner), group, sizeof(group), &size, monthName, sizeof(monthName), &day, &hours, &minutes, name, sizeof(name)) == 10)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 1");
        st.wMonth =  GetMonth(monthName);
        st.wDay =    static_cast<WORD>(day);
        st.wHour =   static_cast<WORD>(hours);
        st.wMinute = static_cast<WORD>(minutes);
        item.Owner = FtpToLocalCP(owner);
        item.Group = FtpToLocalCP(group);
        item.Name = FtpToLocalCP(name);
        // DEBUG_PRINTF(L"NetBox: typeSymb = %c", typeSymb);
        if (typeSymb == 'l')
        {
            item.Type = FTPItem::Link;
            const size_t linkDelim = item.Name.find(L" -> ");
            if (linkDelim != string::npos)
            {
                item.LinkPath = item.Name.substr(linkDelim + 4);
                item.Name.erase(linkDelim);
            }
        }
        else if (typeSymb == 'd')
        {
            item.Type = FTPItem::Directory;
        }
        else
        {
            item.Type = FTPItem::File;
        }
        item.Size = static_cast<unsigned __int64>(size);
        item.Permission = ParsePermission(permission);
    }
    else if (sscanf_s(text, "%c%s %*d %s %s %I64d %s %d %d %[^\n]", &typeSymb, sizeof(typeSymb), permission, sizeof(permission), owner, sizeof(owner), group, sizeof(group), &size, monthName, sizeof(monthName), &day, &year, name, sizeof(name)) == 9)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 2");
        st.wMonth = GetMonth(monthName);
        st.wDay = static_cast<WORD>(day);
        st.wYear = static_cast<WORD>(year);
        item.Owner = FtpToLocalCP(owner);
        item.Group = FtpToLocalCP(group);
        item.Name = FtpToLocalCP(name);
        if (typeSymb == 'l')
        {
            item.Type = FTPItem::Link;
            const size_t linkDelim = item.Name.find(L" -> ");
            if (linkDelim != string::npos)
            {
                item.LinkPath = item.Name.substr(linkDelim + 4);
                item.Name.erase(linkDelim);
            }
        }
        else if (typeSymb == 'd')
        {
            item.Type = FTPItem::Directory;
        }
        else
        {
            item.Type = FTPItem::File;
        }
        item.Size = static_cast<unsigned __int64>(size);
        item.Permission = ParsePermission(permission);
    }
    //****************************** MSDOS *******************************
    //07-18-00  10:16AM       <DIR>          pub
    //04-14-00  03:47PM                  589 readme.htm
    else if (sscanf_s(text, "%d-%d-%d %d:%d%2s %s %[^\n]", &month, &day, &year, &hours, &minutes, dayPart, sizeof(dayPart), msdosStr, sizeof(msdosStr), name, sizeof(name)) == 8)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 3");
        st.wDay =    static_cast<WORD>(day);
        st.wMonth =  static_cast<WORD>(month);
        st.wYear =   static_cast<WORD>(year);
        if (st.wYear < 1900)
        {
            st.wYear += (year > 70 ? 1900 : 2000);
        }
        if (_strcmpi(dayPart, "pm") == 0)
        {
            st.wHour += 12;
        }
        st.wMinute = static_cast<WORD>(minutes);

        item.Name = FtpToLocalCP(name);
        if (strcmp(msdosStr, "<DIR>") == 0)
        {
            item.Type = FTPItem::Directory;
        }
        else
        {
            item.Type = FTPItem::File;
            item.Size = static_cast<unsigned __int64>(_atoi64(msdosStr));
        }
    }
    //***************************** MultiNet *****************************
    //00README.TXT;1      2 30-DEC-1996 17:44 [SYSTEM] (RWED,RWED,RE,RE)
    //CORE.DIR;1          1  8-SEP-1996 16:09 [SYSTEM] (RWE,RWE,RE,RE)
    //CII-MANUAL.TEX;1  213/216  29-JAN-1996 03:33:12  [ANONYMOU,ANONYMOUS]   (RWED,RWED,,)
    else if (sscanf_s(text, "%s %*s %d-%3s-%d %d:%d", name, sizeof(name), &day, monthName, sizeof(monthName), &year, &hours, &minutes) == 6)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 4");
        st.wMonth = GetMonth(monthName);
        st.wDay =   static_cast<WORD>(day);
        st.wYear =  static_cast<WORD>(year);
        st.wHour =   static_cast<WORD>(hours);
        st.wMinute = static_cast<WORD>(minutes);
        item.Name = FtpToLocalCP(name);
        const size_t delim = item.Name.find(';');
        if (delim != string::npos)
        {
            item.Name.erase(delim);
        }
        item.Type = FTPItem::File;  //?
    }
    //****************************** NetWare *****************************
    //d [R----F--] supervisor            512       Jan 16 18:53    login
    //- [R----F--] rhesus             214059       Oct 20 15:27    cx.exe
    else if (sscanf_s(text, "%c %*s %s %I64d %s %d %d:%d %[^\n]", &typeSymb, sizeof(typeSymb), owner, sizeof(owner), &size,  monthName, sizeof(monthName), &day, &hours, &minutes, name, sizeof(name)) == 8)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 5");
        st.wMonth = GetMonth(monthName);
        st.wDay =   static_cast<WORD>(day);
        st.wHour =   static_cast<WORD>(hours);
        st.wMinute = static_cast<WORD>(minutes);
        item.Name = FtpToLocalCP(name);
        const size_t delim = item.Name.find(';');
        if (delim != string::npos)
        {
            item.Name.erase(delim);
        }
        item.Type = (typeSymb == 'd' ? FTPItem::Directory : FTPItem::File);
        item.Size = static_cast<unsigned __int64>(size);
    }
    //*********************** NetPresenz for the Mac *********************
    //-------r--         326  1391972  1392298 Nov 22  1995 MegaPhone.sit
    //drwxrwxr-x               folder        2 May 10  1996 network
    else if (sscanf_s(text, "%c%s %I64d %*s %*s %s %d %d %[^\n]", &typeSymb, sizeof(typeSymb), permission, sizeof(permission), &size, monthName, sizeof(monthName), &day, &year, name, sizeof(name)) == 7)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 6");
        st.wMonth = GetMonth(monthName);
        st.wDay =   static_cast<WORD>(day);
        st.wYear =  static_cast<WORD>(year);
        item.Name = FtpToLocalCP(name);
        item.Type = (typeSymb == 'd' ? FTPItem::Directory : FTPItem::File);
        item.Size = static_cast<unsigned __int64>(size);
        item.Permission = ParsePermission(permission);
    }
    else if (sscanf_s(text, "%c%s folder %*s %s %d %d %[^\n]", &typeSymb, sizeof(typeSymb), permission, sizeof(permission), monthName, sizeof(monthName), &day, &year, name, sizeof(name)) == 6)
    {
        // DEBUG_PRINTF(L"NetBox: ParseFtpList: 7");
        st.wMonth = GetMonth(monthName);
        st.wDay =   static_cast<WORD>(day);
        st.wYear =  static_cast<WORD>(year);
        item.Name = FtpToLocalCP(name);
        item.Type = FTPItem::Directory;
        item.Size = static_cast<unsigned __int64>(size);
        item.Permission = ParsePermission(permission);
    }
    else
    {
        parseSuccess = false;
    }


    if (parseSuccess && st.wYear == 0)
    {
        SYSTEMTIME now;
        GetSystemTime(&now);
        st.wYear = now.wYear;
        if (st.wMonth > now.wMonth || (st.wMonth == now.wMonth && st.wDay > now.wDay))
        {
            --st.wYear;
        }
    }

    if (parseSuccess)
    {
        SystemTimeToFileTime(&st, &item.Modified);
        LocalFileTimeToFileTime(&item.Modified, &item.Modified);
    }

    return parseSuccess;

//  const char* test[] = {
//      "-rw-r--r--   1 root     other        531 Jan 29 03:26 README",
//      "dr-xr-xr-x   2 root     other        512 Apr  8  1994 etc",
//      "lrwxrwxrwx   1 root     other          7 Jan 25 00:17 bin -> usr/bin",
//      "----------   1 owner    group         1803128 Jul 10 10:18 ls-lR.Z",
//      "d---------   1 owner    group               0 May  9 19:45 Softlib",
//      "07-18-00  10:16AM       <DIR>          pub",
//      "04-14-00  03:47PM                  589 readme.htm",
//      "00README.TXT;1      2 30-DEC-1996 17:44 [SYSTEM] (RWED,RWED,RE,RE)",
//      "CORE.DIR;1          1  8-SEP-1996 16:09 [SYSTEM] (RWE,RWE,RE,RE)",
//      "CII-MANUAL.TEX;1  213/216  29-JAN-1996 03:33:12  [ANONYMOU,ANONYMOUS]   (RWED,RWED,,)",
//      "d [R----F--] supervisor            512       Jan 16 18:53    login",
//      "- [R----F--] rhesus             214059       Oct 20 15:27    cx.exe",
//      "-------r--         326  1391972  1392298 Nov 22  1995 MegaPhone.sit",
//      "drwxrwxr-x               folder        2 May 10  1996 network"
//  };
//  for (size_t i = 0; i < sizeof(test) / sizeof(test[0]); ++i) {
//      FTPItem out;
//      const bool ret = ParseFtpList(test[i], out);
//  }
}
