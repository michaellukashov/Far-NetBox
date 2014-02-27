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
#include <afxtempl.h>
#include <wchar.h>
#include "sal.h"



#define new DEBUG_NEW

////////////////////////////////////////////////////////////////////////////
// Serialize member functions for low level classes put here
// for code swapping improvements

#ifdef _AFX_BYTESWAP
#error _AFX_BYTESWAP is not supported.
#endif

// Runtime class serialization code
CObject* PASCAL CRuntimeClass::CreateObject(LPCSTR lpszClassName)
{
	ENSURE(lpszClassName);

	// attempt to find matching runtime class structure
	CRuntimeClass* pClass = FromName(lpszClassName);
	if (pClass == NULL)
	{
		// not found, trace a warning for diagnostic purposes
		return NULL;
	}

	// attempt to create the object with the found CRuntimeClass
	CObject* pObject = pClass->CreateObject();
	return pObject;
}

CObject* PASCAL CRuntimeClass::CreateObject(LPCWSTR lpszClassName)
{
	const CStringA strClassName(lpszClassName);
	return CRuntimeClass::CreateObject(lpszClassName ? strClassName.GetString() : NULL);
}

CRuntimeClass* PASCAL CRuntimeClass::FromName(LPCSTR lpszClassName)
{
	CRuntimeClass* pClass=NULL;

	ENSURE(lpszClassName);

	// search app specific classes
	AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
	AfxLockGlobals(CRIT_RUNTIMECLASSLIST);
	for (pClass = pModuleState->m_classList; pClass != NULL;
		pClass = pClass->m_pNextClass)
	{
		if (lstrcmpA(lpszClassName, pClass->m_lpszClassName) == 0)
		{
			AfxUnlockGlobals(CRIT_RUNTIMECLASSLIST);
			return pClass;
		}
	}
	AfxUnlockGlobals(CRIT_RUNTIMECLASSLIST);
#ifdef _AFXDLL
	// search classes in shared DLLs
	AfxLockGlobals(CRIT_DYNLINKLIST);
	for (CDynLinkLibrary* pDLL = pModuleState->m_libraryList; pDLL != NULL;
		pDLL = pDLL->m_pNextDLL)
	{
		for (pClass = pDLL->m_classList; pClass != NULL;
			pClass = pClass->m_pNextClass)
		{
			if (lstrcmpA(lpszClassName, pClass->m_lpszClassName) == 0)
			{
				AfxUnlockGlobals(CRIT_DYNLINKLIST);
				return pClass;
			}
		}
	}
	AfxUnlockGlobals(CRIT_DYNLINKLIST);
#endif

	return NULL; // not found
}

CRuntimeClass* PASCAL CRuntimeClass::FromName(LPCWSTR lpszClassName)
{
	const CStringA strClassName(lpszClassName);
	if( lpszClassName == NULL )
		return NULL;
	return CRuntimeClass::FromName( strClassName.GetString() );
}

/////////////////////////////////////////////////////////////////////////////
