// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXDTCTL_H__
#define __AFXDTCTL_H__

#pragma once

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#ifndef __AFXDISP_H__
	#include <afxdisp.h>
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

//CObject
	//CCmdTarget;
		//CWnd
			class CMonthCalCtrl;
			class CDateTimeCtrl;

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXDTCTL_INLINE AFX_INLINE
#include <afxdtctl.inl>
#undef _AFXDTCTL_INLINE
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif //__AFXDTCTL_H__

/////////////////////////////////////////////////////////////////////////////
