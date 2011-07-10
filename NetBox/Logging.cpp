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
#include "Strings.h"
#include "Logging.h"
#include "resource.h"

enum LoggingLevel
{
    LEVEL_DEBUG1,
    LEVEL_DEBUG2,
};

CLogger _Logger;

CLogger::CLogger() :
    _first(true),
    _enableLogging(false),
    _loggingLevel(0),
    _logToFile(false),
    _logFileName(L"C:\\log.log")
{
}

void CLogger::Initialize(bool enableLogging, int loggingLevel,
    bool logToFile, const wstring &logFileName)
{
    CLogger::Shutdown();
    _Logger._enableLogging = enableLogging;
    _Logger._loggingLevel = loggingLevel;
    _Logger._logToFile = logToFile;
    _Logger._logFileName = logFileName;
}

void CLogger::Shutdown()
{
}

void CLogger::Log(int level, const wchar_t *format, va_list args)
{
    // DEBUG_PRINTF(L"NetBox: level = %d, _loggingLevel = %d", level, _loggingLevel);
    if (!_enableLogging)
    {
        return;
    }
    if (!_logToFile)
    {
        return;
    }
    if (!format)
    {
        return;
    }
    if (level > _loggingLevel)
    {
        return;
    }
    // string fn(wcslen(_logFileName.c_str()) + 1, 0);
    // size_t sz = 0;
    // wcstombs_s(&sz, (char *)fn.c_str(), fn.size(), _logFileName.c_str(), _logFileName.size());
    // string fn = CFarPlugin::W2MB(_logFileName.c_str());
    if (_logFileName.empty())
        return;
    FILE *f = _wfsopen(_logFileName.c_str(), _first ? L"w" : L"a", SH_DENYWR);
    if (!f)
        return;
    // Time
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "%02d.%02d.%4d %02d:%02d:%02d:%04d ",
            st.wDay, st.wMonth, st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    // Log
    int len = _vscwprintf(format, args) + 1;
    if (len <= 1)
        return;
    wstring buf(len, 0);
    vswprintf_s(&buf[0], buf.size(), format, args);
    // fwprintf_s(f, L"%s\n", CFarPlugin::W2MB(buf.c_str()).c_str());
    fprintf_s(f, "%s\n", (char *)CFarPlugin::W2MB(buf.c_str()).c_str());
    fflush(f);
    if (fclose(f) == EOF)
    {
        clearerr(f);
        fclose(f);
    }
    _first = false;
}

void Log1(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    _Logger.Log(LEVEL_DEBUG1, format, args);
    va_end(args);
}

void Log2(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    _Logger.Log(LEVEL_DEBUG2, format, args);
    va_end(args);
}

