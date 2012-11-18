// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXEXT.H

#pragma once

#ifdef _AFXEXT_INLINE

// CCreateContext
_AFXEXT_INLINE CCreateContext::CCreateContext()
	{ memset(this, 0, sizeof(*this)); }

// CMetaFileDC
_AFXEXT_INLINE BOOL CMetaFileDC::Create(LPCTSTR lpszFilename)
	{ return Attach(::CreateMetaFile(lpszFilename)); }
_AFXEXT_INLINE HMETAFILE CMetaFileDC::Close()
	{ return ::CloseMetaFile(Detach()); }
_AFXEXT_INLINE BOOL CMetaFileDC::CreateEnhanced(CDC* pDCRef,
		LPCTSTR lpszFileName, LPCRECT lpBounds, LPCTSTR lpszDescription)
	{ return Attach(::CreateEnhMetaFile(pDCRef->GetSafeHdc(),
		lpszFileName, lpBounds, lpszDescription)); }
_AFXEXT_INLINE HENHMETAFILE CMetaFileDC::CloseEnhanced()
	{ return ::CloseEnhMetaFile(Detach()); }
_AFXEXT_INLINE CPoint CMetaFileDC::SetViewportOrg(POINT point)
	{ ASSERT(m_hDC != NULL); return SetViewportOrg(point.x, point.y); }
_AFXEXT_INLINE CSize CMetaFileDC::SetViewportExt(SIZE size)
	{ ASSERT(m_hDC != NULL); return SetViewportExt(size.cx, size.cy); }
_AFXEXT_INLINE BOOL CMetaFileDC::TextOut(int x, int y, const CString& str)
	{ ASSERT(m_hDC != NULL); return TextOut(x, y, (LPCTSTR)str, (int)str.GetLength()); }
_AFXEXT_INLINE BOOL CMetaFileDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
	const CString& str, LPINT lpDxWidths)
	{ ASSERT(m_hDC != NULL); return ::ExtTextOut(m_hDC, x, y, nOptions, lpRect,
		str, (UINT)str.GetLength(), lpDxWidths); }
_AFXEXT_INLINE CSize CMetaFileDC::TabbedTextOut(int x, int y, const CString& str,
	int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin)
	{ ASSERT(m_hDC != NULL); return ::TabbedTextOut(m_hDC, x, y, str, (int)str.GetLength(),
		nTabPositions, lpnTabStopPositions, nTabOrigin); }
#pragma push_macro("DrawText")
#pragma push_macro("DrawTextEx")
#undef DrawText
#undef DrawTextEx
_AFXEXT_INLINE int CMetaFileDC::_AFX_FUNCNAME(DrawText)(const CString& str, LPRECT lpRect, UINT nFormat)
	{ ASSERT(m_hDC != NULL);
		return _AFX_FUNCNAME(DrawText)((LPCTSTR)str, (int)str.GetLength(), lpRect, nFormat); }
_AFXEXT_INLINE int CMetaFileDC::_AFX_FUNCNAME(DrawTextEx)(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ ASSERT(m_hDC != NULL);
		return _AFX_FUNCNAME(DrawTextEx)(const_cast<LPTSTR>((LPCTSTR)str), (int)str.GetLength(), lpRect, nFormat, lpDTParams); }
_AFXEXT_INLINE int CMetaFileDC::DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat)
	{ return _AFX_FUNCNAME(DrawText)(lpszString, nCount, lpRect, nFormat); }
_AFXEXT_INLINE int CMetaFileDC::DrawText(const CString& str, LPRECT lpRect, UINT nFormat)
	{ return _AFX_FUNCNAME(DrawText)(str, lpRect, nFormat); }
_AFXEXT_INLINE int CMetaFileDC::DrawTextEx(LPTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ return _AFX_FUNCNAME(DrawTextEx)(lpszString, nCount, lpRect, nFormat, lpDTParams); }
_AFXEXT_INLINE int CMetaFileDC::DrawTextEx(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ return _AFX_FUNCNAME(DrawTextEx)(str, lpRect, nFormat, lpDTParams); }
#pragma pop_macro("DrawText")
#pragma pop_macro("DrawTextEx")

_AFXEXT_INLINE BOOL CMetaFileDC::PtVisible(POINT point) const
	{ ASSERT(m_hDC != NULL); return PtVisible(point.x, point.y); }

// obsolete functions
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(pop)

#endif //_AFXEXT_INLINE

/////////////////////////////////////////////////////////////////////////////
