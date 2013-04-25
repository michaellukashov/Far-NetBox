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

// CDC
_AFXWIN_INLINE CDC::operator HDC() const
	{ return this == NULL ? NULL : m_hDC; }
_AFXWIN_INLINE HDC CDC::GetSafeHdc() const
	{ return this == NULL ? NULL : m_hDC; }
_AFXWIN_INLINE CWnd* CDC::GetWindow() const
	{ ASSERT(m_hDC != NULL); return CWnd::FromHandle(::WindowFromDC(m_hDC)); }
_AFXWIN_INLINE BOOL CDC::IsPrinting() const
	{ return m_bPrinting; }
_AFXWIN_INLINE BOOL CDC::CreateDC(LPCTSTR lpszDriverName,
	LPCTSTR lpszDeviceName, LPCTSTR lpszOutput, const void* lpInitData)
	{ return Attach(::CreateDC(lpszDriverName,
		lpszDeviceName, lpszOutput, (const DEVMODE*)lpInitData)); }
_AFXWIN_INLINE BOOL CDC::CreateIC(LPCTSTR lpszDriverName,
	LPCTSTR lpszDeviceName, LPCTSTR lpszOutput, const void* lpInitData)
	{ return Attach(::CreateIC(lpszDriverName,
		lpszDeviceName, lpszOutput, (const DEVMODE*) lpInitData)); }
_AFXWIN_INLINE BOOL CDC::CreateCompatibleDC(CDC* pDC)
	{ return Attach(::CreateCompatibleDC(pDC->GetSafeHdc())); }
_AFXWIN_INLINE int CDC::ExcludeUpdateRgn(CWnd* pWnd)
	{ ASSERT(m_hDC != NULL); return ::ExcludeUpdateRgn(m_hDC, pWnd->m_hWnd); }
_AFXWIN_INLINE int CDC::GetDeviceCaps(int nIndex) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetDeviceCaps(m_hAttribDC, nIndex); }

_AFXWIN_INLINE int CDC::EnumObjects(int nObjectType,
		int (CALLBACK* lpfn)(LPVOID, LPARAM), LPARAM lpData)
	{ ASSERT(m_hAttribDC != NULL); return ::EnumObjects(m_hAttribDC, nObjectType, (GOBJENUMPROC)lpfn, lpData); }

_AFXWIN_INLINE HGDIOBJ CDC::SelectObject(HGDIOBJ hObject) // Safe for NULL handles
	{ ASSERT(m_hDC == m_hAttribDC); // ASSERT a simple CDC object
		return (hObject != NULL) ? ::SelectObject(m_hDC, hObject) : NULL; }
_AFXWIN_INLINE COLORREF CDC::GetNearestColor(COLORREF crColor) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetNearestColor(m_hAttribDC, crColor); }
_AFXWIN_INLINE UINT CDC::RealizePalette()
	{ ASSERT(m_hDC != NULL); return ::RealizePalette(m_hDC); }
_AFXWIN_INLINE void CDC::UpdateColors()
	{ ASSERT(m_hDC != NULL); ::UpdateColors(m_hDC); }
_AFXWIN_INLINE COLORREF CDC::GetBkColor() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetBkColor(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetBkMode() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetBkMode(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetPolyFillMode() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetPolyFillMode(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetROP2() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetROP2(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetStretchBltMode() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetStretchBltMode(m_hAttribDC); }
_AFXWIN_INLINE COLORREF CDC::GetTextColor() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTextColor(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetMapMode() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetMapMode(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetGraphicsMode() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetGraphicsMode(m_hAttribDC); }
_AFXWIN_INLINE BOOL CDC::GetWorldTransform(XFORM* pXform) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetWorldTransform(m_hAttribDC,pXform); }

_AFXWIN_INLINE CSize CDC::GetViewportExt() const
	{
		ASSERT(m_hAttribDC != NULL);
		SIZE size;
		VERIFY(::GetViewportExtEx(m_hAttribDC, &size));
		return size;
	}
_AFXWIN_INLINE CSize CDC::GetWindowExt() const
	{
		ASSERT(m_hAttribDC != NULL);
		SIZE size;
		VERIFY(::GetWindowExtEx(m_hAttribDC, &size));
		return size;
	}

// non-virtual helpers calling virtual mapping functions
_AFXWIN_INLINE CSize CDC::SetViewportExt(SIZE size)
	{ ASSERT(m_hDC != NULL); return SetViewportExt(size.cx, size.cy); }
_AFXWIN_INLINE CSize CDC::SetWindowExt(SIZE size)
	{ ASSERT(m_hDC != NULL); return SetWindowExt(size.cx, size.cy); }

_AFXWIN_INLINE void CDC::DPtoLP(LPPOINT lpPoints, int nCount) const
	{ ASSERT(m_hAttribDC != NULL); VERIFY(::DPtoLP(m_hAttribDC, lpPoints, nCount)); }
_AFXWIN_INLINE void CDC::DPtoLP(LPRECT lpRect) const
	{ ASSERT(m_hAttribDC != NULL); VERIFY(::DPtoLP(m_hAttribDC, (LPPOINT)lpRect, 2)); }
_AFXWIN_INLINE void CDC::LPtoDP(LPPOINT lpPoints, int nCount) const
	{ ASSERT(m_hAttribDC != NULL); VERIFY(::LPtoDP(m_hAttribDC, lpPoints, nCount)); }
_AFXWIN_INLINE void CDC::LPtoDP(LPRECT lpRect) const
	{ ASSERT(m_hAttribDC != NULL); VERIFY(::LPtoDP(m_hAttribDC, (LPPOINT)lpRect, 2)); }

_AFXWIN_INLINE BOOL CDC::PtVisible(int x, int y) const
	{ ASSERT(m_hDC != NULL); return ::PtVisible(m_hDC, x, y); }
_AFXWIN_INLINE BOOL CDC::PtVisible(POINT point) const
	{ ASSERT(m_hDC != NULL); return PtVisible(point.x, point.y); } // call virtual
_AFXWIN_INLINE BOOL CDC::RectVisible(LPCRECT lpRect) const
	{ ASSERT(m_hDC != NULL); return ::RectVisible(m_hDC, lpRect); }

_AFXWIN_INLINE BOOL CDC::LineTo(POINT point)
	{ ASSERT(m_hDC != NULL); return LineTo(point.x, point.y); }
_AFXWIN_INLINE BOOL CDC::Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{ ASSERT(m_hDC != NULL); return ::Arc(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Arc(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ ASSERT(m_hDC != NULL); return ::Arc(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }

_AFXWIN_INLINE BOOL CDC::Chord(int x1, int y1, int x2, int y2, int x3, int y3,
	int x4, int y4)
	{ ASSERT(m_hDC != NULL); return ::Chord(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Chord(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ ASSERT(m_hDC != NULL); return ::Chord(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE void CDC::DrawFocusRect(LPCRECT lpRect)
	{ ASSERT(m_hDC != NULL); ::DrawFocusRect(m_hDC, lpRect); }
_AFXWIN_INLINE BOOL CDC::Ellipse(int x1, int y1, int x2, int y2)
	{ ASSERT(m_hDC != NULL); return ::Ellipse(m_hDC, x1, y1, x2, y2); }
_AFXWIN_INLINE BOOL CDC::Ellipse(LPCRECT lpRect)
	{ ASSERT(m_hDC != NULL); return ::Ellipse(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom); }
_AFXWIN_INLINE BOOL CDC::Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{ ASSERT(m_hDC != NULL); return ::Pie(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Pie(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ ASSERT(m_hDC != NULL); return ::Pie(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE BOOL CDC::Polygon(const POINT* lpPoints, int nCount)
	{ ASSERT(m_hDC != NULL); return ::Polygon(m_hDC, lpPoints, nCount); }
_AFXWIN_INLINE BOOL CDC::PolyPolygon(const POINT* lpPoints, const INT* lpPolyCounts, int nCount)
	{ ASSERT(m_hDC != NULL); return ::PolyPolygon(m_hDC, lpPoints, lpPolyCounts, nCount); }
_AFXWIN_INLINE BOOL CDC::Rectangle(int x1, int y1, int x2, int y2)
	{ ASSERT(m_hDC != NULL); return ::Rectangle(m_hDC, x1, y1, x2, y2); }
_AFXWIN_INLINE BOOL CDC::Rectangle(LPCRECT lpRect)
	{ ASSERT(m_hDC != NULL); return ::Rectangle(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom); }
_AFXWIN_INLINE BOOL CDC::RoundRect(int x1, int y1, int x2, int y2, int x3, int y3)
	{ ASSERT(m_hDC != NULL); return ::RoundRect(m_hDC, x1, y1, x2, y2, x3, y3); }
_AFXWIN_INLINE BOOL CDC::RoundRect(LPCRECT lpRect, POINT point)
	{ ASSERT(m_hDC != NULL); return ::RoundRect(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, point.x, point.y); }
_AFXWIN_INLINE BOOL CDC::PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop)
	{ ASSERT(m_hDC != NULL); return ::PatBlt(m_hDC, x, y, nWidth, nHeight, dwRop); }
_AFXWIN_INLINE BOOL CDC::BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
	int xSrc, int ySrc, DWORD dwRop)
	{ ASSERT(m_hDC != NULL); return ::BitBlt(m_hDC, x, y, nWidth, nHeight,
		pSrcDC->GetSafeHdc(), xSrc, ySrc, dwRop); }
_AFXWIN_INLINE BOOL CDC::StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
	int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop)
	{ ASSERT(m_hDC != NULL); return ::StretchBlt(m_hDC, x, y, nWidth, nHeight,
		pSrcDC->GetSafeHdc(), xSrc, ySrc, nSrcWidth, nSrcHeight,
		dwRop); }
_AFXWIN_INLINE COLORREF CDC::GetPixel(int x, int y) const
	{ ASSERT(m_hDC != NULL); return ::GetPixel(m_hDC, x, y); }
_AFXWIN_INLINE COLORREF CDC::GetPixel(POINT point) const
	{ ASSERT(m_hDC != NULL); return ::GetPixel(m_hDC, point.x, point.y); }
_AFXWIN_INLINE COLORREF CDC::SetPixel(int x, int y, COLORREF crColor)
	{ ASSERT(m_hDC != NULL); return ::SetPixel(m_hDC, x, y, crColor); }
_AFXWIN_INLINE COLORREF CDC::SetPixel(POINT point, COLORREF crColor)
	{ ASSERT(m_hDC != NULL); return ::SetPixel(m_hDC, point.x, point.y, crColor); }
_AFXWIN_INLINE BOOL CDC::FloodFill(int x, int y, COLORREF crColor)
	{ ASSERT(m_hDC != NULL); return ::FloodFill(m_hDC, x, y, crColor); }
_AFXWIN_INLINE BOOL CDC::ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType)
	{ ASSERT(m_hDC != NULL); return ::ExtFloodFill(m_hDC, x, y, crColor, nFillType); }
_AFXWIN_INLINE BOOL CDC::TextOut(int x, int y, LPCTSTR lpszString, int nCount)
	{ ASSERT(m_hDC != NULL); return ::TextOut(m_hDC, x, y, lpszString, nCount); }
_AFXWIN_INLINE BOOL CDC::TextOut(int x, int y, const CString& str)
	{ ASSERT(m_hDC != NULL); return TextOut(x, y, (LPCTSTR)str, (int)str.GetLength()); } // call virtual
_AFXWIN_INLINE BOOL CDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
	LPCTSTR lpszString, UINT nCount, LPINT lpDxWidths)
	{ ASSERT(m_hDC != NULL); return ::ExtTextOut(m_hDC, x, y, nOptions, lpRect,
		lpszString, nCount, lpDxWidths); }
_AFXWIN_INLINE BOOL CDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
	const CString& str, LPINT lpDxWidths)
	{ ASSERT(m_hDC != NULL); return ::ExtTextOut(m_hDC, x, y, nOptions, lpRect,
		str, (UINT)str.GetLength(), lpDxWidths); }
_AFXWIN_INLINE CSize CDC::TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin)
	{ ASSERT(m_hDC != NULL); return ::TabbedTextOut(m_hDC, x, y, lpszString, nCount,
		nTabPositions, lpnTabStopPositions, nTabOrigin); }
_AFXWIN_INLINE CSize CDC::TabbedTextOut(int x, int y, const CString& str,
	int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin)
	{ ASSERT(m_hDC != NULL); return ::TabbedTextOut(m_hDC, x, y, str, (int)str.GetLength(),
		nTabPositions, lpnTabStopPositions, nTabOrigin); }
_AFXWIN_INLINE int CDC::_AFX_FUNCNAME(DrawText)(LPCTSTR lpszString, int nCount, LPRECT lpRect,
		UINT nFormat)
	{ ASSERT(m_hDC != NULL);
		return ::DrawText(m_hDC, lpszString, nCount, lpRect, nFormat); }
_AFXWIN_INLINE int CDC::_AFX_FUNCNAME(DrawText)(const CString& str, LPRECT lpRect, UINT nFormat)
	{ ASSERT(m_hDC != NULL);
		// these flags would modify the string
		ASSERT((nFormat & (DT_END_ELLIPSIS | DT_MODIFYSTRING)) != (DT_END_ELLIPSIS | DT_MODIFYSTRING));
		ASSERT((nFormat & (DT_PATH_ELLIPSIS | DT_MODIFYSTRING)) != (DT_PATH_ELLIPSIS | DT_MODIFYSTRING));
		return _AFX_FUNCNAME(DrawText)((LPCTSTR)str, (int)str.GetLength(), lpRect, nFormat); }

_AFXWIN_INLINE int CDC::_AFX_FUNCNAME(DrawTextEx)(LPTSTR lpszString, int nCount, LPRECT lpRect,
		UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ ASSERT(m_hDC != NULL);
		return ::DrawTextEx(m_hDC, lpszString, nCount, lpRect, nFormat, lpDTParams); }
_AFXWIN_INLINE int CDC::_AFX_FUNCNAME(DrawTextEx)(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ ASSERT(m_hDC != NULL);
		// these flags would modify the string
		ASSERT((nFormat & (DT_END_ELLIPSIS | DT_MODIFYSTRING)) != (DT_END_ELLIPSIS | DT_MODIFYSTRING));
		ASSERT((nFormat & (DT_PATH_ELLIPSIS | DT_MODIFYSTRING)) != (DT_PATH_ELLIPSIS | DT_MODIFYSTRING));
		return _AFX_FUNCNAME(DrawTextEx)(const_cast<LPTSTR>((LPCTSTR)str), (int)str.GetLength(), lpRect, nFormat, lpDTParams); }

#pragma push_macro("DrawText")
#pragma push_macro("DrawTextEx")
#undef DrawText
#undef DrawTextEx
_AFXWIN_INLINE int CDC::DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat)
	{ return _AFX_FUNCNAME(DrawText)(lpszString, nCount, lpRect, nFormat); }
_AFXWIN_INLINE int CDC::DrawText(const CString& str, LPRECT lpRect, UINT nFormat)
	{ return _AFX_FUNCNAME(DrawText)(str, lpRect, nFormat); }
_AFXWIN_INLINE int CDC::DrawTextEx(LPTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ return _AFX_FUNCNAME(DrawTextEx)(lpszString, nCount, lpRect, nFormat, lpDTParams); }
_AFXWIN_INLINE int CDC::DrawTextEx(const CString& str, LPRECT lpRect, UINT nFormat, LPDRAWTEXTPARAMS lpDTParams)
	{ return _AFX_FUNCNAME(DrawTextEx)(str, lpRect, nFormat, lpDTParams); }
#pragma pop_macro("DrawText")
#pragma pop_macro("DrawTextEx")

_AFXWIN_INLINE CSize CDC::GetTextExtent(LPCTSTR lpszString, int nCount) const
	{
		ASSERT(m_hAttribDC != NULL);
		SIZE size;
		VERIFY(::GetTextExtentPoint32(m_hAttribDC, lpszString, nCount, &size));
		return size;
	}
_AFXWIN_INLINE CSize CDC::GetTextExtent(const CString& str) const
	{
		ASSERT(m_hAttribDC != NULL);
		SIZE size;
		VERIFY(::GetTextExtentPoint32(m_hAttribDC, str, (int)str.GetLength(), &size));
		return size;
	}

_AFXWIN_INLINE CSize CDC::GetOutputTextExtent(LPCTSTR lpszString, int nCount) const
	{
		ASSERT(m_hDC != NULL);
		SIZE size;
		VERIFY(::GetTextExtentPoint32(m_hDC, lpszString, nCount, &size));
		return size;
	}
_AFXWIN_INLINE CSize CDC::GetOutputTextExtent(const CString& str) const
	{
		ASSERT(m_hDC != NULL);
		SIZE size;
		VERIFY(::GetTextExtentPoint32(m_hDC, str, (int)str.GetLength(), &size));
		return size;
	}

_AFXWIN_INLINE CSize CDC::GetTabbedTextExtent(LPCTSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTabbedTextExtent(m_hAttribDC, lpszString, nCount,
		nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE  CSize CDC::GetTabbedTextExtent(const CString& str,
		int nTabPositions, LPINT lpnTabStopPositions) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTabbedTextExtent(m_hAttribDC,
		str, (int)str.GetLength(), nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE CSize CDC::GetOutputTabbedTextExtent(LPCTSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions) const
	{ ASSERT(m_hDC != NULL); return ::GetTabbedTextExtent(m_hDC, lpszString, nCount,
		nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE  CSize CDC::GetOutputTabbedTextExtent(const CString& str,
		int nTabPositions, LPINT lpnTabStopPositions) const
	{ ASSERT(m_hDC != NULL); return ::GetTabbedTextExtent(m_hDC,
		str, (int)str.GetLength(), nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE UINT CDC::GetTextAlign() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTextAlign(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetTextFace(_In_ int nCount, _Out_z_cap_post_count_(nCount, return) LPTSTR lpszFacename) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTextFace(m_hAttribDC, nCount, lpszFacename); }
_AFXWIN_INLINE  int CDC::GetTextFace(CString& rString) const
	{ ASSERT(m_hAttribDC != NULL); int nResult = ::GetTextFace(m_hAttribDC,
		256, rString.GetBuffer(256)); rString.ReleaseBuffer();
		return nResult; }
_AFXWIN_INLINE BOOL CDC::_AFX_FUNCNAME(GetTextMetrics)(LPTEXTMETRIC lpMetrics) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTextMetrics(m_hAttribDC, lpMetrics); }
#pragma push_macro("GetTextMetrics")
#undef GetTextMetrics
_AFXWIN_INLINE BOOL CDC::GetTextMetrics(LPTEXTMETRIC lpMetrics) const
	{ return _AFX_FUNCNAME(GetTextMetrics)(lpMetrics); }
#pragma pop_macro("GetTextMetrics")
_AFXWIN_INLINE BOOL CDC::GetOutputTextMetrics(LPTEXTMETRIC lpMetrics) const
	{ ASSERT(m_hDC != NULL); return ::GetTextMetrics(m_hDC, lpMetrics); }
_AFXWIN_INLINE int CDC::GetTextCharacterExtra() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetTextCharacterExtra(m_hAttribDC); }
_AFXWIN_INLINE BOOL CDC::GetCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetCharWidth(m_hAttribDC, nFirstChar, nLastChar, lpBuffer); }
_AFXWIN_INLINE BOOL CDC::GetOutputCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const
	{ ASSERT(m_hDC != NULL); return ::GetCharWidth(m_hDC, nFirstChar, nLastChar, lpBuffer); }
_AFXWIN_INLINE DWORD CDC::GetFontLanguageInfo() const
	{ ASSERT(m_hDC != NULL); return ::GetFontLanguageInfo(m_hDC); }

_AFXWIN_INLINE DWORD CDC::GetCharacterPlacement(LPCTSTR lpString, int nCount, int nMaxExtent, LPGCP_RESULTS lpResults, DWORD dwFlags) const
	{ ASSERT(m_hDC != NULL); return ::GetCharacterPlacement(m_hDC, lpString, nCount, nMaxExtent, lpResults, dwFlags); }
_AFXWIN_INLINE DWORD CDC::GetCharacterPlacement(CString& str, int nMaxExtent, LPGCP_RESULTS lpResults, DWORD dwFlags) const
	{ ASSERT(m_hDC != NULL); return ::GetCharacterPlacement(m_hDC, (LPCTSTR)str, str.GetLength(), nMaxExtent, lpResults, dwFlags); }


_AFXWIN_INLINE CSize CDC::GetAspectRatioFilter() const
	{
		ASSERT(m_hAttribDC != NULL);
		SIZE size;
		VERIFY(::GetAspectRatioFilterEx(m_hAttribDC, &size));
		return size;
	}
// Printer Escape Functions
_AFXWIN_INLINE int CDC::Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData)
	{ ASSERT(m_hDC != NULL); return ::Escape(m_hDC, nEscape, nCount, lpszInData, lpOutData);}

// CDC 3.1 Specific functions
_AFXWIN_INLINE UINT CDC::SetBoundsRect(LPCRECT lpRectBounds, UINT flags)
	{ ASSERT(m_hDC != NULL); return ::SetBoundsRect(m_hDC, lpRectBounds, flags); }
_AFXWIN_INLINE UINT CDC::GetBoundsRect(LPRECT lpRectBounds, UINT flags)
	{ ASSERT(m_hAttribDC != NULL); return ::GetBoundsRect(m_hAttribDC, lpRectBounds, flags); }
_AFXWIN_INLINE BOOL CDC::ResetDC(const DEVMODE* lpDevMode)
	{ ASSERT(m_hAttribDC != NULL); return ::ResetDC(m_hAttribDC, lpDevMode) != NULL; }
_AFXWIN_INLINE UINT CDC::GetOutlineTextMetrics(UINT cbData, LPOUTLINETEXTMETRIC lpotm) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetOutlineTextMetrics(m_hAttribDC, cbData, lpotm); }
_AFXWIN_INLINE BOOL CDC::GetCharABCWidths(UINT nFirstChar, UINT nLastChar, LPABC lpabc) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetCharABCWidths(m_hAttribDC, nFirstChar, nLastChar, lpabc); }
_AFXWIN_INLINE DWORD CDC::GetFontData(DWORD dwTable, DWORD dwOffset, LPVOID lpData,
	DWORD cbData) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetFontData(m_hAttribDC, dwTable, dwOffset, lpData, cbData); }
_AFXWIN_INLINE int CDC::GetKerningPairs(int nPairs, LPKERNINGPAIR lpkrnpair) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetKerningPairs(m_hAttribDC, nPairs, lpkrnpair); }
_AFXWIN_INLINE DWORD CDC::GetGlyphOutline(UINT nChar, UINT nFormat, LPGLYPHMETRICS lpgm,
		DWORD cbBuffer, LPVOID lpBuffer, const MAT2* lpmat2) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetGlyphOutline(m_hAttribDC, nChar, nFormat,
			lpgm, cbBuffer, lpBuffer, lpmat2); }

// Document handling functions
_AFXWIN_INLINE int CDC::StartDoc(LPDOCINFO lpDocInfo)
	{ ASSERT(m_hDC != NULL); return ::StartDoc(m_hDC, lpDocInfo); }
_AFXWIN_INLINE int CDC::StartPage()
	{ ASSERT(m_hDC != NULL); return ::StartPage(m_hDC); }
_AFXWIN_INLINE int CDC::EndPage()
	{ ASSERT(m_hDC != NULL); return ::EndPage(m_hDC); }
_AFXWIN_INLINE int CDC::SetAbortProc(BOOL (CALLBACK* lpfn)(HDC, int))
	{ ASSERT(m_hDC != NULL); return ::SetAbortProc(m_hDC, (ABORTPROC)lpfn); }
_AFXWIN_INLINE int CDC::AbortDoc()
	{ ASSERT(m_hDC != NULL); return ::AbortDoc(m_hDC); }
_AFXWIN_INLINE int CDC::EndDoc()
	{ ASSERT(m_hDC != NULL); return ::EndDoc(m_hDC); }

_AFXWIN_INLINE BOOL CDC::SetPixelV(int x, int y, COLORREF crColor)
	{ ASSERT(m_hDC != NULL); return ::SetPixelV(m_hDC, x, y, crColor); }
_AFXWIN_INLINE BOOL CDC::SetPixelV(POINT point, COLORREF crColor)
	{ ASSERT(m_hDC != NULL); return ::SetPixelV(m_hDC, point.x, point.y, crColor); }
_AFXWIN_INLINE BOOL CDC::AngleArc(int x, int y, int nRadius,
		float fStartAngle, float fSweepAngle)
	{ ASSERT(m_hDC != NULL); return ::AngleArc(m_hDC, x, y, nRadius, fStartAngle, fSweepAngle); }
// _AFXWIN_INLINE BOOL CDC::ArcTo(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	// { ASSERT(m_hDC != NULL); return ArcTo(lpRect->left, lpRect->top, lpRect->right,
		// lpRect->bottom, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE int CDC::GetArcDirection() const
	{ ASSERT(m_hAttribDC != NULL); return ::GetArcDirection(m_hAttribDC); }
_AFXWIN_INLINE BOOL CDC::PolyPolyline(const POINT* lpPoints, const DWORD* lpPolyPoints,
		int nCount)
	{ ASSERT(m_hDC != NULL); return ::PolyPolyline(m_hDC, lpPoints, lpPolyPoints, nCount); }
_AFXWIN_INLINE BOOL CDC::GetColorAdjustment(LPCOLORADJUSTMENT lpColorAdjust) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetColorAdjustment(m_hAttribDC, lpColorAdjust); }

_AFXWIN_INLINE int CDC::DrawEscape(int nEscape, int nInputSize, LPCSTR lpszInputData)
	{ ASSERT(m_hDC != NULL); return ::DrawEscape(m_hDC, nEscape, nInputSize, lpszInputData); }
_AFXWIN_INLINE int CDC::Escape(_In_ int nEscape, _In_ int nInputSize, _In_bytecount_(nInputSize) LPCSTR lpszInputData,
		_In_ int nOutputSize, _Out_bytecap_(nOutputSize) LPSTR lpszOutputData)
	{ ASSERT(m_hDC != NULL); return ::ExtEscape(m_hDC, nEscape, nInputSize, lpszInputData,
		nOutputSize, lpszOutputData); }

_AFXWIN_INLINE BOOL CDC::GetCharABCWidths(UINT nFirstChar, UINT nLastChar,
		LPABCFLOAT lpABCF) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetCharABCWidthsFloat(m_hAttribDC, nFirstChar, nLastChar, lpABCF); }
_AFXWIN_INLINE BOOL CDC::GetCharWidth(UINT nFirstChar, UINT nLastChar,
		float* lpFloatBuffer) const
	{ ASSERT(m_hAttribDC != NULL); return ::GetCharWidthFloat(m_hAttribDC, nFirstChar, nLastChar, lpFloatBuffer); }

_AFXWIN_INLINE BOOL CDC::AbortPath()
	{ ASSERT(m_hDC != NULL); return ::AbortPath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::BeginPath()
	{ ASSERT(m_hDC != NULL); return ::BeginPath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::CloseFigure()
	{ ASSERT(m_hDC != NULL); return ::CloseFigure(m_hDC); }
_AFXWIN_INLINE BOOL CDC::EndPath()
	{ ASSERT(m_hDC != NULL); return ::EndPath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::FillPath()
	{ ASSERT(m_hDC != NULL); return ::FillPath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::FlattenPath()
	{ ASSERT(m_hDC != NULL); return ::FlattenPath(m_hDC); }
_AFXWIN_INLINE float CDC::GetMiterLimit() const
	{ ASSERT(m_hDC != NULL); float fMiterLimit;
		VERIFY(::GetMiterLimit(m_hDC, &fMiterLimit)); return fMiterLimit; }
_AFXWIN_INLINE int CDC::GetPath(LPPOINT lpPoints, LPBYTE lpTypes, int nCount) const
	{ ASSERT(m_hDC != NULL); return ::GetPath(m_hDC, lpPoints, lpTypes, nCount); }
_AFXWIN_INLINE BOOL CDC::SetMiterLimit(float fMiterLimit)
	{ ASSERT(m_hDC != NULL); return ::SetMiterLimit(m_hDC, fMiterLimit, NULL); }
_AFXWIN_INLINE BOOL CDC::StrokeAndFillPath()
	{ ASSERT(m_hDC != NULL); return ::StrokeAndFillPath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::StrokePath()
	{ ASSERT(m_hDC != NULL); return ::StrokePath(m_hDC); }
_AFXWIN_INLINE BOOL CDC::WidenPath()
	{ ASSERT(m_hDC != NULL); return ::WidenPath(m_hDC); }

_AFXWIN_INLINE BOOL CDC::AddMetaFileComment(UINT nDataSize, const BYTE* pCommentData)
	{ ASSERT(m_hDC != NULL); return ::GdiComment(m_hDC, nDataSize, pCommentData); }
_AFXWIN_INLINE BOOL CDC::PlayMetaFile(HENHMETAFILE hEnhMF, LPCRECT lpBounds)
	{ ASSERT(m_hDC != NULL); return ::PlayEnhMetaFile(m_hDC, hEnhMF, lpBounds); }

/////////////////////////////////////////////////////////////////////////////
// CImageList

/////////////////////////////////////////////////////////////////////////////	

// CCmdUI
_AFXWIN_INLINE void CCmdUI::ContinueRouting()
	{ m_bContinueRouting = TRUE; }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXWIN_INLINE
