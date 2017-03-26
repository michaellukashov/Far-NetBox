// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// afxv_w32.h - target version/configuration control for Win32

#pragma once

#ifdef _WINDOWS_
	#error WINDOWS.H already included.  MFC apps must not #include <windows.h>
#endif

// STRICT is the only supported option (NOSTRICT is no longer supported)
#ifndef STRICT
#define STRICT 1
#endif

// WinSDKVer.h contains the definition for _WIN32_WINNT_MAXVER (and other maximums).
#if !defined(__MINGW32__)
#include <winsdkver.h>
#else

#define _WIN32_MAXVER           0x0501
#define _WIN32_WINDOWS_MAXVER   0x0501
#define NTDDI_MAXVER            0x05010000
#define _WIN32_IE_MAXVER        0x0700
#define _WIN32_WINNT_MAXVER     0x0701
#define WINVER_MAXVER           0x0701

#define _In_
#define _In_z_
#define _Ret_z_
#define _Inout_
#define _Post_z_
#define _Inout_opt_
#define _Inout_opt_z_
#define _In_opt_z_
#define _Check_return_
#define _Out_opt_
#define _Inout_z_
#define _Ret_opt_z_
#define _Printf_format_string_
#define ClassesAllowedInStream
#define _Deref_pre_maybenull_
#define _Deref_post_maybenull_
#define _Deref_out_opt_
#define _Deref_out_opt_z_
#define _Deref_out_
#define _Deref_out_z_
#define _Deref_post_opt_valid_
#define _Deref_opt_out_
#define _Deref_pre_z_

#define _Pre_notnull_
#define __format_string

#define _In_opt_count_(x)
#define _Ret_opt_z_cap_(x)
#define _Ret_opt_bytecap_(x)
#define _Ret_opt_bytecap_x_(x)
#define _Ret_opt_bytecount_x_(x)
#define _Ret_opt_count_(x)
#define _In_bytecount_(x)
#define _Deref_post_cap_(x)
#define _Inout_cap_(x)
#define _Out_z_cap_(x)
#define _Out_z_capcount_(x)
#define _Out_opt_cap_(x)
#define _Out_opt_z_cap_(x)
#define _Inout_z_cap_(x)
#define _Inout_z_bytecap_(x)
#define _In_z_count_(x)
#define _In_z_count_c_(x)
#define _In_count_(x)
#define _In_opt_bytecount_(x)
#define _Prepost_opt_bytecount_x_(x)
#define _Post_z_count_(x)
#define _Out_bytecapcount_(x)
#define _Inout_opt_bytecap_(x)
#define _Ret_cap_(x)
#define _Ret_count_x_(x)
#define _Out_cap_(x)
#define _Out_capcount_(x)
#define _In_opt_z_count_(x)
#define _Out_z_cap_c_(x)
#define _Inout_z_cap_c_(x)
#define _Out_bytecap_(x)
#define _Success_(x)

#define _Out_bytecap_post_bytecount_(x, n)
#define _Out_bytecap_post_bytecount_(x, n)
#define _Out_cap_post_count_(x, n)
#define _Out_z_cap_post_count_(x, n)
#define _Out_opt_z_cap_post_count_(x, n)

#endif

#ifndef _WIN32_WINNT
#ifdef WINVER
#define _WIN32_WINNT WINVER
#else
#pragma message("_WIN32_WINNT not defined. Defaulting to _WIN32_WINNT_MAXVER (see WinSDKVer.h)")
#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#endif
#else
#if _WIN32_WINNT < 0x0400
#error MFC requires _WIN32_WINNT to be #defined to 0x0400 or greater
#endif
#endif

// SDKDDKVer.h will set any of WINVER, NTDDI_VERSION and _WIN32_IE that are yet unset.
#include <sdkddkver.h>

// certain parts of WINDOWS.H are necessary
#undef NOKERNEL
#undef NOGDI
#undef NOUSER
#undef NODRIVERS
#undef NOLOGERROR
#undef NOPROFILER
#undef NOMEMMGR
#undef NOLFILEIO
#undef NOOPENFILE
#undef NORESOURCE
#undef NOATOM
#undef NOLANGUAGE
#undef NOLSTRING
#undef NODBCS
#undef NOKEYBOARDINFO
#undef NOGDICAPMASKS
#undef NOCOLOR
#undef NOGDIOBJ
#undef NODRAWTEXT
#undef NOTEXTMETRIC
#undef NOSCALABLEFONT
#undef NOBITMAP
#undef NORASTEROPS
#undef NOMETAFILE
#undef NOSYSMETRICS
#undef NOSYSTEMPARAMSINFO
#undef NOMSG
#undef NOWINSTYLES
#undef NOWINOFFSETS
#undef NOSHOWWINDOW
#undef NODEFERWINDOWPOS
#undef NOVIRTUALKEYCODES
#undef NOKEYSTATES
#undef NOWH
#undef NOMENUS
#undef NOSCROLL
#undef NOCLIPBOARD
#undef NOICONS
#undef NOMB
#undef NOSYSCOMMANDS
#undef NOMDI
#undef NOCTLMGR
#undef NOWINMESSAGES

#ifndef WIN32
#define WIN32
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _UNICODE
#ifndef UNICODE
#define UNICODE         // UNICODE is used by Windows headers
#endif
#endif

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE        // _UNICODE is used by C-runtime/MFC headers
#endif
#endif

#ifdef VC_EXTRALEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOCOMM
#define NORPC

#ifndef NO_ANSIUNI_ONLY
#ifdef _UNICODE
#define UNICODE_ONLY
#else
#define ANSI_ONLY
#endif
#endif //!NO_ANSIUNI_ONLY

#endif //VC_EXTRALEAN

/////////////////////////////////////////////////////////////////////////////
// Turn off warnings for /W4
// To resume any of these warning: #pragma warning(default: 4xxx)
// which should be placed after the AFX include files

#pragma warning(push)
#pragma warning(disable: 4311 4312)
#pragma warning(disable: 4201)  // winnt.h uses nameless structs

// Don't include winsock.h
#pragma push_macro("_WINSOCKAPI_")
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

#pragma pop_macro("_WINSOCKAPI_")

#pragma warning(pop)

// Mouse message MFC is interested in
#ifndef AFX_WM_MOUSELAST
#define AFX_WM_MOUSELAST 0x0209
#endif

#include <zmouse.h>

//struct HKEY__;
//typedef struct HKEY__ *HKEY;

#ifndef _INC_COMMCTRL
	#include <commctrl.h>
#endif

#ifndef EXPORT
#define EXPORT
#endif

#ifndef _INC_TCHAR
	#include <tchar.h>      // used for ANSI v.s. UNICODE abstraction
#endif
#ifdef _MBCS
#ifndef _INC_MBCTYPE
	#include <mbctype.h>
#endif
#ifndef _INC_MBSTRING
	#include <mbstring.h>
#endif
#endif

#ifdef _WIN64
#define _AFX_NO_CTL3D_SUPPORT
#endif

/////////////////////////////////////////////////////////////////////////////
// Now for the Windows API specific parts

// WM_CTLCOLOR for 16 bit API compatability
#define WM_CTLCOLOR     0x0019

// Win32 uses macros with parameters for this, which breaks C++ code.
#ifdef GetWindowTask
#undef GetWindowTask
AFX_INLINE HTASK GetWindowTask(HWND hWnd)
	{ return (HTASK)(DWORD_PTR)::GetWindowThreadProcessId(hWnd, NULL); }
#endif

// Win32 uses macros with parameters for this, which breaks C++ code.
#ifdef GetNextWindow
#undef GetNextWindow
AFX_INLINE HWND GetNextWindow(HWND hWnd, UINT nDirection)
	{ return ::GetWindow(hWnd, nDirection); }
#endif

// Avoid mapping CToolBar::DrawState to DrawState[A/W]
#ifdef DrawState
#undef DrawState
AFX_INLINE BOOL WINAPI DrawState(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc,
	LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT fuFlags)
#ifdef UNICODE
	{ return ::DrawStateW(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy,
		fuFlags); }
#else
	{ return ::DrawStateA(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy,
		fuFlags); }
#endif
#endif

// FreeResource is not required on Win32 platforms
#undef FreeResource
AFX_INLINE BOOL WINAPI FreeResource(HGLOBAL) { return TRUE; }
// UnlockResource is not required on Win32 platforms
#undef UnlockResource
AFX_INLINE int WINAPI UnlockResource(HGLOBAL) { return 0; }

/////////////////////////////////////////////////////////////////////////////
