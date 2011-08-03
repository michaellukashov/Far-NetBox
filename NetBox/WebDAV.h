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

class TiXmlElement;


/**
 * WebDAV saved session
 */
class CSessionWebDAV : public CSession
{
protected:
    //From CSession
    PSessionEditor CreateEditorInstance();
    PProtocol CreateClientInstance() const;
};


/**
 * WebDAV session editor dialog
 */
class CSessionEditorWebDAV : public CSessionEditor
{
public:
    explicit CSessionEditorWebDAV(CSession *session) :
        CSessionEditor(session, 54, 19)
    {}
};


/**
 * WebDAV client implementation
 */
class CWebDAV : public CProtocolBase<CSessionWebDAV>
{
public:
    explicit CWebDAV(const CSession *session);
    virtual ~CWebDAV();

    //From IProtocol
    bool Connect(HANDLE abortEvent, wstring &errorInfo);
    void Close();
    bool CheckExisting(const wchar_t *path, const ItemType type, bool &isExist, wstring &errorInfo);
    bool MakeDirectory(const wchar_t *path, wstring &errorInfo);
    bool GetList(PluginPanelItem **items, int *itemsNum, wstring &errorInfo);
    bool GetFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo);
    bool PutFile(const wchar_t *remotePath, const wchar_t *localPath, const unsigned __int64 fileSize, wstring &errorInfo);
    bool Rename(const wchar_t *srcPath, const wchar_t *dstPath, const ItemType type, wstring &errorInfo);
    bool Delete(const wchar_t *path, const ItemType type, wstring &errorInfo);
    virtual bool Aborted() const
    {
        return m_CURL.Aborted();
    }

private:
    /**
     * Send PROPFIND request
     * \param dir directory to load
     * \param response response buffer
     * \param errInfo buffer to save error message
     * \return false if error
     */
    bool SendPropFindRequest(const wchar_t *dir, string &response, wstring &errInfo);

    /**
     * Check response for valid code
     * \param expect expected response code
     * \param errInfo buffer to save error message
     * \return false if error (response unexpected)
     */
    bool CheckResponseCode(const long expect, wstring &errInfo);

    /**
     * Check response for valid code
     * \param expect1 expected response code
     * \param expect2 expected response code
     * \param errInfo buffer to save error message
     * \return false if error (response unexpected)
     */
    bool CheckResponseCode(const long expect1, const long expect2, wstring &errInfo);

    /**
     * Get incorrect response information
     * \param code response code
     * \return response information
     */
    wstring GetBadResponseInfo(const int code) const;

    /**
     * Get xml namespace
     * \param element xml element
     * \param name namespace name (URI)
     * \param defaultVal default namespace id
     * \return namespace id
     */
    string GetNamespace(const TiXmlElement *element, const char *name, const char *defaultVal) const;

    /**
     * Parse internet datetime
     * \param dt internet datetime
     * \return corresponding FILETIME (filled zero if error)
     */
    FILETIME ParseDateTime(const char *dt) const;

    /**
     * Check for hexadecimal char (0123456789abcdefABCDEF)
     * \param ch checked char
     * \return true if cahr is a hexadecimal
     */
    inline bool IsHexadecimal(const char ch) const
    {
        return ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
    }

    /**
     * Decode content with safe symbols wrapper (%XX)
     * \param src source string
     * \return decoded content
     */
    string DecodeHex(const string &src) const;

    /**
     * Encode URL to UTF8 format with unsafe symbols wrapper (%XX)
     * \param src source string
     * \return encoded URL
     */
    string EscapeUTF8URL(const wchar_t *src) const;

private:
    CEasyURL m_CURL;         ///< CURL easy
};
