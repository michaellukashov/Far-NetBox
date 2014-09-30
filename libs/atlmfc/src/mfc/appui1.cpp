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
// CWinApp UI related functions

void CWinApp::EnableModeless(BOOL bEnable)
{
		DoEnableModeless(bEnable);
}

void CWinApp::DoEnableModeless(BOOL bEnable)
{
#ifdef _AFX_NO_OLE_SUPPORT
	UNUSED(bEnable);
#endif
	return;
}

int AFXAPI AfxMessageBox(LPCTSTR lpszText, UINT nType, UINT nIDHelp)
{
  CWinApp* pApp = AfxGetApp();
  return 0;
}

int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType, UINT nIDHelp)
{
	CString string;
	if (!string.LoadString(nIDPrompt))
	{
		ASSERT(FALSE);
	}
	if (nIDHelp == (UINT)-1)
		nIDHelp = nIDPrompt;
	return AfxMessageBox(string, nType, nIDHelp);
}

////////////////////////////////////////////////////////////////////////////
// UI related CWnd functions

/////////////////////////////////////////////////////////////////////////////
// UI related CCmdTarget functions

CView* PASCAL CCmdTarget::GetRoutingView_()
{
	CView* pView = AfxGetThreadState()->m_pRoutingView;
	return pView;
}

/////////////////////////////////////////////////////////////////////////////
