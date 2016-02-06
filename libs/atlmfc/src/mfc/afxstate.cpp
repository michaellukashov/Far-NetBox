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
#include <stddef.h>



#pragma warning(disable: 4074)
#pragma init_seg(compiler)

/////////////////////////////////////////////////////////////////////////////
// AFX_MODULE_STATE push/pop implementation

#ifdef _AFXDLL
AFX_MODULE_STATE* AFXAPI AfxSetModuleState(AFX_MODULE_STATE* pNewState) throw()
{
	_AFX_THREAD_STATE* pState = NULL;

	TRY
	{
		pState = _afxThreadState.GetData();
	}
	CATCH(CInvalidArgException, pInvalidArgException)
	{
		DELETE_EXCEPTION(pInvalidArgException);
		pState = NULL;
	}
	END_CATCH

	ASSERT(pState);
	if(pState)
	{
		AFX_MODULE_STATE* pPrevState = pState->m_pModuleState;
		pState->m_pModuleState = pNewState;
		return pPrevState;
	}
	else
	{
		return NULL;
	}
}

// AFX_MAINTAIN_STATE functions

AFX_MAINTAIN_STATE::AFX_MAINTAIN_STATE(AFX_MODULE_STATE* pNewState) throw()
{
	m_pPrevModuleState = AfxSetModuleState(pNewState);
}

AFX_MAINTAIN_STATE::~AFX_MAINTAIN_STATE()
{
	AFX_BEGIN_DESTRUCTOR

	_AFX_THREAD_STATE* pState = _afxThreadState;
	ASSERT(pState);
	if(pState)
	{
		pState->m_pModuleState = m_pPrevModuleState;
	}

	AFX_END_DESTRUCTOR
}
#endif //_AFXDLL

// AFX_MAINTAIN_STATE2 functions

AFX_MAINTAIN_STATE2::AFX_MAINTAIN_STATE2(AFX_MODULE_STATE* pNewState) throw()
{
#ifdef _AFXDLL
	m_pThreadState = _afxThreadState.GetData();
	ASSERT(m_pThreadState);

	if(m_pThreadState)
	{
		m_pPrevModuleState = m_pThreadState->m_pModuleState;
		m_pThreadState->m_pModuleState = pNewState;
	}
	else
	{
		// This is a very bad state; we have no good way to report the error at this moment
		// since exceptions from here are not expected
		m_pPrevModuleState=NULL;
		m_pThreadState=NULL;
	}
#endif

#if 0
	if (AfxGetAmbientActCtx() &&
		pNewState->m_hActCtx != INVALID_HANDLE_VALUE)
	{
		m_bValidActCtxCookie = ActivateActCtx(pNewState->m_hActCtx, &m_ulActCtxCookie);
	}
	else
#endif
	{
		m_bValidActCtxCookie = FALSE;
	}
}

AFX_MAINTAIN_STATE2::~AFX_MAINTAIN_STATE2()
{
#ifdef _AFXDLL
	// Not a good place to report errors here, so just be safe
	if(m_pThreadState)
	{
		m_pThreadState->m_pModuleState = m_pPrevModuleState;
	}
#endif

#if 0
	if (m_bValidActCtxCookie)
	{
		BOOL bRet;
		bRet = DeactivateActCtx(0, m_ulActCtxCookie);
		ASSERT(bRet == TRUE);
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////
// _AFX_THREAD_STATE implementation

_AFX_THREAD_STATE::_AFX_THREAD_STATE()
{
#ifdef _DEBUG
	m_nDisablePumpCount = 0;
#endif
	m_msgCur.message = WM_NULL;
	m_nMsgLast = WM_NULL;
	//::GetCursorPos(&(m_ptCursorLast));
}

_AFX_THREAD_STATE::~_AFX_THREAD_STATE()
{
	// unhook windows hooks
	if (m_hHookOldMsgFilter != NULL)
		::UnhookWindowsHookEx(m_hHookOldMsgFilter);
	if (m_hHookOldCbtFilter != NULL)
		::UnhookWindowsHookEx(m_hHookOldCbtFilter);

	// free safety pool buffer
	if (m_pSafetyPoolBuffer != NULL)
		nb_free(m_pSafetyPoolBuffer);

	// parking window must have already been cleaned up by now!
	ASSERT(m_pWndPark == NULL);
}

_AFX_THREAD_STATE* AFXAPI AfxGetThreadState()
{
	_AFX_THREAD_STATE *pState =_afxThreadState.GetData();
	ENSURE(pState != NULL);
	return pState;
}

#if !defined(__MINGW32__)
THREAD_LOCAL(_AFX_THREAD_STATE, _afxThreadState)
#else
CThreadLocal<_AFX_THREAD_STATE> _afxThreadState;
#endif

/////////////////////////////////////////////////////////////////////////////
// AFX_MODULE_STATE implementation

#ifdef _AFXDLL
AFX_MODULE_STATE::AFX_MODULE_STATE(BOOL bDLL, WNDPROC pfnAfxWndProc,
	DWORD dwVersion, BOOL bSystem)
#else
AFX_MODULE_STATE::AFX_MODULE_STATE(BOOL bDLL)
#endif
{
#ifndef _AFX_NO_OLE_SUPPORT
	m_factoryList.Construct(offsetof(COleObjectFactory, m_pNextFactory));
#endif
	m_classList.Construct(offsetof(CRuntimeClass, m_pNextClass));

	m_fRegisteredClasses = 0;
	m_bDLL = (BYTE)bDLL;
#ifdef _AFXDLL
	m_pfnAfxWndProc = pfnAfxWndProc;
	m_dwVersion = dwVersion;
	m_bSystem = (BYTE)bSystem;
#endif
	BOOL bEnable = TRUE;
	TRY
	{
		//Preallocate the registered classes string, but CRT memory leak report is
		//called before the string frees memory, so need to disable tracking.
		bEnable = AfxEnableMemoryTracking(FALSE);
		m_strUnregisterList.Preallocate(4096);
		AfxEnableMemoryTracking(bEnable);
	}
	CATCH(CMemoryException, e)
	{
		AfxEnableMemoryTracking(bEnable);
		DELETE_EXCEPTION(e);
	}
	END_CATCH
	// app starts out in "user control"
	m_bUserCtrl = TRUE;

#ifndef _AFX_NO_OCC_SUPPORT
	m_lockList.Construct(offsetof(COleControlLock, m_pNextLock));
#endif
#ifdef _AFXDLL
	m_libraryList.Construct(offsetof(CDynLinkLibrary, m_pNextDLL));
#endif


	bEnable = AfxEnableMemoryTracking(FALSE);
	//Fusion: allocate dll wrappers array.
	m_pDllIsolationWrappers = new CDllIsolationWrapperBase*[_AFX_ISOLATION_WRAPPER_ARRAY_SIZE];
#ifndef _AFX_NO_AFXCMN_SUPPORT
	m_pDllIsolationWrappers[_AFX_COMCTL32_ISOLATION_WRAPPER_INDEX] = new CComCtlWrapper;
#endif
	m_pDllIsolationWrappers[_AFX_SHELL_ISOLATION_WRAPPER_INDEX] = new CShellWrapper;
	AfxEnableMemoryTracking(bEnable);
	m_bSetAmbientActCtx = TRUE;
//	m_hActCtx = NULL;
	m_bInitNetworkAddressControl = FALSE;
	m_bInitNetworkAddressControlCalled = FALSE;
}

#if 0
HANDLE AFXAPI AfxCreateActCtxW(PCACTCTXW pActCtx)
{
	HANDLE hCtx = CreateActCtxW(pActCtx);
	return hCtx;
}

void AFXAPI AfxReleaseActCtx(HANDLE hActCtx)
{
	ReleaseActCtx(hActCtx);
}

BOOL AFXAPI AfxActivateActCtx(HANDLE hActCtx, ULONG_PTR *lpCookie)
{
	BOOL rc = ActivateActCtx(hActCtx, lpCookie);
	return rc;
}

BOOL AFXAPI AfxDeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie)
{
	BOOL rc = DeactivateActCtx(dwFlags, ulCookie);
	return rc;
}

eActCtxResult AFXAPI AfxActivateActCtxWrapper(HANDLE hActCtx, ULONG_PTR *lpCookie)
{
	ENSURE_ARG(lpCookie!=NULL);

	eActCtxResult eResult = ActivateActCtx(hActCtx, lpCookie) ? ActCtxSucceeded : ActCtxFailed;

	return eResult;
}

void AFX_MODULE_STATE::CreateActivationContext()
{
	HMODULE hModule = m_hCurrentInstanceHandle;

	WCHAR rgchFullModulePath[MAX_PATH + 2];
	rgchFullModulePath[_countof(rgchFullModulePath) - 1] = 0;
	rgchFullModulePath[_countof(rgchFullModulePath) - 2] = 0;
	DWORD dw = GetModuleFileNameW(hModule, rgchFullModulePath, _countof(rgchFullModulePath)-1);
	if (dw == 0)
	{
		return;
	}
	if (rgchFullModulePath[_countof(rgchFullModulePath) - 2] != 0)
	{
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return;
	}
	//First try ID 2 and then ID 1 - this is to consider also a.dll.manifest file
	//for dlls, which ID 2 ignores.
#if 0
	ACTCTXW actCtx;
	actCtx.cbSize = sizeof(actCtx);
	actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
	actCtx.lpSource = rgchFullModulePath;
	actCtx.lpResourceName =  MAKEINTRESOURCEW(ISOLATIONAWARE_MANIFEST_RESOURCE_ID);
	actCtx.hModule = hModule;
	m_hActCtx = CreateActCtxW(&actCtx);
	if (m_hActCtx == INVALID_HANDLE_VALUE)
	{
		actCtx.lpResourceName =  MAKEINTRESOURCEW(ISOLATIONAWARE_NOSTATICIMPORT_MANIFEST_RESOURCE_ID);
		m_hActCtx = CreateActCtxW(&actCtx);
	}
	if (m_hActCtx == INVALID_HANDLE_VALUE)
	{
		actCtx.lpResourceName =  MAKEINTRESOURCEW(CREATEPROCESS_MANIFEST_RESOURCE_ID);
		m_hActCtx = CreateActCtxW(&actCtx);
	}
	if (m_hActCtx == INVALID_HANDLE_VALUE)
	{
		m_hActCtx = NULL;
	}
#endif
}
#endif

AFX_MODULE_STATE::~AFX_MODULE_STATE()
{
#ifndef _AFX_NO_DAO_SUPPORT
	delete m_pDaoState;
#endif

	// clean up type lib cache map, if any
	/*if (m_pTypeLibCacheMap != NULL)
	{
		m_pTypeLibCacheMap->RemoveAll(&m_typeLibCache);
		delete m_pTypeLibCacheMap;
	}*/
	//Fusion: delete each member of the array and the array itself
#ifndef _AFX_NO_AFXCMN_SUPPORT
	delete m_pDllIsolationWrappers[_AFX_COMCTL32_ISOLATION_WRAPPER_INDEX];
#endif
	delete m_pDllIsolationWrappers[_AFX_SHELL_ISOLATION_WRAPPER_INDEX];
	delete [] m_pDllIsolationWrappers;
#if 0
	if (m_hActCtx != NULL && m_hActCtx != INVALID_HANDLE_VALUE)
	{
		ReleaseActCtx(m_hActCtx);
		m_hActCtx = INVALID_HANDLE_VALUE;
	}
#endif
}

AFX_MODULE_THREAD_STATE::AFX_MODULE_THREAD_STATE()
{
	m_nLastHit = static_cast<INT_PTR>(-1);
	m_nLastStatus = static_cast<INT_PTR>(-1);

	// Note: it is only necessary to initialize non-zero data
	m_pfnNewHandler = &AfxNewHandler;
}

AFX_MODULE_THREAD_STATE::~AFX_MODULE_THREAD_STATE()
{
	// cleanup thread local tooltip window
	/*if (m_pToolTip != NULL)
		m_pToolTip->DestroyToolTipCtrl();*/

	// delete m_pLastInfo;

	// cleanup temp/permanent maps (just the maps themselves)
	// delete m_pmapHWND;
	// delete m_pmapHMENU;
	// delete m_pmapHDC;
	// delete m_pmapHGDIOBJ;
	// delete m_pmapHIMAGELIST;

#ifndef _AFX_NO_SOCKET_SUPPORT
	// cleanup socket notification list
	// if (m_plistSocketNotifications != NULL)
		// while (!m_plistSocketNotifications->IsEmpty())
			// delete m_plistSocketNotifications->RemoveHead();
#ifndef _AFXDLL
	// cleanup dynamically allocated socket maps
	// delete m_pmapSocketHandle;
	// delete m_pmapDeadSockets;
	// delete m_plistSocketNotifications;
#endif
#endif //!_AFX_NO_SOCKET_SUPPORT
}

/////////////////////////////////////////////////////////////////////////////
// AFX_MODULE_STATE for base application

LRESULT CALLBACK AfxWndProcBase(HWND, UINT, WPARAM, LPARAM);

class _AFX_BASE_MODULE_STATE : public AFX_MODULE_STATE
{
public:
#ifdef _AFXDLL
	_AFX_BASE_MODULE_STATE() : AFX_MODULE_STATE(TRUE, AfxWndProcBase, _MFC_VER)
#else
	_AFX_BASE_MODULE_STATE() : AFX_MODULE_STATE(TRUE)
#endif
		{ }
};

#if !defined(__MINGW32__)
PROCESS_LOCAL(_AFX_BASE_MODULE_STATE, _afxBaseModuleState)
#else
CProcessLocal<_AFX_BASE_MODULE_STATE> _afxBaseModuleState;
#endif

#ifdef _AFXDLL

#undef AfxWndProc
LRESULT CALLBACK
AfxWndProcBase(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	AFX_MANAGE_STATE(_afxBaseModuleState.GetData());
	return AfxWndProc(hWnd, nMsg, wParam, lParam);
}

#endif

/////////////////////////////////////////////////////////////////////////////
// helper functions for module state

AFX_MODULE_STATE* AFXAPI AfxGetAppModuleState()
{
	return _afxBaseModuleState.GetData();
}

AFX_MODULE_STATE* AFXAPI AfxGetModuleState()
{
	_AFX_THREAD_STATE* pState = _afxThreadState;
	ENSURE(pState);
	AFX_MODULE_STATE* pResult;
	if (pState->m_pModuleState != NULL)
	{
		// thread state's module state serves as override
		pResult = pState->m_pModuleState;
	}
	else
	{
		// otherwise, use global app state
		pResult = _afxBaseModuleState.GetData();
	}
	ENSURE(pResult != NULL);
	return pResult;
}

HINSTANCE AFXAPI AfxGetInstanceHandleHelper()
{
	return AfxGetModuleState()->m_hCurrentInstanceHandle;
}

BOOL AFXAPI AfxIsModuleDll()
{
	return AfxGetModuleState()->m_bDLL;
}

BOOL AFXAPI AfxInitCurrentStateApp()
{
	return TRUE;
}

AFX_MODULE_THREAD_STATE* AFXAPI AfxGetModuleThreadState()
{
	AFX_MODULE_THREAD_STATE* pResult=AfxGetModuleState()->m_thread.GetData();
	ENSURE(pResult != NULL);
	return pResult;
}

/////////////////////////////////////////////////////////////////////////////
// CTypeLibCache::Unlock
// (Note: the rest of CTypeLibCache is implemented in oletyplb.cpp)

/*void CTypeLibCache::Unlock()
{
	ASSERT(m_cRef > 0);

	if (InterlockedDecrement(&m_cRef) == 0)
	{
		if (m_ptinfo != NULL)
		{
			m_ptinfo->Release();
			m_ptinfo = NULL;
		}
		if (m_ptlib != NULL)
		{
			m_ptlib->Release();
			m_ptlib = NULL;
		}
	}
}*/


/////////////////////////////////////////////////////////////////////////////
// Registry Redirection Flag

AFX_STATIC_DATA BOOL _afxRedirectRegistration = FALSE;

void AFXAPI AfxSetPerUserRegistration(BOOL bEnable)
{
	_afxRedirectRegistration = bEnable;
}

BOOL AFXAPI AfxGetPerUserRegistration(void)
{
	return _afxRedirectRegistration;
}

/////////////////////////////////////////////////////////////////////////////
