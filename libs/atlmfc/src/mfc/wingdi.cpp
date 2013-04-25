// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"



/////////////////////////////////////////////////////////////////////////////
// Diagnostic Output
/////////////////////////////////////////////////////////////////////////////
// CDC

CDC::CDC()
{
	m_hDC = NULL;
	m_hAttribDC = NULL;
	m_bPrinting = FALSE;
}

#ifdef _DEBUG
void CDC::AssertValid() const
{
	CObject::AssertValid();
}

CHandleMap* PASCAL afxMapHDC(BOOL bCreate)
{
	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
	if (pState->m_pmapHDC == NULL && bCreate)
	{
		BOOL bEnable = AfxEnableMemoryTracking(FALSE);
#ifndef _AFX_PORTABLE
		_PNH pnhOldHandler = AfxSetNewHandler(&AfxCriticalNewHandler);
#endif
		pState->m_pmapHDC = new CHandleMap(RUNTIME_CLASS(CDC),
			ConstructDestruct<CDC>::Construct, ConstructDestruct<CDC>::Destruct, 
			offsetof(CDC, m_hDC), 2);

#ifndef _AFX_PORTABLE
		AfxSetNewHandler(pnhOldHandler);
#endif
		AfxEnableMemoryTracking(bEnable);
	}
	return pState->m_pmapHDC;
}

CDC* PASCAL CDC::FromHandle(HDC hDC)
{
	CHandleMap* pMap = afxMapHDC(TRUE); //create map if not exist
	ASSERT(pMap != NULL);
	CDC* pDC = (CDC*)pMap->FromHandle(hDC);
	ASSERT(pDC == NULL || pDC->m_hDC == hDC);
	return pDC;
}

BOOL CDC::Attach(HDC hDC)
{
	ASSERT(m_hDC == NULL);      // only attach once, detach on destroy
	ASSERT(m_hAttribDC == NULL);    // only attach to an empty DC

	if (hDC == NULL)
	{
		return FALSE;
	}
	// remember early to avoid leak
	m_hDC = hDC;
	CHandleMap* pMap = afxMapHDC(TRUE); // create map if not exist
	ASSERT(pMap != NULL);
	pMap->SetPermanent(m_hDC, this);

	SetAttribDC(m_hDC);     // Default to same as output
	return TRUE;
}

HDC CDC::Detach()
{
	HDC hDC = m_hDC;
	if (hDC != NULL)
	{
		CHandleMap* pMap = afxMapHDC(); // don't create if not exist
		if (pMap != NULL)
			pMap->RemoveHandle(m_hDC);
	}

	ReleaseAttribDC();
	m_hDC = NULL;
	return hDC;
}

BOOL CDC::DeleteDC()
{
	if (m_hDC == NULL)
		return FALSE;

	return ::DeleteDC(Detach());
}

CDC::~CDC()
{
	if (m_hDC != NULL)
		::DeleteDC(Detach());
}


void CDC::SetAttribDC(HDC hDC)  // Set the Attribute DC
{
	m_hAttribDC = hDC;
}

void CDC::SetOutputDC(HDC hDC)  // Set the Output DC
{
#ifdef _DEBUG
	CHandleMap* pMap = afxMapHDC();
	if (pMap != NULL && pMap->LookupPermanent(m_hDC) == this)
	{
		ASSERT(FALSE);
	}
#endif
	m_hDC = hDC;
}

void CDC::ReleaseAttribDC()     // Release the Attribute DC
{
	m_hAttribDC = NULL;
}

void CDC::ReleaseOutputDC()     // Release the Output DC
{
#ifdef _DEBUG
	CHandleMap* pMap = afxMapHDC();
	if (pMap != NULL && pMap->LookupPermanent(m_hDC) == this)
	{
		ASSERT(FALSE);
	}
#endif
	m_hDC = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Out-of-line routines

int CDC::StartDoc(LPCTSTR lpszDocName)
{
	DOCINFO di;
	memset(&di, 0, sizeof(DOCINFO));
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = lpszDocName;
	return StartDoc(&di);
}

int CDC::SaveDC()
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;
	if (m_hAttribDC != NULL)
		nRetVal = ::SaveDC(m_hAttribDC);
	if (m_hDC != m_hAttribDC && ::SaveDC(m_hDC) != 0)
		nRetVal = -1;   // -1 is the only valid restore value for complex DCs
	return nRetVal;
}

BOOL CDC::RestoreDC(int nSavedDC)
{
	// if two distinct DCs, nSavedDC can only be -1
	ASSERT(m_hDC != NULL);
	ASSERT(m_hDC == m_hAttribDC || nSavedDC == -1);

	BOOL bRetVal = TRUE;
	if (m_hDC != m_hAttribDC)
		bRetVal = ::RestoreDC(m_hDC, nSavedDC);
	if (m_hAttribDC != NULL)
		bRetVal = (bRetVal && ::RestoreDC(m_hAttribDC, nSavedDC));
	return bRetVal;
}

COLORREF CDC::SetBkColor(COLORREF crColor)
{
	ASSERT(m_hDC != NULL);
	COLORREF crRetVal = CLR_INVALID;

	if (m_hDC != m_hAttribDC)
		crRetVal = ::SetBkColor(m_hDC, crColor);
	if (m_hAttribDC != NULL)
		crRetVal = ::SetBkColor(m_hAttribDC, crColor);
	return crRetVal;
}

int CDC::SetBkMode(int nBkMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetBkMode(m_hDC, nBkMode);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetBkMode(m_hAttribDC, nBkMode);
	return nRetVal;
}

int CDC::SetPolyFillMode(int nPolyFillMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetPolyFillMode(m_hDC, nPolyFillMode);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetPolyFillMode(m_hAttribDC, nPolyFillMode);
	return nRetVal;
}

int CDC::SetROP2(int nDrawMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetROP2(m_hDC, nDrawMode);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetROP2(m_hAttribDC, nDrawMode);
	return nRetVal;
}

int CDC::SetStretchBltMode(int nStretchMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetStretchBltMode(m_hDC, nStretchMode);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetStretchBltMode(m_hAttribDC, nStretchMode);
	return nRetVal;
}

COLORREF CDC::SetTextColor(COLORREF crColor)
{
	ASSERT(m_hDC != NULL);
	COLORREF crRetVal = CLR_INVALID;

	if (m_hDC != m_hAttribDC)
		crRetVal = ::SetTextColor(m_hDC, crColor);
	if (m_hAttribDC != NULL)
		crRetVal = ::SetTextColor(m_hAttribDC, crColor);
	return crRetVal;
}

int CDC::SetGraphicsMode(int iMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
	{
		nRetVal = ::SetGraphicsMode(m_hDC, iMode);
	}

	if (m_hAttribDC != NULL)
	{
		nRetVal = ::SetGraphicsMode(m_hAttribDC, iMode);
	}

	return nRetVal;
}

BOOL CDC::SetWorldTransform(const XFORM* pXform)
{
	ASSERT(m_hDC != NULL);
	BOOL nRetVal = 0;

	if (m_hDC != m_hAttribDC)
	{
		nRetVal = ::SetWorldTransform(m_hDC, pXform);
	}

	if (m_hAttribDC != NULL)
	{
		nRetVal = ::SetWorldTransform(m_hAttribDC, pXform);
	}

	return nRetVal;
}

BOOL CDC::ModifyWorldTransform(const XFORM* pXform,DWORD iMode)
{
	ASSERT(m_hDC != NULL);
	BOOL nRetVal = 0;

	if (m_hDC != m_hAttribDC)
	{
		nRetVal = ::ModifyWorldTransform(m_hDC, pXform, iMode);
	}

	if (m_hAttribDC != NULL)
	{
		nRetVal = ::ModifyWorldTransform(m_hAttribDC, pXform, iMode);
	}

	return nRetVal;
}

int CDC::SetMapMode(int nMapMode)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetMapMode(m_hDC, nMapMode);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetMapMode(m_hAttribDC, nMapMode);
	return nRetVal;
}

CSize CDC::SetViewportExt(int x, int y)
{
	ASSERT(m_hDC != NULL);
	CSize size;

	if (m_hDC != m_hAttribDC)
		VERIFY(::SetViewportExtEx(m_hDC, x, y, &size));
	if (m_hAttribDC != NULL)
		VERIFY(::SetViewportExtEx(m_hAttribDC, x, y, &size));
	return size;
}

CSize CDC::ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom)
{
	ASSERT(m_hDC != NULL);
	CSize size;

	if (m_hDC != m_hAttribDC)
		VERIFY(::ScaleViewportExtEx(m_hDC, xNum, xDenom, yNum, yDenom, &size));
	if (m_hAttribDC != NULL)
		VERIFY(::ScaleViewportExtEx(m_hAttribDC, xNum, xDenom, yNum, yDenom, &size));
	return size;
}

CSize CDC::SetWindowExt(int x, int y)
{
	ASSERT(m_hDC != NULL);
	CSize size;

	if (m_hDC != m_hAttribDC)
		VERIFY(::SetWindowExtEx(m_hDC, x, y, &size));
	if (m_hAttribDC != NULL)
		VERIFY(::SetWindowExtEx(m_hAttribDC, x, y, &size));
	return size;
}

CSize CDC::ScaleWindowExt(int xNum, int xDenom, int yNum, int yDenom)
{
	ASSERT(m_hDC != NULL);
	CSize size;

	if (m_hDC != m_hAttribDC)
		VERIFY(::ScaleWindowExtEx(m_hDC, xNum, xDenom, yNum, yDenom, &size));
	if (m_hAttribDC != NULL)
		VERIFY(::ScaleWindowExtEx(m_hAttribDC, xNum, xDenom, yNum, yDenom, &size));
	return size;
}

int CDC::GetClipBox(LPRECT lpRect) const
{
	ASSERT(m_hDC != NULL);
	return ::GetClipBox(m_hDC, lpRect);
}

int CDC::ExcludeClipRect(int x1, int y1, int x2, int y2)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::ExcludeClipRect(m_hDC, x1, y1, x2, y2);
	if (m_hAttribDC != NULL)
		nRetVal = ::ExcludeClipRect(m_hAttribDC, x1, y1, x2, y2);
	return nRetVal;
}

int CDC::ExcludeClipRect(LPCRECT lpRect)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::ExcludeClipRect(m_hDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom);
	if (m_hAttribDC != NULL)
		nRetVal = ::ExcludeClipRect(m_hAttribDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom);
	return nRetVal;
}

int CDC::IntersectClipRect(int x1, int y1, int x2, int y2)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::IntersectClipRect(m_hDC, x1, y1, x2, y2);
	if (m_hAttribDC != NULL)
		nRetVal = ::IntersectClipRect(m_hAttribDC, x1, y1, x2, y2);
	return nRetVal;
}

int CDC::IntersectClipRect(LPCRECT lpRect)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::IntersectClipRect(m_hDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom);
	if (m_hAttribDC != NULL)
		nRetVal = ::IntersectClipRect(m_hAttribDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom);
	return nRetVal;
}

int CDC::OffsetClipRgn(int x, int y)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::OffsetClipRgn(m_hDC, x, y);
	if (m_hAttribDC != NULL)
		nRetVal = ::OffsetClipRgn(m_hAttribDC, x, y);
	return nRetVal;
}

int CDC::OffsetClipRgn(SIZE size)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = ERROR;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::OffsetClipRgn(m_hDC, size.cx, size.cy);
	if (m_hAttribDC != NULL)
		nRetVal = ::OffsetClipRgn(m_hAttribDC, size.cx, size.cy);
	return nRetVal;
}

BOOL CDC::LineTo(int x, int y)
{
	ASSERT(m_hDC != NULL);
	if (m_hAttribDC != NULL && m_hDC != m_hAttribDC)
		::MoveToEx(m_hAttribDC, x, y, NULL);
	return ::LineTo(m_hDC, x, y);
}

UINT CDC::SetTextAlign(UINT nFlags)
{
	ASSERT(m_hDC != NULL);
	UINT nRetVal = GDI_ERROR;

	if (m_hDC != m_hAttribDC)
		::SetTextAlign(m_hDC, nFlags);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetTextAlign(m_hAttribDC, nFlags);
	return nRetVal;
}

int CDC::SetTextJustification(int nBreakExtra, int nBreakCount)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0;

	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetTextJustification(m_hDC, nBreakExtra, nBreakCount);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetTextJustification(m_hAttribDC, nBreakExtra, nBreakCount);
	return nRetVal;
}

int CDC::SetTextCharacterExtra(int nCharExtra)
{
	ASSERT(m_hDC != NULL);
	int nRetVal = 0x8000000;
	if (m_hDC != m_hAttribDC)
		nRetVal = ::SetTextCharacterExtra(m_hDC, nCharExtra);
	if (m_hAttribDC != NULL)
		nRetVal = ::SetTextCharacterExtra(m_hAttribDC, nCharExtra);
	return nRetVal;
}

DWORD CDC::SetMapperFlags(DWORD dwFlag)
{
	ASSERT(m_hDC != NULL);
	DWORD dwRetVal = GDI_ERROR;
	if (m_hDC != m_hAttribDC)
		dwRetVal = ::SetMapperFlags(m_hDC, dwFlag);
	if (m_hAttribDC != NULL)
		dwRetVal = ::SetMapperFlags(m_hAttribDC, dwFlag);
	return dwRetVal;
}

DWORD CDC::GetLayout() const
{
	ASSERT(m_hDC != NULL);

	DWORD dwGetLayout = ::GetLayout(m_hDC);

	return dwGetLayout;
}

DWORD CDC::SetLayout(DWORD dwSetLayout)
{
	ASSERT(m_hDC != NULL);

	DWORD dwGetLayout = ::SetLayout(m_hDC, dwSetLayout);

	return dwGetLayout;
}

void CWnd::ScreenToClient(LPRECT lpRect) const
{
	ASSERT(::IsWindow(m_hWnd));
	::ScreenToClient(m_hWnd, (LPPOINT)lpRect);
	::ScreenToClient(m_hWnd, ((LPPOINT)lpRect)+1);
	if (GetExStyle() & WS_EX_LAYOUTRTL)
		CRect::SwapLeftRight(lpRect);
}

void CWnd::ClientToScreen(LPRECT lpRect) const
{
	ASSERT(::IsWindow(m_hWnd));
	::ClientToScreen(m_hWnd, (LPPOINT)lpRect);
	::ClientToScreen(m_hWnd, ((LPPOINT)lpRect)+1);
	if (GetExStyle() & WS_EX_LAYOUTRTL)
		CRect::SwapLeftRight(lpRect);
}

/////////////////////////////////////////////////////////////////////////////
// Advanced Win32 GDI functions

BOOL CDC::ArcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
	ASSERT(m_hDC != NULL);
	BOOL bResult = ::ArcTo(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	if (m_hDC != m_hAttribDC)
	{
	}
	return bResult;
}

int CDC::SetArcDirection(int nArcDirection)
{
	ASSERT(m_hDC != NULL);
	int nResult = 0;
	if (m_hDC != m_hAttribDC)
		nResult = ::SetArcDirection(m_hDC, nArcDirection);
	if (m_hAttribDC != NULL)
		nResult = ::SetArcDirection(m_hAttribDC, nArcDirection);
	return nResult;
}

BOOL CDC::PolyDraw(const POINT* lpPoints, const BYTE* lpTypes, int nCount)
{
	ASSERT(m_hDC != NULL);
	BOOL bResult = ::PolyDraw(m_hDC, lpPoints, lpTypes, nCount);
	if (m_hDC != m_hAttribDC)
	{
	}
	return bResult;
}

BOOL CDC::SetColorAdjustment(const COLORADJUSTMENT* lpColorAdjust)
{
	ASSERT(m_hDC != NULL);
	BOOL bResult = FALSE;
	if (m_hDC != m_hAttribDC)
		bResult = ::SetColorAdjustment(m_hDC, lpColorAdjust);
	if (m_hAttribDC != NULL)
		bResult = ::SetColorAdjustment(m_hAttribDC, lpColorAdjust);
	return bResult;
}

BOOL CDC::PolyBezierTo(const POINT* lpPoints, int nCount)
{
	ASSERT(m_hDC != NULL);
	BOOL bResult = ::PolyBezierTo(m_hDC, lpPoints, nCount);
	if (m_hDC != m_hAttribDC)
	{
	}
	return bResult;
}

BOOL CDC::SelectClipPath(int nMode)
{
	ASSERT(m_hDC != NULL);

	// output DC always holds the current path
	if (!::SelectClipPath(m_hDC, nMode))
		return FALSE;

	// transfer clipping region into the attribute DC
	BOOL bResult = TRUE;
	if (m_hDC != m_hAttribDC)
	{
		HRGN hRgn = ::CreateRectRgn(0, 0, 0, 0);
		if (::GetClipRgn(m_hDC, hRgn) < 0 || !::SelectClipRgn(m_hAttribDC, hRgn))
		{
			bResult = FALSE;
		}
		DeleteObject(hRgn);
	}
	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// Special handling for metafile playback

int CALLBACK AfxEnumMetaFileProc(HDC hDC,
	HANDLETABLE* pHandleTable, METARECORD* pMetaRec, int nHandles, LPARAM lParam)
{
	CDC* pDC = (CDC*)lParam;
	ASSERT_VALID(pDC);

	switch (pMetaRec->rdFunction)
	{
	// these records have effects different for each CDC derived class
	case META_SETMAPMODE:
		pDC->SetMapMode((int)(short)pMetaRec->rdParm[0]);
		break;
	case META_SETWINDOWEXT:
		pDC->SetWindowExt(
			(int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
		break;
	case META_SAVEDC:
		pDC->SaveDC();
		break;
	case META_RESTOREDC:
		pDC->RestoreDC((int)(short)pMetaRec->rdParm[0]);
		break;
	case META_SETBKCOLOR:
		pDC->SetBkColor(*(UNALIGNED COLORREF*)&pMetaRec->rdParm[0]);
		break;
	case META_SETTEXTCOLOR:
		pDC->SetTextColor(*(UNALIGNED COLORREF*)&pMetaRec->rdParm[0]);
		break;

	// need to watch out for SelectObject(HFONT), for custom font mapping
	case META_SELECTOBJECT:
		{
			HGDIOBJ hObject = pHandleTable->objectHandle[pMetaRec->rdParm[0]];
			UINT nObjType = GetObjectType(hObject);
			if (nObjType == 0)
			{
				// object type is unknown, determine if it is a font
				HFONT hStockFont = (HFONT)::GetStockObject(SYSTEM_FONT);
				HFONT hFontOld = (HFONT)::SelectObject(pDC->m_hDC, hStockFont);
				HGDIOBJ hObjOld = ::SelectObject(pDC->m_hDC, hObject);
				if (hObjOld == hStockFont)
				{
					// got the stock object back, so must be selecting a font
					break;  // don't play the default record
				}
				else
				{
					// didn't get the stock object back, so restore everything
					::SelectObject(pDC->m_hDC, hFontOld);
					::SelectObject(pDC->m_hDC, hObjOld);
				}
				// and fall through to PlayMetaFileRecord...
			}
			else if (nObjType == OBJ_FONT)
			{
				break;  // don't play the default record
			}
		}
		// fall through...

	default:
		::PlayMetaFileRecord(hDC, pHandleTable, pMetaRec, nHandles);
		break;
	}

	return 1;
}

BOOL CDC::PlayMetaFile(HMETAFILE hMF)
{
	if (::GetDeviceCaps(m_hDC, TECHNOLOGY) == DT_METAFILE)
	{
		// playing metafile in metafile, just use core windows API
		return ::PlayMetaFile(m_hDC, hMF);
	}

	// for special playback, lParam == pDC
	return ::EnumMetaFile(m_hDC, hMF, AfxEnumMetaFileProc, (LPARAM)this);
}

/////////////////////////////////////////////////////////////////////////////
// Coordinate transforms

void CDC::LPtoDP(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	CSize sizeWinExt = GetWindowExt();
	CSize sizeVpExt = GetViewportExt();
	lpSize->cx = MulDiv(lpSize->cx, abs(sizeVpExt.cx), abs(sizeWinExt.cx));
	lpSize->cy = MulDiv(lpSize->cy, abs(sizeVpExt.cy), abs(sizeWinExt.cy));
}

void CDC::DPtoLP(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	CSize sizeWinExt = GetWindowExt();
	CSize sizeVpExt = GetViewportExt();
	lpSize->cx = MulDiv(lpSize->cx, abs(sizeWinExt.cx), abs(sizeVpExt.cx));
	lpSize->cy = MulDiv(lpSize->cy, abs(sizeWinExt.cy), abs(sizeVpExt.cy));
}

/////////////////////////////////////////////////////////////////////////////
// Helper DCs

CClientDC::CClientDC(CWnd* pWnd)
{
	ASSERT(pWnd == NULL || ::IsWindow(pWnd->m_hWnd));

	if (!Attach(::GetDC(m_hWnd = pWnd->GetSafeHwnd())))
		AfxThrowResourceException();
}

CClientDC::~CClientDC()
{
	ASSERT(m_hDC != NULL);
	::ReleaseDC(m_hWnd, Detach());
}

CWindowDC::CWindowDC(CWnd* pWnd)
{
	ASSERT(pWnd == NULL || ::IsWindow(pWnd->m_hWnd));

	if (!Attach(::GetWindowDC(m_hWnd = pWnd->GetSafeHwnd())))
		AfxThrowResourceException();
}

CWindowDC::~CWindowDC()
{
	ASSERT(m_hDC != NULL);
	::ReleaseDC(m_hWnd, Detach());
}

IMPLEMENT_DYNAMIC(CResourceException, CSimpleException)
CResourceException _simpleResourceException(FALSE, AFX_IDS_RESOURCE_EXCEPTION);

IMPLEMENT_DYNAMIC(CUserException, CSimpleException)
CUserException _simpleUserException(FALSE, AFX_IDS_USER_EXCEPTION);

IMPLEMENT_DYNCREATE(CDC, CObject)
IMPLEMENT_DYNAMIC(CClientDC, CDC)
IMPLEMENT_DYNAMIC(CWindowDC, CDC)
IMPLEMENT_DYNAMIC(CPaintDC, CDC)

/////////////////////////////////////////////////////////////////////////////
// Standard exception processing


// resource failure
void AFXAPI AfxThrowResourceException()
{
	THROW((CResourceException*)&_simpleResourceException);
}

// user alert
void AFXAPI AfxThrowUserException()
{
	THROW((CUserException*)&_simpleUserException);
}

/////////////////////////////////////////////////////////////////////////////
