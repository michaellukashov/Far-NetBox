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



AFX_STATIC_DATA HBRUSH _afxHalftoneBrush = 0;

void AFX_CDECL AfxWingdixTerm()
{
	AfxDeleteObject((HGDIOBJ*)&_afxHalftoneBrush);
}
char _afxWingdixTerm = 0;

/////////////////////////////////////////////////////////////////////////////
// More coordinate transforms (in separate file to avoid transitive refs)

#define HIMETRIC_INCH   2540    // HIMETRIC units per inch

void CDC::DPtoHIMETRIC(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	int nMapMode;
	if (this != NULL && (nMapMode = GetMapMode()) < MM_ISOTROPIC &&
		nMapMode != MM_TEXT)
	{
		// when using a constrained map mode, map against physical inch
		((CDC*)this)->SetMapMode(MM_HIMETRIC);
		DPtoLP(lpSize);
		((CDC*)this)->SetMapMode(nMapMode);
	}
	else
	{
		// map against logical inch for non-constrained mapping modes
		int cxPerInch, cyPerInch;
		if (this != NULL)
		{
			ASSERT_VALID(this);
			ASSERT(m_hDC != NULL);  // no HDC attached or created?
			cxPerInch = GetDeviceCaps(LOGPIXELSX);
			cyPerInch = GetDeviceCaps(LOGPIXELSY);
		}
		else
		{
			cxPerInch = afxData.cxPixelsPerInch;
			cyPerInch = afxData.cyPixelsPerInch;
		}
		ASSERT(cxPerInch != 0 && cyPerInch != 0);
		lpSize->cx = MulDiv(lpSize->cx, HIMETRIC_INCH, cxPerInch);
		lpSize->cy = MulDiv(lpSize->cy, HIMETRIC_INCH, cyPerInch);
	}
}

void CDC::HIMETRICtoDP(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	int nMapMode;
	if (this != NULL && (nMapMode = GetMapMode()) < MM_ISOTROPIC &&
		nMapMode != MM_TEXT)
	{
		// when using a constrained map mode, map against physical inch
		((CDC*)this)->SetMapMode(MM_HIMETRIC);
		LPtoDP(lpSize);
		((CDC*)this)->SetMapMode(nMapMode);
	}
	else
	{
		// map against logical inch for non-constrained mapping modes
		int cxPerInch, cyPerInch;
		if (this != NULL)
		{
			ASSERT_VALID(this);
			ASSERT(m_hDC != NULL);  // no HDC attached or created?
			cxPerInch = GetDeviceCaps(LOGPIXELSX);
			cyPerInch = GetDeviceCaps(LOGPIXELSY);
		}
		else
		{
			cxPerInch = afxData.cxPixelsPerInch;
			cyPerInch = afxData.cyPixelsPerInch;
		}
		ASSERT(cxPerInch != 0 && cyPerInch != 0);
		lpSize->cx = MulDiv(lpSize->cx, cxPerInch, HIMETRIC_INCH);
		lpSize->cy = MulDiv(lpSize->cy, cyPerInch, HIMETRIC_INCH);
	}
}

void CDC::LPtoHIMETRIC(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	LPtoDP(lpSize);
	DPtoHIMETRIC(lpSize);
}

void CDC::HIMETRICtoLP(LPSIZE lpSize) const
{
	ASSERT(AfxIsValidAddress(lpSize, sizeof(SIZE)));

	HIMETRICtoDP(lpSize);
	DPtoLP(lpSize);
}

/////////////////////////////////////////////////////////////////////////////
// special CDC drawing primitives/helpers

CBrush* PASCAL CDC::GetHalftoneBrush()
{
	AfxLockGlobals(CRIT_HALFTONEBRUSH);
	if (_afxHalftoneBrush == NULL)
	{
		WORD grayPattern[8];
		for (int i = 0; i < 8; i++)
			grayPattern[i] = (WORD)(0x5555 << (i & 1));
		HBITMAP grayBitmap = CreateBitmap(8, 8, 1, 1, grayPattern);
		if (grayBitmap != NULL)
		{
			_afxHalftoneBrush = ::CreatePatternBrush(grayBitmap);
			DeleteObject(grayBitmap);
		}
	}
	if (!_afxWingdixTerm)
		_afxWingdixTerm = (char)!atexit(&AfxWingdixTerm);
	AfxUnlockGlobals(CRIT_HALFTONEBRUSH);

	return CBrush::FromHandle(_afxHalftoneBrush);
}

void CDC::FillSolidRect(int x, int y, int cx, int cy, COLORREF clr)
{
	ASSERT_VALID(this);
	ASSERT(m_hDC != NULL);

	::SetBkColor(m_hDC, clr);
	CRect rect(x, y, x + cx, y + cy);
	::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

void CDC::Draw3dRect(LPCRECT lpRect,
	COLORREF clrTopLeft, COLORREF clrBottomRight)
{
	Draw3dRect(lpRect->left, lpRect->top, lpRect->right - lpRect->left,
		lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
}

void CDC::Draw3dRect(int x, int y, int cx, int cy,
	COLORREF clrTopLeft, COLORREF clrBottomRight)
{
	FillSolidRect(x, y, cx - 1, 1, clrTopLeft);
	FillSolidRect(x, y, 1, cy - 1, clrTopLeft);
	FillSolidRect(x + cx, y, -1, cy, clrBottomRight);
	FillSolidRect(x, y + cy, cx, -1, clrBottomRight);
}

/////////////////////////////////////////////////////////////////////////////
