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
#define SECURITY_WIN32
#include <windows.h>
#include <winsock2.h>
#include <time.h>

#include <string>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>

#ifndef _export
#define _export __declspec(dllexport)
#endif  //_export

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  //_WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif  //_WIN32_IE

#pragma warning(push, 1)
#include <plugin.hpp>
#include <farkeys.hpp>
#include <farcolor.hpp>
#pragma warning(pop)
