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

enum LoggingLevel
{
    LEVEL_DEBUG1 = 0,
    LEVEL_DEBUG2 = 1,
};

class CLogger
{
public:
    CLogger();

    /**
     * Initialize logger
     */
    static void Initialize(bool enableLogging, int loggingLevel,
        bool logToFile, const wstring &logFileName);
    static void Shutdown();

    void Log(int level, const wchar_t *format, va_list args);
    void Log(int level, const char *str);

private:
    bool m_first;

    bool m_enableLogging;
    int m_loggingLevel;
    bool m_logToFile;
    wstring m_logFileName;
};

void Log1(const wchar_t *format, ...);
void Log2(const wchar_t *format, ...);
void Log2(const char *str);
