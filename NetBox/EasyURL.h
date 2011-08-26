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

#include "FarUtil.h"

#include <curl/curl.h>

#define CHECK_CUCALL(code, fc) if (code == CURLE_OK) code = fc


/**
 * CURL slist wrapper
 */
class CSlistURL
{
public:
    explicit CSlistURL() : m_SList(NULL) {}
    ~CSlistURL()
    {
        if (m_SList)
        {
            curl_slist_free_all(m_SList);
        }
    }
    inline void Append(const char *val)
    {
        assert(val);
        m_SList = curl_slist_append(m_SList, val);
    }
    operator curl_slist *()
    {
        return m_SList;
    }
private:
    curl_slist *m_SList;
};


/**
 * CURL easy wrapper
 */
class CEasyURL
{
public:
    explicit CEasyURL();
    ~CEasyURL();

    /**
     * Initialize easy curl
     * \param url URL to connect
     * \param userName user name
     * \param password password
     * \return false if error
     */
    bool Initialize(const wchar_t *url, const wchar_t *userName, const wchar_t *password,
        const struct ProxySettings &proxySettings);

    /**
     * Close curl
     */
    void Close();

    /**
     * Prepare easy curl state
     * \param path reauested path
     * \param handleTimeout true to handle timeout
     * \return curl status
     */
    CURLcode Prepare(const char *path, const bool handleTimeout = true);

    /**
     * Set slist
     * \param slist slist object
     * \return curl status
     */
    CURLcode SetSlist(CSlistURL &slist);

    /**
     * Set output as std::string buffer
     * \param out output std::string buffer
     * \param progress pointer to variable to save progress percent of the current operation
     * \return curl status
     */
    CURLcode SetOutput(std::string &out, int *progress);

    /**
     * Set output as file
     * \param out output file
     * \param progress pointer to variable to save progress percent of the current operation
     * \return curl status
     */
    CURLcode SetOutput(CFile *out, int *progress);

    /**
     * Set input as file (upload operations)
     * \param in input file
     * \param progress pointer to variable to save progress percent of the current operation
     * \return curl status
     */
    CURLcode SetInput(CFile *in, int *progress);

    /**
     * Set abort event handle
     * \param event abort event handle
     */
    void SetAbortEvent(HANDLE event);

    /**
     * Perform request
     * \return curl status
     */
    CURLcode Perform();

    /**
     * Execute FTP command
     * \param cmd command std::string
     * \return curl status
     */
    CURLcode ExecuteFtpCommand(const char *cmd);

    /**
     * Get top URL
     * \return top URL
     */
    inline const char *GetTopURL() const
    {
        return m_TopURL.c_str();
    }

    operator CURL *()
    {
        return m_CURL;
    }

    bool Aborted() const
    {
        return m_Progress.Aborted;
    }

private:
    int DebugOutput(const char *data, size_t size);

private:
    //Internal reader callback (see libcurl docs)
    static size_t InternalReader(void *buffer, size_t size, size_t nmemb, void *userData);

    //Internal writer callback (see libcurl docs)
    static size_t InternalWriter(void *buffer, size_t size, size_t nmemb, void *userData);

    //Internal progress counter callback (see libcurl docs)
    static int InternalProgress(void *userData, double dltotal, double dlnow, double ultotal, double ulnow);

    static int InternalDebug(CURL *handle, curl_infotype type,
                 char *data, size_t size,
                 void *userp);

private:
    CURL *m_CURL; ///< CURL
    bool m_Prepared; ///< Preapre statement flag

    std::string m_TopURL; ///< Top URL (ftp://host:21)
    std::string m_UserName; ///< User name
    std::string m_Password; ///< Password

    //! Output writer description
    struct OutputWriter
    {
        enum OutputWriterType
        {
            None,
            TypeString,
            TypeFile
        } Type;
        union
        {
            std::string *String;
            CFile  *File;
        };
        HANDLE AbortEvent;
    };
    OutputWriter m_Output;

    //! Input reader description
    struct InputReader
    {
        enum InputReaderType
        {
            None,
            TypeFile
        } Type;
        CFile  *File;
        int    *Progress;
        unsigned __int64 Current;
        unsigned __int64 Total;
        HANDLE AbortEvent;
    };
    InputReader m_Input;

    //! Progress description
    struct Progress
    {
        int *ProgressPtr;
        bool Aborted;
        HANDLE  AbortEvent;
    };
    Progress m_Progress;
    struct ProxySettings m_proxySettings;
    HANDLE m_regex;
    RegExpMatch *m_match;
    int m_brackets;
};

