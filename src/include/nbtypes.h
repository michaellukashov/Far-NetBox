#pragma once

#pragma push_macro("min")
#pragma push_macro("max")
#undef min
#undef max

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 1
#endif
#ifndef _WINSOCKAPI_
#if !defined(__MINGW32__)
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#endif // defined(__MINGW32__)
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

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
constexpr bool True = true;
#endif
#ifndef False
constexpr bool False = false;
#endif
#ifndef Integer
using Integer = intptr_t;
#endif
#ifndef Int64
using Int64 = int64_t;
#endif
#ifndef Boolean
using Boolean = bool;
#endif
#ifndef Word
using Word = WORD;
#endif

#define NullToEmptyA(s) ((s) ? (s) : "")
#define NullToEmpty(s) ((s) ? (s) : L"")


#pragma pop_macro("min")
#pragma pop_macro("max")
