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

CLogger m_Logger;

CLogger::CLogger() :
    m_first(true),
    m_enableLogging(false),
    m_loggingLevel(0),
    m_logToFile(false),
    m_logFileName(L"C:\\log.log")
{
}

void CLogger::Initialize(bool enableLogging, int loggingLevel,
    bool logToFile, const wstring &logFileName)
{
    CLogger::Shutdown();
    m_Logger.m_enableLogging = enableLogging;
    m_Logger.m_loggingLevel = loggingLevel;
    m_Logger.m_logToFile = logToFile;
    m_Logger.m_logFileName = logFileName;
}

void CLogger::Shutdown()
{
}

void CLogger::Log(int level, const wchar_t *format, va_list args)
{
    // DEBUG_PRINTF(L"NetBox: level = %d, m_loggingLevel = %d", level, m_loggingLevel);
    if (!m_enableLogging)
    {
        return;
    }
    if (!m_logToFile)
    {
        return;
    }
    if (!format)
    {
        return;
    }
    if (level > m_loggingLevel)
    {
        return;
    }
    if (m_logFileName.empty())
        return;
    FILE *f = _wfsopen(m_logFileName.c_str(), m_first ? L"a" : L"a", SH_DENYWR);
    if (!f)
        return;
    // Time
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "%02d.%02d.%4d %02d:%02d:%02d:%04d ",
            st.wDay, st.wMonth, st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    // Log
    int len = _vscwprintf(format, args);
    wstring buf(len + sizeof(wchar_t), 0);
    vswprintf_s(&buf[0], buf.size(), format, args);
    fprintf_s(f, "%s\n", (char *)CFarPlugin::W2MB(buf.c_str()).c_str());

    fflush(f);
    if (fclose(f) == EOF)
    {
        clearerr(f);
        fclose(f);
    }
    m_first = false;
}

void CLogger::Log(int level, const char *str)
{
    // DEBUG_PRINTF(L"NetBox: level = %d, m_loggingLevel = %d", level, m_loggingLevel);
    if (!m_enableLogging)
    {
        return;
    }
    if (!m_logToFile)
    {
        return;
    }
    if (!str)
    {
        return;
    }
    if (level > m_loggingLevel)
    {
        return;
    }
    if (m_logFileName.empty())
        return;
    FILE *f = _wfsopen(m_logFileName.c_str(), m_first ? L"a" : L"a", SH_DENYWR);
    if (!f)
        return;
    // Time
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "%02d.%02d.%4d %02d:%02d:%02d:%04d ",
            st.wDay, st.wMonth, st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    // Log
    fprintf_s(f, "%s\n", str);
    fflush(f);
    if (fclose(f) == EOF)
    {
        clearerr(f);
        fclose(f);
    }
    m_first = false;
}

void Log1(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    m_Logger.Log(LEVEL_DEBUG1, format, args);
    va_end(args);
}

void Log2(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);
    m_Logger.Log(LEVEL_DEBUG2, format, args);
    va_end(args);
}

void Log2(const char *str)
{
    m_Logger.Log(LEVEL_DEBUG2, str);
}

