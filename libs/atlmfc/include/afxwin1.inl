// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXWIN.H (part 1)

#pragma once

#ifdef _AFXWIN_INLINE

// Global helper functions
_AFXWIN_INLINE CWinApp* AFXAPI AfxGetApp()
	{ return afxCurrentWinApp; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetInstanceHandle()
	{ ASSERT(afxCurrentInstanceHandle != NULL);
		return afxCurrentInstanceHandle; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetResourceHandle()
	{ ASSERT(afxCurrentResourceHandle != NULL);
		return afxCurrentResourceHandle; }
_AFXWIN_INLINE void AFXAPI AfxSetResourceHandle(HINSTANCE hInstResource)
	{ ASSERT(hInstResource != NULL); afxCurrentResourceHandle = hInstResource; }
_AFXWIN_INLINE LPCTSTR AFXAPI AfxGetAppName()
	{ ASSERT(afxCurrentAppName != NULL); return afxCurrentAppName; }
_AFXWIN_INLINE COleMessageFilter* AFXAPI AfxOleGetMessageFilter()
	{ ASSERT_VALID(AfxGetThread()); return AfxGetThread()->m_pMessageFilter; }
_AFXWIN_INLINE CWnd* AFXAPI AfxGetMainWnd()
	{ CWinThread* pThread = AfxGetThread();
		return pThread != NULL ? pThread->GetMainWnd() : NULL; }

_AFXWIN_INLINE BOOL AFXAPI AfxGetAmbientActCtx()
	{ 	return afxAmbientActCtx; }
_AFXWIN_INLINE void AFXAPI AfxSetAmbientActCtx(BOOL bSet)
	{  afxAmbientActCtx = bSet; }

// exception support
_AFXWIN_INLINE CResourceException::CResourceException()
	: CSimpleException() { }
_AFXWIN_INLINE CResourceException::CResourceException(BOOL bAutoDelete, UINT nResourceID)
	: CSimpleException(bAutoDelete) { m_nResourceID = nResourceID; }
_AFXWIN_INLINE CResourceException::~CResourceException()
	{ }
_AFXWIN_INLINE CUserException::CUserException()
	: CSimpleException() { }
_AFXWIN_INLINE CUserException::CUserException(BOOL bAutoDelete, UINT nResourceID)
	: CSimpleException(bAutoDelete) { m_nResourceID = nResourceID; }
_AFXWIN_INLINE CUserException::~CUserException()
	{ }

/////////////////////////////////////////////////////////////////////////////
// CImageList

/////////////////////////////////////////////////////////////////////////////	

// CCmdUI
_AFXWIN_INLINE void CCmdUI::ContinueRouting()
	{ m_bContinueRouting = TRUE; }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXWIN_INLINE
