#pragma once

/*
headers.hpp

Стандартные заголовки
*/
/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 1
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define SECURITY_WIN32
#include <windows.h>
#include <tchar.h>

#include "disable_warnings_in_std_begin.hpp"
#include <nbglobals.h>

#include <memory>
#include <new>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cwchar>
#include <functional>

#undef _W32API_OLD

#ifdef _MSC_VER
# include <sdkddkver.h>
# if _WIN32_WINNT < 0x0501
#  error Windows SDK v7.0 (or higher) required
# endif
#endif //_MSC_VER

#include "disable_warnings_in_std_end.hpp"

#ifndef True
#define True true
#endif
#ifndef False
#define False false
#endif
#ifndef Integer
typedef intptr_t Integer;
#endif
#ifndef Int64
typedef int64_t Int64;
#endif
#ifndef Boolean
typedef bool Boolean;
#endif
#ifndef Word
typedef WORD Word;
#endif

#define NullToEmptyA(s) (s ? s : "")
#define NullToEmpty(s) (s ? s : L"")

template <class T>
inline const T Min(const T a, const T b) { return a < b ? a : b; }

template <class T>
inline const T Max(const T a, const T b) { return a > b ? a : b; }

template <class T>
inline const T Round(const T a, const T b) { return a / b + (a % b * 2 > b ? 1 : 0); }

template <class T>
inline void * ToPtr(const T a) { return reinterpret_cast<void *>(a); }

template <class T>
inline double ToDouble(const T a) { return static_cast<double>(a); }

template <class T>
inline Word ToWord(const T a) { return static_cast<Word>(a); }

template<typename T>
inline void ClearStruct(T & s) { ::ZeroMemory(&s, sizeof(s)); }

template<typename T>
inline void ClearStruct(T * s) { T dont_instantiate_this_template_with_pointers = s; }

template<typename T, size_t N>
inline void ClearArray(T (&a)[N]) { ::ZeroMemory(a, sizeof(a[0]) * N); }
