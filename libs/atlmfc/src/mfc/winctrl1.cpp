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
#ifndef _AFX_NO_OCC_SUPPORT
#include "occimpl.h"
#endif



#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CStatic

BOOL CStatic::Create(LPCTSTR lpszText, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, UINT nID)
{
	CWnd* pWnd = this;
	return pWnd->Create(_T("STATIC"), lpszText, dwStyle, rect, pParentWnd, nID);
}

CStatic::~CStatic()
{
	DestroyWindow();
}

// Derived class is responsible for implementing all of these handlers
//   for owner/self draw controls
void CStatic::DrawItem(LPDRAWITEMSTRUCT)
{
	ASSERT(FALSE);
}

BOOL CStatic::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam,
	LRESULT* pResult)
{
	if (message != WM_DRAWITEM)
		return CWnd::OnChildNotify(message, wParam, lParam, pResult);

	ASSERT(pResult == NULL);       // no return value expected
	UNUSED(pResult); // unused in release builds
	DrawItem((LPDRAWITEMSTRUCT)lParam);
	return TRUE;
}

// Helper for radio buttons
int CWnd::GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton) const
{
	for (int nID = nIDFirstButton; nID <= nIDLastButton; nID++)
	{
		if (IsDlgButtonChecked(nID))
			return nID; // id that matched
	}
	return 0; // invalid ID
}

IMPLEMENT_DYNAMIC(CStatic, CWnd)

/////////////////////////////////////////////////////////////////////////////
