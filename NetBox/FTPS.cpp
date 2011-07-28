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

/**
 * libssh2 file handle wrapper
 */
class CFTPSFileHandle
{
public:
    CFTPSFileHandle(LIBSSH2_SFTP_HANDLE *h)
    {
        _Object = h;
    }

    CFTPSFileHandle(LIBSSH2_SFTP *sftp, const char *path, const unsigned long flags, const long mode, const int type)
    {
        assert(sftp);
        assert(path && path[0] == L'/');

        _Object = libssh2_sftp_open_ex(sftp, path, static_cast<unsigned int>(strlen(path)), flags, mode, type);
    }

    ~CFTPSFileHandle()
    {
        if (_Object)
        {
            libssh2_sftp_close_handle(_Object);
        }
    }

    inline operator LIBSSH2_SFTP_HANDLE *() const
    {
        return _Object;
    }
    inline LIBSSH2_SFTP_HANDLE *operator->() const
    {
        return _Object;
    };

protected:
    LIBSSH2_SFTP_HANDLE *_Object;
};


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
/*    assert(_FTPSSession);

    const string sftpPath = LocalToSftpCP(path);
    retStatus = (libssh2_sftp_mkdir_ex(_FTPSSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length()), LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH) == LIBSSH2_ERROR_NONE);
    if (!retStatus)
    {
        // errorInfo = FormatSSHLastErrorDescription();
    }
 */
    return retStatus;
}


bool CFTPS::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);
/*     assert(_FTPSSession);

    CFTPSFileHandle dirHandle(_FTPSSession, LocalToSftpCP(m_CurrentDirectory.c_str()).c_str(), 0, 0, LIBSSH2_SFTP_OPENDIR);
    if (!dirHandle)
    {
        // errorInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //!FTPS item description
    struct FTPSItem
    {
        FTPSItem() : Attributes(0), Size(0)
        {
            Modified.dwLowDateTime = Modified.dwHighDateTime = LastAccess.dwLowDateTime = LastAccess.dwHighDateTime =0;
        }
        wstring             Name;
        DWORD               Attributes;
        FILETIME            Modified;
        FILETIME            LastAccess;
        unsigned __int64    Size;
    };
    vector<FTPSItem> sftpItems;

    //Read directory content
    char fileName[512];
    LIBSSH2_SFTP_ATTRIBUTES sftpAttrs;
    while (libssh2_sftp_readdir_ex(dirHandle, fileName, sizeof(fileName), NULL, 0, &sftpAttrs) > 0)
    {
        if (LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions) && (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0))
        {
            continue;
        }
        FTPSItem sftpItem;
        sftpItem.Name = SftpToLocalCP(fileName);
        if (LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions))
        {
            sftpItem.Attributes = FILE_ATTRIBUTE_DIRECTORY;
        }
        else if (sftpAttrs.flags & LIBSSH2_SFTP_ATTR_SIZE)
        {
            sftpItem.Size = sftpAttrs.filesize;
        }
        if (sftpAttrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)
        {
            sftpItem.Modified = UnixTimeToFileTime(sftpAttrs.mtime);
            sftpItem.LastAccess = UnixTimeToFileTime(sftpAttrs.atime);
        }
        sftpItems.push_back(sftpItem);
    }


    *itemsNum = static_cast<int>(sftpItems.size());
    if (*itemsNum)
    {
        *items = new PluginPanelItem[*itemsNum];
        ZeroMemory(*items, sizeof(PluginPanelItem) * (*itemsNum));
        for (int i = 0; i < *itemsNum; ++i)
        {
            PluginPanelItem &farItem = (*items)[i];
            const size_t nameSize = sftpItems[i].Name.length() + 1;
            wchar_t *name = new wchar_t[nameSize];
            wcscpy_s(name, nameSize, sftpItems[i].Name.c_str());
            farItem.FindData.lpwszFileName = name;
            farItem.FindData.dwFileAttributes = sftpItems[i].Attributes;
            farItem.FindData.nFileSize = sftpItems[i].Size;
            farItem.FindData.ftLastWriteTime = sftpItems[i].Modified;
            farItem.FindData.ftLastAccessTime = sftpItems[i].LastAccess;
        }
    } */

    return true;
}


bool CFTPS::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
/*     assert(_FTPSSession);

    //Open source file
    CFTPSFileHandle sftpFile(_FTPSSession, LocalToSftpCP(remotePath).c_str(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
    if (!sftpFile)
    {
        // errorInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //Open destination file
    CFile outFile;
    if (!outFile.OpenWrite(localPath))
    {
        errorInfo = FormatErrorDescription(outFile.LastError());
        return false;
    }

    unsigned __int64 readBytes = 0;
    if (fileSize)
    {
        _ProgressPercent = 0;
    }

    vector<char> buff(4096);
    ssize_t rc;
    while ((rc = libssh2_sftp_read(sftpFile, &buff[0], buff.size())) > 0)
    {
        if (!outFile.Write(&buff[0], static_cast<size_t>(rc)))
        {
            errorInfo = FormatErrorDescription(outFile.LastError());
            return false;
        }
        readBytes += static_cast<unsigned __int64>(rc);
        if (fileSize)
        {
            _ProgressPercent = static_cast<int>((readBytes * 100) / fileSize);
        }

        if (WaitForSingleObject(_AbortEvent, 0) == WAIT_OBJECT_0)
        {
            return false;
        }
    }

    outFile.Close();
    _ProgressPercent = -1; */

    return true;
}


bool CFTPS::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
/*     assert(_FTPSSession);

    //Destination (FTPS) file
    CFTPSFileHandle sftpFile(_FTPSSession, LocalToSftpCP(remotePath).c_str(),
                             LIBSSH2_FXF_WRITE |  LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                             LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IWGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IWOTH,
                             LIBSSH2_SFTP_OPENFILE);
    if (!sftpFile)
    {
        // errorInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //Open source file
    CFile inFile;
    if (!inFile.OpenRead(localPath))
    {
        errorInfo = FormatErrorDescription(inFile.LastError());
        return false;
    }

    unsigned __int64 writeBytes = 0;
    if (fileSize)
    {
        _ProgressPercent = 0;
    }

    vector<char> buff(4096);
    size_t readSize = buff.size();
    while (readSize)
    {
        if (!inFile.Read(&buff[0], readSize))
        {
            errorInfo = FormatErrorDescription(inFile.LastError());
            return false;
        }
        writeBytes += static_cast<unsigned __int64>(readSize);
        if (fileSize)
        {
            _ProgressPercent = static_cast<int>((writeBytes * 100) / fileSize);
        }
        size_t sent = 0;
        while (sent < readSize)
        {
            ssize_t res = libssh2_sftp_write(sftpFile, &buff[sent], readSize - sent);
            if (res < 0)
            {
                // errorInfo = FormatSSHLastErrorDescription();
                return false;
            }
            sent += res;
        }
        if (WaitForSingleObject(_AbortEvent, 0) == WAIT_OBJECT_0)
        {
            return false;
        }
    }

    inFile.Close();
    _ProgressPercent = -1; */

    return true;
}


bool CFTPS::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, wstring &errorInfo)
{
    bool retStatus = false;
/*     assert(_FTPSSession);

    const string srcSftpPath = LocalToSftpCP(srcPath);
    const string dstSftpPath = LocalToSftpCP(dstPath);

    retStatus = (libssh2_sftp_rename_ex(_FTPSSession, srcSftpPath.c_str(),
                            static_cast<unsigned int>(srcSftpPath.length()), dstSftpPath.c_str(),
                            static_cast<unsigned int>(dstSftpPath.length()),
                            LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE
                                                  ) == LIBSSH2_ERROR_NONE);

    if (!retStatus)
    {
        // errorInfo = FormatSSHLastErrorDescription();
    }
 */
    return retStatus;
}


bool CFTPS::Delete(const wchar_t *path, const ItemType type, wstring &errorInfo)
{
    bool retStatus = false;
/*     assert(_FTPSSession);


    const string sftpPath = LocalToSftpCP(path);

    if (type == ItemDirectory)
    {
        retStatus = (libssh2_sftp_rmdir_ex(_FTPSSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }
    else
    {
        retStatus = (libssh2_sftp_unlink_ex(_FTPSSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }

    if (!retStatus)
    {
        // errorInfo = FormatSSHLastErrorDescription();
    } */

    return retStatus;
}

