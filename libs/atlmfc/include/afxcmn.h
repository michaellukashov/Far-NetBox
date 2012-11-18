// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXCMN_H__
#define __AFXCMN_H__

#ifdef _AFX_NO_AFXCMN_SUPPORT
	#error Windows Common Control classes not supported in this library variant.
#endif

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif

#pragma once

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif

#ifndef IMAGE_BITMAP
#define IMAGE_BITMAP 0
#endif

#ifndef HDSIL_NORMAL
#define HDSIL_NORMAL 0
#endif

/////////////////////////////////////////////////////////////////////////////

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

#if (_WIN32_WINNT >= 0x0501)
	#include <uxtheme.h>
#endif	// _WIN32_WINNT >= 0x0501

#ifndef _AFX_NO_RICHEDIT_SUPPORT
	#ifndef _RICHEDIT_
		#include <richedit.h>
	#endif
	#ifdef __AFXOLE_H__  // only include richole if OLE support is included
		#ifndef _RICHOLE_
			#include <richole.h>
			#define _RICHOLE_
		#endif
	#else
		struct IRichEditOle;
		struct IRichEditOleCallback;
	#endif
#endif

#ifdef _AFX_ALL_WARNINGS
#pragma warning(push)
#endif

#pragma warning(disable: 4263 4264)  // base class method is hidden

/////////////////////////////////////////////////////////////////////////////
// AFXCMN - MFC COMCTL32 Control Classes

// Classes declared in this file

//TOOLINFO
	class CToolInfo;

//CObject
	//CCmdTarget;
		//CWnd
			// class CListBox;
				class CDragListBox;
			class CStatusBarCtrl;
			class CListCtrl;
			class CTreeCtrl;
			class CSpinButtonCtrl;
			class CSliderCtrl;
			class CProgressCtrl;
			// class CComboBox;
				class CComboBoxEx;
			class CHeaderCtrl;
			class CHotKeyCtrl;
			class CToolTipCtrl;
			class CTabCtrl;
			class CAnimateCtrl;
			class CToolBarCtrl;
			class CReBarCtrl;
			class CRichEditCtrl;
			class CIPAddressCtrl;
			class CPagerCtrl;
			class CLinkCtrl;

#if (NTDDI_VERSION >= NTDDI_LONGHORN) && defined(UNICODE)
			class CNetAddressCtrl;
#endif

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// CToolInfo

#ifdef _UNICODE
class CToolInfo : public tagTOOLINFOW
#else
class CToolInfo : public tagTOOLINFOA
#endif
{
public:
	TCHAR szText[256];
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ALL_WARNINGS
#pragma warning(pop)
#endif

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXCMN_INLINE AFX_INLINE
#include <afxcmn.inl>
#include <afxcmn2.inl>
#undef _AFXCMN_INLINE
#endif
#include <afxcmn3.inl>

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif //__AFXCMN_H__

/////////////////////////////////////////////////////////////////////////////
