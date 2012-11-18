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

AFX_INLINE COLORREF CDC::GetDCBrushColor() const
	{ ENSURE(m_hDC != NULL); return ::GetDCBrushColor(m_hDC); }
AFX_INLINE COLORREF CDC::SetDCBrushColor(COLORREF crColor)
	{ ENSURE(m_hDC != NULL); return ::SetDCBrushColor(m_hDC, crColor); }

AFX_INLINE COLORREF CDC::GetDCPenColor() const
	{ ENSURE(m_hDC != NULL); return ::GetDCPenColor(m_hDC); }
AFX_INLINE COLORREF CDC::SetDCPenColor(COLORREF crColor)
	{ ENSURE(m_hDC != NULL); return ::SetDCPenColor(m_hDC, crColor); }

AFX_INLINE BOOL CDC::GetCharABCWidthsI(UINT giFirst, UINT cgi, LPWORD pgi, LPABC lpabc) const
	{ ENSURE(m_hDC != NULL); return ::GetCharABCWidthsI(m_hDC, giFirst, cgi, pgi, lpabc); }
AFX_INLINE BOOL CDC::GetCharWidthI(UINT giFirst, UINT cgi, LPWORD pgi, LPINT lpBuffer) const
	{ ENSURE(m_hDC != NULL); return ::GetCharWidthI(m_hDC, giFirst, cgi, pgi, lpBuffer); }

AFX_INLINE BOOL CDC::GetTextExtentExPointI(LPWORD pgiIn, int cgi, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize) const
{
	ENSURE(lpSize != NULL);
	ENSURE(m_hDC != NULL);
	return ::GetTextExtentExPointI(m_hDC, pgiIn, cgi, nMaxExtent, lpnFit, alpDx, lpSize);
}
AFX_INLINE BOOL CDC::GetTextExtentPointI(LPWORD pgiIn, int cgi, LPSIZE lpSize) const
{
	ENSURE(lpSize != NULL);
	ENSURE(m_hDC != NULL);
	return ::GetTextExtentPointI(m_hDC, pgiIn, cgi, lpSize);
}

#endif

#if(WINVER >= 0x0500)

AFX_INLINE BOOL CWnd::GetWindowInfo(PWINDOWINFO pwi) const
	{ ASSERT(::IsWindow(m_hWnd)); return ::GetWindowInfo(m_hWnd, pwi); }

AFX_INLINE CWnd* CWnd::GetAncestor(UINT gaFlags) const
	{ ASSERT(::IsWindow(m_hWnd)); return  CWnd::FromHandle(::GetAncestor(m_hWnd, gaFlags)); }

AFX_INLINE BOOL CWnd::GetScrollBarInfo(LONG idObject, PSCROLLBARINFO psbi) const
{
	ASSERT(::IsWindow(m_hWnd));
	ENSURE(psbi != NULL);
	return ::GetScrollBarInfo(m_hWnd, idObject, psbi);
}

AFX_INLINE BOOL CWnd::GetTitleBarInfo(PTITLEBARINFO pti) const
{
	ASSERT(::IsWindow(m_hWnd));
	ENSURE(pti != NULL);
	return ::GetTitleBarInfo(m_hWnd, pti);
}

AFX_INLINE BOOL CWnd::AnimateWindow(DWORD dwTime, DWORD dwFlags)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::AnimateWindow(m_hWnd, dwTime, dwFlags);
}

AFX_INLINE BOOL CWnd::FlashWindowEx(DWORD dwFlags, UINT  uCount, DWORD dwTimeout)
{
	ASSERT(::IsWindow(m_hWnd));
	FLASHWINFO fwi;
	fwi.cbSize = sizeof(fwi);
	fwi.hwnd = m_hWnd;
	fwi.dwFlags = dwFlags;
	fwi.uCount = uCount;
	fwi.dwTimeout = dwTimeout;

	return ::FlashWindowEx(&fwi);
}

#endif	// WINVER >= 0x0500

#if(_WIN32_WINNT >= 0x0500)

AFX_INLINE BOOL CWnd::SetLayeredWindowAttributes(COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::SetLayeredWindowAttributes(m_hWnd, crKey, bAlpha, dwFlags);
}

AFX_INLINE BOOL CWnd::UpdateLayeredWindow(CDC* pDCDst, POINT *pptDst, SIZE *psize,
		CDC* pDCSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::UpdateLayeredWindow(m_hWnd, pDCDst->GetSafeHdc(), pptDst, psize,
		pDCSrc->GetSafeHdc(), pptSrc, crKey, pblend, dwFlags);
}

#endif	// _WIN32_WINNT >= 0x0500

#if(_WIN32_WINNT >= 0x0501)

AFX_INLINE BOOL CWnd::GetLayeredWindowAttributes(COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags) const
{
	ASSERT(::IsWindow(m_hWnd));
	return ::GetLayeredWindowAttributes(m_hWnd, pcrKey, pbAlpha, pdwFlags);
}

AFX_INLINE BOOL CWnd::PrintWindow(CDC* pDC, UINT nFlags) const
{
	ASSERT(::IsWindow(m_hWnd));
	return ::PrintWindow(m_hWnd, pDC->GetSafeHdc(), nFlags);
}


#endif
