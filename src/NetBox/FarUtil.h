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

#include "FarSettings.h"
#include "FarDialog.h"

/**
 * File read/write wrapper
 */
class CNBFile
{
public:
    CNBFile() : m_File(INVALID_HANDLE_VALUE), m_LastError(0) {}
    ~CNBFile()
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
        CNBFile f;
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
        CNBFile f;
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

        CNBFile f;
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
class CNBCriticalSection : public CRITICAL_SECTION
{
public:
    CNBCriticalSection(const DWORD spinCount = 1024)
    {
        if (!InitializeCriticalSectionAndSpinCount(&m_SyncObj, spinCount))
        {
            throw GetLastError();
        }
    }

    ~CNBCriticalSection()
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
    CLock(CNBCriticalSection &syncObj) : m_SyncObj(syncObj)
    {
        m_SyncObj.Enter();
    }
    ~CLock()
    {
        m_SyncObj.Leave();
    }
private:
    CNBCriticalSection &m_SyncObj;
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

//---------------------------------------------------------------------------

void CheckAbortEvent(HANDLE *AbortEvent);

//---------------------------------------------------------------------------
