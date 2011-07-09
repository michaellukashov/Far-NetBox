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
#include "SFTP.h"
#include "Strings.h"

static const char *ParamCodePage = "CodePage";
static const char *ParamKeyFile =  "KeyFile";


UINT CSessionSFTP::GetCodePage() const
{
    return static_cast<UINT>(GetPropertyNumeric(ParamCodePage, CP_UTF8));
}


void CSessionSFTP::SetCodePage(const UINT cp)
{
    SetProperty(ParamCodePage, static_cast<__int64>(cp));
}


const wchar_t *CSessionSFTP::GetKeyFile() const
{
    return GetProperty(ParamKeyFile, L"");
}


void CSessionSFTP::SetKeyFile(const wchar_t *fileName)
{
    assert(fileName);
    SetProperty(ParamKeyFile, fileName);
}


PSessionEditor CSessionSFTP::CreateEditorInstance()
{
    return PSessionEditor(new CSessionEditorSFTP(this));
}


PProtocol CSessionSFTP::CreateClientInstance() const
{
    return PProtocol(new CSFTP(this));
}


CSessionEditorSFTP::CSessionEditorSFTP(CSession *session)
    : CSessionEditor(session, 52, 23), _IdKeyFile(0)
{
}


void CSessionEditorSFTP::OnPrepareDialog()
{
    CreateText(GetLeft(), GetTop() + 12, CFarPlugin::GetString(StringEdAuthCert));
    _IdKeyFile = CreateEdit(GetLeft(), GetTop() + 13, MAX_SIZE, static_cast<CSessionSFTP *>(_Session)->GetKeyFile());

    CreateSeparator(GetTop() + 14);
    _IdCP = CreateCodePageControl(GetTop() + 15, static_cast<CSessionSFTP *>(_Session)->GetCodePage());
}


void CSessionEditorSFTP::OnSave()
{
    static_cast<CSessionSFTP *>(_Session)->SetKeyFile(GetText(_IdKeyFile).c_str());
    static_cast<CSessionSFTP *>(_Session)->SetCodePage(static_cast<UINT>(_wtoi(GetText(_IdCP).c_str())));
}


/**
 * libssh2 file handle wrapper
 */
class CSFTPFileHandle
{
public:
    CSFTPFileHandle(LIBSSH2_SFTP_HANDLE *h)
    {
        _Object = h;
    }

    CSFTPFileHandle(LIBSSH2_SFTP *sftp, const char *path, const unsigned long flags, const long mode, const int type)
    {
        assert(sftp);
        assert(path && path[0] == L'/');

        _Object = libssh2_sftp_open_ex(sftp, path, static_cast<unsigned int>(strlen(path)), flags, mode, type);
    }

    ~CSFTPFileHandle()
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


CSFTP::CSFTP(const CSession *session)
    : CProtocolBase(session), _Socket(INVALID_SOCKET), _SSHSession(NULL), _SFTPSession(NULL), _AbortEvent(NULL)
{
}


CSFTP::~CSFTP()
{
    Close();
}


bool CSFTP::Connect(HANDLE abortEvent, wstring &errorInfo)
{
    assert(abortEvent);
    assert(_Socket == INVALID_SOCKET);

    _AbortEvent = abortEvent;

    wstring hostName;
    wstring path;
    unsigned short port = 0;
    ParseURL(_Session.GetURL(), NULL, &hostName, &port, &path, NULL, NULL, NULL);

    if (OpenSSHSession(hostName.c_str(), port, errorInfo))
    {
        bool aborted = false;
        libssh2_session_set_blocking(_SSHSession, 0);
        while (!(_SFTPSession = libssh2_sftp_init(_SSHSession)))
        {
            int last_errno = libssh2_session_last_errno(_SSHSession);
            if (last_errno != LIBSSH2SFTP_EAGAIN)
            {
                // DEBUG_PRINTF(L"CSFTP::Connect: libssh2_sftp_init failed: %d", last_errno);
                break;
            }
            CFarPlugin::CheckAbortEvent(&_AbortEvent);
            if (WaitForSingleObject(_AbortEvent, 0) == WAIT_OBJECT_0)
            {
                aborted = true;
                break;
            }
            Sleep(100);
        }
        if (_SFTPSession != NULL)
        {
            libssh2_session_set_blocking(_SSHSession, 1);
        }
        else if (aborted)
        {
            errorInfo = CFarPlugin::GetString(StringOperationCanceledByUser);
        }
        else
        {
            errorInfo = FormatSSHLastErrorDescription();
        }
    }
    // DEBUG_PRINTF(L"CSFTP::Connect: after OpenSSHSession 2: errorInfo = %s", errorInfo.c_str());

    if (_SFTPSession == NULL)
    {
        Close();
    }
    else
    {

        if (path.compare(L"/") != 0)
        {
            //Check initial path existing
            bool dirExist = false;
            if (!CheckExisting(path.c_str(), ItemDirectory, dirExist, errorInfo) || !dirExist)
            {
                return false;
            }
            _CurrentDirectory = path;
        }
        else
        {
            char realPath[256];
            int res = libssh2_sftp_symlink_ex(_SFTPSession, NULL, 0, realPath, static_cast<unsigned int>(sizeof(realPath)), LIBSSH2_SFTP_REALPATH);
            _CurrentDirectory = res >= 0 ? CFarPlugin::MB2W(realPath, CP_UTF8) : L"/";
        }
    }

    while(_CurrentDirectory.size() > 1 && _CurrentDirectory[_CurrentDirectory.length() - 1] == L'/')
    {
        _CurrentDirectory.erase(_CurrentDirectory.length() - 1);
    }

    return (_SFTPSession != NULL);
}


void CSFTP::Close()
{
    if (_SFTPSession)
    {
        libssh2_sftp_shutdown(_SFTPSession);
        _SFTPSession = NULL;
    }

    if (_SSHSession)
    {
        libssh2_session_disconnect(_SSHSession, "Normal shutdown");
        libssh2_session_free(_SSHSession);
        _SSHSession = NULL;
    }

    if (_Socket != INVALID_SOCKET)
    {
        shutdown(_Socket, SD_BOTH);
        closesocket(_Socket);
        _Socket = INVALID_SOCKET;
    }
}


bool CSFTP::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo)
{
    assert(type == ItemDirectory);
    assert(_SFTPSession);

    CSFTPFileHandle dirHandle(_SFTPSession, LocalToSftpCP(path).c_str(), 0, 0, LIBSSH2_SFTP_OPENDIR);
    if (!dirHandle)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }
    isExist = (dirHandle != NULL);
    return true;
}


bool CSFTP::MakeDirectory(const wchar_t *path, wstring &errorInfo)
{
    assert(_SFTPSession);

    const string sftpPath = LocalToSftpCP(path);
    const bool retStatus = (libssh2_sftp_mkdir_ex(_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length()), LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH) == LIBSSH2_ERROR_NONE);
    if (!retStatus)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }
    return retStatus;
}


bool CSFTP::GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);
    assert(_SFTPSession);

    CSFTPFileHandle dirHandle(_SFTPSession, LocalToSftpCP(_CurrentDirectory.c_str()).c_str(), 0, 0, LIBSSH2_SFTP_OPENDIR);
    if (!dirHandle)
    {
        errorInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //!SFTP item description
    struct SFTPItem
    {
        SFTPItem() : Attributes(0), Size(0)
        {
            Modified.dwLowDateTime = Modified.dwHighDateTime = LastAccess.dwLowDateTime = LastAccess.dwHighDateTime =0;
        }
        wstring             Name;
        DWORD               Attributes;
        FILETIME            Modified;
        FILETIME            LastAccess;
        unsigned __int64    Size;
    };
    vector<SFTPItem> sftpItems;

    //Read directory content
    char fileName[512];
    LIBSSH2_SFTP_ATTRIBUTES sftpAttrs;
    while (libssh2_sftp_readdir_ex(dirHandle, fileName, sizeof(fileName), NULL, 0, &sftpAttrs) > 0)
    {
        if (LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions) && (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0))
        {
            continue;
        }
        SFTPItem sftpItem;
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
    }

    return true;
}


bool CSFTP::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
    assert(_SFTPSession);

    //Open source file
    CSFTPFileHandle sftpFile(_SFTPSession, LocalToSftpCP(remotePath).c_str(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
    if (!sftpFile)
    {
        errorInfo = FormatSSHLastErrorDescription();
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
    _ProgressPercent = -1;

    return true;
}


bool CSFTP::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo)
{
    assert(localPath && *localPath);
    assert(_SFTPSession);

    //Destination (SFTP) file
    CSFTPFileHandle sftpFile(_SFTPSession, LocalToSftpCP(remotePath).c_str(),
                             LIBSSH2_FXF_WRITE |  LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                             LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IWGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IWOTH,
                             LIBSSH2_SFTP_OPENFILE);
    if (!sftpFile)
    {
        errorInfo = FormatSSHLastErrorDescription();
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
                errorInfo = FormatSSHLastErrorDescription();
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
    _ProgressPercent = -1;

    return true;
}


bool CSFTP::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, wstring &errorInfo)
{
    assert(_SFTPSession);

    const string srcSftpPath = LocalToSftpCP(srcPath);
    const string dstSftpPath = LocalToSftpCP(dstPath);

    const bool retStatus = (libssh2_sftp_rename_ex(_SFTPSession, srcSftpPath.c_str(),
                            static_cast<unsigned int>(srcSftpPath.length()), dstSftpPath.c_str(),
                            static_cast<unsigned int>(dstSftpPath.length()),
                            LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE
                                                  ) == LIBSSH2_ERROR_NONE);

    if (!retStatus)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }

    return retStatus;
}


bool CSFTP::Delete(const wchar_t *path, const ItemType type, wstring &errorInfo)
{
    assert(_SFTPSession);

    bool retStatus = false;

    const string sftpPath = LocalToSftpCP(path);

    if (type == ItemDirectory)
    {
        retStatus = (libssh2_sftp_rmdir_ex(_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }
    else
    {
        retStatus = (libssh2_sftp_unlink_ex(_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }

    if (!retStatus)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }

    return retStatus;
}


bool CSFTP::OpenSSHSession(const wchar_t *hostName, const unsigned short port, wstring &errInfo)
{
    assert(_Socket == INVALID_SOCKET);

    //Check private key file existing (libssh2 doesn't do it)
    const wchar_t *keyFileName = _Session.GetKeyFile();
    if (*keyFileName == 0)
    {
        keyFileName = NULL;
    }
    if (keyFileName && GetFileAttributes(keyFileName) == INVALID_FILE_ATTRIBUTES)
    {
        errInfo = FormatErrorDescription(GetLastError(), CFarPlugin::GetFormattedString(StringErrKeyFile, keyFileName).c_str());
        return false;
    }

    const hostent *remoteHost = gethostbyname(CFarPlugin::W2MB(hostName).c_str());
    if (!remoteHost)
    {
        errInfo = FormatErrorDescription(WSAGetLastError());
        return false;
    }
    const unsigned long address = *reinterpret_cast<unsigned long *>(remoteHost->h_addr_list[0]);

    sockaddr_in sockAddr;
    ZeroMemory(&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port ? port : 22);
    sockAddr.sin_addr.s_addr = address;

    //Establish network connection
    _Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (_Socket == INVALID_SOCKET || connect(_Socket, reinterpret_cast<sockaddr *>(&sockAddr), sizeof(sockAddr)) != 0)
    {
        errInfo = FormatErrorDescription(WSAGetLastError());
        return false;
    }

    //Open SSH2 session
    _SSHSession = libssh2_session_init();
    if (!_SSHSession)
    {
        errInfo = L"Unable to initialize ssh session";
        return false;
    }

    if (libssh2_session_startup(_SSHSession, static_cast<int>(_Socket)) != LIBSSH2_ERROR_NONE)
    {
        errInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //Authenticate
    const string userName = CFarPlugin::W2MB(_Session.GetUserName(), CP_UTF8);
    const string password = CFarPlugin::W2MB(_Session.GetPassword(), CP_UTF8);
    if (keyFileName)
    {
        //By key
        const string keyPlaneFileName = CFarPlugin::W2MB(keyFileName);
        if (libssh2_userauth_publickey_fromfile_ex(_SSHSession, userName.c_str(), static_cast<unsigned int>(userName.length()), NULL, keyPlaneFileName.c_str(), password.c_str()))
        {
            errInfo = FormatSSHLastErrorDescription();
            return false;
        }
    }
    else
    {
        //By password
        if (libssh2_userauth_password_ex(_SSHSession, userName.c_str(), static_cast<unsigned int>(userName.length()), password.c_str(), static_cast<unsigned int>(password.length()), NULL))
        {
            errInfo = FormatSSHLastErrorDescription();
            return false;
        }
    }

    return true;
}


wstring CSFTP::FormatSSHLastErrorDescription() const
{
    assert(_SSHSession);

    string errorMessage = "SSH session error (libssh2)\n";

    char *sshErrMsg = NULL;
    const int errCode = libssh2_session_last_error(_SSHSession, &sshErrMsg, NULL, 0);

    char codeText[16];
    _itoa_s(errCode, codeText, 10);
    errorMessage += codeText;
    errorMessage += ": ";

    if (sshErrMsg)
    {
        errorMessage += sshErrMsg;
    }
    else
    {
        errorMessage += "Unknown error";
    }

    return CFarPlugin::MB2W(errorMessage.c_str());
}
