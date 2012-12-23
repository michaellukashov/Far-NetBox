// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXEXT_H__
#define __AFXEXT_H__

#pragma once

#ifndef __AFXWIN_H__
	#include <afxwin.h>
#endif
#ifndef __AFXDLGS_H__
	#include <afxdlgs.h>
#endif

#include <uxtheme.h>

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, off)
#endif 

#ifdef _AFX_PACKING
#pragma pack(push, _AFX_PACKING)
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXEXT - MFC Advanced Extensions and Advanced Customizable classes

// Classes declared in this file

//CObject
	//CCmdTarget;
		//CWnd
			//CButton
				class CBitmapButton;    // Bitmap button (self-draw)

			class CControlBar;          // control bar
				class CStatusBar;       // status bar
				class CToolBar;         // toolbar
				class CDialogBar;       // dialog as control bar
				class CReBar;			// ie40 dock bar

			class CSplitterWnd;         // splitter manager

			//CView
				//CScrollView
				class CFormView;        // view with a dialog template
				class CEditView;        // simple text editor view

	//CDC
		class CMetaFileDC;              // a metafile with proxy

class CRectTracker;                     // tracker for rectangle objects

// information structures
struct CPrintInfo;          // Printing context
struct CPrintPreviewState;  // Print Preview context/state
struct CCreateContext;      // Creation context

#undef AFX_DATA
#define AFX_DATA AFX_CORE_DATA

/////////////////////////////////////////////////////////////////////////////
// Control Bars

// Layout Modes for CalcDynamicLayout
#define LM_STRETCH  0x01    // same meaning as bStretch in CalcFixedLayout.  If set, ignores nLength
							// and returns dimensions based on LM_HORZ state, otherwise LM_HORZ is used
							// to determine if nLength is the desired horizontal or vertical length
							// and dimensions are returned based on nLength
#define LM_HORZ     0x02    // same as bHorz in CalcFixedLayout
#define LM_MRUWIDTH 0x04    // Most Recently Used Dynamic Width
#define LM_HORZDOCK 0x08    // Horizontal Docked Dimensions
#define LM_VERTDOCK 0x10    // Vertical Docked Dimensions
#define LM_LENGTHY  0x20    // Set if nLength is a Height instead of a Width
#define LM_COMMIT   0x40    // Remember MRUWidth

// Styles for status bar panes
#define SBPS_NORMAL     0x0000
#define SBPS_NOBORDERS  SBT_NOBORDERS
#define SBPS_POPOUT     SBT_POPOUT
#define SBPS_OWNERDRAW  SBT_OWNERDRAW
#define SBPS_DISABLED   0x04000000
#define SBPS_STRETCH    0x08000000  // stretch to fill status bar

/////////////////////////////////////////////////////////////////////////////
// CMetaFileDC

class CMetaFileDC : public CDC
{
	DECLARE_DYNAMIC(CMetaFileDC)

// Constructors
public:
	CMetaFileDC();
	BOOL Create(LPCTSTR lpszFilename = NULL);
	BOOL CreateEnhanced(CDC* pDCRef, LPCTSTR lpszFileName,
		LPCRECT lpBounds, LPCTSTR lpszDescription);

// Operations
	HMETAFILE Close();
	HENHMETAFILE CloseEnhanced();

// Implementation
public:
	virtual void SetAttribDC(HDC hDC);  // Set the Attribute DC

protected:
	virtual void SetOutputDC(HDC hDC);  // Set the Output DC -- Not allowed
	virtual void ReleaseOutputDC();     // Release the Output DC -- Not allowed

public:
	virtual ~CMetaFileDC();

// Clipping Functions (use the Attribute DC's clip region)
	virtual int GetClipBox(LPRECT lpRect) const;
	virtual BOOL PtVisible(int x, int y) const;
			BOOL PtVisible(POINT point) const;
	virtual BOOL RectVisible(LPCRECT lpRect) const;

// Text Functions
	virtual BOOL TextOut(int x, int y, LPCTSTR lpszString, int nCount);
			BOOL TextOut(int x, int y, const CString& str);
	virtual BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
				LPCTSTR lpszString, UINT nCount, LPINT lpDxWidths);
			BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
				const CString& str, LPINT lpDxWidths);
	virtual CSize TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount,
				int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
			CSize TabbedTextOut(int x, int y, const CString& str,
				int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
#pragma push_macro("DrawText")
#pragma push_macro("DrawTextEx")
#undef DrawText
#undef DrawTextEx
	virtual int _AFX_FUNCNAME(DrawText)(LPCTSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat);
			int _AFX_FUNCNAME(DrawText)(const CString& str, LPRECT lpRect, UINT nFormat);

	virtual int _AFX_FUNCNAME(DrawTextEx)(_In_count_(nCount) LPTSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat, LPDRAWTEXTPARAMS lpDTParams);
			int _AFX_FUNCNAME(DrawTextEx)(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams);


			int DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat);
			int DrawText(const CString& str, LPRECT lpRect, UINT nFormat);

			int DrawTextEx(_In_count_(nCount) LPTSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat, LPDRAWTEXTPARAMS lpDTParams);
			int DrawTextEx(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams);
#pragma pop_macro("DrawText")
#pragma pop_macro("DrawTextEx")

// Printer Escape Functions
	virtual int Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData);

	virtual CSize SetViewportExt(int x, int y);
			CSize SetViewportExt(SIZE size);
	virtual CSize ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom);

protected:
	void AdjustCP(int cx);
};

struct CCreateContext   // Creation information structure
	// All fields are optional and may be NULL
{
	// for creating new views
	CRuntimeClass* m_pNewViewClass; // runtime class of view to create or NULL

	// for creating MDI children (CMDIChildWnd::LoadFrame)
	CDocTemplate* m_pNewDocTemplate;

	// for sharing view/frame state from the original view/frame

// Implementation
	CCreateContext();
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_PACKING
#pragma pack(pop)
#endif

#ifdef _AFX_ENABLE_INLINES
#define _AFXEXT_INLINE AFX_INLINE
#include <afxext.inl>
#endif

#undef AFX_DATA
#define AFX_DATA

#ifdef _AFX_MINREBUILD
#pragma component(minrebuild, on)
#endif

#endif //__AFXEXT_H__

/////////////////////////////////////////////////////////////////////////////
