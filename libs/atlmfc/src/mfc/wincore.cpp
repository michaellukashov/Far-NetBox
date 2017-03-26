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

// #include <atlacc.h>	// Accessible Proxy from ATL
#include "sal.h"

// #include "afxctrlcontainer.h"

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// Globals

const UINT CWnd::m_nMsgDragList = ::RegisterWindowMessage(DRAGLISTMSGSTRING);

// CWnds for setting z-order with SetWindowPos's pWndInsertAfter parameter

const TCHAR _afxWnd[] = AFX_WND;
const TCHAR _afxWndControlBar[] = AFX_WNDCONTROLBAR;
const TCHAR _afxWndMDIFrame[] = AFX_WNDMDIFRAME;
const TCHAR _afxWndFrameOrView[] = AFX_WNDFRAMEORVIEW;
const TCHAR _afxWndOleControl[] = AFX_WNDOLECONTROL;

/////////////////////////////////////////////////////////////////////////////
// CWnd construction

CWnd::CWnd()
{
	// m_hWnd = NULL;
	m_bEnableActiveAccessibility = false;
	m_bIsTouchWindowRegistered = FALSE;
	m_hWndOwner = NULL;
	m_nFlags = 0;
	m_pfnSuper = NULL;
	m_nModalResult = 0;
	// m_pDropTarget = NULL;
#ifndef _AFX_NO_OCC_SUPPORT
	m_pCtrlCont = NULL;
	m_pCtrlSite = NULL;
#endif
}

CWnd::CWnd(HWND hWnd)
{
	// m_hWnd = hWnd;
	m_bEnableActiveAccessibility = false;
	m_bIsTouchWindowRegistered = FALSE;
	m_hWndOwner = NULL;
	m_nFlags = 0;
	m_pfnSuper = NULL;
	m_nModalResult = 0;
	// m_pDropTarget = NULL;
#ifndef _AFX_NO_OCC_SUPPORT
	m_pCtrlCont = NULL;
	m_pCtrlSite = NULL;
#endif

}

// Change a window's style

AFX_STATIC BOOL AFXAPI _AfxModifyStyle(HWND hWnd, int nStyleOffset,
	DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	ASSERT(hWnd != NULL);
	DWORD dwStyle = ::GetWindowLong(hWnd, nStyleOffset);
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
	if (dwStyle == dwNewStyle)
		return FALSE;

	::SetWindowLong(hWnd, nStyleOffset, dwNewStyle);
	if (nFlags != 0)
	{
		::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
	}
	return TRUE;
}

BOOL PASCAL
CWnd::ModifyStyle(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	return _AfxModifyStyle(hWnd, GWL_STYLE, dwRemove, dwAdd, nFlags);
}

BOOL PASCAL
CWnd::ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
	return _AfxModifyStyle(hWnd, GWL_EXSTYLE, dwRemove, dwAdd, nFlags);
}

/////////////////////////////////////////////////////////////////////////////
// Special helpers for certain windows messages

AFX_STATIC void AFXAPI
_AfxHandleActivate(CWnd* pWnd, WPARAM nState, CWnd* pWndOther)
{
	ASSERT(pWnd != NULL);

}

AFX_STATIC BOOL AFXAPI
_AfxHandleSetCursor(CWnd* pWnd, UINT nHitTest, UINT nMsg)
{
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Official way to send message to a CWnd

LRESULT AFXAPI AfxCallWndProc(CWnd* pWnd, HWND hWnd, UINT nMsg,
	WPARAM wParam = 0, LPARAM lParam = 0)
{
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	MSG oldState = pThreadState->m_lastSentMsg;   // save for nesting
	pThreadState->m_lastSentMsg.hwnd = hWnd;
	pThreadState->m_lastSentMsg.message = nMsg;
	pThreadState->m_lastSentMsg.wParam = wParam;
	pThreadState->m_lastSentMsg.lParam = lParam;

	// Catch exceptions thrown outside the scope of a callback
	// in debug builds and warn the user.
	LRESULT lResult;
	TRY
	{
#ifndef _AFX_NO_OCC_SUPPORT
		// special case for WM_DESTROY
		if ((nMsg == WM_DESTROY) && (pWnd->m_pCtrlCont != NULL))
			pWnd->m_pCtrlCont->OnUIActivate(NULL);
#endif

		// delegate to object's WindowProc
		lResult = pWnd->WindowProc(nMsg, wParam, lParam);

		// more special case for WM_INITDIALOG
	}
	CATCH_ALL(e)
	{
		lResult = AfxProcessWndProcException(e, &pThreadState->m_lastSentMsg);
		DELETE_EXCEPTION(e);
	}
	END_CATCH_ALL

	pThreadState->m_lastSentMsg = oldState;
	return lResult;
}

const MSG* PASCAL CWnd::GetCurrentMessage()
{
	// fill in time and position when asked for
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	pThreadState->m_lastSentMsg.time = ::GetMessageTime();
	return &pThreadState->m_lastSentMsg;
}

LRESULT CWnd::Default()
{
	// call DefWindowProc with the last message
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	return DefWindowProc(pThreadState->m_lastSentMsg.message,
		pThreadState->m_lastSentMsg.wParam, pThreadState->m_lastSentMsg.lParam);
}

/////////////////////////////////////////////////////////////////////////////
// Map from HWND to CWnd*

//CHandleMap* PASCAL afxMapHWND(BOOL bCreate)
//{
//	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
//	// if (pState->m_pmapHWND == NULL && bCreate)
//	{
//		BOOL bEnable = AfxEnableMemoryTracking(FALSE);
//#ifndef _AFX_PORTABLE
//		_PNH pnhOldHandler = AfxSetNewHandler(&AfxCriticalNewHandler);
//#endif
//		// pState->m_pmapHWND = new CHandleMap(RUNTIME_CLASS(CWnd),
//			// ConstructDestruct<CWnd>::Construct, ConstructDestruct<CWnd>::Destruct,
//			// offsetof(CWnd, m_hWnd));

//#ifndef _AFX_PORTABLE
//		AfxSetNewHandler(pnhOldHandler);
//#endif
//		AfxEnableMemoryTracking(bEnable);
//	}
//	// return pState->m_pmapHWND;
//  return 0;
//}

CWnd* PASCAL CWnd::FromHandle(HWND hWnd)
{
//	CHandleMap* pMap = afxMapHWND(TRUE); //create map if not exist
  // ASSERT(pMap != NULL);
    CWnd* pWnd = NULL; // (CWnd*)pMap->FromHandle(hWnd);

#ifndef _AFX_NO_OCC_SUPPORT
  pWnd->AttachControlSite(pMap);
#endif

	// ASSERT(pWnd == NULL || pWnd->m_hWnd == hWnd);
	return pWnd;
}

CWnd* PASCAL CWnd::FromHandlePermanent(HWND hWnd)
{
//	CHandleMap* pMap = afxMapHWND();
  CWnd* pWnd = NULL;
    /*if (pMap != NULL)
  {
    // only look in the permanent map - does no allocations
    pWnd = (CWnd*)pMap->LookupPermanent(hWnd);
    // ASSERT(pWnd == NULL || pWnd->m_hWnd == hWnd);
    }*/
  return pWnd;
}

BOOL CWnd::Attach(HWND hWndNew)
{
	// ASSERT(m_hWnd == NULL);     // only attach once, detach on destroy
	ASSERT(FromHandlePermanent(hWndNew) == NULL);
		// must not already be in permanent map

	if (hWndNew == NULL)
		return FALSE;

//	CHandleMap* pMap = afxMapHWND(TRUE); // create map if not exist
//	ASSERT(pMap != NULL);

	// pMap->SetPermanent(m_hWnd = hWndNew, this);


#ifndef _AFX_NO_OCC_SUPPORT
	AttachControlSite(pMap);
#endif

	return TRUE;
}

HWND CWnd::Detach()
{
	// HWND hWnd = m_hWnd;
	// if (hWnd != NULL)
	// {
		// CHandleMap* pMap = afxMapHWND(); // don't create if not exist
		// if (pMap != NULL)
			// pMap->RemoveHandle(m_hWnd);
	// m_hWnd = NULL;
	// }

#ifndef _AFX_NO_OCC_SUPPORT
	m_pCtrlSite = NULL;
#endif

	return 0; // hWnd;
}

void CWnd::PreSubclassWindow()
{
	// no default processing
}

////////////////////////////////////////////////////////////////////////////
// The WndProc for all CWnd's and derived classes

LRESULT CALLBACK
AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// special message which identifies the window as using AfxWndProc
	// if (nMsg == WM_QUERYAFXWNDPROC)
		// return 1;

	// all other messages route through message map
	// CWnd* pWnd = CWnd::FromHandlePermanent(hWnd);
	// ASSERT(pWnd != NULL);
	// ASSERT(pWnd==NULL || pWnd->m_hWnd == hWnd);
	// if (pWnd == NULL || pWnd->m_hWnd != hWnd)
		// return ::DefWindowProc(hWnd, nMsg, wParam, lParam);
	// return AfxCallWndProc(pWnd, hWnd, nMsg, wParam, lParam);
	return 0;
}

// always indirectly accessed via AfxGetAfxWndProc
WNDPROC AFXAPI AfxGetAfxWndProc()
{
#ifdef _AFXDLL
	return AfxGetModuleState()->m_pfnAfxWndProc;
#else
	return &AfxWndProc;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Special WndProcs (activation handling & gray dialogs)

AFX_STATIC_DATA const TCHAR _afxOldWndProc[] = _T("AfxOldWndProc423");

LRESULT CALLBACK
_AfxActivationWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, _afxOldWndProc);
	ASSERT(oldWndProc != NULL);

	LRESULT lResult = 0;
	TRY
	{
		BOOL bCallDefault = TRUE;
		switch (nMsg)
		{
		case WM_ACTIVATE:
			_AfxHandleActivate(CWnd::FromHandle(hWnd), wParam,
				CWnd::FromHandle((HWND)lParam));
			break;
		}

		// call original wndproc for default handling
		if (bCallDefault)
			lResult = CallWindowProc(oldWndProc, hWnd, nMsg, wParam, lParam);
	}
	CATCH_ALL(e)
	{
		// handle exception
		MSG msg;
		msg.hwnd = hWnd;
		msg.message = nMsg;
		msg.wParam = wParam;
		msg.lParam = lParam;
		lResult = AfxProcessWndProcException(e, &msg);
		DELETE_EXCEPTION(e);
	}
	END_CATCH_ALL

	return lResult;
}

/////////////////////////////////////////////////////////////////////////////
// Window creation hooks

LRESULT CALLBACK
_AfxCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
{
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	if (code != HCBT_CREATEWND)
	{
		// wait for HCBT_CREATEWND just pass others on...
		return CallNextHookEx(pThreadState->m_hHookOldCbtFilter, code,
			wParam, lParam);
	}

	ASSERT(lParam != NULL);
	LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;
	ASSERT(lpcs != NULL);

	CWnd* pWndInit = pThreadState->m_pWndInit;
	BOOL bContextIsDLL = afxContextIsDLL;
	if (pWndInit != NULL || (!(lpcs->style & WS_CHILD) && !bContextIsDLL))
	{
		ASSERT(wParam != NULL); // should be non-NULL HWND
		HWND hWnd = (HWND)wParam;
		WNDPROC oldWndProc;
		if (pWndInit != NULL)
		{
//			AFX_MANAGE_STATE(pWndInit->m_pModuleState);

			// the window should not be in the permanent map at this time
			ASSERT(CWnd::FromHandlePermanent(hWnd) == NULL);

			// connect the HWND to pWndInit...
			pWndInit->Attach(hWnd);
			// allow other subclassing to occur first
			pWndInit->PreSubclassWindow();

			WNDPROC *pOldWndProc = pWndInit->GetSuperWndProcAddr();
			ASSERT(pOldWndProc != NULL);

			// subclass the window with standard AfxWndProc
			WNDPROC afxWndProc = AfxGetAfxWndProc();
			oldWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC,
				(DWORD_PTR)afxWndProc);
			ASSERT(oldWndProc != NULL);
			if (oldWndProc != afxWndProc)
				*pOldWndProc = oldWndProc;

			pThreadState->m_pWndInit = NULL;
		}
		else
		{
			ASSERT(!bContextIsDLL);   // should never get here

			static ATOM s_atomMenu = 0;
			bool bSubclass = true;

			if (s_atomMenu == 0)
			{
				WNDCLASSEX wc;
				memset(&wc, 0, sizeof(WNDCLASSEX));
				wc.cbSize = sizeof(WNDCLASSEX);
				// s_atomMenu = (ATOM)::AfxCtxGetClassInfoEx(NULL, _T("#32768"), &wc);
			}

			// Do not subclass menus.
			if (s_atomMenu != 0)
			{
				ATOM atomWnd = (ATOM)::GetClassLongPtr(hWnd, GCW_ATOM);
				if (atomWnd == s_atomMenu)
						bSubclass = false;
			}
			else
			{
				TCHAR szClassName[256];
				if (::GetClassName(hWnd, szClassName, 256))
				{
					szClassName[255] = NULL;
					if (_tcscmp(szClassName, _T("#32768")) == 0)
						bSubclass = false;
				}
			}
			if (bSubclass)
			{
				// subclass the window with the proc which does gray backgrounds
				oldWndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
				if (oldWndProc != NULL && GetProp(hWnd, _afxOldWndProc) == NULL)
				{
					SetProp(hWnd, _afxOldWndProc, oldWndProc);
					if ((WNDPROC)GetProp(hWnd, _afxOldWndProc) == oldWndProc)
					{
						GlobalAddAtom(_afxOldWndProc);
						SetWindowLongPtr(hWnd, GWLP_WNDPROC, (DWORD_PTR)_AfxActivationWndProc);
						ASSERT(oldWndProc != NULL);
					}
				}
			}
		}
	}

	LRESULT lResult = CallNextHookEx(pThreadState->m_hHookOldCbtFilter, code,
		wParam, lParam);

#ifndef _AFXDLL
	if (bContextIsDLL)
	{
		::UnhookWindowsHookEx(pThreadState->m_hHookOldCbtFilter);
		pThreadState->m_hHookOldCbtFilter = NULL;
	}
#endif
	return lResult;
}

void AFXAPI AfxHookWindowCreate(CWnd* pWnd)
{
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	if (pThreadState->m_pWndInit == pWnd)
		return;

	if (pThreadState->m_hHookOldCbtFilter == NULL)
	{
		pThreadState->m_hHookOldCbtFilter = ::SetWindowsHookEx(WH_CBT,
			_AfxCbtFilterHook, NULL, ::GetCurrentThreadId());
		if (pThreadState->m_hHookOldCbtFilter == NULL)
			AfxThrowMemoryException();
	}
	ASSERT(pThreadState->m_hHookOldCbtFilter != NULL);
	ASSERT(pWnd != NULL);
	// ASSERT(pWnd->m_hWnd == NULL);   // only do once

	ASSERT(pThreadState->m_pWndInit == NULL);   // hook not already in progress
	pThreadState->m_pWndInit = pWnd;
}

BOOL AFXAPI AfxUnhookWindowCreate()
{
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
#ifndef _AFXDLL
	if (afxContextIsDLL && pThreadState->m_hHookOldCbtFilter != NULL)
	{
		::UnhookWindowsHookEx(pThreadState->m_hHookOldCbtFilter);
		pThreadState->m_hHookOldCbtFilter = NULL;
	}
#endif
	if (pThreadState->m_pWndInit != NULL)
	{
		pThreadState->m_pWndInit = NULL;
		return FALSE;   // was not successfully hooked
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWnd creation

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, UINT nID,
		LPVOID lpParam /* = NULL */)
{
	return CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, lpParam);
}

BOOL CWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
	LPCTSTR lpszWindowName, DWORD dwStyle,
	int x, int y, int nWidth, int nHeight,
	HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	ASSERT(lpszClassName == NULL || AfxIsValidString(lpszClassName) ||
		AfxIsValidAtom(lpszClassName));
	ENSURE_ARG(lpszWindowName == NULL || AfxIsValidString(lpszWindowName));

	// allow modification of several common create parameters
	CREATESTRUCT cs;
	cs.dwExStyle = dwExStyle;
	cs.lpszClass = lpszClassName;
	cs.lpszName = lpszWindowName;
	cs.style = dwStyle;
	cs.x = x;
	cs.y = y;
	cs.cx = nWidth;
	cs.cy = nHeight;
	cs.hwndParent = hWndParent;
	cs.hMenu = nIDorHMenu;
	cs.hInstance = AfxGetInstanceHandle();
	cs.lpCreateParams = lpParam;

	if (!PreCreateWindow(cs))
	{
		PostNcDestroy();
		return FALSE;
	}

	AfxHookWindowCreate(this);
	HWND hWnd = 0; // ::AfxCtxCreateWindowEx(cs.dwExStyle, cs.lpszClass,
			// cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
			// cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);

	if (!AfxUnhookWindowCreate())
		PostNcDestroy();        // cleanup if CreateWindowEx fails too soon

	if (hWnd == NULL)
		return FALSE;
	// ASSERT(hWnd == m_hWnd); // should have been set in send msg hook
	return TRUE;
}

// for child windows
BOOL CWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if (cs.lpszClass == NULL)
	{
		// make sure the default window class is registered
		VERIFY(AfxDeferRegisterClass(AFX_WND_REG));

		// no WNDCLASS provided - use child window default
		ASSERT(cs.style & WS_CHILD);
		cs.lpszClass = _afxWnd;
	}

	if ((cs.hMenu == NULL) && (cs.style & WS_CHILD))
	{
		cs.hMenu = (HMENU)(UINT_PTR)this;
	}

	return TRUE;
}

BOOL CWnd::Create(LPCTSTR lpszClassName,
	LPCTSTR lpszWindowName, DWORD dwStyle,
	const RECT& rect,
	CWnd* pParentWnd, UINT nID,
	CCreateContext* pContext)
{
	// can't use for desktop or pop-up windows (use CreateEx instead)
	ASSERT(pParentWnd != NULL);
	ASSERT((dwStyle & WS_POPUP) == 0);

	return CreateEx(0, lpszClassName, lpszWindowName,
		dwStyle | WS_CHILD,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), (HMENU)(UINT_PTR)nID, (LPVOID)pContext);
}

CWnd::~CWnd()
{
	// if (m_hWnd != NULL &&
		// this != (CWnd*)&wndTop && this != (CWnd*)&wndBottom &&
		// this != (CWnd*)&wndTopMost && this != (CWnd*)&wndNoTopMost)
	{
		// DestroyWindow();
	}

#ifndef _AFX_NO_OCC_SUPPORT
	// cleanup control container,
	// including destroying controls

	delete m_pCtrlCont;

	// cleanup control site
	if (m_pCtrlSite != NULL && m_pCtrlSite->m_pWndCtrl == this)
		m_pCtrlSite->m_pWndCtrl = NULL;
#endif

}

void CWnd::OnDestroy()
{
#ifndef _AFX_NO_OCC_SUPPORT
	// cleanup control container
	delete m_pCtrlCont;
	m_pCtrlCont = NULL;
#endif

	if (m_bIsTouchWindowRegistered)
	{
		RegisterTouchWindow(FALSE);
	}

	Default();
}

void CWnd::PostNcDestroy()
{
	// default to nothing
}

void CWnd::OnFinalRelease()
{
	// if (m_hWnd != NULL)
		// DestroyWindow();    // will call PostNcDestroy
	// else
		// PostNcDestroy();
}

BOOL CWnd::DestroyWindow()
{
	CWnd* pWnd;
	CHandleMap* pMap;
	HWND hWndOrig;
	BOOL bResult;

	bResult = FALSE;
	pMap = NULL;
	pWnd = NULL;
	hWndOrig = NULL;

	if (hWndOrig != NULL)
	{
		// Note that 'this' may have been deleted at this point,
		//  (but only if pWnd != NULL)
		if (pWnd != NULL)
		{
			// Should have been detached by OnNcDestroy
		}
		else
		{
		// Detach after DestroyWindow called just in case
			Detach();
		}
	}

	return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// Default CWnd implementation

LRESULT CWnd::DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// if (m_pfnSuper != NULL)
		// return ::CallWindowProc(m_pfnSuper, m_hWnd, nMsg, wParam, lParam);

	// WNDPROC pfnWndProc;
	// if ((pfnWndProc = *GetSuperWndProcAddr()) == NULL)
		// return ::DefWindowProc(m_hWnd, nMsg, wParam, lParam);
	// else
		// return ::CallWindowProc(pfnWndProc, m_hWnd, nMsg, wParam, lParam);
	return 0;
}

WNDPROC* CWnd::GetSuperWndProcAddr()
{
	// Note: it is no longer necessary to override GetSuperWndProcAddr
	//  for each control class with a different WNDCLASS.
	//  This implementation now uses instance data, such that the previous
	//  WNDPROC can be anything.

	return &m_pfnSuper;
}

BOOL CWnd::PreTranslateMessage(MSG* pMsg)
{
	// handle tooltip messages (some messages cancel, some may cause it to popup)
	AFX_MODULE_STATE* pModuleState = _AFX_CMDTARGET_GETSTATE();
	if (pModuleState->m_pfnFilterToolTipMessage != NULL)
		(*pModuleState->m_pfnFilterToolTipMessage)(pMsg, this);

	// no default processing
	return FALSE;
}

void CWnd::GetWindowText(CString& rString) const
{
	// ASSERT(::IsWindow(m_hWnd));
}

/////////////////////////////////////////////////////////////////////////////
// CWnd will delegate owner draw messages to self drawing controls

// Drawing: for all 4 control types
void CWnd::OnDrawItem(int /*nIDCtl*/, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct->CtlType == ODT_MENU)
	{
	}

	// reflect notification to child window control
	// if (ReflectLastMsg(lpDrawItemStruct->hwndItem))
		// return;     // eat it

	// not handled - do default
	Default();
}

// Drawing: for all 4 control types
int CWnd::OnCompareItem(int /*nIDCtl*/, LPCOMPAREITEMSTRUCT lpCompareItemStruct)
{
	// reflect notification to child window control
	// not handled - do default
	return (int)Default();
}

void CWnd::OnDeleteItem(int /*nIDCtl*/, LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
	// reflect notification to child window control
	// if (ReflectLastMsg(lpDeleteItemStruct->hwndItem))
		// return;     // eat it
	// not handled - do default
	Default();
}

// Measure item implementation relies on unique control/menu IDs
void CWnd::OnMeasureItem(int /*nIDCtl*/, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// not handled - do default
	Default();
}

/////////////////////////////////////////////////////////////////////////////
// Additional helpers for WNDCLASS init

// like RegisterClass, except will automatically call UnregisterClass
BOOL AFXAPI AfxRegisterClass(WNDCLASS* lpWndClass)
{
	WNDCLASS wndcls;
	// if (AfxCtxGetClassInfo(lpWndClass->hInstance, lpWndClass->lpszClassName,
		// &wndcls))
	{
		// class already registered
		return TRUE;
	}

	// if (!::AfxCtxRegisterClass(lpWndClass))
	// {
		// TRACE(traceAppMsg, 0, _T("Can't register window class named %s\n"),
			// lpWndClass->lpszClassName);
		// return FALSE;
	// }

	BOOL bRet = TRUE;

	if (afxContextIsDLL)
	{
		AfxLockGlobals(CRIT_REGCLASSLIST);
		TRY
		{
			// class registered successfully, add to registered list
			AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
			pModuleState->m_strUnregisterList+=lpWndClass->lpszClassName;
			pModuleState->m_strUnregisterList+='\n';
		}
		CATCH_ALL(e)
		{
			AfxUnlockGlobals(CRIT_REGCLASSLIST);
			THROW_LAST();
			// Note: DELETE_EXCEPTION not required.
		}
		END_CATCH_ALL
		AfxUnlockGlobals(CRIT_REGCLASSLIST);
	}

	return bRet;
}

LPCTSTR AFXAPI AfxRegisterWndClass(UINT nClassStyle,
	HCURSOR hCursor, HBRUSH hbrBackground, HICON hIcon)
{
	// Returns a temporary string name for the class
	//  Save in a CString if you want to use it for a long time
	LPTSTR lpszName = AfxGetThreadState()->m_szTempClassName;

	// generate a synthetic name for this class
	HINSTANCE hInst = AfxGetInstanceHandle();

	if (hCursor == NULL && hbrBackground == NULL && hIcon == NULL)
	{
		ATL_CRT_ERRORCHECK_SPRINTF(_sntprintf_s(lpszName, _AFX_TEMP_CLASS_NAME_SIZE, _AFX_TEMP_CLASS_NAME_SIZE - 1, _T("Afx:%p:%x"), hInst, nClassStyle));
	}
	else
	{
		ATL_CRT_ERRORCHECK_SPRINTF(_sntprintf_s(lpszName, _AFX_TEMP_CLASS_NAME_SIZE, _AFX_TEMP_CLASS_NAME_SIZE - 1, _T("Afx:%p:%x:%p:%p:%p"), hInst, nClassStyle,
			hCursor, hbrBackground, hIcon));
	}

	// see if the class already exists
	WNDCLASS wndcls;
	/*if (::AfxCtxGetClassInfo(hInst, lpszName, &wndcls))
	{
		// already registered, assert everything is good
		ASSERT(wndcls.style == nClassStyle);

		// NOTE: We have to trust that the hIcon, hbrBackground, and the
		//  hCursor are semantically the same, because sometimes Windows does
		//  some internal translation or copying of those handles before
		//  storing them in the internal WNDCLASS retrieved by GetClassInfo.
		return lpszName;
	}*/

	// otherwise we need to register a new class
	wndcls.style = nClassStyle;
	wndcls.lpfnWndProc = DefWindowProc;
	wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
	wndcls.hInstance = hInst;
	wndcls.hIcon = hIcon;
	wndcls.hCursor = hCursor;
	wndcls.hbrBackground = hbrBackground;
	wndcls.lpszMenuName = NULL;
	wndcls.lpszClassName = lpszName;
	// if (!AfxRegisterClass(&wndcls))
		// AfxThrowResourceException();

	// return thread-local pointer
	return lpszName;
}

struct AFX_CTLCOLOR
{
	HWND hWnd;
	HDC hDC;
	UINT nCtlType;
};

/////////////////////////////////////////////////////////////////////////////
// Multi-touch support:

BOOL CWnd::RegisterTouchWindow(BOOL bRegister, ULONG ulFlags)
{
	// m_bIsTouchWindowRegistered = FALSE;

	// static HMODULE hUserDll = AfxCtxLoadLibrary(_T("user32.dll"));
	// ENSURE(hUserDll != NULL);

	// typedef	BOOL (__stdcall *PFNREGISTERTOUCHWINDOW)(HWND, ULONG);
	// typedef	BOOL (__stdcall *PFNUNREGISTERTOUCHWINDOW)(HWND);

	// static PFNREGISTERTOUCHWINDOW pfRegister = (PFNREGISTERTOUCHWINDOW)GetProcAddress(hUserDll, "RegisterTouchWindow");
	// static PFNUNREGISTERTOUCHWINDOW pfUnregister = (PFNUNREGISTERTOUCHWINDOW)GetProcAddress(hUserDll, "UnregisterTouchWindow");

	// if (pfRegister == NULL || pfUnregister == NULL)
	// {
		// return FALSE;
	// }

	// if (!bRegister)
	// {
		// return (*pfUnregister)(GetSafeHwnd());
	// }

	// m_bIsTouchWindowRegistered = (*pfRegister)(GetSafeHwnd(), ulFlags);
	// return m_bIsTouchWindowRegistered;
	return false;
}

BOOL CWnd::IsTouchWindow() const
{
	// static HMODULE hUserDll = AfxCtxLoadLibrary(_T("user32.dll"));
	// ENSURE(hUserDll != NULL);

	// typedef	BOOL (__stdcall *PFNISTOUCHWINDOW)(HWND);
	// static PFNISTOUCHWINDOW pfIsTouchWindow = (PFNISTOUCHWINDOW)GetProcAddress(hUserDll, "IsTouchWindow");

	// if (pfIsTouchWindow == NULL)
	// {
		// return FALSE;
	// }

	// return (*pfIsTouchWindow)(GetSafeHwnd());
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Message table implementation

BEGIN_MESSAGE_MAP(CWnd, CCmdTarget)
	// ON_MESSAGE(WM_CTLCOLORSTATIC, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLOREDIT, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLORBTN, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLORLISTBOX, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLORDLG, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLORMSGBOX, &CWnd::OnNTCtlColor)
	// ON_MESSAGE(WM_CTLCOLORSCROLLBAR, &CWnd::OnNTCtlColor)
	//{{AFX_MSG_MAP(CWnd)
		ON_WM_SETFOCUS()
	// ON_WM_DRAWITEM()
	// ON_WM_MEASUREITEM()
	// ON_WM_CTLCOLOR()
//	ON_WM_DELETEITEM()
	// ON_WM_SYSCOLORCHANGE()
	// ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
#ifndef _AFX_NO_OCC_SUPPORT
	ON_WM_DESTROY()
#endif
//	ON_MESSAGE(WM_ACTIVATETOPLEVEL, &CWnd::OnActivateTopLevel)
//	ON_MESSAGE(WM_DISPLAYCHANGE, &CWnd::OnDisplayChange)
	// ON_REGISTERED_MESSAGE(CWnd::m_nMsgDragList, &CWnd::OnDragList)
//	ON_MESSAGE(WM_GETOBJECT, &CWnd::OnGetObject)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Routines for fast search of message maps

const AFX_MSGMAP_ENTRY* AFXAPI
AfxFindMessageEntry(const AFX_MSGMAP_ENTRY* lpEntry,
	UINT nMsg, UINT nCode, UINT nID)
{
#if defined(_M_IX86) && !defined(_AFX_PORTABLE)
// 32-bit Intel 386/486 version.

	ASSERT(offsetof(AFX_MSGMAP_ENTRY, nMessage) == 0);
	ASSERT(offsetof(AFX_MSGMAP_ENTRY, nCode) == 4);
	ASSERT(offsetof(AFX_MSGMAP_ENTRY, nID) == 8);
	ASSERT(offsetof(AFX_MSGMAP_ENTRY, nLastID) == 12);
	ASSERT(offsetof(AFX_MSGMAP_ENTRY, nSig) == 16);

	_asm
	{
			MOV     EBX,lpEntry
			MOV     EAX,nMsg
			MOV     EDX,nCode
			MOV     ECX,nID
	__loop:
			CMP     DWORD PTR [EBX+16],0        ; nSig (0 => end)
			JZ      __failed
			CMP     EAX,DWORD PTR [EBX]         ; nMessage
			JE      __found_message
	__next:
			ADD     EBX,SIZE AFX_MSGMAP_ENTRY
			JMP     short __loop
	__found_message:
			CMP     EDX,DWORD PTR [EBX+4]       ; nCode
			JNE     __next
	// message and code good so far
	// check the ID
			CMP     ECX,DWORD PTR [EBX+8]       ; nID
			JB      __next
			CMP     ECX,DWORD PTR [EBX+12]      ; nLastID
			JA      __next
	// found a match
			MOV     lpEntry,EBX                 ; return EBX
			JMP     short __end
	__failed:
			XOR     EAX,EAX                     ; return NULL
			MOV     lpEntry,EAX
	__end:
	}
	return lpEntry;
#else  // _AFX_PORTABLE
	// C version of search routine
	while (lpEntry->nSig != AfxSig_end)
	{
		if (lpEntry->nMessage == nMsg && lpEntry->nCode == nCode &&
			nID >= lpEntry->nID && nID <= lpEntry->nLastID)
		{
			return lpEntry;
		}
		lpEntry++;
	}
	return NULL;    // not found
#endif  // _AFX_PORTABLE
}

/////////////////////////////////////////////////////////////////////////////
// Cache of most recently sent messages

// #ifndef iHashMax
// iHashMax must be a power of two
	#define iHashMax 16 // 512
// #endif

struct AFX_MSG_CACHE
{
	UINT nMsg;
	const AFX_MSGMAP_ENTRY* lpEntry;
	const AFX_MSGMAP* pMessageMap;
};

AFX_MSG_CACHE _afxMsgCache[iHashMax];

void AFXAPI AfxResetMsgCache()
{
	memset(_afxMsgCache, 0, sizeof(_afxMsgCache));
}

/////////////////////////////////////////////////////////////////////////////
// main WindowProc implementation

LRESULT CWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// OnWndMsg does most of the work, except for DefWindowProc call
	LRESULT lResult = 0;
	if (!OnWndMsg(message, wParam, lParam, &lResult))
		lResult = DefWindowProc(message, wParam, lParam);
	return lResult;
}

BOOL CWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LRESULT lResult = 0;
	union MessageMapFunctions mmf;
	mmf.pfn = 0;
	CInternalGlobalLock winMsgLock;
	// special case for commands
	if (message == WM_COMMAND)
	{
		if (OnCommand(wParam, lParam))
		{
			lResult = 1;
			goto LReturnTrue;
		}
		return FALSE;
	}

	// special case for notifies
	if (message == WM_NOTIFY)
	{
		NMHDR* pNMHDR = (NMHDR*)lParam;
		if (pNMHDR->hwndFrom != NULL && OnNotify(wParam, lParam, &lResult))
			goto LReturnTrue;
		return FALSE;
	}

	// special case for activation
	if (message == WM_ACTIVATE)
		_AfxHandleActivate(this, wParam, CWnd::FromHandle((HWND)lParam));

	// special case for set cursor HTERROR
	if (message == WM_SETCURSOR &&
		_AfxHandleSetCursor(this, (short)LOWORD(lParam), HIWORD(lParam)))
	{
		lResult = 1;
		goto LReturnTrue;
	}

   // special case for windows that contain windowless ActiveX controls
   BOOL bHandled;

   bHandled = FALSE;
   /*if ((m_pCtrlCont != NULL) && (m_pCtrlCont->m_nWindowlessControls > 0))
   {
    if (((message >= WM_MOUSEFIRST) && (message <= AFX_WM_MOUSELAST)) ||
     ((message >= WM_KEYFIRST) && (message <= WM_IME_KEYLAST)) ||
     ((message >= WM_IME_SETCONTEXT) && (message <= WM_IME_KEYUP)))
    {
     bHandled = m_pCtrlCont->HandleWindowlessMessage(message, wParam, lParam, &lResult);
    }
   }*/
   if (bHandled)
   {
    goto LReturnTrue;
   }

	const AFX_MSGMAP* pMessageMap; pMessageMap = GetMessageMap();
	UINT iHash; iHash = (LOWORD((DWORD_PTR)pMessageMap) ^ message) & (iHashMax-1);
	winMsgLock.Lock(CRIT_WINMSGCACHE);
	AFX_MSG_CACHE* pMsgCache; pMsgCache = &_afxMsgCache[iHash];
	const AFX_MSGMAP_ENTRY* lpEntry;
	if (message == pMsgCache->nMsg && pMessageMap == pMsgCache->pMessageMap)
	{
		// cache hit
		lpEntry = pMsgCache->lpEntry;
		winMsgLock.Unlock();
		if (lpEntry == NULL)
			return FALSE;

		// cache hit, and it needs to be handled
		if (message < 0xC000)
			goto LDispatch;
		else
			goto LDispatchRegistered;
	}
	else
	{
		// not in cache, look for it
		pMsgCache->nMsg = message;
		pMsgCache->pMessageMap = pMessageMap;

		for (/* pMessageMap already init'ed */; pMessageMap->pfnGetBaseMap != NULL;
			pMessageMap = (*pMessageMap->pfnGetBaseMap)())
		{
			// Note: catch not so common but fatal mistake!!
			//      BEGIN_MESSAGE_MAP(CMyWnd, CMyWnd)
			ASSERT(pMessageMap != (*pMessageMap->pfnGetBaseMap)());
			if (message < 0xC000)
			{
				// constant window message
				if ((lpEntry = AfxFindMessageEntry(pMessageMap->lpEntries,
					message, 0, 0)) != NULL)
				{
					pMsgCache->lpEntry = lpEntry;
					winMsgLock.Unlock();
					goto LDispatch;
				}
			}
			else
			{
				// registered windows message
				lpEntry = pMessageMap->lpEntries;
				while ((lpEntry = AfxFindMessageEntry(lpEntry, 0xC000, 0, 0)) != NULL)
				{
					UINT* pnID = (UINT*)(lpEntry->nSig);
					ASSERT(*pnID >= 0xC000 || *pnID == 0);
						// must be successfully registered
					if (*pnID == message)
					{
						pMsgCache->lpEntry = lpEntry;
						winMsgLock.Unlock();
						goto LDispatchRegistered;
					}
					lpEntry++;      // keep looking past this one
				}
			}
		}

		pMsgCache->lpEntry = NULL;
		winMsgLock.Unlock();
		return FALSE;
	}

LDispatch:
	ASSERT(message < 0xC000);

	mmf.pfn = lpEntry->pfn;

	switch (lpEntry->nSig)
	{
	default:
		ASSERT(FALSE);
		break;

	case AfxSig_b_b_v:
		lResult = (this->*mmf.pfn_b_b)(static_cast<BOOL>(wParam));
		break;

	case AfxSig_b_u_v:
		lResult = (this->*mmf.pfn_b_u)(static_cast<UINT>(wParam));
		break;

	case AfxSig_b_h_v:
		lResult = (this->*mmf.pfn_b_h)(reinterpret_cast<HANDLE>(wParam));
		break;

	case AfxSig_i_u_v:
		lResult = (this->*mmf.pfn_i_u)(static_cast<UINT>(wParam));
		break;

	case AfxSig_C_v_v:
		lResult = reinterpret_cast<LRESULT>((this->*mmf.pfn_C_v)());
		break;

	case AfxSig_v_u_W:
		(this->*mmf.pfn_v_u_W)(static_cast<UINT>(wParam),
			CWnd::FromHandle(reinterpret_cast<HWND>(lParam)));
		break;

	case AfxSig_u_u_v:
		lResult = (this->*mmf.pfn_u_u)(static_cast<UINT>(wParam));
		break;

	case AfxSig_b_v_v:
		lResult = (this->*mmf.pfn_b_v)();
		break;

	case AfxSig_b_W_uu:
		lResult = (this->*mmf.pfn_b_W_u_u)(CWnd::FromHandle(reinterpret_cast<HWND>(wParam)),
			LOWORD(lParam), HIWORD(lParam));
		break;

	case AfxSig_b_W_COPYDATASTRUCT:
		lResult = (this->*mmf.pfn_b_W_COPYDATASTRUCT)(
			CWnd::FromHandle(reinterpret_cast<HWND>(wParam)),
			reinterpret_cast<COPYDATASTRUCT*>(lParam));
		break;

	case AfxSig_b_v_HELPINFO:
		lResult = (this->*mmf.pfn_b_HELPINFO)(reinterpret_cast<LPHELPINFO>(lParam));
		break;



	case AfxSig_i_u_W_u:
		lResult = (this->*mmf.pfn_i_u_W_u)(LOWORD(wParam),
			CWnd::FromHandle(reinterpret_cast<HWND>(lParam)), HIWORD(wParam));
		break;

	case AfxSig_i_uu_v:
		lResult = (this->*mmf.pfn_i_u_u)(LOWORD(wParam), HIWORD(wParam));
		break;

	case AfxSig_i_W_uu:
		lResult = (this->*mmf.pfn_i_W_u_u)(CWnd::FromHandle(reinterpret_cast<HWND>(wParam)),
			LOWORD(lParam), HIWORD(lParam));
		break;

	case AfxSig_i_v_s:
		lResult = (this->*mmf.pfn_i_s)(reinterpret_cast<LPTSTR>(lParam));
		break;

	case AfxSig_l_w_l:
		lResult = (this->*mmf.pfn_l_w_l)(wParam, lParam);
		break;

	case AfxSig_l_uu_M:
		break;

	case AfxSig_v_b_h:
			(this->*mmf.pfn_v_b_h)(static_cast<BOOL>(wParam),
			reinterpret_cast<HANDLE>(lParam));
		break;

	case AfxSig_v_h_v:
			(this->*mmf.pfn_v_h)(reinterpret_cast<HANDLE>(wParam));
		break;

	case AfxSig_v_h_h:
			(this->*mmf.pfn_v_h_h)(reinterpret_cast<HANDLE>(wParam),
			reinterpret_cast<HANDLE>(lParam));
		break;

	case AfxSig_v_v_v:
		(this->*mmf.pfn_v_v)();
		break;

	case AfxSig_v_u_v:
		(this->*mmf.pfn_v_u)(static_cast<UINT>(wParam));
		break;

	case AfxSig_v_u_u:
		(this->*mmf.pfn_v_u_u)(static_cast<UINT>(wParam), static_cast<UINT>(lParam));
		break;

	case AfxSig_v_uu_v:
		(this->*mmf.pfn_v_u_u)(LOWORD(wParam), HIWORD(wParam));
		break;

	case AfxSig_v_v_ii:
		(this->*mmf.pfn_v_i_i)(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case AfxSig_v_u_uu:
		(this->*mmf.pfn_v_u_u_u)(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		break;

	case AfxSig_v_u_ii:
		(this->*mmf.pfn_v_u_i_i)(static_cast<UINT>(wParam), LOWORD(lParam), HIWORD(lParam));
		break;

	case AfxSig_v_w_l:
		(this->*mmf.pfn_v_w_l)(wParam, lParam);
		break;

	case AfxSig_v_M_v:
		break;

	case AfxSig_v_M_ub:
		break;

	case AfxSig_v_W_v:
		(this->*mmf.pfn_v_W)(CWnd::FromHandle(reinterpret_cast<HWND>(wParam)));
		break;

	case AfxSig_v_v_W:
		(this->*mmf.pfn_v_W)(CWnd::FromHandle(reinterpret_cast<HWND>(lParam)));
		break;

	case AfxSig_v_W_uu:
		(this->*mmf.pfn_v_W_u_u)(CWnd::FromHandle(reinterpret_cast<HWND>(wParam)), LOWORD(lParam),
			HIWORD(lParam));
		break;

	case AfxSig_v_W_h:
		(this->*mmf.pfn_v_W_h)(CWnd::FromHandle(reinterpret_cast<HWND>(wParam)),
				reinterpret_cast<HANDLE>(lParam));
		break;

	case AfxSig_ACTIVATE:
		(this->*mmf.pfn_v_u_W_b)(LOWORD(wParam),
			CWnd::FromHandle(reinterpret_cast<HWND>(lParam)), HIWORD(wParam));
		break;

	case AfxSig_SCROLL:
	case AfxSig_SCROLL_REFLECT:
		{
			// special case for WM_VSCROLL and WM_HSCROLL
			ASSERT(message == WM_VSCROLL || message == WM_HSCROLL ||
				message == WM_VSCROLL+WM_REFLECT_BASE || message == WM_HSCROLL+WM_REFLECT_BASE);
			int nScrollCode = (short)LOWORD(wParam);
			int nPos = (short)HIWORD(wParam);
			if (lpEntry->nSig == AfxSig_SCROLL)
				(this->*mmf.pfn_v_u_u_W)(nScrollCode, nPos,
					CWnd::FromHandle(reinterpret_cast<HWND>(lParam)));
			else
				(this->*mmf.pfn_v_u_u)(nScrollCode, nPos);
		}
		break;

	case AfxSig_v_v_s:
		(this->*mmf.pfn_v_s)(reinterpret_cast<LPTSTR>(lParam));
		break;

	case AfxSig_v_u_cs:
		(this->*mmf.pfn_v_u_cs)(static_cast<UINT>(wParam), reinterpret_cast<LPCTSTR>(lParam));
		break;

	case AfxSig_OWNERDRAW:
		(this->*mmf.pfn_v_i_s)(static_cast<int>(wParam), reinterpret_cast<LPTSTR>(lParam));
		lResult = TRUE;
		break;

	case AfxSig_i_i_s:
		lResult = (this->*mmf.pfn_i_i_s)(static_cast<int>(wParam), reinterpret_cast<LPTSTR>(lParam));
		break;

	case AfxSig_u_v_v:
		lResult = (this->*mmf.pfn_u_v)();
		break;

	case AfxSig_v_b_NCCALCSIZEPARAMS:
		(this->*mmf.pfn_v_b_NCCALCSIZEPARAMS)(static_cast<BOOL>(wParam),
			reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam));
		break;

	case AfxSig_v_v_WINDOWPOS:
		(this->*mmf.pfn_v_v_WINDOWPOS)(reinterpret_cast<WINDOWPOS*>(lParam));
		break;

	case AfxSig_v_uu_M:
		(this->*mmf.pfn_v_u_u_M)(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HMENU>(lParam));
		break;

	case AfxSig_SIZING:
		(this->*mmf.pfn_v_u_pr)(static_cast<UINT>(wParam), reinterpret_cast<LPRECT>(lParam));
		lResult = TRUE;
		break;

	case AfxSig_RAWINPUT:
		(this->*mmf.pfn_RAWINPUT)(static_cast<UINT>(GET_RAWINPUT_CODE_WPARAM(wParam)), reinterpret_cast<HRAWINPUT>(lParam));
		break;
	case AfxSig_u_u_u:
		lResult = (this->*mmf.pfn_u_u_u)(static_cast<UINT>(wParam), static_cast<UINT>(lParam));
		break;
	// case AfxSig_INPUTDEVICECHANGE:
		// (this->*mmf.pfn_INPUTDEVICECHANGE)(GET_DEVICE_CHANGE_WPARAM(wParam), reinterpret_cast<HANDLE>(lParam));
		// break;
	case AfxSig_v_u_hkl:
		(this->*mmf.pfn_v_u_h)(static_cast<UINT>(wParam), reinterpret_cast<HKL>(lParam));
		break;
	}
	goto LReturnTrue;

LDispatchRegistered:    // for registered windows messages
	ASSERT(message >= 0xC000);
	ASSERT(sizeof(mmf) == sizeof(mmf.pfn));
	mmf.pfn = lpEntry->pfn;
	lResult = (this->*mmf.pfn_l_w_l)(wParam, lParam);

LReturnTrue:
	if (pResult != NULL)
		*pResult = lResult;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTestCmdUI - used to test for disabled commands before dispatching

class CTestCmdUI : public CCmdUI
{
public:
	CTestCmdUI();

public: // re-implementations only
	virtual void Enable(BOOL bOn);
	virtual void SetCheck(int nCheck);
	virtual void SetRadio(BOOL bOn);
	virtual void SetText(LPCTSTR);

	BOOL m_bEnabled;
};

CTestCmdUI::CTestCmdUI()
{
	m_bEnabled = TRUE;  // assume it is enabled
}

void CTestCmdUI::Enable(BOOL bOn)
{
	m_bEnabled = bOn;
	m_bEnableChanged = TRUE;
}

void CTestCmdUI::SetCheck(int)
{
	// do nothing -- just want to know about calls to Enable
}

void CTestCmdUI::SetRadio(BOOL)
{
	// do nothing -- just want to know about calls to Enable
}

void CTestCmdUI::SetText(LPCTSTR)
{
	// do nothing -- just want to know about calls to Enable
}

/////////////////////////////////////////////////////////////////////////////
// CWnd command handling

BOOL CWnd::OnCommand(WPARAM wParam, LPARAM lParam)
	// return TRUE if command invocation was attempted
{
	UINT nID = LOWORD(wParam);
	HWND hWndCtrl = (HWND)lParam;
	int nCode = HIWORD(wParam);

	// default routing for command messages (through closure table)

	if (hWndCtrl == NULL)
	{
		// zero IDs for normal commands are not allowed
		if (nID == 0)
			return FALSE;

		// make sure command has not become disabled before routing
		CTestCmdUI state;
		state.m_nID = nID;
		OnCmdMsg(nID, CN_UPDATE_COMMAND_UI, &state, NULL);
		if (!state.m_bEnabled)
		{
			return TRUE;
		}

		// menu or accelerator
		nCode = CN_COMMAND;
	}
	else
	{
		// control notification
		ASSERT(nID == 0 || ::IsWindow(hWndCtrl));

		// reflect notification to child window control
		// if (ReflectLastMsg(hWndCtrl))
			// return TRUE;    // eaten by child

		// zero IDs for normal commands are not allowed
		if (nID == 0)
			return FALSE;
	}

	return OnCmdMsg(nID, nCode, NULL, NULL);
}

BOOL CWnd::OnNotify(WPARAM, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);
	NMHDR* pNMHDR = (NMHDR*)lParam;
	HWND hWndCtrl = pNMHDR->hwndFrom;

	// get the child ID from the window itself
	UINT_PTR nID = _AfxGetDlgCtrlID(hWndCtrl);
	int nCode = pNMHDR->code;

	ASSERT(hWndCtrl != NULL);
	ASSERT(::IsWindow(hWndCtrl));

	// reflect notification to child window control
	// if (ReflectLastMsg(hWndCtrl, pResult))
		// return TRUE;        // eaten by child

	AFX_NOTIFY notify;
	notify.pResult = pResult;
	notify.pNMHDR = pNMHDR;
	return OnCmdMsg((UINT)nID, MAKELONG(nCode, WM_NOTIFY), &notify, NULL);
}

HWND AFXAPI AfxGetParentOwner(HWND hWnd)
{
	// check for permanent-owned window first
	CWnd* pWnd = CWnd::FromHandlePermanent(hWnd);
	if (pWnd != NULL)
		return pWnd->GetOwner()->GetSafeHwnd();

	// otherwise, return parent in the Windows sense
	return (::GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) ?
		::GetParent(hWnd) : ::GetWindow(hWnd, GW_OWNER);
}

CWnd* CWnd::GetTopLevelParent() const
{
	if (GetSafeHwnd() == NULL) // no Window attached
		return NULL;

	ASSERT_VALID(this);

	return NULL; // CWnd::FromHandle(hWndParent);
}

CWnd* CWnd::GetTopLevelOwner() const
{
	if (GetSafeHwnd() == NULL) // no Window attached
		return NULL;

	ASSERT_VALID(this);

	return 0; // CWnd::FromHandle(hWndOwner);
}

CWnd* CWnd::GetParentOwner() const
{
	if (GetSafeHwnd() == NULL) // no Window attached
		return NULL;

	ASSERT_VALID(this);

	return 0; // CWnd::FromHandle(hWndParent);
}

BOOL CWnd::IsTopParentActive() const
{
	ASSERT_VALID(this);

	return false;
}

void CWnd::ActivateTopParent()
{
}

CWnd* PASCAL CWnd::GetSafeOwner(CWnd* pParent, HWND* pWndTop)
{
	HWND hWnd = 0; // GetSafeOwner_(pParent->GetSafeHwnd(), pWndTop);
	return CWnd::FromHandle(hWnd);
}

CWnd* PASCAL CWnd::GetDescendantWindow(HWND hWnd, int nID, BOOL bOnlyPerm)
{
	return NULL;    // not found
}

void PASCAL CWnd::SendMessageToDescendants(HWND hWnd, UINT message,
	WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm)
{
	// walk through HWNDs to avoid creating temporary CWnd objects
	// unless we need to call this function recursively
	for (HWND hWndChild = ::GetTopWindow(hWnd); hWndChild != NULL;
		hWndChild = ::GetNextWindow(hWndChild, GW_HWNDNEXT))
	{
		// if bOnlyPerm is TRUE, don't send to non-permanent windows
		if (bOnlyPerm)
		{
			CWnd* pWnd = CWnd::FromHandlePermanent(hWndChild);
			if (pWnd != NULL)
			{
				// call window proc directly since it is a C++ window
				// AfxCallWndProc(pWnd, pWnd->m_hWnd, message, wParam, lParam);
			}
		}
		else
		{
			// send message with Windows SendMessage API
			::SendMessage(hWndChild, message, wParam, lParam);
		}
		if (bDeep && ::GetTopWindow(hWndChild) != NULL)
		{
			// send to child windows after parent
			SendMessageToDescendants(hWndChild, message, wParam, lParam,
				bDeep, bOnlyPerm);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Scroll bar helpers
//  hook for CWnd functions
//    only works for derived class (eg: CView) that override 'GetScrollBarCtrl'
// if the window doesn't have a _visible_ windows scrollbar - then
//   look for a sibling with the appropriate ID

void CWnd::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
}

/////////////////////////////////////////////////////////////////////////////
BOOL PASCAL CWnd::WalkPreTranslateTree(HWND hWndStop, MSG* pMsg)
{
	ASSERT(hWndStop == NULL || ::IsWindow(hWndStop));
	ASSERT(pMsg != NULL);
	return FALSE;       // no special processing
}

BOOL CWnd::SendChildNotifyLastMsg(LRESULT* pResult)
{
	_AFX_THREAD_STATE* pThreadState = _afxThreadState.GetData();
	return OnChildNotify(pThreadState->m_lastSentMsg.message,
		pThreadState->m_lastSentMsg.wParam, pThreadState->m_lastSentMsg.lParam, pResult);
}

BOOL CWnd::OnChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
#ifndef _AFX_NO_OCC_SUPPORT
	if (m_pCtrlSite != NULL)
	{
		// first forward raw OCM_ messages to OLE control sources
		LRESULT lResult = SendMessage(OCM__BASE+uMsg, wParam, lParam);
		if (uMsg >= WM_CTLCOLORMSGBOX && uMsg <= WM_CTLCOLORSTATIC &&
			(HBRUSH)lResult == NULL)
		{
			// for WM_CTLCOLOR msgs, returning NULL implies continue routing
			return FALSE;
		}
		if (pResult != NULL)
			*pResult = lResult;
		return TRUE;
	}
#endif

	return ReflectChildNotify(uMsg, wParam, lParam, pResult);
}

BOOL CWnd::ReflectChildNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Note: reflected messages are send directly to CWnd::OnWndMsg
	//  and CWnd::OnCmdMsg for speed and because these messages are not
	//  routed by normal OnCmdMsg routing (they are only dispatched)

	switch (uMsg)
	{
	// normal messages (just wParam, lParam through OnWndMsg)
	case WM_HSCROLL:
	case WM_VSCROLL:
	case WM_PARENTNOTIFY:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_DELETEITEM:
	case WM_VKEYTOITEM:
	case WM_CHARTOITEM:
	case WM_COMPAREITEM:
		// reflect the message through the message map as WM_REFLECT_BASE+uMsg
		return CWnd::OnWndMsg(WM_REFLECT_BASE+uMsg, wParam, lParam, pResult);

	// special case for WM_COMMAND
	case WM_COMMAND:
		{
			// reflect the message through the message map as OCM_COMMAND
			int nCode = HIWORD(wParam);
			if (CWnd::OnCmdMsg(0, MAKELONG(nCode, WM_REFLECT_BASE+WM_COMMAND), NULL, NULL))
			{
				if (pResult != NULL)
					*pResult = 1;
				return TRUE;
			}
		}
		break;

	// special case for WM_NOTIFY
	case WM_NOTIFY:
		{
			// reflect the message through the message map as OCM_NOTIFY
			NMHDR* pNMHDR = (NMHDR*)lParam;
			int nCode = pNMHDR->code;
			AFX_NOTIFY notify;
			notify.pResult = pResult;
			notify.pNMHDR = pNMHDR;
			return CWnd::OnCmdMsg(0, MAKELONG(nCode, WM_REFLECT_BASE+WM_NOTIFY), &notify, NULL);
		}

	// other special cases (WM_CTLCOLOR family)
	default:
		break;
	}

	return FALSE;   // let the parent handle it
}

void CWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	if ((LOWORD(message) == WM_CREATE || LOWORD(message) == WM_DESTROY))
	{
		// if (ReflectLastMsg((HWND)lParam))
			// return;     // eat it
	}
	// not handled - do default
	Default();
}

void CWnd::OnSetFocus(CWnd*)
{
   BOOL bHandled;

   bHandled = FALSE;
#ifndef _AFX_NO_OCC_SUPPORT
   if (m_pCtrlCont != NULL)
   {
    bHandled = m_pCtrlCont->HandleSetFocus();
   }
#endif  //!_AFX_NO_OCC_SUPPORT
   if( !bHandled )
   {
    Default();
   }
}

LRESULT CWnd::OnActivateTopLevel(WPARAM wParam, LPARAM)
{
  return 0;
}

BOOL _afxGotScrollLines;

BOOL CWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	return Default() != 0;
}

LRESULT CWnd::OnDragList(WPARAM, LPARAM lParam)
{
	return (int)Default();
}

// Accessibility
LRESULT CWnd::OnGetObject(WPARAM wParam, LPARAM lParam)
{
	if (!m_bEnableActiveAccessibility)
		return Default();

	LRESULT lRet = 0;
	return lRet;
}

//Helper class for Active Accesibility Proxy
//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
void AFXAPI AfxOleLockApp();
void AFXAPI AfxOleUnlockApp();

template <class Base>
class CMFCComObject : public Base
{
public:
	typedef Base _BaseClass;
	CMFCComObject(void* = NULL)
	{
		AfxOleLockApp();
	}
	// Set refcount to -(LONG_MAX/2) to protect destruction and
	// also catch mismatched Release in debug builds
	~CMFCComObject()
	{
		// m_dwRef = -(LONG_MAX/2);
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		AfxOleUnlockApp();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() throw() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)() throw()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
	{return _InternalQueryInterface(iid, ppvObject);}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}

	static HRESULT WINAPI CreateInstance(CMFCComObject<Base>** pp)
	{
		ATLASSERT(pp != NULL);
		if (pp == NULL)
			return E_POINTER;
		*pp = NULL;

		HRESULT hRes = E_OUTOFMEMORY;
		CMFCComObject<Base>* p = NULL;
		ATLTRY(p = new CMFCComObject<Base>())
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->_AtlInitialConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->FinalConstruct();
			if (SUCCEEDED(hRes))
				hRes = p->_AtlFinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}
};

long CWnd::GetWindowedChildCount()
{
	long lCount = 0;
	for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT), lCount++);
	return lCount;
}

long CWnd::GetAccessibleChildCount()
{
		return 0; // GetWindowedChildCount() + GetWindowLessChildCount();
}

HRESULT CWnd::GetAccessibleChild(VARIANT varChild, IDispatch** ppdispChild)
{
	if (ppdispChild == NULL)
		return E_POINTER;
	*ppdispChild = NULL;

	long lCount = varChild.lVal - 1;
	if (lCount < 0)
		return E_INVALIDARG;

	CWnd* pChild = NULL;
	for(pChild = GetWindow(GW_CHILD); pChild != NULL && lCount != 0; pChild = pChild->GetWindow(GW_HWNDNEXT), lCount--);
	if (pChild != NULL)
	{
		// Found HWND
		return 0;
	}
	// Windowless controls don't support accessibility.
	return S_FALSE;
}


HRESULT CWnd::GetAccessibleName(VARIANT varChild, BSTR* pszName)
{
	//out of range
	return E_INVALIDARG;
}
HRESULT CWnd::GetAccessibilityLocation(VARIANT varChild, long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight)
{
	return 0;
}

HRESULT CWnd::GetAccessibilityHitTest(long xLeft, long yTop, VARIANT *pvarChild)
{
	return 0;
}

void CWnd::OnPaint()
{
   /*if (m_pCtrlCont != NULL)
   {
    // Paint windowless controls
    CPaintDC dc(this);
    m_pCtrlCont->OnPaint(&dc);
   }*/

   Default();
}

void CWnd::OnEnterIdle(UINT /*nWhy*/, CWnd* /*pWho*/)
{
	// In some OLE inplace active scenarios, OLE will post a
	// message instead of sending it.  This causes so many WM_ENTERIDLE
	// messages to be sent that tasks running in the background stop
	// running.  By dispatching the pending WM_ENTERIDLE messages
	// when the first one is received, we trick Windows into thinking
	// that only one was really sent and dispatched.
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, WM_ENTERIDLE, WM_ENTERIDLE, PM_REMOVE))
			DispatchMessage(&msg);
	}

	Default();
}

/////////////////////////////////////////////////////////////////////////////
// 'dialog data' support

BOOL CWnd::UpdateData(BOOL bSaveAndValidate)
{
	CDataExchange dx(this, bSaveAndValidate);

	// prevent control notifications from being dispatched during UpdateData
	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	HWND hWndOldLockout = pThreadState->m_hLockoutNotifyWindow;

	BOOL bOK = FALSE;       // assume failure
	TRY
	{
		DoDataExchange(&dx);
		bOK = TRUE;         // it worked
	}
	// CATCH(CUserException, e)
	// {
		// validation failed - user already alerted, fall through
		// ASSERT(!bOK);
		// Note: DELETE_EXCEPTION_(e) not required
	// }
	CATCH_ALL(e)
	{
		// validation failed due to OOM or other resource failure
		e->ReportError(MB_ICONEXCLAMATION, AFX_IDP_INTERNAL_FAILURE);
		ASSERT(!bOK);
		DELETE_EXCEPTION(e);
	}
	END_CATCH_ALL

	pThreadState->m_hLockoutNotifyWindow = hWndOldLockout;
	return bOK;
}

CDataExchange::CDataExchange(CWnd* pDlgWnd, BOOL bSaveAndValidate)
{
	ASSERT_VALID(pDlgWnd);
	m_bSaveAndValidate = bSaveAndValidate;
	m_pDlgWnd = pDlgWnd;
	m_idLastControl = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Centering dialog support (works for any non-child window)

BOOL CWnd::CheckAutoCenter()
{
	return TRUE;
}

BOOL CWnd::PreTranslateInput(LPMSG lpMsg)
{
	// don't translate non-input events
	if ((lpMsg->message < WM_KEYFIRST || lpMsg->message > WM_KEYLAST) &&
		(lpMsg->message < WM_MOUSEFIRST || lpMsg->message > AFX_WM_MOUSELAST))
		return FALSE;

	return IsDialogMessage(lpMsg);
}

int CWnd::RunModalLoop(DWORD dwFlags)
{
	ASSERT(!(m_nFlags & WF_MODALLOOP)); // window must not already be in modal state

	// for tracking the idle time state
	BOOL bIdle = TRUE;
	LONG lIdleCount = 0;
	BOOL bShowIdle = (dwFlags & MLF_SHOWONIDLE) && !(GetStyle() & WS_VISIBLE);
	m_nFlags |= (WF_MODALLOOP|WF_CONTINUEMODAL);
	MSG *pMsg = AfxGetCurrentMessage();

	// acquire and dispatch messages until the modal state is done
	for (;;)
	{
		ASSERT(ContinueModal());

		// phase1: check to see if we can do idle work
		while (bIdle &&
			!::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
		{
			ASSERT(ContinueModal());

			// show the dialog when the message queue goes idle
			if (bShowIdle)
			{
				ShowWindow(SW_SHOWNORMAL);
				UpdateWindow();
				bShowIdle = FALSE;
			}

			// call OnIdle while in bIdle state
			// if (!(dwFlags & MLF_NOIDLEMSG) && hWndParent != NULL && lIdleCount == 0)
			{
				// send WM_ENTERIDLE to the parent
			}
			if ((dwFlags & MLF_NOKICKIDLE) ||
				!SendMessage(WM_KICKIDLE, MSGF_DIALOGBOX, lIdleCount++))
			{
				// stop idle processing next time
				bIdle = FALSE;
			}
		}

		// phase2: pump messages while available
		do
		{
			ASSERT(ContinueModal());

			// pump message, but quit on WM_QUIT
			if (!AfxPumpMessage())
			{
				AfxPostQuitMessage(0);
				return -1;
			}

			// show the window when certain special messages rec'd
			if (bShowIdle &&
				(pMsg->message == 0x118 || pMsg->message == WM_SYSKEYDOWN))
			{
				ShowWindow(SW_SHOWNORMAL);
				UpdateWindow();
				bShowIdle = FALSE;
			}

			if (!ContinueModal())
				goto ExitModal;

			// reset "no idle" state after pumping "normal" message
			if (AfxIsIdleMessage(pMsg))
			{
				bIdle = TRUE;
				lIdleCount = 0;
			}

		} while (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE));
	}

ExitModal:
	m_nFlags &= ~(WF_MODALLOOP|WF_CONTINUEMODAL);
	return m_nModalResult;
}

BOOL CWnd::ContinueModal()
{
	return m_nFlags & WF_CONTINUEMODAL;
}

void CWnd::EndModalLoop(int nResult)
{
	// ASSERT(::IsWindow(m_hWnd));

	// this result will be returned from CWnd::RunModalLoop
	m_nModalResult = nResult;

	// make sure a message goes through to exit the modal loop
	if (m_nFlags & WF_CONTINUEMODAL)
	{
		m_nFlags &= ~WF_CONTINUEMODAL;
		PostMessage(WM_NULL);
	}
}

#ifndef _AFX_NO_OCC_SUPPORT

BOOL CWnd::SetOccDialogInfo(_AFX_OCC_DIALOG_INFO*)
{
	ASSERT(FALSE); // this class doesn't support dialog creation
	return FALSE;
}
_AFX_OCC_DIALOG_INFO* CWnd::GetOccDialogInfo()
{
	return NULL;
}

#endif

/////////////////////////////////////////////////////////////////////////////
// Standard init called by WinMain

AFX_STATIC BOOL AFXAPI _AfxRegisterWithIcon(WNDCLASS* pWndCls,
	LPCTSTR lpszClassName, UINT nIDIcon)
{
	pWndCls->lpszClassName = lpszClassName;
	HINSTANCE hInst = AfxFindResourceHandle(
		ATL_MAKEINTRESOURCE(nIDIcon), ATL_RT_GROUP_ICON);
	if ((pWndCls->hIcon = ::LoadIconW(hInst, ATL_MAKEINTRESOURCEW(nIDIcon))) == NULL)
	{
		// use default icon
		pWndCls->hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	}
	return AfxRegisterClass(pWndCls);
}

LONG AFXAPI _AfxInitCommonControls(LPINITCOMMONCONTROLSEX lpInitCtrls, LONG fToRegister)
{
	ASSERT(fToRegister != 0);

	LONG lResult = 0;
	// if (AFX_COMCTL32_IF_EXISTS(InitCommonControlsEx))
	{
		// if (AfxInitCommonControlsEx(lpInitCtrls))
		{
			// InitCommonControlsEx was successful so return the full mask
			// lResult = fToRegister;
		}
	}
	// else
	{
		// not there, so call InitCommonControls if possible
		if ((fToRegister & AFX_WIN95CTLS_MASK) == fToRegister)
		{
			// AfxInitCommonControls();
			lResult = AFX_WIN95CTLS_MASK;
		}
	}
	return lResult;
}

BOOL AFXAPI AfxEndDeferRegisterClass(LONG fToRegister)
{
	// mask off all classes that are already registered
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	fToRegister &= ~pModuleState->m_fRegisteredClasses;
	if (fToRegister == 0)
		return TRUE;

	LONG fRegisteredClasses = 0;

	// common initialization
	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL defaults
	wndcls.lpfnWndProc = DefWindowProc;
	wndcls.hInstance = AfxGetInstanceHandle();

	INITCOMMONCONTROLSEX init;
	init.dwSize = sizeof(init);

	// work to register classes as specified by fToRegister, populate fRegisteredClasses as we go
	if (fToRegister & AFX_WND_REG)
	{
		// Child windows - no brush, no icon, safest default class styles
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpszClassName = _afxWnd;
		if (AfxRegisterClass(&wndcls))
			fRegisteredClasses |= AFX_WND_REG;
	}
	if (fToRegister & AFX_WNDOLECONTROL_REG)
	{
		// OLE Control windows - use parent DC for speed
		wndcls.style |= CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpszClassName = _afxWndOleControl;
		if (AfxRegisterClass(&wndcls))
			fRegisteredClasses |= AFX_WNDOLECONTROL_REG;
	}
	if (fToRegister & AFX_WNDCONTROLBAR_REG)
	{
		// Control bar windows
		wndcls.style = 0;   // control bars don't handle double click
		wndcls.lpszClassName = _afxWndControlBar;
		wndcls.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		if (AfxRegisterClass(&wndcls))
			fRegisteredClasses |= AFX_WNDCONTROLBAR_REG;
	}
	if (fToRegister & AFX_WNDMDIFRAME_REG)
	{
		// MDI Frame window (also used for splitter window)
		wndcls.style = CS_DBLCLKS;
		wndcls.hbrBackground = NULL;
		if (_AfxRegisterWithIcon(&wndcls, _afxWndMDIFrame, AFX_IDI_STD_MDIFRAME))
			fRegisteredClasses |= AFX_WNDMDIFRAME_REG;
	}
	if (fToRegister & AFX_WNDFRAMEORVIEW_REG)
	{
		// SDI Frame or MDI Child windows or views - normal colors
		wndcls.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		if (_AfxRegisterWithIcon(&wndcls, _afxWndFrameOrView, AFX_IDI_STD_FRAME))
			fRegisteredClasses |= AFX_WNDFRAMEORVIEW_REG;
	}
	if (fToRegister & AFX_WNDCOMMCTLS_REG)
	{
		// this flag is compatible with the old InitCommonControls() API
		init.dwICC = ICC_WIN95_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WIN95CTLS_MASK);
		fToRegister &= ~AFX_WIN95CTLS_MASK;
	}
	if (fToRegister & AFX_WNDCOMMCTL_UPDOWN_REG)
	{
		init.dwICC = ICC_UPDOWN_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_UPDOWN_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_TREEVIEW_REG)
	{
		init.dwICC = ICC_TREEVIEW_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_TREEVIEW_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_TAB_REG)
	{
		init.dwICC = ICC_TAB_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_TAB_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_PROGRESS_REG)
	{
		init.dwICC = ICC_PROGRESS_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_PROGRESS_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_LISTVIEW_REG)
	{
		init.dwICC = ICC_LISTVIEW_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_LISTVIEW_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_HOTKEY_REG)
	{
		init.dwICC = ICC_HOTKEY_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_HOTKEY_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_BAR_REG)
	{
		init.dwICC = ICC_BAR_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_BAR_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_ANIMATE_REG)
	{
		init.dwICC = ICC_ANIMATE_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_ANIMATE_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_INTERNET_REG)
	{
		init.dwICC = ICC_INTERNET_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_INTERNET_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_COOL_REG)
	{
		init.dwICC = ICC_COOL_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_COOL_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_USEREX_REG)
	{
		init.dwICC = ICC_USEREX_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_USEREX_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_DATE_REG)
	{
		init.dwICC = ICC_DATE_CLASSES;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_DATE_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_LINK_REG)
	{
		init.dwICC = ICC_LINK_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_LINK_REG);
	}
	if (fToRegister & AFX_WNDCOMMCTL_PAGER_REG)
	{
		init.dwICC = ICC_PAGESCROLLER_CLASS;
		fRegisteredClasses |= _AfxInitCommonControls(&init, AFX_WNDCOMMCTL_PAGER_REG);
	}

	// save new state of registered controls
	pModuleState->m_fRegisteredClasses |= fRegisteredClasses;

	// special case for all common controls registered, turn on AFX_WNDCOMMCTLS_REG
	if ((pModuleState->m_fRegisteredClasses & AFX_WIN95CTLS_MASK) == AFX_WIN95CTLS_MASK)
	{
		pModuleState->m_fRegisteredClasses |= AFX_WNDCOMMCTLS_REG;
		fRegisteredClasses |= AFX_WNDCOMMCTLS_REG;
	}

	// must have registered at least as mamy classes as requested
	return (fToRegister & fRegisteredClasses) == fToRegister;
}

BOOL AFXAPI AfxInitNetworkAddressControl()
{
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	ENSURE(pModuleState);
	if (pModuleState->m_bInitNetworkAddressControlCalled == FALSE)
	{
		OSVERSIONINFO vi;
		ZeroMemory(&vi, sizeof(OSVERSIONINFO));
		vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		BOOL b = ::GetVersionEx(&vi);
		ENSURE(b);

		// if running under Vista
		if (vi.dwMajorVersion >= 6)
		{
			// Only try to GetProcAddress and call InitNetworkAddressControl if we are vista and higher.
			// If we call this on OS lower than Vista, this will throw.  We don't want it to throw.
			pModuleState->m_bInitNetworkAddressControl = 0; // AfxCtxInitNetworkAddressControl();
		}

		pModuleState->m_bInitNetworkAddressControlCalled = TRUE;
	}
	return pModuleState->m_bInitNetworkAddressControl;
}

/////////////////////////////////////////////////////////////////////////////
// Extra CWnd support for dynamic subclassing of controls

BOOL CWnd::SubclassWindow(HWND hWnd)
{
	if (!Attach(hWnd))
		return FALSE;

	// allow any other subclassing to occur
	PreSubclassWindow();

	// now hook into the AFX WndProc
	WNDPROC* lplpfn = GetSuperWndProcAddr();
	WNDPROC oldWndProc = (WNDPROC)::SetWindowLongPtr(hWnd, GWLP_WNDPROC,
		(INT_PTR)AfxGetAfxWndProc());
	ASSERT(oldWndProc != AfxGetAfxWndProc());

	if (*lplpfn == NULL)
		*lplpfn = oldWndProc;   // the first control of that type created
#ifdef _DEBUG
	else if (*lplpfn != oldWndProc)
	{
		ASSERT(FALSE);
		// undo the subclassing if continuing after assert
		::SetWindowLongPtr(hWnd, GWLP_WNDPROC, (INT_PTR)oldWndProc);
	}
#endif

	return TRUE;
}

BOOL CWnd::SubclassDlgItem(UINT nID, CWnd* pParent)
{
	ASSERT(pParent != NULL);
	// ASSERT(::IsWindow(pParent->m_hWnd));

	// check for normal dialog control first
	// HWND hWndControl = ::GetDlgItem(pParent->m_hWnd, nID);
	// if (hWndControl != NULL)
		// return SubclassWindow(hWndControl);

#ifndef _AFX_NO_OCC_SUPPORT
	if (pParent->m_pCtrlCont != NULL)
	{
		// normal dialog control not found
		COleControlSite* pSite = pParent->m_pCtrlCont->FindItem(nID);
		if (pSite != NULL)
		{
			// ASSERT(pSite->m_hWnd != NULL);
			// VERIFY(SubclassWindow(pSite->m_hWnd));

#ifndef _AFX_NO_OCC_SUPPORT
			// If the control has reparented itself (e.g., invisible control),
			// make sure that the CWnd gets properly wired to its control site.
			// if (pParent->m_hWnd != ::GetParent(pSite->m_hWnd))
				// AttachControlSite(pParent);
#endif //!_AFX_NO_OCC_SUPPORT

			return TRUE;
		}
	}
#endif

	return FALSE;   // control not found
}

HWND CWnd::UnsubclassWindow()
{
	// ASSERT(::IsWindow(m_hWnd));

	// set WNDPROC back to original value
	WNDPROC* lplpfn = GetSuperWndProcAddr();
	// SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (INT_PTR)*lplpfn);
	*lplpfn = NULL;

	// and Detach the HWND from the CWnd object
	return Detach();
}

/////////////////////////////////////////////////////////////////////////////


IMPLEMENT_DYNCREATE(CWnd, CCmdTarget)

/////////////////////////////////////////////////////////////////////////////
