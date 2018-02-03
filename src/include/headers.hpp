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
#include <nbtypes.h>

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
inline double ToDouble(const T a) { return static_cast<double>(a); }

template <class T>
inline Word ToWord(const T a) { return static_cast<Word>(a); }
template <class T>
inline DWORD ToDWord(const T a) { return static_cast<DWORD>(a); }

template <class T>
inline typename std::is_convertible<T, intptr_t>::value
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_integral<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_enum<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_pointer<T>::value, intptr_t>::type
ToIntPtr(T a) { return (intptr_t)(a); }

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value, intptr_t>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }
/*
inline intptr_t
ToIntPtr(int a) { return static_cast<intptr_t>(a); }

inline intptr_t
ToIntPtr(size_t a) { return static_cast<intptr_t>(a); }

inline intptr_t
ToIntPtr(int64_t a) { return static_cast<intptr_t>(a); }

inline intptr_t
ToIntPtr(DWORD a) { return static_cast<intptr_t>(a); }

template <class T>
inline typename std::enable_if<!std::is_same<T, intptr_t>::value>::type
ToIntPtr(T a) { return static_cast<intptr_t>(a); }
*/

template <class T>
inline typename std::is_convertible<T, uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_enum<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<std::is_pointer<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return (uintptr_t)(a); }

template <class T>
inline typename std::enable_if<std::is_floating_point<T>::value, uintptr_t>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

inline uintptr_t
ToUIntPtr(intptr_t a) { return static_cast<uintptr_t>(a); }

/*
inline uintptr_t
ToUIntPtr(int a) { return static_cast<uintptr_t>(a); }

inline uintptr_t
ToUIntPtr(uint32_t a) { return static_cast<uintptr_t>(a); }

inline uintptr_t
ToUIntPtr(DWORD a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<!std::is_same<T, intptr_t>::value>::type
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }

template <class T>
inline typename std::enable_if<sizeof(uintptr_t) >= sizeof(T), uintptr_t>::value
ToUIntPtr(T a) { return static_cast<uintptr_t>(a); }
*/

template <class T>
inline typename std::is_convertible<T, int>::value
ToInt(T a) { return static_cast<int>(a); }

template <class T>
inline int
ToInt(T a) { return static_cast<int>(a); }

template <class T>
inline typename std::is_convertible<T, uint32_t>::value
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline uint32_t
ToUInt32(T a) { return static_cast<uint32_t>(a); }

template <class T>
inline typename std::is_convertible<T, int64_t>::value
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline int64_t
ToInt64(T a) { return static_cast<int64_t>(a); }

template <class T>
inline typename std::is_convertible<T, void *>::value
ToPtr(T a) { return const_cast<void *>(a); }

template <class T>
inline void *ToPtr(T a) { return reinterpret_cast<void *>((intptr_t)(a)); }

template<typename T>
inline void ClearStruct(T &s) { ::ZeroMemory(&s, sizeof(s)); }

template<typename T>
inline void ClearStruct(T *s) { T dont_instantiate_this_template_with_pointers = s; }

template<typename T, size_t N>
inline void ClearArray(T (&a)[N]) { ::ZeroMemory(a, sizeof(a[0]) * N); }
