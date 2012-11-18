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
#include "sal.h"



/////////////////////////////////////////////////////////////////////////////
// CWinApp User Interface Extensions

void CWinApp::OnAppExit()
{
	// same as double-clicking on main window close box
	ASSERT(m_pMainWnd != NULL);
	m_pMainWnd->SendMessage(WM_CLOSE);
}


void CWinApp::HideApplication()
{
	ASSERT_VALID(m_pMainWnd);

	// hide the application's windows before closing all the documents
	m_pMainWnd->ShowWindow(SW_HIDE);
	m_pMainWnd->ShowOwnedPopups(FALSE);

	// put the window at the bottom of zorder, so it isn't activated
	m_pMainWnd->SetWindowPos(&CWnd::wndBottom, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}

void CWinApp::DoWaitCursor(int nCode)
{
	// 0 => restore, 1=> begin, -1=> end
	ENSURE_ARG(nCode == 0 || nCode == 1 || nCode == -1);
	ENSURE(afxData.hcurWait != NULL);
	AfxLockGlobals(CRIT_WAITCURSOR);
	m_nWaitCursorCount += nCode;
	if (m_nWaitCursorCount > 0)
	{
		HCURSOR hcurPrev = ::SetCursor(afxData.hcurWait);
		if (nCode > 0 && m_nWaitCursorCount == 1)
			m_hcurWaitCursorRestore = hcurPrev;
	}
	else
	{
		// turn everything off
		m_nWaitCursorCount = 0;     // prevent underflow
		::SetCursor(m_hcurWaitCursorRestore);
	}
	AfxUnlockGlobals(CRIT_WAITCURSOR);
}


BOOL CWinApp::SaveAllModified()
{
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// MRU file list default implementation

void CWinApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	ENSURE_ARG(pCmdUI != NULL);
	if (m_pRecentFileList == NULL) // no MRU files
		pCmdUI->Enable(FALSE);
	else
		m_pRecentFileList->UpdateMenu(pCmdUI);
}

/////////////////////////////////////////////////////////////////////////////
// DDE and ShellExecute support

BOOL CWinApp::OnDDECommand(_In_z_ LPTSTR lpszCommand)
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// MRU file list default implementation

/////////////////////////////////////////////////////////////////////////////
