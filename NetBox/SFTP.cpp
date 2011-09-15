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
#include "Common.h"
#include "SFTP.h"
#include "Settings.h"
#include "Logging.h"
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
    : CSessionEditor(session, 54, 24), m_IdKeyFile(0)
{
}


void CSessionEditorSFTP::OnPrepareDialog()
{
    m_IdText = CreateText(GetLeft(), GetHeight() - 7, CFarPlugin::GetString(StringEdAuthCert));
    m_IdKeyFile = CreateEdit(GetLeft(), GetHeight() - 6, MAX_SIZE, static_cast<CSessionSFTP *>(m_Session)->GetKeyFile());

    m_IdSeparator = CreateSeparator(GetHeight() - 5);
    CreateCodePageControl(GetHeight() - 4, static_cast<CSessionSFTP *>(m_Session)->GetCodePage(),
        m_IdCPText, m_IdCP);
}


void CSessionEditorSFTP::OnSave()
{
    static_cast<CSessionSFTP *>(m_Session)->SetKeyFile(GetText(m_IdKeyFile).c_str());
    static_cast<CSessionSFTP *>(m_Session)->SetCodePage(static_cast<UINT>(_wtoi(GetText(m_IdCP).c_str())));
}

void CSessionEditorSFTP::ShowSessionDlgItems(bool visible)
{
    CSessionEditor::ShowSessionDlgItems(visible);
    ShowDlgItem(m_IdText, visible);
    ShowDlgItem(m_IdKeyFile, visible);
    ShowDlgItem(m_IdSeparator, visible);
    ShowDlgItem(m_IdCPText, visible);
    ShowDlgItem(m_IdCP, visible);
}

/**
 * libssh2 file handle wrapper
 */
class CSFTPFileHandle
{
public:
    explicit  CSFTPFileHandle(LIBSSH2_SFTP_HANDLE *h)
    {
        m_Object = h;
    }

    explicit CSFTPFileHandle(LIBSSH2_SFTP *sftp, const char *path, const unsigned long flags, const long mode, const int type)
    {
        assert(sftp);
        assert(path && path[0] == L'/');

        m_Object = libssh2_sftp_open_ex(sftp, path, static_cast<unsigned int>(strlen(path)), flags, mode, type);
    }

    ~CSFTPFileHandle()
    {
        if (m_Object)
        {
            libssh2_sftp_close_handle(m_Object);
        }
    }

    inline operator LIBSSH2_SFTP_HANDLE *() const
    {
        return m_Object;
    }
    inline LIBSSH2_SFTP_HANDLE *operator->() const
    {
        return m_Object;
    };

protected:
    LIBSSH2_SFTP_HANDLE *m_Object;
};


CSFTP::CSFTP(const CSession *session)
    : CProtocolBase(session), m_Socket(INVALID_SOCKET), m_SSHSession(NULL), m_SFTPSession(NULL), m_AbortEvent(NULL)
{
}


CSFTP::~CSFTP()
{
    Close();
}


bool CSFTP::Connect(HANDLE abortEvent, std::wstring &errorInfo)
{
    assert(abortEvent);
    assert(m_Socket == INVALID_SOCKET);

    m_AbortEvent = abortEvent;

    std::wstring hostName;
    std::wstring path;
    unsigned short port = 0;
    ParseURL(m_Session.GetURL(), NULL, &hostName, &port, &path, NULL, NULL, NULL);

    if (OpenSSHSession(hostName.c_str(), port, errorInfo))
    {
        bool aborted = false;
        libssh2_session_set_blocking(m_SSHSession, 0);
        m_SFTPSession = libssh2_sftp_init(m_SSHSession);
        while (!m_SFTPSession)
        {
            int last_errno = libssh2_session_last_errno(m_SSHSession);
            if (last_errno != LIBSSH2SFTP_EAGAIN)
            {
                // DEBUG_PRINTF(L"CSFTP::Connect: libssh2_sftp_init failed: %d", last_errno);
                break;
            }
            ::CheckAbortEvent(&m_AbortEvent);
            if (WaitForSingleObject(m_AbortEvent, 0) == WAIT_OBJECT_0)
            {
                aborted = true;
                break;
            }
            Sleep(100);
            m_SFTPSession = libssh2_sftp_init(m_SSHSession);
        }
        if (m_SFTPSession != NULL)
        {
            libssh2_session_set_blocking(m_SSHSession, 1);
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

    if (m_SFTPSession == NULL)
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
            m_CurrentDirectory = path;
        }
        else
        {
            char realPath[256];
            int res = libssh2_sftp_symlink_ex(m_SFTPSession, NULL, 0, realPath, static_cast<unsigned int>(sizeof(realPath)), LIBSSH2_SFTP_REALPATH);
            m_CurrentDirectory = res >= 0 ? ::MB2W(realPath, CP_UTF8) : L"/";
        }
    }

    while(m_CurrentDirectory.size() > 1 && m_CurrentDirectory[m_CurrentDirectory.length() - 1] == L'/')
    {
        m_CurrentDirectory.erase(m_CurrentDirectory.length() - 1);
    }

    return (m_SFTPSession != NULL);
}


void CSFTP::Close()
{
    if (m_SFTPSession)
    {
        libssh2_sftp_shutdown(m_SFTPSession);
        m_SFTPSession = NULL;
    }

    if (m_SSHSession)
    {
        libssh2_session_disconnect(m_SSHSession, "Normal shutdown");
        libssh2_session_free(m_SSHSession);
        m_SSHSession = NULL;
    }

    if (m_Socket != INVALID_SOCKET)
    {
        shutdown(m_Socket, SD_BOTH);
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
}


bool CSFTP::CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, std::wstring &errorInfo)
{
    assert(type == ItemDirectory);
    assert(m_SFTPSession);
    (void)type;

    CSFTPFileHandle dirHandle(m_SFTPSession, LocalToSftpCP(path).c_str(), 0, 0, LIBSSH2_SFTP_OPENDIR);
    if (!dirHandle)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }
    isExist = (dirHandle != NULL);
    return true;
}


bool CSFTP::MakeDirectory(const wchar_t *path, std::wstring &errorInfo)
{
    assert(m_SFTPSession);

    const std::string sftpPath = LocalToSftpCP(path);
    const bool retStatus = (libssh2_sftp_mkdir_ex(m_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length()), LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IXGRP | LIBSSH2_SFTP_S_IROTH | LIBSSH2_SFTP_S_IXOTH) == LIBSSH2_ERROR_NONE);
    if (!retStatus)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }
    return retStatus;
}


bool CSFTP::GetList(PluginPanelItem **items, int *itemsNum, std::wstring &errorInfo)
{
    assert(items);
    assert(itemsNum);
    assert(m_SFTPSession);

    CSFTPFileHandle dirHandle(m_SFTPSession, LocalToSftpCP(m_CurrentDirectory.c_str()).c_str(), 0, 0, LIBSSH2_SFTP_OPENDIR);
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
        std::wstring             Name;
        DWORD               Attributes;
        FILETIME            Modified;
        FILETIME            LastAccess;
        unsigned __int64    Size;
    };
    std::vector<SFTPItem> sftpItems;

    //Read directory content
    char fileName[512];
    LIBSSH2_SFTP_ATTRIBUTES sftpAttrs;
    while (libssh2_sftp_readdir_ex(dirHandle, fileName, sizeof(fileName), NULL, 0, &sftpAttrs) > 0)
    {
        // DEBUG_PRINTF(L"NetBox: fileName = %s, isdir = %u, islink = %u", ::MB2W(fileName).c_str(), LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions), LIBSSH2_SFTP_S_ISLNK(sftpAttrs.permissions));
        if (LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions) && (strcmp(fileName, ".") == 0 || strcmp(fileName, "..") == 0))
        {
            continue;
        }
        SFTPItem sftpItem;
        sftpItem.Name = SftpToLocalCP(fileName);
        if (LIBSSH2_SFTP_S_ISLNK(sftpAttrs.permissions))
        {
            // check symlink type
            char target[512];
            int rc = libssh2_sftp_symlink_ex(m_SFTPSession, fileName, strlen(fileName),
                target, sizeof(target), LIBSSH2_SFTP_REALPATH);
            // DEBUG_PRINTF(L"NetBox: rc = %u, target = %s", rc, ::MB2W(target));
            if (rc > 0)
            {
                CSFTPFileHandle sftpDir(m_SFTPSession, target, LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENDIR);
                if (sftpDir)
                {
                    sftpItem.Attributes = FILE_ATTRIBUTE_DIRECTORY;
                }
            }
        }
        else if (LIBSSH2_SFTP_S_ISDIR(sftpAttrs.permissions))
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


bool CSFTP::GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo)
{
    assert(localPath && *localPath);
    assert(m_SFTPSession);

    //Open source file
    CSFTPFileHandle sftpFile(m_SFTPSession, LocalToSftpCP(remotePath).c_str(), LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
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
        m_ProgressPercent = 0;
    }

    std::vector<char> buff(4096);
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
            m_ProgressPercent = static_cast<int>((readBytes * 100) / fileSize);
        }

        if (WaitForSingleObject(m_AbortEvent, 0) == WAIT_OBJECT_0)
        {
            return false;
        }
    }

    outFile.Close();
    m_ProgressPercent = -1;

    return true;
}


bool CSFTP::PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo)
{
    assert(localPath && *localPath);
    assert(m_SFTPSession);

    //Destination (SFTP) file
    CSFTPFileHandle sftpFile(m_SFTPSession, LocalToSftpCP(remotePath).c_str(),
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
        m_ProgressPercent = 0;
    }

    std::vector<char> buff(4096);
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
            m_ProgressPercent = static_cast<int>((writeBytes * 100) / fileSize);
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
        if (WaitForSingleObject(m_AbortEvent, 0) == WAIT_OBJECT_0)
        {
            return false;
        }
    }

    inFile.Close();
    m_ProgressPercent = -1;

    return true;
}


bool CSFTP::Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType /*type*/, std::wstring &errorInfo)
{
    assert(m_SFTPSession);

    const std::string srcSftpPath = LocalToSftpCP(srcPath);
    const std::string dstSftpPath = LocalToSftpCP(dstPath);

    const bool retStatus = (libssh2_sftp_rename_ex(m_SFTPSession, srcSftpPath.c_str(),
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


bool CSFTP::Delete(const wchar_t *path, const ItemType type, std::wstring &errorInfo)
{
    assert(m_SFTPSession);

    bool retStatus = false;

    const std::string sftpPath = LocalToSftpCP(path);

    if (type == ItemDirectory)
    {
        retStatus = (libssh2_sftp_rmdir_ex(m_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }
    else
    {
        retStatus = (libssh2_sftp_unlink_ex(m_SFTPSession, sftpPath.c_str(), static_cast<unsigned int>(sftpPath.length())) == LIBSSH2_ERROR_NONE);
    }

    if (!retStatus)
    {
        errorInfo = FormatSSHLastErrorDescription();
    }

    return retStatus;
}

std::wstring CSFTP::GetURL()
{
    return CProtocolBase<CSessionSFTP>::GetURL();
}

bool CSFTP::OpenSSHSession(const wchar_t *hostName, const unsigned short port, std::wstring &errInfo)
{
    assert(m_Socket == INVALID_SOCKET);

    //Check private key file existing (libssh2 doesn't do it)
    const wchar_t *keyFileName = m_Session.GetKeyFile();
    if (*keyFileName == 0)
    {
        keyFileName = NULL;
    }
    if (keyFileName && GetFileAttributes(keyFileName) == INVALID_FILE_ATTRIBUTES)
    {
        errInfo = FormatErrorDescription(GetLastError(), CFarPlugin::GetFormattedString(StringErrKeyFile, keyFileName).c_str());
        return false;
    }

    const hostent *remoteHost = gethostbyname(::W2MB(hostName).c_str());
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
    m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_Socket == INVALID_SOCKET || connect(m_Socket, reinterpret_cast<sockaddr *>(&sockAddr), sizeof(sockAddr)) != 0)
    {
        errInfo = FormatErrorDescription(WSAGetLastError());
        return false;
    }

    //Open SSH2 session
    m_SSHSession = libssh2_session_init();
    if (!m_SSHSession)
    {
        errInfo = L"Unable to initialize ssh session";
        return false;
    }

    if (libssh2_session_startup(m_SSHSession, static_cast<int>(m_Socket)) != LIBSSH2_ERROR_NONE)
    {
        errInfo = FormatSSHLastErrorDescription();
        return false;
    }

    //Authenticate
    const std::string userName = ::W2MB(m_Session.GetUserName(), CP_UTF8);
    const std::string password = ::W2MB(m_Session.GetPassword(), CP_UTF8);
    if (keyFileName)
    {
        //By key
        const std::string keyPlaneFileName = ::W2MB(keyFileName);
        if (libssh2_userauth_publickey_fromfile_ex(m_SSHSession, userName.c_str(), static_cast<unsigned int>(userName.length()), NULL, keyPlaneFileName.c_str(), password.c_str()))
        {
            errInfo = FormatSSHLastErrorDescription();
            return false;
        }
    }
    else
    {
        //By password
        if (libssh2_userauth_password_ex(m_SSHSession, userName.c_str(), static_cast<unsigned int>(userName.length()), password.c_str(), static_cast<unsigned int>(password.length()), NULL))
        {
            errInfo = FormatSSHLastErrorDescription();
            return false;
        }
    }
    if (m_Settings.EnableLogging() && m_Settings.LoggingLevel() == LEVEL_DEBUG2)
    {
        DEBUG_PRINTF(L"NetBox: before libssh2_session_callback_set");
        libssh2_trace_sethandler(m_SSHSession, this, libssh2_trace_handler_func);
        libssh2_trace(m_SSHSession, LIBSSH2_TRACE_AUTH | LIBSSH2_TRACE_CONN | LIBSSH2_TRACE_SFTP | LIBSSH2_TRACE_ERROR);
        // libssh2_trace(m_SSHSession, LIBSSH2_TRACE_AUTH | LIBSSH2_TRACE_SCP | LIBSSH2_TRACE_SFTP);
    }

    return true;
}

void CSFTP::libssh2_trace_handler_func(LIBSSH2_SESSION *session,
   void *context,
   const char *message,
   size_t len)
{
    // DEBUG_PRINTF(L"NetBox: %s", ::MB2W(message).c_str());
    (void)session;
    (void)context;
    (void)len;
    Log2(message);
}

std::wstring CSFTP::FormatSSHLastErrorDescription() const
{
    assert(m_SSHSession);

    std::string errorMessage = "SSH session error (libssh2)\n";

    char *sshErrMsg = NULL;
    const int errCode = libssh2_session_last_error(m_SSHSession, &sshErrMsg, NULL, 0);

    errorMessage += NumberToText(errCode);
    errorMessage += ": ";

    if (sshErrMsg)
    {
        errorMessage += sshErrMsg;
    }
    else
    {
        errorMessage += "Unknown error";
    }

    return ::MB2W(errorMessage.c_str());
}
