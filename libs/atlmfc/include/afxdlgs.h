// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXDLGS_H__
#define __AFXDLGS_H__

#pragma once

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#include <objbase.h>

#ifndef _INC_COMMDLG
//	#include <commdlg.h>    // common dialog APIs
#endif

#if WINVER >= 0x0600
#ifndef _WIN32_IE
#define _WIN32_IE 0x0700
#else
#undef _WIN32_IE
#define _WIN32_IE 0x0700
#endif
#ifndef __shobjidl_h__
	// #include <shobjidl.h>    // for IFileDialog/IFileOpenDialog/IFileSaveDialog
#endif
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

#ifndef _AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////
// Win32 libraries

#endif //!_AFX_NOFORCE_LIBS

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXDLGS - MFC Standard dialogs

// Classes declared in this file

/////////////////////////////////////////////////////////////////////////////

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA


/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif //__AFXDLGS_H__

/////////////////////////////////////////////////////////////////////////////
