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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <time.h>

#include "..\Common\FarPlugin.h"
#include "..\Common\FarSettings.h"
#include "..\Common\FarDialog.h"
#include "..\Common\FarProgress.h"
#include "..\Common\FarUtil.h"

/**
 * Get system error message by error code
 * \param errCode system error code
 * \return error message
 */
wstring GetSystemErrorMessage(const DWORD errCode);

/**
 * Parse URL
 * \param url source url string
 * \param scheme scheme name
 * \param hostName host name
 * \param port port number
 * \param path path
 * \param query additional query string
 * \param userName user name
 * \param password password
 */
void ParseURL(const wchar_t *url, wstring *scheme, wstring *hostName, unsigned short *port, wstring *path, wstring *query, wstring *userName, wstring *password);

/**
 * Convert unix time to file time
 * \param t unix time
 * \return file time
 */
FILETIME UnixTimeToFileTime(const time_t t);
