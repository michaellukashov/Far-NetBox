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

/**
 * File read/write wrapper
 */
class CFile
{
public:
    CFile() : _File(INVALID_HANDLE_VALUE), _LastError(0) {}
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
        assert(_File == INVALID_HANDLE_VALUE);
        assert(fileName);
        _LastError = ERROR_SUCCESS;

        _File = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (_File == INVALID_HANDLE_VALUE)
        {
            _LastError = GetLastError();
        }
        return (_LastError == ERROR_SUCCESS);
    }

    /**
     * Open file for reading
     * \param fileName file name
     * \return false if error
     */
    bool OpenRead(const wchar_t *fileName)
    {
        assert(_File == INVALID_HANDLE_VALUE);
        assert(fileName);
        _LastError = ERROR_SUCCESS;

        _File = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (_File == INVALID_HANDLE_VALUE)
        {
            _LastError = GetLastError();
        }
        return (_LastError == ERROR_SUCCESS);
    }

    /**
     * Read file
     * \param buff read buffer
     * \param buffSize on input - buffer size, on output - read size in bytes
     * \return false if error
     */
    bool Read(void *buff, size_t &buffSize)
    {
        assert(_File != INVALID_HANDLE_VALUE);
        _LastError = ERROR_SUCCESS;

        DWORD bytesRead = static_cast<DWORD>(buffSize);
        if (!ReadFile(_File, buff, bytesRead, &bytesRead, NULL))
        {
            _LastError = GetLastError();
            buffSize = 0;
        }
        else
        {
            buffSize = static_cast<size_t>(bytesRead);
        }
        return (_LastError == ERROR_SUCCESS);
    }

    /**
     * Write file
     * \param buff write buffer
     * \param buffSize buffer size
     * \return false if error
     */
    bool Write(const void *buff, const size_t buffSize)
    {
        assert(_File != INVALID_HANDLE_VALUE);
        _LastError = ERROR_SUCCESS;

        DWORD bytesWritten;
        if (!WriteFile(_File, buff, static_cast<DWORD>(buffSize), &bytesWritten, NULL))
        {
            _LastError = GetLastError();
        }
        return (_LastError == ERROR_SUCCESS);
    }

    /**
     * Get file size
     * \return file size or -1 if error
     */
    __int64 GetFileSize()
    {
        assert(_File != INVALID_HANDLE_VALUE);
        _LastError = ERROR_SUCCESS;

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(_File, &fileSize))
        {
            _LastError = GetLastError();
            return -1;
        }
        return fileSize.QuadPart;
    }

    /**
     * Close file
     */
    void Close()
    {
        if (_File != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_File);
            _File = INVALID_HANDLE_VALUE;
        }
    }

    /**
     * Get last errno
     * \return last errno
     */
    DWORD LastError() const
    {
        return _LastError;
    }

    /**
     * Save file
     * \param fileName file name
     * \param fileContent file content
     * \return error code
     */
    static DWORD SaveFile(const wchar_t *fileName, const vector<char>& fileContent)
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
    static DWORD LoadFile(const wchar_t *fileName, vector<char>& fileContent)
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
    HANDLE  _File;          ///< File handle
    DWORD   _LastError;     ///< Laset errno
};


/**
 * Critical section wrapper
 */
class CCriticalSection : public CRITICAL_SECTION
{
public:
    CCriticalSection(const DWORD spinCount = 1024)
    {
        if (!InitializeCriticalSectionAndSpinCount(&_SyncObj, spinCount))
        {
            throw GetLastError();
        }
    }

    ~CCriticalSection()
    {
        DeleteCriticalSection(&_SyncObj);
    }

    //Acquire the critical section
    inline void Enter()
    {
        EnterCriticalSection(&_SyncObj);
    }

    //Release the critical section
    inline void Leave()
    {
        LeaveCriticalSection(&_SyncObj);
    }

private:
    CRITICAL_SECTION _SyncObj;
};


#pragma warning(disable: 4512)  //assignment operator could not be generated
/**
 * Simple lock
 */
class CLock
{
public:
    CLock(CCriticalSection &syncObj) : _SyncObj(syncObj)
    {
        _SyncObj.Enter();
    }
    ~CLock()
    {
        _SyncObj.Leave();
    }
private:
    CCriticalSection &_SyncObj;
};
#pragma warning(default: 4512)  //assignment operator could not be generated
