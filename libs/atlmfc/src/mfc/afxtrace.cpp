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

#ifdef _DEBUG       // entire file for debugging


#include "dde.h"


/////////////////////////////////////////////////////////////////////////////
// Build data tables by including data file three times

/////////////////////////////////////////////////////////////////////////////
// DDE special case

/////////////////////////////////////////////////////////////////////////////

void AFXAPI _AfxTraceMsg(LPCTSTR lpszPrefix, const MSG* pMsg)
{
	ENSURE_ARG(AfxIsValidString(lpszPrefix));
	ENSURE_ARG(pMsg != NULL);

	if (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_NCMOUSEMOVE ||
		pMsg->message == WM_NCHITTEST || pMsg->message == WM_SETCURSOR ||
		pMsg->message == WM_CTLCOLORBTN ||
		pMsg->message == WM_CTLCOLORDLG ||
		pMsg->message == WM_CTLCOLOREDIT ||
		pMsg->message == WM_CTLCOLORLISTBOX ||
		pMsg->message == WM_CTLCOLORMSGBOX ||
		pMsg->message == WM_CTLCOLORSCROLLBAR ||
		pMsg->message == WM_CTLCOLORSTATIC ||
		pMsg->message == WM_ENTERIDLE || pMsg->message == WM_CANCELMODE ||
		pMsg->message == 0x0118)    // WM_SYSTIMER (caret blink)
	{
		// don't report very frequently sent messages
		return;
	}

	LPCSTR lpszMsgName = NULL;
	char szBuf[80];

	// find message name
	if (pMsg->message >= 0xC000)
	{
		// Window message registered with 'RegisterWindowMessage'
		//  (actually a USER atom)
		if (::GetClipboardFormatNameA(pMsg->message, szBuf, _countof(szBuf)))
			lpszMsgName = szBuf;
	}
	else if (pMsg->message >= WM_USER)
	{
		// User message
		sprintf_s(szBuf, _countof(szBuf), "WM_USER+0x%04X", pMsg->message - WM_USER);
		lpszMsgName = szBuf;
	}
	else
	{
		// a system windows message
		const AFX_MAP_MESSAGE* pMapMsg = allMessages;
		for (/*null*/; pMapMsg->lpszMsg != NULL; pMapMsg++)
		{
			if (pMapMsg->nMsg == pMsg->message)
			{
				lpszMsgName = pMapMsg->lpszMsg;
				break;
			}
		}
	}

	if (lpszMsgName != NULL)
	{
	}
	else
	{
	}

	if (pMsg->message >= WM_DDE_FIRST && pMsg->message <= WM_DDE_LAST)
		TraceDDE(lpszPrefix, pMsg);
}

/////////////////////////////////////////////////////////////////////////////

#endif // _DEBUG (entire file)
