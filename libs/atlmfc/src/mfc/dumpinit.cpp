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

#ifdef _DEBUG   // entire file


AFX_DATADEF BOOL afxTraceEnabled = TRUE;
AFX_DATADEF UINT afxTraceFlags = 0;
static BOOL _afxDiagnosticInit = AfxDiagnosticInit();
static BOOL _afxMemoryLeakDump = TRUE;

/////////////////////////////////////////////////////////////////////////////
// _AFX_DEBUG_STATE implementation

#ifndef _AFX_NO_DEBUG_CRT
static _CRT_DUMP_CLIENT pfnOldCrtDumpClient = NULL;


void __cdecl _AfxCrtDumpClient(void * pvData, size_t nBytes)
{
	return;
}

int __cdecl _AfxCrtReportHook(int nRptType, _In_ char *szMsg, int* pResult)
{
	return FALSE;
}
#endif // _AFX_NO_DEBUG_CRT

_AFX_DEBUG_STATE::_AFX_DEBUG_STATE()
{
#ifndef _AFX_NO_DEBUG_CRT
	ASSERT(pfnOldCrtDumpClient == NULL);
	pfnOldCrtDumpClient = _CrtSetDumpClient(_AfxCrtDumpClient);

	ASSERT(_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL,_AfxCrtReportHook) != -1);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
#endif // _AFX_NO_DEBUG_CRT
}

_AFX_DEBUG_STATE::~_AFX_DEBUG_STATE()
{
#ifndef _AFX_NO_DEBUG_CRT
	if (_afxMemoryLeakDump)
		_CrtDumpMemoryLeaks();
	int nOldState = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	_CrtSetDbgFlag(nOldState & ~_CRTDBG_LEAK_CHECK_DF);

	ASSERT(_CrtSetReportHook2(_CRT_RPTHOOK_REMOVE,_AfxCrtReportHook) != -1);	
	_CrtSetDumpClient(pfnOldCrtDumpClient);
#endif // _AFX_NO_DEBUG_CRT
}

#pragma warning(disable: 4074)
#pragma init_seg(lib)

#if !defined(__MINGW32__)
PROCESS_LOCAL(_AFX_DEBUG_STATE, afxDebugState)
#else
CProcessLocal<_AFX_DEBUG_STATE> afxDebugState;
#endif

BOOL AFXAPI AfxDiagnosticInit(void)
{
	// just get the debug state to cause initialization
	_AFX_DEBUG_STATE* pState = afxDebugState.GetData();
	ASSERT(pState != NULL);

	return TRUE;
}

BOOL AFXAPI AfxEnableMemoryLeakDump(BOOL bDump)
{
	// get the old flag state
	BOOL bRet = _afxMemoryLeakDump;

	// set the flag to the desired state
	_afxMemoryLeakDump = bDump;

	return bRet;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
