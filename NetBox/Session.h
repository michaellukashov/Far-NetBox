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

#include "Protocol.h"
#include "SessionEditor.h"

class CSession;
typedef auto_ptr<CSession> PSession;


/**
 * Base (common) saved session
 */
class CSession
{
protected:
    CSession();

public:
    typedef PSession (*CreateSessionFx)();
    template<class T> static PSession SessionCreator()
    {
        return PSession(new T);
    }

    /**
     * Register client's protocol implementation
     * \param id unique protocol Id
     * \param name protocol name
     * \param fx create new session function
     * \param scheme1 first supported internet scheme name
     * \param scheme2 second supported internet scheme name
     */
    static void RegisterProtocolClient(const int id, const wchar_t *name, CreateSessionFx fx, const wchar_t *scheme1, const wchar_t *scheme2 = NULL);

    /**
     * Get protocol name
     * \param protoId unique protocol Id
     * \return protocol name
     */
    static const wchar_t *GetProtocolName(const int protoId);

    /**
     * Get default scheme name
     * \param protoId unique protocol Id
     * \return default scheme name
     */
    static const wchar_t *GetDefaultScheme(const int protoId);

    /**
     * Get supported protocols prefixes
     * \return supported protocols prefixes
     */
    static wstring GetSupportedPrefixes();

    /**
     * Create new session
     * \return pointer to created session (NULL if error)
     */
    static PSession Create();

    /**
     * Create session from command prefix
     * \param prefix command prefix
     * \return pointer to created session (NULL if error)
     */
    static PSession Create(const wchar_t *prefix);

    /**
     * Load session from xml file
     * \param fileName xml file name
     * \return pointer to loaded session (NULL if error)
     */
    static PSession Load(const wchar_t *fileName);

    /**
     * Import session from Far FTP plugin
     * \param sourcePath source path
     * \param panelItem pointer to panel items
     * \param itemsNumber items counter
     * \param deleteSource delete source files flag (move mode)
     * \return FAR's return code for PutFilesW
     */
    static PSession ImportFromFTP(const wchar_t *fileName);

    /**
     * Export sessions from registry to NetBox 1.10 format
     * !!!! this functionality will be removed in next release !!!!
     */
#ifndef _DEBUG
    __declspec(deprecated)
#endif
    static void ExportFromRegistry();

    /**
     * Edit session
     * \return false if edition was canceled by user
     */
    bool Edit();

    /**
     * Safe save session to xml file
     * \param path to save xml file
     * \param overwrite overwrite existing file flag
     * \return false if error
     */
    bool Save(const wchar_t *path, const bool overwrite) const;

    /**
     * Create protocol client implementation instance
     * \return protocol client implementation instance
     */
    PProtocol CreateClient();

    //Accessors
    inline int GetProtocolId() const
    {
        return _ProtoId;
    }
    const wchar_t *GetSessionName() const;
    void SetSessionName(const wchar_t *val);
    const wchar_t *GetURL() const;
    void SetURL(const wchar_t *val);
    const wchar_t *GetUserName() const;
    void SetUserName(const wchar_t *val);
    const wchar_t *GetPassword() const;
    void SetPassword(const wchar_t *val);
    bool GetPromptPwd() const;
    void SetPromptPwd(const bool val);

protected:
    /**
     * Get property
     * \param name property name
     * \param defaultVal default value if key not found
     * \return property value
     */
    const wchar_t *GetProperty(const char *name, const wchar_t *defaultVal = NULL) const;

    /**
     * Get property as numeric value
     * \param name property name
     * \param defaultVal default value if key not found
     * \return property value
     */
    __int64 GetPropertyNumeric(const char *name, const __int64 defaultVal = 0) const;

    /**
     * Set property
     * \param name property name
     * \param val property value
     * \param needCrypt crypt flag
     */
    void SetProperty(const char *name, const wchar_t *val, const bool needCrypt = false);

    /**
     * Set property
     * \param name property name
     * \param val property value
     */
    void SetProperty(const char *name, const __int64 val);

protected:
    /**
     * Cretae editor implementation instance
     * \return session editor
     */
    virtual PSessionEditor CreateEditorInstance() = 0;

    /**
     * Create protocol client implementation instance
     * \return protocol client implementation instance
     */
    virtual PProtocol CreateClientInstance() const = 0;

private:
    /**
     * Create session for specified protocol
     * \param protoId protocol's id
     * \return pointer to created session (NULL if error)
     */
    static PSession Create(const int protoId);

    /**
     * Save session to xml file
     * \param fileName xml file name
     * \return false if error
     */
    bool Save(const wchar_t *fileName) const;

    /**
     * Crypt/decrypt string
     * \param src source string
     * \param encrypt operation type (true = encode; false = decode)
     * \return encrypted/decrypted string value
     */
    wstring Crypt(const wstring &src, const bool encrypt) const;

    //! Client's protocol implementation
    struct ProtoImplInfo
    {
        int             ProtoId;            ///< Unique protocol Id
        const wchar_t  *Name;               ///< Protocol name
        const wchar_t  *Schemes[2];         ///< Supported scheme names
        CreateSessionFx CreateSession;      ///< Create new session function
    };

    /**
     * Get client's protocol implementation structure pointer
     * \param protoId unique protocol Id
     * \return client's protocol implementation structure pointer (NULL if not found)
     */
    static const ProtoImplInfo *GetProtoImplInfo(const int protoId);

private:
    //! Session property description
    struct Property
    {
        Property() : NeedCrypt(false) {}
        string  Name;           ///< Property name
        wstring Value;          ///< Property value
        bool    NeedCrypt;      ///< Crypt value flag
    };

private:
    int                 _ProtoId;           ///< Unique protocol id
    wstring             _SessionName;       ///< Session name
    vector<Property>    _Properties;        ///< Session's properties

    static string       _CryptKey;          ///< Crypt key
    static vector<ProtoImplInfo> _Factory;  ///< Client protocol implementation factory
};
