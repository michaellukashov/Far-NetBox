// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Always Inline. Functions only in Win2K or later

#pragma once

#if (_WIN32_WINNT >= 0x0500)

#endif

#if(WINVER >= 0x0500)

AFX_INLINE BOOL CWnd::GetWindowInfo(PWINDOWINFO pwi) const
	{ ASSERT(::IsWindow(m_hWnd)); return ::GetWindowInfo(m_hWnd, pwi); }

AFX_INLINE CWnd* CWnd::GetAncestor(UINT gaFlags) const
	{ ASSERT(::IsWindow(m_hWnd)); return  CWnd::FromHandle(::GetAncestor(m_hWnd, gaFlags)); }

#endif	// WINVER >= 0x0500

#if(_WIN32_WINNT >= 0x0501)

AFX_INLINE BOOL CWnd::GetLayeredWindowAttributes(COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags) const
{
	ASSERT(::IsWindow(m_hWnd));
	return ::GetLayeredWindowAttributes(m_hWnd, pcrKey, pbAlpha, pdwFlags);
}


#endif
