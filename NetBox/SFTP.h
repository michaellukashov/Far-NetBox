/**************************************************************************
 *  NetBox plugin for FAR 2.0 (http://code.google.com/p/farplugs)         *
 *  Copyright (C) 2011 by Artem Senichev <artemsen@gmail.com>             *
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
	const wchar_t* GetKeyFile() const;
	void SetKeyFile(const wchar_t* fileName);

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
	CSessionEditorSFTP(CSession* session);

	//From CSessionEditDlg
	void OnPrepareDialog();
	void OnSave();

private:
	int _IdKeyFile;
	int _IdCP;
};


/**
 * SFTP client implementation
 */
class CSFTP : public CProtocolBase<CSessionSFTP>
{
public:
	CSFTP(const CSession* session);
	~CSFTP();

	//From IProtocol
	bool Connect(HANDLE abortEvent, wstring& errorInfo);
	void Close();
	bool CheckExisting(const wchar_t* path, const ItemType type, bool& isExist, wstring& errorInfo);
	bool MakeDirectory(const wchar_t* path, wstring& errorInfo);
	bool GetList(PluginPanelItem** items, int* itemsNum, wstring& errorInfo);
	bool GetFile(const wchar_t* remotePath, const wchar_t* localPath, const unsigned __int64 fileSize, wstring& errorInfo);
	bool PutFile(const wchar_t* remotePath, const wchar_t* localPath, const unsigned __int64 fileSize, wstring& errorInfo);
	bool Rename(const wchar_t* srcPath, const wchar_t* dstPath, const ItemType type, wstring& errorInfo);
	bool Delete(const wchar_t* path, const ItemType type, wstring& errorInfo);
	wstring GetURL();
	
private:
	/**
	 * Open SSH session
	 * \param hostName remote host name
	 * \param port remote host port
	 * \param errInfo buffer to save error message
	 * \return false if error
	 */
	bool OpenSSHSession(const wchar_t* hostName, const unsigned short port, wstring& errInfo);

	/**
	 * Format error description from SSH last error
	 * \return error description
	 */
	wstring FormatSSHLastErrorDescription() const;

	/**
	 * Convert local (unicode) charset to ftp codepage
	 * \param src source path
	 * \return path in ftp codepage
	 */
	inline string LocalToSftpCP(const wchar_t* src) const	{ assert(src && src[0] == L'/'); return CFarPlugin::W2MB(src, _Session.GetCodePage()); }

	/**
	 * Convert ftp charset to local (unicode) codepage
	 * \param src source path
	 * \return path in local (unicode) codepage
	 */
	inline wstring SftpToLocalCP(const char* src) const		{ return CFarPlugin::MB2W(src, _Session.GetCodePage()); }

private:
	SOCKET				_Socket;		///< Session socket
	LIBSSH2_SESSION*	_SSHSession;	///< SSH2 session
	LIBSSH2_SFTP*		_SFTPSession;	///< SFTP session
	HANDLE				_AbortEvent;	///< Abort event
};
