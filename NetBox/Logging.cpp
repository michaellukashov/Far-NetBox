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

void CLogger::Log(const wchar_t *format, ...)
{
    if (!format)
        return;
    string fn(wcslen(_logFileName.c_str()) + 1, 0);
    wcstombs((char *)fn.c_str(), _logFileName.c_str(), _logFileName.size());
    // DEBUG_PRINTF(L"NetBox: fn = %s", (wchar_t *)fn.c_str());
    FILE *f = !fn.empty() ? fopen(fn.c_str(), _first ? "w" : "a") : NULL;
    if (!f)
        return;
    // Time
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f,"%4d.%02d.%02d %02d:%02d:%02d:%04d ",
            st.wYear, st.wMonth,  st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    // Log
    va_list args;
    va_start(args, format);
    int len = _vscwprintf(format, args) + 1;
    if (len <= 1)
        return;
    wstring buf(len, 0);
    vswprintf_s(&buf[0], buf.size(), format, args);
    va_end(args);
    buf.erase(buf.size() - 1);
    fwprintf(f, buf.c_str());
    // EOL
    fprintf(f,"\n");
    fclose(f);
    _first = false;
}

