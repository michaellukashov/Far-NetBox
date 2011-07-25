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
#include "EasyURL.h"


/**
 * FTP saved session
 */
class CSessionFTP : public CSession
{
public:
    //Accessors
    UINT GetCodePage() const;
    void SetCodePage(const UINT cp);

protected:
    //From CSession
    PSessionEditor CreateEditorInstance();
    PProtocol CreateClientInstance() const;
};


/**
 * FTP session editor dialog
 */
class CSessionEditorFTP : public CSessionEditor
{
public:
    CSessionEditorFTP(CSession *session);

    //From CSessionEditDlg
    void OnPrepareDialog();
    void OnSave();

protected:
    virtual void ShowSessionDlgItems(bool visible);

private:
    int m_IdSeparator;
    int m_IdCPText;
    int m_IdCP;
};


/**
 * FTP client implementation
 */
class CFTP : public CProtocolBase<CSessionFTP>
{
public:
    CFTP(const CSession *session);
    ~CFTP();

    //From IProtocol
    bool Connect(HANDLE abortEvent, wstring &errorInfo);
    void Close();
    virtual bool CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo);
    virtual bool MakeDirectory(const wchar_t *path, wstring &errorInfo);
    virtual bool GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo);
    virtual bool GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo);
    virtual bool PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo);
    virtual bool Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, wstring &errorInfo);
    virtual bool Delete(const wchar_t *path, const ItemType type, wstring &errorInfo);

private:
    //!FTP item description
    struct FTPItem
    {
        enum ItemType
        {
            Undefined,
            File,
            Directory,
            Link
        };
        FTPItem() : Type(Undefined), Size(0), Permission(0)
        {
            Modified.dwLowDateTime = Modified.dwHighDateTime = 0;
        }
        wstring             Name;
        wstring             LinkPath;
        wstring             Owner;
        wstring             Group;
        wstring             FullText;
        ItemType            Type;
        unsigned __int64    Size;
        unsigned short      Permission;
        FILETIME            Modified;
    };

    /**
     * Get month number by name
     * \param name month's name
     * \return month number or 0 if name is incorrect
     */
    WORD GetMonth(const char *name) const;

    /**
     * Parse permission (from string "rwxr-wr-w" to num 755)
     * \param perm source string
     * \return permission as number
     */
    unsigned short ParsePermission(const char *perm) const;

    /**
     * Parse FTP list
     * \param text list out
     * \param item itemt to save data
     * \return false if error
     */
    bool ParseFtpList(const char *text, FTPItem &item) const;

private:
    CEasyURL m_CURL; ///< CURL easy
};
