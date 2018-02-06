#pragma once

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 1
#endif
#ifndef _WINSOCKAPI_
#if !defined(__MINGW32__)
#define _WINSOCKAPI_
#endif // defined(__MINGW32__)
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

