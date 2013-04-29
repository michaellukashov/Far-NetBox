// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLTRACE_H__
#define __ATLTRACE_H__

#pragma once

#include <atldef.h>
#include <atlconv.h>


#ifndef ATLTRACE
#define ATLTRACE
#define ATLTRACE2 ATLTRACE
#endif

#pragma warning(push)
#pragma warning(disable : 4793)
inline void __cdecl AtlTraceNull(...)
{
}
inline void __cdecl AtlTrace(
	_In_z_ _Printf_format_string_ LPCSTR, ...)
{
}
inline void __cdecl AtlTrace2(
	_In_ DWORD_PTR,
	_In_ UINT,
	_In_z_ _Printf_format_string_ LPCSTR, ...)
{
}
inline void __cdecl AtlTrace(
	_In_z_ _Printf_format_string_ LPCWSTR, ...)
{
}
inline void __cdecl AtlTrace2(
	_In_ DWORD_PTR,
	_In_ UINT,
	_In_z_ _Printf_format_string_ LPCWSTR, ...)
{
}
#pragma warning(pop)

#ifndef ATLTRACE

#define ATLTRACE            __noop
#define ATLTRACE2           __noop
#endif //ATLTRACE
#define ATLTRACENOTIMPL(funcname)   return E_NOTIMPL
#define DECLARE_NOUIASSERT()

#endif  // __ATLTRACE_H__
