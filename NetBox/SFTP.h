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

#pragma once

#include "ProtocolBase.h"
#include <libssh2.h>
#include <libssh2_sftp.h>


/**
 * SFTP saved session
 */
class CSessionSFTP : public CSession
{
public:
    //Accessors
    UINT GetCodePage() const;
    void SetCodePage(const UINT cp);
    const wchar_t *GetKeyFile() const;
    void SetKeyFile(const wchar_t *fileName);

protected:
    //From CSession
    PSessionEditor CreateEditorInstance();
    PProtocol CreateClientInstance() const;
};


/**
 * SFTP session editor dialog
 */
class CSessionEditorSFTP : public CSessionEditor
{
public:
    explicit CSessionEditorSFTP(CSession *session);

    //From CSessionEditDlg
    void OnPrepareDialog();
    void OnSave();

protected:
    virtual void ShowSessionDlgItems(bool visible);

private:
    int m_IdText;
    int m_IdKeyFile;
    int m_IdSeparator;
    int m_IdCPText;
    int m_IdCP;
};


/**
 * SFTP client implementation
 */
class CSFTP : public CProtocolBase<CSessionSFTP>
{
public:
    explicit CSFTP(const CSession *session);
    virtual ~CSFTP();

    //From IProtocol
    virtual bool Connect(HANDLE abortEvent, std::wstring &errorInfo);
    virtual void Close();
    virtual bool CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, std::wstring &errorInfo);
    virtual bool MakeDirectory(const wchar_t *path, std::wstring &errorInfo);
    virtual bool GetList(PluginPanelItem **items, int *itemsNum, std::wstring &errorInfo);
    virtual bool GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo);
    virtual bool PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, std::wstring &errorInfo);
    virtual bool Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, std::wstring &errorInfo);
    virtual bool Delete(const wchar_t *path, const ItemType type, std::wstring &errorInfo);
    virtual std::wstring GetURL();

private:
    /**
     * Open SSH session
     * \param hostName remote host name
     * \param port remote host port
     * \param errInfo buffer to save error message
     * \return false if error
     */
    bool OpenSSHSession(const wchar_t *hostName, const unsigned short port, std::wstring &errInfo);

    /**
     * Format error description from SSH last error
     * \return error description
     */
    std::wstring FormatSSHLastErrorDescription() const;

    /**
     * Convert local (unicode) charset to ftp codepage
     * \param src source path
     * \return path in ftp codepage
     */
    inline std::string LocalToSftpCP(const wchar_t *src) const
    {
        assert(src && src[0] == L'/');
        return ::W2MB(src, m_Session.GetCodePage());
    }

    /**
     * Convert ftp charset to local (unicode) codepage
     * \param src source path
     * \return path in local (unicode) codepage
     */
    inline std::wstring SftpToLocalCP(const char *src) const
    {
        return ::MB2W(src, m_Session.GetCodePage());
    }

private:
    static void libssh2_trace_handler_func(LIBSSH2_SESSION *,
       void *,
       const char *,
       size_t);
private:
    SOCKET m_Socket; ///< Session socket
    LIBSSH2_SESSION *m_SSHSession; ///< SSH2 session
    LIBSSH2_SFTP *m_SFTPSession; ///< SFTP session
    HANDLE m_AbortEvent; ///< Abort event
};
