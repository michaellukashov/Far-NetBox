/**************************************************************************
 *  Far plugin helper (for FAR 2.0) (http://code.google.com/p/farplugs)   *
 *  Copyright (C) 2000-2011 by Artem Senichev <artemsen@gmail.com>        *
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

#include "FarPlugin.h"
#include "FarSettings.h"
#include "FarDialog.h"

/**
 * File read/write wrapper
 */
class CFile
{
public:
    CFile() : m_File(INVALID_HANDLE_VALUE), m_LastError(0) {}
    ~CFile()
    {
        Close();
    }

    /**
     * Open file for writing
     * \param fileName file name
     * \return false if error
     */
    bool OpenWrite(const wchar_t *fileName)
    {
        assert(m_File == INVALID_HANDLE_VALUE);
        assert(fileName);
        m_LastError = ERROR_SUCCESS;

        m_File = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_File == INVALID_HANDLE_VALUE)
        {
            m_LastError = GetLastError();
        }
        return (m_LastError == ERROR_SUCCESS);
    }

    /**
     * Open file for reading
     * \param fileName file name
     * \return false if error
     */
    bool OpenRead(const wchar_t *fileName)
    {
        assert(m_File == INVALID_HANDLE_VALUE);
        assert(fileName);
        m_LastError = ERROR_SUCCESS;

        m_File = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_File == INVALID_HANDLE_VALUE)
        {
            m_LastError = GetLastError();
        }
        return (m_LastError == ERROR_SUCCESS);
    }

    /**
     * Read file
     * \param buff read buffer
     * \param buffSize on input - buffer size, on output - read size in bytes
     * \return false if error
     */
    bool Read(void *buff, size_t &buffSize)
    {
        assert(m_File != INVALID_HANDLE_VALUE);
        m_LastError = ERROR_SUCCESS;

        DWORD bytesRead = static_cast<DWORD>(buffSize);
        if (!ReadFile(m_File, buff, bytesRead, &bytesRead, NULL))
        {
            m_LastError = GetLastError();
            buffSize = 0;
        }
        else
        {
            buffSize = static_cast<size_t>(bytesRead);
        }
        return (m_LastError == ERROR_SUCCESS);
    }

    /**
     * Write file
     * \param buff write buffer
     * \param buffSize buffer size
     * \return false if error
     */
    bool Write(const void *buff, const size_t buffSize)
    {
        assert(m_File != INVALID_HANDLE_VALUE);
        m_LastError = ERROR_SUCCESS;

        DWORD bytesWritten;
        if (!WriteFile(m_File, buff, static_cast<DWORD>(buffSize), &bytesWritten, NULL))
        {
            m_LastError = GetLastError();
        }
        return (m_LastError == ERROR_SUCCESS);
    }

    /**
     * Get file size
     * \return file size or -1 if error
     */
    __int64 GetFileSize()
    {
        assert(m_File != INVALID_HANDLE_VALUE);
        m_LastError = ERROR_SUCCESS;

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(m_File, &fileSize))
        {
            m_LastError = GetLastError();
            return -1;
        }
        return fileSize.QuadPart;
    }

    /**
     * Close file
     */
    void Close()
    {
        if (m_File != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_File);
            m_File = INVALID_HANDLE_VALUE;
        }
    }

    /**
     * Get last errno
     * \return last errno
     */
    DWORD LastError() const
    {
        return m_LastError;
    }

    /**
     * Save file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
    static DWORD SaveFile(const wchar_t *fileName, const std::vector<char>& fileContent)
    {
        CFile f;
        if (f.OpenWrite(fileName) && !fileContent.empty())
        {
            f.Write(&fileContent[0], fileContent.size());
        }
        return f.LastError();
    }

    /**
     * Save file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
    static DWORD SaveFile(const wchar_t *fileName, const char *fileContent)
    {
        assert(fileContent);
        CFile f;
        if (f.OpenWrite(fileName) && *fileContent)
        {
            f.Write(fileContent, strlen(fileContent));
        }
        return f.LastError();
    }

    /**
     * Load file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
    static DWORD LoadFile(const wchar_t *fileName, std::vector<char>& fileContent)
    {
        fileContent.clear();

        CFile f;
        if (f.OpenRead(fileName))
        {
            const __int64 fs = f.GetFileSize();
            if (fs < 0)
            {
                return f.LastError();
            }
            if (fs == 0)
            {
                return ERROR_SUCCESS;
            }
            size_t s = static_cast<size_t>(fs);
            fileContent.resize(s);
            f.Read(&fileContent[0], s);
        }
        return f.LastError();
    }

private:
    HANDLE  m_File;          ///< File handle
    DWORD   m_LastError;     ///< Laset errno
};


/**
 * Critical section wrapper
 */
class CCriticalSection : public CRITICAL_SECTION
{
public:
    CCriticalSection(const DWORD spinCount = 1024)
    {
        if (!InitializeCriticalSectionAndSpinCount(&m_SyncObj, spinCount))
        {
            throw GetLastError();
        }
    }

    ~CCriticalSection()
    {
        DeleteCriticalSection(&m_SyncObj);
    }

    //Acquire the critical section
    inline void Enter()
    {
        EnterCriticalSection(&m_SyncObj);
    }

    //Release the critical section
    inline void Leave()
    {
        LeaveCriticalSection(&m_SyncObj);
    }

private:
    CRITICAL_SECTION m_SyncObj;
};


#pragma warning(disable: 4512)  //assignment operator could not be generated
/**
 * Simple lock
 */
class CLock
{
public:
    CLock(CCriticalSection &syncObj) : m_SyncObj(syncObj)
    {
        m_SyncObj.Enter();
    }
    ~CLock()
    {
        m_SyncObj.Leave();
    }
private:
    CCriticalSection &m_SyncObj;
};

#pragma warning(default: 4512)  //assignment operator could not be generated

/**
 * Get system error message by error code
 * \param errCode system error code
 * \return error message
 */
std::wstring GetSystemErrorMessage(const DWORD errCode);

/**
 * Parse URL
 * \param url source url std::string
 * \param scheme scheme name
 * \param hostName host name
 * \param port port number
 * \param path path
 * \param query additional query std::string
 * \param userName user name
 * \param password password
 */
void ParseURL(const wchar_t *url, std::wstring *scheme, std::wstring *hostName, unsigned short *port, std::wstring *path, std::wstring *query, std::wstring *userName, std::wstring *password);

/**
 * Convert unix time to file time
 * \param t unix time
 * \return file time
 */
FILETIME UnixTimeToFileTime(const time_t t);

unsigned long TextToNumber(const std::wstring &text);

/**
 * Convert int to std::string
 * \param number number
 * \return std::string result
 */
std::string NumberToText(int number);

std::wstring NumberToWString(unsigned long number);

struct ProxySettings
{
    int proxyType;
    std::wstring proxyHost;
    unsigned long proxyPort;
    std::wstring proxyLogin;
    std::wstring proxyPassword;
};

struct ProxySettingsDialogParams
{
    FarDialogItem *proxyTypeTextItem;
    FarDialogItem *proxyTypeComboBoxItem;
    FarList proxyTypeList;
    std::vector<FarListItem> proxyTypeListItems;
    FarDialogItem *separatorItem;
    FarDialogItem *proxyHostTextItem;
    FarDialogItem *proxyHostItem;
    FarDialogItem *proxyPortTextItem;
    FarDialogItem *proxyPortItem;
    FarDialogItem *proxyLoginTextItem;
    FarDialogItem *proxyLoginItem;
    FarDialogItem *proxyPasswordTextItem;
    FarDialogItem *proxyPasswordItem;
    std::wstring proxyPortStr;

    int idProxyTypeText;
    int idProxyTypeComboBox;
    int idSeparatorItem;
    int idProxyHostText;
    int idProxyHost;
    int idProxyPortText;
    int idProxyPort;
    int idProxyLoginText;
    int idProxyLogin;
    int idProxyPasswordText;
    int idProxyPassword;
};

void InitProxySettingsDialog(CFarDialog &dlg, int &topPos,
    ProxySettings &ps,
    ProxySettingsDialogParams &params,
    bool visible
);

void GetProxySettings(const CFarDialog &dlg, const struct ProxySettingsDialogParams &params,
    struct ProxySettings &proxySettings);

void AppendWChar(std::wstring &str, const wchar_t ch);
void AppendChar(std::string &str, const char ch);

void AppendPathDelimiterW(std::wstring &str);
void AppendPathDelimiterA(std::string &str);

void CheckAbortEvent(HANDLE *AbortEvent);

std::wstring ExpandEnvVars(const std::wstring& str);

//---------------------------------------------------------------------------

std::wstring StringOfChar(const wchar_t c, size_t len);

char *StrNew(const char *str);

wchar_t *AnsiStrScan(const wchar_t *Str, const wchar_t TokenPrefix);

std::wstring EncryptPassword(std::wstring Password, std::wstring Key);
std::wstring DecryptPassword(std::wstring Password, std::wstring Key);

std::wstring ChangeFileExt(std::wstring FileName, std::wstring ext);
std::wstring ExtractFileExt(std::wstring FileName);
std::wstring ExpandUNCFileName(std::wstring FileName);

static void FileSeek(HANDLE file, __int64 offset, __int64 size)
{
}
